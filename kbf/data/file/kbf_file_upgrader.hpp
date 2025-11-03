#pragma once

#include <kbf/data/file/kbf_file_type.hpp>
#include <kbf/util/versioning/semantic_version.hpp>

#include <rapidjson/document.h>

namespace kbf {


	class KbfFileUpgrader {
	public:
		KbfFileUpgrader() = default;
		~KbfFileUpgrader() = default;

		enum class UpgradeResult {
			SUCCESS,
			NO_UPGRADE_NEEDED,
			FAILED,
		};

		UpgradeResult upgradeFile(rapidjson::Document& doc, KbfFileType fileType);

	private:
		SemanticVersion getFileVersion(const rapidjson::Document& doc) const;

		// Files that need upgrades
		UpgradeResult upgradePreset(SemanticVersion ver, rapidjson::Document& doc);
		bool upgradePreset_1_0_4(rapidjson::Document& doc);

		// Currently no upgrades needed, so these are just stubs.
		UpgradeResult upgradeSettings      (SemanticVersion ver, rapidjson::Document& doc) { return UpgradeResult::NO_UPGRADE_NEEDED; }
		UpgradeResult upgradeAlmaConfig    (SemanticVersion ver, rapidjson::Document& doc) { return UpgradeResult::NO_UPGRADE_NEEDED; }
		UpgradeResult upgradeErikConfig    (SemanticVersion ver, rapidjson::Document& doc) { return UpgradeResult::NO_UPGRADE_NEEDED; }
		UpgradeResult upgradeGemmaConfig   (SemanticVersion ver, rapidjson::Document& doc) { return UpgradeResult::NO_UPGRADE_NEEDED; }
		UpgradeResult upgradeNpcConfig     (SemanticVersion ver, rapidjson::Document& doc) { return UpgradeResult::NO_UPGRADE_NEEDED; }
		UpgradeResult upgradePlayerConfig  (SemanticVersion ver, rapidjson::Document& doc) { return UpgradeResult::NO_UPGRADE_NEEDED; }
		UpgradeResult upgradeDotKBF        (SemanticVersion ver, rapidjson::Document& doc) { return UpgradeResult::NO_UPGRADE_NEEDED; }
		UpgradeResult upgradeFBSPreset     (SemanticVersion ver, rapidjson::Document& doc) { return UpgradeResult::NO_UPGRADE_NEEDED; }
		UpgradeResult upgradePresetGroup   (SemanticVersion ver, rapidjson::Document& doc) { return UpgradeResult::NO_UPGRADE_NEEDED; }
		UpgradeResult upgradePlayerOverride(SemanticVersion ver, rapidjson::Document& doc) { return UpgradeResult::NO_UPGRADE_NEEDED; }
		UpgradeResult upgradeArmourList    (SemanticVersion ver, rapidjson::Document& doc) { return UpgradeResult::NO_UPGRADE_NEEDED; }
		UpgradeResult upgradeBoneCache     (SemanticVersion ver, rapidjson::Document& doc) { return UpgradeResult::NO_UPGRADE_NEEDED; }
		UpgradeResult upgradePartCache     (SemanticVersion ver, rapidjson::Document& doc) { return UpgradeResult::NO_UPGRADE_NEEDED; }
	};

}