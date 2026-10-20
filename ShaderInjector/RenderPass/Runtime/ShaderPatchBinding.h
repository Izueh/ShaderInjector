#pragma once

#include <string>
#include <vector>

#include <d3d12.h>

namespace RenderPassRuntime
{
	//a patched pipeline that samples imported textures through an expanded copy of the game's root.
	//the added table lives in the reserved tail of the game's own heap, so game tables stay bound as-is.
	struct ShaderPatchBinding
	{
		ID3D12Device* device = nullptr;
		ID3D12PipelineState* pipelineState = nullptr;
		ID3D12RootSignature* rootSignature = nullptr;
		UINT appendedTableIndex = 0;
		bool computePipeline = false;
		//imported DDS ids in the appended table's order; the patch is skipped while one is not loaded.
		std::vector<std::string> textures;
	};
} //namespace RenderPassRuntime
