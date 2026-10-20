#pragma once

#include <cstdint>

#include <d3d12.h>
#include <wrl/client.h>

namespace ShaderResourceRuntime
{
	//keep the uploaded texture and its shader view alive while passes reference it.
	struct TextureGPU
	{
		Microsoft::WRL::ComPtr<ID3D12Resource> texture;
		Microsoft::WRL::ComPtr<ID3D12Resource> upload;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> shaderResourceViewHeap;
		D3D12_SHADER_RESOURCE_VIEW_DESC view{};
		bool uploadRecorded = false;
		//loaded at startup or reload and waited for, so any command list may read it; a lazy upload is
		//recorded into the game's own command list and only usable by work recorded after it.
		bool uploaded = false;
		//the DDS file this texture was loaded from; a reload replaces the texture when it changes.
		uint64_t fileTime = 0;
		uint64_t fileSize = 0;
	};
} //namespace ShaderResourceRuntime
