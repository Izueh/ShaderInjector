#include "ShaderResource/Runtime/DescriptorHeapReserve.h"

#include <algorithm>
#include <atomic>
#include <shared_mutex>
#include <unordered_map>
#include <utility>

#include "MinHook.h"

#include "HookD3D12/HookD3D12.h"
#include "IO/ShaderInjectorIO.h"
#include "StringHelper.h"

namespace ShaderResourceRuntime
{
	namespace
	{
		//enough for many tables of a few descriptors each; tables are immutable once written.
		constexpr UINT tailDescriptors = 1024;
		//shader-visible CBV/SRV/UAV heap limit below resource binding tier 3.
		constexpr UINT tierTwoHeapLimit = 1000000;
		constexpr size_t getDescriptorHeapDescIndex = 8;

		struct Table
		{
			std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> sources;
			D3D12_GPU_DESCRIPTOR_HANDLE table{};
		};

		struct Tail
		{
			ID3D12Device* device = nullptr;
			UINT requestedDescriptors = 0;
			UINT reserve = 0;
			UINT used = 0;
			UINT increment = 0;
			D3D12_CPU_DESCRIPTOR_HANDLE cpuStart{};
			D3D12_GPU_DESCRIPTOR_HANDLE gpuStart{};
			std::vector<Table> tables;
			bool exhaustedReported = false;
		};

		std::shared_mutex gMutex;
		std::unordered_map<ID3D12DescriptorHeap*, Tail> gTails;
		std::atomic<size_t> gTailCount{0};

		//GetDesc returns the description through a hidden pointer, so the hook takes it explicitly.
		using GetDescFunction = D3D12_DESCRIPTOR_HEAP_DESC*(STDMETHODCALLTYPE*)(ID3D12DescriptorHeap*, D3D12_DESCRIPTOR_HEAP_DESC*);
		GetDescFunction gOriginalGetDesc = nullptr;
		void* gHookedGetDesc = nullptr;

		D3D12_DESCRIPTOR_HEAP_DESC* STDMETHODCALLTYPE HookGetDesc(ID3D12DescriptorHeap* heap, D3D12_DESCRIPTOR_HEAP_DESC* result)
		{
			gOriginalGetDesc(heap, result);
			if (gTailCount.load(std::memory_order_acquire))
			{
				std::shared_lock<std::shared_mutex> lock(gMutex);
				const auto tail = gTails.find(heap);
				if (tail != gTails.end())
					result->NumDescriptors = tail->second.requestedDescriptors;
			}
			return result;
		}

		bool HookGetDescFor(ID3D12DescriptorHeap* heap)
		{
			void* const target = (*reinterpret_cast<void***>(heap))[getDescriptorHeapDescIndex];
			if (gHookedGetDesc)
				return gHookedGetDesc == target;

			const MH_STATUS created = MH_CreateHook(target, reinterpret_cast<void*>(&HookGetDesc), reinterpret_cast<void**>(&gOriginalGetDesc));
			if (created != MH_OK || MH_EnableHook(target) != MH_OK)
			{
				ShaderInjectorIO::WriteToLogFileError("DescriptorHeapReserve: could not hook ID3D12DescriptorHeap::GetDesc; reserved tables are unavailable");
				return false;
			}
			gHookedGetDesc = target;
			return true;
		}

		ID3D12DescriptorHeap* AsHeap(IUnknown* heapObject)
		{
			ID3D12DescriptorHeap* heap = nullptr;
			if (!heapObject || FAILED(heapObject->QueryInterface(IID_PPV_ARGS(&heap))) || !heap)
				return nullptr;
			heap->Release();
			return heap;
		}
	} //namespace

