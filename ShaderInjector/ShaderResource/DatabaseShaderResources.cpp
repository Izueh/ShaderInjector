#include "ShaderResource/DatabaseShaderResources.h"

#include <algorithm>
#include <atomic>
#include <filesystem>
#include <mutex>
#include <unordered_map>

#include "IO/ShaderInjectorIO.h"
#include "ShaderResource/ShaderResourceCatalog.h"
#include "DDS/DDS.h"

namespace DatabaseShaderResources
{
	std::vector<ShaderResource::TextureDisk> gShaderResources;
	std::unordered_map<std::string, size_t> gShaderResourceIndices;
	std::atomic<bool> gShaderResourcesLoaded = false;
	//device creation loads textures while the startup worker publishes render passes; scan once, swap in whole.
	std::mutex gShaderResourcesMutex;
	std::mutex gEnsureLoadedMutex;

	void RefreshShaderResources()
	{
		std::vector<ShaderResource::TextureDisk> shaderResources;
		std::unordered_map<std::string, size_t> shaderResourceIndices;
		const std::string rootDirectory = ShaderInjectorIO::GetShaderResourcesDirectory();
		ShaderInjectorIO::DirectoryCreate(rootDirectory);

		std::vector<std::string> texturePaths;
		ShaderInjectorIO::CollectFilesByExtension(rootDirectory, ".dds", texturePaths, true, true);
		const std::filesystem::path rootPath = std::filesystem::u8path(rootDirectory);

		for (const std::string& texturePath : texturePaths)
		{
			std::error_code error;
			const std::filesystem::path path = std::filesystem::u8path(texturePath);
			std::filesystem::path relativePath = std::filesystem::relative(path, rootPath, error);

			if (error)
				continue;

			ShaderResource::TextureDisk resource{};
			resource.id = relativePath.generic_u8string();
			resource.name = path.stem().u8string();
			resource.fileName = path.filename().u8string();
			resource.filePath = texturePath;

			DDS::Metadata metadata{};

			if (DDS::ReadMetadata(texturePath, metadata, resource.validationError))
			{
				resource.dimension = metadata.dimension;
				resource.width = metadata.width;
				resource.height = metadata.height;
				resource.depth = metadata.depth;
				resource.arraySize = metadata.arraySize;
				resource.mipLevels = metadata.mipLevels;
				resource.format = static_cast<uint32_t>(metadata.format);
			}
			else
			{
				ShaderInjectorIO::WriteToLogFileWarning("DatabaseShaderResources->RefreshShaderResources: invalid DDS resource = " + resource.id + " error=" + resource.validationError);
			}

			shaderResources.push_back(std::move(resource));
		}

		std::sort(shaderResources.begin(), shaderResources.end(), [](const auto& left, const auto& right)
				  { return left.id < right.id; });

		shaderResourceIndices.reserve(shaderResources.size());

		for (size_t resourceIndex = 0; resourceIndex < shaderResources.size(); ++resourceIndex)
			shaderResourceIndices.emplace(shaderResources[resourceIndex].id, resourceIndex);

		const size_t resourceCount = shaderResources.size();
		{
			std::lock_guard<std::mutex> lock(gShaderResourcesMutex);
			gShaderResources.swap(shaderResources);
			gShaderResourceIndices.swap(shaderResourceIndices);
			gShaderResourcesLoaded.store(true, std::memory_order_release);
			ShaderResourceCatalog::PublishDiskResources(gShaderResources);
		}

		ShaderInjectorIO::WriteToLogFile("DatabaseShaderResources->RefreshShaderResources: loaded DDS textures = " + std::to_string(resourceCount));
	}

	void EnsureShaderResourcesLoaded()
	{
		if (gShaderResourcesLoaded.load(std::memory_order_acquire))
			return;
		std::lock_guard<std::mutex> lock(gEnsureLoadedMutex);
		if (!gShaderResourcesLoaded.load(std::memory_order_acquire))
			RefreshShaderResources();
	}

	std::vector<ShaderResource::TextureDisk> CopyShaderResources()
	{
		EnsureShaderResourcesLoaded();
		std::lock_guard<std::mutex> lock(gShaderResourcesMutex);
		return gShaderResources;
	}

	const std::vector<ShaderResource::TextureDisk>& GetShaderResources()
	{
		EnsureShaderResourcesLoaded();
		return gShaderResources;
	}

	const ShaderResource::TextureDisk* FindShaderResourceById(const std::string& resourceId)
	{
		EnsureShaderResourcesLoaded();
		const auto resourceIt = gShaderResourceIndices.find(resourceId);
		if (resourceIt != gShaderResourceIndices.end())
			return &gShaderResources[resourceIt->second];
		return nullptr;
	}
} //namespace DatabaseShaderResources
