#pragma once

#include <d3d12.h>

namespace RenderPassGPUProfiler
{
	struct GPUTimingPage;

	struct GPUTimingToken
	{
		GPUTimingPage* page = nullptr;
		UINT sampleIndex = 0;
	};
}
