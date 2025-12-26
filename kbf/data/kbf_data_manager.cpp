#include <kbf/data/kbf_data_manager.hpp>

#include <kbf/data/file/field_parsers.hpp>
#include <kbf/data/ids/config_ids.hpp>
#include <kbf/data/ids/preset_ids.hpp>
#include <kbf/data/fbs_compat/fbs_preset_ids.hpp>
#include <kbf/data/fbs_compat/fbs_armour_set_compat.hpp>
#include <kbf/data/ids/preset_group_ids.hpp>
#include <kbf/data/ids/player_override_ids.hpp>
#include <kbf/data/ids/format_ids.hpp>
#include <kbf/data/ids/kbf_file_ids.hpp>
#include <kbf/data/ids/settings_ids.hpp>
#include <kbf/data/file/kbf_file_upgrader.hpp>
#include <kbf/debug/debug_stack.hpp>
#include <kbf/util/id/uuid_generator.hpp>
#include <kbf/util/functional/invoke_callback.hpp>
#include <kbf/util/string/to_lower.hpp>
#include <kbf/util/string/cvt_utf16_utf8.hpp>
#include <kbf/util/string/ansi_encode.hpp>
#include <kbf/util/io/zip_file.hpp>
#include <kbf/util/io/get_relative_subfolder.hpp>
#include <kbf/data/ids/armour_list_ids.hpp>
#include <kbf/data/bones/bone_symmetry_utils.hpp>

#include <filesystem>
#include <fstream>
#include <format>
#include <unordered_set>
#include <set>
#include <unordered_map>
#include <locale>
#include <codecvt>

#undef GetObject

#define KBF_DATA_MANAGER_LOG_TAG "[KBFDataManager]"

namespace kbf {
    void KBFDataManager::loadData() {
        DEBUG_STACK.push(std::format("{} Loading Data...", KBF_DATA_MANAGER_LOG_TAG), DebugStack::Color::COL_DEBUG);

        verifyDirectoriesExist();

        loadSettings();

        m_boneCacheManager.loadCaches();
        m_partCacheManager.loadCaches();
		m_matCacheManager.loadCaches();

        loadAlmaConfig(&presetDefaults.alma);
        loadErikConfig(&presetDefaults.erik);
        loadGemmaConfig(&presetDefaults.gemma);
        loadNpcConfig(&presetGroupDefaults.npc);
        loadPlayerConfig(&presetGroupDefaults.player);

        loadPresets();
        loadPresetGroups();
        loadPlayerOverrides();

        validateObjectsUsingPresets();
        validateObjectsUsingPresetGroups();

        // Armour list stuff
        #if KBF_DEBUG_BUILD
		DEBUG_STACK.push(std::format("{} Note: Debug Build - using & writing fallback armour list...", KBF_DATA_MANAGER_LOG_TAG), DebugStack::Color::COL_WARNING);
            writeArmourList(armourListPath, ArmourList::FALLBACK_MAPPING);
        #endif

        loadArmourList(armourListPath, &ArmourList::ACTIVE_MAPPING);
    }

    void KBFDataManager::clearData() {
        DEBUG_STACK.push(std::format("{} Clearing Data...", KBF_DATA_MANAGER_LOG_TAG), DebugStack::Color::COL_DEBUG);

        presetDefaults = {};
        presetGroupDefaults = {};

        presets.clear();
        presetGroups.clear();
        playerOverrides.clear();
    }

    void KBFDataManager::reloadData() {
        clearData();
        loadData();
    }

    bool KBFDataManager::presetExists(const std::string& name) const {
        for (const auto& [uuid, preset] : presets) {
            if (preset.name == name) {
                return true;
            }
        }
        return false;
    }

    bool KBFDataManager::presetGroupExists(const std::string& name) const {
        for (const auto& [uuid, presetGroup] : presetGroups) {
            if (presetGroup.name == name) {
                return true;
            }
        }
        return false;
    }

    bool KBFDataManager::playerOverrideExists(const PlayerData& player) const {
        for (const auto& [playerData, playerOverride] : playerOverrides) {
            if (player == playerData) {
                return true;
            }
        }
        return false;
    }

    size_t KBFDataManager::getPresetBundleCount(const std::string& bundleName) const {
        size_t count = 0;
        for (const auto& [uuid, preset] : presets) {
            if (preset.bundle == bundleName) {
                count++;
            }
        }
        return count;
    }

    Preset* KBFDataManager::getPresetByUUID(const std::string& uuid) {
        if (presets.find(uuid) != presets.end()) return &presets.at(uuid);
        return nullptr;
    }

    PresetGroup* KBFDataManager::getPresetGroupByUUID(const std::string& uuid) {
        if (presetGroups.find(uuid) != presetGroups.end()) return &presetGroups.at(uuid);
        return nullptr;
    }

    PlayerOverride* KBFDataManager::getPlayerOverride(const PlayerData& player) {
        if (playerOverrides.find(player) != playerOverrides.end()) return &playerOverrides.at(player);
        return nullptr;
    }

    const PresetGroup* KBFDataManager::getActivePresetGroup(const PlayerData& player) const {
        // Check for player override first
        const PlayerOverride* override = getPlayerOverride(player);
        if (override) return getPresetGroupByUUID(override->presetGroup);

        // Otherwise use the default preset group for the character's sex
        return getActiveDefaultPlayerPresetGroup(player.female);
    }

    const PresetGroup* KBFDataManager::getActivePresetGroup(NpcID npcID, bool female) const {
        if (female) return getPresetGroupByUUID(presetGroupDefaults.npc.female);
        return getPresetGroupByUUID(presetGroupDefaults.npc.male);
    }

    const PresetGroup* KBFDataManager::getActiveDefaultPlayerPresetGroup(bool female) const {
        if (female) return getPresetGroupByUUID(presetGroupDefaults.player.female);
        return getPresetGroupByUUID(presetGroupDefaults.player.male);
    }

    const Preset* KBFDataManager::getActivePreset(const PlayerData& player, const ArmourSet& armourSet, ArmourPiece piece) const {
        const PresetGroup* activePresetGroup = getActivePresetGroup(player);
        if (activePresetGroup == nullptr) return nullptr;

        const std::unordered_map<ArmourSet, std::string>* targetMap = nullptr;	
        switch (piece) {
        case ArmourPiece::AP_SET:  targetMap = &activePresetGroup->setPresets;  break;
        case ArmourPiece::AP_HELM: targetMap = &activePresetGroup->helmPresets; break;
        case ArmourPiece::AP_BODY: targetMap = &activePresetGroup->bodyPresets; break;
        case ArmourPiece::AP_ARMS: targetMap = &activePresetGroup->armsPresets; break;
        case ArmourPiece::AP_COIL: targetMap = &activePresetGroup->coilPresets; break;
        case ArmourPiece::AP_LEGS: targetMap = &activePresetGroup->legsPresets; break;
        }
        if (targetMap == nullptr) return nullptr;

        // Check if the preset group has an assigned preset for the given armour set
        if (activePresetGroup->armourHasPresetUUID(armourSet, piece)) {
            const std::string& presetUUID = targetMap->at(armourSet);
            return getPresetByUUID(presetUUID);
        }
        else if (activePresetGroup->armourHasPresetUUID(ArmourList::DefaultArmourSet(), piece)) {
            const std::string& presetUUID = targetMap->at(ArmourList::DefaultArmourSet());
            return getPresetByUUID(presetUUID);
        }

        return nullptr;
    }

    const Preset* KBFDataManager::getActivePreset(NpcID npcId, bool female, const ArmourSet& armourSet, ArmourPiece piece) const {
        switch (npcId) {
        case NpcID::NPC_ID_UNKNOWN: return nullptr;
        case NpcID::NPC_ID_ALMA: {
            ArmourID armour = ArmourList::getArmourIdFromSet(armourSet);
            std::string pieceId = armour.getPiece(piece);
            
            std::string presetUUID = "";
            if      (pieceId == "ch04_000_0000") presetUUID = presetDefaults.alma.handlersOutfit;
            else if	(pieceId == "ch04_000_0002") presetUUID = presetDefaults.alma.scrivenersCoat;
            else if	(pieceId == "ch04_000_0070") presetUUID = presetDefaults.alma.springBlossomKimono;
            else if	(pieceId == "ch04_000_0071") presetUUID = presetDefaults.alma.summerPoncho;
            else if	(pieceId == "ch04_000_0074") presetUUID = presetDefaults.alma.newWorldCommission;
            else if	(pieceId == "ch04_000_0075") presetUUID = presetDefaults.alma.chunLiOutfit;
            else if (pieceId == "ch04_000_0076") presetUUID = presetDefaults.alma.cammyOutfit;
            else if	(pieceId == "ch04_000_0072") presetUUID = presetDefaults.alma.autumnWitch;

            return getPresetByUUID(presetUUID);
        }
        case NpcID::NPC_ID_GEMMA: {
            ArmourID armour = ArmourList::getArmourIdFromSet(armourSet);
            std::string pieceId = armour.getPiece(piece);
            
            std::string presetUUID = "";
            if      (pieceId == "ch04_004_0000") presetUUID = presetDefaults.gemma.smithysOutfit;
            else if	(pieceId == "ch04_004_0072") presetUUID = presetDefaults.gemma.summerCoveralls;

            return getPresetByUUID(presetUUID);
        }
        case NpcID::NPC_ID_ERIK: {
            ArmourID armour = ArmourList::getArmourIdFromSet(armourSet);
            std::string pieceId = armour.getPiece(piece);
            
            std::string presetUUID = "";
            if      (pieceId == "ch04_010_0000") presetUUID = presetDefaults.erik.handlersOutfit;
            else if (pieceId == "ch04_010_0071") presetUUID = presetDefaults.erik.summerHat;
            else if	(pieceId == "ch04_010_0072") presetUUID = presetDefaults.erik.autumnTherian;

            return getPresetByUUID(presetUUID);
        }
        case NpcID::NPC_ID_UNNAMED:
        case NpcID::NPC_ID_HUNTER: {
            const PresetGroup* activePresetGroup = getActivePresetGroup(npcId, female);
            if (activePresetGroup == nullptr) return nullptr;

            const std::unordered_map<ArmourSet, std::string>* targetMap = nullptr;
            switch (piece) {
            case ArmourPiece::AP_SET:  targetMap = &activePresetGroup->setPresets;  break;
            case ArmourPiece::AP_HELM: targetMap = &activePresetGroup->helmPresets; break;
            case ArmourPiece::AP_BODY: targetMap = &activePresetGroup->bodyPresets; break;
            case ArmourPiece::AP_ARMS: targetMap = &activePresetGroup->armsPresets; break;
            case ArmourPiece::AP_COIL: targetMap = &activePresetGroup->coilPresets; break;
            case ArmourPiece::AP_LEGS: targetMap = &activePresetGroup->legsPresets; break;
            }
            if (targetMap == nullptr) return nullptr;

            // Check if the preset group has an assigned preset for the given armour set
            if (activePresetGroup->armourHasPresetUUID(armourSet, piece)) {
                const std::string& presetUUID = targetMap->at(armourSet);
                return getPresetByUUID(presetUUID);
            }
            else if (activePresetGroup->armourHasPresetUUID(ArmourList::DefaultArmourSet(), piece)) {
                const std::string& presetUUID = targetMap->at(ArmourList::DefaultArmourSet());
                return getPresetByUUID(presetUUID);
            }

            return nullptr;
        }
        default: return nullptr;
        }
    }


    std::vector<const Preset*> KBFDataManager::getPresets(const std::string& filter, ArmourPieceFlags pieceFilters, bool sort) const {
        std::vector<const Preset*> filteredPresets;

        std::string filterLower = toLower(filter);

        for (const auto& [uuid, preset] : presets) {
            std::string presetNameLower = toLower(preset.name);

            if (filterLower.empty() || presetNameLower.find(filterLower) != std::string::npos) {
                if (pieceFilters & APF_SET  && !preset.hasModifiers(ArmourPiece::AP_SET))  continue;
                if (pieceFilters & APF_HELM && !preset.hasModifiers(ArmourPiece::AP_HELM)) continue;
                if (pieceFilters & APF_BODY && !preset.hasModifiers(ArmourPiece::AP_BODY)) continue;
                if (pieceFilters & APF_ARMS && !preset.hasModifiers(ArmourPiece::AP_ARMS)) continue;
                if (pieceFilters & APF_COIL && !preset.hasModifiers(ArmourPiece::AP_COIL)) continue;
                if (pieceFilters & APF_LEGS && !preset.hasModifiers(ArmourPiece::AP_LEGS)) continue;

                filteredPresets.push_back(&preset);
            }
        }

        if (sort) {
            std::sort(filteredPresets.begin(), filteredPresets.end(),
                [](const Preset* a, const Preset* b) {
                    return a->name < b->name;
                });
        }

        return filteredPresets;
    }

    std::vector<std::string> KBFDataManager::getPresetBundles(const std::string& filter, bool sort) const {
        std::unordered_set<std::string> presetBundles;
        std::string filterLower = toLower(filter);

        for (const auto& [uuid, preset] : presets) {
            if (!preset.bundle.empty()) {
                std::string bundleNameLower = toLower(preset.bundle);
                if (filterLower.empty() || bundleNameLower.find(filterLower) != std::string::npos) {
                    presetBundles.insert(preset.bundle);
                }
            }
        }

        std::vector<std::string> sortedPresetBundles(presetBundles.begin(), presetBundles.end());
        if (sort) {
            std::sort(sortedPresetBundles.begin(), sortedPresetBundles.end());
        }

        return sortedPresetBundles;
    }

    std::vector<std::pair<std::string, size_t>> KBFDataManager::getPresetBundlesWithCounts(const std::string& filter, bool sort) const {
        std::unordered_map<std::string, size_t> presetBundles;
        std::string filterLower = toLower(filter);

        for (const auto& [uuid, preset] : presets) {
            if (!preset.bundle.empty()) {
                std::string bundleNameLower = toLower(preset.bundle);
                if (filterLower.empty() || bundleNameLower.find(filterLower) != std::string::npos) {
                    auto it = presetBundles.find(preset.bundle);
                    if (it != presetBundles.end()) {
                        it->second++;
                    } else {
                        presetBundles.emplace(preset.bundle, 1);
                    }
                }
            }
        }

        std::vector<std::pair<std::string, size_t>> sortedPresetBundles(presetBundles.begin(), presetBundles.end());
        if (sort) {
            std::sort(sortedPresetBundles.begin(), sortedPresetBundles.end(),
                [](const std::pair<std::string, size_t>& a, const std::pair<std::string, size_t>& b) {
                    return a.first < b.first;
                });
        }

        return sortedPresetBundles;
    }

    std::vector<std::string> KBFDataManager::getPresetsInBundle(const std::string& bundleName) const {
        std::vector<std::string> presetsInBundle;
        for (const auto& [uuid, preset] : presets) {
            if (preset.bundle == bundleName) {
                presetsInBundle.push_back(preset.uuid);
            }
        }
        return presetsInBundle;
    }

    std::vector<const PresetGroup*> KBFDataManager::getPresetGroups(const std::string& filter, bool sort) const {
        std::vector<const PresetGroup*> filteredPresetGroups;

        std::string filterLower = toLower(filter);

        for (const auto& [uuid, presetGroup] : presetGroups) {
            std::string presetGroupNameLower = toLower(presetGroup.name);

            if (filterLower.empty() || presetGroupNameLower.find(filterLower) != std::string::npos) {
                filteredPresetGroups.push_back(&presetGroup);
            }
        }

        if (sort) {
            std::sort(filteredPresetGroups.begin(), filteredPresetGroups.end(),
                [](const PresetGroup* a, const PresetGroup* b) {
                    return a->name < b->name;
                });
        }

        return filteredPresetGroups;
    }

    std::vector<const PlayerOverride*> KBFDataManager::getPlayerOverrides(const std::string& filter, bool sort) const {
        std::vector<const PlayerOverride*> filteredPlayerOverrides;

        std::string filterLower = toLower(filter);

        for (const auto& [uuid, playerOverride] : playerOverrides) {
            std::string overridePlayerNameLower = toLower(playerOverride.player.name);

            if (filterLower.empty() || overridePlayerNameLower.find(filterLower) != std::string::npos) {
                filteredPlayerOverrides.push_back(&playerOverride);
            }
        }

        if (sort) {
            std::sort(filteredPlayerOverrides.begin(), filteredPlayerOverrides.end(),
                [](const PlayerOverride* a, const PlayerOverride* b) {
                    return a->player.name < b->player.name;
                });
        }

        return filteredPlayerOverrides;
    }

    std::vector<std::string> KBFDataManager::getPresetIds(const std::string& filter, ArmourPieceFlags pieceFilters, bool sort) const {
        std::vector<std::string> presetIds;
        std::string filterLower = toLower(filter);

        for (const auto& [uuid, preset] : presets) {
            std::string presetNameLower = toLower(preset.name);
            if (filterLower.empty() || presetNameLower.find(filterLower) != std::string::npos) {
                if (pieceFilters & ArmourPiece::AP_SET  && !preset.hasModifiers(ArmourPiece::AP_SET))  continue;
                if (pieceFilters & ArmourPiece::AP_HELM && !preset.hasModifiers(ArmourPiece::AP_HELM)) continue;
                if (pieceFilters & ArmourPiece::AP_BODY && !preset.hasModifiers(ArmourPiece::AP_BODY)) continue;
                if (pieceFilters & ArmourPiece::AP_ARMS && !preset.hasModifiers(ArmourPiece::AP_ARMS)) continue;
                if (pieceFilters & ArmourPiece::AP_COIL && !preset.hasModifiers(ArmourPiece::AP_COIL)) continue;
                if (pieceFilters & ArmourPiece::AP_LEGS && !preset.hasModifiers(ArmourPiece::AP_LEGS)) continue;

                presetIds.push_back(uuid);
            }
        }
        if (sort) {
            std::sort(presetIds.begin(), presetIds.end(),
                [&](const std::string& a, const std::string& b) {
                    return presets.at(a).name < presets.at(b).name;
                });
        }

        return presetIds;
    }

