#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <d3d12.h>
#include <wrl/client.h>

namespace RenderPassResourceRegistry
{
	struct ResourceRegister
	{
		uint32_t shaderRegister = 0;
		uint32_t registerSpace = 0;
	};

	//a game root signature with one SRV descriptor table appended for injected textures.
	//bindings are sorted by (space, register); binding N occupies descriptor N of the appended table.
	struct ExpandedRootSignature
	{
		Microsoft::WRL::ComPtr<ID3D12RootSignature> root;
		std::vector<ResourceRegister> bindings;
		UINT appendedTableIndex = 0;
	};

	//stages is a bit mask of ShaderTarget::ShaderType values that read the added SRVs.
	std::shared_ptr<const ExpandedRootSignature> GetOrCreateExpandedRootSignature(
		ID3D12Device* device,
		const std::vector<uint8_t>& originalRoot,
		std::vector<ResourceRegister> bindings,
		uint32_t stages,
		std::string& outError);
} //namespace RenderPassResourceRegistry
