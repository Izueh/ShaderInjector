#include "Recipe.h"
#include <algorithm>
#include <cstring>
#include <variant>
#include <dxp/Version.hpp>
#include <dxp/sm5/Recipe.hpp>
#include <dxp/sm6/Recipe.hpp>

namespace ShaderPatch
{
	struct Recipe::Implementation
	{
		RecipeInfo info;
		std::variant<dxp::sm5::Recipe, dxp::sm6::Recipe> recipe;
	};

	Recipe::Recipe(std::shared_ptr<Implementation> implementation) : implementation(std::move(implementation))
	{
	}

	std::shared_ptr<const Recipe> Recipe::Load(const RecipeInfo& source, const std::string& text, std::string& error)
	{
		auto implementation = std::make_shared<Implementation>();
		implementation->info = source;
		if (source.shaderModel6)
		{
			auto parsed = dxp::sm6::Recipe::ParseFromText(text, source.path);
			if (!parsed)
			{
				error = source.name + ": " + parsed.error();
				return {};
			}
			implementation->info.version = parsed->GetVersion();
			implementation->info.requiresResourceBindings = parsed->HasAddResourceSteps();
			implementation->recipe = std::move(*parsed);
		}
		else
		{
			auto parsed = dxp::sm5::Recipe::ParseFromText(text, source.path);
			if (!parsed)
			{
				error = source.name + ": " + parsed.error();
				return {};
			}
			implementation->info.version = parsed->GetVersion();
			implementation->info.requiresResourceBindings = parsed->HasAddResourceSteps();
			implementation->recipe = std::move(*parsed);
		}
		return std::shared_ptr<const Recipe>(new Recipe(std::move(implementation)));
	}

	const RecipeInfo& Recipe::GetInfo() const
	{
		return implementation->info;
	}

	bool Recipe::Execute(const std::vector<uint8_t>& input, PatchResult& result) const
	{
		return std::visit([&](const auto& recipe)
						  {
			auto report = recipe.Execute(input);
			if (!report) { result.error = report.error(); return false; }
			if (!report->modified) return false;
			result.bytecode = std::move(report->output_bytes);
			result.recipeName = implementation->info.name;
			for (const auto& entry : report->new_bindings)
			{
				const auto& binding = entry.second;
				if (binding.binding_class != dxp::BindingClass::Texture || binding.handle.empty())
				{
					result.error = "Only named texture SRVs can be added by shader recipes";
					result.bytecode.clear();
					return false;
				}
				result.textures.push_back({binding.handle, binding.register_index, binding.space});
			}
			std::sort(result.textures.begin(), result.textures.end(), [](const auto& left, const auto& right)
			{
				if (left.registerSpace != right.registerSpace) return left.registerSpace < right.registerSpace;
				return left.shaderRegister < right.shaderRegister;
			});
			return true; }, implementation->recipe);
	}

	//the DXP release version plus patch-2, which versions the output semantics, keys the patch cache.
	std::string GetBuildIdentity()
	{
		return DXP_VERSION_STRING "|patch-2";
	}

} //namespace ShaderPatch
