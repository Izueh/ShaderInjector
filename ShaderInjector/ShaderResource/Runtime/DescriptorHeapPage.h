#pragma once

#include <d3d12.h>
#include <wrl/client.h>

namespace ShaderResourceRuntime
{
	//allocate descriptor copies from pages so a command list can grow in chunks.
	struct DescriptorHeapPage
	{
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap;
		D3D12_CPU_DESCRIPTOR_HANDLE cpuStart{};
		D3D12_GPU_DESCRIPTOR_HANDLE gpuStart{};
		UINT capacity = 0;
		UINT usedDescriptors = 0;
	};
} //namespace ShaderResourceRuntime
