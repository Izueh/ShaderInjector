#pragma once

#include <vector>

#include <d3d12.h>
#include <wrl/client.h>

#include "Performance/GPUTimingSample.h"

namespace RenderPassGPUProfiler
{
	//query storage belongs to a recording until its actual queue fence retires.
	struct GPUTimingPage
	{
		Microsoft::WRL::ComPtr<ID3D12QueryHeap> queryHeap;
		Microsoft::WRL::ComPtr<ID3D12Resource> readback;
		Microsoft::WRL::ComPtr<ID3D12Fence> retirementFence;
		std::vector<GPUTimingSample> samples;
		UINT64 retirementValue = 0;
		UINT64 timestampFrequency = 0;
		bool recorded = false;
		bool collected = true;
		bool retirementBlocked = false;
	};
}
