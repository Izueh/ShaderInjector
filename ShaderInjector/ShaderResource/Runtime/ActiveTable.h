#pragma once

#include <d3d12.h>

#include "RenderPass/Registry/DescriptorContentVersion.h"
#include "ShaderResource/Runtime/DescriptorAllocation.h"

namespace ShaderResourceRuntime
{
	//record where a copied game table lives in the injector's descriptor heap.
	struct ActiveTable
	{
		UINT rootParameterIndex = UINT32_MAX;
		D3D12_DESCRIPTOR_HEAP_TYPE heapType = D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;
		UINT descriptorCount = 0;
		UINT sourceDescriptorCount = 0;
		D3D12_GPU_DESCRIPTOR_HANDLE originalGpu{};
		D3D12_CPU_DESCRIPTOR_HANDLE originalCpu{};
		DescriptorAllocation allocation;
		RenderPassResourceRegistry::DescriptorContentVersion sourceVersion;
		bool sourceVersionKnown = false;
		bool hasOverrides = false;
		bool needsCopy = false;
		bool canReuseBinding = false;
	};
} //namespace ShaderResourceRuntime
