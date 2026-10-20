#pragma once

#include <cstdint>
#include <string>

#include "ShaderPatch/Recipe.h"

//DXP results per (shader, stage, applicable recipes), kept in memory and stored in one file, like a pipeline
//library: loaded once at startup and written at a clean shutdown, so gameplay does no cache I/O.
namespace ShaderPatchCache
{
	void Load();
	//an entry with empty bytecode records that no applicable recipe modified the shader.
	bool Find(const std::string& key, ShaderPatch::PatchResult& result);
	void Store(const std::string& key, const ShaderPatch::PatchResult& result);
	void Save();
} //namespace ShaderPatchCache
