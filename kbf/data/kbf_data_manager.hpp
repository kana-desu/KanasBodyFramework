#pragma once

#include <kbf/data/bones/bone_cache_manager.hpp>
#include <kbf/data/mesh/part_cache_manager.hpp>
#include <kbf/data/preset/preset_group.hpp>
#include <kbf/data/preset/preset.hpp>
#include <kbf/data/fbs_compat/fbs_preset.hpp>
#include <kbf/data/preset/player_override.hpp>
#include <kbf/data/preset/preset_group_defaults.hpp>
#include <kbf/data/preset/preset_defaults.hpp>
#include <kbf/data/formats/kbf_file_data.hpp>
#include <kbf/data/formats/kbf_settings.hpp>
#include <kbf/data/armour/armour_list.hpp>
#include <kbf/npc/npc_id.hpp>

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

#include <kbf/cimgui/cimgui_funcs.hpp>

#include <string>
#include <filesystem>

namespace kbf {

	class KBFDataManager {
	public:
		KBFDataManager(const std::string& path, const std::string& fbsPath) 
			: dataBasePath{ std::filesystem::absolute(path) }, fbsPath{ std::filesystem::absolute(fbsPath) } {}

		void loadData();
		void clearData();
		void reloadData();

		// TODO: If can ever be bothered, most of this can be abstracted to 3 
		//        JSON handler classes that derive from some base.

		BoneCacheManager& boneCache() { return boneCacheManager; }
		PartCacheManager& partCache() { return partCacheManager; }

		PlayerDefaults& playerDefaults() { return presetGroupDefaults.player; }
		NpcDefaults& npcDefaults() { return presetGroupDefaults.npc; }
		AlmaDefaults& almaConfig() { return presetDefaults.alma; }
		GemmaDefaults& gemmaConfig() { return presetDefaults.gemma; }
		ErikDefaults& erikConfig() { return presetDefaults.erik; }

		void setPlayerConfig_Male  (std::string presetUUID) { playerDefaults().male   = presetUUID; writePlayerConfig(playerDefaults()); }
		void setPlayerConfig_Female(std::string presetUUID) { playerDefaults().female = presetUUID; writePlayerConfig(playerDefaults()); }
		void setNpcConfig_Male     (std::string presetUUID) { npcDefaults().male      = presetUUID; writeNpcConfig(npcDefaults()); }
		void setNpcConfig_Female   (std::string presetUUID) { npcDefaults().female    = presetUUID; writeNpcConfig(npcDefaults()); }

		void setAlmaConfig_HandlersOutfit     (std::string presetUUID) { almaConfig().handlersOutfit      = presetUUID; writeAlmaConfig(almaConfig()); }
		void setAlmaConfig_NewWorldCommission (std::string presetUUID) { almaConfig().newWorldCommission  = presetUUID; writeAlmaConfig(almaConfig()); }
		void setAlmaConfig_ScrivenersCoat     (std::string presetUUID) { almaConfig().scrivenersCoat      = presetUUID; writeAlmaConfig(almaConfig()); }
		void setAlmaConfig_SpringBlossomKimono(std::string presetUUID) { almaConfig().springBlossomKimono = presetUUID; writeAlmaConfig(almaConfig()); }
		void setAlmaConfig_ChunLiOutfit       (std::string presetUUID) { almaConfig().chunLiOutfit        = presetUUID; writeAlmaConfig(almaConfig()); }
		void setAlmaConfig_CammyOutfit        (std::string presetUUID) { almaConfig().cammyOutfit         = presetUUID; writeAlmaConfig(almaConfig()); }
		void setAlmaConfig_SummerPoncho       (std::string presetUUID) { almaConfig().summerPoncho        = presetUUID; writeAlmaConfig(almaConfig()); }
		void setAlmaConfig_AutumnWitch        (std::string presetUUID) { almaConfig().autumnWitch         = presetUUID; writeAlmaConfig(almaConfig()); }

		void setGemmaConfig_SmithysOutfit     (std::string presetUUID) { gemmaConfig().smithysOutfit   = presetUUID; writeGemmaConfig(gemmaConfig()); }
		void setGemmaConfig_SummerCoveralls   (std::string presetUUID) { gemmaConfig().summerCoveralls = presetUUID; writeGemmaConfig(gemmaConfig()); }

		void setErikConfig_HandlersOutfit     (std::string presetUUID) { erikConfig().handlersOutfit = presetUUID; writeErikConfig(erikConfig()); }
		void setErikConfig_SummerHat          (std::string presetUUID) { erikConfig().summerHat      = presetUUID; writeErikConfig(erikConfig()); }
		void setErikConfig_AutumnTherian      (std::string presetUUID) { erikConfig().autumnTherian  = presetUUID; writeErikConfig(erikConfig()); }

		void previewPreset(const Preset* preset) { previewedPreset = preset; }
		const Preset* getPreviewedPreset() const { return previewedPreset; }

		bool presetExists(const std::string& name) const;
		bool presetGroupExists(const std::string& name) const;
		bool playerOverrideExists(const PlayerData& player) const;
		size_t getPresetBundleCount(const std::string& bundleName) const;

