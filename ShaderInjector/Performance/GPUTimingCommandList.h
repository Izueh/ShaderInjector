#pragma once

#include <memory>
#include <vector>

#include <d3d12.h>
#include <wrl/client.h>

#include "Performance/GPUTimingPage.h"
#include "Performance/GPUTimingToken.h"

namespace RenderPassGPUProfiler
{
	struct GPUTimingCommandList
	{
		Microsoft::WRL::ComPtr<ID3D12Device> device;
		std::vector<std::unique_ptr<GPUTimingPage>> pages;
		GPUTimingPage* currentPage = nullptr;
		GPUTimingToken originalDraw;
		bool allocationFailed = false;
	};
}
