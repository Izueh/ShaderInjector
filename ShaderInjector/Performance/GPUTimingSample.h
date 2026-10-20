#pragma once

#include <string>

#include <d3d12.h>

namespace RenderPassGPUProfiler
{
	struct GPUTimingSample
	{
		std::string name;
		UINT firstQuery = 0;
		UINT64 cpuStartTicks = 0;
		double cpuMilliseconds = 0;
		bool completed = false;
	};
}
