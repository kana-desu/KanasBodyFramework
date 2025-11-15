#include <kbf/data/bones/bone_cache_manager.hpp>

#include <kbf/data/ids/bone_cache_ids.hpp>
#include <kbf/data/file/field_parsers.hpp>
#include <kbf/data/ids/format_ids.hpp>

#define BONE_CACHE_MANAGER_LOG_TAG "[BoneCacheManager]"

namespace kbf {

	void BoneCacheManager::cache(const ArmourSetWithCharacterSex& armour, const std::vector<std::string>& bones, ArmourPiece piece) {
		size_t hash = HashedBoneList::hashBones(bones);

		//DEBUG_STACK.push(std::format("{} Caching bones for armour set {} ({}-{}) [{}]. Hash: {}",
		//	BONE_CACHE_MANAGER_LOG_TAG,
		//	armour.set.name,
		//	armour.characterFemale ? "F" : "M",
		//	armour.set.female ? "F" : "M",
		//	body ? "BODY" : "LEGS",
		//	hash
		//), DebugStack::Color::DEBUG);

		bool changed = false;
		if (caches.find(armour) != caches.end()) {
			BoneCache& existingCache = caches.at(armour);

			//DEBUG_STACK.push(std::format("{} Existing cache found. [BODY: {} | {}] [LEGS: {} | {}]",
			//	BONE_CACHE_MANAGER_LOG_TAG,
			//	existingCache.body.getBones().size(),
			//	existingCache.body.getHash(),
			//	existingCache.legs.getBones().size(),
			//	existingCache.legs.getHash()
			//), DebugStack::Color::DEBUG);

			HashedBoneList* targetList = nullptr;
			switch (piece) {
			case ArmourPiece::AP_SET:  targetList = &existingCache.set;  break;
			case ArmourPiece::AP_HELM: targetList = &existingCache.helm; break;
			case ArmourPiece::AP_BODY: targetList = &existingCache.body; break;
			case ArmourPiece::AP_ARMS: targetList = &existingCache.arms; break;
			case ArmourPiece::AP_COIL: targetList = &existingCache.coil; break;
			case ArmourPiece::AP_LEGS: targetList = &existingCache.legs; break;
			}

			if (!targetList || targetList->getHash() == hash) return; // No need to update if the hash is the same
			*targetList = HashedBoneList{ bones, hash };
			changed = true;
		}
		else {
			BoneCache newCache{
				armour,
				(piece == ArmourPiece::AP_SET) ? HashedBoneList{ bones, hash } : HashedBoneList{},
				(piece == ArmourPiece::AP_HELM) ? HashedBoneList{ bones, hash } : HashedBoneList{},
				(piece == ArmourPiece::AP_BODY) ? HashedBoneList{ bones, hash } : HashedBoneList{},
				(piece == ArmourPiece::AP_ARMS) ? HashedBoneList{ bones, hash } : HashedBoneList{},
				(piece == ArmourPiece::AP_COIL) ? HashedBoneList{ bones, hash } : HashedBoneList{},
				(piece == ArmourPiece::AP_LEGS) ? HashedBoneList{ bones, hash } : HashedBoneList{},
			};
			caches.emplace(armour, newCache);
			changed = true;
		}

		if (changed) writeCache(armour);
	}

	bool BoneCacheManager::boneExists(const ArmourSetWithCharacterSex& armour, ArmourPiece piece, const std::string& boneName) const {
		if (caches.find(armour) == caches.end()) return false;
		const BoneCache& cache = caches.at(armour);

		switch (piece) {
		case ArmourPiece::AP_SET:  return cache.set.hasBone(boneName);
		case ArmourPiece::AP_HELM: return cache.helm.hasBone(boneName);
		case ArmourPiece::AP_BODY: return cache.body.hasBone(boneName);
		case ArmourPiece::AP_ARMS: return cache.arms.hasBone(boneName);
		case ArmourPiece::AP_COIL: return cache.coil.hasBone(boneName);
		case ArmourPiece::AP_LEGS: return cache.legs.hasBone(boneName);
		}

		return false;
	}

