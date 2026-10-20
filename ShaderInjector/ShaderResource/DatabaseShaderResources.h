#pragma once

#include <string>
#include <vector>

#include "ShaderResource/ShaderResource.h"

namespace DatabaseShaderResources
{
	void RefreshShaderResources();
	void EnsureShaderResourcesLoaded();

	const std::vector<ShaderResource::TextureDisk>& GetShaderResources();
	//a copy that stays valid while another thread refreshes the folder.
	std::vector<ShaderResource::TextureDisk> CopyShaderResources();

	const ShaderResource::TextureDisk* FindShaderResourceById(const std::string& resourceId);
} //namespace DatabaseShaderResources
