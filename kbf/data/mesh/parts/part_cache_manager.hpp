#pragma once

#include <kbf/data/file/cache_manager.hpp>
#include <kbf/data/mesh/parts/part_cache.hpp>
#include <kbf/data/mesh/parts/mesh_part.hpp>

namespace kbf {

	using PartCacheManager_t = CacheManager<PartCache, MeshPart>;

	class PartCacheManager : public PartCacheManager_t {
	public:
		using PartCacheManager_t::CacheManager;

		void cache(const ArmourSetWithCharacterSex& armour, const std::vector<MeshPart>& parts, ArmourPiece piece) override;

	private:
		bool getCacheFromDocument(const rapidjson::Document& doc, ArmourSetWithCharacterSex armour, PartCache& out) const override;
		bool loadPartCacheList(const rapidjson::Value& object, std::vector<MeshPart>& out) const;

		bool writeCacheJson(const std::filesystem::path& path, const PartCache& out) const override;
		void writePartCacheData(
			std::string id,
			std::string hashId,
			const std::vector<MeshPart>& parts,
			size_t hash,
			rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const;
		rapidjson::StringBuffer writeCompactRemovedPart(const MeshPart& part) const;

	};

}