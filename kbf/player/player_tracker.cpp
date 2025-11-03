#include <kbf/player/player_tracker.hpp>
#include <kbf/situation/situation_watcher.hpp>

#include <kbf/hook/hook_manager.hpp>
#include <kbf/data/armour/find_object_armours.hpp>
#include <kbf/util/re_engine/reinvoke.hpp>
#include <kbf/util/string/ptr_to_hex_string.hpp>
#include <kbf/util/re_engine/dump_transform_tree.hpp>
#include <kbf/util/re_engine/find_transform.hpp>
#include <kbf/util/hash/ptr_hasher.hpp>
#include <kbf/util/re_engine/re_memory_ptr.hpp>
#include <kbf/util/re_engine/dump_components.hpp>
#include <kbf/util/re_engine/print_re_object.hpp>
#include <kbf/debug/debug_stack.hpp>

#include <kbf/profiling/cpu_profiler.hpp>

#define PLAYER_TRACKER_LOG_TAG "[PlayerTracker]"

using REApi = reframework::API;

namespace kbf {

    // Notes:
    //  | app.user_data should probably contain everything we need here.
    //  | app.user_data.PlayerArmorList / .PlayerArmorVisualParam
    //  | app.user_data.MemberListData might provide a way to get the player HunterID in online singleplayer, otherwise will have to cache and make do.
    //  | A lot of info accessible from app.ArmorUtil
    //  | ArmorSeriesData.cData provides an armour id -> name mapping.
    //  | app.user_data.EquipDataSetting contains a bunch of equipment info

    PlayerTracker* PlayerTracker::g_instance = nullptr;

    void PlayerTracker::initialize() {
        g_instance = this;

        auto& api = REApi::get();

        // Main Menu Singletons
		sceneManager = api->get_native_singleton("via.SceneManager");
		assert(sceneManager != nullptr && "Could not get sceneManager!");
		saveDataManager = api->get_managed_singleton("app.SaveDataManager");
		assert(saveDataManager != nullptr && "Could not get saveDataManager!");

        // Guild Card Singletons
        guiManager = api->get_managed_singleton("app.GUIManager");
        assert(guiManager != nullptr && "Could not get GUIManager!");

        // Normal Gameplay Singletons
        playerManager = api->get_managed_singleton("app.PlayerManager");
        assert(playerManager != nullptr && "Could not get playerManager!");
        networkManager = api->get_managed_singleton("app.NetworkManager");
        assert(networkManager != nullptr && "Could not get networkManager!");
		netContextManager = REInvokePtr<REApi::ManagedObject>(networkManager, "get_ContextManager", {});
		assert(netContextManager != nullptr && "Could not get netContextManager!");
        netUserInfoManager = REInvokePtr<REApi::ManagedObject>(networkManager, "get_UserInfoManager", {});
        assert(netUserInfoManager != nullptr && "Could not get networkManager!");
        Net_UserInfoList = REInvokePtr<REApi::ManagedObject>(netUserInfoManager, "getUserInfoList(app.net_session_manager.SESSION_TYPE)", { (void*)1 });
		assert(Net_UserInfoList != nullptr && "Could not get Net_UserInfoList!");

        kbf::HookManager::add_tdb("app.HunterCharacter", "isEquipBuildEnd", onIsEquipBuildEndHook, nullptr, false);
		kbf::HookManager::add_tdb("app.HunterCharacter", "warp",            onWarpHook,            nullptr, false);

        kbf::HookManager::add_tdb("app.GUI010102", "callback_ListSelect", saveSelectListSelectHook, nullptr, false);
    }

    const std::vector<PlayerData> PlayerTracker::getPlayerList() const {
		std::vector<PlayerData> playerDataList;
        for (const auto& info : playerInfos) {
            if (info != nullptr) playerDataList.push_back(info->playerData);
        }
		return playerDataList;
    }

    void PlayerTracker::updatePlayers() {
        fetchPlayers();
        updateApplyDelays();
    }

    void PlayerTracker::applyPresets() {
        bool inQuest = SituationWatcher::inSituation(isinQuestPlayingasGuest) || SituationWatcher::inSituation(isinQuestPlayingasHost);
        if (dataManager.settings().enableDuringQuestsOnly && !inQuest) return;

		// Additionally consider one extra 'preview preset' for those currently being edited in the GUI
		const Preset* previewedPreset = dataManager.getPreviewedPreset();
		const bool hasPreview = previewedPreset != nullptr;
		const bool applyPreviewUnconditional = hasPreview && previewedPreset->armour == ArmourList::DefaultArmourSet();

        // Only apply first n players based on distance to camera
        const int maxPlayersToApply = std::max<int>(dataManager.settings().maxConcurrentApplications, 0);

        std::vector<std::pair<const PlayerData*, size_t>> players;
        players.reserve(playerSlotTable.size());

        for (const auto& [player, idx] : playerSlotTable)
            players.emplace_back(&player, idx);

        if (maxPlayersToApply > 0) {
            std::nth_element(
                players.begin(),
                players.begin() + std::min<size_t>(maxPlayersToApply, players.size()),
                players.end(),
                [&](const std::pair<const PlayerData*, size_t>& a, const std::pair<const PlayerData*, size_t>& b) {
				    return playerInfos[a.second]->distanceFromCameraSq < playerInfos[b.second]->distanceFromCameraSq;
                }
            );
        }

        size_t limit = maxPlayersToApply > 0 ? std::min<size_t>(maxPlayersToApply, players.size()) : players.size();

        for (size_t i = 0; i < limit; ++i) {
            const PlayerData& player = *players[i].first;
            size_t idx = players[i].second;

            if (playerInfos[idx] == nullptr) continue;
            if (playerApplyDelays[player] && playerApplyDelays[player].has_value()) continue;

			const PlayerInfo* infoPtr = playerInfos[idx].get();
            if (infoPtr == nullptr) {
                playerApplyDelays.erase(player);
                continue;
            }

			const PlayerInfo& info = *playerInfos[idx];
            if (!info.visible) continue;

			PersistentPlayerInfo* pInfo = persistentPlayerInfos[idx].get();
            if (pInfo == nullptr) continue;
            if (!pInfo->areSetPointersValid()) {
                persistentPlayerInfos[idx] = nullptr;
                continue;
            }

            if (pInfo->boneManager && pInfo->partManager) {

				// Always apply base presets when they are present, but refrain from re-applying the same base preset multiple times.
                std::unordered_set<std::string> presetBasesApplied{};

                bool hideWeapon = false;
                bool hideSlinger = false;

                bool applyError = false;
                for (ArmourPiece piece = ArmourPiece::AP_MIN_EXCLUDING_SET; piece <= ArmourPiece::AP_MAX_EXCLUDING_SLINGER; piece = static_cast<ArmourPiece>(static_cast<int>(piece) + 1)) {
					std::optional<ArmourSet>& armourPiece = pInfo->armourInfo.getPiece(piece);

                    if (armourPiece.has_value()) {
                        const Preset* preset = dataManager.getActivePreset(player, armourPiece.value(), piece);

                        bool usePreview = hasPreview && (applyPreviewUnconditional || previewedPreset->armour == armourPiece.value());
                        if (preset == nullptr && !usePreview) continue;

                        const Preset* activePreset = usePreview ? previewedPreset : preset;

                        BoneManager::BoneApplyStatusFlag applyFlag = pInfo->boneManager->applyPreset(activePreset, piece);
                        bool invalidBones = applyFlag == BoneManager::BoneApplyStatusFlag::BONE_APPLY_ERROR_INVALID_BONE;
                        if (invalidBones) { 
                            applyError = true; 
                            clearPlayerSlot(idx); 
                            playersToFetch[idx] = true;
                            break; 
                        }

                        pInfo->partManager->applyPreset(activePreset, piece);

                        if (!invalidBones && activePreset->set.hasModifiers() && !presetBasesApplied.contains(activePreset->uuid)) {
                            presetBasesApplied.insert(activePreset->uuid);
                            BoneManager::BoneApplyStatusFlag baseApplyFlag = pInfo->boneManager->applyPreset(activePreset, AP_SET);
                            bool invalidBaseBones = baseApplyFlag == BoneManager::BoneApplyStatusFlag::BONE_APPLY_ERROR_INVALID_BONE;
                            if (invalidBaseBones) { 
                                applyError = true; 
                                clearPlayerSlot(idx); 
                                playersToFetch[idx] = true; 
                                break;
                            }
                        }

                        // Check Weapon & Slinger Disables
                        hideWeapon  |= activePreset->hideWeapon;
                        hideSlinger |= activePreset->hideSlinger;
                    }
                }

                if (!applyError) {
				    // Handle weapon & slinger visibility
                    bool weaponVisible = info.weaponDrawn || !hideWeapon ||
                        (info.inCombat && dataManager.settings().hideWeaponsOutsideOfCombatOnly);

				    if (pInfo->Wp_Parent_GameObject)           REInvokeVoid(pInfo->Wp_Parent_GameObject,           "set_DrawSelf", { (void*)(weaponVisible) });
				    if (pInfo->WpSub_Parent_GameObject)        REInvokeVoid(pInfo->WpSub_Parent_GameObject,        "set_DrawSelf", { (void*)(weaponVisible) });
				    if (pInfo->Wp_ReserveParent_GameObject)    REInvokeVoid(pInfo->Wp_ReserveParent_GameObject,    "set_DrawSelf", { (void*)(weaponVisible) });
				    if (pInfo->WpSub_ReserveParent_GameObject) REInvokeVoid(pInfo->WpSub_ReserveParent_GameObject, "set_DrawSelf", { (void*)(weaponVisible) });
            
				    bool slingerVisible = !hideSlinger || (info.inCombat && dataManager.settings().hideSlingerOutsideOfCombatOnly);
				    if (pInfo->Slinger_GameObject) REInvokeVoid(pInfo->Slinger_GameObject, "set_DrawSelf", { (void*)(slingerVisible) });
                }
            }
        }
    }

