#pragma once

#include <kbf/data/file/cache_manager.hpp>
#include <kbf/data/bones/bone_cache.hpp>

namespace kbf {

	using BoneCacheManager_t = CacheManager<BoneCache, std::string>;

	class BoneCacheManager : public BoneCacheManager_t {
	public:
		using BoneCacheManager_t::CacheManager;

		void cache(const ArmourSetWithCharacterSex& armour, const std::vector<std::string>& bones, ArmourPiece piece) override;
		bool boneExists(const ArmourSetWithCharacterSex& armour, ArmourPiece piece, const std::string& boneName) const;

	private:
		bool getCacheFromDocument(const rapidjson::Document& doc, ArmourSetWithCharacterSex armour, BoneCache& out) const override;
		bool writeCacheJson(const std::filesystem::path& path, const BoneCache& out) const override;

		void writeBoneCacheData(
			std::string id,
			std::string hashId,
			const std::vector<std::string>& bones,
			size_t hash,
			rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const;
	};

}