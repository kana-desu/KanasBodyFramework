#include <kbf/data/bones/bone_cache_manager.hpp>

#include <kbf/debug/debug_stack.hpp>
#include <kbf/data/field_parsers.hpp>
#include <kbf/data/ids/bone_cache_ids.hpp>
#include <kbf/data/armour/armour_list.hpp>

#include <fstream>

#define BONE_CACHE_MANAGER_LOG_TAG "[BoneCacheManager]"

namespace kbf {

	BoneCacheManager::BoneCacheManager(const std::filesystem::path& dataBasePath) : dataBasePath{ dataBasePath } {
		verifyDirectoryExists();
	}

	bool BoneCacheManager::loadBoneCaches() {
		verifyDirectoryExists();
		caches.clear();

		bool hasFailure = false;

		DEBUG_STACK.push(std::format("{} Loading Bone Caches...", BONE_CACHE_MANAGER_LOG_TAG), DebugStack::Color::INFO);

		// Load all presets from the preset directory
		for (const auto& entry : std::filesystem::directory_iterator(boneCachesPath)) {
			if (entry.is_regular_file() && entry.path().extension() == ".json") {
				BoneCache boneCache;

				if (loadBoneCache(entry.path(), &boneCache)) {
					caches.emplace(boneCache.armour, boneCache);
					DEBUG_STACK.push(std::format("{} Loaded bone cache: {} ({}-{})",
						BONE_CACHE_MANAGER_LOG_TAG,
						boneCache.armour.set.name,
						boneCache.armour.characterFemale ? "F" : "M",
						boneCache.armour.set.female ? "F" : "M"
					), DebugStack::Color::SUCCESS);
				}
				else {
					hasFailure = true;
				}
			}
		}
		return hasFailure;
	}