    void PlayerTracker::reset() {
        playerSlotTable.clear();
        playerApplyDelays.clear();
        for (auto& p : playerInfos)                 p.reset();
        for (auto& p : persistentPlayerInfos)       p.reset();
        
        playersToFetch.fill(false);
        occupiedNormalGameplaySlots.fill(false);

        saveSelectHunterTransformCache              = nullptr;
        characterCreatorHunterTransformCache        = nullptr;
        guildCardHunterTransformCache               = nullptr;
        saveSelectHashedArmourTransformsCache       = std::nullopt;
        characterCreatorHashedArmourTransformsCache = std::nullopt;
        guildCardHashedArmourTransformsCache        = std::nullopt;
    }

    void PlayerTracker::fetchPlayers() {
        frameBoneFetchCount = 0;
        std::optional<CustomSituation> thisUpdateSituation = std::nullopt;

        const bool mainMenu         = SituationWatcher::inCustomSituation(CustomSituation::isInMainMenuScene);
        const bool saveSelect       = SituationWatcher::inCustomSituation(CustomSituation::isInSaveSelectGUI);
        const bool characterCreator = SituationWatcher::inCustomSituation(CustomSituation::isInCharacterCreator);
        const bool guildCard        = SituationWatcher::inCustomSituation(CustomSituation::isInHunterGuildCard);
        const bool cutscene         = SituationWatcher::inCustomSituation(CustomSituation::isInCutscene);


        if      (mainMenu        ) thisUpdateSituation = CustomSituation::isInMainMenuScene;   
        else if (saveSelect      ) thisUpdateSituation = CustomSituation::isInSaveSelectGUI;   
        else if (characterCreator) thisUpdateSituation = CustomSituation::isInCharacterCreator;
        else if (guildCard       ) thisUpdateSituation = CustomSituation::isInHunterGuildCard; 
        else if (cutscene        ) thisUpdateSituation = CustomSituation::isInCutscene;        

        if (thisUpdateSituation != lastSituation) {
            lastSituation = thisUpdateSituation;
            reset();
		}

        if      (mainMenu        ) fetchPlayers_MainMenu();
        else if (saveSelect      ) fetchPlayers_SaveSelect();
        else if (characterCreator) fetchPlayers_CharacterCreator();
        else if (guildCard       ) fetchPlayers_HunterGuildCard();
        else if (cutscene        ) fetchPlayers_NormalGameplay();
        else                       fetchPlayers_NormalGameplay();
    }

    void PlayerTracker::fetchPlayers_MainMenu() {
        // Player info only needs to be fetched once, as it will never change until we leave and re-enter.
        if (playerSlotTable.size() > 0) return;

		//- Basic Info -------------------------------------
        BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Main Menu - Basic Info");

        int saveIdx = -1;
        PlayerInfo info{};
        bool fetchedBasicInfo = fetchPlayers_MainMenu_BasicInfo(info, saveIdx);
		if (!fetchedBasicInfo) return;

        END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Main Menu - Basic Info");
		//--------------------------------------------------

        PersistentPlayerInfo persistentInfo{};
        persistentInfo.playerData = info.playerData;
        persistentInfo.index      = 0;

		//- Equipped Armours -------------------------------
        bool fetchedArmours = fetchPlayer_EquippedArmours(info, persistentInfo);
        BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Main Menu - Equipped Armours");

        if (!fetchedArmours) {
            DEBUG_STACK.push(std::format("{} Failed to fetch equipped armours for Main Menu Hunter: {} [{}]", PLAYER_TRACKER_LOG_TAG, info.playerData.name, info.playerData.hunterId), DebugStack::Color::WARNING);
            return; // We terminate this whole fetch in this case, but can probably just try this portion again instead - ...Too bad!
        }

        END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Main Menu - Equipped Armours");
        //--------------------------------------------------

		//- Armour Transforms ------------------------------
        BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Main Menu - Armour Transforms");

        bool fetchedTransforms = fetchPlayer_ArmourTransforms(info, persistentInfo);
        if (!fetchedTransforms) {
            DEBUG_STACK.push(std::format("{} Failed to fetch armour transforms for Main Menu Hunter: {} [{}]", PLAYER_TRACKER_LOG_TAG, info.playerData.name, info.playerData.hunterId), DebugStack::Color::WARNING);
            return;
        }

		END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Main Menu - Armour Transforms");
		//--------------------------------------------------

		//- Bones ------------------------------------------
        BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Main Menu - Bones");

        bool fetchedBones = fetchPlayer_Bones(info, persistentInfo);
        if (!fetchedBones) {
            std::string reason = "Unknown";
            if (info.pointers.Transform == nullptr)                reason = "Body ptr was null";
            else if (persistentInfo.Transform_body == nullptr)     reason = "Body Transform ptr was null";
            else if (persistentInfo.Transform_legs == nullptr)     reason = "Legs Transform ptr was null";
            else if (!persistentInfo.armourInfo.body.has_value())  reason = "No body armour found";
            else if (!persistentInfo.armourInfo.legs.has_value())  reason = "No legs armour found";
            DEBUG_STACK.push(std::format("{} Failed to fetch bones for Main Menu Hunter: {} [{}]. Reason: {}.", PLAYER_TRACKER_LOG_TAG, info.playerData.name, info.playerData.hunterId, reason), DebugStack::Color::WARNING);
            return;
        }

		END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Main Menu - Bones");
        //--------------------------------------------------

		//- Parts ------------------------------------------
        BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Main Menu - Parts");

        bool fetchedParts = fetchPlayer_Parts(info, persistentInfo);
        if (!fetchedParts) {
            DEBUG_STACK.push(std::format("{} Failed to fetch parts for Main Menu Hunter: {} [{}]", PLAYER_TRACKER_LOG_TAG, info.playerData.name, info.playerData.hunterId), DebugStack::Color::WARNING);
            return;
        }

		END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Main Menu - Parts");
        //--------------------------------------------------

		//- Weapon Objects ---------------------------------
        BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Main Menu - Weapon Objects");

		bool fetchedWeapons = fetchPlayers_MainMenu_WeaponObjects(info, persistentInfo);
        if (!fetchedWeapons) {
            DEBUG_STACK.push(std::format("{} Failed to fetch weapon objects for Main Menu Hunter: {} [{}]", PLAYER_TRACKER_LOG_TAG, info.playerData.name, info.playerData.hunterId), DebugStack::Color::WARNING);
            return;
		}

		END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Main Menu - Weapon Objects");
        //--------------------------------------------------

        playerApplyDelays[persistentInfo.playerData] = std::chrono::high_resolution_clock::now();
        persistentPlayerInfos[0] = std::make_unique<PersistentPlayerInfo>(std::move(persistentInfo));

        playerSlotTable.emplace(info.playerData, 0);
        playerInfos[0] = std::make_unique<PlayerInfo>(std::move(info));
	}

    bool PlayerTracker::fetchPlayers_MainMenu_BasicInfo(PlayerInfo& outInfo, int& outSaveIdx) {
        outInfo = PlayerInfo{};

        REApi::ManagedObject* currentScene = getCurrentScene();
        if (currentScene == nullptr) return false;

        static const REApi::ManagedObject* transformType = REApi::get()->typeof("via.Transform");
        REApi::ManagedObject* transformComponents = REInvokePtr<REApi::ManagedObject>(currentScene, "findComponents(System.Type)", { (void*)transformType });

        constexpr const char* playerTransformNamePrefix = "Pl000_00";
        const int numComponents = REInvoke<int>(transformComponents, "GetLength", { (void*)0 }, InvokeReturnType::DWORD);

        for (int i = 0; i < numComponents; i++) {
            REApi::ManagedObject* transform = REInvokePtr<REApi::ManagedObject>(transformComponents, "get_Item", { (void*)i });
            if (transform == nullptr) continue;

            REApi::ManagedObject* gameObject = REInvokePtr<REApi::ManagedObject>(transform, "get_GameObject", {});
            if (gameObject == nullptr) continue;

            std::string name = REInvokeStr(gameObject, "get_Name", {});
            if (name.starts_with(playerTransformNamePrefix)) {

                static const REApi::ManagedObject* typeof_EventModelSetupper = REApi::get()->typeof("app.EventModelSetupper");
                REApi::ManagedObject* eventModelSetupper = REInvokePtr<REApi::ManagedObject>(gameObject, "getComponent(System.Type)", { (void*)typeof_EventModelSetupper });
                if (eventModelSetupper != nullptr) {
                    outInfo.optionalPointers.EventModelSetupper = eventModelSetupper;
                    outInfo.pointers.Transform = transform; // set transform here to avoid re-lookup
                    break;
                }
            }
        }

        if (outInfo.optionalPointers.EventModelSetupper == nullptr) return false;

        // TODO: Note, there is also mcNpcVisualController for use with NPCs
        REApi::ManagedObject* mcPreviewHunterVisualController = REFieldPtr<REApi::ManagedObject>(outInfo.optionalPointers.EventModelSetupper, "_HunterVisualController", false);
		if (mcPreviewHunterVisualController == nullptr) return false;

        int32_t* equipAppearanceSaveIndex = REFieldPtr<int32_t>(mcPreviewHunterVisualController, "_EquipAppearanceSaveIndex", true);
		if (equipAppearanceSaveIndex == nullptr) return false;
        // This is sketch af, the field ptr here is off by 0x10 for some reason... would be better to straight read the memmory.
        // WARNING: This might change with game updates.
        equipAppearanceSaveIndex = reinterpret_cast<int32_t*>((uintptr_t)equipAppearanceSaveIndex + 0x10);

		outSaveIdx = *equipAppearanceSaveIndex;

        PlayerData mainHunter{};
        bool fetchedMainHunter = getSavePlayerData(outSaveIdx, mainHunter);
		if (!fetchedMainHunter) return false;

        outInfo.playerData = mainHunter;
        outInfo.index      = 0;
        outInfo.visible    = true;

        return true;
    }