	UINT GetReservedDescriptorCount(ID3D12Device* device, const D3D12_DESCRIPTOR_HEAP_DESC& description)
	{
		if (!device || description.Type != D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV ||
			!(description.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE) ||
			description.NumDescriptors > UINT_MAX - tailDescriptors)
		{
			return 0;
		}

		D3D12_FEATURE_DATA_D3D12_OPTIONS options{};
		const bool tierThree = SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &options, sizeof(options))) &&
							   options.ResourceBindingTier >= D3D12_RESOURCE_BINDING_TIER_3;
		if (!tierThree && description.NumDescriptors + tailDescriptors > tierTwoHeapLimit)
		{
			ShaderInjectorIO::WriteToLogFileWarning(StringHelper::Format("DescriptorHeapReserve: a %u-descriptor heap has no room for a reserve", description.NumDescriptors));
			return 0;
		}

		return tailDescriptors;
	}

	bool RegisterReservedDescriptorHeap(ID3D12Device* device, IUnknown* heapObject, const D3D12_DESCRIPTOR_HEAP_DESC& description, UINT reserve)
	{
		ID3D12DescriptorHeap* heap = AsHeap(heapObject);
		if (!device || !heap)
			return false;

		std::unique_lock<std::shared_mutex> lock(gMutex);
		if (!HookGetDescFor(heap))
			return false;

		Tail tail;
		tail.device = device;
		tail.requestedDescriptors = description.NumDescriptors;
		tail.reserve = reserve;
		tail.increment = device->GetDescriptorHandleIncrementSize(description.Type);
		tail.cpuStart = heap->GetCPUDescriptorHandleForHeapStart();
		tail.gpuStart = heap->GetGPUDescriptorHandleForHeapStart();
		gTails[heap] = std::move(tail);
		gTailCount.store(gTails.size(), std::memory_order_release);
		return true;
	}

	void UnregisterReservedDescriptorHeap(IUnknown* heapObject)
	{
		if (!gTailCount.load(std::memory_order_acquire))
			return;

		ID3D12DescriptorHeap* heap = AsHeap(heapObject);
		std::unique_lock<std::shared_mutex> lock(gMutex);
		if (heap && gTails.erase(heap))
			gTailCount.store(gTails.size(), std::memory_order_release);
	}

	bool GetOrCreateReservedTable(ID3D12DescriptorHeap* heap, const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& descriptors, D3D12_GPU_DESCRIPTOR_HANDLE& outTable)
	{
		if (!heap || descriptors.empty() || !gTailCount.load(std::memory_order_acquire))
			return false;

		const auto findTable = [&](const Tail& tail)
		{
			return std::find_if(tail.tables.begin(), tail.tables.end(), [&](const Table& table)
								{ return std::equal(table.sources.begin(), table.sources.end(), descriptors.begin(), descriptors.end(),
													[](D3D12_CPU_DESCRIPTOR_HANDLE left, D3D12_CPU_DESCRIPTOR_HANDLE right)
													{ return left.ptr == right.ptr; }); });
		};

		{
			std::shared_lock<std::shared_mutex> lock(gMutex);
			const auto tail = gTails.find(heap);
			if (tail == gTails.end())
				return false;
			const auto table = findTable(tail->second);
			if (table != tail->second.tables.end())
			{
				outTable = table->table;
				return true;
			}
		}

		std::unique_lock<std::shared_mutex> lock(gMutex);
		const auto tailIt = gTails.find(heap);
		if (tailIt == gTails.end())
			return false;
		Tail& tail = tailIt->second;
		const auto existing = findTable(tail);
		if (existing != tail.tables.end())
		{
			outTable = existing->table;
			return true;
		}

		const UINT count = static_cast<UINT>(descriptors.size());
		if (count > tail.reserve - tail.used)
		{
			if (!tail.exhaustedReported)
				ShaderInjectorIO::WriteToLogFileWarning("DescriptorHeapReserve: a heap's descriptor reserve is full; further reserved tables in it are refused");
			tail.exhaustedReported = true;
			return false;
		}

		//slots past the game's requested range are never written by the game and never reused, so writing them
		//here cannot race a table the GPU is reading.
		const UINT offset = tail.requestedDescriptors + tail.used;
		D3D12_CPU_DESCRIPTOR_HANDLE destination = tail.cpuStart;
		destination.ptr += static_cast<SIZE_T>(offset) * tail.increment;
		std::vector<UINT> sourceSizes(count, 1);
		{
			HookD3D12::ScopedRenderPassInjection injectionScope;
			tail.device->CopyDescriptors(1, &destination, &count, count, descriptors.data(), sourceSizes.data(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}

		Table table;
		table.sources = descriptors;
		table.table = tail.gpuStart;
		table.table.ptr += static_cast<UINT64>(offset) * tail.increment;
		tail.used += count;
		tail.tables.push_back(std::move(table));
		outTable = tail.tables.back().table;
		return true;
	}
} //namespace ShaderResourceRuntime