	void BoneCacheManager::cacheBones(const ArmourSetWithCharacterSex& armour, const std::vector<std::string>& bones, ArmourPiece piece) {
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
				(piece == ArmourPiece::AP_SET)  ? HashedBoneList{ bones, hash } : HashedBoneList{},
				(piece == ArmourPiece::AP_HELM) ? HashedBoneList{ bones, hash } : HashedBoneList{},
				(piece == ArmourPiece::AP_BODY) ? HashedBoneList{ bones, hash } : HashedBoneList{},
				(piece == ArmourPiece::AP_ARMS) ? HashedBoneList{ bones, hash } : HashedBoneList{},
				(piece == ArmourPiece::AP_COIL) ? HashedBoneList{ bones, hash } : HashedBoneList{},
				(piece == ArmourPiece::AP_LEGS) ? HashedBoneList{ bones, hash } : HashedBoneList{},
			};
			caches.emplace(armour, newCache);
			changed = true;
		}

		if (changed) writeBoneCache(armour);
	}

	const BoneCache* BoneCacheManager::getCachedBones(const ArmourSetWithCharacterSex& armour) const {
		if (caches.find(armour) == caches.end()) return nullptr;
		return &caches.at(armour);
	}

	std::vector<ArmourSetWithCharacterSex> BoneCacheManager::getCachedArmourSets() const {
		std::vector<ArmourSetWithCharacterSex> armourSets;
		for (const auto& [key, _] : caches) {
			armourSets.push_back(key);
		}
		return armourSets;
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

	void BoneCacheManager::verifyDirectoryExists() const {
		if (!std::filesystem::exists(boneCachesPath)) {
			std::filesystem::create_directories(boneCachesPath);
		}
	}

	bool BoneCacheManager::loadBoneCache(const std::filesystem::path& path, BoneCache* out) {
		assert(out != nullptr);

		DEBUG_STACK.push(std::format("{} Loading Bone Cache @ \"{}\"...", BONE_CACHE_MANAGER_LOG_TAG, path.string()), DebugStack::Color::INFO);

		ArmourSetWithCharacterSex armour;

		if (!getBoneCacheArmourSet(path.stem().string(), &armour)) {
			DEBUG_STACK.push(std::format("{} Bone cache @ {} has an invalid name. Skipping...", BONE_CACHE_MANAGER_LOG_TAG, path.string()), DebugStack::Color::WARNING);
			return false;
		}

		rapidjson::Document presetDoc = loadCacheJson(path.string());
		if (!presetDoc.IsObject() || presetDoc.HasParseError()) return false;

		std::vector<std::string> setBones{};
		std::vector<std::string> helmBones{};
		std::vector<std::string> bodyBones{};
		std::vector<std::string> armsBones{};
		std::vector<std::string> coilBones{};
		std::vector<std::string> legsBones{};
		size_t setHash  = 0;
		size_t helmHash = 0;
		size_t bodyHash = 0;
		size_t armsHash = 0;
		size_t coilHash = 0;
		size_t legsHash = 0;

		bool parsed = true;
		parsed &= parseStringArray(presetDoc, BONE_CACHE_SET_ID, BONE_CACHE_SET_ID, &setBones);
		parsed &= parseUint64(presetDoc, BONE_CACHE_SET_HASH_ID, BONE_CACHE_SET_HASH_ID, &setHash);
		parsed &= parseStringArray(presetDoc, BONE_CACHE_HELM_ID, BONE_CACHE_HELM_ID, &helmBones);
		parsed &= parseUint64(presetDoc, BONE_CACHE_HELM_HASH_ID, BONE_CACHE_HELM_HASH_ID, &helmHash);
		parsed &= parseStringArray(presetDoc, BONE_CACHE_BODY_ID, BONE_CACHE_BODY_ID, &bodyBones);
		parsed &= parseUint64(presetDoc, BONE_CACHE_BODY_HASH_ID, BONE_CACHE_BODY_HASH_ID, &bodyHash);
		parsed &= parseStringArray(presetDoc, BONE_CACHE_ARMS_ID, BONE_CACHE_ARMS_ID, &armsBones);
		parsed &= parseUint64(presetDoc, BONE_CACHE_ARMS_HASH_ID, BONE_CACHE_ARMS_HASH_ID, &armsHash);
		parsed &= parseStringArray(presetDoc, BONE_CACHE_COIL_ID, BONE_CACHE_COIL_ID, &coilBones);
		parsed &= parseUint64(presetDoc, BONE_CACHE_COIL_HASH_ID, BONE_CACHE_COIL_HASH_ID, &coilHash);
		parsed &= parseStringArray(presetDoc, BONE_CACHE_LEGS_ID, BONE_CACHE_LEGS_ID, &legsBones);
		parsed &= parseUint64(presetDoc, BONE_CACHE_LEGS_HASH_ID, BONE_CACHE_LEGS_HASH_ID, &legsHash);

		if (!parsed) {
			DEBUG_STACK.push(std::format("{} Failed to parse bone cache \"{}\". One or more required values were missing. Please rectify or remove the file.", BONE_CACHE_MANAGER_LOG_TAG, path.string()), DebugStack::Color::ERROR);
		}
		else {
			*out = BoneCache{
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

	rapidjson::Document BoneCacheManager::loadCacheJson(const std::string& path) const {
		bool exists = std::filesystem::exists(path);
		if (!exists) {
			DEBUG_STACK.push(std::format("{} Could not find json at {}. Please rectify or delete the file.", BONE_CACHE_MANAGER_LOG_TAG, path), DebugStack::Color::ERROR);
		}

		std::string json = readJsonFile(path);

		rapidjson::Document config;
		config.Parse(json.c_str());

		if (!config.IsObject() || config.HasParseError()) {
			DEBUG_STACK.push(std::format("{} Failed to parse json at {}. Please rectify or delete the file.", BONE_CACHE_MANAGER_LOG_TAG, path), DebugStack::Color::ERROR);
		}

		return config;
	}

	std::string BoneCacheManager::readJsonFile(const std::string& path) const {
		std::ifstream file{ path, std::ios::ate };

		if (!file.is_open()) {
			const std::string error_pth_out{ path };
			DEBUG_STACK.push(std::format("{} Could not open json file for read at {}", BONE_CACHE_MANAGER_LOG_TAG, error_pth_out), DebugStack::Color::ERROR);
			return "";
		}

		size_t fileSize = static_cast<size_t>(file.tellg());
		std::string buffer(fileSize, ' ');
		file.seekg(0);
		file.read(buffer.data(), fileSize);

		file.close();

		return buffer;
	}

	bool BoneCacheManager::writeJsonFile(std::string path, const std::string& json) const {
		std::ofstream file(path, std::ios::trunc);
		if (file.is_open()) {
			file << json;

			try { file.close(); }
			catch (...) { return false; }

			return true;
		}

		return false;
	}

	bool BoneCacheManager::writeBoneCache(const ArmourSetWithCharacterSex& armour) const {
		if (caches.find(armour) == caches.end()) {
			DEBUG_STACK.push(std::format("{} Tried to write bone cache for armour set {} ({}-{}), but no cache data exists.", 
				BONE_CACHE_MANAGER_LOG_TAG,
				armour.set.name,
				armour.characterFemale ? "F" : "M",
				armour.set.female ? "F" : "M"
			), DebugStack::Color::ERROR);
			return false;
		}

		return writeBoneCacheJson(boneCachesPath / getBoneCacheFilename(armour), caches.at(armour));
	}

	bool BoneCacheManager::writeBoneCacheJson(const std::filesystem::path& path, const BoneCache& out) const {
		DEBUG_STACK.push(std::format("{} Writing Bone Cache @ \"{}\"...", BONE_CACHE_MANAGER_LOG_TAG, path.string()), DebugStack::Color::INFO);

		rapidjson::StringBuffer s;
		rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(s);

		writer.StartObject();
		writeBoneCacheData(BONE_CACHE_SET_ID,  BONE_CACHE_SET_HASH_ID,  out.set.getBones(),  out.set.getHash(),  writer);
		writeBoneCacheData(BONE_CACHE_HELM_ID, BONE_CACHE_HELM_HASH_ID, out.helm.getBones(), out.helm.getHash(), writer);
		writeBoneCacheData(BONE_CACHE_BODY_ID, BONE_CACHE_BODY_HASH_ID, out.body.getBones(), out.body.getHash(), writer);
		writeBoneCacheData(BONE_CACHE_ARMS_ID, BONE_CACHE_ARMS_HASH_ID, out.arms.getBones(), out.arms.getHash(), writer);
		writeBoneCacheData(BONE_CACHE_COIL_ID, BONE_CACHE_COIL_HASH_ID, out.coil.getBones(), out.coil.getHash(), writer);
		writeBoneCacheData(BONE_CACHE_LEGS_ID, BONE_CACHE_LEGS_HASH_ID, out.legs.getBones(), out.legs.getHash(), writer);
		writer.EndObject();

		bool success = writeJsonFile(path.string(), s.GetString());

		if (!success) {
			DEBUG_STACK.push(std::format("{} Failed to write bone cache to {}", BONE_CACHE_MANAGER_LOG_TAG, path.string()), DebugStack::Color::ERROR);
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

	std::string BoneCacheManager::getBoneCacheFilename(const ArmourSetWithCharacterSex& armour) const {
		std::string characterSex = (armour.characterFemale ? "F" : "M");
		std::string sex = (armour.set.female ? "F" : "M");

		// Replace any illegal filename characters in the armour set name with underscores
		std::string armourNameSanitized = armour.set.name;
		std::replace(armourNameSanitized.begin(), armourNameSanitized.end(), '/', '_');

		return std::format("{}-{}~{}.json", characterSex, sex, armourNameSanitized);
	}

	bool BoneCacheManager::getBoneCacheArmourSet(const std::string& filename, ArmourSetWithCharacterSex* out) const {
		assert(out != nullptr);

		if (filename.size() < 4)
			return false;

		char characterSex = filename[0];
		if (characterSex != 'M' && characterSex != 'F')
			return false;

		char sex = filename[2];
		if (sex != 'M' && sex != 'F')
			return false;

		bool characterFemale = (characterSex == 'F');
		bool female = (sex == 'F');
		std::string armourName = filename.substr(4);
		armourName = armourName.substr(0, armourName.find_last_of('.')); // Remove json file extension

		// Restore illegal characters
		std::replace(armourName.begin(), armourName.end(), '_', '/');

		if (!ArmourList::isValidArmourSet(armourName, female))
			return false;

		*out = ArmourSetWithCharacterSex{ ArmourSet{ armourName, female }, characterFemale };
		return true;
	}

}