    bool PlayerTracker::fetchPlayers_MainMenu_WeaponObjects(const PlayerInfo& info, PersistentPlayerInfo& outPInfo) {
        if (info.optionalPointers.EventModelSetupper == nullptr) return false;

        outPInfo.Wp_Parent_GameObject    = REFieldPtr<REApi::ManagedObject>(info.optionalPointers.EventModelSetupper, "_WeaponObj",    false);
        outPInfo.WpSub_Parent_GameObject = REFieldPtr<REApi::ManagedObject>(info.optionalPointers.EventModelSetupper, "_WeaponSubObj", false);

        return (outPInfo.Wp_Parent_GameObject || outPInfo.WpSub_Parent_GameObject);
    }

    void PlayerTracker::fetchPlayers_SaveSelect() {
		//- Basic Info -------------------------------------
		BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Save Select - Basic Info");

        PlayerInfo info{};
        bool fetchedBasicInfo = fetchPlayers_SaveSelect_BasicInfo(info);
        if (!fetchedBasicInfo) {
            saveSelectHunterTransformCache = nullptr;
            saveSelectHashedArmourTransformsCache = std::nullopt;
            return;
        }

		END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Save Select - Basic Info");
        //--------------------------------------------------

        PersistentPlayerInfo persistentInfo{};
        persistentInfo.playerData = info.playerData;
        persistentInfo.index = 0;

		//- Equipped Armours -------------------------------
		BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Save Select - Equipped Armours");

        bool fetchedArmours = fetchPlayer_EquippedArmours(info, persistentInfo);
        if (!fetchedArmours) {
            return;
        }

		END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Save Select - Equipped Armours");
        //--------------------------------------------------

		//- Armour Transforms ------------------------------
		BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Save Select - Armour Transforms");

        bool fetchedTransforms = fetchPlayer_ArmourTransforms(info, persistentInfo);
        if (!fetchedTransforms) {
            return;
        }

        // Hash the armour transforms to see if anything else needs to be done.
        PtrHasher hasher;
        size_t hashedArmourTransforms = hasher(
            persistentInfo.Transform_base,
            persistentInfo.Transform_helm,
            persistentInfo.Transform_body,
            persistentInfo.Transform_arms,
            persistentInfo.Transform_coil,
            persistentInfo.Transform_legs,
            persistentInfo.Slinger_GameObject);

		END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Save Select - Armour Transforms");
		//--------------------------------------------------

        if (hashedArmourTransforms != saveSelectHashedArmourTransformsCache) {
            saveSelectHashedArmourTransformsCache = hashedArmourTransforms;

		    //- Bones ------------------------------------------
		    BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Save Select - Bones");

            bool fetchedBones = fetchPlayer_Bones(info, persistentInfo);
            if (!fetchedBones) {
                std::string reason = "Unknown";
                if (info.pointers.Transform == nullptr)                reason = "Body ptr was null";
                else if (persistentInfo.Transform_body == nullptr)     reason = "Body Transform ptr was null";
                else if (persistentInfo.Transform_legs == nullptr)     reason = "Legs Transform ptr was null";
                else if (!persistentInfo.armourInfo.body.has_value())  reason = "No body armour found";
                else if (!persistentInfo.armourInfo.legs.has_value())  reason = "No legs armour found";
                DEBUG_STACK.push(std::format("{} Failed to fetch bones for Save Select Hunter: {} [{}]. Reason: {}.", PLAYER_TRACKER_LOG_TAG, info.playerData.name, info.playerData.hunterId, reason), DebugStack::Color::WARNING);
                return;
            }

		    END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Save Select - Bones");
            //--------------------------------------------------

		    //- Parts ------------------------------------------
		    BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Save Select - Parts");

            bool fetchedParts = fetchPlayer_Parts(info, persistentInfo);
            if (!fetchedParts) {
                DEBUG_STACK.push(std::format("{} Failed to fetch parts for Save Select Hunter: {} [{}]", PLAYER_TRACKER_LOG_TAG, info.playerData.name, info.playerData.hunterId), DebugStack::Color::WARNING);
                return;
            }

		    END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Save Select - Parts");
            //--------------------------------------------------

		    //- Weapon Objects ------------------------------e---
		    BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Save Select - Weapon Objects");

            bool fetchedWeapons = fetchPlayers_SaveSelect_WeaponObjects(info, persistentInfo);
            if (!fetchedWeapons) {
                DEBUG_STACK.push(std::format("{} Failed to fetch weapon objects for Save Select Hunter: {} [{}]", PLAYER_TRACKER_LOG_TAG, info.playerData.name, info.playerData.hunterId), DebugStack::Color::WARNING);
                return;
            }

		    END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Save Select - Weapon Objects");
            //--------------------------------------------------

            playerApplyDelays[persistentInfo.playerData] = std::chrono::high_resolution_clock::now();
            persistentPlayerInfos[0] = std::make_unique<PersistentPlayerInfo>(std::move(persistentInfo));
        }

        playerSlotTable.emplace(info.playerData, 0);
        playerInfos[0] = std::make_unique<PlayerInfo>(std::move(info));
    }

    bool PlayerTracker::fetchPlayers_SaveSelect_BasicInfo(PlayerInfo& outInfo) {
        PlayerData mainHunter{};
        bool fetchedMainHunter = getSavePlayerData(lastSelectedSaveIdx, mainHunter);
        if (!fetchedMainHunter) return false;

        outInfo = PlayerInfo{};

        bool usedCache = false;
        if (saveSelectHunterTransformCache != nullptr) {
            // Check the cache hasn't been invalidated
            static const REApi::TypeDefinition* def_ViaTransform = REApi::get()->tdb()->find_type("via.Transform");
            if (checkREPtrValidity(saveSelectHunterTransformCache, def_ViaTransform)) {
                outInfo.pointers.Transform = saveSelectHunterTransformCache;
                usedCache = true;
            }
        }
        
        if (!usedCache) {
            REApi::ManagedObject* currentScene = getCurrentScene();
            if (currentScene == nullptr) return false;

            static const REApi::ManagedObject* transformType = REApi::get()->typeof("via.Transform");
            REApi::ManagedObject* transformComponents = REInvokePtr<REApi::ManagedObject>(currentScene, "findComponents(System.Type)", { (void*)transformType });

            // Mannequin_Hunter > SaveSelectHunter_XX / XY
            constexpr const char* playerTransformNamePrefixXX = "SaveSelect_HunterXX";
		    constexpr const char* playerTransformNamePrefixXY = "SaveSelect_HunterXY";
            const int numComponents = REInvoke<int>(transformComponents, "GetLength", { (void*)0 }, InvokeReturnType::DWORD);

            for (int i = 0; i < numComponents; i++) {
                REApi::ManagedObject* transform = REInvokePtr<REApi::ManagedObject>(transformComponents, "get_Item", { (void*)i });
                if (transform == nullptr) continue;

                REApi::ManagedObject* gameObject = REInvokePtr<REApi::ManagedObject>(transform, "get_GameObject", {});
                if (gameObject == nullptr) continue;

                // TODO: Choose based on char info
                std::string name = REInvokeStr(gameObject, "get_Name", {});
                if (name.starts_with(mainHunter.female ? playerTransformNamePrefixXX : playerTransformNamePrefixXY)) {
                    outInfo.pointers.Transform = transform;
					saveSelectHunterTransformCache = transform;
                    break;
                }

            }
        }

        int* visibilityPtr = re_memory_ptr<int>(outInfo.optionalPointers.VolumeOccludee, 0x2C9);

        outInfo.playerData = mainHunter;
        outInfo.index      = 0;
        outInfo.visible    = true;

        return true;
    }

