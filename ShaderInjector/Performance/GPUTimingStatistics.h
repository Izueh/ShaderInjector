#pragma once

#include <cstdint>

namespace RenderPassGPUProfiler
{
	struct GPUTimingStatistics
	{
		uint64_t samples = 0;
		double totalMilliseconds = 0;
		double maximumMilliseconds = 0;
		double totalCPUMilliseconds = 0;
		double maximumCPUMilliseconds = 0;
	};
}
