#pragma once

#include <cstddef>

#include <d3d12.h>

namespace ShaderResourceRuntime
{
	//resolve register locations first, then allocate only the tables that actually change.
	struct DescriptorOverride
	{
		size_t tableIndex = 0;
		UINT tableOffset = 0;
		D3D12_CPU_DESCRIPTOR_HANDLE source{};
	};
}