    bool PlayerTracker::fetchPlayers_SaveSelect_WeaponObjects(const PlayerInfo& info, PersistentPlayerInfo& outPInfo) {
        REApi::ManagedObject* currentScene = getCurrentScene();
        if (currentScene == nullptr) return false;

        static const REApi::ManagedObject* transformType = REApi::get()->typeof("via.Transform");
        REApi::ManagedObject* transformComponents = REInvokePtr<REApi::ManagedObject>(currentScene, "findComponents(System.Type)", { (void*)transformType });

        constexpr const char* weaponStrPrefix = "Wp";
        constexpr const char* weaponParentStr = "Wp_Parent";
        constexpr const char* weaponSubParentStr = "WpSub_Parent";
        const int numComponents = REInvoke<int>(transformComponents, "GetLength", { (void*)0 }, InvokeReturnType::DWORD);

        bool foundMainWp = false;
        bool foundSubWp  = false;
        for (int i = 0; i < numComponents; i++) {
            if (foundMainWp && foundSubWp) break;

            REApi::ManagedObject* transform = REInvokePtr<REApi::ManagedObject>(transformComponents, "get_Item", { (void*)i });
            if (transform == nullptr) continue;

            REApi::ManagedObject* gameObject = REInvokePtr<REApi::ManagedObject>(transform, "get_GameObject", {});
            if (gameObject == nullptr) continue;

            std::string name = REInvokeStr(gameObject, "get_Name", {});
            if (name.starts_with(weaponStrPrefix)) {
                if (!foundMainWp && name.starts_with(weaponParentStr)) {
                    REApi::ManagedObject* partsSwitch = REInvokePtr<REApi::ManagedObject>(gameObject, "getComponent(System.Type)", { (void*)REApi::get()->typeof("app.PartsSwitch") });
                    if (partsSwitch != nullptr) {
                        outPInfo.Wp_Parent_GameObject = gameObject;
                        foundMainWp = true;
                    }
                }
                else if (!foundSubWp && name.starts_with(weaponSubParentStr)) {
                    REApi::ManagedObject* partsSwitch = REInvokePtr<REApi::ManagedObject>(gameObject, "getComponent(System.Type)", { (void*)REApi::get()->typeof("app.PartsSwitch") });
                    if (partsSwitch != nullptr) {
                        outPInfo.WpSub_Parent_GameObject = gameObject;
                        foundSubWp = true;
                    }
                }
            }
        }

        return foundMainWp || foundSubWp;
	}

    void PlayerTracker::fetchPlayers_CharacterCreator() {
        //- Basic Info -------------------------------------
        BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Character Creator - Basic Info");

        PlayerInfo info{};
        bool fetchedBasicInfo = fetchPlayers_CharacterCreator_BasicInfo(info);
        if (!fetchedBasicInfo) {
            characterCreatorHunterTransformCache = nullptr;
            characterCreatorHashedArmourTransformsCache = std::nullopt;
            return;
        }

        END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Character Creator - Basic Info");
        //--------------------------------------------------

        PersistentPlayerInfo persistentInfo{};
        persistentInfo.playerData = info.playerData;
        persistentInfo.index = 0;

        //- Equipped Armours -------------------------------
        BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Character Creator - Equipped Armours");

        bool fetchedArmours = fetchPlayer_EquippedArmours(info, persistentInfo);
        if (!fetchedArmours) {
            return;
        }

        END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Character Creator - Equipped Armours");
        //--------------------------------------------------

        //- Armour Transforms ------------------------------
        BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Character Creator - Armour Transforms");

        bool fetchedTransforms = fetchPlayer_ArmourTransforms(info, persistentInfo);
        if (!fetchedTransforms) {
            return;
        }

        // Hash the armour transforms to see if anything else needs to be done.
        PtrHasher hasher;
        size_t hashedArmourTransforms = hasher(
            persistentInfo.Transform_base,
            persistentInfo.Transform_helm,
            persistentInfo.Transform_body,
            persistentInfo.Transform_arms,
            persistentInfo.Transform_coil,
            persistentInfo.Transform_legs,
            persistentInfo.Slinger_GameObject);

        END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Character Creator - Armour Transforms");
        //--------------------------------------------------

        if (hashedArmourTransforms != characterCreatorHashedArmourTransformsCache) {
            characterCreatorHashedArmourTransformsCache = hashedArmourTransforms;

            //- Bones ------------------------------------------
            BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Character Creator - Bones");

            bool fetchedBones = fetchPlayer_Bones(info, persistentInfo);
            if (!fetchedBones) {
                std::string reason = "Unknown";
                if (info.pointers.Transform == nullptr)                reason = "Body ptr was null";
                else if (persistentInfo.Transform_body == nullptr)     reason = "Body Transform ptr was null";
                else if (persistentInfo.Transform_legs == nullptr)     reason = "Legs Transform ptr was null";
                else if (!persistentInfo.armourInfo.body.has_value())  reason = "No body armour found";
                else if (!persistentInfo.armourInfo.legs.has_value())  reason = "No legs armour found";
                DEBUG_STACK.push(std::format("{} Failed to fetch bones for Character Creator Hunter: {} [{}]. Reason: {}.", PLAYER_TRACKER_LOG_TAG, info.playerData.name, info.playerData.hunterId, reason), DebugStack::Color::WARNING);
                return;
            }

            END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Character Creator - Bones");
            //--------------------------------------------------

            //- Parts ------------------------------------------
            BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Character Creator - Parts");

            bool fetchedParts = fetchPlayer_Parts(info, persistentInfo);
            if (!fetchedParts) {
                DEBUG_STACK.push(std::format("{} Failed to fetch parts for Character Creator Hunter: {} [{}]", PLAYER_TRACKER_LOG_TAG, info.playerData.name, info.playerData.hunterId), DebugStack::Color::WARNING);
                return;
            }

            END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Character Creator - Parts");
            //--------------------------------------------------

            playerApplyDelays[persistentInfo.playerData] = std::chrono::high_resolution_clock::now();
            persistentPlayerInfos[0] = std::make_unique<PersistentPlayerInfo>(std::move(persistentInfo));
        }

        playerSlotTable.emplace(info.playerData, 0);
        playerInfos[0] = std::make_unique<PlayerInfo>(std::move(info));
	}

    bool PlayerTracker::fetchPlayers_CharacterCreator_BasicInfo(PlayerInfo& outInfo) {
        PlayerData hunter{};
        bool gotSaveData = false;

        // If accessed from save select, use last hovered save idx, otherwise if from in-game, read from active save data.
        if (SituationWatcher::inCustomSituation(CustomSituation::isInTitleMenus)) {
            gotSaveData = getSavePlayerData(lastSelectedSaveIdx, hunter);
        } 
        else if (SituationWatcher::inCustomSituation(CustomSituation::isInGame)) {
            gotSaveData = getActiveSavePlayerData(hunter);
        } 

        if (!gotSaveData) return false;

        outInfo = PlayerInfo{};

        bool usedCache = false;
        if (saveSelectHunterTransformCache != nullptr) {
            // Check the cache hasn't been invalidated
            static const REApi::TypeDefinition* def_ViaTransform = REApi::get()->tdb()->find_type("via.Transform");
            if (checkREPtrValidity(saveSelectHunterTransformCache, def_ViaTransform)) {
                outInfo.pointers.Transform = saveSelectHunterTransformCache;
                usedCache = true;
            }
        }

        if (!usedCache) {
            REApi::ManagedObject* currentScene = getCurrentScene();
            if (currentScene == nullptr) return false;

            static const REApi::ManagedObject* transformType = REApi::get()->typeof("via.Transform");
            REApi::ManagedObject* transformComponents = REInvokePtr<REApi::ManagedObject>(currentScene, "findComponents(System.Type)", { (void*)transformType });

            constexpr const char* playerTransformNamePrefixXX = "CharaMake_HunterXX";
            constexpr const char* playerTransformNamePrefixXY = "CharaMake_HunterXY";
            const int numComponents = REInvoke<int>(transformComponents, "GetLength", { (void*)0 }, InvokeReturnType::DWORD);

            for (int i = 0; i < numComponents; i++) {
                REApi::ManagedObject* transform = REInvokePtr<REApi::ManagedObject>(transformComponents, "get_Item", { (void*)i });
                if (transform == nullptr) continue;

                REApi::ManagedObject* gameObject = REInvokePtr<REApi::ManagedObject>(transform, "get_GameObject", {});
                if (gameObject == nullptr) continue;

                // TODO: Choose based on char info
                std::string name = REInvokeStr(gameObject, "get_Name", {});
                if (name.starts_with(hunter.female ? playerTransformNamePrefixXX : playerTransformNamePrefixXY)) {
                    outInfo.pointers.Transform = transform;
                    saveSelectHunterTransformCache = transform;
                    break;
                }
            }
        }

        outInfo.playerData = hunter;
        outInfo.index = 0;
        outInfo.visible = true;

        return true;
    }

