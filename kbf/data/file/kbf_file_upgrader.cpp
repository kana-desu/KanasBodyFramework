#include <kbf/data/file/kbf_file_upgrader.hpp>

#include <kbf/data/file/field_parsers.hpp>
#include <kbf/data/ids/format_ids.hpp>

#include <kbf/data/mesh/parts/mesh_part.hpp>
#include <kbf/data/mesh/parts/hashed_part_list.hpp>

#include <regex>

#define FILE_UPGRADER_LOG_TAG "[KbfFileUpgrader]"

namespace kbf {

	SemanticVersion KbfFileUpgrader::getFileVersion(const rapidjson::Document& doc) const {
		std::string ver;
		bool parsed = parseString(doc, FORMAT_VERSION_ID, FORMAT_VERSION_ID, &ver);
		if (!parsed) return SemanticVersion{};

		return SemanticVersion::fromString(ver);
	}

	KbfFileUpgrader::UpgradeResult KbfFileUpgrader::upgradeFile(rapidjson::Document& doc, KbfFileType fileType) {
		// For now, these files don't support versioning :[ 
		if      (fileType == KbfFileType::SETTINGS)    return UpgradeResult::NO_UPGRADE_NEEDED;
		else if (fileType == KbfFileType::ARMOUR_LIST) return UpgradeResult::NO_UPGRADE_NEEDED;

		SemanticVersion ver = getFileVersion(doc);
		if (ver.isZero()) {
			DEBUG_STACK.push(
				std::format("{} Failed to get version from file of type {}. It will be taken as \"0.0\"", FILE_UPGRADER_LOG_TAG, kbfFileTypeToString(fileType)),
				DebugStack::Color::COL_WARNING
			);
		}

		UpgradeResult res = UpgradeResult::NO_UPGRADE_NEEDED;

		switch (fileType) {
			case KbfFileType::SETTINGS:              res = UPGRADE_NO_OP(ver, doc); break;
			case KbfFileType::ALMA_CONFIG:           res = UPGRADE_NO_OP(ver, doc); break;
			case KbfFileType::ERIK_CONFIG:           res = UPGRADE_NO_OP(ver, doc); break;
			case KbfFileType::GEMMA_CONFIG:          res = UPGRADE_NO_OP(ver, doc); break;
			case KbfFileType::SUPPORT_HUNTER_CONFIG: res = UPGRADE_NO_OP(ver, doc); break;
			case KbfFileType::NPC_CONFIG:            res = UPGRADE_NO_OP(ver, doc); break;
			case KbfFileType::PLAYER_CONFIG:         res = UPGRADE_NO_OP(ver, doc); break;
			case KbfFileType::DOT_KBF:               res = UPGRADE_NO_OP(ver, doc); break;
			case KbfFileType::FBS_PRESET:            res = UPGRADE_NO_OP(ver, doc); break;
			case KbfFileType::PRESET:                res = upgradeFileUsingLUT(ver, doc, presetUpgradeLUT); break;
			case KbfFileType::PRESET_GROUP:          res = UPGRADE_NO_OP(ver, doc); break;
			case KbfFileType::PLAYER_OVERRIDE:       res = UPGRADE_NO_OP(ver, doc); break;
			case KbfFileType::ARMOUR_LIST:           res = UPGRADE_NO_OP(ver, doc); break;
			case KbfFileType::BONE_CACHE:            res = upgradeFileUsingLUT(ver, doc, boneCacheUpgradeLUT); break;
			case KbfFileType::PART_CACHE:            res = upgradeFileUsingLUT(ver, doc, partCacheUpgradeLUT); break;
			case KbfFileType::MATERIAL_CACHE:        res = UPGRADE_NO_OP(ver, doc); break;
		}

		// Update version in file to latest
		if (res == UpgradeResult::SUCCESS) {
			SemanticVersion currentVer = SemanticVersion::currentVersion();
			std::string currentVerStr = currentVer.toString();

			auto& alloc = doc.GetAllocator();
			rapidjson::Value versionVal;
			versionVal.SetString(currentVerStr.c_str(), static_cast<rapidjson::SizeType>(currentVerStr.size()), alloc);
			if (doc.HasMember(FORMAT_VERSION_ID)) {
				doc[FORMAT_VERSION_ID] = versionVal;
			}
			else {
				doc.AddMember(rapidjson::Value(FORMAT_VERSION_ID, alloc), versionVal, alloc);
			}
		}

		return res;
	}