    std::vector<std::string> KBFDataManager::getPresetGroupIds(const std::string& filter, bool sort) const {
        std::vector<std::string> presetGroupIds;
        std::string filterLower = toLower(filter);
        for (const auto& [uuid, presetGroup] : presetGroups) {
            std::string presetGroupNameLower = toLower(presetGroup.name);
            if (filterLower.empty() || presetGroupNameLower.find(filterLower) != std::string::npos) {
                presetGroupIds.push_back(uuid);
            }
        }
        if (sort) {
            std::sort(presetGroupIds.begin(), presetGroupIds.end(),
                [&](const std::string& a, const std::string& b) {
                    return presetGroups.at(a).name < presetGroups.at(b).name;
                });
        }

        return presetGroupIds;
    }

    bool KBFDataManager::addPreset(const Preset& preset, bool write) {
        if (presets.find(preset.uuid) != presets.end()) {
            DEBUG_STACK.push(std::format("{} Tried to add new preset {} with UUID {}, but a preset with this UUID already exists. Skipping...", KBF_DATA_MANAGER_LOG_TAG, preset.name, preset.uuid), DebugStack::Color::COL_WARNING);
            return false;
        }
        presets.emplace(preset.uuid, preset);

        if (write) {
            std::filesystem::path presetPath = this->presetPath / (preset.name + ".json");
            if (writePreset(presetPath, preset)) {
                DEBUG_STACK.push(std::format("{} Added new preset: {} ({})", KBF_DATA_MANAGER_LOG_TAG, preset.name, preset.uuid), DebugStack::Color::COL_SUCCESS);
            }
        }
        else {
            DEBUG_STACK.push(std::format("{} Added new preset (NON-PERSISTENT): {} ({})", KBF_DATA_MANAGER_LOG_TAG, preset.name, preset.uuid), DebugStack::Color::COL_SUCCESS);
        }

        return true;
    }


    bool KBFDataManager::addPresetGroup(const PresetGroup& presetGroup, bool write) {
        if (presetGroups.find(presetGroup.uuid) != presetGroups.end()) {
            DEBUG_STACK.push(std::format("{} Tried to add new preset group {} with UUID {}, but a preset with this UUID already exists. Skipping...", 
                KBF_DATA_MANAGER_LOG_TAG, presetGroup.name, presetGroup.uuid
            ), DebugStack::Color::COL_WARNING);
            return false;
        }
        presetGroups.emplace(presetGroup.uuid, presetGroup);

        if (write) {
            std::filesystem::path presetGroupPath = this->presetGroupPath / (presetGroup.name + ".json");
            if (writePresetGroup(presetGroupPath, presetGroup)) {
                DEBUG_STACK.push(std::format("{} Added new preset group: {} ({})", KBF_DATA_MANAGER_LOG_TAG, presetGroup.name, presetGroup.uuid), DebugStack::Color::COL_SUCCESS);
            }
        }
        else {
            DEBUG_STACK.push(std::format("{} Added new preset group (NON-PERSISTENT): {} ({})", KBF_DATA_MANAGER_LOG_TAG, presetGroup.name, presetGroup.uuid), DebugStack::Color::COL_SUCCESS);
        }

        return true;
    }

    bool KBFDataManager::addPlayerOverride(const PlayerOverride& playerOverride, bool write) {
        const PlayerData& player = playerOverride.player;

        if (playerOverrides.find(playerOverride.player) != playerOverrides.end()) {
            DEBUG_STACK.push(std::format("{} Tried to add new player override for player {}, but an override for this player already exists. Skipping...", KBF_DATA_MANAGER_LOG_TAG, player.string()), DebugStack::Color::COL_WARNING);
            return false;
        }
        playerOverrides.emplace(player, playerOverride);

        if (write) {
            std::filesystem::path playerOverridePath = this->playerOverridePath / (getPlayerOverrideFilename(player) + ".json");
            if (writePlayerOverride(playerOverridePath, playerOverride)) {
                DEBUG_STACK.push(std::format("{} Added new player override: {}", KBF_DATA_MANAGER_LOG_TAG, player.string()), DebugStack::Color::COL_SUCCESS);
            }
        }
        else {
            DEBUG_STACK.push(std::format("{} Added new player override (NON-PERSISTENT): {}", KBF_DATA_MANAGER_LOG_TAG, player.string()), DebugStack::Color::COL_SUCCESS);
        }

        return true;
    }

    void KBFDataManager::deletePreset(const std::string& uuid, bool validate) {
        if (presets.find(uuid) == presets.end()) {
            DEBUG_STACK.push(std::format("{} Tried to delete preset with UUID {}, but no such preset exists. Skipping...", KBF_DATA_MANAGER_LOG_TAG, uuid), DebugStack::Color::COL_WARNING);
            return;
        }
        std::string presetName = presets.at(uuid).name;

        // Delete corresponding preset file.
        std::filesystem::path currPresetPath = this->presetPath / (presetName + ".json");
        if (!std::filesystem::exists(currPresetPath)) {
            DEBUG_STACK.push(std::format("{} Deleted preset {} ({}) locally, but no corresponding .json file exists.", KBF_DATA_MANAGER_LOG_TAG, presetName, uuid), DebugStack::Color::COL_WARNING);
            presets.erase(uuid);
            if (validate) validateObjectsUsingPresets();
            return;
        }

        if (deleteJsonFile(currPresetPath.string())) {
            DEBUG_STACK.push(std::format("{} Deleted preset {} ({})", KBF_DATA_MANAGER_LOG_TAG, presetName, uuid), DebugStack::Color::COL_SUCCESS);
            presets.erase(uuid);
            if (validate) validateObjectsUsingPresets();
        }
    }

    void KBFDataManager::deletePresetBundle(const std::string& bundleName) {
        std::vector<std::string> presetUUIDs = getPresetsInBundle(bundleName);
        if (presetUUIDs.empty()) {
            DEBUG_STACK.push(std::format("{} Tried to delete preset bundle {}, but no presets found in this bundle. Skipping...", KBF_DATA_MANAGER_LOG_TAG, bundleName), DebugStack::Color::COL_WARNING);
            return;
        }
        for (size_t i = 0; i < presetUUIDs.size(); i++) {
            bool validate = (i == presetUUIDs.size() - 1); // Validate only on the last deletion.
            deletePreset(presetUUIDs[i], validate);
        }
        DEBUG_STACK.push(std::format("{} Deleted preset bundle: {}", KBF_DATA_MANAGER_LOG_TAG, bundleName), DebugStack::Color::COL_SUCCESS);
    }

    void KBFDataManager::deletePresetGroup(const std::string& uuid) {
        if (presetGroups.find(uuid) == presetGroups.end()) {
            DEBUG_STACK.push(std::format("{} Tried to delete preset group with UUID {}, but no such group exists. Skipping...", KBF_DATA_MANAGER_LOG_TAG, uuid), DebugStack::Color::COL_WARNING);
            return;
        }
        std::string presetGroupName = presetGroups.at(uuid).name;

        // Delete corresponding preset file.
        std::filesystem::path currPresetGroupPath = this->presetGroupPath / (presetGroupName + ".json");
        if (!std::filesystem::exists(currPresetGroupPath)) {
            DEBUG_STACK.push(std::format("{} Deleted preset group {} ({}) locally, but no corresponding .json file exists.", KBF_DATA_MANAGER_LOG_TAG, presetGroupName, uuid), DebugStack::Color::COL_WARNING);
            presetGroups.erase(uuid);
            validateObjectsUsingPresetGroups();
            return;
        }

        if (deleteJsonFile(currPresetGroupPath.string())) {
            DEBUG_STACK.push(std::format("{} Deleted preset group {} ({})", KBF_DATA_MANAGER_LOG_TAG, presetGroupName, uuid), DebugStack::Color::COL_SUCCESS);
            presetGroups.erase(uuid);
            validateObjectsUsingPresetGroups();
        }
    }

    void KBFDataManager::deletePlayerOverride(const PlayerData& player) {
        if (playerOverrides.find(player) == playerOverrides.end()) {
            DEBUG_STACK.push(std::format("{} Tried to delete player override for player {}, but no such group exists. Skipping...", KBF_DATA_MANAGER_LOG_TAG, player.string()), DebugStack::Color::COL_WARNING);
            return;
        }

        // Delete corresponding preset file.
        std::filesystem::path currPlayerOverridePath = this->playerOverridePath / (getPlayerOverrideFilename(player) + ".json");
        if (!std::filesystem::exists(currPlayerOverridePath)) {
            DEBUG_STACK.push(std::format("{} Deleted player override: {} locally, but no corresponding .json file exists ({}).", KBF_DATA_MANAGER_LOG_TAG, player.string(), currPlayerOverridePath.string()), DebugStack::Color::COL_WARNING);
            playerOverrides.erase(player);
            return;
        }

        if (deleteJsonFile(currPlayerOverridePath.string())) {
            DEBUG_STACK.push(std::format("{} Deleted player override: {}", KBF_DATA_MANAGER_LOG_TAG, player.string()), DebugStack::Color::COL_SUCCESS);
            playerOverrides.erase(player);
        }
    }

    void KBFDataManager::updatePreset(const std::string& uuid, Preset newPreset) {
        if (presets.find(uuid) == presets.end()) {
            DEBUG_STACK.push(std::format("{} Tried to update preset with UUID {}, but no such preset exists. Skipping...", KBF_DATA_MANAGER_LOG_TAG, uuid), DebugStack::Color::COL_WARNING);
            return;
        }
        Preset& currentPreset = presets.at(uuid);

        if (currentPreset.uuid != newPreset.uuid) {
            DEBUG_STACK.push(std::format("{} Tried to update preset {} ({}) to a preset with a different uuid: {}. Skipping...", KBF_DATA_MANAGER_LOG_TAG, currentPreset.name, currentPreset.uuid, newPreset.uuid), DebugStack::Color::COL_WARNING);
            return;
        }

        std::filesystem::path presetPathBefore = this->presetPath / (currentPreset.name + ".json");
        std::filesystem::path presetPathAfter  = this->presetPath / (newPreset.name + ".json");
        
        if (presetPathBefore != presetPathAfter) deleteJsonFile(presetPathBefore.string());
        if (writePreset(presetPathAfter, newPreset)) {
            DEBUG_STACK.push(std::format("{} Updated preset: {} -> {} ({})", KBF_DATA_MANAGER_LOG_TAG, currentPreset.name, newPreset.name, newPreset.uuid), DebugStack::Color::COL_SUCCESS);
            currentPreset = newPreset;
        }
    }

    void KBFDataManager::updatePresetGroup(const std::string& uuid, PresetGroup newPresetGroup) {
        if (presetGroups.find(uuid) == presetGroups.end()) {
            DEBUG_STACK.push(std::format("{} Tried to update preset group with UUID {}, but no such preset exists. Skipping...", KBF_DATA_MANAGER_LOG_TAG, uuid), DebugStack::Color::COL_WARNING);
            return;
        }
        PresetGroup& currentPresetGroup = presetGroups.at(uuid);

        if (currentPresetGroup.uuid != newPresetGroup.uuid) {
            DEBUG_STACK.push(std::format("{} Tried to update preset group {} ({}) to a preset group with a different uuid: {}. Skipping...", KBF_DATA_MANAGER_LOG_TAG, currentPresetGroup.name, currentPresetGroup.uuid, newPresetGroup.uuid), DebugStack::Color::COL_WARNING);
            return;
        }

        std::filesystem::path presetPathBefore = this->presetGroupPath / (currentPresetGroup.name + ".json");
        std::filesystem::path presetPathAfter  = this->presetGroupPath / (newPresetGroup.name + ".json");

        if (presetPathBefore != presetPathAfter) deleteJsonFile(presetPathBefore.string());
        if (writePresetGroup(presetPathAfter, newPresetGroup)) {
            DEBUG_STACK.push(std::format("{} Updated preset group: {} -> {} ({})", KBF_DATA_MANAGER_LOG_TAG, currentPresetGroup.name, newPresetGroup.name, newPresetGroup.uuid), DebugStack::Color::COL_SUCCESS);
            currentPresetGroup = newPresetGroup;
        }
    }

    void KBFDataManager::updatePlayerOverride(const PlayerData& player, PlayerOverride newOverride) {
        if (playerOverrides.find(player) == playerOverrides.end()) {
            DEBUG_STACK.push(std::format("{} Tried to update player override for player {}, but no such override exists. Skipping...", KBF_DATA_MANAGER_LOG_TAG, player.string()), DebugStack::Color::COL_WARNING);
            return;
        }
        PlayerOverride& currentOverride = playerOverrides.at(player);

        std::filesystem::path overridePathBefore = this->playerOverridePath / (getPlayerOverrideFilename(player) + ".json");
        std::filesystem::path overridePathAfter = this->playerOverridePath / (getPlayerOverrideFilename(newOverride.player) + ".json");

        if (overridePathBefore != overridePathAfter) deleteJsonFile(overridePathBefore.string());
        if (writePlayerOverride(overridePathAfter, newOverride)) {
            DEBUG_STACK.push(std::format("{} Updated player override: {} -> {}", KBF_DATA_MANAGER_LOG_TAG, currentOverride.player.string(), newOverride.player.string()), DebugStack::Color::COL_SUCCESS);
            // Have to update the entire entry here as data in the key is NOT constant
            playerOverrides.erase(player);
            playerOverrides.emplace(newOverride.player, newOverride);
        }
    }

    bool KBFDataManager::getFBSpresets(std::vector<FBSPreset>* out, bool female, std::string bundle, float* progressOut) const {
        if (!out) return false;
        if (!fbsDirectoryFound()) return false;

        out->clear();

        bool hasFailure = false;

        std::filesystem::path bodyPath = this->fbsPath / "Body";
        std::filesystem::path legPath = this->fbsPath / "Leg";

        if (!std::filesystem::exists(bodyPath)) {
            DEBUG_STACK.push(std::format("{} FBS Body presets directory does not exist at {}", KBF_DATA_MANAGER_LOG_TAG, bodyPath.string()), DebugStack::Color::COL_ERROR);
            return false;
        }

        if (!std::filesystem::exists(legPath)) {
            DEBUG_STACK.push(std::format("{} FBS Leg presets directory does not exist at {}", KBF_DATA_MANAGER_LOG_TAG, legPath.string()), DebugStack::Color::COL_ERROR);
            return false;
        }

        // lambda to count total JSON files (including subfolders)
        auto countJsonFiles = [](const std::filesystem::path& dir) -> size_t {
            size_t count = 0;
            for (const auto& entry : std::filesystem::recursive_directory_iterator(dir)) {
                if (entry.is_regular_file() && entry.path().extension() == ".json")
                    count++;
            }
            return count;
            };

        size_t totalPresets = countJsonFiles(bodyPath) + countJsonFiles(legPath);
        size_t processedCnt = 0;

        // lambda to load presets recursively
        auto loadPresetsFromDir = [&](const std::filesystem::path& dir, bool isBody) {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(dir)) {
                if (!entry.is_regular_file() || entry.path().extension() != ".json")
                    continue;

                FBSPreset preset;
                if (loadFBSPreset(entry.path(), isBody, female, bundle, &preset)) {
                    std::string subfolder = getRelativeSubfolder(dir, entry.path());
                    if (!subfolder.empty() && subfolder != ".") {
                        preset.preset.name = std::format("[{}] {}",
                            subfolder,
                            preset.preset.name
                        );
                    }

                    out->push_back(preset);
                }
                else {
                    hasFailure = true;
                }

                if (progressOut && totalPresets > 0) {
                    processedCnt++;
                    *progressOut = static_cast<float>(processedCnt) / static_cast<float>(totalPresets);
                }
            }
            };

        // Load body + leg presets (including subfolders)
        loadPresetsFromDir(bodyPath, true);
        loadPresetsFromDir(legPath, false);

        // Rename duplicates between groups for uniqueness
        std::unordered_map<std::string, std::vector<Preset*>> groups;
        for (FBSPreset& fbspreset : *out) {
            groups[fbspreset.preset.name].push_back(&fbspreset.preset);
        }

        for (auto& [name, vec] : groups) {
            if (vec.size() <= 1) continue;
            for (Preset* p : vec) {
                p->name = std::format(
                    "{} ({})",
                    name,
                    p->hasModifiers(ArmourPiece::AP_BODY) ? "Body" : "Legs"
                );
            }
        }