    void PlayerTracker::fetchPlayers_HunterGuildCard() {
        //- Basic Info -------------------------------------
        BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Guild Card - Basic Info");

        PlayerInfo info{};
        bool fetchedBasicInfo = fetchPlayers_HunterGuildCard_BasicInfo(info);
        if (!fetchedBasicInfo) {
            guildCardHunterTransformCache = nullptr;
            guildCardHashedArmourTransformsCache = std::nullopt;
            return;
        }

        END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Guild Card - Basic Info");
        //--------------------------------------------------

        PersistentPlayerInfo persistentInfo{};
        persistentInfo.playerData = info.playerData;
        persistentInfo.index = 0;

        //- Equipped Armours -------------------------------
        BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Guild Card - Equipped Armours");

        bool fetchedArmours = fetchPlayer_EquippedArmours(info, persistentInfo);
        if (!fetchedArmours) {
            return;
        }

        END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Guild Card - Equipped Armours");
        //--------------------------------------------------

        //- Armour Transforms ------------------------------
        BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Guild Card - Armour Transforms");

        bool fetchedTransforms = fetchPlayer_ArmourTransforms(info, persistentInfo);
        if (!fetchedTransforms) {
            return;
        }

        // Hash the armour transforms to see if anything else needs to be done.
        PtrHasher hasher;
        size_t hashedArmourTransforms = hasher(
            persistentInfo.Transform_base,
            persistentInfo.Transform_helm,
            persistentInfo.Transform_body,
            persistentInfo.Transform_arms,
            persistentInfo.Transform_coil,
            persistentInfo.Transform_legs,
            persistentInfo.Slinger_GameObject);

        END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Guild Card - Armour Transforms");
        //--------------------------------------------------

        if (hashedArmourTransforms != guildCardHashedArmourTransformsCache) {
            guildCardHashedArmourTransformsCache = hashedArmourTransforms;

            //- Bones ------------------------------------------
            BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Guild Card - Bones");

            bool fetchedBones = fetchPlayer_Bones(info, persistentInfo);
            if (!fetchedBones) {
                std::string reason = "Unknown";
                if (info.pointers.Transform == nullptr)                reason = "Body ptr was null";
                else if (persistentInfo.Transform_body == nullptr)     reason = "Body Transform ptr was null";
                else if (persistentInfo.Transform_legs == nullptr)     reason = "Legs Transform ptr was null";
                else if (!persistentInfo.armourInfo.body.has_value())  reason = "No body armour found";
                else if (!persistentInfo.armourInfo.legs.has_value())  reason = "No legs armour found";
                DEBUG_STACK.push(std::format("{} Failed to fetch bones for Guild Card Hunter: {} [{}]. Reason: {}.", PLAYER_TRACKER_LOG_TAG, info.playerData.name, info.playerData.hunterId, reason), DebugStack::Color::WARNING);
                return;
            }

            END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Guild Card - Bones");
            //--------------------------------------------------

            //- Parts ------------------------------------------
            BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Guild Card - Parts");

            bool fetchedParts = fetchPlayer_Parts(info, persistentInfo);
            if (!fetchedParts) {
                DEBUG_STACK.push(std::format("{} Failed to fetch parts for Guild Card Hunter: {} [{}]", PLAYER_TRACKER_LOG_TAG, info.playerData.name, info.playerData.hunterId), DebugStack::Color::WARNING);
                return;
            }

            END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Guild Card - Parts");
            //--------------------------------------------------

            playerApplyDelays[persistentInfo.playerData] = std::chrono::high_resolution_clock::now();
            persistentPlayerInfos[0] = std::make_unique<PersistentPlayerInfo>(std::move(persistentInfo));
        }

        playerSlotTable.emplace(info.playerData, 0);
        playerInfos[0] = std::make_unique<PlayerInfo>(std::move(info));
	}

    bool PlayerTracker::fetchPlayers_HunterGuildCard_BasicInfo(PlayerInfo& outInfo) {
        // TODO Maybe check field IsWaitingOpenProfile

        bool* isSelfProfile = re_memory_ptr<bool>(guiManager, 0x454); //REFieldPtr<bool>(guiManager, "_HunterProfile_IsSelfProfile", true);
        if (isSelfProfile == nullptr) return false;

        bool isSelfProfile_value = *isSelfProfile;
        PlayerData hunter{};

        if (isSelfProfile_value == true) {
            // Get from Current save data
            bool gotData = getActiveSavePlayerData(hunter);
            if (!gotData) return false;
        }
        else {
            // Name & Hunter ID
            REApi::ManagedObject* HunterProfile_UserInfo = REFieldPtr<REApi::ManagedObject>(guiManager, "_HunterProfile_UserInfo", false);
            if (HunterProfile_UserInfo == nullptr) return false;

            std::string name = REInvokeStr(HunterProfile_UserInfo, "get_PlName", {});
            if (name.empty()) return false;

            std::string shortHunterId = REInvokeStr(HunterProfile_UserInfo, "get_ShortHunterId", {});
            if (shortHunterId.empty()) return false;

            // Go fishing for the gender...
            REApi::ManagedObject* GuildCardSceneController = REFieldPtr<REApi::ManagedObject>(guiManager, "_HunterProfile_SceneController", false);
            if (GuildCardSceneController == nullptr) return false;

            REApi::ManagedObject* CharacterEditBuilder = REFieldPtr<REApi::ManagedObject>(GuildCardSceneController, "_HunterBuilder", false);
            if (CharacterEditBuilder == nullptr) return false;

            int physiqueStyle = REInvoke<int>(CharacterEditBuilder, "get_PhysiqueStyle", {}, InvokeReturnType::DWORD);
            bool female = physiqueStyle == 2;

            hunter.female = female;
            hunter.name = name;
            hunter.hunterId = shortHunterId;
        }

        outInfo = PlayerInfo{};

        bool usedCache = false;
        if (guildCardHunterTransformCache != nullptr) {
            // Check the cache hasn't been invalidated
            static const REApi::TypeDefinition* def_ViaTransform = REApi::get()->tdb()->find_type("via.Transform");
            if (checkREPtrValidity(guildCardHunterTransformCache, def_ViaTransform)) {
                outInfo.pointers.Transform = guildCardHunterTransformCache;
                usedCache = true;
            }
        }

        if (!usedCache) {
            REApi::ManagedObject* currentScene = getCurrentScene();
            if (currentScene == nullptr) return false;

            static const REApi::ManagedObject* transformType = REApi::get()->typeof("via.Transform");
            REApi::ManagedObject* transformComponents = REInvokePtr<REApi::ManagedObject>(currentScene, "findComponents(System.Type)", { (void*)transformType });

            constexpr const char* playerTransformNamePrefixXX = "GuildCard_HunterXX";
            constexpr const char* playerTransformNamePrefixXY = "GuildCard_HunterXY";
            const int numComponents = REInvoke<int>(transformComponents, "GetLength", { (void*)0 }, InvokeReturnType::DWORD);

            for (int i = 0; i < numComponents; i++) {
                REApi::ManagedObject* transform = REInvokePtr<REApi::ManagedObject>(transformComponents, "get_Item", { (void*)i });
                if (transform == nullptr) continue;

                REApi::ManagedObject* gameObject = REInvokePtr<REApi::ManagedObject>(transform, "get_GameObject", {});
                if (gameObject == nullptr) continue;

                // TODO: Choose based on char info
                std::string name = REInvokeStr(gameObject, "get_Name", {});
                if (name.starts_with(hunter.female ? playerTransformNamePrefixXX : playerTransformNamePrefixXY)) {
                    outInfo.pointers.Transform = transform;
                    guildCardHunterTransformCache = transform;
                    break;
                }
            }
        }

        outInfo.playerData = hunter;
        outInfo.index = 0;
        outInfo.visible = true;

        return true;
    }

    void PlayerTracker::fetchPlayers_NormalGameplay() {
        bool inQuest = SituationWatcher::inSituation(isinQuestPlayingasGuest) ||
                       SituationWatcher::inSituation(isinQuestPlayingasHost);
        bool online  = SituationWatcher::inSituation(isOnline);

        const bool useCache = !needsAllPlayerFetch;
        for (size_t i = 0; i < PLAYER_LIST_SIZE; i++) {
            if (needsAllPlayerFetch || occupiedNormalGameplaySlots[i] || playersToFetch[i]) {
                fetchPlayers_NormalGameplay_SinglePlayer(i, useCache, inQuest, online);
            }
        }

        needsAllPlayerFetch = false;
    }


    void PlayerTracker::fetchPlayers_NormalGameplay_SinglePlayer(size_t i, bool useCache, bool inQuest, bool online) {
        // -- Basic Info --------------------------------------------------------------------------------------------

        PlayerInfo info{};
        bool usedCache = false;
        const bool cacheExists = playerInfoCaches[i].has_value();
        const bool cacheValid = cacheExists && playerInfoCaches[i].value().isValid();
        if (useCache && cacheValid) {
            BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Normal Gameplay - Basic Info");
            BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Normal Gameplay - Basic Info - Cache Load");
            const NormalGameplayPlayerCache& slotCache = playerInfoCaches[i].value();
            if (!slotCache.isEmpty()) {
                info.index                            = i;
                info.playerData                       = slotCache.playerData;
                info.pointers.Transform               = slotCache.Transform;
                info.optionalPointers.Motion          = slotCache.Motion;
                info.optionalPointers.HunterCharacter = slotCache.HunterCharacter;
                usedCache = true;
            }
            END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Normal Gameplay - Basic Info - Cache Load");
            END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Normal Gameplay - Basic Info");
        }

        if (!usedCache) {
            BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Normal Gameplay - Basic Info");
            PlayerFetchFlags fetchFlags = fetchPlayer_BasicInfo(i, inQuest, online, info);
            if (fetchFlags == PlayerFetchFlags::FETCH_PLAYER_SLOT_EMPTY) {
                playersToFetch[i] = false;
                return;
            }
            else if (fetchFlags == PlayerFetchFlags::FETCH_ERROR_NULL || info.pointers.Transform == nullptr) {
                playerInfoCaches[i] = NormalGameplayPlayerCache{ .cacheIsEmpty = true };
                END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Normal Gameplay - Basic Info");
                return;
            }
            END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Normal Gameplay - Basic Info");
            // Update Cached Basic Info
            NormalGameplayPlayerCache newCache{};
            newCache.playerData      = info.playerData;
            newCache.Transform       = info.pointers.Transform;
            newCache.Motion          = info.optionalPointers.Motion;
            newCache.HunterCharacter = info.optionalPointers.HunterCharacter;
            playerInfoCaches[i] = newCache;
            END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Normal Gameplay - Basic Info");
        }

        // -- Visibility --------------------------------------------------------------------------------------------
        BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Normal Gameplay - Visibility");
        fetchPlayer_Visibility(info);
        END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Normal Gameplay - Visibility");

        // Fetch when requested, or if no fetch has been done but the player is in-view.
        if (playersToFetch[i] || (info.visible && persistentPlayerInfos[i] == nullptr)) {
            BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Normal Gameplay - Persistent Info");

            PersistentPlayerInfo persistentInfo{};
            persistentInfo.playerData = info.playerData;
            persistentInfo.index = i;

            bool fetchedPInfo = fetchPlayer_PersistentInfo(i, info, persistentInfo);
            if (fetchedPInfo) {
                playersToFetch[i] = false;
                playerApplyDelays[persistentInfo.playerData] = std::chrono::high_resolution_clock::now();
                persistentPlayerInfos[i] = std::make_unique<PersistentPlayerInfo>(std::move(persistentInfo));
                occupiedNormalGameplaySlots[i] = true;
            }

            END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Normal Gameplay - Persistent Info");
        }

        if (!playerSlotTable.contains(info.playerData)) playerSlotTable.emplace(info.playerData, i);
        playerInfos[i] = std::make_unique<PlayerInfo>(std::move(info));
    }

