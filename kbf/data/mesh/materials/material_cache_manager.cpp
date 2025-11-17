#include <kbf/data/mesh/materials/material_cache_manager.hpp>

#include <kbf/data/ids/material_cache_ids.hpp>
#include <kbf/data/file/field_parsers.hpp>
#include <kbf/data/ids/format_ids.hpp>

#define MATERIAL_CACHE_MANAGER_LOG_TAG "[MaterialCacheManager]"

#undef GetObject

namespace kbf {

	void MaterialCacheManager::cache(const ArmourSetWithCharacterSex& armour, const std::vector<MeshMaterial>& mats, ArmourPiece piece) {
		size_t hash = HashedMaterialList::hashMaterials(mats);

		bool changed = false;
		if (caches.find(armour) != caches.end()) {
			MaterialCache& existingCache = caches.at(armour);

			HashedMaterialList* targetList = nullptr;
			switch (piece) {
			case ArmourPiece::AP_HELM: targetList = &existingCache.helm; break;
			case ArmourPiece::AP_BODY: targetList = &existingCache.body; break;
			case ArmourPiece::AP_ARMS: targetList = &existingCache.arms; break;
			case ArmourPiece::AP_COIL: targetList = &existingCache.coil; break;
			case ArmourPiece::AP_LEGS: targetList = &existingCache.legs; break;
			}

			if (!targetList || targetList->getHash() == hash) return; // No need to update if the hash is the same
			*targetList = HashedMaterialList{ mats, hash };
			changed = true;
		}
		else {
			MaterialCache newCache{
				armour,
				(piece == ArmourPiece::AP_HELM) ? HashedMaterialList{ mats, hash } : HashedMaterialList{},
				(piece == ArmourPiece::AP_BODY) ? HashedMaterialList{ mats, hash } : HashedMaterialList{},
				(piece == ArmourPiece::AP_ARMS) ? HashedMaterialList{ mats, hash } : HashedMaterialList{},
				(piece == ArmourPiece::AP_COIL) ? HashedMaterialList{ mats, hash } : HashedMaterialList{},
				(piece == ArmourPiece::AP_LEGS) ? HashedMaterialList{ mats, hash } : HashedMaterialList{},
			};
			caches.emplace(armour, newCache);
			changed = true;
		}

		if (changed) writeCache(armour);
	}

	void MaterialCacheManager::cache(
		const ArmourSetWithCharacterSex& armour,
		const std::unordered_map<std::string, MeshMaterial>& parts,
		ArmourPiece piece)
	{
		// Convert map values to vector container
		std::vector<MeshMaterial> mats;
		mats.reserve(parts.size());

		for (const auto& kv : parts)
			mats.push_back(kv.second);

		cache(armour, mats, piece);
	}

	bool MaterialCacheManager::getCacheFromDocument(const rapidjson::Document& doc, ArmourSetWithCharacterSex armour, MaterialCache& out) const {
		std::vector<MeshMaterial> helmParts{};
		std::vector<MeshMaterial> bodyParts{};
		std::vector<MeshMaterial> armsParts{};
		std::vector<MeshMaterial> coilParts{};
		std::vector<MeshMaterial> legsParts{};
		size_t helmHash = 0;
		size_t bodyHash = 0;
		size_t armsHash = 0;
		size_t coilHash = 0;
		size_t legsHash = 0;

		// various mesh parts as objects with name as key
		bool parsed = true;
		parsed &= parseObject(doc, MATERIAL_CACHE_HELM_ID, MATERIAL_CACHE_HELM_ID);
		if (parsed) parsed &= loadMaterialCacheList(doc[MATERIAL_CACHE_HELM_ID], helmParts);
		parsed &= parseObject(doc, MATERIAL_CACHE_BODY_ID, MATERIAL_CACHE_BODY_ID);
		if (parsed) parsed &= loadMaterialCacheList(doc[MATERIAL_CACHE_BODY_ID], bodyParts);
		parsed &= parseObject(doc, MATERIAL_CACHE_ARMS_ID, MATERIAL_CACHE_ARMS_ID);
		if (parsed) parsed &= loadMaterialCacheList(doc[MATERIAL_CACHE_ARMS_ID], armsParts);
		parsed &= parseObject(doc, MATERIAL_CACHE_COIL_ID, MATERIAL_CACHE_COIL_ID);
		if (parsed) parsed &= loadMaterialCacheList(doc[MATERIAL_CACHE_COIL_ID], coilParts);
		parsed &= parseObject(doc, MATERIAL_CACHE_LEGS_ID, MATERIAL_CACHE_LEGS_ID);
		if (parsed) parsed &= loadMaterialCacheList(doc[MATERIAL_CACHE_LEGS_ID], legsParts);

		parsed &= parseUint64(doc, MATERIAL_CACHE_HELM_HASH_ID, MATERIAL_CACHE_HELM_HASH_ID, &helmHash);
		parsed &= parseUint64(doc, MATERIAL_CACHE_BODY_HASH_ID, MATERIAL_CACHE_BODY_HASH_ID, &bodyHash);
		parsed &= parseUint64(doc, MATERIAL_CACHE_ARMS_HASH_ID, MATERIAL_CACHE_ARMS_HASH_ID, &armsHash);
		parsed &= parseUint64(doc, MATERIAL_CACHE_COIL_HASH_ID, MATERIAL_CACHE_COIL_HASH_ID, &coilHash);
		parsed &= parseUint64(doc, MATERIAL_CACHE_LEGS_HASH_ID, MATERIAL_CACHE_LEGS_HASH_ID, &legsHash);

		if (parsed) {
			out = MaterialCache{
				armour,
				HashedMaterialList{ helmParts, helmHash },
				HashedMaterialList{ bodyParts, bodyHash },
				HashedMaterialList{ armsParts, armsHash },
				HashedMaterialList{ coilParts, coilHash },
				HashedMaterialList{ legsParts, legsHash }
			};
		}

		return parsed;
	}

