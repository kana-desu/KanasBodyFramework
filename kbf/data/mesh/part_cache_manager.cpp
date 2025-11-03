#include <kbf/data/mesh/part_cache_manager.hpp>

#include <kbf/debug/debug_stack.hpp>
#include <kbf/data/file/field_parsers.hpp>
#include <kbf/data/file/kbf_file_upgrader.hpp>
#include <kbf/data/ids/part_cache_ids.hpp>
#include <kbf/data/armour/armour_list.hpp>

#include <fstream>

#define PART_CACHE_MANAGER_LOG_TAG "[PartCacheManager]"

namespace kbf {

	PartCacheManager::PartCacheManager(const std::filesystem::path& dataBasePath) : dataBasePath{ dataBasePath } {
		verifyDirectoryExists();
	}

	bool PartCacheManager::loadPartCaches() {
		verifyDirectoryExists();

		bool hasFailure = false;

		DEBUG_STACK.push(std::format("{} Loading Part Caches...", PART_CACHE_MANAGER_LOG_TAG), DebugStack::Color::INFO);

		// Load all presets from the preset directory
		for (const auto& entry : std::filesystem::directory_iterator(partCachesPath)) {
			if (entry.is_regular_file() && entry.path().extension() == ".json") {
				PartCache partCache;

				if (loadPartCache(entry.path(), &partCache)) {
					caches.emplace(partCache.armour, partCache);
					DEBUG_STACK.push(std::format("{} Loaded part cache: {} ({}-{})",
						PART_CACHE_MANAGER_LOG_TAG,
						partCache.armour.set.name,
						partCache.armour.characterFemale ? "F" : "M",
						partCache.armour.set.female ? "F" : "M"
					), DebugStack::Color::SUCCESS);
				}
				else {
					hasFailure = true;
				}
			}
		}
		return hasFailure;
	}