    PlayerFetchFlags PlayerTracker::fetchPlayer_BasicInfo(size_t i, bool inQuest, bool online, PlayerInfo& out) {
        // app.cPlayerManageInfo
        REApi::ManagedObject* cPlayerManageInfo = REInvokePtr<REApi::ManagedObject>(playerManager, "findPlayer_StableMemberIndex(System.Int32, app.net_session_manager.SESSION_TYPE)", { (void*)i, (void*)1 });
        if (cPlayerManageInfo == nullptr) { clearPlayerSlot(i); return PlayerFetchFlags::FETCH_PLAYER_SLOT_EMPTY; } // Player slot is empty, clear it

        bool includeThisPlayer = false;
        if (inQuest) includeThisPlayer = REInvoke<bool>(playerManager, "isQuestMember(System.Int32)", { (void*)i }, InvokeReturnType::BOOL);
        else         includeThisPlayer = cPlayerManageInfo->is_managed_object();
        if (!includeThisPlayer) { clearPlayerSlot(i); return PlayerFetchFlags::FETCH_PLAYER_SLOT_EMPTY; }

        // Fetch player idenfication data
        REApi::ManagedObject* HunterCharacter      = REInvokePtr<REApi::ManagedObject>(cPlayerManageInfo,    "get_Character",     {}); // app.HunterCharacter
        REApi::ManagedObject* cPlayerContextHolder = REInvokePtr<REApi::ManagedObject>(cPlayerManageInfo,    "get_ContextHolder", {}); // app.cPlayerContextHolder
        REApi::ManagedObject* cPlayerContext       = REInvokePtr<REApi::ManagedObject>(cPlayerContextHolder, "get_Pl",            {}); // app.cPlayerContext
		REApi::ManagedObject* cHunterContext       = REInvokePtr<REApi::ManagedObject>(cPlayerContextHolder, "get_Hunter",        {}); // app.cHunterContext
		REApi::ManagedObject* cHunterCreateInfo    = REInvokePtr<REApi::ManagedObject>(cHunterContext,       "get_CreateInfo",    {}); // app.cHunterCreateInfo

        std::string playerName = REInvokeStr(cPlayerContext, "get_PlayerName", {});
        if (playerName.empty()) {
            DEBUG_STACK.push(std::format("{} Fetched player at index {}, but name returned nullptr, skipping.", PLAYER_TRACKER_LOG_TAG, i), DebugStack::Color::WARNING);
            return PlayerFetchFlags::FETCH_ERROR_NULL;
        }

        int networkIndex = REInvoke<int>(cPlayerContext, "get_CurrentNetworkMemberIndex", {}, InvokeReturnType::DWORD);

        REApi::ManagedObject* playerNetInfo = REInvokePtr<REApi::ManagedObject>(Net_UserInfoList, "getInfoSystem", { (void*)networkIndex }); // How tf does this determine the correct signature between System.Int32 & System.Guid??
        if (playerNetInfo == nullptr) {
            DEBUG_STACK.push(std::format("{} Fetched player at index {}, but playerNetInfo returned nullptr, skipping.", PLAYER_TRACKER_LOG_TAG, i), DebugStack::Color::WARNING);
            return PlayerFetchFlags::FETCH_ERROR_NULL;
        }
        //DEBUG_STACK.push(std::format("playerNetInfo PTR: {} - Properties:\n{}",       ptrToHexString(playerNetInfo),       reObjectPropertiesToString(playerNetInfo)),       DebugStack::Color::DEBUG);

        std::string hunterId = "";
        if (online) {
            hunterId = REInvokeStr(playerNetInfo, "get_ShortHunterId", {});
            if (hunterId.empty()) {
                DEBUG_STACK.push(std::format("{} Fetched player at index {}, but hunterId returned nullptr, skipping.", PLAYER_TRACKER_LOG_TAG, i), DebugStack::Color::WARNING);
                return PlayerFetchFlags::FETCH_ERROR_NULL;
            }
        }
        else {
            // In offline mode, grab the main player's hunter ID from the context manager instead.
			hunterId = REInvokeStr(netContextManager, "get_HunterShortId", {});
            if (hunterId.empty()) {
                DEBUG_STACK.push(std::format("{} Failed to fetch Hunter ID in singleplayer.", PLAYER_TRACKER_LOG_TAG, i), DebugStack::Color::WARNING);
                return PlayerFetchFlags::FETCH_ERROR_NULL;
            }
        }

        bool female      = REInvoke<bool>(HunterCharacter, "get_IsFemale",   {}, InvokeReturnType::BOOL);
		bool weaponDrawn = REInvoke<bool>(HunterCharacter, "get_IsWeaponOn", {}, InvokeReturnType::BOOL);
        bool inCombat    = REInvoke<bool>(HunterCharacter, "get_IsCombat",   {}, InvokeReturnType::BOOL);

        PlayerData playerData{};
        playerData.name = playerName;
        playerData.female = female;
        playerData.hunterId = hunterId;

        REApi::ManagedObject* GameObject = REInvokePtr<REApi::ManagedObject>(cPlayerManageInfo, "get_Object", {}); // via.GameObject
        REApi::ManagedObject* Transform = REInvokePtr<REApi::ManagedObject>(GameObject, "get_Transform", {}); // via.Transform

        static const REApi::ManagedObject* typeof_MotionAnimation = REApi::get()->typeof("via.motion.Animation");
        REApi::ManagedObject* Motion = REInvokePtr<REApi::ManagedObject>(GameObject, "getComponent(System.Type)", { (void*)typeof_MotionAnimation }); // via.motion.Animation

        PlayerPointers pointers{};
        pointers.Transform = Transform;

        PlayerOptionalPointers optPointers{};
        optPointers.cPlayerManageInfo = cPlayerManageInfo;
        optPointers.HunterCharacter   = HunterCharacter;
        optPointers.Motion            = Motion;
		optPointers.cHunterCreateInfo = cHunterCreateInfo;

        out.playerData       = playerData;
        out.index            = i;
        out.pointers         = pointers;
		out.optionalPointers = optPointers;
        out.visible          = false;
        out.inCombat         = inCombat;
        out.weaponDrawn      = weaponDrawn;

        return PlayerFetchFlags::FETCH_SUCCESS;
    }

    void PlayerTracker::fetchPlayer_Visibility(PlayerInfo& info) {
        info.visible = false;
        info.distanceFromCameraSq = FLT_MAX;

        info.weaponDrawn = REInvoke<bool>(info.optionalPointers.HunterCharacter, "get_IsWeaponOn", {}, InvokeReturnType::BOOL);
        info.inCombat    = REInvoke<bool>(info.optionalPointers.HunterCharacter, "get_IsCombat", {}, InvokeReturnType::BOOL);

        const bool motionSkipped = REInvoke<bool>(info.optionalPointers.Motion, "get_SkipUpdate", {}, InvokeReturnType::BOOL);
        if (motionSkipped) return;

        const float& distThreshold = dataManager.settings().applicationRange;
        double sqDist = REInvoke<double>(info.optionalPointers.HunterCharacter, "getCameraDistanceSqXZ", {}, InvokeReturnType::DOUBLE);
        if (distThreshold > 0 && sqDist > distThreshold * distThreshold) return;

        info.distanceFromCameraSq = sqDist;
        info.visible = true;
        return;
    }