	KbfFileUpgrader::UpgradeResult KbfFileUpgrader::upgradeFileUsingLUT(SemanticVersion ver, rapidjson::Document& doc, const UpgradeLUT& lut) {
		bool tried = false;
		bool upgraded = true;

		for (const auto& [targetVer, upgradeFunc] : lut) {
			if (ver < targetVer) {
				tried = true;
				upgraded &= upgradeFunc(doc);
			}
		}

		if (!tried) return UpgradeResult::NO_UPGRADE_NEEDED;
		return upgraded ? UpgradeResult::SUCCESS : UpgradeResult::FAILED;
	}

	bool KbfFileUpgrader::upgradePreset_1_0_4(rapidjson::Document& doc) {  
		// 1. Add field "hide" to objects in array "removedParts" with default value true. These are nested within objects "set, helm, body, arms, coil, legs".
		// 2. Rename keys "removedParts" to "partOverrides".

		static constexpr const char* kSlots[] = {
			"set",
			"helm",
			"body",
			"arms",
			"coil",
			"legs"
		};

		auto& alloc = doc.GetAllocator();

		for (const char* slotKey : kSlots) {
			// missing slot == not an error (older presets)
			if (!doc.HasMember(slotKey))
				continue;

			rapidjson::Value& slotObj = doc[slotKey];
			if (!slotObj.IsObject()) {
				DEBUG_STACK.push(
					std::format("{} Preset upgrade to 1.0.4 failed - \"{}\" is not an object.", FILE_UPGRADER_LOG_TAG, slotKey),
					DebugStack::Color::COL_ERROR
				);
				return false;
			}

			// missing removedParts is still not fatal (the preset simply has no overrides in that slot)
			if (!slotObj.HasMember("removedParts"))
				continue;

			rapidjson::Value& removedParts = slotObj["removedParts"];

			// ---- Note: removedParts is expected to be an OBJECT whose members are objects ----
			if (!removedParts.IsObject()) {
				DEBUG_STACK.push(
					std::format("{} Preset upgrade to 1.0.4 failed - \"{}.removedParts\" is not an object.", FILE_UPGRADER_LOG_TAG, slotKey),
					DebugStack::Color::COL_ERROR
				);
				return false;
			}

			// === 1) add hide: true to each child object inside removedParts ===
			for (auto m = removedParts.MemberBegin(); m != removedParts.MemberEnd(); ++m) {
				rapidjson::Value& childVal = m->value;
				if (!childVal.IsObject()) {
					DEBUG_STACK.push(
						std::format("{} Preset upgrade to 1.0.4 failed - entry \"{}.removedParts.{}\" is not an object.",
							FILE_UPGRADER_LOG_TAG,
							slotKey, m->name.GetString()),
						DebugStack::Color::COL_ERROR
					);
					return false;
				}

				if (!childVal.HasMember("hide")) {
					childVal.AddMember("hide", rapidjson::Value(true), alloc);
				}
			}

			// === 2) rename removedParts -> partOverrides (keep object shape) ===
			rapidjson::Value partOverrides(rapidjson::kObjectType);

			// copy members into new object (deep-copy with allocator)
			for (auto m = removedParts.MemberBegin(); m != removedParts.MemberEnd(); ++m) {
				// deep-copy name and value into allocator context
				rapidjson::Value nameCopy(m->name, alloc);
				rapidjson::Value valueCopy(m->value, alloc);
				partOverrides.AddMember(nameCopy, valueCopy, alloc);
			}

			slotObj.RemoveMember("removedParts");
			slotObj.AddMember("partOverrides", partOverrides, alloc);
		}

		DEBUG_STACK.push(
			std::format("{} Preset upgrade to 1.0.4 complete.", FILE_UPGRADER_LOG_TAG),
			DebugStack::Color::COL_INFO
		);

		return true;
	}

