#pragma once

#include <cstddef>

#include "RenderPass/Registry/DescriptorPage.h"

namespace RenderPassResourceRegistry
{
	//one source range can cross a page boundary, so each span stays inside both metadata pages.
	struct DescriptorCopyBatchSpan
	{
		DescriptorPage* sourcePage = nullptr;
		DescriptorPage* destinationPage = nullptr;
		size_t sourceOffset = 0;
		size_t destinationOffset = 0;
		UINT descriptorCount = 0;
	};
}
