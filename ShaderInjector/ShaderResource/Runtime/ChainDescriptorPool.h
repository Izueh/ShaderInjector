#pragma once

#include <vector>

#include "ShaderResource/Runtime/ChainDescriptorTable.h"

namespace ShaderResourceRuntime
{
	//all tables used together must reside in the same shader-visible heap.
	struct ChainDescriptorPool
	{
		ID3D12DescriptorHeap* heap = nullptr;
		std::vector<ChainDescriptorTable> tables;

		void Reset()
		{
			//drop lookup validity only; the execution slot still owns every range until the GPU fence retires.
			heap = nullptr;
			tables.clear();
		}
	};
}