	bool MaterialCacheManager::loadMaterialCacheList(const rapidjson::Value& object, std::vector<MeshMaterial>& out) const {
		bool parsed = true;
		for (const auto& part : object.GetObject()) {
			MeshMaterial mat{};
			mat.name = part.name.GetString();
			parsed &= part.value.IsObject();
			if (parsed) {

				parsed &= parseUint64(part.value, MATERIAL_CACHE_MAT_INDEX_ID, mat.name + "." + MATERIAL_CACHE_MAT_INDEX_ID, &mat.index);
				parsed &= parseObject(part.value, MATERIAL_CACHE_MAT_PARAMS_ID, mat.name + "." + MATERIAL_CACHE_MAT_PARAMS_ID);

				if (parsed) {
					for (const auto& matParam : part.value[MATERIAL_CACHE_MAT_PARAMS_ID].GetObject()) {
						MeshMaterialParam param{};
						size_t paramIndex = 0;
						uint64_t paramTypeInt = 0;

						parsed &= parseUint64(matParam.value, MATERIAL_CACHE_MAT_PARAM_INDEX_ID, mat.name + "." + matParam.name.GetString() + "." + MATERIAL_CACHE_MAT_PARAM_INDEX_ID, &paramIndex);
						parsed &= parseUint64(matParam.value, MATERIAL_CACHE_MAT_PARAM_TYPE_ID, mat.name + "." + matParam.name.GetString() + "." + MATERIAL_CACHE_MAT_PARAM_TYPE_ID, &paramTypeInt);
						param.type = static_cast<MeshMaterialParamType>(paramTypeInt);
						param.name = matParam.name.GetString();
						param.index = paramIndex;
						if (parsed) {
							mat.params.emplace(param.name, param);
						}
					}
				}
			}
			if (parsed) out.push_back(mat);
		}
		return parsed;
	}


	bool MaterialCacheManager::writeCacheJson(const std::filesystem::path& path, const MaterialCache& out) const {
		DEBUG_STACK.push(std::format("{} Writing Material Cache @ \"{}\"...", MATERIAL_CACHE_MANAGER_LOG_TAG, path.string()), DebugStack::Color::COL_INFO);

		rapidjson::StringBuffer s;
		rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(s);

		writer.StartObject();
		writer.Key(FORMAT_VERSION_ID);
		writer.String(KBF_VERSION);
		writeMaterialCacheData(MATERIAL_CACHE_HELM_ID, MATERIAL_CACHE_HELM_HASH_ID, out.helm.getMaterials(), out.helm.getHash(), writer);
		writeMaterialCacheData(MATERIAL_CACHE_BODY_ID, MATERIAL_CACHE_BODY_HASH_ID, out.body.getMaterials(), out.body.getHash(), writer);
		writeMaterialCacheData(MATERIAL_CACHE_ARMS_ID, MATERIAL_CACHE_ARMS_HASH_ID, out.arms.getMaterials(), out.arms.getHash(), writer);
		writeMaterialCacheData(MATERIAL_CACHE_COIL_ID, MATERIAL_CACHE_COIL_HASH_ID, out.coil.getMaterials(), out.coil.getHash(), writer);
		writeMaterialCacheData(MATERIAL_CACHE_LEGS_ID, MATERIAL_CACHE_LEGS_HASH_ID, out.legs.getMaterials(), out.legs.getHash(), writer);
		writer.EndObject();

		bool success = writeJsonFile(path.string(), s.GetString());

		if (!success) {
			DEBUG_STACK.push(std::format("{} Failed to write material cache to {}", MATERIAL_CACHE_MANAGER_LOG_TAG, path.string()), DebugStack::Color::COL_ERROR);
		}

		return success;
	}

	void MaterialCacheManager::writeMaterialCacheData(
		std::string id,
		std::string hashId,
		const std::vector<MeshMaterial>& mats,
		size_t hash,
		rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer
	) const {
		writer.Key(id.c_str());
		writer.StartObject();
		for (const auto& mat : mats) {
			writer.Key(mat.name.c_str());
			writer.StartObject();

			writer.Key(MATERIAL_CACHE_MAT_INDEX_ID);
			writer.Uint64(mat.index);

			writer.Key(MATERIAL_CACHE_MAT_PARAMS_ID);
			writer.StartObject();
			for (const auto& [paramName, paramValue] : mat.params) {
				writer.Key(paramValue.name.c_str());
				rapidjson::StringBuffer compactBuf = writeCompactMeshMaterialParam(paramValue.index, paramValue);
				writer.RawValue(compactBuf.GetString(), compactBuf.GetSize(), rapidjson::kObjectType);
			}
			writer.EndObject();

			writer.EndObject();

		}
		writer.EndObject();
		writer.Key(hashId.c_str());
		writer.Uint64(hash);
	}

	rapidjson::StringBuffer MaterialCacheManager::writeCompactMeshMaterialParam(size_t idx, const MeshMaterialParam& mat) const {
		rapidjson::StringBuffer buf;
		rapidjson::Writer<rapidjson::StringBuffer> compactWriter(buf); // one-line writer

		compactWriter.StartObject();
		compactWriter.Key(MATERIAL_CACHE_MAT_PARAM_INDEX_ID);
		compactWriter.Uint64(idx);
		compactWriter.Key(MATERIAL_CACHE_MAT_PARAM_TYPE_ID);
		compactWriter.Uint64(static_cast<uint64_t>(mat.type));
		compactWriter.EndObject();

		return buf;
	}


}