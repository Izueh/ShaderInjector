#pragma once

#include <vector>

#include <d3d12.h>

//A reserved tail of descriptors appended to the game's shader-visible CBV/SRV/UAV heaps when they are
//created, so injector-owned tables can be bound while the game's heap and tables stay bound as-is.
//GetDesc keeps reporting the size the game requested. Tables are immutable once written, so the
//reserve suits static descriptors such as imported textures; callers decide when a heap needs one.
namespace ShaderResourceRuntime
{
	//extra descriptors to append to a heap being created; 0 when the heap type or size cannot take a reserve.
	UINT GetReservedDescriptorCount(ID3D12Device* device, const D3D12_DESCRIPTOR_HEAP_DESC& description);
	//record a heap created with `reserve` extra descriptors; false if its GetDesc cannot be hooked.
	bool RegisterReservedDescriptorHeap(ID3D12Device* device, IUnknown* heapObject, const D3D12_DESCRIPTOR_HEAP_DESC& description, UINT reserve);
	//a heap created without a tail may reuse the address of a released one.
	void UnregisterReservedDescriptorHeap(IUnknown* heapObject);
	//a table in this heap's reserve holding these descriptors in order, written the first time it is requested.
	bool GetOrCreateReservedTable(ID3D12DescriptorHeap* heap, const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& descriptors, D3D12_GPU_DESCRIPTOR_HANDLE& outTable);
} //namespace ShaderResourceRuntime