    bool PlayerTracker::fetchPlayer_PersistentInfo(size_t i, const PlayerInfo& info, PersistentPlayerInfo& pInfo) {
        if (frameBoneFetchCount != 0 && frameBoneFetchCount >= dataManager.settings().maxBoneFetchesPerFrame) return false;

        BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Normal Gameplay - Equipped Armours");
        bool fetchedArmours = fetchPlayer_EquippedArmours(info, pInfo);
        END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Normal Gameplay - Equipped Armours");
        if (!fetchedArmours) {
            DEBUG_STACK.push(std::format("{} Failed to fetch equipped armours for Player: {} [{}]", PLAYER_TRACKER_LOG_TAG, info.playerData.name, i), DebugStack::Color::WARNING);
            return false;
        }

        BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Normal Gameplay - Armour Transforms");
        bool fetchedTransforms = fetchPlayer_ArmourTransforms(info, pInfo);
        END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Normal Gameplay - Armour Transforms");
        if (!fetchedTransforms) {
            ArmourSet expectedHelm    = pInfo.armourInfo.helm.value_or({ "UNKNOWN!", false });
            ArmourSet expectedBody    = pInfo.armourInfo.body.value_or({ "UNKNOWN!", false });
            ArmourSet expectedArms    = pInfo.armourInfo.arms.value_or({ "UNKNOWN!", false });
            ArmourSet expectedCoil    = pInfo.armourInfo.coil.value_or({ "UNKNOWN!", false });
            ArmourSet expectedLegs    = pInfo.armourInfo.legs.value_or({ "UNKNOWN!", false });
            ArmourSet expectedSlinger = pInfo.armourInfo.slinger.value_or({ "UNKNOWN!", false });

            DEBUG_STACK.push(std::format("{} Failed to fetch armour transforms for Player: {} [{}]."
                "\nExpecting the following transforms:\n   Head:    {} ({})\n   Body:    {} ({})\n   Arms:    {} ({})\n   Waist:   {} ({})\n   Legs:    {} ({})\n   Slinger: {} ({})"
                "\nDumped the following transforms:"
                "\n{}",
                PLAYER_TRACKER_LOG_TAG, info.playerData.name, i,
                ArmourList::getArmourId(expectedHelm, ArmourPiece::AP_HELM, expectedHelm.female),          expectedHelm.name,
                ArmourList::getArmourId(expectedBody, ArmourPiece::AP_BODY, expectedBody.female),          expectedBody.name,
                ArmourList::getArmourId(expectedArms, ArmourPiece::AP_ARMS, expectedArms.female),          expectedArms.name,
                ArmourList::getArmourId(expectedCoil, ArmourPiece::AP_COIL, expectedCoil.female),          expectedCoil.name,
                ArmourList::getArmourId(expectedLegs, ArmourPiece::AP_LEGS, expectedLegs.female),          expectedLegs.name,
                ArmourList::getArmourId(expectedSlinger, ArmourPiece::AP_SLINGER, expectedSlinger.female), expectedSlinger.name,
                dumpTransformTreeString(info.pointers.Transform)
            ), DebugStack::Color::DEBUG);
            return false;
        }

        BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Normal Gameplay - Bones");
        bool fetchedBones = fetchPlayer_Bones(info, pInfo);
        END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Normal Gameplay - Bones");
        if (!fetchedBones) {
            std::string reason = "Unknown";
            if (info.pointers.Transform == nullptr)       reason = "Body ptr was null";
            else if (pInfo.Transform_body == nullptr)     reason = "Body Transform ptr was null";
            else if (pInfo.Transform_legs == nullptr)     reason = "Legs Transform ptr was null";
            else if (!pInfo.armourInfo.body.has_value())  reason = "No body armour found";
            else if (!pInfo.armourInfo.legs.has_value())  reason = "No legs armour found";
            DEBUG_STACK.push(std::format("{} Failed to fetch bones for Player: {} [{}]. Reason: {}.", PLAYER_TRACKER_LOG_TAG, info.playerData.name, i, reason), DebugStack::Color::WARNING);
            return false;
        }

        BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Normal Gameplay - Parts");
        bool fetchedParts = fetchPlayer_Parts(info, pInfo);
        END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Normal Gameplay - Parts");
        if (!fetchedParts) {
            DEBUG_STACK.push(std::format("{} Failed to fetch parts for Player: {} [{}]", PLAYER_TRACKER_LOG_TAG, info.playerData.name, i), DebugStack::Color::WARNING);
            return false;
        }

        BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Normal Gameplay - Weapons");
        bool fetechedWeapons = fetchPlayer_WeaponObjects(pInfo);
        END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Normal Gameplay - Weapons");

        frameBoneFetchCount++; // Consider moving this to top to limit effect of failed fetches - may make fetches inaccessible if there are enough errors though.
        return true;
    }

    bool PlayerTracker::fetchPlayer_EquippedArmours(const PlayerInfo& info, PersistentPlayerInfo& pInfo) {
        if (info.pointers.Transform == nullptr) return false;

        pInfo.armourInfo = ArmourInfo{}; // Reset

        std::array<ArmourSet, 6> foundArmours = findAllArmoursInObjectFromList(info.pointers.Transform, info.playerData.female);
        for (size_t i = 0; foundArmours.size() < 5; i++) {
			if (i == static_cast<int>(ArmourPiece::AP_HELM) - 1) continue; // Helm is optional due to toggle
			if (i == static_cast<int>(ArmourPiece::AP_SLINGER) - 1) continue; // Slinger is also just optional
            if (foundArmours[i] == ArmourList::DefaultArmourSet()) return false;
		}

		pInfo.armourInfo.helm    = foundArmours[static_cast<size_t>(ArmourPiece::AP_HELM)    - 1];
		pInfo.armourInfo.body    = foundArmours[static_cast<size_t>(ArmourPiece::AP_BODY)    - 1];
		pInfo.armourInfo.arms    = foundArmours[static_cast<size_t>(ArmourPiece::AP_ARMS)    - 1];
		pInfo.armourInfo.coil    = foundArmours[static_cast<size_t>(ArmourPiece::AP_COIL)    - 1];
		pInfo.armourInfo.legs    = foundArmours[static_cast<size_t>(ArmourPiece::AP_LEGS)    - 1];
		pInfo.armourInfo.slinger = foundArmours[static_cast<size_t>(ArmourPiece::AP_SLINGER) - 1];

        return true;
    }

    bool PlayerTracker::fetchPlayer_ArmourTransforms(const PlayerInfo& info, PersistentPlayerInfo& pInfo) {
        if (info.pointers.Transform == nullptr) return false;
        // Note: Helm is optional due to toggle
		if (!pInfo.armourInfo.body.has_value()) return false;
		if (!pInfo.armourInfo.arms.has_value()) return false;
		if (!pInfo.armourInfo.coil.has_value()) return false;
		if (!pInfo.armourInfo.legs.has_value()) return false;

        // Base transform is fetched every frame
		pInfo.Transform_base = info.pointers.Transform;

        // TODO: Can probably reduce to 1, 100
        constexpr size_t maxDepth = 1;
        constexpr size_t maxBreadth = 100;

        if (pInfo.armourInfo.helm.has_value()) {
            std::string helmId = ArmourList::getArmourId(pInfo.armourInfo.helm.value(), ArmourPiece::AP_HELM, info.playerData.female);
            pInfo.Transform_helm = findTransform(info.pointers.Transform, helmId, maxDepth, maxBreadth);
        }
        if (pInfo.armourInfo.body.has_value()) {
            std::string bodyId = ArmourList::getArmourId(pInfo.armourInfo.body.value(), ArmourPiece::AP_BODY, info.playerData.female);
			pInfo.Transform_body = findTransform(info.pointers.Transform, bodyId, maxDepth, maxBreadth);
        }
        if (pInfo.armourInfo.arms.has_value()) {
            std::string armsId = ArmourList::getArmourId(pInfo.armourInfo.arms.value(), ArmourPiece::AP_ARMS, info.playerData.female);
			pInfo.Transform_arms = findTransform(info.pointers.Transform, armsId, maxDepth, maxBreadth);
		}
        if (pInfo.armourInfo.coil.has_value()) {
            std::string coilId = ArmourList::getArmourId(pInfo.armourInfo.coil.value(), ArmourPiece::AP_COIL, info.playerData.female);
            pInfo.Transform_coil = findTransform(info.pointers.Transform, coilId, maxDepth, maxBreadth);
        }
        if (pInfo.armourInfo.legs.has_value()) {
            std::string legsId = ArmourList::getArmourId(pInfo.armourInfo.legs.value(), ArmourPiece::AP_LEGS, info.playerData.female);
            pInfo.Transform_legs = findTransform(info.pointers.Transform, legsId, maxDepth, maxBreadth);
        }
        if (pInfo.armourInfo.slinger.has_value()) {
            std::string slingerId = ArmourList::getArmourId(pInfo.armourInfo.slinger.value(), ArmourPiece::AP_SLINGER, info.playerData.female);
            REApi::ManagedObject* slingerTransform = findTransform(info.pointers.Transform, slingerId, maxDepth, maxBreadth);
            pInfo.Slinger_GameObject = (slingerTransform) ? REInvokePtr<REApi::ManagedObject>(slingerTransform, "get_GameObject", {}) : nullptr;
        }

		return (pInfo.Transform_base &&
               pInfo.Transform_body &&
               pInfo.Transform_arms && 
               pInfo.Transform_coil &&
               pInfo.Transform_legs);
    }

    bool PlayerTracker::fetchPlayer_WeaponObjects(PersistentPlayerInfo& pInfo) {
        if (pInfo.Transform_base == nullptr) return false;

        REApi::ManagedObject* Wp_Parent           = findTransform(pInfo.Transform_base, "Wp_Parent");
		REApi::ManagedObject* WpSub_Parent        = findTransform(pInfo.Transform_base, "WpSub_Parent");
		REApi::ManagedObject* Wp_ReserveParent    = findTransform(pInfo.Transform_base, "Wp_ReserveParent");
		REApi::ManagedObject* WpSub_ReserveParent = findTransform(pInfo.Transform_base, "WpSub_ReserveParent");

		if (Wp_Parent == nullptr)           return false;
		if (WpSub_Parent == nullptr)        return false;
		if (Wp_ReserveParent == nullptr)    return false;
		if (WpSub_ReserveParent == nullptr) return false;

		pInfo.Wp_Parent_GameObject           = REInvokePtr<REApi::ManagedObject>(Wp_Parent          , "get_GameObject", {});
		pInfo.WpSub_Parent_GameObject        = REInvokePtr<REApi::ManagedObject>(WpSub_Parent       , "get_GameObject", {});
		pInfo.Wp_ReserveParent_GameObject    = REInvokePtr<REApi::ManagedObject>(Wp_ReserveParent   , "get_GameObject", {});
		pInfo.WpSub_ReserveParent_GameObject = REInvokePtr<REApi::ManagedObject>(WpSub_ReserveParent, "get_GameObject", {});

		return (pInfo.Wp_Parent_GameObject && pInfo.WpSub_Parent_GameObject && pInfo.Wp_ReserveParent_GameObject && pInfo.WpSub_ReserveParent_GameObject);
    }

