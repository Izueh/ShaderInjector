#include "ShaderPatch/ShaderPatchCache.h"

#include <filesystem>
#include <fstream>
#include <mutex>
#include <unordered_map>
#include <vector>

#include <windows.h>

#include "IO/ShaderInjectorIO.h"
#include "JsonHelper.h"

namespace ShaderPatchCache
{
	namespace
	{
		constexpr int cacheFormat = 5;
		constexpr size_t maximumFileBytes = 256u * 1024u * 1024u;
		//entries no launch has used for this many sessions are dropped when the file is written.
		constexpr uint64_t maximumUnusedSessions = 8;

		struct Entry
		{
			ShaderPatch::PatchResult result;
			uint64_t lastSession = 0;
		};

		std::mutex gMutex;
		std::unordered_map<std::string, Entry> gEntries;
		uint64_t gSession = 1;
		bool gLoaded = false;
		bool gDirty = false;

		std::string GetCacheDirectory()
		{
			return ShaderInjectorIO::JoinPath(ShaderInjectorIO::GetShaderInjectorDirectory(), "Cache");
		}

		std::string GetCachePath()
		{
			return ShaderInjectorIO::JoinPath(GetCacheDirectory(), "ShaderPatches.bin");
		}

		bool ReadBytes(const std::string& path, std::vector<uint8_t>& bytes)
		{
			std::ifstream file(ShaderInjectorIO::PathFromUTF8(path), std::ios::binary | std::ios::ate);
			if (!file)
				return false;
			const auto size = file.tellg();
			if (size <= 0 || static_cast<uint64_t>(size) > maximumFileBytes)
				return false;
			bytes.resize(static_cast<size_t>(size));
			file.seekg(0);
			return static_cast<bool>(file.read(reinterpret_cast<char*>(bytes.data()), bytes.size()));
		}

		//write to a process-unique temporary file and rename it, so a crash mid-write keeps the previous file.
		bool WriteAtomically(const std::string& path, const std::vector<uint8_t>& bytes)
		{
			ShaderInjectorIO::DirectoryCreate(ShaderInjectorIO::DirectoryFromPath(path));
			const std::string temporaryPath = path + "." + std::to_string(GetCurrentProcessId()) + ".tmp";
			if (!ShaderInjectorIO::WriteBinaryFile(temporaryPath, bytes.data(), bytes.size()))
				return false;
			if (MoveFileExW(ShaderInjectorIO::PathFromUTF8(temporaryPath).c_str(), ShaderInjectorIO::PathFromUTF8(path).c_str(), MOVEFILE_REPLACE_EXISTING))
				return true;
			ShaderInjectorIO::DeleteFileIfExists(temporaryPath);
			return false;
		}
	} //namespace

	void Load()
	{
		std::lock_guard<std::mutex> lock(gMutex);
		if (gLoaded)
			return;
		gLoaded = true;

		//earlier builds kept one file per shader in this folder.
		std::error_code error;
		std::filesystem::remove_all(ShaderInjectorIO::PathFromUTF8(ShaderInjectorIO::JoinPath(GetCacheDirectory(), "ShaderPatches")), error);

		std::vector<uint8_t> bytes;
		if (!ReadBytes(GetCachePath(), bytes))
			return;

		//a corrupt or outdated file is discarded; recipes simply execute again.
		try
		{
			const auto json = nlohmann::ordered_json::from_cbor(bytes);
			if (json.at("format") != cacheFormat)
				return;

			std::unordered_map<std::string, Entry> entries;
			for (const auto& item : json.at("entries"))
			{
				Entry entry;
				const auto& bytecode = item.at("bytecode").get_binary();
				entry.result.bytecode.assign(bytecode.begin(), bytecode.end());
				entry.result.recipeName = item.at("recipe").get<std::string>();
				for (const auto& binding : item.at("textures"))
					entry.result.textures.push_back({binding.at("name").get<std::string>(), binding.at("register").get<uint32_t>(), binding.at("space").get<uint32_t>()});
				entry.lastSession = item.at("session").get<uint64_t>();
				if (entry.result.bytecode.empty() != entry.result.recipeName.empty())
					return;
				entries.emplace(item.at("key").get<std::string>(), std::move(entry));
			}

			gEntries = std::move(entries);
			gSession = json.at("session").get<uint64_t>() + 1;
		}
		catch (const std::exception&)
		{
			gEntries.clear();
		}
	}

	bool Find(const std::string& key, ShaderPatch::PatchResult& result)
	{
		std::lock_guard<std::mutex> lock(gMutex);
		const auto entry = gEntries.find(key);
		if (entry == gEntries.end())
			return false;

		if (entry->second.lastSession != gSession)
		{
			entry->second.lastSession = gSession;
			gDirty = true;
		}
		result = entry->second.result;
		return true;
	}

	void Store(const std::string& key, const ShaderPatch::PatchResult& result)
	{
		std::lock_guard<std::mutex> lock(gMutex);
		gEntries[key] = {result, gSession};
		gDirty = true;
	}

	void Save()
	{
		//at process exit the worker is already terminated; if it died holding the lock, the entries may be torn.
		std::unique_lock<std::mutex> lock(gMutex, std::try_to_lock);
		if (!lock.owns_lock() || !gDirty)
			return;

		nlohmann::ordered_json entries = nlohmann::ordered_json::array();
		for (const auto& [key, entry] : gEntries)
		{
			if (gSession - entry.lastSession >= maximumUnusedSessions)
				continue;

			nlohmann::ordered_json textures = nlohmann::ordered_json::array();
			for (const ShaderPatch::TextureBinding& binding : entry.result.textures)
				textures.push_back({{"name", binding.resourceName}, {"register", binding.shaderRegister}, {"space", binding.registerSpace}});
			entries.push_back({{"key", key},
							   {"session", entry.lastSession},
							   {"recipe", entry.result.recipeName},
							   {"bytecode", nlohmann::ordered_json::binary(entry.result.bytecode)},
							   {"textures", std::move(textures)}});
		}

		const nlohmann::ordered_json json = {{"format", cacheFormat}, {"session", gSession}, {"entries", std::move(entries)}};
		//no logging: the log's lock may belong to a thread process exit already terminated.
		if (WriteAtomically(GetCachePath(), nlohmann::ordered_json::to_cbor(json)))
			gDirty = false;
	}
} //namespace ShaderPatchCache
