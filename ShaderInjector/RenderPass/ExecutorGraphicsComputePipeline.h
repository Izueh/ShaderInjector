#pragma once

#include <vector>
#include <d3d12.h>
#include <wrl/client.h>

namespace RenderPassExecutor
{
	//keep the compute root and its graphics-to-compute argument mapping together.
	struct GraphicsComputePipeline
	{
		Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;
		std::vector<UINT> rootParameterMap;
	};
}