	bool KbfFileUpgrader::upgradePreset_1_0_6(rapidjson::Document& doc) {
		// 1. Add empty quick material overrides object
		// 2. Add part specific material overrides object
		// 3. Move any hidden/shown parts which previously were materials to part specific material overrides
		// 4. Remove "type" field from part override entries

		auto& alloc = doc.GetAllocator();

		// 1. Add empty quick material overrides object
		if (!doc.HasMember("quickMaterialOverrides")) {
			doc.AddMember(rapidjson::Value("quickMaterialOverrides", alloc), rapidjson::Value(rapidjson::kObjectType), alloc);
		}
		else if (!doc["quickMaterialOverrides"].IsObject()) {
			doc["quickMaterialOverrides"].SetObject();
		}

		static constexpr const char* kSlots[] = { "set", "helm", "body", "arms", "coil", "legs" };
		static std::regex re(R"DELIM(Material\s*"([^"]*)")DELIM");

		for (const char* slotKey : kSlots) {
			if (!doc.HasMember(slotKey))
				continue;

			rapidjson::Value& slotObj = doc[slotKey];
			if (!slotObj.IsObject()) {
				DEBUG_STACK.push(
					std::format("{} Preset upgrade to 1.0.6 failed - \"{}\" is not an object.", FILE_UPGRADER_LOG_TAG, slotKey),
					DebugStack::Color::COL_ERROR
				);
				return false;
			}

			// Ensure per-piece materialOverrides object exists
			if (!slotObj.HasMember("materialOverrides")) {
				slotObj.AddMember(rapidjson::Value("materialOverrides", alloc), rapidjson::Value(rapidjson::kObjectType), alloc);
			}
			else if (!slotObj["materialOverrides"].IsObject()) {
				slotObj["materialOverrides"].SetObject();
			}
			rapidjson::Value& materialOverridesObj = slotObj["materialOverrides"];

			// 3. Move legacy "Material "<name>"" entries into materialOverrides
			for (auto m = slotObj.MemberBegin(); m != slotObj.MemberEnd();) {
				std::string key = m->name.GetString();
				std::smatch match;
				if (std::regex_search(key, match, re)) {
					// Build new object with empty paramOverrides
					rapidjson::Value newObj(rapidjson::kObjectType);
					newObj.AddMember("paramOverrides", rapidjson::Value(rapidjson::kObjectType), alloc);

					// Preserve "hide" from legacy entry and convert to "show"
					if (m->value.IsObject() && m->value.HasMember("hide") && m->value["hide"].IsBool()) {
						bool hide = m->value["hide"].GetBool();
						newObj.AddMember("show", !hide, alloc); // invert hide -> show
					}

					// Add or overwrite in materialOverrides
					rapidjson::Value newKey;
					newKey.SetString(key.c_str(), static_cast<rapidjson::SizeType>(key.size()), alloc);
					materialOverridesObj.RemoveMember(newKey);
					materialOverridesObj.AddMember(newKey, newObj, alloc);

					// Erase original from slotObj
					m = slotObj.RemoveMember(m);
				}
				else {
					++m;
				}
			}

			// 4. Remove "type" fields from remaining partOverrides (slotObj members)
			for (auto it = slotObj.MemberBegin(); it != slotObj.MemberEnd(); ++it) {
				rapidjson::Value& partObj = it->value;
				if (partObj.IsObject() && partObj.HasMember("type")) {
					partObj.RemoveMember("type");
				}
			}
		}

		DEBUG_STACK.push(
			std::format("{} Preset upgrade to 1.0.6 complete.", FILE_UPGRADER_LOG_TAG),
			DebugStack::Color::COL_INFO
		);

		return true;
	}


	bool KbfFileUpgrader::upgradeBoneCache_1_0_6(rapidjson::Document& doc) {
		// 1. Add VERSION number

		// Add the version number as a field under FORMAT_VERSION_ID
		auto& alloc = doc.GetAllocator();

		std::string currentVerStr = "1.0.6";
		rapidjson::Value versionVal;
		versionVal.SetString(currentVerStr.c_str(), static_cast<rapidjson::SizeType>(currentVerStr.size()), alloc);
		if (doc.HasMember(FORMAT_VERSION_ID)) {
			doc[FORMAT_VERSION_ID] = versionVal;
		}
		else {
			doc.AddMember(rapidjson::Value(FORMAT_VERSION_ID, alloc), versionVal, alloc);
		}

		DEBUG_STACK.push(
			std::format("{} Bone cache upgrade to 1.0.6 complete.", FILE_UPGRADER_LOG_TAG),
			DebugStack::Color::COL_INFO
		);

		return true;
	}

	bool KbfFileUpgrader::upgradePartCache_1_0_6(rapidjson::Document& doc) {
		// 1. Add VERSION number
		// 2. Remove Material "<...>" entries
		// 3. Remove "type" filed from entries
		// 4. Update Hashes

		auto& alloc = doc.GetAllocator();

		// 1. Add the version number
		std::string currentVerStr = "1.0.6";
		rapidjson::Value versionVal;
		versionVal.SetString(currentVerStr.c_str(), static_cast<rapidjson::SizeType>(currentVerStr.size()), alloc);
		if (doc.HasMember(FORMAT_VERSION_ID)) {
			doc[FORMAT_VERSION_ID] = versionVal;
		}
		else {
			doc.AddMember(rapidjson::Value(FORMAT_VERSION_ID, alloc), versionVal, alloc);
		}

		static constexpr const char* kSlots[] = {
			"set", "helm", "body", "arms", "coil", "legs"
		};

		static std::regex re(R"DELIM(Material\s*"([^"]*)")DELIM");

		// This will store the remaining entries for hashing

		for (const char* slotKey : kSlots) {
			if (!doc.HasMember(slotKey))
				continue;

			std::vector<MeshPart> remainingEntries;

			rapidjson::Value& slotObj = doc[slotKey];
			if (!slotObj.IsObject()) {
				DEBUG_STACK.push(
					std::format("{} Preset upgrade to 1.0.6 failed - \"{}\" is not an object.", FILE_UPGRADER_LOG_TAG, slotKey),
					DebugStack::Color::COL_ERROR
				);
				return false;
			}

			for (auto m = slotObj.MemberBegin(); m != slotObj.MemberEnd(); ) {
				std::string partName = m->name.GetString();
				bool erased = false;

				// 2. Remove Material "<...>" entries
				std::smatch match;
				if (std::regex_search(partName, match, re)) {
					m = slotObj.RemoveMember(m);
					erased = true;
				}

				if (!erased) {
					rapidjson::Value& childObj = m->value;

					// 3. Remove "type" field if it exists
					if (childObj.IsObject() && childObj.HasMember("type")) {
						childObj.RemoveMember("type");
					}

					// Keep track of remaining entries for hashing
					if (childObj.IsObject() && childObj.HasMember("index") && childObj["index"].IsInt()) {
						MeshPart entry;
						entry.name = partName;
						entry.index = childObj["index"].GetInt();
						remainingEntries.push_back(entry);
					}

					++m;
				}

			}

			size_t hash = HashedPartList::hashParts(remainingEntries);

			std::string hashKey = std::string(slotKey) + "Hash";
			rapidjson::Value hashVal;
			hashVal.SetUint64(static_cast<uint64_t>(hash));

			if (doc.HasMember(hashKey.c_str())) {
				doc[hashKey.c_str()] = hashVal;
			}
			else {
				doc.AddMember(rapidjson::Value(hashKey.c_str(), alloc), hashVal, alloc);
			}
		}

		DEBUG_STACK.push(
			std::format("{} Part cache upgrade to 1.0.6 complete.", FILE_UPGRADER_LOG_TAG),
			DebugStack::Color::COL_INFO
		);

		return true;
	}

}