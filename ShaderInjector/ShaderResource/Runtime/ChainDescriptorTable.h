#pragma once

#include "RenderPass/Registry/DescriptorContentVersion.h"
#include "ShaderResource/Runtime/DescriptorAllocation.h"

namespace ShaderResourceRuntime
{
	//base copies and repeated sampler settings stay immutable; texture overrides get their own table.
	struct ChainDescriptorTable
	{
		D3D12_CPU_DESCRIPTOR_HANDLE source{};
		UINT descriptorCount = 0;
		RenderPassResourceRegistry::DescriptorContentVersion sourceVersion;
		DescriptorAllocation allocation;
		bool hasSamplerOverride = false;
		UINT samplerOffset = 0;
		D3D12_SAMPLER_DESC samplerDescription{};
	};
}