	void PartCacheManager::cacheParts(const ArmourSetWithCharacterSex& armour, const std::vector<MeshPart>& parts, ArmourPiece piece) {
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
				(piece == ArmourPiece::AP_SET)  ? HashedPartList{ parts, hash } : HashedPartList{},
				(piece == ArmourPiece::AP_HELM) ? HashedPartList{ parts, hash } : HashedPartList{},
				(piece == ArmourPiece::AP_BODY) ? HashedPartList{ parts, hash } : HashedPartList{},
				(piece == ArmourPiece::AP_ARMS) ? HashedPartList{ parts, hash } : HashedPartList{},
				(piece == ArmourPiece::AP_COIL) ? HashedPartList{ parts, hash } : HashedPartList{},
				(piece == ArmourPiece::AP_LEGS) ? HashedPartList{ parts, hash } : HashedPartList{},
			};
			caches.emplace(armour, newCache);
			changed = true;
		}

		if (changed) writePartCache(armour);
	}

	const PartCache* PartCacheManager::getCachedParts(const ArmourSetWithCharacterSex& armour) const {
		if (caches.find(armour) == caches.end()) return nullptr;
		return &caches.at(armour);
	}

	std::vector<ArmourSetWithCharacterSex> PartCacheManager::getCachedArmourSets() const {
		std::vector<ArmourSetWithCharacterSex> armourSets;
		for (const auto& [key, _] : caches) {
			armourSets.push_back(key);
		}
		return armourSets;
	}

	void PartCacheManager::verifyDirectoryExists() const {
		if (!std::filesystem::exists(partCachesPath)) {
			std::filesystem::create_directories(partCachesPath);
		}
	}

	bool PartCacheManager::loadPartCache(const std::filesystem::path& path, PartCache* out) {
		assert(out != nullptr);

		DEBUG_STACK.push(std::format("{} Loading Part Cache @ \"{}\"...", PART_CACHE_MANAGER_LOG_TAG, path.string()), DebugStack::Color::INFO);

		ArmourSetWithCharacterSex armour;

		if (!getPartCacheArmourSet(path.stem().string(), &armour)) {
			DEBUG_STACK.push(std::format("{} Part cache @ {} has an invalid name. Skipping...", PART_CACHE_MANAGER_LOG_TAG, path.string()), DebugStack::Color::WARNING);
			return false;
		}

		rapidjson::Document partDoc = loadCacheJson(path.string());
		if (!partDoc.IsObject() || partDoc.HasParseError()) return false;

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
		parsed &= parseObject(partDoc, PART_CACHE_SET_ID, PART_CACHE_SET_ID);
		if (parsed) parsed &= loadPartCacheList(partDoc[PART_CACHE_SET_ID], &setParts);
		parsed &= parseObject(partDoc, PART_CACHE_HELM_ID, PART_CACHE_HELM_ID);
		if (parsed) parsed &= loadPartCacheList(partDoc[PART_CACHE_HELM_ID], &helmParts);
		parsed &= parseObject(partDoc, PART_CACHE_BODY_ID, PART_CACHE_BODY_ID);
		if (parsed) parsed &= loadPartCacheList(partDoc[PART_CACHE_BODY_ID], &bodyParts);
		parsed &= parseObject(partDoc, PART_CACHE_ARMS_ID, PART_CACHE_ARMS_ID);
		if (parsed) parsed &= loadPartCacheList(partDoc[PART_CACHE_ARMS_ID], &armsParts);
		parsed &= parseObject(partDoc, PART_CACHE_COIL_ID, PART_CACHE_COIL_ID);
		if (parsed) parsed &= loadPartCacheList(partDoc[PART_CACHE_COIL_ID], &coilParts);
		parsed &= parseObject(partDoc, PART_CACHE_LEGS_ID, PART_CACHE_LEGS_ID);
		if (parsed) parsed &= loadPartCacheList(partDoc[PART_CACHE_LEGS_ID], &legsParts);

		parsed &= parseUint64(partDoc, PART_CACHE_SET_HASH_ID,  PART_CACHE_SET_HASH_ID,  &setHash);
		parsed &= parseUint64(partDoc, PART_CACHE_HELM_HASH_ID, PART_CACHE_HELM_HASH_ID, &helmHash);
		parsed &= parseUint64(partDoc, PART_CACHE_BODY_HASH_ID, PART_CACHE_BODY_HASH_ID, &bodyHash);
		parsed &= parseUint64(partDoc, PART_CACHE_ARMS_HASH_ID, PART_CACHE_ARMS_HASH_ID, &armsHash);
		parsed &= parseUint64(partDoc, PART_CACHE_COIL_HASH_ID, PART_CACHE_COIL_HASH_ID, &coilHash);
		parsed &= parseUint64(partDoc, PART_CACHE_LEGS_HASH_ID, PART_CACHE_LEGS_HASH_ID, &legsHash);

		if (!parsed) {
			DEBUG_STACK.push(std::format("{} Failed to parse part cache \"{}\". One or more required values were missing. Please rectify or remove the file.", PART_CACHE_MANAGER_LOG_TAG, path.string()), DebugStack::Color::ERROR);
		}
		else {
			*out = PartCache{
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

	bool PartCacheManager::loadPartCacheList(const rapidjson::Value& object, std::vector<MeshPart>* out) const {
		assert(out != nullptr);

		bool parsed = true;
		for (const auto& part : object.GetObject()) {
			MeshPart meshPart{};
			meshPart.name = part.name.GetString();
			parsed &= part.value.IsObject();
			if (parsed) {
				std::string typeStr;
				parsed &= parseString(part.value, PART_CACHE_PART_TYPE_ID, meshPart.name + "." + PART_CACHE_PART_TYPE_ID, &typeStr);
				parsed &= parseUint64(part.value, PART_CACHE_PART_INDEX_ID, meshPart.name + "." + PART_CACHE_PART_INDEX_ID, &meshPart.index);
				if (typeStr == "PART_GROUP") {
					meshPart.type = MeshPartType::PART_GROUP;
				}
				else if (typeStr == "MATERIAL") {
					meshPart.type = MeshPartType::MATERIAL;
				}
				else {
					DEBUG_STACK.push(std::format("{} Part cache has an invalid part type \"{}\" for part \"{}\". Skipping...", PART_CACHE_MANAGER_LOG_TAG, typeStr, meshPart.name), DebugStack::Color::WARNING);
					parsed = false;
				}
			}
			if (parsed) out->push_back(meshPart);
		}
		return parsed;
	}

	rapidjson::Document PartCacheManager::loadCacheJson(const std::string& path) const {
		bool exists = std::filesystem::exists(path);
		if (!exists) {
			DEBUG_STACK.push(std::format("{} Could not find json at {}. Please rectify or delete the file.", PART_CACHE_MANAGER_LOG_TAG, path), DebugStack::Color::ERROR);
		}

		std::string json = readJsonFile(path);

		rapidjson::Document config;
		config.Parse(json.c_str());

		if (!config.IsObject() || config.HasParseError()) {
			DEBUG_STACK.push(std::format("{} Failed to parse json at {}. Please rectify or delete the file.", PART_CACHE_MANAGER_LOG_TAG, path), DebugStack::Color::ERROR);
		}

		return config;
	}

	std::string PartCacheManager::readJsonFile(const std::string& path) const {
		std::ifstream file{ path, std::ios::ate };

		if (!file.is_open()) {
			const std::string error_pth_out{ path };
			DEBUG_STACK.push(std::format("{} Could not open json file for read at {}", PART_CACHE_MANAGER_LOG_TAG, error_pth_out), DebugStack::Color::ERROR);
			return "";
		}

		size_t fileSize = static_cast<size_t>(file.tellg());
		std::string buffer(fileSize, ' ');
		file.seekg(0);
		file.read(buffer.data(), fileSize);

		file.close();

		return buffer;
	}

	bool PartCacheManager::writeJsonFile(std::string path, const std::string& json) const {
		std::ofstream file(path, std::ios::trunc);
		if (file.is_open()) {
			file << json;

			try { file.close(); }
			catch (...) { return false; }

			return true;
		}

		return false;
	}

	bool PartCacheManager::writePartCache(const ArmourSetWithCharacterSex& armour) const {
		if (caches.find(armour) == caches.end()) {
			DEBUG_STACK.push(std::format("{} Tried to write part cache for armour set {} ({}-{}), but no cache data exists.",
				PART_CACHE_MANAGER_LOG_TAG,
				armour.set.name,
				armour.characterFemale ? "F" : "M",
				armour.set.female ? "F" : "M"
			), DebugStack::Color::ERROR);
			return false;
		}

		return writePartCacheJson(partCachesPath / getPartCacheFilename(armour), caches.at(armour));
	}

	bool PartCacheManager::writePartCacheJson(const std::filesystem::path& path, const PartCache& out) const {
		DEBUG_STACK.push(std::format("{} Writing Part Cache @ \"{}\"...", PART_CACHE_MANAGER_LOG_TAG, path.string()), DebugStack::Color::INFO);

		rapidjson::StringBuffer s;
		rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(s);

		writer.StartObject();
		writePartCacheData(PART_CACHE_SET_ID,  PART_CACHE_SET_HASH_ID,  out.set.getParts(),  out.set.getHash(),  writer);
		writePartCacheData(PART_CACHE_HELM_ID, PART_CACHE_HELM_HASH_ID, out.helm.getParts(), out.helm.getHash(), writer);
		writePartCacheData(PART_CACHE_BODY_ID, PART_CACHE_BODY_HASH_ID, out.body.getParts(), out.body.getHash(), writer);
		writePartCacheData(PART_CACHE_ARMS_ID, PART_CACHE_ARMS_HASH_ID, out.arms.getParts(), out.arms.getHash(), writer);
		writePartCacheData(PART_CACHE_COIL_ID, PART_CACHE_COIL_HASH_ID, out.coil.getParts(), out.coil.getHash(), writer);
		writePartCacheData(PART_CACHE_LEGS_ID, PART_CACHE_LEGS_HASH_ID, out.legs.getParts(), out.legs.getHash(), writer);
		writer.EndObject();

		bool success = writeJsonFile(path.string(), s.GetString());

		if (!success) {
			DEBUG_STACK.push(std::format("{} Failed to write part cache to {}", PART_CACHE_MANAGER_LOG_TAG, path.string()), DebugStack::Color::ERROR);
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
		compactWriter.Key(PART_CACHE_PART_TYPE_ID);
		if (part.type == MeshPartType::PART_GROUP) {
			compactWriter.String("PART_GROUP");
		}
		else if (part.type == MeshPartType::MATERIAL) {
			compactWriter.String("MATERIAL");
		}
		else {
			compactWriter.String("UNKNOWN");
		}
		compactWriter.Key(PART_CACHE_PART_INDEX_ID);
		compactWriter.Uint64(part.index);
		compactWriter.EndObject();

		return buf;
	}

	std::string PartCacheManager::getPartCacheFilename(const ArmourSetWithCharacterSex& armour) const {
		std::string characterSex = (armour.characterFemale ? "F" : "M");
		std::string sex = (armour.set.female ? "F" : "M");

		// Replace any illegal filename characters in the armour set name with underscores
		std::string armourNameSanitized = armour.set.name;
		std::replace(armourNameSanitized.begin(), armourNameSanitized.end(), '/', '_');

		return std::format("{}-{}~{}.json", characterSex, sex, armourNameSanitized);
	}

	bool PartCacheManager::getPartCacheArmourSet(const std::string& filename, ArmourSetWithCharacterSex* out) const {
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