    bool PlayerTracker::fetchPlayer_Bones(const PlayerInfo& info, PersistentPlayerInfo& pInfo) {
		if (info.pointers.Transform == nullptr) return false;
        if (!pInfo.Transform_body) return false;
		if (!pInfo.Transform_legs) return false;

        pInfo.boneManager = std::make_unique<BoneManager>(
            dataManager, 
            pInfo.armourInfo, 
            pInfo.Transform_base,
            pInfo.Transform_helm,
            pInfo.Transform_body, 
            pInfo.Transform_arms,
			pInfo.Transform_coil,
            pInfo.Transform_legs, 
            info.playerData.female);

        return pInfo.boneManager->isInitialized();
    }

    bool PlayerTracker::fetchPlayer_Parts(const PlayerInfo& info, PersistentPlayerInfo& pInfo) {
        if (info.pointers.Transform == nullptr) return false;
        if (!pInfo.Transform_body) return false;
        if (!pInfo.Transform_legs) return false;

        pInfo.partManager = std::make_unique<PartManager>(
            dataManager, 
            pInfo.armourInfo,
            pInfo.Transform_base,
            pInfo.Transform_helm,
            pInfo.Transform_body,
            pInfo.Transform_arms,
            pInfo.Transform_coil,
            pInfo.Transform_legs,
            info.playerData.female);

        return pInfo.partManager->isInitialized();
    }

    bool PlayerTracker::getSavePlayerData(int saveIdx, PlayerData& out) const {
		if (saveIdx < 0 || saveIdx >= 3) return false; // Invalid save index

		out = PlayerData{};

		REApi::ManagedObject* currentSaveData = REInvokePtr<REApi::ManagedObject>(saveDataManager, "getUserSaveData(System.Int32)", { (void*)saveIdx });
		if (currentSaveData == nullptr) return false;

        char* activeByte = re_memory_ptr<char>(currentSaveData, 0x3AC);
        if (activeByte == nullptr || *activeByte == 0) return false;

		REApi::ManagedObject* cBasicParam = REInvokePtr<REApi::ManagedObject>(currentSaveData, "get_BasicData", {});
		if (cBasicParam == nullptr) return false;

		REApi::ManagedObject* cCharacterEdit_Hunter = REInvokePtr<REApi::ManagedObject>(currentSaveData, "get_CharacterEdit_Hunter", {});
		if (cCharacterEdit_Hunter == nullptr) return false;

		std::string playerName = REFieldStr(cBasicParam, "CharName", REStringType::SYSTEM_STRING, false);
		if (playerName.empty()) return false;

		std::string hunterShortId = REFieldStr(currentSaveData, "HunterShortId", REStringType::SYSTEM_STRING, false);
        if (hunterShortId.empty()) return false;

		int* genderIdentity = REFieldPtr<int>(cCharacterEdit_Hunter, "GenderIdentity", true);
        if (genderIdentity == nullptr) return false;
        bool female = *genderIdentity == 1;

        out.name     = playerName;
        out.hunterId = hunterShortId;
        out.female   = female;

        return true;
    }

    bool PlayerTracker::getActiveSavePlayerData(PlayerData& out) const {
        out = PlayerData{};

        REApi::ManagedObject* currentSaveData = REInvokePtr<REApi::ManagedObject>(saveDataManager, "getCurrentUserSaveData", {});
        if (currentSaveData == nullptr) return false;

        char* activeByte = re_memory_ptr<char>(currentSaveData, 0x3AC);
        if (activeByte == nullptr || *activeByte == 0) return false;

        REApi::ManagedObject* cBasicParam = REInvokePtr<REApi::ManagedObject>(currentSaveData, "get_BasicData", {});
        if (cBasicParam == nullptr) return false;

        REApi::ManagedObject* cCharacterEdit_Hunter = REInvokePtr<REApi::ManagedObject>(currentSaveData, "get_CharacterEdit_Hunter", {});
        if (cCharacterEdit_Hunter == nullptr) return false;

        std::string playerName = REFieldStr(cBasicParam, "CharName", REStringType::SYSTEM_STRING, false);
        if (playerName.empty()) return false;

        std::string hunterShortId = REFieldStr(currentSaveData, "HunterShortId", REStringType::SYSTEM_STRING, false);
        if (hunterShortId.empty()) return false;

        int* genderIdentity = REFieldPtr<int>(cCharacterEdit_Hunter, "GenderIdentity", true);
        if (genderIdentity == nullptr) return false;
        bool female = *genderIdentity == 1;

        out.name = playerName;
        out.hunterId = hunterShortId;
        out.female = female;

        return true;
    }

    REApi::ManagedObject* PlayerTracker::getCurrentScene() const {
        static auto sceneManagerTypeDefinition = REApi::get()->tdb()->find_type("via.SceneManager");
        static auto getCurrentSceneMethodDefinition = sceneManagerTypeDefinition->find_method("get_CurrentScene");

        REApi::ManagedObject* scene = getCurrentSceneMethodDefinition->call<REApi::ManagedObject*>(REApi::get()->get_vm_context(), sceneManager);
        
        return scene;
    }

    void PlayerTracker::updateApplyDelays() {
        std::vector<PlayerData> timestampsExpired{};
        std::chrono::steady_clock::time_point now = std::chrono::high_resolution_clock::now();

        for (const auto& [player, optTimestamp] : playerApplyDelays) {
            if (!optTimestamp.has_value()) continue;
            
            double durationMs = std::chrono::duration<double, std::milli>(now - optTimestamp.value()).count();
            if (durationMs >= dataManager.settings().delayOnEquip * 1000.0f) {
                playerApplyDelays[player] = std::nullopt;
            }
        }
    }

    int PlayerTracker::onIsEquipBuildEndHook(int argc, void** argv, REFrameworkTypeDefinitionHandle* arg_tys, unsigned long long ret_addr) {
        if (g_instance) return g_instance->onIsEquipBuildEnd(argc, argv, arg_tys, ret_addr);
        return REFRAMEWORK_HOOK_CALL_ORIGINAL;
    }

    int PlayerTracker::onIsEquipBuildEnd(int argc, void** argv, REFrameworkTypeDefinitionHandle* arg_tys, unsigned long long ret_addr) {
        if (argc < 2) return REFRAMEWORK_HOOK_CALL_ORIGINAL;
        return detectPlayer(argv[1], "Changed Equipment");
    }

    int PlayerTracker::onWarpHook(int argc, void** argv, REFrameworkTypeDefinitionHandle* arg_tys, unsigned long long ret_addr) {
        if (g_instance) return g_instance->onWarp(argc, argv, arg_tys, ret_addr);
        return REFRAMEWORK_HOOK_CALL_ORIGINAL;
    }

    int PlayerTracker::onWarp(int argc, void** argv, REFrameworkTypeDefinitionHandle* arg_tys, unsigned long long ret_addr) {
        if (argc < 2) return REFRAMEWORK_HOOK_CALL_ORIGINAL;
        return detectPlayer(argv[1], "Warped");
    }

    int PlayerTracker::saveSelectListSelectHook(int argc, void** argv, REFrameworkTypeDefinitionHandle* arg_tys, unsigned long long ret_addr) {
		if (argc < 5) return REFRAMEWORK_HOOK_CALL_ORIGINAL;
        if (g_instance) {
            uint32_t selectedSaveIdx = reinterpret_cast<uint32_t>(argv[4]);
            if (g_instance->lastSelectedSaveIdx != selectedSaveIdx) {
			    g_instance->lastSelectedSaveIdx = selectedSaveIdx;
                g_instance->reset();
                //DEBUG_STACK.push(std::format("Save select state change detected | Idx = {}", selectedSaveIdx));
            }
        }
        return REFRAMEWORK_HOOK_CALL_ORIGINAL;
	}

    int PlayerTracker::detectPlayer(void* hunterCharacterPtr, const std::string& logStrSuffix) {
        bool inQuest = SituationWatcher::inSituation(isinQuestPlayingasGuest)
            || SituationWatcher::inSituation(isinQuestPlayingasHost);
        if (dataManager.settings().enableDuringQuestsOnly && !inQuest) return REFRAMEWORK_HOOK_CALL_ORIGINAL;

        REApi::ManagedObject* app_HunterCharacter = reinterpret_cast<REApi::ManagedObject*>(hunterCharacterPtr);
        if (app_HunterCharacter == nullptr) return REFRAMEWORK_HOOK_CALL_ORIGINAL;

        int idx = REInvoke<int>(app_HunterCharacter, "get_StableMemberIndex", {}, InvokeReturnType::DWORD);
        if (idx < 0 || idx >= PLAYER_LIST_SIZE) return REFRAMEWORK_HOOK_CALL_ORIGINAL;

        playersToFetch[static_cast<size_t>(idx)] = true;

        return REFRAMEWORK_HOOK_CALL_ORIGINAL;
    }

    void PlayerTracker::clearPlayerSlot(size_t index) {
        if (index >= playerInfos.size()) return;
        if (playerInfos[index]) {
            playerSlotTable.erase(playerInfos[index]->playerData);
            occupiedNormalGameplaySlots[index] = false;
            playerInfos[index] = nullptr;
			persistentPlayerInfos[index] = nullptr;
        }
	}

}