        return true;
    }

    void KBFDataManager::resolvePresetNameConflicts(std::vector<Preset>& presets) const {
        if (presets.empty()) return;

        // Resolve name conflicts by appending a number to the name if it already exists
        std::unordered_map<std::string, int> nameCount;
        for (auto& preset : presets) {
            std::string name = preset.name;

            std::string newName = name;
            size_t count = 1;
            while (presetExists(newName) && count < 1000) {
                // Try appending a number to the name until its unique
                newName = std::format("{} ({})", name, count);
                count++;
            }

            // Update the preset name
            preset.name = newName;
        }
    }

    void KBFDataManager::resolvePresetGroupNameConflicts(std::vector<PresetGroup>& presetGroups) const {
        if (presetGroups.empty()) return;

        // Resolve name conflicts by appending a number to the name if it already exists
        std::unordered_map<std::string, int> nameCount;
        for (auto& presetGroup : presetGroups) {
            std::string name = presetGroup.name;

            std::string newName = name;
            size_t count = 1;
            while (presetGroupExists(newName) && count < 1000) {
                // Try appending a number to the name until its unique
                newName = std::format("{} ({})", name, count);
                count++;
            }

            // Update the preset group name
            presetGroup.name = newName;
        }
    }

    bool KBFDataManager::importKBF(std::string filepath, size_t* conflictsCount) {
        DEBUG_STACK.push(std::format("{} Importing KBF file: \"{}\"", KBF_DATA_MANAGER_LOG_TAG, filepath), DebugStack::Color::COL_INFO);

        KBFFileData data;
        bool success = readKBF(filepath, &data);

        if (!success) return false;

        resolvePresetNameConflicts(data.presets);
        resolvePresetGroupNameConflicts(data.presetGroups);

        size_t nConflicts = 0;

        for (Preset& preset : data.presets)                         nConflicts += !addPreset(preset, true);
        for (PresetGroup& presetGroup : data.presetGroups)          nConflicts += !addPresetGroup(presetGroup, true);
        for (PlayerOverride& playerOverride : data.playerOverrides) nConflicts += !addPlayerOverride(playerOverride, true);

        if (conflictsCount != nullptr) *conflictsCount = nConflicts;
        return success;
    }

    bool KBFDataManager::readKBF(std::string filepath, KBFFileData* out) const {
        assert(out != nullptr);

        rapidjson::Document doc = loadConfigJson(KbfFileType::DOT_KBF, filepath, nullptr);
        if (!doc.IsObject() || doc.HasParseError()) return false;

        bool parsed = true;
        parsed &= parseString(doc, FORMAT_VERSION_ID, FORMAT_VERSION_ID, &out->metadata.VERSION);
        parsed &= parseString(doc, FORMAT_VERSION_ID, FORMAT_VERSION_ID, &out->metadata.VERSION);

        // Load presets
        parsed &= parseObject(doc, KBF_FILE_PRESETS_ID, KBF_FILE_PRESETS_ID);
        if (parsed) {
            const rapidjson::Value& presets = doc[KBF_FILE_PRESETS_ID];
            for (const auto& preset : presets.GetObject()) {
                Preset presetOut;
                presetOut.name = preset.name.GetString();

                if (preset.value.IsObject()) {
                    parsed &= loadPresetData(preset.value, &presetOut);

                    out->presets.push_back(std::move(presetOut));
                }
                else {
                    DEBUG_STACK.push(std::format("{} Failed to parse preset {} in KBF \"{}\". Expected an object, but got a different type.", KBF_DATA_MANAGER_LOG_TAG, presetOut.name, filepath), DebugStack::Color::COL_ERROR);
                }
            }
        }

        // Load Preset Groups
        parsed &= parseObject(doc, KBF_FILE_PRESETS_GROUPS_ID, KBF_FILE_PRESETS_GROUPS_ID);
        if (parsed) {
            const rapidjson::Value& presetGroups = doc[KBF_FILE_PRESETS_GROUPS_ID];
            for (const auto& presetGroup : presetGroups.GetObject()) {
                PresetGroup presetGroupOut;
                presetGroupOut.name = presetGroup.name.GetString();

                if (presetGroup.value.IsObject()) {
                    parsed &= loadPresetGroupData(presetGroup.value, &presetGroupOut);

                    out->presetGroups.push_back(std::move(presetGroupOut));
                }
                else {
                    DEBUG_STACK.push(std::format("{} Failed to parse preset group {} in .KBF \"{}\". Expected an object, but got a different type.", KBF_DATA_MANAGER_LOG_TAG, presetGroupOut.name, filepath), DebugStack::Color::COL_ERROR);
                }
            }
        }

        // Load Player Overrides
        parsed &= parseObject(doc, KBF_FILE_PLAYER_OVERRIDES_ID, KBF_FILE_PLAYER_OVERRIDES_ID);
        if (parsed) {
            const rapidjson::Value& playerOverrides = doc[KBF_FILE_PLAYER_OVERRIDES_ID];
            for (const auto& override : playerOverrides.GetObject()) {
                PlayerOverride overrideOut;

                if (override.value.IsObject()) {
                    parsed &= loadPlayerOverrideData(override.value, &overrideOut);

                    out->playerOverrides.push_back(std::move(overrideOut));
                }
                else {
                    DEBUG_STACK.push(std::format("{} Failed to parse player override {} in .KBF \"{}\". Expected an object, but got a different type.", KBF_DATA_MANAGER_LOG_TAG, override.name.GetString(), filepath), DebugStack::Color::COL_ERROR);
                }
            }
        }

        if (!parsed) {
            DEBUG_STACK.push(std::format("{} Failed to parse KBF {}. One or more required values were invalid / missing. Please rectify or remove the file.", KBF_DATA_MANAGER_LOG_TAG, filepath), DebugStack::Color::COL_ERROR);
        }

        return parsed;
    }

    bool KBFDataManager::writeKBF(std::string filepath, KBFFileData data) const {
        rapidjson::StringBuffer s;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(s);

        writer.StartObject();
        writer.Key(FORMAT_VERSION_ID);
        writer.String(data.metadata.VERSION.c_str());
        writer.Key(FORMAT_MOD_ARCHIVE_ID);
        writer.String(data.metadata.MOD_ARCHIVE.c_str());
        writer.Key(KBF_FILE_PRESETS_GROUPS_ID);
        writer.StartObject();
        for (const PresetGroup& presetGroup : data.presetGroups) {
            writer.Key(presetGroup.name.c_str());
            writePresetGroupJsonContent(presetGroup, writer);
        }
        writer.EndObject();
        writer.Key(KBF_FILE_PRESETS_ID);
        writer.StartObject();
        for (const Preset& preset : data.presets) {
            writer.Key(preset.name.c_str());
            writePresetJsonContent(preset, writer);
        }
        writer.EndObject();
        writer.Key(KBF_FILE_PLAYER_OVERRIDES_ID);
        writer.StartObject();
        for (const PlayerOverride& playerOverride : data.playerOverrides) {
            writer.Key(getPlayerOverrideFilename(playerOverride.player).c_str());
            writePlayerOverrideJsonContent(playerOverride, writer);
        }
        writer.EndObject();
        writer.EndObject();

        bool success = writeJsonFile(filepath, s.GetString());

        if (!success) {
            DEBUG_STACK.push(std::format("{} Failed to write kbf file: \"{}\"", KBF_DATA_MANAGER_LOG_TAG, filepath), DebugStack::Color::COL_ERROR);
        }

        return success;
    }

    bool KBFDataManager::writeModArchive(std::string filepath, KBFFileData data) const {
        try {
            miniz_cpp::zip_file zip;

            // Add PresetGroups
            for (const PresetGroup& presetGroup : data.presetGroups) {
                std::ostringstream oss;
                oss << "reframework/data/KBF/PresetGroups/" << presetGroup.name << ".json";

                rapidjson::StringBuffer s;
                rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(s);
                writePresetGroupJsonContent(presetGroup, writer);

                zip.writestr(oss.str(), s.GetString());
            }

            // Add Presets
            for (const Preset& preset : data.presets) {
                std::ostringstream oss;
                oss << "reframework/data/KBF/Presets/" << preset.name << ".json";

                rapidjson::StringBuffer s;
                rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(s);
                writePresetJsonContent(preset, writer);

                zip.writestr(oss.str(), s.GetString());
            }

            // Add PlayerOverrides
            for (const PlayerOverride& override : data.playerOverrides) {
                std::ostringstream oss;
                oss << "reframework/data/KBF/PlayerOverrides/" << getPlayerOverrideFilename(override.player) << ".json";

                rapidjson::StringBuffer s;
                rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(s);
                writePlayerOverrideJsonContent(override, writer);

                zip.writestr(oss.str(), s.GetString());
            }

            // Save the archive to disk
            zip.save(filepath);
            return true;
        }
        catch (const std::exception& e) {
            DEBUG_STACK.push(std::format("{} Error writing zip file @ \"{}\": {}", KBF_DATA_MANAGER_LOG_TAG, filepath, e.what()), DebugStack::Color::COL_ERROR);
            return false;
        }
    }

    void KBFDataManager::deleteLocalModArchive(std::string name) {
        DEBUG_STACK.push(std::format("{} Deleting local mod archive: {}", KBF_DATA_MANAGER_LOG_TAG, name), DebugStack::Color::COL_INFO);

        std::vector<std::string> presetsToDelete;
        for (const auto& [uuid, preset] : presets) {
            if (preset.metadata.MOD_ARCHIVE == name) {
                presetsToDelete.push_back(uuid);
            }
        }
        std::vector<std::string> presetGroupsToDelete;
        for (const auto& [uuid, presetGroup] : presetGroups) {
            if (presetGroup.metadata.MOD_ARCHIVE == name) {
                presetGroupsToDelete.push_back(uuid);
            }
        }
        std::vector<PlayerData> overridesToDelete;
        for (const auto& [player, playerOverride] : playerOverrides) {
            if (playerOverride.metadata.MOD_ARCHIVE == name) {
                overridesToDelete.push_back(player);
            }
        }

        for (const std::string& uuid : presetsToDelete) {
            deletePreset(uuid, true);
        }
        for (const std::string& uuid : presetGroupsToDelete) {
            deletePresetGroup(uuid);
        }
        for (const PlayerData& player : overridesToDelete) {
            deletePlayerOverride(player);
        }

        DEBUG_STACK.push(std::format("{} Deleted {} presets, {} preset groups, and {} player overrides from mod archive: {}", 
            KBF_DATA_MANAGER_LOG_TAG, presetsToDelete.size(), presetGroupsToDelete.size(), overridesToDelete.size(), name), DebugStack::Color::COL_SUCCESS);
    }

    std::unordered_map<std::string, KBFDataManager::ModArchiveCounts> KBFDataManager::getModArchiveInfo() const {
        std::unordered_map<std::string, ModArchiveCounts> counts;
        for (const auto& [uuid, preset] : presets) {
            if (!preset.metadata.MOD_ARCHIVE.empty()) {
                counts[preset.metadata.MOD_ARCHIVE].presets++;
            }
        }
        for (const auto& [uuid, presetGroup] : presetGroups) {
            if (!presetGroup.metadata.MOD_ARCHIVE.empty()) {
                counts[presetGroup.metadata.MOD_ARCHIVE].presetGroups++;
            }
        }
        for (const auto& [player, playerOverride] : playerOverrides) {
            if (!playerOverride.metadata.MOD_ARCHIVE.empty()) {
                counts[playerOverride.metadata.MOD_ARCHIVE].playerOverrides++;
            }
        }
        return counts;
    }

    void KBFDataManager::verifyDirectoriesExist() const {
        createDirectoryIfNotExists(dataBasePath);
        createDirectoryIfNotExists(defaultConfigsPath);
        createDirectoryIfNotExists(presetPath);
        createDirectoryIfNotExists(presetGroupPath);
        createDirectoryIfNotExists(playerOverridePath);
        createDirectoryIfNotExists(exportsPath);
    }

    void KBFDataManager::createDirectoryIfNotExists(const std::filesystem::path& path) const {
        if (!std::filesystem::exists(path)) {
            std::filesystem::create_directories(path);
        }
    }

    rapidjson::Document KBFDataManager::loadConfigJson(KbfFileType fileType, const std::string& path, std::function<bool()> onRequestCreateDefault) const {
        bool exists = std::filesystem::exists(path);
        if (!exists && onRequestCreateDefault) {
            DEBUG_STACK.push(std::format("{} Json file does not exist at {}. Creating...", KBF_DATA_MANAGER_LOG_TAG, path), DebugStack::Color::COL_WARNING);

            bool createdDefault = INVOKE_OPTIONAL_CALLBACK_TYPED(bool, false, onRequestCreateDefault);

            if (createdDefault) {
                DEBUG_STACK.push(std::format("{} Created default json at {}", KBF_DATA_MANAGER_LOG_TAG, path), DebugStack::Color::COL_SUCCESS);
            }
        }

        std::string json = readJsonFile(path);

        rapidjson::Document config;
        config.Parse(json.c_str());

        if (!config.IsObject() || config.HasParseError()) {
            DEBUG_STACK.push(std::format("{} Failed to parse json at {}. Please rectify or delete the file.", KBF_DATA_MANAGER_LOG_TAG, path), DebugStack::Color::COL_ERROR);

            bool createdDefault = INVOKE_OPTIONAL_CALLBACK_TYPED(bool, false, onRequestCreateDefault);

            if (createdDefault) {
                DEBUG_STACK.push(std::format("{} Created default json at {}", KBF_DATA_MANAGER_LOG_TAG, path), DebugStack::Color::COL_SUCCESS);
            }
        }

        // Handle upgrading the file
        static KbfFileUpgrader upgrader{};
        KbfFileUpgrader::UpgradeResult res = upgrader.upgradeFile(config, fileType);

        if (res == KbfFileUpgrader::UpgradeResult::FAILED) {
			DEBUG_STACK.push(std::format("{} Failed to upgrade json file at {}. Please rectify or delete the file.", KBF_DATA_MANAGER_LOG_TAG, path), DebugStack::Color::COL_ERROR);
        }

        if (res == KbfFileUpgrader::UpgradeResult::SUCCESS) {
            DEBUG_STACK.push(std::format("{} Upgraded json file at \"{}\" to the latest format.", KBF_DATA_MANAGER_LOG_TAG, path), DebugStack::Color::COL_SUCCESS);

		    // Before rewriting the file, make a backup of the existing one
			std::string backupPath = path + ".backup";
			writeJsonFile(backupPath, json);
			DEBUG_STACK.push(std::format("{} Created backup of the previous version at {}", KBF_DATA_MANAGER_LOG_TAG, backupPath), DebugStack::Color::COL_INFO);

            // Write the upgraded file back to disk
            rapidjson::StringBuffer s;
            rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(s);
            config.Accept(writer);
            writeJsonFile(path, s.GetString());
		}

        return config;
    }

    std::string KBFDataManager::readJsonFile(const std::string& path) const {
        std::wstring wpath = cvt_utf8_to_utf16(path);
        std::ifstream file{ wpath, std::ios::ate };

        if (!file.is_open()) {
            const std::string error_pth_out{ path };
            DEBUG_STACK.push(std::format("{} Could not open json file for read at {}", KBF_DATA_MANAGER_LOG_TAG, error_pth_out), DebugStack::Color::COL_ERROR);
            return "";
        }

        size_t fileSize = static_cast<size_t>(file.tellg());
        std::string buffer(fileSize, ' ');
        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();

        return buffer;
    }

    bool KBFDataManager::writeJsonFile(std::string path, const std::string& json) const {
        std::wstring wpath = cvt_utf8_to_utf16(path);

        // Open file in binary mode to avoid any encoding conversion
        std::ofstream file(wpath, std::ios::binary | std::ios::trunc);
        if (!file.is_open()) return false;

        // Write JSON content as-is (assumed UTF-8)
        file.write(json.data(), json.size());
        file.close();

        return true;
    }

    bool KBFDataManager::deleteJsonFile(std::string path) const {
        try {
            if (std::filesystem::remove(path)) {
                return true;
            }
            else {
                DEBUG_STACK.push(std::format("{} Failed to delete json file: {}", KBF_DATA_MANAGER_LOG_TAG, path), DebugStack::Color::COL_ERROR);
                return false;
            }
        }
        catch (const std::filesystem::filesystem_error& e) {
            DEBUG_STACK.push(std::format("{} Failed to delete json file: {}", KBF_DATA_MANAGER_LOG_TAG, path), DebugStack::Color::COL_ERROR);
            return false;
        }

    }

    bool KBFDataManager::loadSettings(KBFSettings* out) {
        assert(out != nullptr);

        rapidjson::Document config = loadConfigJson(KbfFileType::SETTINGS, settingsPath.string(), [&]() {
            KBFSettings temp{};
            return writeSettings(temp);
        });
        if (!config.IsObject() || config.HasParseError()) return false;

        // Outfits 
        parseBool(config, SETTINGS_KBF_ENABLED_ID, SETTINGS_KBF_ENABLED_ID, &out->enabled);
        parseBool(config, SETTINGS_KBF_ENABLE_PLAYERS_ID, SETTINGS_KBF_ENABLE_PLAYERS_ID, &out->enablePlayers);
        parseBool(config, SETTINGS_KBF_ENABLE_NPCS_ID, SETTINGS_KBF_ENABLE_NPCS_ID, &out->enableNpcs);
        parseFloat(config, SETTINGS_DELAY_ON_EQUIP_ID, SETTINGS_DELAY_ON_EQUIP_ID, &out->delayOnEquip);
        parseFloat(config, SETTINGS_APPLICATION_RANGE_ID, SETTINGS_APPLICATION_RANGE_ID, &out->applicationRange);
        parseInt(config, SETTINGS_MAX_CONCURRENT_APPLICATIONS_ID, SETTINGS_MAX_CONCURRENT_APPLICATIONS_ID, &out->maxConcurrentApplications);
        parseInt(config, SETTINGS_MAX_BONE_FETCHES_PER_FRAME_ID, SETTINGS_MAX_BONE_FETCHES_PER_FRAME_ID, &out->maxBoneFetchesPerFrame);
        parseBool(config, SETTINGS_ENABLE_DURING_QUESTS_ONLY_ID, SETTINGS_ENABLE_DURING_QUESTS_ONLY_ID, &out->enableDuringQuestsOnly);
        parseBool(config, SETTINGS_ENABLE_HIDE_WEAPONS_ID, SETTINGS_ENABLE_HIDE_WEAPONS_ID, &out->enableHideWeapons);
		parseBool(config, SETTINGS_ENABLE_HIDE_KINSECT_ID, SETTINGS_ENABLE_HIDE_KINSECT_ID, &out->enableHideKinsect);
        parseBool(config, SETTINGS_FORCE_SHOW_WEAPON_IN_TENT_ID, SETTINGS_FORCE_SHOW_WEAPON_IN_TENT_ID, &out->forceShowWeaponInTent);
        parseBool(config, SETTINGS_FORCE_SHOW_WEAPON_WHEN_SHARPENING_ID, SETTINGS_FORCE_SHOW_WEAPON_WHEN_SHARPENING_ID, &out->forceShowWeaponWhenSharpening);
        parseBool(config, SETTINGS_FORCE_SHOW_WEAPON_WHEN_ON_SEIKRET_ID, SETTINGS_FORCE_SHOW_WEAPON_WHEN_ON_SEIKRET_ID, &out->forceShowWeaponWhenOnSeikret);
        parseBool(config, SETTINGS_HIDE_WEAPONS_OUTSIDE_OF_COMBAT_ONLY_ID, SETTINGS_HIDE_WEAPONS_OUTSIDE_OF_COMBAT_ONLY_ID, &out->hideWeaponsOutsideOfCombatOnly);
        parseBool(config, SETTINGS_HIDE_SLINGER_OUTSIDE_OF_COMBAT_ONLY_ID, SETTINGS_HIDE_SLINGER_OUTSIDE_OF_COMBAT_ONLY_ID, &out->hideSlingerOutsideOfCombatOnly);
        parseBool(config, SETTINGS_ENABLE_PROFILING_ID, SETTINGS_ENABLE_PROFILING_ID, &out->enableProfiling);

        DEBUG_STACK.push(std::format("{} Loaded Settings from {}", KBF_DATA_MANAGER_LOG_TAG, settingsPath.string()), DebugStack::Color::COL_SUCCESS);
        return true;
    }

    bool KBFDataManager::writeSettings(const KBFSettings& settings) const {
        rapidjson::StringBuffer s;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(s);

        writer.StartObject();
        writer.Key(SETTINGS_KBF_ENABLED_ID);
        writer.Bool(settings.enabled);
        writer.Key(SETTINGS_KBF_ENABLE_PLAYERS_ID);
        writer.Bool(settings.enablePlayers);
        writer.Key(SETTINGS_KBF_ENABLE_NPCS_ID);
        writer.Bool(settings.enableNpcs);
        writer.Key(SETTINGS_DELAY_ON_EQUIP_ID);
        writer.Double(settings.delayOnEquip);
        writer.Key(SETTINGS_APPLICATION_RANGE_ID);
        writer.Double(settings.applicationRange);
        writer.Key(SETTINGS_MAX_CONCURRENT_APPLICATIONS_ID);
        writer.Int(settings.maxConcurrentApplications);
        writer.Key(SETTINGS_MAX_BONE_FETCHES_PER_FRAME_ID);
        writer.Int(settings.maxBoneFetchesPerFrame);
        writer.Key(SETTINGS_ENABLE_DURING_QUESTS_ONLY_ID);
        writer.Bool(settings.enableDuringQuestsOnly);
		writer.Key(SETTINGS_ENABLE_HIDE_WEAPONS_ID);
		writer.Bool(settings.enableHideWeapons);
		writer.Key(SETTINGS_ENABLE_HIDE_KINSECT_ID);
		writer.Bool(settings.enableHideKinsect);
		writer.Key(SETTINGS_FORCE_SHOW_WEAPON_IN_TENT_ID);
		writer.Bool(settings.forceShowWeaponInTent);
		writer.Key(SETTINGS_FORCE_SHOW_WEAPON_WHEN_SHARPENING_ID);
		writer.Bool(settings.forceShowWeaponWhenSharpening);
		writer.Key(SETTINGS_FORCE_SHOW_WEAPON_WHEN_ON_SEIKRET_ID);
		writer.Bool(settings.forceShowWeaponWhenOnSeikret);
        writer.Key(SETTINGS_HIDE_WEAPONS_OUTSIDE_OF_COMBAT_ONLY_ID);
        writer.Bool(settings.hideWeaponsOutsideOfCombatOnly);
        writer.Key(SETTINGS_HIDE_SLINGER_OUTSIDE_OF_COMBAT_ONLY_ID);
        writer.Bool(settings.hideSlingerOutsideOfCombatOnly);
        writer.Key(SETTINGS_ENABLE_PROFILING_ID);
        writer.Bool(settings.enableProfiling);
        writer.EndObject();

        bool success = writeJsonFile(settingsPath.string(), s.GetString());

        if (!success) {
            DEBUG_STACK.push(std::format("{} Failed to write settings to {}", KBF_DATA_MANAGER_LOG_TAG, settingsPath.string()), DebugStack::Color::COL_ERROR);
        }

        return success;
    }
    
    bool KBFDataManager::loadAlmaConfig(AlmaDefaults* out) {
        assert(out != nullptr);

        DEBUG_STACK.push(std::format("{} Loading Alma Config...", KBF_DATA_MANAGER_LOG_TAG), DebugStack::Color::COL_INFO);

        rapidjson::Document config = loadConfigJson(KbfFileType::ALMA_CONFIG, almaConfigPath.string(), [&]() {
            AlmaDefaults temp{};
            return writeAlmaConfig(temp);
        });
        if (!config.IsObject() || config.HasParseError()) return false;

        parseString(config, FORMAT_VERSION_ID, FORMAT_VERSION_ID, &out->metadata.VERSION);
        parseString(config, FORMAT_MOD_ARCHIVE_ID, FORMAT_MOD_ARCHIVE_ID, &out->metadata.MOD_ARCHIVE);

        // Outfits 
        parseString(config, ALMA_HANDLERS_OUTFIT_ID, ALMA_HANDLERS_OUTFIT_ID, &out->handlersOutfit);
        parseString(config, ALMA_NEW_WORLD_COMISSION_ID, ALMA_NEW_WORLD_COMISSION_ID, &out->newWorldCommission);
        parseString(config, ALMA_SCRIVENERS_COAT_ID, ALMA_SCRIVENERS_COAT_ID, &out->scrivenersCoat);
        parseString(config, ALMA_SPRING_BLOSSOM_KIMONO_ID, ALMA_SPRING_BLOSSOM_KIMONO_ID, &out->springBlossomKimono);
        parseString(config, ALMA_CHUN_LI_OUTFIT_ID, ALMA_CHUN_LI_OUTFIT_ID, &out->chunLiOutfit);
        parseString(config, ALMA_CAMMY_OUTFIT_ID, ALMA_CAMMY_OUTFIT_ID, &out->cammyOutfit);
        parseString(config, ALMA_SUMMER_PONCHO_ID, ALMA_SUMMER_PONCHO_ID, &out->summerPoncho);
        parseString(config, ALMA_AUTUMN_WITCH_ID, ALMA_AUTUMN_WITCH_ID, &out->autumnWitch);
        parseString(config, ALMA_FEATHERSKIRT_SEIKRET_DRESS_ID, ALMA_FEATHERSKIRT_SEIKRET_DRESS_ID, &out->featherskirtSeikretDress);

        DEBUG_STACK.push(std::format("{} Loaded Alma config from {}", KBF_DATA_MANAGER_LOG_TAG, almaConfigPath.string()), DebugStack::Color::COL_SUCCESS);
        return true;
    }

    bool KBFDataManager::writeAlmaConfig(const AlmaDefaults& out) const {
        rapidjson::StringBuffer s;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(s);

        writer.StartObject();
        writer.Key(FORMAT_VERSION_ID);
        writer.String(out.metadata.VERSION.c_str());
        writer.Key(FORMAT_MOD_ARCHIVE_ID);
        writer.String(out.metadata.MOD_ARCHIVE.c_str());
        writer.Key(ALMA_HANDLERS_OUTFIT_ID);
        writer.String(out.handlersOutfit.c_str());
        writer.Key(ALMA_NEW_WORLD_COMISSION_ID);
        writer.String(out.newWorldCommission.c_str());
        writer.Key(ALMA_SCRIVENERS_COAT_ID);
        writer.String(out.scrivenersCoat.c_str());
        writer.Key(ALMA_SPRING_BLOSSOM_KIMONO_ID);
        writer.String(out.springBlossomKimono.c_str());
        writer.Key(ALMA_CHUN_LI_OUTFIT_ID);
        writer.String(out.chunLiOutfit.c_str());
        writer.Key(ALMA_CAMMY_OUTFIT_ID);
        writer.String(out.cammyOutfit.c_str());
        writer.Key(ALMA_SUMMER_PONCHO_ID);
        writer.String(out.summerPoncho.c_str());
        writer.Key(ALMA_AUTUMN_WITCH_ID);
        writer.String(out.autumnWitch.c_str());
		writer.Key(ALMA_FEATHERSKIRT_SEIKRET_DRESS_ID);
		writer.String(out.featherskirtSeikretDress.c_str());
        writer.EndObject();

        bool success = writeJsonFile(almaConfigPath.string(), s.GetString());

        if (!success) {
            DEBUG_STACK.push(std::format("{} Failed to write Alma config to {}", KBF_DATA_MANAGER_LOG_TAG, almaConfigPath.string()), DebugStack::Color::COL_ERROR);
        }

        return success;
    }

    bool KBFDataManager::loadErikConfig(ErikDefaults* out) {
        assert(out != nullptr);

        DEBUG_STACK.push(std::format("{} Loading Erik Config...", KBF_DATA_MANAGER_LOG_TAG), DebugStack::Color::COL_INFO);

        rapidjson::Document config = loadConfigJson(KbfFileType::ERIK_CONFIG, erikConfigPath.string(), [&]() {
            ErikDefaults temp{};
            return writeErikConfig(temp);
        });
        if (!config.IsObject() || config.HasParseError()) return false;

        parseString(config, FORMAT_VERSION_ID, FORMAT_VERSION_ID, &out->metadata.VERSION);
        parseString(config, FORMAT_MOD_ARCHIVE_ID, FORMAT_MOD_ARCHIVE_ID, &out->metadata.MOD_ARCHIVE);

        // Outfits
        parseString(config, ERIK_HANDLERS_OUTFIT_ID, ERIK_HANDLERS_OUTFIT_ID, &out->handlersOutfit);
        parseString(config, ERIK_SUMMER_HAT_ID, ERIK_SUMMER_HAT_ID, &out->summerHat);
        parseString(config, ERIK_AUTUMN_THERIAN_ID, ERIK_AUTUMN_THERIAN_ID, &out->autumnTherian);
		parseString(config, ERIK_CRESTCOLLAR_SEIKRET_SUIT_ID, ERIK_CRESTCOLLAR_SEIKRET_SUIT_ID, &out->crestcollarSeikretSuit);

        DEBUG_STACK.push(std::format("{} Loaded Erik config from {}", KBF_DATA_MANAGER_LOG_TAG, erikConfigPath.string()), DebugStack::Color::COL_SUCCESS);
        return true;
    }

    bool KBFDataManager::writeErikConfig(const ErikDefaults& out) const {
        rapidjson::StringBuffer s;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(s);

        writer.StartObject();
        writer.Key(FORMAT_VERSION_ID);
        writer.String(out.metadata.VERSION.c_str());
        writer.Key(FORMAT_MOD_ARCHIVE_ID);
        writer.String(out.metadata.MOD_ARCHIVE.c_str());
        writer.Key(ERIK_HANDLERS_OUTFIT_ID);
        writer.String(out.handlersOutfit.c_str());
        writer.Key(ERIK_SUMMER_HAT_ID);
        writer.String(out.summerHat.c_str());
        writer.Key(ERIK_AUTUMN_THERIAN_ID);
        writer.String(out.autumnTherian.c_str());
		writer.Key(ERIK_CRESTCOLLAR_SEIKRET_SUIT_ID);
		writer.String(out.crestcollarSeikretSuit.c_str());
        writer.EndObject();

        bool success = writeJsonFile(erikConfigPath.string(), s.GetString());

        if (!success) {
            DEBUG_STACK.push(std::format("{} Failed to write Erik config to ", KBF_DATA_MANAGER_LOG_TAG, erikConfigPath.string()), DebugStack::Color::COL_ERROR);
        }

        return success;
    }

    bool KBFDataManager::loadGemmaConfig(GemmaDefaults* out) {
        assert(out != nullptr);

        DEBUG_STACK.push(std::format("{} Loading Gemma Config...", KBF_DATA_MANAGER_LOG_TAG), DebugStack::Color::COL_INFO);

        rapidjson::Document config = loadConfigJson(KbfFileType::GEMMA_CONFIG, gemmaConfigPath.string(), [&]() {
            GemmaDefaults temp{};
            return writeGemmaConfig(temp);
        });
        if (!config.IsObject() || config.HasParseError()) return false;

        parseString(config, FORMAT_VERSION_ID, FORMAT_VERSION_ID, &out->metadata.VERSION);
        parseString(config, FORMAT_MOD_ARCHIVE_ID, FORMAT_MOD_ARCHIVE_ID, &out->metadata.MOD_ARCHIVE);

        // Outfits
        parseString(config, GEMMA_SMITHYS_OUTFIT_ID, GEMMA_SMITHYS_OUTFIT_ID, &out->smithysOutfit);
        parseString(config, GEMMA_SUMMER_COVERALLS_ID, GEMMA_SUMMER_COVERALLS_ID, &out->summerCoveralls);
		parseString(config, GEMMA_REDVEIL_SEIKRET_DRESS_ID, GEMMA_REDVEIL_SEIKRET_DRESS_ID, &out->redveilSeikretDress);

        DEBUG_STACK.push(std::format("{} Loaded Gemma config from {}", KBF_DATA_MANAGER_LOG_TAG, gemmaConfigPath.string()), DebugStack::Color::COL_SUCCESS);
        return true;
    }

    bool KBFDataManager::writeGemmaConfig(const GemmaDefaults& out) const {
        rapidjson::StringBuffer s;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(s);

        writer.StartObject();
        writer.Key(FORMAT_VERSION_ID);
        writer.String(out.metadata.VERSION.c_str());
        writer.Key(FORMAT_MOD_ARCHIVE_ID);
        writer.String(out.metadata.MOD_ARCHIVE.c_str());
        writer.Key(GEMMA_SMITHYS_OUTFIT_ID);
        writer.String(out.smithysOutfit.c_str());
        writer.Key(GEMMA_SUMMER_COVERALLS_ID);
        writer.String(out.summerCoveralls.c_str());
		writer.Key(GEMMA_REDVEIL_SEIKRET_DRESS_ID);
		writer.String(out.redveilSeikretDress.c_str());
        writer.EndObject();

        bool success = writeJsonFile(gemmaConfigPath.string(), s.GetString());

        if (!success) {
            DEBUG_STACK.push(std::format("{} Failed to write Gemma config to {}", KBF_DATA_MANAGER_LOG_TAG, gemmaConfigPath.string()), DebugStack::Color::COL_ERROR);
        }

        return success;
    }

    bool KBFDataManager::loadNpcConfig(NpcDefaults* out) {
        assert(out != nullptr);

        DEBUG_STACK.push(std::format("{} Loading NPC Config...", KBF_DATA_MANAGER_LOG_TAG), DebugStack::Color::COL_INFO);

        rapidjson::Document config = loadConfigJson(KbfFileType::NPC_CONFIG, npcConfigPath.string(), [&]() {
            NpcDefaults temp{};
            return writeNpcConfig(temp);
        });
        if (!config.IsObject() || config.HasParseError()) return false;

        parseString(config, FORMAT_VERSION_ID, FORMAT_VERSION_ID, &out->metadata.VERSION);
        parseString(config, FORMAT_MOD_ARCHIVE_ID, FORMAT_MOD_ARCHIVE_ID, &out->metadata.MOD_ARCHIVE);

        parseString(config, NPC_MALE_ID, NPC_MALE_ID, &out->male);
        parseString(config, NPC_FEMALE_ID, NPC_FEMALE_ID, &out->female);

        DEBUG_STACK.push(std::format("{} Loaded NPC config from {}", KBF_DATA_MANAGER_LOG_TAG, npcConfigPath.string()), DebugStack::Color::COL_SUCCESS);
        return true;
    }

    bool KBFDataManager::writeNpcConfig(const NpcDefaults& out) const {
        rapidjson::StringBuffer s;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(s);

        writer.StartObject();
        writer.Key(FORMAT_VERSION_ID);
        writer.String(out.metadata.VERSION.c_str());
        writer.Key(FORMAT_MOD_ARCHIVE_ID);
        writer.String(out.metadata.MOD_ARCHIVE.c_str());
        writer.Key(NPC_MALE_ID);
        writer.String(out.male.c_str());
        writer.Key(NPC_FEMALE_ID);
        writer.String(out.female.c_str());
        writer.EndObject();

        bool success = writeJsonFile(npcConfigPath.string(), s.GetString());

        if (!success) {
            DEBUG_STACK.push(std::format("{} Failed to write NPC config to {}", KBF_DATA_MANAGER_LOG_TAG, npcConfigPath.string()), DebugStack::Color::COL_ERROR);
        }

        return success;
    }

    bool KBFDataManager::loadPlayerConfig(PlayerDefaults* out) {
        assert(out != nullptr);

        DEBUG_STACK.push(std::format("{} Loading Player Config...", KBF_DATA_MANAGER_LOG_TAG), DebugStack::Color::COL_INFO);

        rapidjson::Document config = loadConfigJson(KbfFileType::PLAYER_CONFIG, playerConfigPath.string(), [&]() {
            PlayerDefaults temp{};
            return writePlayerConfig(temp);
        });
        if (!config.IsObject() || config.HasParseError()) return false;

        parseString(config, FORMAT_VERSION_ID, FORMAT_VERSION_ID, &out->metadata.VERSION);
        parseString(config, FORMAT_MOD_ARCHIVE_ID, FORMAT_MOD_ARCHIVE_ID, &out->metadata.MOD_ARCHIVE);

        parseString(config, PLAYER_MALE_ID, PLAYER_MALE_ID, &out->male);
        parseString(config, PLAYER_FEMALE_ID, PLAYER_FEMALE_ID, &out->female);

        DEBUG_STACK.push(std::format("{} Loaded Player config from {}", KBF_DATA_MANAGER_LOG_TAG, playerConfigPath.string()), DebugStack::Color::COL_SUCCESS);
        return true;
    }

    bool KBFDataManager::writePlayerConfig(const PlayerDefaults& out) const {
        rapidjson::StringBuffer s;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(s);

        writer.StartObject();
        writer.Key(FORMAT_VERSION_ID);
        writer.String(out.metadata.VERSION.c_str());
        writer.Key(FORMAT_MOD_ARCHIVE_ID);
        writer.String(out.metadata.MOD_ARCHIVE.c_str());
        writer.Key(PLAYER_MALE_ID);
        writer.String(out.male.c_str());
        writer.Key(PLAYER_FEMALE_ID);
        writer.String(out.female.c_str());
        writer.EndObject();

        bool success = writeJsonFile(playerConfigPath.string(), s.GetString());

        if (!success) {
            DEBUG_STACK.push(std::format("{} Failed to write Player config to {}", KBF_DATA_MANAGER_LOG_TAG, playerConfigPath.string()), DebugStack::Color::COL_ERROR);
        }

        return success;
    }

    bool KBFDataManager::loadPreset(const std::filesystem::path& path, Preset* out) {
        assert(out != nullptr);

        rapidjson::Document presetDoc = loadConfigJson(KbfFileType::PRESET, path.string(), nullptr);
        if (!presetDoc.IsObject() || presetDoc.HasParseError()) return false;

        out->name = path.stem().string();

        bool parsed = loadPresetData(presetDoc, out);

        if (!parsed) {
            DEBUG_STACK.push(std::format("{} Failed to parse preset {}. One or more required values were invalid / missing. Please rectify or remove the file.", KBF_DATA_MANAGER_LOG_TAG, path.string()), DebugStack::Color::COL_ERROR);
        }

        return parsed;
    }

    bool KBFDataManager::loadPresetData(const rapidjson::Value& doc, Preset* out) const {
        assert(out != nullptr);

        bool parsed = true;

        parsed &= parseString(doc, FORMAT_VERSION_ID, FORMAT_VERSION_ID, &out->metadata.VERSION);
        parsed &= parseString(doc, FORMAT_MOD_ARCHIVE_ID, FORMAT_MOD_ARCHIVE_ID, &out->metadata.MOD_ARCHIVE);

        // Metadata
        parsed &= parseString(doc, PRESET_UUID_ID, PRESET_UUID_ID, &out->uuid);
        parsed &= parseString(doc, PRESET_BUNDLE_ID, PRESET_BUNDLE_ID, &out->bundle);
        parsed &= parseString(doc, PRESET_ARMOUR_NAME_ID, PRESET_ARMOUR_NAME_ID, &out->armour.name);
        parsed &= parseBool(doc, PRESET_ARMOUR_FEMALE_ID, PRESET_ARMOUR_FEMALE_ID, &out->armour.female);
        parsed &= parseBool(doc, PRESET_FEMALE_ID, PRESET_FEMALE_ID, &out->female);
        parsed &= parseBool(doc, PRESET_HIDE_SLINGER_ID, PRESET_HIDE_SLINGER_ID, &out->hideSlinger);
        parsed &= parseBool(doc, PRESET_HIDE_WEAPON_ID, PRESET_HIDE_WEAPON_ID, &out->hideWeapon);

        parsed &= parseObject(doc, PRESET_QUICK_MATERIAL_OVERRIDES_ID, PRESET_QUICK_MATERIAL_OVERRIDES_ID);
        if (!parsed) return false;

        parsed &= loadPresetQuickMaterialOverrides(doc[PRESET_QUICK_MATERIAL_OVERRIDES_ID], out);

        PresetPieceSettings set{};
        PresetPieceSettings helm{};
        PresetPieceSettings body{};
        PresetPieceSettings arms{};
        PresetPieceSettings coil{};
        PresetPieceSettings legs{};

        parsed &= parseObject(doc, PRESET_PIECE_SETTINGS_SET_ID, PRESET_PIECE_SETTINGS_SET_ID);
        parsed &= parseObject(doc, PRESET_PIECE_SETTINGS_HELM_ID, PRESET_PIECE_SETTINGS_HELM_ID);
        parsed &= parseObject(doc, PRESET_PIECE_SETTINGS_BODY_ID, PRESET_PIECE_SETTINGS_BODY_ID);
        parsed &= parseObject(doc, PRESET_PIECE_SETTINGS_ARMS_ID, PRESET_PIECE_SETTINGS_ARMS_ID);
        parsed &= parseObject(doc, PRESET_PIECE_SETTINGS_COIL_ID, PRESET_PIECE_SETTINGS_COIL_ID);
        parsed &= parseObject(doc, PRESET_PIECE_SETTINGS_LEGS_ID, PRESET_PIECE_SETTINGS_LEGS_ID);

        if (!parsed) return false;

        parsed &= loadPresetPieceSettings(doc[PRESET_PIECE_SETTINGS_SET_ID],  &set);
        parsed &= loadPresetPieceSettings(doc[PRESET_PIECE_SETTINGS_HELM_ID], &helm);
        parsed &= loadPresetPieceSettings(doc[PRESET_PIECE_SETTINGS_BODY_ID], &body);
        parsed &= loadPresetPieceSettings(doc[PRESET_PIECE_SETTINGS_ARMS_ID], &arms);
        parsed &= loadPresetPieceSettings(doc[PRESET_PIECE_SETTINGS_COIL_ID], &coil);
        parsed &= loadPresetPieceSettings(doc[PRESET_PIECE_SETTINGS_LEGS_ID], &legs);

        out->set = set;
        out->helm = helm;
        out->body = body;
        out->arms = arms;
        out->coil = coil;
        out->legs = legs;

        return parsed;
    }

    bool KBFDataManager::loadPresetQuickMaterialOverrides(const rapidjson::Value& object, Preset* out) const {
        assert(out != nullptr);

        bool parsed = true;

        for (const auto& qMatOverride : object.GetObject()) {
            std::string qMatOverrideKey = qMatOverride.name.GetString();

            if (!qMatOverride.value.IsObject()) {
                DEBUG_STACK.push(
                    std::format("{} Failed to parse quick material override \"{}\". Expected an object, but got a different type.",
                        KBF_DATA_MANAGER_LOG_TAG, qMatOverrideKey),
                    DebugStack::Color::COL_ERROR
                );
                parsed = false;
                continue;
            }

            const rapidjson::Value& overrideObj = qMatOverride.value; // << use this!

            uint64_t rawType = 0;
            parsed &= parseUint64(overrideObj, PRESET_QUICK_MATERIAL_OVERRIDE_TYPE_ID, PRESET_QUICK_MATERIAL_OVERRIDE_TYPE_ID, &rawType);
            if (!parsed) continue;

            MeshMaterialParamType type = static_cast<MeshMaterialParamType>(rawType);

            bool enabled = false;
            std::string matName;
            std::string paramName;

            parsed &= parseBool(overrideObj, PRESET_QUICK_MATERIAL_OVERRIDE_ENABLED_ID, PRESET_QUICK_MATERIAL_OVERRIDE_ENABLED_ID, &enabled);
            parsed &= parseString(overrideObj, PRESET_QUICK_MATERIAL_OVERRIDE_MATCHING_MATERIAL_NAME_ID, PRESET_QUICK_MATERIAL_OVERRIDE_MATCHING_MATERIAL_NAME_ID, &matName);
            parsed &= parseString(overrideObj, PRESET_QUICK_MATERIAL_OVERRIDE_PARAM_NAME_ID, PRESET_QUICK_MATERIAL_OVERRIDE_PARAM_NAME_ID, &paramName);

            switch (type) {
            case MeshMaterialParamType::MAT_TYPE_FLOAT: {
                QuickMaterialOverride<float> overrideVal = { enabled, matName, paramName, 0.0f };
                parsed &= parseFloat(overrideObj, PRESET_QUICK_MATERIAL_OVERRIDE_VALUE_ID, PRESET_QUICK_MATERIAL_OVERRIDE_VALUE_ID, &overrideVal.value);

                if (parsed) out->quickMaterialOverridesFloat[qMatOverrideKey] = overrideVal;
            } break;

            case MeshMaterialParamType::MAT_TYPE_FLOAT4: {
                QuickMaterialOverride<glm::vec4> overrideVal = { enabled, matName, paramName, glm::vec4{} };
                parsed &= parseVec4(overrideObj, PRESET_QUICK_MATERIAL_OVERRIDE_VALUE_ID, PRESET_QUICK_MATERIAL_OVERRIDE_VALUE_ID, &overrideVal.value);

                if (parsed) out->quickMaterialOverridesVec4[qMatOverrideKey] = overrideVal;
            } break;
            }
        }

        return parsed;
    }

    bool KBFDataManager::loadPresetPieceSettings(const rapidjson::Value& object, PresetPieceSettings* out) const {
        assert(out != nullptr);

        bool parsed = true;
        parsed &= parseFloat(object, PRESET_PIECE_SETTINGS_MOD_LIMIT_ID, PRESET_PIECE_SETTINGS_MOD_LIMIT_ID, &out->modLimit);
        parsed &= parseBool(object, PRESET_PIECE_SETTINGS_USE_SYMMETRY_ID, PRESET_PIECE_SETTINGS_USE_SYMMETRY_ID, &out->useSymmetry);

        parsed &= parseObject(object, PRESET_PIECE_SETTINGS_BONE_MODIFIERS_ID, PRESET_PIECE_SETTINGS_BONE_MODIFIERS_ID);
        if (parsed) {
            const rapidjson::Value& boneModifiers = object[PRESET_PIECE_SETTINGS_BONE_MODIFIERS_ID];
            parsed &= loadBoneModifiers(boneModifiers, &out->modifiers);
        }

        parsed &= parseObject(object, PRESET_PIECE_SETTINGS_REMOVED_PARTS_ID, PRESET_PIECE_SETTINGS_REMOVED_PARTS_ID);
        if (parsed) {
            const rapidjson::Value& removedParts = object[PRESET_PIECE_SETTINGS_REMOVED_PARTS_ID];
            std::vector<OverrideMeshPart> parts;
            parsed &= loadOverrideParts(removedParts, &parts);
            out->partOverrides = std::set<OverrideMeshPart>(parts.begin(), parts.end());
        }

        parsed &= parseObject(object, PRESET_OVERRIDE_MATERIALS_OVERRIDES_ID, PRESET_OVERRIDE_MATERIALS_OVERRIDES_ID);
        if (parsed) {
            const rapidjson::Value& matOverrides = object[PRESET_OVERRIDE_MATERIALS_OVERRIDES_ID];
            parsed &= loadOverrideMaterials(matOverrides, &out->materialOverrides);
        }

        return parsed;
    }

    bool KBFDataManager::loadOverrideParts(const rapidjson::Value& object, std::vector<OverrideMeshPart>* out) const {
        assert(out != nullptr);

        bool parsed = true;
        for (const auto& part : object.GetObject()) {
            OverrideMeshPart meshPart{};
            meshPart.part.name = part.name.GetString();
            parsed &= part.value.IsObject();
            if (parsed) {
                std::string typeStr;
                parsed &= parseUint64(part.value, PRESET_OVERRIDE_PARTS_INDEX_ID, meshPart.part.name + "." + PRESET_OVERRIDE_PARTS_INDEX_ID, &meshPart.part.index);

                bool hidden = false;
				parsed &= parseBool(part.value, PRESET_OVERRIDE_PARTS_HIDE_ID, meshPart.part.name + "." + PRESET_OVERRIDE_PARTS_HIDE_ID, &hidden);
                meshPart.shown = !hidden;
            }
            if (parsed) out->push_back(meshPart);
        }
        return parsed;
    }

    bool KBFDataManager::loadOverrideMaterials(
        const rapidjson::Value& matOverrides,
        std::set<OverrideMaterial>* out) const
    {
        assert(out != nullptr);
        bool parsed = true;

        for (auto it = matOverrides.MemberBegin(); it != matOverrides.MemberEnd(); ++it) {
            const std::string matName = it->name.GetString();
            const rapidjson::Value& matObject = it->value;

            if (!matObject.IsObject()) continue;

            OverrideMaterial mat;
            mat.material.name = matName;

            // Load "shown"
            parseBool(matObject, PRESET_OVERRIDE_MATERIALS_SHOW_ID, PRESET_OVERRIDE_MATERIALS_SHOW_ID, &mat.shown);

            // Load param overrides
            if (matObject.HasMember(PRESET_OVERRIDE_MATERIALS_PARAM_OVERRIDES_ID) &&
                matObject[PRESET_OVERRIDE_MATERIALS_PARAM_OVERRIDES_ID].IsObject()) {
                const rapidjson::Value& paramOverrides = matObject[PRESET_OVERRIDE_MATERIALS_PARAM_OVERRIDES_ID];
                for (auto pIt = paramOverrides.MemberBegin(); pIt != paramOverrides.MemberEnd(); ++pIt) {
                    const std::string paramName = pIt->name.GetString();
                    const rapidjson::Value& paramObject = pIt->value;

                    if (!paramObject.IsObject()) continue;

                    // Read type
                    MeshMaterialParamType type = MeshMaterialParamType::MAT_TYPE_FLOAT; // default
                    if (paramObject.HasMember(PRESET_OVERRIDE_MATERIALS_PARAM_OVERRIDE_DATA_TYPE_ID) &&
                        paramObject[PRESET_OVERRIDE_MATERIALS_PARAM_OVERRIDE_DATA_TYPE_ID].IsUint64()) {
                        type = static_cast<MeshMaterialParamType>(
                            paramObject[PRESET_OVERRIDE_MATERIALS_PARAM_OVERRIDE_DATA_TYPE_ID].GetUint64());
                    }

                    MaterialParamValue paramValue;
                    paramValue.type = type;

                    // Read value
                    if (paramObject.HasMember(PRESET_OVERRIDE_MATERIALS_PARAM_OVERRIDE_VALUE_ID)) {
                        const rapidjson::Value& val = paramObject[PRESET_OVERRIDE_MATERIALS_PARAM_OVERRIDE_VALUE_ID];
                        switch (type) {
                        case MeshMaterialParamType::MAT_TYPE_FLOAT:
                            if (val.IsNumber()) paramValue.value = static_cast<float>(val.GetDouble());
                            break;
                        case MeshMaterialParamType::MAT_TYPE_FLOAT4:
                            if (val.IsArray() && val.Size() == 4) {
                                paramValue.value = glm::vec4{
                                    val[0].GetFloat(),
                                    val[1].GetFloat(),
                                    val[2].GetFloat(),
                                    val[3].GetFloat()
                                };
                            }
                            break;
                        default:
                            paramValue.value = 0.0f;
                            break;
                        }
                    }

                    mat.paramOverrides[paramName] = paramValue;
                }
            }

            out->insert(std::move(mat));
        }

        return parsed;
    }

    bool KBFDataManager::loadBoneModifiers(const rapidjson::Value& object, BoneModifierMap* out) const {
        assert(out != nullptr);

        bool parsed = true;

        for (const auto& bone : object.GetObject()) {
            std::string boneName = bone.name.GetString();
            if (bone.value.IsObject()) {
                BoneModifier modifier{};

                parsed &= parseVec3(bone.value, PRESET_BONE_MODIFIERS_SCALE_ID, PRESET_BONE_MODIFIERS_SCALE_ID, &modifier.scale);
                parsed &= parseVec3(bone.value, PRESET_BONE_MODIFIERS_POSITION_ID, PRESET_BONE_MODIFIERS_POSITION_ID, &modifier.position);

                glm::vec3 rotation{};
                parsed &= parseVec3(bone.value, PRESET_BONE_MODIFIERS_ROTATION_ID, PRESET_BONE_MODIFIERS_ROTATION_ID, &rotation);
                modifier.setRotation(rotation);

                out->emplace(boneName, modifier);
            }
            else {
                DEBUG_STACK.push(std::format("{} Failed to parse bone modifier for bone {}. Expected an object, but got a different type.", KBF_DATA_MANAGER_LOG_TAG, boneName), DebugStack::Color::COL_ERROR);
            }
        }

        return parsed;
    }

    bool KBFDataManager::writePreset(const std::filesystem::path& path, const Preset& preset) const {
        rapidjson::StringBuffer s;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(s);

        writePresetJsonContent(preset, writer);

        bool success = writeJsonFile(path.string(), s.GetString());

        if (!success) {
            DEBUG_STACK.push(std::format("{} Failed to write preset {} ({}) to {}", KBF_DATA_MANAGER_LOG_TAG, preset.name, preset.uuid, path.string()), DebugStack::Color::COL_ERROR);
        }

        return success;
    }

    void KBFDataManager::writePresetJsonContent(
        const Preset& preset, 
        rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer
    ) const {
        writer.StartObject();
        writer.Key(FORMAT_VERSION_ID);
        writer.String(preset.metadata.VERSION.c_str());
        writer.Key(FORMAT_MOD_ARCHIVE_ID);
        writer.String(preset.metadata.MOD_ARCHIVE.c_str());
        writer.Key(PRESET_UUID_ID);
        writer.String(preset.uuid.c_str());
        writer.Key(PRESET_BUNDLE_ID);
        writer.String(preset.bundle.c_str());
        writer.Key(PRESET_ARMOUR_NAME_ID);
        writer.String(preset.armour.name.c_str());
        writer.Key(PRESET_ARMOUR_FEMALE_ID);
        writer.Bool(preset.armour.female);
        writer.Key(PRESET_FEMALE_ID);
        writer.Bool(preset.female);
        writer.Key(PRESET_HIDE_SLINGER_ID);
        writer.Bool(preset.hideSlinger);
        writer.Key(PRESET_HIDE_WEAPON_ID);
        writer.Bool(preset.hideWeapon);

        writer.Key(PRESET_QUICK_MATERIAL_OVERRIDES_ID);
        writer.StartObject();
        writePresetQuickMaterialOverrideContent(preset.quickMaterialOverridesFloat, writer);
        writePresetQuickMaterialOverrideContent(preset.quickMaterialOverridesVec4, writer);
        writer.EndObject();

        // TODO: Write Armour Pieces out
        for (ArmourPiece piece = ArmourPiece::AP_MIN; piece <= ArmourPiece::AP_MAX_EXCLUDING_SLINGER; piece = static_cast<ArmourPiece>(static_cast<int>(piece) + 1)) {
            switch (piece) {
            case ArmourPiece::AP_SET:  writer.Key(PRESET_PIECE_SETTINGS_SET_ID);  break;
            case ArmourPiece::AP_HELM: writer.Key(PRESET_PIECE_SETTINGS_HELM_ID); break;
            case ArmourPiece::AP_BODY: writer.Key(PRESET_PIECE_SETTINGS_BODY_ID); break;
            case ArmourPiece::AP_ARMS: writer.Key(PRESET_PIECE_SETTINGS_ARMS_ID); break;
            case ArmourPiece::AP_COIL: writer.Key(PRESET_PIECE_SETTINGS_COIL_ID); break;
            case ArmourPiece::AP_LEGS: writer.Key(PRESET_PIECE_SETTINGS_LEGS_ID); break;
            }

            PresetPieceSettings pieceSettings = preset.getPieceSettings(piece);
            writePresetPieceSettingsJsonContent(pieceSettings, writer);
        }

        writer.EndObject();
    }

    template<typename T>
    void KBFDataManager::writePresetQuickMaterialOverrideContent(
        const std::unordered_map<std::string, QuickMaterialOverride<T>>& quickOverrides,
        rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer
    ) const
    {
        auto always_false = []() { return false; };

        for (const auto& [key, override] : quickOverrides) {
            writer.Key(key.c_str());
            writer.StartObject();

            writer.Key(PRESET_QUICK_MATERIAL_OVERRIDE_ENABLED_ID);
            writer.Bool(override.enabled);


            writer.Key(PRESET_QUICK_MATERIAL_OVERRIDE_MATCHING_MATERIAL_NAME_ID);
            writer.String(override.materialName.c_str());

            writer.Key(PRESET_QUICK_MATERIAL_OVERRIDE_PARAM_NAME_ID);
            writer.String(override.paramName.c_str());

            if constexpr (std::is_same_v<T, float>) {
                writer.Key(PRESET_QUICK_MATERIAL_OVERRIDE_TYPE_ID);
                writer.Uint64(static_cast<uint64_t>(MeshMaterialParamType::MAT_TYPE_FLOAT));

                writer.Key(PRESET_QUICK_MATERIAL_OVERRIDE_VALUE_ID);
                writer.Double(override.value);
            }
            else if constexpr (std::is_same_v<T, glm::vec4>) {
                writer.Key(PRESET_QUICK_MATERIAL_OVERRIDE_TYPE_ID);
                writer.Uint64(static_cast<uint64_t>(MeshMaterialParamType::MAT_TYPE_FLOAT4));

                writer.Key(PRESET_QUICK_MATERIAL_OVERRIDE_VALUE_ID);
                writer.StartArray();
                writer.Double(override.value.x);
                writer.Double(override.value.y);
                writer.Double(override.value.z);
                writer.Double(override.value.w);
                writer.EndArray();
            }
            else {
                static_assert(always_false(), "Unsupported QuickMaterialOverride type");
            }

            writer.EndObject();
        }
    }

    void KBFDataManager::writePresetPieceSettingsJsonContent(
        const PresetPieceSettings& pieceSettings,
        rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer
    ) const {
        writer.StartObject();

        writer.Key(PRESET_PIECE_SETTINGS_MOD_LIMIT_ID);
        writer.Double(pieceSettings.modLimit);
        writer.Key(PRESET_PIECE_SETTINGS_USE_SYMMETRY_ID);
        writer.Bool(pieceSettings.useSymmetry);
        writer.Key(PRESET_PIECE_SETTINGS_BONE_MODIFIERS_ID);
        writer.StartObject();
        for (const auto& [boneName, modifier] : pieceSettings.modifiers) {
            writer.Key(boneName.c_str());

            rapidjson::StringBuffer compactBuf = getCompactBoneModifierJsonContent(modifier);
            writer.RawValue(compactBuf.GetString(), compactBuf.GetSize(), rapidjson::kObjectType);
        }
        writer.EndObject();
        writer.Key(PRESET_PIECE_SETTINGS_REMOVED_PARTS_ID);
        writer.StartObject();
        for (const OverrideMeshPart& partOverride : pieceSettings.partOverrides) {
            writer.Key(partOverride.part.name.c_str());

            rapidjson::StringBuffer compactBuf = getCompactOverridePartJsonContent(partOverride);
            writer.RawValue(compactBuf.GetString(), compactBuf.GetSize(), rapidjson::kObjectType);
        }
        writer.EndObject();

        writer.Key(PRESET_OVERRIDE_MATERIALS_OVERRIDES_ID);
        writer.StartObject();
        for (const OverrideMaterial& materialOverride : pieceSettings.materialOverrides) {
            writer.Key(materialOverride.material.name.c_str());

            writeOverrideMaterialJsonContent(materialOverride, writer);
        }
        writer.EndObject();

        writer.EndObject();
    }

    rapidjson::StringBuffer KBFDataManager::getCompactBoneModifierJsonContent(const BoneModifier& modifier) const {
        rapidjson::StringBuffer buf;
        rapidjson::Writer<rapidjson::StringBuffer> compactWriter(buf); // one-line writer

        compactWriter.StartObject();
        compactWriter.Key(PRESET_BONE_MODIFIERS_SCALE_ID);
        compactWriter.StartArray();
        compactWriter.Double(modifier.scale.x);
        compactWriter.Double(modifier.scale.y);
        compactWriter.Double(modifier.scale.z);
        compactWriter.EndArray();

        compactWriter.Key(PRESET_BONE_MODIFIERS_POSITION_ID);
        compactWriter.StartArray();
        compactWriter.Double(modifier.position.x);
        compactWriter.Double(modifier.position.y);
        compactWriter.Double(modifier.position.z);
        compactWriter.EndArray();

        compactWriter.Key(PRESET_BONE_MODIFIERS_ROTATION_ID);
        compactWriter.StartArray();
        const glm::vec3& rotation = modifier.getRotation();
        compactWriter.Double(rotation.x);
        compactWriter.Double(rotation.y);
        compactWriter.Double(rotation.z);
        compactWriter.EndArray();

        compactWriter.EndObject();

        return buf;
    }

    rapidjson::StringBuffer KBFDataManager::getCompactOverridePartJsonContent(const OverrideMeshPart& partOverride) const {
        rapidjson::StringBuffer buf;
        rapidjson::Writer<rapidjson::StringBuffer> compactWriter(buf); // one-line writer

        compactWriter.StartObject();
        compactWriter.Key(PRESET_OVERRIDE_PARTS_INDEX_ID);
        compactWriter.Uint64(partOverride.part.index);
        compactWriter.Key(PRESET_OVERRIDE_PARTS_HIDE_ID);
		compactWriter.Bool(!partOverride.shown);
        compactWriter.EndObject();
        return buf;
    }

    void KBFDataManager::writeOverrideMaterialJsonContent(
        const OverrideMaterial& mat,
        rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer
    ) const {
        writer.StartObject();
        writer.Key(PRESET_OVERRIDE_MATERIALS_SHOW_ID);
        writer.Bool(mat.shown);

        writer.Key(PRESET_OVERRIDE_MATERIALS_PARAM_OVERRIDES_ID);
        writer.StartObject();
        for (const auto& [paramName, paramOverride] : mat.paramOverrides) {
            writer.Key(paramName.c_str());
            rapidjson::StringBuffer compactBuf = getCompactParamOverrideJsonContent(paramOverride);
            writer.RawValue(compactBuf.GetString(), compactBuf.GetSize(), rapidjson::kObjectType);

        }
        writer.EndObject();

        writer.EndObject();
    }

    rapidjson::StringBuffer KBFDataManager::getCompactParamOverrideJsonContent(const MaterialParamValue& paramOverride) const {
        rapidjson::StringBuffer buf;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buf); // compact writer

        writer.StartObject();
        writer.Key(PRESET_OVERRIDE_MATERIALS_PARAM_OVERRIDE_DATA_TYPE_ID);
        writer.Uint64(static_cast<uint64_t>(paramOverride.type));

        writer.Key(PRESET_OVERRIDE_MATERIALS_PARAM_OVERRIDE_VALUE_ID);

        switch (paramOverride.type) {
        case MeshMaterialParamType::MAT_TYPE_FLOAT:
            writer.Double(paramOverride.asFloat());
            break;

        case MeshMaterialParamType::MAT_TYPE_FLOAT4: {
            const auto& v = paramOverride.asVec4();
            writer.StartArray();
            writer.Double(v.x);
            writer.Double(v.y);
            writer.Double(v.z);
            writer.Double(v.w);
            writer.EndArray();
            break;
        }

        default:
            writer.Double(0.0);
        }

        writer.EndObject();

        return buf;
    }

    bool KBFDataManager::loadPresets() {
        bool hasFailure = false;

        // Load all presets from the preset directory
        for (const auto& entry : std::filesystem::directory_iterator(presetPath)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {

                DEBUG_STACK.push(std::format("{} Loading preset from {}", KBF_DATA_MANAGER_LOG_TAG, entry.path().string()), DebugStack::Color::COL_INFO);

                Preset preset;
                if (loadPreset(entry.path(), &preset)) {
                    presets.emplace(preset.uuid, preset);
                    DEBUG_STACK.push(std::format("{} Loaded preset: {} ({})", KBF_DATA_MANAGER_LOG_TAG, preset.name, preset.uuid), DebugStack::Color::COL_SUCCESS);
                }
                else {
                    hasFailure = true;
                }
            }
        }
        return hasFailure;
    }

    bool KBFDataManager::loadFBSPreset(const std::filesystem::path& path, bool body, bool female, std::string bundle, FBSPreset* out) const {
        assert(out != nullptr);

        DEBUG_STACK.push(std::format("{} Loading FBS preset from {}", KBF_DATA_MANAGER_LOG_TAG, path.string()), DebugStack::Color::COL_INFO);

        rapidjson::Document presetDoc = loadConfigJson(KbfFileType::FBS_PRESET, path.string(), nullptr);
        if (!presetDoc.IsObject() || presetDoc.HasParseError()) return false;

        out->preset.name = std::format("{} ({})", path.stem().string(), body ? "body" : "legs");

        bool parsed = true;

        uint32_t fbsArmourID = 0;

        std::string autoSwitchID = body ? FBS_AUTO_SWITCH_BODY_ID : FBS_AUTO_SWITCH_LEGS_ID;
        std::string armourID     = body ? FBS_ARMOUR_ID_BODY_ID : FBS_ARMOUR_ID_LEGS_ID;
        parsed &= parseBool(presetDoc, autoSwitchID, autoSwitchID, &out->autoswitchingEnabled);
        parsed &= parseUint(presetDoc, armourID, armourID, &fbsArmourID);

        // Deal with lua indexing at 1 (why lua... why...?)
        if (fbsArmourID == 0) return false; // No armour at index 0
        fbsArmourID--;

        parsed &= fbsArmourExists(fbsArmourID, body);

        if (!parsed) return false;

        // Get all the modifiers (this is a horrible file read)
        BoneModifierMap& baseArmatureModifiers = out->preset.set.modifiers; 
        BoneModifierMap& presetSpecificModifiers = body ? out->preset.body.modifiers : out->preset.legs.modifiers;
        const auto addModifierIfNotExistsFn = [&presetSpecificModifiers, &baseArmatureModifiers](const std::string& boneName) {
            BoneModifierMap& targetMap = boneName.starts_with("CustomBone") ? presetSpecificModifiers : baseArmatureModifiers;
            if (targetMap.find(boneName) == targetMap.end()) targetMap.emplace(boneName, BoneModifier{});
        };

        std::unordered_map<std::string, glm::vec3> rotations{};

        for (const auto& key : presetDoc.GetObject()) {
            std::string keyName = key.name.GetString();
            std::string boneName;

            // The key will be a modifier if it ends in "EulervecX/Y/Z", "PosvecX/Y/Z", or "ScalevecX/Y/Z" (There is a Eularvec but not used... what the fuck?)
            bool isEulerX = keyName.ends_with("EulervecX");
            bool isEulerY = keyName.ends_with("EulervecY");
            bool isEulerZ = keyName.ends_with("EulervecZ");
            bool isPosX   = keyName.ends_with("PosvecX");
            bool isPosY   = keyName.ends_with("PosvecY");
            bool isPosZ   = keyName.ends_with("PosvecZ");
            bool isScaleX = keyName.ends_with("ScalevecX");
            bool isScaleY = keyName.ends_with("ScalevecY");
            bool isScaleZ = keyName.ends_with("ScalevecZ");

            bool isModifier = isEulerX || isEulerY || isEulerZ || isPosX || isPosY || isPosZ || isScaleX || isScaleY || isScaleZ;
            if (!isModifier) continue;

            if      (isEulerX) boneName = keyName.substr(0, keyName.size() - std::string("EulervecX").size());
            else if (isEulerY) boneName = keyName.substr(0, keyName.size() - std::string("EulervecY").size());
            else if (isEulerZ) boneName = keyName.substr(0, keyName.size() - std::string("EulervecZ").size());
            else if (isPosX)   boneName = keyName.substr(0, keyName.size() - std::string("PosvecX").size());
            else if (isPosY)   boneName = keyName.substr(0, keyName.size() - std::string("PosvecY").size());
            else if (isPosZ)   boneName = keyName.substr(0, keyName.size() - std::string("PosvecZ").size());
            else if (isScaleX) boneName = keyName.substr(0, keyName.size() - std::string("ScalevecX").size());
            else if (isScaleY) boneName = keyName.substr(0, keyName.size() - std::string("ScalevecY").size());
            else if (isScaleZ) boneName = keyName.substr(0, keyName.size() - std::string("ScalevecZ").size());

            BoneModifierMap& targetMap = boneName.starts_with("CustomBone") ? presetSpecificModifiers : baseArmatureModifiers;

            float modValue = 0.0f;
            parsed &= parseFloat(presetDoc, keyName.c_str(), keyName.c_str(), &modValue);

            if      (isEulerX) { addModifierIfNotExistsFn(boneName); rotations[boneName].x = modValue; }
            else if (isEulerY) { addModifierIfNotExistsFn(boneName); rotations[boneName].y = modValue; }
            else if (isEulerZ) { addModifierIfNotExistsFn(boneName); rotations[boneName].z = modValue; }
            else if (isPosX)   { addModifierIfNotExistsFn(boneName); targetMap[boneName].position.x = modValue; }
            else if (isPosY)   { addModifierIfNotExistsFn(boneName); targetMap[boneName].position.y = modValue; }
            else if (isPosZ)   { addModifierIfNotExistsFn(boneName); targetMap[boneName].position.z = modValue; }
            else if (isScaleX) { addModifierIfNotExistsFn(boneName); targetMap[boneName].scale.x = modValue; }
            else if (isScaleY) { addModifierIfNotExistsFn(boneName); targetMap[boneName].scale.y = modValue; }
            else if (isScaleZ) { addModifierIfNotExistsFn(boneName); targetMap[boneName].scale.z = modValue; }
        }

        for (const auto& [boneName, rotation] : rotations) {
			BoneModifierMap& targetMap = boneName.starts_with("CustomBone") ? presetSpecificModifiers : baseArmatureModifiers;
            targetMap[boneName].setRotation(rotation);
        }

        // add a placeholder for whichever part this is *supposed* to be for (But FBS doesn't support)
        if (body) {
            out->preset.body.modifiers.emplace("Auto-Generated FBS Compatability Part Marker (DISPLAY ONLY)", BoneModifier{});
        }
        else {
            out->preset.legs.modifiers.emplace("Auto-Generated FBS Compatability Part Marker (DISPLAY ONLY)", BoneModifier{});
        }

        // Do a pass over to enforce symmetry as FBS does not automatically populate symmetrical bones (though they might exist)
        for (auto& [boneName, modifier] : baseArmatureModifiers) {
            std::string complement = "";
            const auto _ = getSymmetryProxyModifier(boneName, baseArmatureModifiers, &complement);
            if (!complement.empty()) {
                baseArmatureModifiers[complement] = modifier.reflect();
			}
		}

        if (parsed) {
            // fill in kbf preset data
            out->preset.uuid = uuid::v4::UUID::New().String();
            out->preset.bundle = bundle;
            out->preset.female = female;

            out->preset.set.modLimit = 2.50f;
            out->preset.set.useSymmetry = true;

            out->preset.armour = getArmourSetFromFBSidx(fbsArmourID, body);
        }

        return parsed;

    }

    bool KBFDataManager::loadPresetGroup(const std::filesystem::path& path, PresetGroup* out) {
        assert(out != nullptr);

        rapidjson::Document presetGroupDoc = loadConfigJson(KbfFileType::PRESET_GROUP, path.string(), nullptr);
        if (!presetGroupDoc.IsObject() || presetGroupDoc.HasParseError()) return false;

        out->name = path.stem().string();

        bool parsed = loadPresetGroupData(presetGroupDoc, out);

        if (!parsed) {
            DEBUG_STACK.push(std::format("{} Failed to parse preset group {}. One or more required values were missing. Please rectify or remove the file.", KBF_DATA_MANAGER_LOG_TAG, path.string()), DebugStack::Color::COL_ERROR);
        }

        return parsed;
    }

    bool KBFDataManager::loadPresetGroupData(const rapidjson::Value& doc, PresetGroup* out) const {
        bool parsed = true;
        parsed &= parseString(doc, FORMAT_VERSION_ID, FORMAT_VERSION_ID, &out->metadata.VERSION);
        parsed &= parseString(doc, FORMAT_MOD_ARCHIVE_ID, FORMAT_MOD_ARCHIVE_ID, &out->metadata.MOD_ARCHIVE);

        parsed &= parseString(doc, PRESET_GROUP_UUID_ID, PRESET_GROUP_UUID_ID, &out->uuid);
        parsed &= parseBool(doc, PRESET_GROUP_FEMALE_ID, PRESET_GROUP_FEMALE_ID, &out->female);

        const auto parseAssignedPresetsObj = [this](
            const rapidjson::Value& doc,
            const char* id,
            std::unordered_map<ArmourSet, std::string>* out
        ) -> bool {
            bool parsedObj = true;
            parsedObj &= parseObject(doc, id, id);
            if (parsedObj) {
                const rapidjson::Value& assignedPresets = doc[id];
                parsedObj &= loadAssignedPresets(assignedPresets, out);
            }

            return parsedObj;
        };

        parsed &= parseAssignedPresetsObj(doc, PRESET_GROUP_SET_PRESETS_ID,  &out->setPresets);
        parsed &= parseAssignedPresetsObj(doc, PRESET_GROUP_HELM_PRESETS_ID, &out->helmPresets);
        parsed &= parseAssignedPresetsObj(doc, PRESET_GROUP_BODY_PRESETS_ID, &out->bodyPresets);
        parsed &= parseAssignedPresetsObj(doc, PRESET_GROUP_ARMS_PRESETS_ID, &out->armsPresets);
        parsed &= parseAssignedPresetsObj(doc, PRESET_GROUP_COIL_PRESETS_ID, &out->coilPresets);
        parsed &= parseAssignedPresetsObj(doc, PRESET_GROUP_LEGS_PRESETS_ID, &out->legsPresets);

        return parsed;
    }

    bool KBFDataManager::loadAssignedPresets(const rapidjson::Value& object, std::unordered_map<ArmourSet, std::string>* out) const {
        assert(out != nullptr);

        bool parsed = true;

        for (const auto& assignedPreset : object.GetObject()) {
            if (assignedPreset.value.IsObject()) {
                ArmourSet armourSet{};
                std::string presetUUID;

                parsed &= parseString(assignedPreset.value, PRESET_GROUP_PRESETS_ARMOUR_NAME_ID, PRESET_GROUP_PRESETS_ARMOUR_NAME_ID, &armourSet.name);
                parsed &= parseBool(assignedPreset.value, PRESET_GROUP_PRESETS_FEMALE_ID, PRESET_GROUP_PRESETS_FEMALE_ID, &armourSet.female);
                parsed &= parseString(assignedPreset.value, PRESET_GROUP_PRESETS_UUID_ID, PRESET_GROUP_PRESETS_UUID_ID, &presetUUID);

                out->emplace(armourSet, presetUUID);
            }
            else {
                DEBUG_STACK.push(std::format("{} Failed to parse assigned preset. Expected an object, but got a different type.", KBF_DATA_MANAGER_LOG_TAG), DebugStack::Color::COL_ERROR);
            }
        }

        return parsed;
    }

    bool KBFDataManager::writePresetGroup(const std::filesystem::path& path, const PresetGroup& presetGroup) const {
        rapidjson::StringBuffer s;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(s);

        writePresetGroupJsonContent(presetGroup, writer);

        bool success = writeJsonFile(path.string(), s.GetString());

        if (!success) {
            DEBUG_STACK.push(std::format("{} Failed to write preset {} ({}) to {}", KBF_DATA_MANAGER_LOG_TAG, presetGroup.name, presetGroup.uuid, path.string()), DebugStack::Color::COL_ERROR);
        }

        return success;
    }

    void KBFDataManager::writePresetGroupJsonContent(
        const PresetGroup& presetGroup, 
        rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer
    ) const {
        writer.StartObject();
        writer.Key(FORMAT_VERSION_ID);
        writer.String(presetGroup.metadata.VERSION.c_str());
        writer.Key(FORMAT_MOD_ARCHIVE_ID);
        writer.String(presetGroup.metadata.MOD_ARCHIVE.c_str());
        writer.Key(PRESET_GROUP_UUID_ID);
        writer.String(presetGroup.uuid.c_str());
        writer.Key(PRESET_GROUP_FEMALE_ID);
        writer.Bool(presetGroup.female);

        writePresetGroupAssignedPresets(PRESET_GROUP_SET_PRESETS_ID,  presetGroup.setPresets,  writer);
        writePresetGroupAssignedPresets(PRESET_GROUP_HELM_PRESETS_ID, presetGroup.helmPresets, writer);
        writePresetGroupAssignedPresets(PRESET_GROUP_BODY_PRESETS_ID, presetGroup.bodyPresets, writer);
        writePresetGroupAssignedPresets(PRESET_GROUP_ARMS_PRESETS_ID, presetGroup.armsPresets, writer);
        writePresetGroupAssignedPresets(PRESET_GROUP_COIL_PRESETS_ID, presetGroup.coilPresets, writer);
        writePresetGroupAssignedPresets(PRESET_GROUP_LEGS_PRESETS_ID, presetGroup.legsPresets, writer);

        writer.EndObject();
    }

    void KBFDataManager::writePresetGroupAssignedPresets(
        std::string id,
        const std::unordered_map<ArmourSet, std::string>& presets,
        rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer
    ) const {
        constexpr auto writeCompactAssignedPreset = [](const ArmourSet& armourSet, std::string presetUUID) {
            rapidjson::StringBuffer buf;
            rapidjson::Writer<rapidjson::StringBuffer> compactWriter(buf); // one-line writer

            compactWriter.StartObject();
            compactWriter.Key(PRESET_GROUP_PRESETS_ARMOUR_NAME_ID);
            compactWriter.String(armourSet.name.c_str());
            compactWriter.Key(PRESET_GROUP_PRESETS_FEMALE_ID);
            compactWriter.Bool(armourSet.female);
            compactWriter.Key(PRESET_GROUP_PRESETS_UUID_ID);
            compactWriter.String(presetUUID.c_str());
            compactWriter.EndObject();

            return buf;
        };

        writer.Key(id.c_str());
        writer.StartObject();
        size_t i = 0;
        for (const auto& [armourSet, presetUUID] : presets) {
            writer.Key(std::to_string(i).c_str());

            rapidjson::StringBuffer compactBuf = writeCompactAssignedPreset(armourSet, presetUUID);
            writer.RawValue(compactBuf.GetString(), compactBuf.GetSize(), rapidjson::kObjectType);
            i++;
        }
        writer.EndObject();
    }

    bool KBFDataManager::loadPresetGroups() {
        bool hasFailure = false;

        // Load all preset groups from the preset directory
        for (const auto& entry : std::filesystem::directory_iterator(presetGroupPath)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {

                DEBUG_STACK.push(std::format("{} Loading preset group from {}", KBF_DATA_MANAGER_LOG_TAG, entry.path().string()), DebugStack::Color::COL_INFO);

                PresetGroup presetGroup;
                if (loadPresetGroup(entry.path(), &presetGroup)) {
                    presetGroups.emplace(presetGroup.uuid, presetGroup);
                    DEBUG_STACK.push(std::format("{} Loaded preset group: {} ({})", KBF_DATA_MANAGER_LOG_TAG, presetGroup.name, presetGroup.uuid), DebugStack::Color::COL_SUCCESS);
                }
                else {
                    hasFailure = true;
                }
            }
        }
        return hasFailure;
    }

    bool KBFDataManager::loadPlayerOverride(const std::filesystem::path& path, PlayerOverride* out) {
        assert(out != nullptr);

        std::string utf8_path = cvt_utf16_to_utf8(path.wstring());

        rapidjson::Document overrideDoc = loadConfigJson(KbfFileType::PLAYER_OVERRIDE, utf8_path, nullptr);
        if (!overrideDoc.IsObject() || overrideDoc.HasParseError()) return false;

        bool parsed = loadPlayerOverrideData(overrideDoc, out);

        if (!parsed) {
            DEBUG_STACK.push(std::format("{} Failed to parse player override {}. One or more required values were missing. Please rectify or remove the file.", KBF_DATA_MANAGER_LOG_TAG, utf8_path), DebugStack::Color::COL_ERROR);
        }

        return parsed;
    }

    bool KBFDataManager::loadPlayerOverrideData(const rapidjson::Value& doc, PlayerOverride* out) const {
        assert(out != nullptr);

        bool parsed = true;
        parsed &= parseString(doc, FORMAT_VERSION_ID, FORMAT_VERSION_ID, &out->metadata.VERSION);
        parsed &= parseString(doc, FORMAT_MOD_ARCHIVE_ID, FORMAT_MOD_ARCHIVE_ID, &out->metadata.MOD_ARCHIVE);

        parsed &= parseString(doc, PLAYER_OVERRIDE_PLAYER_NAME_ID, PLAYER_OVERRIDE_PLAYER_NAME_ID, &out->player.name);
        parsed &= parseString(doc, PLAYER_OVERRIDE_PLAYER_HUNTER_ID_ID, PLAYER_OVERRIDE_PLAYER_HUNTER_ID_ID, &out->player.hunterId);
        parsed &= parseBool(doc, PLAYER_OVERRIDE_PLAYER_FEMALE_ID, PRESET_GROUP_FEMALE_ID, &out->player.female);
        parsed &= parseString(doc, PLAYER_OVERRIDE_PRESET_GROUP_UUID_ID, PLAYER_OVERRIDE_PRESET_GROUP_UUID_ID, &out->presetGroup);

        return parsed;
    }

    bool KBFDataManager::writePlayerOverride(const std::filesystem::path& path, const PlayerOverride& playerOverride) const {
        rapidjson::StringBuffer s;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(s);

        writePlayerOverrideJsonContent(playerOverride, writer);

        bool success = writeJsonFile(path.string(), s.GetString());

        if (!success) {
            std::string utf8_path = cvt_utf16_to_utf8(path.wstring());
            DEBUG_STACK.push(std::format("{} Failed to write player override for player {} to {}", KBF_DATA_MANAGER_LOG_TAG, playerOverride.player.string(), utf8_path), DebugStack::Color::COL_ERROR);
        }

        return success;
    }

    void KBFDataManager::writePlayerOverrideJsonContent(
        const PlayerOverride& playerOverride, 
        rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer
    ) const {
        writer.StartObject();
        writer.Key(FORMAT_VERSION_ID);
        writer.String(playerOverride.metadata.VERSION.c_str());
        writer.Key(FORMAT_MOD_ARCHIVE_ID);
        writer.String(playerOverride.metadata.MOD_ARCHIVE.c_str());
        writer.Key(PLAYER_OVERRIDE_PLAYER_NAME_ID);
        writer.String(playerOverride.player.name.c_str());
        writer.Key(PLAYER_OVERRIDE_PLAYER_HUNTER_ID_ID);
        writer.String(playerOverride.player.hunterId.c_str());
        writer.Key(PLAYER_OVERRIDE_PLAYER_FEMALE_ID);
        writer.Bool(playerOverride.player.female);
        writer.Key(PLAYER_OVERRIDE_PRESET_GROUP_UUID_ID);
        writer.String(playerOverride.presetGroup.c_str());
        writer.EndObject();
    }

    bool KBFDataManager::loadPlayerOverrides() {
        bool hasFailure = false;

        // Load all player overrides from the preset directory
        for (const auto& entry : std::filesystem::directory_iterator(playerOverridePath)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {

                std::string utf8_path = cvt_utf16_to_utf8(entry.path().wstring());
                DEBUG_STACK.push(std::format("{} Loading player override from {}", KBF_DATA_MANAGER_LOG_TAG, utf8_path), DebugStack::Color::COL_INFO);

                PlayerOverride playerOverride;
                if (loadPlayerOverride(entry.path(), &playerOverride)) {
                    playerOverrides.emplace(playerOverride.player, playerOverride);
                    DEBUG_STACK.push(std::format("{} Loaded player override: {}", KBF_DATA_MANAGER_LOG_TAG, playerOverride.player.string()), DebugStack::Color::COL_SUCCESS);
                }
                else {
                    hasFailure = true;
                }
            }
        }
        return hasFailure;
    }

    std::string KBFDataManager::getPlayerOverrideFilename(const PlayerData& player) const {
        return AnsiPercentEncode(player.name) + "-" + (player.female ? "Female" : "Male") + "-" + player.hunterId;
    }

    void KBFDataManager::validateObjectsUsingPresetGroups() {
        validatePlayerOverrides();
        validateDefaultConfigs_PresetGroups();
    }

    void KBFDataManager::validatePlayerOverrides() {
        DEBUG_STACK.push(std::format("{} Validating Player Overrides...", KBF_DATA_MANAGER_LOG_TAG), DebugStack::Color::COL_DEBUG);

        std::vector<PlayerData> overridesToUpdate{};

        for (auto& [player, playerOverride] : playerOverrides) {
            if (!playerOverride.presetGroup.empty() && getPresetGroupByUUID(playerOverride.presetGroup) == nullptr) {
                overridesToUpdate.push_back(player);
            }
        }

        for (PlayerData& player : overridesToUpdate) {
            PlayerOverride& playerOverride = playerOverrides.at(player);

            DEBUG_STACK.push(std::format("{} Player override for {} has an invalid preset group ({}) - it may have been deleted. Reverting to default...",
                KBF_DATA_MANAGER_LOG_TAG,
                player.string(),
                playerOverride.presetGroup
            ), DebugStack::Color::COL_WARNING);

            PlayerOverride newPlayerOverride = playerOverride;
            newPlayerOverride.presetGroup = "";

            updatePlayerOverride(player, newPlayerOverride);
        }
    }

    void KBFDataManager::validateDefaultConfigs_PresetGroups() {
        // Players
        DEBUG_STACK.push(std::format("{} Validating Player Defaults...", KBF_DATA_MANAGER_LOG_TAG), DebugStack::Color::COL_DEBUG);
        std::unordered_set<std::string> badUUIDs;

        PlayerDefaults& player = presetGroupDefaults.player;
        const std::string playerMaleBefore   = player.male;
        const std::string playerFemaleBefore = player.female;
        if (!validatePresetGroupExists(player.male))   badUUIDs.insert(playerMaleBefore);
        if (!validatePresetGroupExists(player.female)) badUUIDs.insert(playerFemaleBefore);

        if (badUUIDs.size() > 0) {
            std::string errStr = "Player defaults had invalid preset group(s):\n";
            for (const auto& id : badUUIDs) {
                errStr += "   - " + id + "\n";
            }
            errStr += "   Which may have been deleted. Reverting to default...";
            DEBUG_STACK.push(std::format("{} {}", KBF_DATA_MANAGER_LOG_TAG, errStr), DebugStack::Color::COL_WARNING);
            writePlayerConfig(player);
        }

        // NPCs
        badUUIDs.clear();
        DEBUG_STACK.push(std::format("{} Validating NPC Defaults...", KBF_DATA_MANAGER_LOG_TAG), DebugStack::Color::COL_DEBUG);

        NpcDefaults& npc = presetGroupDefaults.npc;
        const std::string npcMaleBefore   = npc.male;
        const std::string npcFemaleBefore = npc.female;
        if (!validatePresetGroupExists(npc.male))   badUUIDs.insert(npcMaleBefore);
        if (!validatePresetGroupExists(npc.female)) badUUIDs.insert(npcFemaleBefore);

        if (badUUIDs.size() > 0) {
            std::string errStr = "NPC defaults had invalid preset group(s):\n";
            for (const auto& id : badUUIDs) {
                errStr += "   - " + id + "\n";
            }
            errStr += "   Which may have been deleted. Reverting to default...";
            DEBUG_STACK.push(std::format("{} {}", KBF_DATA_MANAGER_LOG_TAG, errStr), DebugStack::Color::COL_WARNING);
            writeNpcConfig(npc);
        }
    }

    bool KBFDataManager::validatePresetGroupExists(std::string& uuid) const {
        if (!uuid.empty() && getPresetGroupByUUID(uuid) == nullptr) {
            uuid = "";
            return false;
        }
        return true;
    }


    void KBFDataManager::validateObjectsUsingPresets() {
        validatePresetGroups();
        validateDefaultConfigs_Presets();
    }

    void KBFDataManager::validatePresetGroups() {
        DEBUG_STACK.push(std::format("{} Validating Preset Groups...", KBF_DATA_MANAGER_LOG_TAG), DebugStack::Color::COL_DEBUG);
        for (auto& [uuid, presetGroup] : presetGroups) {
            PresetGroup* newPresetGroup = nullptr;
            size_t defaultCount = 0;
            size_t invalidCount = 1;

            // Body
            for (auto& [armourSet, presetUUID] : presetGroup.bodyPresets) {
                bool isDefault = presetUUID.empty();
                bool isInvalid = getPresetByUUID(presetUUID) == nullptr;

                if (isDefault || isInvalid) {
                    if (!newPresetGroup) {
                        newPresetGroup = new PresetGroup{ presetGroup };
                    }

                    // remove the entry
                    newPresetGroup->bodyPresets.erase(armourSet);

                    if      (isDefault) defaultCount++;
                    else if (isInvalid) invalidCount++;
                }
            }

            // Legs
            for (auto& [armourSet, presetUUID] : presetGroup.legsPresets) {
                bool isDefault = presetUUID.empty();
                bool isInvalid = getPresetByUUID(presetUUID) == nullptr;

                if (isDefault || isInvalid) {
                    if (!newPresetGroup) {
                        newPresetGroup = new PresetGroup{ presetGroup };
                    }

                    // remove the entry
                    newPresetGroup->legsPresets.erase(armourSet);

                    if (isDefault) defaultCount++;
                    else if (isInvalid) invalidCount++;
                }
            }

            if (newPresetGroup) {
                DEBUG_STACK.push(std::format("{} Validated Preset Group {} ({}): Cleaned {} defaults and reverted {} invalid presets to default.", 
                    KBF_DATA_MANAGER_LOG_TAG,
                    presetGroup.name,
                    presetGroup.uuid,
                    defaultCount,
                    invalidCount
                ), DebugStack::Color::COL_WARNING);

                updatePresetGroup(presetGroup.uuid, *newPresetGroup);
                delete newPresetGroup;
            }
        }
    }

    void KBFDataManager::validateDefaultConfigs_Presets() {
        // Alma
        DEBUG_STACK.push(std::format("{} Validating Alma Config...", KBF_DATA_MANAGER_LOG_TAG), DebugStack::Color::COL_DEBUG);
        std::unordered_set<std::string> badUUIDs;

        {
            AlmaDefaults& alma = presetDefaults.alma;
            const std::string handlersOutfitBefore      = alma.handlersOutfit;
            const std::string newWorldCommissionBefore  = alma.newWorldCommission;
            const std::string scrivenersCoatBefore      = alma.scrivenersCoat;
            const std::string springBlossomKimonoBefore = alma.springBlossomKimono;
            const std::string chunLiOutfitBefore        = alma.chunLiOutfit;
            const std::string cammyOutfitBefore         = alma.cammyOutfit;
            const std::string summerPonchoBefore        = alma.summerPoncho;
            if (!validatePresetExists(alma.handlersOutfit))      badUUIDs.insert(handlersOutfitBefore     );
            if (!validatePresetExists(alma.newWorldCommission))  badUUIDs.insert(newWorldCommissionBefore );
            if (!validatePresetExists(alma.scrivenersCoat))      badUUIDs.insert(scrivenersCoatBefore     );
            if (!validatePresetExists(alma.springBlossomKimono)) badUUIDs.insert(springBlossomKimonoBefore);
            if (!validatePresetExists(alma.chunLiOutfit))        badUUIDs.insert(chunLiOutfitBefore       );
            if (!validatePresetExists(alma.cammyOutfit))         badUUIDs.insert(cammyOutfitBefore        );
            if (!validatePresetExists(alma.summerPoncho))        badUUIDs.insert(summerPonchoBefore       );

            if (badUUIDs.size() > 0) {
                std::string errStr = "Alma config had invalid preset group(s):\n";
                for (const auto& id : badUUIDs) {
                    errStr += "   - " + id + "\n";
                }
                errStr += "   Which may have been deleted. Reverting to default...";
                DEBUG_STACK.push(std::format("{} {}", KBF_DATA_MANAGER_LOG_TAG, errStr), DebugStack::Color::COL_WARNING);
                writeAlmaConfig(alma);
            }
        }

        // Gemma
        {
            badUUIDs.clear();
            DEBUG_STACK.push(std::format("{} Validating Gemma Config...", KBF_DATA_MANAGER_LOG_TAG), DebugStack::Color::COL_DEBUG);

            GemmaDefaults& gemma = presetDefaults.gemma;
            const std::string smithysOutfitBefore   = gemma.smithysOutfit;
            const std::string summerCoverallsBefore = gemma.summerCoveralls;
            if (!validatePresetExists(gemma.smithysOutfit))   badUUIDs.insert(smithysOutfitBefore  );
            if (!validatePresetExists(gemma.summerCoveralls)) badUUIDs.insert(summerCoverallsBefore);

            if (badUUIDs.size() > 0) {
                std::string errStr = "Gemma config had invalid preset group(s):\n";
                for (const auto& id : badUUIDs) {
                    errStr += "   - " + id + "\n";
                }
                errStr += "   Which may have been deleted. Reverting to default...";
                DEBUG_STACK.push(std::format("{} {}", KBF_DATA_MANAGER_LOG_TAG, errStr), DebugStack::Color::COL_WARNING);
                writeGemmaConfig(gemma);
            }
        }

        // Erik
        {
            badUUIDs.clear();
            DEBUG_STACK.push(std::format("{} Validating Erik Config...", KBF_DATA_MANAGER_LOG_TAG), DebugStack::Color::COL_DEBUG);

            ErikDefaults& erik = presetDefaults.erik;
            const std::string handlersOutfitBefore = erik.handlersOutfit;
            const std::string summerHatBefore      = erik.summerHat;
            if (!validatePresetExists(erik.handlersOutfit))   badUUIDs.insert(handlersOutfitBefore);
            if (!validatePresetExists(erik.summerHat))        badUUIDs.insert(summerHatBefore     );

            if (badUUIDs.size() > 0) {
                std::string errStr = "Erik config had invalid preset group(s):\n";
                for (const auto& id : badUUIDs) {
                    errStr += "   - " + id + "\n";
                }
                errStr += "   Which may have been deleted. Reverting to default...";
                DEBUG_STACK.push(std::format("{} {}", KBF_DATA_MANAGER_LOG_TAG, errStr), DebugStack::Color::COL_WARNING);
                writeErikConfig(erik);
            }
        }
    }

    bool KBFDataManager::validatePresetExists(std::string& uuid) const {
        if (!uuid.empty() && getPresetByUUID(uuid) == nullptr) {
            uuid = "";
            return false;
        }
        return true;
    }

    bool KBFDataManager::loadArmourList(const std::filesystem::path& path, ArmourMapping* out) {
        assert(out != nullptr);

        DEBUG_STACK.push(std::format("{} Loading armour list from \"{}\"", KBF_DATA_MANAGER_LOG_TAG, path.string()), DebugStack::Color::COL_INFO);

        rapidjson::Document armourListDoc = loadConfigJson(KbfFileType::ARMOUR_LIST, path.string(), nullptr);
        if (!armourListDoc.IsObject() || armourListDoc.HasParseError()) {
            DEBUG_STACK.push(std::format("{} Failed to parse Armour List at \"{}\". Using internal fallback...", KBF_DATA_MANAGER_LOG_TAG, path.string()), DebugStack::Color::COL_ERROR);
            return false;
        }

        bool parsed = loadArmourListData(armourListDoc, out);

        if (!parsed) {
            DEBUG_STACK.push(std::format("{} Failed to parse Armour List at \"{}\". Using internal fallback...", KBF_DATA_MANAGER_LOG_TAG, path.string()), DebugStack::Color::COL_ERROR);
            *out = ArmourList::FALLBACK_MAPPING;
        }
        else {
            DEBUG_STACK.push(std::format("{} Successfully loaded Armour List from \"{}\"", KBF_DATA_MANAGER_LOG_TAG, path.string()), DebugStack::Color::COL_SUCCESS);
        }

        return parsed;
    }

    bool KBFDataManager::loadArmourListData(const rapidjson::Value& doc, ArmourMapping* out) const {
        assert(out != nullptr);

        bool parsed = true;

        for (const auto& armourSet : doc.GetObject()) {
            std::string name = armourSet.name.GetString();
            if (armourSet.value.IsObject()) {

                bool hasEntry = false;
                if (armourSet.value.HasMember(ARMOUR_LIST_FEMALE_ID) && armourSet.value[ARMOUR_LIST_FEMALE_ID].IsObject()) {
                    const auto& femaleArmour = armourSet.value[ARMOUR_LIST_FEMALE_ID].GetObject();
                    ArmourSet set{ name, true };
                    ArmourID id;

                    bool hasAny = false;
                    if (femaleArmour.HasMember(ARMOUR_LIST_HELM_ID) && femaleArmour[ARMOUR_LIST_HELM_ID].IsString()) {
                        id.helm = femaleArmour[ARMOUR_LIST_HELM_ID].GetString();
                        hasAny = true;
                    }
                    if (femaleArmour.HasMember(ARMOUR_LIST_BODY_ID) && femaleArmour[ARMOUR_LIST_BODY_ID].IsString()) {
                        id.body = femaleArmour[ARMOUR_LIST_BODY_ID].GetString();
                        hasAny = true;
                    }
                    if (femaleArmour.HasMember(ARMOUR_LIST_ARMS_ID) && femaleArmour[ARMOUR_LIST_ARMS_ID].IsString()) {
                        id.arms = femaleArmour[ARMOUR_LIST_ARMS_ID].GetString();
                        hasAny = true;
                    }
                    if (femaleArmour.HasMember(ARMOUR_LIST_COIL_ID) && femaleArmour[ARMOUR_LIST_COIL_ID].IsString()) {
                        id.coil = femaleArmour[ARMOUR_LIST_COIL_ID].GetString();
                        hasAny = true;
                    }
                    if (femaleArmour.HasMember(ARMOUR_LIST_LEGS_ID) && femaleArmour[ARMOUR_LIST_LEGS_ID].IsString()) {
                        id.legs = femaleArmour[ARMOUR_LIST_LEGS_ID].GetString();
                        hasAny = true;
                    }

                    if (!hasAny) {
                        DEBUG_STACK.push(std::format("{} Failed to parse armour list entry: \"{}\" (Female). No piece entries found.", KBF_DATA_MANAGER_LOG_TAG, name), DebugStack::Color::COL_ERROR);
                    }

                    out->emplace(set, id);
                    
                    hasEntry |= hasAny;
                }

                if (armourSet.value.HasMember(ARMOUR_LIST_MALE_ID) && armourSet.value[ARMOUR_LIST_MALE_ID].IsObject()) {
                    const auto& maleArmour = armourSet.value[ARMOUR_LIST_MALE_ID].GetObject();
                    ArmourSet set{ name, false };
                    ArmourID id;

                    bool hasAny = false;
                    if (maleArmour.HasMember(ARMOUR_LIST_HELM_ID) && maleArmour[ARMOUR_LIST_HELM_ID].IsString()) {
                        id.helm = maleArmour[ARMOUR_LIST_HELM_ID].GetString();
                        hasAny = true;
                    }
                    if (maleArmour.HasMember(ARMOUR_LIST_BODY_ID) && maleArmour[ARMOUR_LIST_BODY_ID].IsString()) {
                        id.body = maleArmour[ARMOUR_LIST_BODY_ID].GetString();
                        hasAny = true;
                    }
                    if (maleArmour.HasMember(ARMOUR_LIST_ARMS_ID) && maleArmour[ARMOUR_LIST_ARMS_ID].IsString()) {
                        id.arms = maleArmour[ARMOUR_LIST_ARMS_ID].GetString();
                        hasAny = true;
                    }
                    if (maleArmour.HasMember(ARMOUR_LIST_COIL_ID) && maleArmour[ARMOUR_LIST_COIL_ID].IsString()) {
                        id.coil = maleArmour[ARMOUR_LIST_COIL_ID].GetString();
                        hasAny = true;
                    }
                    if (maleArmour.HasMember(ARMOUR_LIST_LEGS_ID) && maleArmour[ARMOUR_LIST_LEGS_ID].IsString()) {
                        id.legs = maleArmour[ARMOUR_LIST_LEGS_ID].GetString();
                        hasAny = true;
                    }

                    if (!hasAny) {
                        DEBUG_STACK.push(std::format("{} Failed to parse armour list entry: \"{}\" (Male). No piece entries found.", KBF_DATA_MANAGER_LOG_TAG, name), DebugStack::Color::COL_ERROR);
                    }

                    out->emplace(set, id);
                    
                    hasEntry |= hasAny;
                }

                if (!hasEntry) {
                    parsed = false;
                    DEBUG_STACK.push(std::format("{} Failed to parse armour list entry: \"{}\". No piece entries found.", KBF_DATA_MANAGER_LOG_TAG, name), DebugStack::Color::COL_ERROR);
                }
            }
            else {
                DEBUG_STACK.push(std::format("{} Failed to parse armour list entry: {}. Expected an object, but got a different type.", KBF_DATA_MANAGER_LOG_TAG, name), DebugStack::Color::COL_ERROR);
            }
        }

        return parsed;
    }

    bool KBFDataManager::writeArmourList(const std::filesystem::path& path, const ArmourMapping& mapping) const {
        DEBUG_STACK.push(std::format("{} Writing armour list to {}", KBF_DATA_MANAGER_LOG_TAG, path.string()), DebugStack::Color::COL_INFO);

        rapidjson::StringBuffer s;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(s);

        writeArmourListJsonContent(mapping, writer);

        bool success = writeJsonFile(path.string(), s.GetString());

        if (!success) {
            DEBUG_STACK.push(std::format("{} Failed to write armour list to {}", KBF_DATA_MANAGER_LOG_TAG, path.string()), DebugStack::Color::COL_ERROR);
        }

        return success;
    }

    void KBFDataManager::writeArmourListJsonContent(
        const ArmourMapping& mapping, 
        rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer
    ) const {
        std::set<std::string> armourNames;
        for (const auto& [set, _] : mapping) {
            armourNames.insert(set.name);
        }

        auto writeCompactArmourSet = [](const ArmourSet& armourSet, const ArmourMapping& mapping) {
            rapidjson::StringBuffer buf;
            rapidjson::Writer<rapidjson::StringBuffer> compactWriter(buf); // one-line writer

            compactWriter.StartObject();
            if (!mapping.at(armourSet).helm.empty()) {
                compactWriter.Key(ARMOUR_LIST_HELM_ID);
                compactWriter.String(mapping.at(armourSet).helm.c_str());
            }
            if (!mapping.at(armourSet).body.empty()) {
                compactWriter.Key(ARMOUR_LIST_BODY_ID);
                compactWriter.String(mapping.at(armourSet).body.c_str());
            }
            if (!mapping.at(armourSet).arms.empty()) {
                compactWriter.Key(ARMOUR_LIST_ARMS_ID);
                compactWriter.String(mapping.at(armourSet).arms.c_str());
            }
            if (!mapping.at(armourSet).coil.empty()) {
                compactWriter.Key(ARMOUR_LIST_COIL_ID);
                compactWriter.String(mapping.at(armourSet).coil.c_str());
            }
            if (!mapping.at(armourSet).legs.empty()) {
                compactWriter.Key(ARMOUR_LIST_LEGS_ID);
                compactWriter.String(mapping.at(armourSet).legs.c_str());
            }
            compactWriter.EndObject();

            return buf;
        };

        writer.StartObject();
        for (const std::string& name : armourNames) {
            writer.Key(name.c_str());
            writer.StartObject();

            ArmourSet femaleArmour{ name, true };
            if (mapping.find(femaleArmour) != mapping.end()) {
                writer.Key(ARMOUR_LIST_FEMALE_ID);

                rapidjson::StringBuffer compactBuf = writeCompactArmourSet(femaleArmour, mapping);
                writer.RawValue(compactBuf.GetString(), compactBuf.GetSize(), rapidjson::kObjectType);
            }

            ArmourSet maleArmour{ name, false };
            if (mapping.find(maleArmour) != mapping.end()) {
                writer.Key(ARMOUR_LIST_MALE_ID);

                rapidjson::StringBuffer compactBuf = writeCompactArmourSet(maleArmour, mapping);
                writer.RawValue(compactBuf.GetString(), compactBuf.GetSize(), rapidjson::kObjectType);
            }

            writer.EndObject();
        }
        writer.EndObject();
    }

}