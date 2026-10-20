#pragma once

#include <cstdint>

namespace RenderPassResourceRegistry
{
	struct DescriptorContentVersion
	{
		uint64_t heapGeneration = 0;
		uint64_t revision = 0;
	};
} //namespace RenderPassResourceRegistry
