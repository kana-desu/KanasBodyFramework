#pragma once

#include <kbf/data/bones/bone_cache.hpp>
#include <kbf/data/armour/armour_piece.hpp>

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>

namespace kbf {

	class BoneCacheManager {
	public:
		BoneCacheManager(const std::filesystem::path& dataBasePath);

		bool loadBoneCaches();
		void cacheBones(const ArmourSetWithCharacterSex& armour, const std::vector<std::string>& bones, ArmourPiece piece);

		const BoneCache* getCachedBones(const ArmourSetWithCharacterSex& armour) const;
		std::vector<ArmourSetWithCharacterSex> getCachedArmourSets() const;

		bool boneExists(const ArmourSetWithCharacterSex& armour, ArmourPiece piece, const std::string& boneName) const;

	private:
		const std::filesystem::path dataBasePath;
		const std::filesystem::path boneCachesPath = dataBasePath / "BoneCaches";
		void verifyDirectoryExists() const;

		bool loadBoneCache(const std::filesystem::path& path, BoneCache* out);

		rapidjson::Document loadCacheJson(const std::string& path) const;
		// UNSAFE - Do not use directly. Call loadCacheJson instead.
		std::string readJsonFile(const std::string& path) const;
		bool writeJsonFile(std::string path, const std::string& json) const;

		bool writeBoneCache(const ArmourSetWithCharacterSex& armour) const;
		bool writeBoneCacheJson(const std::filesystem::path& path, const BoneCache& out) const;
		void writeBoneCacheData(
			std::string id, 
			std::string hashId, 
			const std::vector<std::string>& bones,
			size_t hash,
			rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const;

		std::string getBoneCacheFilename(const ArmourSetWithCharacterSex& armour) const;
		bool getBoneCacheArmourSet(const std::string& filename, ArmourSetWithCharacterSex* out) const;

		std::unordered_map<ArmourSetWithCharacterSex, BoneCache> caches;
	};

}