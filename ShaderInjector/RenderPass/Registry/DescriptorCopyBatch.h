#pragma once

#include <memory>
#include <mutex>
#include <shared_mutex>
#include <vector>

#include "RenderPass/Registry/DescriptorCopyBatchSpan.h"
#include "RenderPass/Registry/DescriptorHeapRecord.h"

namespace RenderPassResourceRegistry
{
	//reuse per-thread scratch storage while retaining each source heap only once per API call.
	struct DescriptorCopyBatch
	{
		std::vector<std::shared_ptr<DescriptorHeapRecord>> sourceHeapOwners;
		std::vector<DescriptorCopyBatchSpan> spans;
		std::vector<DescriptorPage*> sourcePages;
		std::vector<DescriptorPage*> destinationPages;
		std::vector<std::shared_lock<std::shared_mutex>> sourceLocks;
		std::vector<std::unique_lock<std::shared_mutex>> destinationLocks;
		bool containsTrackedMetadata = false;

		void Reset()
		{
			//release locks before their owning heaps, but keep vector capacity for the next copy.
			destinationLocks.clear();

			sourceLocks.clear();

			sourceHeapOwners.clear();

			spans.clear();

			sourcePages.clear();

			destinationPages.clear();

			containsTrackedMetadata = false;
		}
	};
}