		Preset* getPresetByUUID(const std::string& uuid);
		PresetGroup* getPresetGroupByUUID(const std::string& uuid);
		PlayerOverride* getPlayerOverride(const PlayerData& player);
		const Preset* getPresetByUUID(const std::string& uuid) const { return const_cast<KBFDataManager*>(this)->getPresetByUUID(uuid); }
		const PresetGroup* getPresetGroupByUUID(const std::string& uuid) const { return const_cast<KBFDataManager*>(this)->getPresetGroupByUUID(uuid); }
		const PlayerOverride* getPlayerOverride(const PlayerData& player) const { return const_cast<KBFDataManager*>(this)->getPlayerOverride(player); }

		const PresetGroup* getActivePresetGroup(const PlayerData& player) const;
		const PresetGroup* getActivePresetGroup(NpcID npcID, bool female) const;
		const PresetGroup* getActiveDefaultPlayerPresetGroup(bool female) const;
		const Preset* getActivePreset(const PlayerData& player, const ArmourSet& armourSet, ArmourPiece piece) const;
		const Preset* getActivePreset(NpcID npcId, bool female, const ArmourSet& armourSet, ArmourPiece piece) const;

		std::vector<const Preset*> getPresets(const std::string& filter = "", ArmourPieceFlags pieceFilters = APF_NONE, bool sort = false) const;
		std::vector<std::string> getPresetBundles(const std::string& filter = "", bool sort = false) const;
		std::vector<std::pair<std::string, size_t>> getPresetBundlesWithCounts(const std::string& filter = "", bool sort = false) const;
		std::vector<std::string> getPresetsInBundle(const std::string& bundleName) const;
		std::vector<const PresetGroup*> getPresetGroups(const std::string& filter = "", bool sort = false) const;
		std::vector<const PlayerOverride*> getPlayerOverrides(const std::string& filter = "", bool sort = false) const;

		std::vector<std::string> getPresetIds(const std::string& filter = "", ArmourPieceFlags pieceFilters = APF_NONE, bool sort = false) const;
		std::vector<std::string> getPresetGroupIds(const std::string& filter = "", bool sort = false) const;
		std::vector<std::string> getPlayerOverrideIds(const std::string& filter = "", bool sort = false) const;

		bool addPreset(const Preset& preset, bool write = true);
		bool addPresetGroup(const PresetGroup& presetGroup, bool write = true);
		bool addPlayerOverride(const PlayerOverride& playerOverride, bool write = true);

		void deletePreset(const std::string& uuid, bool validate = true);
		void deletePresetBundle(const std::string& bundleName);
		void deletePresetGroup(const std::string& uuid);
		void deletePlayerOverride(const PlayerData& player);

		void updatePreset(const std::string& uuid, Preset newPreset);
		void updatePresetGroup(const std::string& uuid, PresetGroup newPresetGroup);
		void updatePlayerOverride(const PlayerData& player, PlayerOverride newOverride);

		void setRegularFontOverride(ImFont* font) { regularFontOverride = font; }
		ImFont* getRegularFontOverride() const { return regularFontOverride; }

		bool fbsDirectoryFound() const { return std::filesystem::exists(fbsPath); }
		bool getFBSpresets(std::vector<FBSPreset>* out, bool female = true, std::string bundle = "FBS Presets", float* progressOut = nullptr) const;

		void resolvePresetNameConflicts(std::vector<Preset>& presets) const;
		void resolvePresetGroupNameConflicts(std::vector<PresetGroup>& presets) const;

		bool importKBF(std::string filepath, size_t* conflcitsCount = nullptr);
		bool readKBF(std::string filepath, KBFFileData* out) const;
		bool writeKBF(std::string filepath, KBFFileData data) const;
		bool writeModArchive(std::string filepath, KBFFileData data) const;
		void deleteLocalModArchive(std::string name);

		struct ModArchiveCounts {
			size_t presetGroups = 0;
			size_t presets = 0;
			size_t playerOverrides = 0;
		};
		std::unordered_map<std::string, ModArchiveCounts> getModArchiveInfo() const;

		KBFSettings& settings() { return m_settings; }
		bool loadSettings(KBFSettings* out);
		bool loadSettings() { return loadSettings(&m_settings); }
		bool writeSettings(const KBFSettings& settings) const;
		bool writeSettings() const { return writeSettings(m_settings); }

		const std::filesystem::path dataBasePath;
		const std::filesystem::path fbsPath;
		const std::filesystem::path defaultConfigsPath = dataBasePath / "DefaultConfigs";
		const std::filesystem::path presetPath         = dataBasePath / "Presets";
		const std::filesystem::path presetGroupPath    = dataBasePath / "PresetGroups";
		const std::filesystem::path playerOverridePath = dataBasePath / "PlayerOverrides";
		const std::filesystem::path exportsPath        = dataBasePath / "Exports";

		const std::filesystem::path almaConfigPath     = defaultConfigsPath / "alma.json";
		const std::filesystem::path erikConfigPath     = defaultConfigsPath / "erik.json";
		const std::filesystem::path gemmaConfigPath    = defaultConfigsPath / "gemma.json";
		const std::filesystem::path npcConfigPath      = defaultConfigsPath / "npc.json";
		const std::filesystem::path playerConfigPath   = defaultConfigsPath / "players.json";