	bool BoneCacheManager::getCacheFromDocument(const rapidjson::Document& doc, ArmourSetWithCharacterSex armour, BoneCache& out) const {
		std::vector<std::string> setBones{};
		std::vector<std::string> helmBones{};
		std::vector<std::string> bodyBones{};
		std::vector<std::string> armsBones{};
		std::vector<std::string> coilBones{};
		std::vector<std::string> legsBones{};
		size_t setHash = 0;
		size_t helmHash = 0;
		size_t bodyHash = 0;
		size_t armsHash = 0;
		size_t coilHash = 0;
		size_t legsHash = 0;

		bool parsed = true;
		parsed &= parseStringArray(doc, BONE_CACHE_SET_ID, BONE_CACHE_SET_ID, &setBones);
		parsed &= parseUint64(doc, BONE_CACHE_SET_HASH_ID, BONE_CACHE_SET_HASH_ID, &setHash);
		parsed &= parseStringArray(doc, BONE_CACHE_HELM_ID, BONE_CACHE_HELM_ID, &helmBones);
		parsed &= parseUint64(doc, BONE_CACHE_HELM_HASH_ID, BONE_CACHE_HELM_HASH_ID, &helmHash);
		parsed &= parseStringArray(doc, BONE_CACHE_BODY_ID, BONE_CACHE_BODY_ID, &bodyBones);
		parsed &= parseUint64(doc, BONE_CACHE_BODY_HASH_ID, BONE_CACHE_BODY_HASH_ID, &bodyHash);
		parsed &= parseStringArray(doc, BONE_CACHE_ARMS_ID, BONE_CACHE_ARMS_ID, &armsBones);
		parsed &= parseUint64(doc, BONE_CACHE_ARMS_HASH_ID, BONE_CACHE_ARMS_HASH_ID, &armsHash);
		parsed &= parseStringArray(doc, BONE_CACHE_COIL_ID, BONE_CACHE_COIL_ID, &coilBones);
		parsed &= parseUint64(doc, BONE_CACHE_COIL_HASH_ID, BONE_CACHE_COIL_HASH_ID, &coilHash);
		parsed &= parseStringArray(doc, BONE_CACHE_LEGS_ID, BONE_CACHE_LEGS_ID, &legsBones);
		parsed &= parseUint64(doc, BONE_CACHE_LEGS_HASH_ID, BONE_CACHE_LEGS_HASH_ID, &legsHash);

		if (parsed) {
			out = BoneCache{
				armour,
				HashedBoneList{ setBones,  setHash  },
				HashedBoneList{ helmBones, helmHash },
				HashedBoneList{ bodyBones, bodyHash },
				HashedBoneList{ armsBones, armsHash },
				HashedBoneList{ coilBones, coilHash },
				HashedBoneList{ legsBones, legsHash }
			};
		}

		return parsed;
	}

	bool BoneCacheManager::writeCacheJson(const std::filesystem::path& path, const BoneCache& out) const {
		DEBUG_STACK.push(std::format("{} Writing Bone Cache @ \"{}\"...", BONE_CACHE_MANAGER_LOG_TAG, path.string()), DebugStack::Color::COL_INFO);

		rapidjson::StringBuffer s;
		rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(s);

		writer.StartObject();
		writer.Key(FORMAT_VERSION_ID);
		writer.String(KBF_VERSION);
		writeBoneCacheData(BONE_CACHE_SET_ID, BONE_CACHE_SET_HASH_ID, out.set.getBones(), out.set.getHash(), writer);
		writeBoneCacheData(BONE_CACHE_HELM_ID, BONE_CACHE_HELM_HASH_ID, out.helm.getBones(), out.helm.getHash(), writer);
		writeBoneCacheData(BONE_CACHE_BODY_ID, BONE_CACHE_BODY_HASH_ID, out.body.getBones(), out.body.getHash(), writer);
		writeBoneCacheData(BONE_CACHE_ARMS_ID, BONE_CACHE_ARMS_HASH_ID, out.arms.getBones(), out.arms.getHash(), writer);
		writeBoneCacheData(BONE_CACHE_COIL_ID, BONE_CACHE_COIL_HASH_ID, out.coil.getBones(), out.coil.getHash(), writer);
		writeBoneCacheData(BONE_CACHE_LEGS_ID, BONE_CACHE_LEGS_HASH_ID, out.legs.getBones(), out.legs.getHash(), writer);
		writer.EndObject();

		bool success = writeJsonFile(path.string(), s.GetString());

		if (!success) {
			DEBUG_STACK.push(std::format("{} Failed to write bone cache to {}", BONE_CACHE_MANAGER_LOG_TAG, path.string()), DebugStack::Color::COL_ERROR);
		}

		return success;
	}

	void BoneCacheManager::writeBoneCacheData(
		std::string id,
		std::string hashId,
		const std::vector<std::string>& bones,
		size_t hash,
		rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer
	) const {
		writer.Key(id.c_str());
		writer.StartArray();
		for (const auto& bone : bones) {
			writer.String(bone.c_str());
		}
		writer.EndArray();
		writer.Key(hashId.c_str());
		writer.Uint64(hash);
	}

}