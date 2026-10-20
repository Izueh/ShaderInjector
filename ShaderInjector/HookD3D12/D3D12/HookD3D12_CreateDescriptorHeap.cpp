
#include <array>

#include "../HookD3D12.h"
#include "RenderPass/RenderPassResourceRegistry.h"
#include "RenderPass/RenderPassRuntime.h"
#include "ShaderPatch/ShaderPatchRuntime.h"
#include "ShaderResource/Runtime/DescriptorHeapReserve.h"

namespace HookD3D12
{
	HRESULT STDMETHODCALLTYPE Hook_CreateDescriptorHeap(ID3D12Device* device, const D3D12_DESCRIPTOR_HEAP_DESC* description, REFIID interfaceId, void** descriptorHeap)
	{
		return Handle_CreateDescriptorHeap(device, description, interfaceId, descriptorHeap);
	}

	HRESULT STDMETHODCALLTYPE Handle_CreateDescriptorHeap(ID3D12Device* device, const D3D12_DESCRIPTOR_HEAP_DESC* description, REFIID interfaceId, void** descriptorHeapObject)
	{
		//shader-visible heaps get a descriptor reserve while a consumer needs one: texture shader patches keep
		//their added tables there.
		const UINT reserve = device && description && descriptorHeapObject && !IsInsideRenderPassInjection() &&
								 ShaderPatchRuntime::HasResourceRecipes()
								 ? ShaderResourceRuntime::GetReservedDescriptorCount(device, *description)
								 : 0;
		HRESULT result = E_FAIL;

		if (reserve)
		{
			D3D12_DESCRIPTOR_HEAP_DESC expandedDescription = *description;
			expandedDescription.NumDescriptors += reserve;
			result = Original_CreateDescriptorHeap(device, &expandedDescription, interfaceId, descriptorHeapObject);
			if (SUCCEEDED(result) && !ShaderResourceRuntime::RegisterReservedDescriptorHeap(device, reinterpret_cast<IUnknown*>(*descriptorHeapObject), *description, reserve))
			{
				reinterpret_cast<IUnknown*>(*descriptorHeapObject)->Release();
				*descriptorHeapObject = nullptr;
				result = E_FAIL;
			}
		}

		if (FAILED(result))
		{
			result = Original_CreateDescriptorHeap(device, description, interfaceId, descriptorHeapObject);
			if (SUCCEEDED(result) && descriptorHeapObject && *descriptorHeapObject)
				ShaderResourceRuntime::UnregisterReservedDescriptorHeap(reinterpret_cast<IUnknown*>(*descriptorHeapObject));
		}

		if (FAILED(result) ||
			!device ||
			!description ||
			!descriptorHeapObject ||
			!*descriptorHeapObject ||
			IsInsideRenderPassInjection() ||
			RenderPassResourceRegistry::IsInsideDescriptorMirrorOperation() ||
			(!ShouldTrackRenderPassDescriptorMetadata() && !RenderPassResourceRegistry::IsDescriptorMirroringActive()))
		{
			return result;
		}

		ID3D12DescriptorHeap* descriptorHeap = nullptr;
		IUnknown* unknown = reinterpret_cast<IUnknown*>(*descriptorHeapObject);

		if (SUCCEEDED(unknown->QueryInterface(IID_PPV_ARGS(&descriptorHeap))) && descriptorHeap)
		{
			const UINT descriptorIncrement = device->GetDescriptorHandleIncrementSize(description->Type);
			RenderPassResourceRegistry::RegisterDescriptorHeap(descriptorHeap, descriptorIncrement, true);
			descriptorHeap->Release();
		}

		return result;
	}
} //namespace HookD3D12
