#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include "Enum/ShaderType.h"

namespace ShaderPatch
{
	struct RecipeInfo
	{
		std::string name;
		std::string path;
		ShaderTarget::ShaderType stage = ShaderTarget::Unknown;
		bool shaderModel6 = false;
		bool requiresResourceBindings = false;
		uint32_t version = 1;
		uint64_t contentHash = 0;
	};
	struct TextureBinding
	{
		std::string resourceName;
		uint32_t shaderRegister = 0;
		uint32_t registerSpace = 0;
	};
	struct PatchResult
	{
		std::vector<uint8_t> bytecode;
		std::vector<TextureBinding> textures;
		std::string recipeName;
		std::string error;
	};
	class Recipe
	{
	  public:
		static std::shared_ptr<const Recipe> Load(const RecipeInfo& source, const std::string& text, std::string& error);
		const RecipeInfo& GetInfo() const;
		bool Execute(const std::vector<uint8_t>& input, PatchResult& result) const;

	  private:
		struct Implementation;
		explicit Recipe(std::shared_ptr<Implementation> implementation);
		std::shared_ptr<Implementation> implementation;
	};
	std::string GetBuildIdentity();
} //namespace ShaderPatch
