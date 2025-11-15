#include <kbf/data/mesh/parts/part_cache_manager.hpp>

#include <kbf/data/ids/part_cache_ids.hpp>
#include <kbf/data/file/field_parsers.hpp>
#include <kbf/data/ids/format_ids.hpp>

#define PART_CACHE_MANAGER_LOG_TAG "[PartCacheManager]"

#undef GetObject

namespace kbf {

	void PartCacheManager::cache(const ArmourSetWithCharacterSex& armour, const std::vector<MeshPart>& parts, ArmourPiece piece) {
		size_t hash = HashedPartList::hashParts(parts);

		//DEBUG_STACK.push(std::format("{} Caching parts for armour set {} ({}-{}) [{}]. Hash: {}",
		//	PART_CACHE_MANAGER_LOG_TAG,
		//	armour.set.name,
		//	armour.characterFemale ? "F" : "M",
		//	armour.set.female ? "F" : "M",
		//	body ? "BODY" : "LEGS",
		//	hash
		//), DebugStack::Color::DEBUG);

		bool changed = false;
		if (caches.find(armour) != caches.end()) {
			PartCache& existingCache = caches.at(armour);

			//DEBUG_STACK.push(std::format("{} Existing cache found. [BODY: {} | {}] [LEGS: {} | {}]",
			//	PART_CACHE_MANAGER_LOG_TAG,
			//	existingCache.body.getParts().size(),
			//	existingCache.body.getHash(),
			//	existingCache.legs.getParts().size(),
			//	existingCache.legs.getHash()
			//), DebugStack::Color::DEBUG);

			HashedPartList* targetList = nullptr;
			switch (piece) {
			case ArmourPiece::AP_SET:  targetList = &existingCache.set;  break;
			case ArmourPiece::AP_HELM: targetList = &existingCache.helm; break;
			case ArmourPiece::AP_BODY: targetList = &existingCache.body; break;
			case ArmourPiece::AP_ARMS: targetList = &existingCache.arms; break;
			case ArmourPiece::AP_COIL: targetList = &existingCache.coil; break;
			case ArmourPiece::AP_LEGS: targetList = &existingCache.legs; break;
			}

			if (!targetList || targetList->getHash() == hash) return; // No need to update if the hash is the same
			*targetList = HashedPartList{ parts, hash };
			changed = true;
		}
		else {
			PartCache newCache{
				armour,
				(piece == ArmourPiece::AP_SET) ? HashedPartList{ parts, hash } : HashedPartList{},
				(piece == ArmourPiece::AP_HELM) ? HashedPartList{ parts, hash } : HashedPartList{},
				(piece == ArmourPiece::AP_BODY) ? HashedPartList{ parts, hash } : HashedPartList{},
				(piece == ArmourPiece::AP_ARMS) ? HashedPartList{ parts, hash } : HashedPartList{},
				(piece == ArmourPiece::AP_COIL) ? HashedPartList{ parts, hash } : HashedPartList{},
				(piece == ArmourPiece::AP_LEGS) ? HashedPartList{ parts, hash } : HashedPartList{},
			};
			caches.emplace(armour, newCache);
			changed = true;
		}

		if (changed) writeCache(armour);
	}

	bool PartCacheManager::getCacheFromDocument(const rapidjson::Document& doc, ArmourSetWithCharacterSex armour, PartCache& out) const {
		std::vector<MeshPart> setParts{};
		std::vector<MeshPart> helmParts{};
		std::vector<MeshPart> bodyParts{};
		std::vector<MeshPart> armsParts{};
		std::vector<MeshPart> coilParts{};
		std::vector<MeshPart> legsParts{};
		size_t setHash = 0;
		size_t helmHash = 0;
		size_t bodyHash = 0;
		size_t armsHash = 0;
		size_t coilHash = 0;
		size_t legsHash = 0;

		// various mesh parts as objects with name as key
		bool parsed = true;
		parsed &= parseObject(doc, PART_CACHE_SET_ID, PART_CACHE_SET_ID);
		if (parsed) parsed &= loadPartCacheList(doc[PART_CACHE_SET_ID], setParts);
		parsed &= parseObject(doc, PART_CACHE_HELM_ID, PART_CACHE_HELM_ID);
		if (parsed) parsed &= loadPartCacheList(doc[PART_CACHE_HELM_ID], helmParts);
		parsed &= parseObject(doc, PART_CACHE_BODY_ID, PART_CACHE_BODY_ID);
		if (parsed) parsed &= loadPartCacheList(doc[PART_CACHE_BODY_ID], bodyParts);
		parsed &= parseObject(doc, PART_CACHE_ARMS_ID, PART_CACHE_ARMS_ID);
		if (parsed) parsed &= loadPartCacheList(doc[PART_CACHE_ARMS_ID], armsParts);
		parsed &= parseObject(doc, PART_CACHE_COIL_ID, PART_CACHE_COIL_ID);
		if (parsed) parsed &= loadPartCacheList(doc[PART_CACHE_COIL_ID], coilParts);
		parsed &= parseObject(doc, PART_CACHE_LEGS_ID, PART_CACHE_LEGS_ID);
		if (parsed) parsed &= loadPartCacheList(doc[PART_CACHE_LEGS_ID], legsParts);

		parsed &= parseUint64(doc, PART_CACHE_SET_HASH_ID, PART_CACHE_SET_HASH_ID, &setHash);
		parsed &= parseUint64(doc, PART_CACHE_HELM_HASH_ID, PART_CACHE_HELM_HASH_ID, &helmHash);
		parsed &= parseUint64(doc, PART_CACHE_BODY_HASH_ID, PART_CACHE_BODY_HASH_ID, &bodyHash);
		parsed &= parseUint64(doc, PART_CACHE_ARMS_HASH_ID, PART_CACHE_ARMS_HASH_ID, &armsHash);
		parsed &= parseUint64(doc, PART_CACHE_COIL_HASH_ID, PART_CACHE_COIL_HASH_ID, &coilHash);
		parsed &= parseUint64(doc, PART_CACHE_LEGS_HASH_ID, PART_CACHE_LEGS_HASH_ID, &legsHash);

		if (parsed) {
			out = PartCache{
				armour,
				HashedPartList{ setParts,  setHash  },
				HashedPartList{ helmParts, helmHash },
				HashedPartList{ bodyParts, bodyHash },
				HashedPartList{ armsParts, armsHash },
				HashedPartList{ coilParts, coilHash },
				HashedPartList{ legsParts, legsHash }
			};
		}

		return parsed;
	}