		const std::filesystem::path settingsPath   = dataBasePath / "settings.json";
		const std::filesystem::path armourListPath = dataBasePath / "armour_list.json";

	private:
		void verifyDirectoriesExist() const;
		void createDirectoryIfNotExists(const std::filesystem::path& path) const;

		rapidjson::Document loadConfigJson(const std::string& path, std::function<bool()> onRequestCreateDefault) const;

		// UNSAFE - Do not use directly. Call loadConfigJson instead.
		std::string readJsonFile(const std::string& path) const;
		bool writeJsonFile(std::string path, const std::string& json) const;
		bool deleteJsonFile(std::string path) const;

		bool loadAlmaConfig(AlmaDefaults* out);
		bool writeAlmaConfig(const AlmaDefaults& out) const;
		bool loadGemmaConfig(GemmaDefaults* out);
		bool writeGemmaConfig(const GemmaDefaults& out) const;
		bool loadErikConfig(ErikDefaults* out);
		bool writeErikConfig(const ErikDefaults& out) const;

		bool loadNpcConfig(NpcDefaults* out);
		bool writeNpcConfig(const NpcDefaults& out) const;
		bool loadPlayerConfig(PlayerDefaults* out);
		bool writePlayerConfig(const PlayerDefaults& out) const;

		PresetDefaults      presetDefaults;
		PresetGroupDefaults presetGroupDefaults;

		// This should've been abstracted to some common json based data class.
		// .... Too bad!
		std::unordered_map<std::string, Preset> presets; // index by uuid
		bool loadPreset(const std::filesystem::path& path, Preset* out);
		bool loadPresetData(const rapidjson::Value& doc, Preset* out) const;
		bool loadPresetPieceSettings(const rapidjson::Value& object, PresetPieceSettings* out) const;
		bool loadRemovedParts(const rapidjson::Value& object, std::vector<MeshPart>* out) const;
		bool loadBoneModifiers(const rapidjson::Value& object, BoneModifierMap* out) const;
		bool writePreset(const std::filesystem::path& path, const Preset& preset) const;
		void writePresetJsonContent(const Preset& preset, rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const;
		void writePresetPieceSettingsJsonContent(const PresetPieceSettings& settings, rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const;
		rapidjson::StringBuffer getCompactBoneModifierJsonContent(const BoneModifier& modifier) const;
		rapidjson::StringBuffer getCompactRemovedPartJsonContent(const MeshPart& part) const;
		bool loadPresets();

		bool loadFBSPreset(const std::filesystem::path& path, bool body, bool female, std::string bundle, FBSPreset* out) const;

		std::unordered_map<std::string, PresetGroup> presetGroups;
		bool loadPresetGroup(const std::filesystem::path& path, PresetGroup* out);
		bool loadPresetGroupData(const rapidjson::Value& doc, PresetGroup* out) const;
		bool loadAssignedPresets(const rapidjson::Value& object, std::unordered_map<ArmourSet, std::string>* out) const;
		bool writePresetGroup(const std::filesystem::path& path, const PresetGroup& presetGroup) const;
		void writePresetGroupJsonContent(const PresetGroup& presetGroup, rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const;
		void writePresetGroupAssignedPresets(
			std::string id, 
			const std::unordered_map<ArmourSet, std::string>& presets,
			rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const;
		bool loadPresetGroups();

		std::unordered_map<PlayerData, PlayerOverride> playerOverrides;
		bool loadPlayerOverride(const std::filesystem::path& path, PlayerOverride* out);
		bool loadPlayerOverrideData(const rapidjson::Value& doc, PlayerOverride* out) const;
		bool writePlayerOverride(const std::filesystem::path& path, const PlayerOverride& playerOverride) const;
		void writePlayerOverrideJsonContent(const PlayerOverride& playerOverride, rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const;
		bool loadPlayerOverrides();
		std::string getPlayerOverrideFilename(const PlayerData& playerData) const;

		void validateObjectsUsingPresetGroups();
		void validatePlayerOverrides();
		void validateDefaultConfigs_PresetGroups();
		bool validatePresetGroupExists(std::string& uuid) const;

		void validateObjectsUsingPresets();
		void validatePresetGroups();
		void validateDefaultConfigs_Presets();
		bool validatePresetExists(std::string& uuid) const;

		bool loadArmourList(const std::filesystem::path& path, ArmourMapping* out);
		bool loadArmourListData(const rapidjson::Value& doc, ArmourMapping* out) const;
		bool writeArmourList(const std::filesystem::path& path, const ArmourMapping& mapping) const;
		void writeArmourListJsonContent(const ArmourMapping& mapping, rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const;

		KBFSettings m_settings;
		BoneCacheManager boneCacheManager{ dataBasePath };
		PartCacheManager partCacheManager{ dataBasePath };
		const Preset* previewedPreset = nullptr;

		ImFont* regularFontOverride = nullptr;
	};

}