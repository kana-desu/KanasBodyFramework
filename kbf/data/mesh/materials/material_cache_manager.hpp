#pragma once

#include <kbf/data/file/cache_manager.hpp>
#include <kbf/data/mesh/materials/material_cache.hpp>
#include <kbf/data/mesh/parts/mesh_part.hpp>

namespace kbf {

	using MaterialCacheManager_t = CacheManager<MaterialCache, MeshMaterial>;

	class MaterialCacheManager : public MaterialCacheManager_t {
	public:
		using MaterialCacheManager_t::CacheManager;

		void cache(const ArmourSetWithCharacterSex& armour, const std::vector<MeshMaterial>& parts, ArmourPiece piece) override;

	private:
		bool getCacheFromDocument(const rapidjson::Document& doc, ArmourSetWithCharacterSex armour, MaterialCache& out) const override;
		bool loadMaterialCacheList(const rapidjson::Value& object, std::vector<MeshMaterial>& out) const;

		bool writeCacheJson(const std::filesystem::path& path, const MaterialCache& out) const override;
		void writeMaterialCacheData(
			std::string id,
			std::string hashId,
			const std::vector<MeshMaterial>& mats,
			size_t hash,
			rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const;
		rapidjson::StringBuffer writeCompactMeshMaterialParam(size_t idx, const MeshMaterialParam& mat) const;

	};

}