#include "../HookD3D12.h"

#include "RenderPass/RenderPassRuntime.h"

namespace HookD3D12
{
	HRESULT STDMETHODCALLTYPE Hook_CreateCommandSignature(ID3D12Device* device, const D3D12_COMMAND_SIGNATURE_DESC* description, ID3D12RootSignature* rootSignature, REFIID interfaceId, void** commandSignature)
	{
		const HRESULT result = Original_CreateCommandSignature(device, description, rootSignature, interfaceId, commandSignature);

		//texture shader patches need to know whether ExecuteIndirect rewrites root arguments laid out for the game's root.
		if (SUCCEEDED(result) && description && commandSignature && *commandSignature && !IsInsideRenderPassInjection())
			RenderPassRuntime::TrackCommandSignature(static_cast<ID3D12CommandSignature*>(*commandSignature), *description);

		return result;
	}
} //namespace HookD3D12
