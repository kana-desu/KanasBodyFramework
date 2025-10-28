#pragma once

#include <kbf/data/mesh/part_cache.hpp>
#include <kbf/data/mesh/mesh_part.hpp>
#include <kbf/data/armour/armour_piece.hpp>

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>

namespace kbf {

	class PartCacheManager {
	public:
		PartCacheManager(const std::filesystem::path& dataBasePath);

		bool loadPartCaches();
		void cacheParts(const ArmourSetWithCharacterSex& armour, const std::vector<MeshPart>& parts, ArmourPiece piece);

		const PartCache* getCachedParts(const ArmourSetWithCharacterSex& armour) const;
		std::vector<ArmourSetWithCharacterSex> getCachedArmourSets() const;

	private:
		const std::filesystem::path dataBasePath;
		const std::filesystem::path partCachesPath = dataBasePath / "PartCaches";
		void verifyDirectoryExists() const;

		bool loadPartCache(const std::filesystem::path& path, PartCache* out);
		bool loadPartCacheList(const rapidjson::Value& object, std::vector<MeshPart>* out) const;

		rapidjson::Document loadCacheJson(const std::string& path) const;
		// UNSAFE - Do not use directly. Call loadCacheJson instead.
		std::string readJsonFile(const std::string& path) const;
		bool writeJsonFile(std::string path, const std::string& json) const;

		bool writePartCache(const ArmourSetWithCharacterSex& armour) const;
		bool writePartCacheJson(const std::filesystem::path& path, const PartCache& out) const;
		void writePartCacheData(
			std::string id,
			std::string hashId,
			const std::vector<MeshPart>& parts,
			size_t hash,
			rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const;
		rapidjson::StringBuffer writeCompactRemovedPart(const MeshPart& part) const;

		std::string getPartCacheFilename(const ArmourSetWithCharacterSex& armour) const;
		bool getPartCacheArmourSet(const std::string& filename, ArmourSetWithCharacterSex* out) const;

		std::unordered_map<ArmourSetWithCharacterSex, PartCache> caches;
	};

}