	bool PartCacheManager::loadPartCacheList(const rapidjson::Value& object, std::vector<MeshPart>& out) const {
		bool parsed = true;
		for (const auto& part : object.GetObject()) {
			MeshPart meshPart{};
			meshPart.name = part.name.GetString();
			parsed &= part.value.IsObject();
			if (parsed) {
				std::string typeStr;
				parsed &= parseUint64(part.value, PART_CACHE_PART_INDEX_ID, meshPart.name + "." + PART_CACHE_PART_INDEX_ID, &meshPart.index);
			}
			if (parsed) out.push_back(meshPart);
		}
		return parsed;
	}


	bool PartCacheManager::writeCacheJson(const std::filesystem::path& path, const PartCache& out) const {
		DEBUG_STACK.push(std::format("{} Writing Part Cache @ \"{}\"...", PART_CACHE_MANAGER_LOG_TAG, path.string()), DebugStack::Color::COL_INFO);

		rapidjson::StringBuffer s;
		rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(s);

		writer.StartObject();
		writer.Key(FORMAT_VERSION_ID);
		writer.String(KBF_VERSION);
		writePartCacheData(PART_CACHE_SET_ID, PART_CACHE_SET_HASH_ID, out.set.getParts(), out.set.getHash(), writer);
		writePartCacheData(PART_CACHE_HELM_ID, PART_CACHE_HELM_HASH_ID, out.helm.getParts(), out.helm.getHash(), writer);
		writePartCacheData(PART_CACHE_BODY_ID, PART_CACHE_BODY_HASH_ID, out.body.getParts(), out.body.getHash(), writer);
		writePartCacheData(PART_CACHE_ARMS_ID, PART_CACHE_ARMS_HASH_ID, out.arms.getParts(), out.arms.getHash(), writer);
		writePartCacheData(PART_CACHE_COIL_ID, PART_CACHE_COIL_HASH_ID, out.coil.getParts(), out.coil.getHash(), writer);
		writePartCacheData(PART_CACHE_LEGS_ID, PART_CACHE_LEGS_HASH_ID, out.legs.getParts(), out.legs.getHash(), writer);
		writer.EndObject();

		bool success = writeJsonFile(path.string(), s.GetString());

		if (!success) {
			DEBUG_STACK.push(std::format("{} Failed to write part cache to {}", PART_CACHE_MANAGER_LOG_TAG, path.string()), DebugStack::Color::COL_ERROR);
		}

		return success;
	}

	void PartCacheManager::writePartCacheData(
		std::string id,
		std::string hashId,
		const std::vector<MeshPart>& parts,
		size_t hash,
		rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer
	) const {
		writer.Key(id.c_str());
		writer.StartObject();
		for (const auto& part : parts) {
			writer.Key(part.name.c_str());

			rapidjson::StringBuffer compactBuf = writeCompactRemovedPart(part);
			writer.RawValue(compactBuf.GetString(), compactBuf.GetSize(), rapidjson::kObjectType);
		}
		writer.EndObject();
		writer.Key(hashId.c_str());
		writer.Uint64(hash);
	}

	rapidjson::StringBuffer PartCacheManager::writeCompactRemovedPart(const MeshPart& part) const {
		rapidjson::StringBuffer buf;
		rapidjson::Writer<rapidjson::StringBuffer> compactWriter(buf); // one-line writer

		compactWriter.StartObject();
		compactWriter.Key(PART_CACHE_PART_INDEX_ID);
		compactWriter.Uint64(part.index);
		compactWriter.EndObject();

		return buf;
	}


}