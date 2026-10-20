#pragma once

#include <cstddef>

#include <d3d12.h>

namespace ShaderResourceRuntime
{
	struct SamplerOverride
	{
		size_t tableIndex = 0;
		UINT tableOffset = 0;
		D3D12_SAMPLER_DESC description{};
	};
}
