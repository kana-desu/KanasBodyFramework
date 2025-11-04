#include <kbf/data/file/kbf_file_upgrader.hpp>

#include <kbf/data/file/field_parsers.hpp>
#include <kbf/data/ids/format_ids.hpp>

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
				std::format("[KbfFileUpgrader] Failed to get version from file of type {} - cannot upgrade.", kbfFileTypeToString(fileType)),
				DebugStack::Color::ERROR
			);
		}

		UpgradeResult res = UpgradeResult::NO_UPGRADE_NEEDED;

		switch (fileType) {
			case KbfFileType::SETTINGS:        res = upgradeSettings      (ver, doc); break;
			case KbfFileType::ALMA_CONFIG:     res = upgradeAlmaConfig    (ver, doc); break;
			case KbfFileType::ERIK_CONFIG:     res = upgradeErikConfig    (ver, doc); break;
			case KbfFileType::GEMMA_CONFIG:    res = upgradeGemmaConfig   (ver, doc); break;
			case KbfFileType::NPC_CONFIG:      res = upgradeNpcConfig     (ver, doc); break;
			case KbfFileType::PLAYER_CONFIG:   res = upgradePlayerConfig  (ver, doc); break;
			case KbfFileType::DOT_KBF:         res = upgradeDotKBF        (ver, doc); break;
			case KbfFileType::FBS_PRESET:      res = upgradeFBSPreset     (ver, doc); break;
			case KbfFileType::PRESET:          res = upgradePreset        (ver, doc); break;
			case KbfFileType::PRESET_GROUP:    res = upgradePresetGroup   (ver, doc); break;
			case KbfFileType::PLAYER_OVERRIDE: res = upgradePlayerOverride(ver, doc); break;
			case KbfFileType::ARMOUR_LIST:     res = upgradeArmourList    (ver, doc); break;
			case KbfFileType::BONE_CACHE:      res = upgradeBoneCache     (ver, doc); break;
			case KbfFileType::PART_CACHE:      res = upgradePartCache     (ver, doc); break;
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

	KbfFileUpgrader::UpgradeResult KbfFileUpgrader::upgradePreset(SemanticVersion ver, rapidjson::Document& doc) {
		bool tried = false;
		bool upgraded = true;

		if (ver < SemanticVersion(1, 0, 4)) { tried = true; upgraded &= upgradePreset_1_0_4(doc); }

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
					std::format("[KbfFileUpgrader] Preset upgrade to 1.0.4 failed - \"{}\" is not an object.", slotKey),
					DebugStack::Color::ERROR
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
					std::format("[KbfFileUpgrader] Preset upgrade to 1.0.4 failed - \"{}.removedParts\" is not an object.", slotKey),
					DebugStack::Color::ERROR
				);
				return false;
			}

			// === 1) add hide: true to each child object inside removedParts ===
			for (auto m = removedParts.MemberBegin(); m != removedParts.MemberEnd(); ++m) {
				rapidjson::Value& childVal = m->value;
				if (!childVal.IsObject()) {
					DEBUG_STACK.push(
						std::format("[KbfFileUpgrader] Preset upgrade to 1.0.4 failed - entry \"{}.removedParts.{}\" is not an object.",
							slotKey, m->name.GetString()),
						DebugStack::Color::ERROR
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
			"[KbfFileUpgrader] Preset upgrade to 1.0.4 complete.",
			DebugStack::Color::INFO
		);

		return true;
	}

}