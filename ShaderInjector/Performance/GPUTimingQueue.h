#pragma once

#include <d3d12.h>
#include <wrl/client.h>

namespace RenderPassGPUProfiler
{
	struct GPUTimingQueue
	{
		Microsoft::WRL::ComPtr<ID3D12Fence> fence;
		UINT64 nextFenceValue = 0;
		UINT64 timestampFrequency = 0;
		bool unavailable = false;
	};
}
