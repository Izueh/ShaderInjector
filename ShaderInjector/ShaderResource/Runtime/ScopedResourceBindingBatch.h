#pragma once

#include <d3d12.h>

namespace ShaderResourceRuntime
{
	//keep injector heaps bound between passes, then restore the game once at the end of the graph.
	class ScopedResourceBindingBatch
	{
		ID3D12GraphicsCommandList* commandList = nullptr;
		ID3D12GraphicsCommandList* previousCommandList = nullptr;
		bool keepResourcesForOriginalDraw = false;

	public:
		explicit ScopedResourceBindingBatch(ID3D12GraphicsCommandList* commandList);
		~ScopedResourceBindingBatch();

		//a replacement pass needs its bindings until the original draw or dispatch has completed.
		void KeepResourcesForOriginalDraw();

		ScopedResourceBindingBatch(const ScopedResourceBindingBatch&) = delete;
		ScopedResourceBindingBatch& operator=(const ScopedResourceBindingBatch&) = delete;
	};
}
