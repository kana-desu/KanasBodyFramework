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
#include <kbf/util/string/ptr_to_hex_string.hpp>
#include <kbf/util/re_engine/get_component.hpp>
#include <kbf/enums/armor_parts.hpp>
#include <kbf/data/armour/armor_set_id.hpp>
#include <kbf/data/armour/armour_data_manager.hpp>
#include <kbf/debug/debug_stack.hpp>

#include <kbf/util/re_engine/guid_to_string.hpp>
#include <kbf/profiling/cpu_profiler.hpp>

#define PLAYER_TRACKER_LOG_TAG "[PlayerTracker]"

using REApi = reframework::API;

namespace kbf {

    PlayerTracker* PlayerTracker::g_instance = nullptr;

    void PlayerTracker::initialize() {
        g_instance = this;

        auto& api = REApi::get();

        netContextManager = REInvokePtr<REApi::ManagedObject>(networkManager.get(), "get_ContextManager", {});
        assert(netContextManager != nullptr && "Could not get netContextManager!");
        netUserInfoManager = REInvokePtr<REApi::ManagedObject>(networkManager.get(), "get_UserInfoManager", {});
        assert(netUserInfoManager != nullptr && "Could not get networkManager!");
        Net_UserInfoList = REInvokePtr<REApi::ManagedObject>(netUserInfoManager, "getUserInfoList(app.net_session_manager.SESSION_TYPE)", { (void*)1 });
        assert(Net_UserInfoList != nullptr && "Could not get Net_UserInfoList!");

        kbf::HookManager::add_tdb("app.HunterCharacter", "isEquipBuildEnd", onIsEquipBuildEndHook, nullptr, false);
        kbf::HookManager::add_tdb("app.HunterCharacter", "warp",            onWarpHook,            nullptr, false);

        kbf::HookManager::add_tdb("app.GUI010102", "callback_ListSelect", saveSelectListSelectHook, nullptr, false);

        // Fetch everything again after leaving these areas as lists will be cleared.
        SituationWatcher::get().onLeaveSituation(CustomSituation::isInHunterGuildCard, [this]() { needsAllPlayerFetch = true; });
        SituationWatcher::get().onLeaveSituation(CustomSituation::isInCharacterCreator, [this]() { needsAllPlayerFetch = true; });

        setupLists();
    }

    void PlayerTracker::setupLists() {
        bool gotNumPlayers = getPlayerListSize(playerListSize);
        if (!gotNumPlayers) {
            DEBUG_STACK.fpush<PLAYER_TRACKER_LOG_TAG>(DebugStack::Color::COL_ERROR, "Failed to get player list from Player Manager! Player modifications will not function.");
        }
        else {
            DEBUG_STACK.fpush<PLAYER_TRACKER_LOG_TAG>(DebugStack::Color::COL_SUCCESS, "Successfully fetched player list size: {}", playerListSize);
            
            // empty initialize arrays
			playersToFetch             .resize(playerListSize, false);
			occupiedNormalGameplaySlots.resize(playerListSize, false);
			playerInfos                .resize(playerListSize, std::nullopt);
			persistentPlayerInfos      .resize(playerListSize, std::nullopt);
			playerInfoCaches           .resize(playerListSize, std::nullopt);
        }
	}

    bool PlayerTracker::getPlayerListSize(size_t& out) {
        REApi::ManagedObject* list = REFieldPtr<REApi::ManagedObject>(playerManager.get(), "_PlayerList");
        if (!list) return false;

        REApi::ManagedObject* enumerator = REInvokePtr<REApi::ManagedObject>(list, "GetEnumerator()", {});
        if (!enumerator) return false;

        constexpr size_t fetchCap = 2000; // Arbitrary cap to prevent infinite loops in case of something going wrong with the enumerator
        bool canMove = REInvoke<bool>(enumerator, "MoveNext()", {}, InvokeReturnType::BOOL);
        size_t cnt = 0;
        while (canMove&& cnt < fetchCap) {
            cnt++;
            canMove = REInvoke<bool>(enumerator, "MoveNext()", {}, InvokeReturnType::BOOL);
        }

        out = cnt;
        return cnt > 0;
	}

    const std::vector<PlayerData> PlayerTracker::getPlayerList() const {
        std::vector<PlayerData> playerDataList;
        for (const auto& info : playerInfos) {
            if (info.has_value()) playerDataList.push_back(info->playerData);
        }
        return playerDataList;
    }

    void PlayerTracker::updatePlayers() {
        fetchPlayers();
        updateApplyDelays();
    }

    void PlayerTracker::applyPresets() {
		constexpr auto& profiler = CpuProfiler::GlobalMultiScopeProfiler;
        constexpr const char* BLOCK_PRECOMPUTE      = "Player Apply - Precompute & Sort";
        constexpr const char* BLOCK_INFO_VALIDATION = "Player Apply - Info Validation";
        constexpr const char* BLOCK_APPLY_BONES     = "Player Apply - Apply Bones";
        constexpr const char* BLOCK_APPLY_PARTS     = "Player Apply - Apply Parts";
        constexpr const char* BLOCK_APPLY_MATS      = "Player Apply - Apply Materials";
        constexpr const char* BLOCK_WEAPON_VIS      = "Player Apply - Weapon Visibility";
        constexpr const char* BLOCK_SLINGER_VIS     = "Player Apply - Slinger Visibility";

        bool inQuest = SituationWatcher::inSituation(isinQuestPlayingasGuest) || SituationWatcher::inSituation(isinQuestPlayingasHost);
        if (dataManager.settings().enableDuringQuestsOnly && !inQuest) return;

        // ==== PRECOMPUTE ==================================================================================================
        BEGIN_CPU_PROFILING_BLOCK(profiler, BLOCK_PRECOMPUTE);
        // Additionally consider one extra 'preview preset' for those currently being edited in the GUI
        const Preset* previewedPreset = dataManager.getPreviewedPreset();
        const bool hasPreview = previewedPreset != nullptr;
        const bool applyPreviewUnconditional = hasPreview && previewedPreset->armour == ArmourSet::DEFAULT;

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
                    if (!playerInfos[a.second]) return false;
                    if (!playerInfos[b.second]) return true;
                    return playerInfos[a.second]->distanceFromCameraSq < playerInfos[b.second]->distanceFromCameraSq;
                }
            );
        }

        size_t limit = maxPlayersToApply > 0 ? std::min<size_t>(maxPlayersToApply, players.size()) : players.size();
        END_CPU_PROFILING_BLOCK(profiler, BLOCK_PRECOMPUTE);
        // ==================================================================================================================

        for (size_t i = 0; i < limit; ++i) {
			BEGIN_CPU_PROFILING_BLOCK(profiler, BLOCK_INFO_VALIDATION);
            const PlayerData& player = *players[i].first;
            size_t idx = players[i].second;

            if (!playerInfos[idx].has_value())                                      PROFILED_FLOW_OP(profiler, BLOCK_INFO_VALIDATION, continue);
            if (playerApplyDelays[player] && playerApplyDelays[player].has_value()) PROFILED_FLOW_OP(profiler, BLOCK_INFO_VALIDATION, continue);

            if (!playerInfos[idx].has_value()) {
                playerApplyDelays.erase(player);
                PROFILED_FLOW_OP(profiler, BLOCK_INFO_VALIDATION, continue);
            }

            const PlayerInfo& info = *playerInfos[idx];
            if (!info.visible) PROFILED_FLOW_OP(profiler, BLOCK_INFO_VALIDATION, continue);

            if (!persistentPlayerInfos[idx].has_value()) PROFILED_FLOW_OP(profiler, BLOCK_INFO_VALIDATION, continue);

			PersistentPlayerInfo& pInfo = persistentPlayerInfos[idx].value();
            if (!persistentPlayerInfos[idx].value().areSetPointersValid()) {
                persistentPlayerInfos[idx] = std::nullopt;
                PROFILED_FLOW_OP(profiler, BLOCK_INFO_VALIDATION, continue);
            }
			END_CPU_PROFILING_BLOCK(profiler, BLOCK_INFO_VALIDATION);

            if (pInfo.boneManager && pInfo.partManager) {

                // Always apply base presets when they are present, but refrain from re-applying the same base preset multiple times.
                std::unordered_set<std::string> presetBasesApplied{};

                bool hideWeapon = false;
                bool hideSlinger = false;

                bool applyError = false;
                for (ArmourPiece piece = ArmourPiece::AP_MIN_EXCLUDING_SET; piece <= ArmourPiece::AP_MAX_EXCLUDING_SLINGER; piece = static_cast<ArmourPiece>(static_cast<int>(piece) + 1)) {
                    std::optional<ArmourSet>& armourPiece = pInfo.armourInfo.getPiece(piece);

                    if (armourPiece.has_value()) {
                        const Preset* preset = dataManager.getActivePreset(player, armourPiece.value(), piece);

                        bool usePreview = hasPreview && (applyPreviewUnconditional || previewedPreset->armour == armourPiece.value());
                        if (preset == nullptr && !usePreview) continue;

                        const Preset* activePreset = usePreview ? previewedPreset : preset;
                        const Preset* setWidePartsPreset = usePreview ? nullptr : dataManager.getActivePreset(player, armourPiece.value(), ArmourPiece::CUSTOM_AP_PARTS);
                        const Preset* setWideMatsPreset  = usePreview ? nullptr : dataManager.getActivePreset(player, armourPiece.value(), ArmourPiece::CUSTOM_AP_MATS);

						BEGIN_CPU_PROFILING_BLOCK(profiler, BLOCK_APPLY_BONES);
                        BoneManager::BoneApplyStatusFlag applyFlag = pInfo.boneManager->applyPreset(activePreset, piece);
                        bool invalidBones = applyFlag == BoneManager::BoneApplyStatusFlag::BONE_APPLY_ERROR_INVALID_BONE;
                        if (invalidBones) { 
                            applyError = true; 
                            clearPlayerSlot(idx); 
                            playersToFetch[idx] = true;
                            PROFILED_FLOW_OP(profiler, BLOCK_APPLY_BONES, break);
                        }
						END_CPU_PROFILING_BLOCK(profiler, BLOCK_APPLY_BONES);

						BEGIN_CPU_PROFILING_BLOCK(profiler, BLOCK_APPLY_PARTS);
                        pInfo.partManager->applyPreset(setWidePartsPreset, piece); // Apply set-wide part overrides first
                        pInfo.partManager->applyPreset(activePreset, piece);
						END_CPU_PROFILING_BLOCK(profiler, BLOCK_APPLY_PARTS);

						BEGIN_CPU_PROFILING_BLOCK(profiler, BLOCK_APPLY_MATS);
                        pInfo.materialManager->applyPreset(setWideMatsPreset, piece); // Apply set-wide material overrides first
						pInfo.materialManager->applyPreset(activePreset, piece);
						END_CPU_PROFILING_BLOCK(profiler, BLOCK_APPLY_MATS);

                        if (!invalidBones && activePreset->set.hasModifiers() && !presetBasesApplied.contains(activePreset->uuid)) {
                            BEGIN_CPU_PROFILING_BLOCK(profiler, BLOCK_APPLY_BONES);
                            presetBasesApplied.insert(activePreset->uuid);
                            BoneManager::BoneApplyStatusFlag baseApplyFlag = pInfo.boneManager->applyPreset(activePreset, AP_SET);
                            bool invalidBaseBones = baseApplyFlag == BoneManager::BoneApplyStatusFlag::BONE_APPLY_ERROR_INVALID_BONE;
                            if (invalidBaseBones) { 
                                applyError = true; 
                                clearPlayerSlot(idx); 
                                playersToFetch[idx] = true; 
                                PROFILED_FLOW_OP(profiler, BLOCK_APPLY_BONES, break);
                            }
                            END_CPU_PROFILING_BLOCK(profiler, BLOCK_APPLY_BONES);
                        }

                        // Check Weapon & Slinger Disables
                        bool setWideWantsToHideWeapon  = setWidePartsPreset ? setWidePartsPreset->hideWeapon : false;
                        bool setWideWantsToHideSlinger = setWidePartsPreset ? setWidePartsPreset->hideSlinger : false;
                        hideWeapon  |= setWideWantsToHideWeapon  | activePreset->hideWeapon;
                        hideSlinger |= setWideWantsToHideSlinger | activePreset->hideSlinger;
                    }
                }

                if (!applyError) {
					BEGIN_CPU_PROFILING_BLOCK(profiler, BLOCK_WEAPON_VIS);
                    // Weapon Visibility
                    if (dataManager.settings().enableHideWeapons) {

                        bool weaponVisible = info.weaponDrawn || !hideWeapon 
                            || (info.inCombat && dataManager.settings().hideWeaponsOutsideOfCombatOnly)
                            || (info.inTent && dataManager.settings().forceShowWeaponInTent)
                            || (info.isRidingSeikret && dataManager.settings().forceShowWeaponWhenOnSeikret)
                            || (info.isSharpening && dataManager.settings().forceShowWeaponWhenSharpening);

                        if (pInfo.Wp_Parent_GameObject)           REInvokeVoid(pInfo.Wp_Parent_GameObject,           "set_DrawSelf", { (void*)(weaponVisible) });
                        if (pInfo.WpSub_Parent_GameObject)        REInvokeVoid(pInfo.WpSub_Parent_GameObject,        "set_DrawSelf", { (void*)(weaponVisible) });
                        if (pInfo.Wp_ReserveParent_GameObject)    REInvokeVoid(pInfo.Wp_ReserveParent_GameObject,    "set_DrawSelf", { (void*)(weaponVisible) });
                        if (pInfo.WpSub_ReserveParent_GameObject) REInvokeVoid(pInfo.WpSub_ReserveParent_GameObject, "set_DrawSelf", { (void*)(weaponVisible) });
                    
                        bool kinsectVisible = !dataManager.settings().enableHideKinsect || weaponVisible;

                        const static reframework::API::TypeDefinition* def_GameObject = reframework::API::get()->tdb()->find_type("via.GameObject");
                        bool validWpInsect        = pInfo.Wp_Insect        && checkREPtrValidity(pInfo.Wp_Insect,        def_GameObject);
						bool validWpReserveInsect = pInfo.Wp_ReserveInsect && checkREPtrValidity(pInfo.Wp_ReserveInsect, def_GameObject);

                        if (validWpInsect)        REInvokeVoid(pInfo.Wp_Insect,        "set_DrawSelf", { (void*)(kinsectVisible) });
                        if (validWpReserveInsect) REInvokeVoid(pInfo.Wp_ReserveInsect, "set_DrawSelf", { (void*)(kinsectVisible) });
                    }
					END_CPU_PROFILING_BLOCK(profiler, BLOCK_WEAPON_VIS);

					BEGIN_CPU_PROFILING_BLOCK(profiler, BLOCK_SLINGER_VIS);
                    // Slinger Visibility
                    bool slingerVisible = !hideSlinger || (info.inCombat && dataManager.settings().hideSlingerOutsideOfCombatOnly);
                    if (pInfo.Slinger_GameObject) REInvokeVoid(pInfo.Slinger_GameObject, "set_DrawSelf", { (void*)(slingerVisible) });
					END_CPU_PROFILING_BLOCK(profiler, BLOCK_SLINGER_VIS);
                }
            }
        }
    }

    void PlayerTracker::reset() {
        playerSlotTable.clear();
        playerApplyDelays.clear();
        for (auto& p : playerInfos)                 p.reset();
        for (auto& p : persistentPlayerInfos)       p.reset();
        
		std::fill(playersToFetch.begin(), playersToFetch.end(), false);
		std::fill(occupiedNormalGameplaySlots.begin(), occupiedNormalGameplaySlots.end(), false);

        saveSelectHunterTransformCache              = nullptr;
        saveSelectSceneControllerCache              = nullptr;
        saveSelectHashedArmourTransformsCache       = std::nullopt;
        characterCreatorHunterTransformCache        = nullptr;
        charaMakeSceneControllerCache               = nullptr;
        characterCreatorHashedArmourTransformsCache = std::nullopt;
        guildCardHunterTransformCache               = nullptr;
        guildCardHunterGameObjCache                 = nullptr;
        guildCardSceneControllerCache               = nullptr;
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

        // Try refetch once after cutscene ends/begins to avoid being untracked.
        needsAllPlayerFetch |= (frameIsCutscene && !cutscene) || (!frameIsCutscene && cutscene); 
        frameIsCutscene = cutscene;

        needsAllPlayerFetch |= (frameIsGuildCard && !guildCard) || (!frameIsGuildCard && guildCard);
        frameIsGuildCard = guildCard;

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
        bool fetchedArmours = fetchPlayer_EquippedArmours_FromSaveFile(info, persistentInfo);
        BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Main Menu - Equipped Armours");

        if (!fetchedArmours) {
            DEBUG_STACK.push(std::format("{} Failed to fetch equipped armours for Main Menu Hunter: {} [{}]", PLAYER_TRACKER_LOG_TAG, info.playerData.name, info.playerData.hunterId), DebugStack::Color::COL_WARNING);
            return; // We terminate this whole fetch in this case, but can probably just try this portion again instead - ...Too bad!
        }

        END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Main Menu - Equipped Armours");
        //--------------------------------------------------

        //- Armour Transforms ------------------------------
        BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Main Menu - Armour Transforms");

        bool fetchedTransforms = fetchPlayer_ArmourTransforms_FromEventModel(info, persistentInfo);
        if (!fetchedTransforms) {
            DEBUG_STACK.push(std::format(
                "{} Failed to fetch armour transforms for Main Menu Hunter: {} [{}]. Relevant info:\n"
                "  Base @ {}\n  Helm: {} @ {}\n  Body: {} @ {}\n  Arms: {} @ {}\n  Coil: {} @ {}\n  Legs: {} @ {}",
                PLAYER_TRACKER_LOG_TAG, info.playerData.name, info.playerData.hunterId,
                ptrToHexString(persistentInfo.Transform_base),
                persistentInfo.armourInfo.helm.has_value() ? persistentInfo.armourInfo.helm.value().name : "NULL", ptrToHexString(persistentInfo.Transform_helm),
                persistentInfo.armourInfo.body.has_value() ? persistentInfo.armourInfo.body.value().name : "NULL", ptrToHexString(persistentInfo.Transform_body),
                persistentInfo.armourInfo.arms.has_value() ? persistentInfo.armourInfo.arms.value().name : "NULL", ptrToHexString(persistentInfo.Transform_arms),
                persistentInfo.armourInfo.coil.has_value() ? persistentInfo.armourInfo.coil.value().name : "NULL", ptrToHexString(persistentInfo.Transform_coil),
                persistentInfo.armourInfo.legs.has_value() ? persistentInfo.armourInfo.legs.value().name : "NULL", ptrToHexString(persistentInfo.Transform_legs)
            ), DebugStack::Color::COL_WARNING);
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
            DEBUG_STACK.push(std::format("{} Failed to fetch bones for Main Menu Hunter: {} [{}]. Reason: {}.", PLAYER_TRACKER_LOG_TAG, info.playerData.name, info.playerData.hunterId, reason), DebugStack::Color::COL_WARNING);
            return;
        }

        END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Main Menu - Bones");
        //--------------------------------------------------

        //- Parts ------------------------------------------
        BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Main Menu - Parts");

        bool fetchedParts = fetchPlayer_Parts(info, persistentInfo);
        if (!fetchedParts) {
            DEBUG_STACK.push(std::format("{} Failed to fetch parts for Main Menu Hunter: {} [{}]", PLAYER_TRACKER_LOG_TAG, info.playerData.name, info.playerData.hunterId), DebugStack::Color::COL_WARNING);
            return;
        }

        END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Main Menu - Parts");
        //--------------------------------------------------

        // - Materials -------------------------------------
        BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Guild Card - Materials");

        bool fetchedMaterials = fetchPlayer_Materials(info, persistentInfo);
        if (!fetchedMaterials) {
            DEBUG_STACK.push(std::format("{} Failed to fetch materials for Main Menu Hunter: {} [{}]", PLAYER_TRACKER_LOG_TAG, info.playerData.name, info.playerData.hunterId), DebugStack::Color::COL_WARNING);
            return;
        }

        END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Guild Card - Materials");
        //--------------------------------------------------

        //- Weapon Objects ---------------------------------
        BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Main Menu - Weapon Objects");

        bool fetchedWeapons = fetchPlayers_MainMenu_WeaponObjects(info, persistentInfo);
        if (!fetchedWeapons) {
            DEBUG_STACK.push(std::format("{} Failed to fetch weapon objects for Main Menu Hunter: {} [{}]", PLAYER_TRACKER_LOG_TAG, info.playerData.name, info.playerData.hunterId), DebugStack::Color::COL_WARNING);
            return;
        }

        END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Main Menu - Weapon Objects");
        //--------------------------------------------------

        playerApplyDelays[persistentInfo.playerData] = std::chrono::high_resolution_clock::now();
        persistentPlayerInfos[0] = std::move(persistentInfo);

        playerSlotTable.emplace(info.playerData, 0);
        playerInfos[0] = std::move(info);
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
        REApi::ManagedObject* mcPreviewHunterVisualController = REFieldPtr<REApi::ManagedObject>(outInfo.optionalPointers.EventModelSetupper, "_HunterVisualController");
        if (mcPreviewHunterVisualController == nullptr) return false;

        int32_t* equipAppearanceSaveIndex = REFieldPtr<int32_t>(mcPreviewHunterVisualController, "_EquipAppearanceSaveIndex");
        if (equipAppearanceSaveIndex == nullptr) return false;

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

        outPInfo.Wp_Parent_GameObject    = REFieldPtr<REApi::ManagedObject>(info.optionalPointers.EventModelSetupper, "_WeaponObj"        );
        outPInfo.WpSub_Parent_GameObject = REFieldPtr<REApi::ManagedObject>(info.optionalPointers.EventModelSetupper, "_WeaponSubObj"     );
        outPInfo.Wp_Insect               = REFieldPtr<REApi::ManagedObject>(info.optionalPointers.EventModelSetupper, "_WeaponExternalObj");

        return (outPInfo.Wp_Parent_GameObject || outPInfo.WpSub_Parent_GameObject);
    }

    void PlayerTracker::fetchPlayers_SaveSelect() {
        //- Basic Info -------------------------------------
        BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Save Select - Basic Info");

        PlayerInfo info{};
        bool fetchedBasicInfo = fetchPlayers_SaveSelect_BasicInfo(info);
        if (!fetchedBasicInfo) {
            saveSelectHunterTransformCache = nullptr;
            saveSelectSceneControllerCache = nullptr;
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

        bool fetchedArmours = fetchPlayer_EquippedArmours_FromSaveFile(info, persistentInfo, lastSelectedSaveIdx);
        if (!fetchedArmours) {
            return;
        }

        END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Save Select - Equipped Armours");
        //--------------------------------------------------

        //- Armour Transforms ------------------------------
        BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Save Select - Armour Transforms");

        bool fetchedTransforms = fetchPlayer_ArmourTransforms_FromSaveSelectSceneController(info.optionalPointers.SaveSelectSceneController, info, persistentInfo);
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
                DEBUG_STACK.push(std::format("{} Failed to fetch bones for Save Select Hunter: {} [{}]. Reason: {}.", PLAYER_TRACKER_LOG_TAG, info.playerData.name, info.playerData.hunterId, reason), DebugStack::Color::COL_WARNING);
                return;
            }

            END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Save Select - Bones");
            //--------------------------------------------------

            //- Parts ------------------------------------------
            BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Save Select - Parts");

            bool fetchedParts = fetchPlayer_Parts(info, persistentInfo);
            if (!fetchedParts) {
                DEBUG_STACK.push(std::format("{} Failed to fetch parts for Save Select Hunter: {} [{}]", PLAYER_TRACKER_LOG_TAG, info.playerData.name, info.playerData.hunterId), DebugStack::Color::COL_WARNING);
                return;
            }

            END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Save Select - Parts");
            //--------------------------------------------------

            // - Materials -------------------------------------
            BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Guild Card - Materials");

            bool fetchedMaterials = fetchPlayer_Materials(info, persistentInfo);
            if (!fetchedMaterials) {
                DEBUG_STACK.push(std::format("{} Failed to fetch materials for Save Select Hunter: {} [{}]", PLAYER_TRACKER_LOG_TAG, info.playerData.name, info.playerData.hunterId), DebugStack::Color::COL_WARNING);
                return;
            }

            END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Guild Card - Materials");
            //--------------------------------------------------

            //- Weapon Objects ------------------------------e---
            BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Save Select - Weapon Objects");

            bool fetchedWeapons = fetchPlayers_SaveSelect_WeaponObjects(info, persistentInfo);
            if (!fetchedWeapons) {
                DEBUG_STACK.push(std::format("{} Failed to fetch weapon objects for Save Select Hunter: {} [{}]", PLAYER_TRACKER_LOG_TAG, info.playerData.name, info.playerData.hunterId), DebugStack::Color::COL_WARNING);
                return;
            }

            END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Save Select - Weapon Objects");
            //--------------------------------------------------

            playerApplyDelays[persistentInfo.playerData] = std::chrono::high_resolution_clock::now();
            persistentPlayerInfos[0] = std::move(persistentInfo);
        }

        playerSlotTable.emplace(info.playerData, 0);
        playerInfos[0] = std::move(info);
    }

    bool PlayerTracker::fetchPlayers_SaveSelect_BasicInfo(PlayerInfo& outInfo) {
        PlayerData hunter{};
        if (!getSavePlayerData(lastSelectedSaveIdx, hunter))
            return false;

        outInfo = PlayerInfo{};

        if (!resolveHunterAndController(
            outInfo,
            hunter,
            outInfo.optionalPointers.SaveSelectSceneController,
            saveSelectHunterTransformCache,
            saveSelectSceneControllerCache,
            "SaveSelect_HunterXX",
            "SaveSelect_HunterXY",
            "SaveSelectSceneController",
            "app.SaveSelectSceneController"
        )) return false;

        int* visibilityPtr = re_memory_ptr<int>(outInfo.optionalPointers.VolumeOccludee, 0x2C9);

        outInfo.playerData = hunter;
        outInfo.index = 0;
        outInfo.visible = true;

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

        constexpr const char* itStrPrefix = "it";
        constexpr const char* weaponKinsectStr = "it1003_";
        const int numComponents = REInvoke<int>(transformComponents, "GetLength", { (void*)0 }, InvokeReturnType::DWORD);

        bool foundMainWp = false;
        bool foundSubWp  = false;
        bool foundKinsect = false;
        for (int i = 0; i < numComponents; i++) {
            // TODO: Best to find out if wep is insect glaive so this exit condition can still be used...
            if (foundMainWp && foundSubWp && foundKinsect) break;

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
            else if (name.starts_with(itStrPrefix)) {
                if (!foundKinsect && name.starts_with(weaponKinsectStr)) {
                    REApi::ManagedObject* Wp10Insect = REInvokePtr<REApi::ManagedObject>(gameObject, "getComponent(System.Type)", { (void*)REApi::get()->typeof("app.Wp10Insect") });
                    if (Wp10Insect != nullptr) {
                        outPInfo.Wp_Insect = gameObject;
                        foundKinsect = true;
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
            charaMakeSceneControllerCache = nullptr;
            return;
        }

        END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Character Creator - Basic Info");
        //--------------------------------------------------

        PersistentPlayerInfo persistentInfo{};
        persistentInfo.playerData = info.playerData;
        persistentInfo.index = 0;

        //- Equipped Armours -------------------------------
        BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Character Creator - Equipped Armours");

        bool fetchedArmours = fetchPlayer_EquippedArmours_FromCharaMakeSceneController(info.optionalPointers.CharaMakeSceneController, info, persistentInfo);
        if (!fetchedArmours) {
            return;
        }

        END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Character Creator - Equipped Armours");
        //--------------------------------------------------

        //- Armour Transforms ------------------------------
        BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Character Creator - Armour Transforms");

        bool fetchedTransforms = fetchPlayer_ArmourTransforms_FromCharaMakeSceneController(info.optionalPointers.CharaMakeSceneController, info, persistentInfo);
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
                DEBUG_STACK.push(std::format("{} Failed to fetch bones for Character Creator Hunter: {} [{}]. Reason: {}.", PLAYER_TRACKER_LOG_TAG, info.playerData.name, info.playerData.hunterId, reason), DebugStack::Color::COL_WARNING);
                return;
            }

            END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Character Creator - Bones");
            //--------------------------------------------------

            //- Parts ------------------------------------------
            BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Character Creator - Parts");

            bool fetchedParts = fetchPlayer_Parts(info, persistentInfo);
            if (!fetchedParts) {
                DEBUG_STACK.push(std::format("{} Failed to fetch parts for Character Creator Hunter: {} [{}]", PLAYER_TRACKER_LOG_TAG, info.playerData.name, info.playerData.hunterId), DebugStack::Color::COL_WARNING);
                return;
            }

            END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Character Creator - Parts");
            //--------------------------------------------------

            // - Materials -------------------------------------
            BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Guild Card - Materials");

            bool fetchedMaterials = fetchPlayer_Materials(info, persistentInfo);
            if (!fetchedMaterials) {
                DEBUG_STACK.push(std::format("{} Failed to fetch materials for Character Creator Hunter: {} [{}]", PLAYER_TRACKER_LOG_TAG, info.playerData.name, info.playerData.hunterId), DebugStack::Color::COL_WARNING);
                return;
            }

            END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Guild Card - Materials");
            //--------------------------------------------------

            playerApplyDelays[persistentInfo.playerData] = std::chrono::high_resolution_clock::now();
            persistentPlayerInfos[0] = std::move(persistentInfo);
        }

        playerSlotTable.emplace(info.playerData, 0);
        playerInfos[0] = std::move(info);
    }

    bool PlayerTracker::fetchPlayers_CharacterCreator_BasicInfo(PlayerInfo& outInfo) {
        PlayerData hunter{};

        bool gotSaveData =
            SituationWatcher::inCustomSituation(CustomSituation::isInGame)
            ? getActiveSavePlayerData(hunter)
            : getSavePlayerData(lastSelectedSaveIdx, hunter);

        if (!gotSaveData) return false;

        outInfo = PlayerInfo{};

        if (!resolveHunterAndController(
            outInfo,
            hunter,
            outInfo.optionalPointers.CharaMakeSceneController,
            characterCreatorHunterTransformCache,
            charaMakeSceneControllerCache,
            "CharaMake_HunterXX",
            "CharaMake_HunterXY",
            "CharaMakeSceneController",
            "app.CharaMakeSceneController"
        )) return false;

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
            guildCardSceneControllerCache = nullptr;
            guildCardHunterGameObjCache   = nullptr;
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

        bool fetchedArmours = fetchPlayer_EquippedArmours_FromGuildCardHunter(info.optionalPointers.GuildCard_Hunter, info, persistentInfo);
        if (!fetchedArmours) {
            return;
        }

        END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Guild Card - Equipped Armours");
        //--------------------------------------------------

        //- Armour Transforms ------------------------------
        BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Guild Card - Armour Transforms");

        bool fetchedTransforms = fetchPlayer_ArmourTransforms_FromGuildCardHunter(info.optionalPointers.GuildCard_Hunter, info, persistentInfo);
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
                DEBUG_STACK.push(std::format("{} Failed to fetch bones for Guild Card Hunter: {} [{}]. Reason: {}.", PLAYER_TRACKER_LOG_TAG, info.playerData.name, info.playerData.hunterId, reason), DebugStack::Color::COL_WARNING);
                return;
            }

            END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Guild Card - Bones");
            //--------------------------------------------------

            //- Parts ------------------------------------------
            BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Guild Card - Parts");

            bool fetchedParts = fetchPlayer_Parts(info, persistentInfo);
            if (!fetchedParts) {
                DEBUG_STACK.push(std::format("{} Failed to fetch parts for Guild Card Hunter: {} [{}]", PLAYER_TRACKER_LOG_TAG, info.playerData.name, info.playerData.hunterId), DebugStack::Color::COL_WARNING);
                return;
            }

            END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Guild Card - Parts");
            //--------------------------------------------------

            // - Materials -------------------------------------
            BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Guild Card - Materials");

			bool fetchedMaterials = fetchPlayer_Materials(info, persistentInfo);
            if (!fetchedMaterials) {
                DEBUG_STACK.push(std::format("{} Failed to fetch materials for Guild Card Hunter: {} [{}]", PLAYER_TRACKER_LOG_TAG, info.playerData.name, info.playerData.hunterId), DebugStack::Color::COL_WARNING);
                return;
			}

			END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Guild Card - Materials");
			//--------------------------------------------------

            playerApplyDelays[persistentInfo.playerData] = std::chrono::high_resolution_clock::now();
            persistentPlayerInfos[0] = std::move(persistentInfo);
        }

        playerSlotTable.emplace(info.playerData, 0);
        playerInfos[0] = std::move(info);
    }

    bool PlayerTracker::fetchPlayers_HunterGuildCard_BasicInfo(PlayerInfo& outInfo) {
        bool* isSelfProfile = REFieldPtr<bool>(guiManager.get(), "_HunterProfile_IsSelfProfile");

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
            REApi::ManagedObject* HunterProfile_UserInfo = REFieldPtr<REApi::ManagedObject>(guiManager.get(), "_HunterProfile_UserInfo");
            if (HunterProfile_UserInfo == nullptr) return false;

            std::string name = REInvokeStr(HunterProfile_UserInfo, "get_PlName", {});
            if (name.empty()) return false;

            std::string shortHunterId = REInvokeStr(HunterProfile_UserInfo, "get_ShortHunterId", {});
            if (shortHunterId.empty()) return false;

            // Go fishing for the gender...
            REApi::ManagedObject* GuildCardSceneController = REFieldPtr<REApi::ManagedObject>(guiManager.get(), "_HunterProfile_SceneController");
            if (GuildCardSceneController == nullptr) return false;

            REApi::ManagedObject* CharacterEditBuilder = REFieldPtr<REApi::ManagedObject>(GuildCardSceneController, "_HunterBuilder");
            if (CharacterEditBuilder == nullptr) return false;

            int physiqueStyle = REInvoke<int>(CharacterEditBuilder, "get_PhysiqueStyle", {}, InvokeReturnType::DWORD);
            bool female = physiqueStyle == 2;

            hunter.female = female;
            hunter.name = name;
            hunter.hunterId = shortHunterId;
        }

        outInfo = PlayerInfo{};

        bool usedCache = false;
        if (guildCardHunterTransformCache && guildCardHunterGameObjCache && guildCardSceneControllerCache) {
            // Check the cache hasn't been invalidated
            static const REApi::TypeDefinition* def_ViaTransform = REApi::get()->tdb()->find_type("via.Transform");
            if (checkREPtrValidity(guildCardHunterTransformCache, def_ViaTransform)) {
                outInfo.pointers.Transform = guildCardHunterTransformCache;
                outInfo.optionalPointers.GuildCard_Hunter = guildCardHunterGameObjCache;
                outInfo.optionalPointers.GuildCardSceneController = guildCardSceneControllerCache;
                usedCache = true;
            }
        }

        if (!usedCache) {
            REApi::ManagedObject* currentScene = getCurrentScene();
            if (currentScene == nullptr) return false;

            static const REApi::ManagedObject* transformType = REApi::get()->typeof("via.Transform");
            REApi::ManagedObject* transformComponents = REInvokePtr<REApi::ManagedObject>(currentScene, "findComponents(System.Type)", { (void*)transformType });

            bool foundHunterGameObj = false;
            bool foundHunterTransform = false;
            bool foundGuildCardSceneController = false;

            constexpr const char* guildCardSceneControllerName = "GuildCardSceneController";
            constexpr const char* guildCardHunterName = "GuildCard_Hunter";
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
                    foundHunterTransform = true;
                }
                else if (name == guildCardHunterName) {
                    outInfo.optionalPointers.GuildCard_Hunter = gameObject;
                    guildCardHunterGameObjCache = gameObject;
                    foundHunterGameObj = true;
                }
                else if (name == guildCardSceneControllerName) {
                    REApi::ManagedObject* controller = getComponent(gameObject, "app.GuildCardSceneController");
                    outInfo.optionalPointers.GuildCardSceneController = controller;
                    guildCardSceneControllerCache = controller;
                    foundGuildCardSceneController = true;
                }

                if (foundHunterGameObj && foundHunterTransform && foundGuildCardSceneController) break;
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
        for (size_t i = 0; i < playerListSize; i++) {
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
                info.index                              = i;
                info.playerData                         = slotCache.playerData;
                info.pointers.Transform                 = slotCache.Transform;
                info.optionalPointers.Motion            = slotCache.Motion;
                info.optionalPointers.HunterCharacter   = slotCache.HunterCharacter;
                info.optionalPointers.cHunterCreateInfo = slotCache.cHunterCreateInfo;
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
            newCache.playerData        = info.playerData;
            newCache.Transform         = info.pointers.Transform;
            newCache.Motion            = info.optionalPointers.Motion;
            newCache.HunterCharacter   = info.optionalPointers.HunterCharacter;
            newCache.cHunterCreateInfo = info.optionalPointers.cHunterCreateInfo;
            playerInfoCaches[i] = newCache;
            END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Normal Gameplay - Basic Info");
        }

        // -- Visibility --------------------------------------------------------------------------------------------
        BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Normal Gameplay - Visibility");
        fetchPlayer_Visibility(info);
        END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Normal Gameplay - Visibility");

        // Fetch when requested, or if no fetch has been done but the player is in-view.
        if (playersToFetch[i] || (info.visible && !persistentPlayerInfos[i].has_value())) {
            BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Normal Gameplay - Persistent Info");

            PersistentPlayerInfo persistentInfo{};
            persistentInfo.playerData = info.playerData;
            persistentInfo.index = i;

            bool fetchedPInfo = fetchPlayer_PersistentInfo(i, info, persistentInfo);
            if (fetchedPInfo) {
                playersToFetch[i] = false;
                playerApplyDelays[persistentInfo.playerData] = std::chrono::high_resolution_clock::now();
                persistentPlayerInfos[i] = std::move(persistentInfo);
                occupiedNormalGameplaySlots[i] = true;
            }

            END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Normal Gameplay - Persistent Info");
        }

        if (!playerSlotTable.contains(info.playerData)) playerSlotTable.emplace(info.playerData, i);
        playerInfos[i] = std::move(info);
    }

    PlayerFetchFlags PlayerTracker::fetchPlayer_BasicInfo(size_t i, bool inQuest, bool online, PlayerInfo& out) {
        // app.cPlayerManageInfo
        REApi::ManagedObject* cPlayerManageInfo = REInvokePtr<REApi::ManagedObject>(playerManager.get(), "findPlayer_StableMemberIndex(System.Int32, app.net_session_manager.SESSION_TYPE)", {(void*)i, (void*)1});
        if (cPlayerManageInfo == nullptr) { clearPlayerSlot(i); return PlayerFetchFlags::FETCH_PLAYER_SLOT_EMPTY; } // Player slot is empty, clear it

        // Query for app.cPlayerManageControl to find most up-to-date HunterCreateInfo that includes previews.
        REApi::ManagedObject* cPlayerManageControl = REInvokePtr<REApi::ManagedObject>(playerManager.get(), "findPlayerControl_StableMemberIndex(System.Int32)", { (void*)i });
        if (!cPlayerManageControl) { clearPlayerSlot(i); return PlayerFetchFlags::FETCH_PLAYER_SLOT_EMPTY; }

        bool includeThisPlayer = false;
        if (inQuest) includeThisPlayer = REInvoke<bool>(playerManager.get(), "isQuestMember(System.Int32)", {(void*)i}, InvokeReturnType::BOOL);
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
            DEBUG_STACK.push(std::format("{} Fetched player at index {}, but name returned nullptr, skipping.", PLAYER_TRACKER_LOG_TAG, i), DebugStack::Color::COL_WARNING);
            return PlayerFetchFlags::FETCH_ERROR_NULL;
        }

        int networkIndex = REInvoke<int>(cPlayerContext, "get_CurrentNetworkMemberIndex", {}, InvokeReturnType::DWORD);
		uint32_t unsignedNetworkIdx = static_cast<uint32_t>(networkIndex);

        REApi::ManagedObject* playerNetInfo = REInvokePtr<REApi::ManagedObject>(Net_UserInfoList, "getInfoSystem(System.UInt32)", { (void*)unsignedNetworkIdx });
        if (playerNetInfo == nullptr) {
            DEBUG_STACK.push(std::format("{} Fetched player at index {}, but playerNetInfo returned nullptr, skipping.", PLAYER_TRACKER_LOG_TAG, i), DebugStack::Color::COL_WARNING);
            return PlayerFetchFlags::FETCH_ERROR_NULL;
        }
        //DEBUG_STACK.push(std::format("playerNetInfo PTR: {} - Properties:\n{}",       ptrToHexString(playerNetInfo),       reObjectPropertiesToString(playerNetInfo)),       DebugStack::Color::DEBUG);

        std::string hunterId = "";
        if (online) {
            hunterId = REInvokeStr(playerNetInfo, "get_ShortHunterId", {});
            if (hunterId.empty()) {
                DEBUG_STACK.push(std::format("{} Fetched player at index {}, but hunterId returned nullptr, skipping.", PLAYER_TRACKER_LOG_TAG, i), DebugStack::Color::COL_WARNING);
                return PlayerFetchFlags::FETCH_ERROR_NULL;
            }
        }
        else {
            // Net info is incosistent as sh*t, so use save data for offline main hunter
            PlayerData p{};
            if (getActiveSavePlayerData(p)) {
                hunterId = REInvokeStr(netContextManager, "get_HunterShortId", {});
            }
            else {
                DEBUG_STACK.push(std::format("{} Failed to fetch Hunter ID in singleplayer.", PLAYER_TRACKER_LOG_TAG, i), DebugStack::Color::COL_WARNING);
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

		REApi::ManagedObject* requestedReloadingCreateInfo = REFieldPtr<REApi::ManagedObject>(cPlayerManageControl, "_RequestedReloadingCreateInfo");

        PlayerOptionalPointers optPointers{};
        optPointers.cPlayerManageInfo = cPlayerManageInfo;
        optPointers.HunterCharacter   = HunterCharacter;
        optPointers.Motion            = Motion;
        optPointers.cHunterCreateInfo = requestedReloadingCreateInfo;

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

		bool isSetUp = REInvoke<bool>(info.optionalPointers.HunterCharacter, "get_IsSetUp", {}, InvokeReturnType::BOOL);
        if (!isSetUp) return;

        info.distanceFromCameraSq = FLT_MAX;

        info.weaponDrawn     = REInvoke<bool>(info.optionalPointers.HunterCharacter, "get_IsWeaponOn", {}, InvokeReturnType::BOOL);
        info.inCombat        = REInvoke<bool>(info.optionalPointers.HunterCharacter, "get_IsCombat", {}, InvokeReturnType::BOOL);
        info.inTent          = REInvoke<bool>(info.optionalPointers.HunterCharacter, "get_IsInAllTent", {}, InvokeReturnType::BOOL);
        info.isRidingSeikret = REInvoke<bool>(info.optionalPointers.HunterCharacter, "get_IsPorterRiding", {}, InvokeReturnType::BOOL);

        // UPDATE NOTE: These will likely change with future updates!!
        // ITEM_0019 = Whetstone (v=20)
        // ITEM_0297 = Whetfish Fin (v=270)
        // ITEM_0710 = Whetfish Fin+ (v=683)
        uint32_t itemDef_ID = REInvoke<uint32_t>(info.optionalPointers.HunterCharacter, "get_UsedItemID", {}, InvokeReturnType::DWORD);
        info.isSharpening = (itemDef_ID == 20 || itemDef_ID == 270 || itemDef_ID == 683);

        const bool motionSkipped = REInvoke<bool>(info.optionalPointers.Motion, "get_SkipUpdate", {}, InvokeReturnType::BOOL);
        if (motionSkipped) return;

        const float& distThreshold = dataManager.settings().applicationRange;
        double sqDist = REInvoke<double>(info.optionalPointers.HunterCharacter, "getCameraDistanceSqXZ", {}, InvokeReturnType::DOUBLE);
        if (distThreshold > 0 && sqDist > distThreshold * distThreshold) return;

        info.distanceFromCameraSq = sqDist;
        info.visible = info.index == 0 || (info.index != 0 && !info.inTent);
        return;
    }

    bool PlayerTracker::fetchPlayer_PersistentInfo(size_t i, const PlayerInfo& info, PersistentPlayerInfo& pInfo) {
        if (frameBoneFetchCount != 0 && frameBoneFetchCount >= dataManager.settings().maxBoneFetchesPerFrame) return false;

        BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Normal Gameplay - Equipped Armours");
        bool fetchedArmours = fetchPlayer_EquippedArmours(info, pInfo);
        END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Normal Gameplay - Equipped Armours");
        if (!fetchedArmours) {
            DEBUG_STACK.push(std::format("{} Failed to fetch equipped armours for Player: {} [{}]", PLAYER_TRACKER_LOG_TAG, info.playerData.name, i), DebugStack::Color::COL_WARNING);
            return false;
        }

        BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Normal Gameplay - Armour Transforms");
        bool fetchedTransforms = fetchPlayer_ArmourTransforms(info, pInfo);
        END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Normal Gameplay - Armour Transforms");
        if (!fetchedTransforms) {
            DEBUG_STACK.fpush<PLAYER_TRACKER_LOG_TAG>("Failed to fetch armour transforms for Player: {} [{}].", info.playerData.name, i);

            //ArmourSet expectedHelm    = pInfo.armourInfo.helm.value_or({ "UNKNOWN!", false });
            //ArmourSet expectedBody    = pInfo.armourInfo.body.value_or({ "UNKNOWN!", false });
            //ArmourSet expectedArms    = pInfo.armourInfo.arms.value_or({ "UNKNOWN!", false });
            //ArmourSet expectedCoil    = pInfo.armourInfo.coil.value_or({ "UNKNOWN!", false });
            //ArmourSet expectedLegs    = pInfo.armourInfo.legs.value_or({ "UNKNOWN!", false });
            //ArmourSet expectedSlinger = pInfo.armourInfo.slinger.value_or({ "UNKNOWN!", false });

            //DEBUG_STACK.push(std::format("{} Failed to fetch armour transforms for Player: {} [{}]."
            //    "\nExpecting the following transforms:\n   Head:    {}\n   Body:    {}\n   Arms:    {}\n   Waist:   {}\n   Legs:    {}\n   Slinger: {}"
            //    "\nDumped the following transforms:"
            //    "\n{}",
            //    PLAYER_TRACKER_LOG_TAG, info.playerData.name, i,
            //    expectedHelm.name,
            //    expectedBody.name,
            //    expectedArms.name,
            //    expectedCoil.name,
            //    expectedLegs.name,
            //    expectedSlinger.name,
            //    dumpTransformTreeString(info.pointers.Transform)
            //), DebugStack::Color::COL_DEBUG);
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
            DEBUG_STACK.push(std::format("{} Failed to fetch bones for Player: {} [{}]. Reason: {}.", PLAYER_TRACKER_LOG_TAG, info.playerData.name, i, reason), DebugStack::Color::COL_WARNING);
            return false;
        }

        BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Normal Gameplay - Parts");
        bool fetchedParts = fetchPlayer_Parts(info, pInfo);
        END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Normal Gameplay - Parts");
        if (!fetchedParts) {
            DEBUG_STACK.push(std::format("{} Failed to fetch parts for Player: {} [{}]", PLAYER_TRACKER_LOG_TAG, info.playerData.name, i), DebugStack::Color::COL_WARNING);
            return false;
        }

        BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Normal Gameplay - Materials");
        bool fetchedMats = fetchPlayer_Materials(info, pInfo);
        END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Normal Gameplay - Materials");
        if (!fetchedMats) {
            DEBUG_STACK.push(std::format("{} Failed to fetch materials for Player: {} [{}]", PLAYER_TRACKER_LOG_TAG, info.playerData.name, i), DebugStack::Color::COL_WARNING);
            return false;
        }

        BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Normal Gameplay - Weapons");
        bool fetechedWeapons = fetchPlayer_WeaponObjects(info, pInfo);
        END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Player Fetch - Normal Gameplay - Weapons");

        frameBoneFetchCount++; // Consider moving this to top to limit effect of failed fetches - may make fetches inaccessible if there are enough errors though.
        return true;
    }

    bool PlayerTracker::fetchPlayer_EquippedArmours(const PlayerInfo& info, PersistentPlayerInfo& pInfo) {
        if (info.pointers.Transform == nullptr) return false;

        pInfo.armourInfo = ArmourInfo{}; // Reset

        // TODO: Gunna need a different function here that relies on finding the transforms like before to make menus work.

        bool considerPreviews = false;
        if (info.index == 0 && info.optionalPointers.cHunterCreateInfo) {
            // Potentially handle previews for the main player
            considerPreviews |= REInvoke<bool>(info.optionalPointers.cHunterCreateInfo, "get_IsFittingMode",    {}, InvokeReturnType::BOOL);
			considerPreviews |= REInvoke<bool>(info.optionalPointers.cHunterCreateInfo, "get_IsArenaQuestMode", {}, InvokeReturnType::BOOL);
        }

        ArmourDataManager& dataMgr = ArmourDataManager::get();
        if (considerPreviews) {
            // Use this when in the smithy / already used once?? Maybe trigger usage from onEquipArmor thing??
            WholeBodyArmorSetID* armourSetIDwholeBody = REFieldPtr<WholeBodyArmorSetID>(info.optionalPointers.cHunterCreateInfo, "ArmorSetID_WholeBody");
            if (armourSetIDwholeBody == nullptr) return false;

            pInfo.armourInfo.helm    = dataMgr.getArmourSetFromArmourID(armourSetIDwholeBody->helm);
            pInfo.armourInfo.body    = dataMgr.getArmourSetFromArmourID(armourSetIDwholeBody->body);
		    pInfo.armourInfo.arms    = dataMgr.getArmourSetFromArmourID(armourSetIDwholeBody->arms);
		    pInfo.armourInfo.coil    = dataMgr.getArmourSetFromArmourID(armourSetIDwholeBody->coil);
		    pInfo.armourInfo.legs    = dataMgr.getArmourSetFromArmourID(armourSetIDwholeBody->legs);
            pInfo.armourInfo.slinger = std::nullopt;
        }
        else {
            ArmorSetID helmSetID    = REInvoke<ArmorSetID>(info.optionalPointers.HunterCharacter, "getArmorSetId(app.ArmorDef.ARMOR_PARTS)", { (void*)ArmorParts::HELM }, InvokeReturnType::WORD);
            ArmorSetID bodySetID    = REInvoke<ArmorSetID>(info.optionalPointers.HunterCharacter, "getArmorSetId(app.ArmorDef.ARMOR_PARTS)", { (void*)ArmorParts::BODY }, InvokeReturnType::WORD);
            ArmorSetID armsSetID    = REInvoke<ArmorSetID>(info.optionalPointers.HunterCharacter, "getArmorSetId(app.ArmorDef.ARMOR_PARTS)", { (void*)ArmorParts::ARMS }, InvokeReturnType::WORD);
            ArmorSetID coilSetID    = REInvoke<ArmorSetID>(info.optionalPointers.HunterCharacter, "getArmorSetId(app.ArmorDef.ARMOR_PARTS)", { (void*)ArmorParts::COIL }, InvokeReturnType::WORD);
            ArmorSetID legsSetID    = REInvoke<ArmorSetID>(info.optionalPointers.HunterCharacter, "getArmorSetId(app.ArmorDef.ARMOR_PARTS)", { (void*)ArmorParts::LEGS }, InvokeReturnType::WORD);
            ArmorSetID slingerSetID = REInvoke<ArmorSetID>(info.optionalPointers.HunterCharacter, "getArmorSetId(app.ArmorDef.ARMOR_PARTS)", { (void*)ArmorParts::SLINGER }, InvokeReturnType::WORD);

            pInfo.armourInfo.helm    = dataMgr.getArmourSetFromArmourID(helmSetID);
            pInfo.armourInfo.body    = dataMgr.getArmourSetFromArmourID(bodySetID);
		    pInfo.armourInfo.arms    = dataMgr.getArmourSetFromArmourID(armsSetID);
		    pInfo.armourInfo.coil    = dataMgr.getArmourSetFromArmourID(coilSetID);
		    pInfo.armourInfo.legs    = dataMgr.getArmourSetFromArmourID(legsSetID);
		    pInfo.armourInfo.slinger = dataMgr.getArmourSetFromArmourID(slingerSetID);
        }

        return true;
    }

    bool PlayerTracker::fetchPlayer_EquippedArmours_FromSaveFile(const PlayerInfo& info, PersistentPlayerInfo& pInfo, int saveIdx, bool overrideInner) {
        if (saveIdx >= 3)
            return false;

        REApi::ManagedObject* save = getSaveDataObject(saveIdx);
        if (!save || !isSaveActive(save))
            return false;

        REApi::ManagedObject* equip = REInvokePtr<REApi::ManagedObject>(save, "get_Equip", {});
        if (!equip)
            return false;

        REApi::ManagedObject* outerSet = REInvokePtr<REApi::ManagedObject>(equip, "get_OuterArmorCurrent", {});
        REApi::ManagedObject* visible = REInvokePtr<REApi::ManagedObject>(equip, "get_EquipVisible", {});
        if (!outerSet || !visible)
            return false;

        pInfo.armourInfo.helm = getArmourForPartFromSave(save, equip, outerSet, visible, ArmorParts::HELM, overrideInner);
        pInfo.armourInfo.body = getArmourForPartFromSave(save, equip, outerSet, visible, ArmorParts::BODY, overrideInner);
        pInfo.armourInfo.arms = getArmourForPartFromSave(save, equip, outerSet, visible, ArmorParts::ARMS, overrideInner);
        pInfo.armourInfo.coil = getArmourForPartFromSave(save, equip, outerSet, visible, ArmorParts::COIL, overrideInner);
        pInfo.armourInfo.legs = getArmourForPartFromSave(save, equip, outerSet, visible, ArmorParts::LEGS, overrideInner);

        return true;
    }

    bool PlayerTracker::fetchPlayer_EquippedArmours_FromCharaMakeSceneController(REApi::ManagedObject* controller, const PlayerInfo& info, PersistentPlayerInfo& pInfo) {
        if (!controller) return false;

		REApi::ManagedObject* hunterDoll = REFieldPtr<REApi::ManagedObject>(controller, "_HunterDoll");
		if (!hunterDoll) return false;

		REApi::ManagedObject* mcCharaMakeHunterController = REInvokePtr<REApi::ManagedObject>(hunterDoll, "get_CharaMakeHunterController", {});
		if (!mcCharaMakeHunterController) return false;

		bool isArmorVisible = REInvoke<bool>(mcCharaMakeHunterController, "get_IsArmorVisible", {}, InvokeReturnType::BOOL);

        size_t saveIdx = REInvoke<size_t>(controller, "get_SaveIndex", {}, InvokeReturnType::BOOL);
        return fetchPlayer_EquippedArmours_FromSaveFile(info, pInfo, saveIdx, !isArmorVisible);
    }

    bool PlayerTracker::fetchPlayer_EquippedArmours_FromGuildCardHunter(REApi::ManagedObject* hunter, const PlayerInfo& info, PersistentPlayerInfo& pInfo) {
        if (!hunter) return false;
        if (!info.optionalPointers.GuildCardSceneController) return false;

        size_t phase = REInvoke<size_t>(info.optionalPointers.GuildCardSceneController, "get_Phase", {}, InvokeReturnType::DWORD);
        if (phase != 5) return false; // 5 = ACTIVE, models should be loaded.

        REApi::ManagedObject* hunterDoll = getComponent(hunter, "app.HunterDoll");
        if (!hunterDoll) return false;

        REApi::ManagedObject* mcHunterProfileHunterController = REInvokePtr<REApi::ManagedObject>(hunterDoll, "get_HunterProfileHunterController", {});
        if (!mcHunterProfileHunterController) return false;

        REApi::ManagedObject* mcPreviewHunterVisualController = REFieldPtr<REApi::ManagedObject>(mcHunterProfileHunterController, "_VisualController");
        if (!mcPreviewHunterVisualController) return false;

        ArmourDataManager& dataMgr = ArmourDataManager::get();

        ArmorSetID helmSetID    = REInvoke<ArmorSetID>(mcPreviewHunterVisualController, "getArmorID(app.ArmorDef.ARMOR_PARTS)", { (void*)ArmorParts::HELM }, InvokeReturnType::WORD);
        ArmorSetID bodySetID    = REInvoke<ArmorSetID>(mcPreviewHunterVisualController, "getArmorID(app.ArmorDef.ARMOR_PARTS)", { (void*)ArmorParts::BODY }, InvokeReturnType::WORD);
        ArmorSetID armsSetID    = REInvoke<ArmorSetID>(mcPreviewHunterVisualController, "getArmorID(app.ArmorDef.ARMOR_PARTS)", { (void*)ArmorParts::ARMS }, InvokeReturnType::WORD);
        ArmorSetID coilSetID    = REInvoke<ArmorSetID>(mcPreviewHunterVisualController, "getArmorID(app.ArmorDef.ARMOR_PARTS)", { (void*)ArmorParts::COIL }, InvokeReturnType::WORD);
        ArmorSetID legsSetID    = REInvoke<ArmorSetID>(mcPreviewHunterVisualController, "getArmorID(app.ArmorDef.ARMOR_PARTS)", { (void*)ArmorParts::LEGS }, InvokeReturnType::WORD);
        ArmorSetID slingerSetID = REInvoke<ArmorSetID>(mcPreviewHunterVisualController, "getArmorID(app.ArmorDef.ARMOR_PARTS)", { (void*)ArmorParts::SLINGER }, InvokeReturnType::WORD);

        pInfo.armourInfo.helm    = dataMgr.getArmourSetFromArmourID(helmSetID);
        pInfo.armourInfo.body    = dataMgr.getArmourSetFromArmourID(bodySetID);
		pInfo.armourInfo.arms    = dataMgr.getArmourSetFromArmourID(armsSetID);
		pInfo.armourInfo.coil    = dataMgr.getArmourSetFromArmourID(coilSetID);
		pInfo.armourInfo.legs    = dataMgr.getArmourSetFromArmourID(legsSetID);
		pInfo.armourInfo.slinger = dataMgr.getArmourSetFromArmourID(slingerSetID);

        return true;
    }
    
    bool PlayerTracker::fetchPlayer_ArmourTransforms(const PlayerInfo& info, PersistentPlayerInfo& pInfo) {
        if (info.pointers.Transform == nullptr) return false;
        if (info.optionalPointers.HunterCharacter == nullptr) return false;

        // Note: Helm is optional due to toggle
        if (!pInfo.armourInfo.body.has_value()) return false;
        if (!pInfo.armourInfo.arms.has_value()) return false;
        if (!pInfo.armourInfo.coil.has_value()) return false;
        if (!pInfo.armourInfo.legs.has_value()) return false;

        // Base transform is fetched every frame
        pInfo.Transform_base = info.pointers.Transform;

        REApi::ManagedObject* helmGameObj = REInvokePtr<REApi::ManagedObject>(info.optionalPointers.HunterCharacter, "getParts(app.ArmorDef.ARMOR_PARTS)", { (void*)ArmorParts::HELM }   );
        REApi::ManagedObject* bodyGameObj = REInvokePtr<REApi::ManagedObject>(info.optionalPointers.HunterCharacter, "getParts(app.ArmorDef.ARMOR_PARTS)", { (void*)ArmorParts::BODY }   );
        REApi::ManagedObject* armsGameObj = REInvokePtr<REApi::ManagedObject>(info.optionalPointers.HunterCharacter, "getParts(app.ArmorDef.ARMOR_PARTS)", { (void*)ArmorParts::ARMS }   );
        REApi::ManagedObject* coilGameObj = REInvokePtr<REApi::ManagedObject>(info.optionalPointers.HunterCharacter, "getParts(app.ArmorDef.ARMOR_PARTS)", { (void*)ArmorParts::COIL }   );
        REApi::ManagedObject* legsGameObj = REInvokePtr<REApi::ManagedObject>(info.optionalPointers.HunterCharacter, "getParts(app.ArmorDef.ARMOR_PARTS)", { (void*)ArmorParts::LEGS }   );
        
		pInfo.Transform_helm = (helmGameObj) ? REInvokePtr<REApi::ManagedObject>(helmGameObj, "get_Transform", {}) : nullptr;
		pInfo.Transform_body = (bodyGameObj) ? REInvokePtr<REApi::ManagedObject>(bodyGameObj, "get_Transform", {}) : nullptr;
		pInfo.Transform_arms = (armsGameObj) ? REInvokePtr<REApi::ManagedObject>(armsGameObj, "get_Transform", {}) : nullptr;
		pInfo.Transform_coil = (coilGameObj) ? REInvokePtr<REApi::ManagedObject>(coilGameObj, "get_Transform", {}) : nullptr;
		pInfo.Transform_legs = (legsGameObj) ? REInvokePtr<REApi::ManagedObject>(legsGameObj, "get_Transform", {}) : nullptr;
        
        pInfo.Slinger_GameObject = REInvokePtr<REApi::ManagedObject>(info.optionalPointers.HunterCharacter, "getParts(app.ArmorDef.ARMOR_PARTS)", { (void*)ArmorParts::SLINGER });

        bool foundRequired = (pInfo.Transform_base &&
            pInfo.Transform_body &&
            pInfo.Transform_arms &&
            pInfo.Transform_coil &&
            pInfo.Transform_legs);

        return foundRequired;
    }

    bool PlayerTracker::fetchPlayer_ArmourTransforms_FromEventModel(const PlayerInfo& info, PersistentPlayerInfo& pInfo) {
        if (info.pointers.Transform == nullptr) return false;

        pInfo.Transform_base = info.pointers.Transform;

        // Note: Helm is optional due to toggle
        if (!pInfo.armourInfo.body.has_value()) return false;
        if (!pInfo.armourInfo.arms.has_value()) return false;
        if (!pInfo.armourInfo.coil.has_value()) return false;
        if (!pInfo.armourInfo.legs.has_value()) return false;

        REApi::ManagedObject* gameObj = REInvokePtr<REApi::ManagedObject>(info.pointers.Transform, "get_GameObject", {});
        if (!gameObj) return false;

        REApi::ManagedObject* eventModelSetupper = getComponent(gameObj, "app.EventModelSetupper");
        if (!eventModelSetupper) return false;

        REApi::ManagedObject* equip = REFieldPtr<REApi::ManagedObject>(eventModelSetupper, "_PlEquip");
        if (!equip) return false;

        REApi::ManagedObject* helmGameObj = REInvokePtr<REApi::ManagedObject>(equip, "get_Item(System.Int32)", { (void*)0 });
        REApi::ManagedObject* bodyGameObj = REInvokePtr<REApi::ManagedObject>(equip, "get_Item(System.Int32)", { (void*)1 });
        REApi::ManagedObject* armsGameObj = REInvokePtr<REApi::ManagedObject>(equip, "get_Item(System.Int32)", { (void*)2 });
        REApi::ManagedObject* coilGameObj = REInvokePtr<REApi::ManagedObject>(equip, "get_Item(System.Int32)", { (void*)3 });
        REApi::ManagedObject* legsGameObj = REInvokePtr<REApi::ManagedObject>(equip, "get_Item(System.Int32)", { (void*)4 });

        pInfo.Transform_helm = (helmGameObj) ? REInvokePtr<REApi::ManagedObject>(helmGameObj, "get_Transform", {}) : nullptr;
        pInfo.Transform_body = (bodyGameObj) ? REInvokePtr<REApi::ManagedObject>(bodyGameObj, "get_Transform", {}) : nullptr;
        pInfo.Transform_arms = (armsGameObj) ? REInvokePtr<REApi::ManagedObject>(armsGameObj, "get_Transform", {}) : nullptr;
        pInfo.Transform_coil = (coilGameObj) ? REInvokePtr<REApi::ManagedObject>(coilGameObj, "get_Transform", {}) : nullptr;
        pInfo.Transform_legs = (legsGameObj) ? REInvokePtr<REApi::ManagedObject>(legsGameObj, "get_Transform", {}) : nullptr;
        
        pInfo.Slinger_GameObject = REInvokePtr<REApi::ManagedObject>(equip, "get_Item(System.Int32)", { (void*)5 });

        bool foundRequired = (pInfo.Transform_base &&
            pInfo.Transform_body &&
            pInfo.Transform_arms &&
            pInfo.Transform_coil &&
            pInfo.Transform_legs);

        return foundRequired;
    }

    bool PlayerTracker::fetchPlayer_ArmourTransforms_FromSaveSelectSceneController(REApi::ManagedObject* sceneController, const PlayerInfo& info, PersistentPlayerInfo& pInfo) {
        if (info.pointers.Transform == nullptr) return false;
        if (sceneController == nullptr) return false;

        pInfo.Transform_base = info.pointers.Transform;

        // Note: Helm is optional due to toggle
        if (!pInfo.armourInfo.body.has_value()) return false;
        if (!pInfo.armourInfo.arms.has_value()) return false;
        if (!pInfo.armourInfo.coil.has_value()) return false;
        if (!pInfo.armourInfo.legs.has_value()) return false;

        size_t* displaySaveIdx = REFieldPtr<size_t>(sceneController, "_DisplaySaveIndex");
		if (!displaySaveIdx || *displaySaveIdx >= 3) return false;

        REApi::ManagedObject* hunterDoll = REFieldPtr<REApi::ManagedObject>(sceneController, "_HunterController");
        if (!hunterDoll) return false;

        // TODO: get_HunterProfileHunterController for guild card too
        REApi::ManagedObject* controller = REInvokePtr<REApi::ManagedObject>(hunterDoll, "get_SaveSelectHunterController", {});
        if (!controller) return false;

        REApi::ManagedObject* visualController = REFieldPtr<REApi::ManagedObject>(controller, "_VisualController");
        if (!visualController) return false;

        REApi::ManagedObject* helmGameObj = REInvokePtr<REApi::ManagedObject>(visualController, "getParts(app.ArmorDef.ARMOR_PARTS)", { (void*)ArmorParts::HELM });
        REApi::ManagedObject* bodyGameObj = REInvokePtr<REApi::ManagedObject>(visualController, "getParts(app.ArmorDef.ARMOR_PARTS)", { (void*)ArmorParts::BODY });
        REApi::ManagedObject* armsGameObj = REInvokePtr<REApi::ManagedObject>(visualController, "getParts(app.ArmorDef.ARMOR_PARTS)", { (void*)ArmorParts::ARMS });
        REApi::ManagedObject* coilGameObj = REInvokePtr<REApi::ManagedObject>(visualController, "getParts(app.ArmorDef.ARMOR_PARTS)", { (void*)ArmorParts::COIL });
        REApi::ManagedObject* legsGameObj = REInvokePtr<REApi::ManagedObject>(visualController, "getParts(app.ArmorDef.ARMOR_PARTS)", { (void*)ArmorParts::LEGS });

        pInfo.Transform_helm = (helmGameObj) ? REInvokePtr<REApi::ManagedObject>(helmGameObj, "get_Transform", {}) : nullptr;
        pInfo.Transform_body = (bodyGameObj) ? REInvokePtr<REApi::ManagedObject>(bodyGameObj, "get_Transform", {}) : nullptr;
        pInfo.Transform_arms = (armsGameObj) ? REInvokePtr<REApi::ManagedObject>(armsGameObj, "get_Transform", {}) : nullptr;
        pInfo.Transform_coil = (coilGameObj) ? REInvokePtr<REApi::ManagedObject>(coilGameObj, "get_Transform", {}) : nullptr;
        pInfo.Transform_legs = (legsGameObj) ? REInvokePtr<REApi::ManagedObject>(legsGameObj, "get_Transform", {}) : nullptr;

        pInfo.Slinger_GameObject = REInvokePtr<REApi::ManagedObject>(visualController, "getParts(app.ArmorDef.ARMOR_PARTS)", { (void*)ArmorParts::SLINGER });

        bool foundRequired = (pInfo.Transform_base &&
            pInfo.Transform_body &&
            pInfo.Transform_arms &&
            pInfo.Transform_coil &&
            pInfo.Transform_legs);

        return foundRequired;
    }

    bool PlayerTracker::fetchPlayer_ArmourTransforms_FromCharaMakeSceneController(REApi::ManagedObject* sceneController, const PlayerInfo& info, PersistentPlayerInfo& pInfo) {
        if (info.pointers.Transform == nullptr) return false;
        if (sceneController == nullptr) return false;

        pInfo.Transform_base = info.pointers.Transform;

        // Note: Helm is optional due to toggle
        if (!pInfo.armourInfo.body.has_value()) return false;
        if (!pInfo.armourInfo.arms.has_value()) return false;
        if (!pInfo.armourInfo.coil.has_value()) return false;
        if (!pInfo.armourInfo.legs.has_value()) return false;

        // Check model isn't still loading
		REApi::ManagedObject* hunterDoll = REFieldPtr<REApi::ManagedObject>(sceneController, "_HunterDoll");
		if (!hunterDoll) return false;

		bool* requiresPartsSetup = REFieldPtr<bool>(hunterDoll, "_RequiresPartsSetup");
		if (!requiresPartsSetup || *requiresPartsSetup) return false;

        REApi::ManagedObject* mcCharaMakeController = REFieldPtr<REApi::ManagedObject>(sceneController, "_HunterCharaMake");
        if (!mcCharaMakeController) return false;

        REApi::ManagedObject* helmGameObj = REInvokePtr<REApi::ManagedObject>(mcCharaMakeController, "getPartsObject(app.ArmorDef.ARMOR_PARTS)", { (void*)ArmorParts::HELM });
        REApi::ManagedObject* bodyGameObj = REInvokePtr<REApi::ManagedObject>(mcCharaMakeController, "getPartsObject(app.ArmorDef.ARMOR_PARTS)", { (void*)ArmorParts::BODY });
        REApi::ManagedObject* armsGameObj = REInvokePtr<REApi::ManagedObject>(mcCharaMakeController, "getPartsObject(app.ArmorDef.ARMOR_PARTS)", { (void*)ArmorParts::ARMS });
        REApi::ManagedObject* coilGameObj = REInvokePtr<REApi::ManagedObject>(mcCharaMakeController, "getPartsObject(app.ArmorDef.ARMOR_PARTS)", { (void*)ArmorParts::COIL });
        REApi::ManagedObject* legsGameObj = REInvokePtr<REApi::ManagedObject>(mcCharaMakeController, "getPartsObject(app.ArmorDef.ARMOR_PARTS)", { (void*)ArmorParts::LEGS });

        pInfo.Transform_helm = (helmGameObj) ? REInvokePtr<REApi::ManagedObject>(helmGameObj, "get_Transform", {}) : nullptr;
        pInfo.Transform_body = (bodyGameObj) ? REInvokePtr<REApi::ManagedObject>(bodyGameObj, "get_Transform", {}) : nullptr;
        pInfo.Transform_arms = (armsGameObj) ? REInvokePtr<REApi::ManagedObject>(armsGameObj, "get_Transform", {}) : nullptr;
        pInfo.Transform_coil = (coilGameObj) ? REInvokePtr<REApi::ManagedObject>(coilGameObj, "get_Transform", {}) : nullptr;
        pInfo.Transform_legs = (legsGameObj) ? REInvokePtr<REApi::ManagedObject>(legsGameObj, "get_Transform", {}) : nullptr;

        pInfo.Slinger_GameObject = REInvokePtr<REApi::ManagedObject>(mcCharaMakeController, "getPartsObject(app.ArmorDef.ARMOR_PARTS)", { (void*)ArmorParts::SLINGER });

        bool foundRequired = (pInfo.Transform_base &&
            pInfo.Transform_body &&
            pInfo.Transform_arms &&
            pInfo.Transform_coil &&
            pInfo.Transform_legs);

        return foundRequired;
    }

    bool PlayerTracker::fetchPlayer_ArmourTransforms_FromGuildCardHunter(REApi::ManagedObject* hunter, const PlayerInfo& info, PersistentPlayerInfo& pInfo) {
        if (info.pointers.Transform == nullptr) return false;
        if (!hunter) return false;

        pInfo.Transform_base = info.pointers.Transform;

        if (!pInfo.armourInfo.body.has_value()) return false;
        if (!pInfo.armourInfo.arms.has_value()) return false;
        if (!pInfo.armourInfo.coil.has_value()) return false;
        if (!pInfo.armourInfo.legs.has_value()) return false;

        REApi::ManagedObject* hunterDoll = getComponent(hunter, "app.HunterDoll");
        if (!hunterDoll) return false;

        REApi::ManagedObject* mcHunterProfileHunterController = REInvokePtr<REApi::ManagedObject>(hunterDoll, "get_HunterProfileHunterController", {});
        if (!mcHunterProfileHunterController) return false;

        REApi::ManagedObject* mcPreviewHunterVisualController = REFieldPtr<REApi::ManagedObject>(mcHunterProfileHunterController, "_VisualController");
        if (!mcPreviewHunterVisualController) return false;

        REApi::ManagedObject* helmGameObj = REInvokePtr<REApi::ManagedObject>(mcPreviewHunterVisualController, "getParts(app.ArmorDef.ARMOR_PARTS)", { (void*)ArmorParts::HELM });
        REApi::ManagedObject* bodyGameObj = REInvokePtr<REApi::ManagedObject>(mcPreviewHunterVisualController, "getParts(app.ArmorDef.ARMOR_PARTS)", { (void*)ArmorParts::BODY });
        REApi::ManagedObject* armsGameObj = REInvokePtr<REApi::ManagedObject>(mcPreviewHunterVisualController, "getParts(app.ArmorDef.ARMOR_PARTS)", { (void*)ArmorParts::ARMS });
        REApi::ManagedObject* coilGameObj = REInvokePtr<REApi::ManagedObject>(mcPreviewHunterVisualController, "getParts(app.ArmorDef.ARMOR_PARTS)", { (void*)ArmorParts::COIL });
        REApi::ManagedObject* legsGameObj = REInvokePtr<REApi::ManagedObject>(mcPreviewHunterVisualController, "getParts(app.ArmorDef.ARMOR_PARTS)", { (void*)ArmorParts::LEGS });

        pInfo.Transform_helm = (helmGameObj) ? REInvokePtr<REApi::ManagedObject>(helmGameObj, "get_Transform", {}) : nullptr;
        pInfo.Transform_body = (bodyGameObj) ? REInvokePtr<REApi::ManagedObject>(bodyGameObj, "get_Transform", {}) : nullptr;
        pInfo.Transform_arms = (armsGameObj) ? REInvokePtr<REApi::ManagedObject>(armsGameObj, "get_Transform", {}) : nullptr;
        pInfo.Transform_coil = (coilGameObj) ? REInvokePtr<REApi::ManagedObject>(coilGameObj, "get_Transform", {}) : nullptr;
        pInfo.Transform_legs = (legsGameObj) ? REInvokePtr<REApi::ManagedObject>(legsGameObj, "get_Transform", {}) : nullptr;

        pInfo.Slinger_GameObject = REInvokePtr<REApi::ManagedObject>(mcPreviewHunterVisualController, "getParts(app.ArmorDef.ARMOR_PARTS)", { (void*)ArmorParts::SLINGER });

        bool foundRequired = (pInfo.Transform_base &&
            pInfo.Transform_body &&
            pInfo.Transform_arms &&
            pInfo.Transform_coil &&
            pInfo.Transform_legs);

        return foundRequired;
    }

    bool PlayerTracker::fetchPlayer_WeaponObjects(const PlayerInfo& info, PersistentPlayerInfo& pInfo) {
        if (pInfo.Transform_base == nullptr) return false;

        // TOOD: Could grab these from HunterCharacter::get_Weapon() / ::get_ReserveWeapon() / get_SubWeapon() / get_ReserveSubWeapon()
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

        // Kinsect
        if (info.optionalPointers.HunterCharacter) {
            REApi::ManagedObject* Wp_Insect        = REInvokePtr<REApi::ManagedObject>(info.optionalPointers.HunterCharacter, "get_Wp10Insect", {});
            REApi::ManagedObject* Wp_ReserveInsect = REInvokePtr<REApi::ManagedObject>(info.optionalPointers.HunterCharacter, "get_ReserveWp10Insect", {});
        
            if (Wp_Insect)        pInfo.Wp_Insect        = REInvokePtr<REApi::ManagedObject>(Wp_Insect,        "get_GameObject", {});
            if (Wp_ReserveInsect) pInfo.Wp_ReserveInsect = REInvokePtr<REApi::ManagedObject>(Wp_ReserveInsect, "get_GameObject", {});
        }

        return (pInfo.Wp_Parent_GameObject && pInfo.WpSub_Parent_GameObject && pInfo.Wp_ReserveParent_GameObject && pInfo.WpSub_ReserveParent_GameObject);
    }

    bool PlayerTracker::fetchPlayer_Bones(const PlayerInfo& info, PersistentPlayerInfo& pInfo) {
        if (info.pointers.Transform == nullptr) return false;
        if (!pInfo.Transform_body) return false;
        if (!pInfo.Transform_legs) return false;

        pInfo.boneManager = BoneManager{
            dataManager,
            pInfo.armourInfo,
            pInfo.Transform_base,
            pInfo.Transform_helm,
            pInfo.Transform_body,
            pInfo.Transform_arms,
            pInfo.Transform_coil,
            pInfo.Transform_legs,
            info.playerData.female };

        return pInfo.boneManager->isInitialized();
    }

    bool PlayerTracker::fetchPlayer_Parts(const PlayerInfo& info, PersistentPlayerInfo& pInfo) {
        if (info.pointers.Transform == nullptr) return false;
        if (!pInfo.Transform_body) return false;
        if (!pInfo.Transform_legs) return false;

        pInfo.partManager = PartManager{
            dataManager,
            pInfo.armourInfo,
            pInfo.Transform_base,
            pInfo.Transform_helm,
            pInfo.Transform_body,
            pInfo.Transform_arms,
            pInfo.Transform_coil,
            pInfo.Transform_legs,
            info.playerData.female };

        return pInfo.partManager->isInitialized();
    }

    bool PlayerTracker::fetchPlayer_Materials(const PlayerInfo& info, PersistentPlayerInfo& pInfo) {
        if (info.pointers.Transform == nullptr) return false;
        if (!pInfo.Transform_body) return false;

        pInfo.materialManager = MaterialManager{
            dataManager,
            pInfo.armourInfo,
            pInfo.Transform_base,
            pInfo.Transform_helm,
            pInfo.Transform_body,
            pInfo.Transform_arms,
            pInfo.Transform_coil,
            pInfo.Transform_legs,
            info.playerData.female };

        return pInfo.materialManager->isInitialized();
    }

    REApi::ManagedObject* PlayerTracker::getSaveDataObject(int saveIdx) {
        if (saveIdx >= 0) {
            return REInvokePtr<REApi::ManagedObject>(
                saveDataManager.get(),
                "getUserSaveData(System.Int32)",
                { (void*)saveIdx });
        }

        return REInvokePtr<REApi::ManagedObject>(
            saveDataManager.get(),
            "getCurrentUserSaveData",
            {});
    }

    bool PlayerTracker::isSaveActive(REApi::ManagedObject* save) {
        if (save == nullptr) return false;

        char* activeByte = re_memory_ptr<char>(save, 0x3AC);
        return activeByte && *activeByte != 0;
    }

    bool PlayerTracker::getSavePlayerData(int saveIdx, PlayerData& out) {
        if (saveIdx < 0 || saveIdx >= 3) return false; // Invalid save index

        out = PlayerData{};

		REApi::ManagedObject* currentSaveData = getSaveDataObject(saveIdx);
        if (currentSaveData == nullptr || !isSaveActive(currentSaveData)) return false;

        REApi::ManagedObject* cBasicParam = REInvokePtr<REApi::ManagedObject>(currentSaveData, "get_BasicData", {});
        if (cBasicParam == nullptr) return false;

        REApi::ManagedObject* cCharacterEdit_Hunter = REInvokePtr<REApi::ManagedObject>(currentSaveData, "get_CharacterEdit_Hunter", {});
        if (cCharacterEdit_Hunter == nullptr) return false;

        std::string playerName = REFieldStr(cBasicParam, "CharName", REStringType::SYSTEM_STRING);
        if (playerName.empty()) return false;

        std::string hunterShortId = REFieldStr(currentSaveData, "HunterShortId", REStringType::SYSTEM_STRING);
        if (hunterShortId.empty()) return false;

        int* genderIdentity = REFieldPtr<int>(cCharacterEdit_Hunter, "GenderIdentity");
        if (genderIdentity == nullptr) return false;
        bool female = *genderIdentity == 1;

        out.name     = playerName;
        out.hunterId = hunterShortId;
        out.female   = female;

        return true;
    }

    bool PlayerTracker::getActiveSavePlayerData(PlayerData& out) {
        out = PlayerData{};

        REApi::ManagedObject* currentSaveData = getSaveDataObject();
        if (currentSaveData == nullptr || !isSaveActive(currentSaveData)) return false;

        REApi::ManagedObject* cBasicParam = REInvokePtr<REApi::ManagedObject>(currentSaveData, "get_BasicData", {});
        if (cBasicParam == nullptr) return false;

        REApi::ManagedObject* cCharacterEdit_Hunter = REInvokePtr<REApi::ManagedObject>(currentSaveData, "get_CharacterEdit_Hunter", {});
        if (cCharacterEdit_Hunter == nullptr) return false;

        std::string playerName = REFieldStr(cBasicParam, "CharName", REStringType::SYSTEM_STRING);
        if (playerName.empty()) return false;

        std::string hunterShortId = REFieldStr(currentSaveData, "HunterShortId", REStringType::SYSTEM_STRING);
        if (hunterShortId.empty()) return false;

        int* genderIdentity = REFieldPtr<int>(cCharacterEdit_Hunter, "GenderIdentity");
        if (genderIdentity == nullptr) return false;
        bool female = *genderIdentity == 1;

        out.name = playerName;
        out.hunterId = hunterShortId;
        out.female = female;

        return true;
    }

    std::optional<ArmourSet> PlayerTracker::getArmourForPartFromSave(
        REApi::ManagedObject* save,
        REApi::ManagedObject* equip,
        REApi::ManagedObject* outerSet,
        REApi::ManagedObject* visible,
        ArmorParts part,
        bool overrideInner)
    {
        auto outerInfo = getSaveOuterPartInfo(outerSet, part);
        if (!outerInfo) return std::nullopt;

        const auto& [outerSeries, female] = *outerInfo;

        bool isVisible = overrideInner ? false : REInvoke<bool>(
            visible,
            "isVisibleArmor(app.ArmorDef.ARMOR_PARTS)",
            { (void*)part },
            InvokeReturnType::BOOL);

        ArmorSetID id{};

        if (isVisible) {
            id = resolveSaveVisibleArmour(equip, part, outerSeries, female);
        }
        else {
            auto inner = resolveSaveInnerArmour(save, part);
            if (!inner) return std::nullopt;
            id = *inner;
        }

        return ArmourDataManager::get().getArmourSetFromArmourID(id);
    }

    std::optional<std::pair<uint32_t, bool>> PlayerTracker::getSaveOuterPartInfo(REApi::ManagedObject* outerSet, ArmorParts part) {
        REApi::ManagedObject* armorParam = REInvokePtr<REApi::ManagedObject>(outerSet, "get_Armor()", {});
        if (!armorParam) return std::nullopt;

        REApi::ManagedObject* partParam = REInvokePtr<REApi::ManagedObject>(armorParam, "get_Item(System.Int32)", { (void*)static_cast<size_t>(part) });
        if (!partParam) return std::nullopt;

        uint32_t* series = REFieldPtr<uint32_t>(partParam, "Series");
        uint32_t* gender = REFieldPtr<uint32_t>(partParam, "Gender");
        if (!series || !gender) return std::nullopt;

        return { { *series, (*gender == 1) } };
    }

    ArmorSetID PlayerTracker::resolveSaveVisibleArmour(
        REApi::ManagedObject* equip,
        ArmorParts part,
        uint32_t outerSeries,
        bool female
    ) {
        // If outer exists, use it.
        if (outerSeries != 0)
            return ArmourDataManager::getArmourSetIDFromArmourSeries(outerSeries, female);

        // Otherwise read equipped.
        REApi::ManagedObject* equipIndex = REInvokePtr<REApi::ManagedObject>(equip, "get_EquipIndex", {});
        REApi::ManagedObject* equipBox   = REInvokePtr<REApi::ManagedObject>(equip, "get_EquipBox", {});
        if (!equipIndex || !equipBox) return {};

        REApi::ManagedObject* indices = REFieldPtr<REApi::ManagedObject>(equipIndex, "Index");
        if (!indices) return {};

        size_t equippedIdx = REInvoke<size_t>(
            indices,
            "get_Item(System.Int32)",
            { (void*)(static_cast<size_t>(part) + 1) },
            InvokeReturnType::DWORD);

        REApi::ManagedObject* equipData =
            REInvokePtr<REApi::ManagedObject>(
                equipBox,
                "get_Item(System.Int32)",
                { (void*)equippedIdx });

        if (!equipData) return {};

        uint32_t* series = REFieldPtr<uint32_t>(equipData, "FreeVal0");
        if (!series) return {};

        return ArmourDataManager::getArmourSetIDFromArmourSeries(*series, female);
    }

    bool PlayerTracker::resolveHunterAndController(
        PlayerInfo& outInfo,
        const PlayerData& hunter,
        REApi::ManagedObject*& controllerOut,
        REApi::ManagedObject*& hunterTransformCache,
        REApi::ManagedObject*& sceneControllerCache,
        const char* transformPrefixXX,
        const char* transformPrefixXY,
        const char* sceneControllerName,
        const char* componentTypeName
    ) {
        bool usedCache = false;

        if (hunterTransformCache && sceneControllerCache) {
            static const REApi::TypeDefinition* def_ViaTransform = REApi::get()->tdb()->find_type("via.Transform");

            if (checkREPtrValidity(hunterTransformCache, def_ViaTransform)) {
                outInfo.pointers.Transform = hunterTransformCache;
                controllerOut = sceneControllerCache;
                usedCache = true;
            }
        }

        if (usedCache) return true;

        REApi::ManagedObject* currentScene = getCurrentScene();
        if (!currentScene) return false;

        static const REApi::ManagedObject* transformType = REApi::get()->typeof("via.Transform");

        REApi::ManagedObject* transformComponents = REInvokePtr<REApi::ManagedObject>(currentScene, "findComponents(System.Type)", { (void*)transformType });

        const int numComponents = REInvoke<int>(transformComponents, "GetLength", { (void*)0 }, InvokeReturnType::DWORD);

        bool hunterFound = false;
        bool sceneControllerFound = false;

        for (int i = 0; i < numComponents; i++) {
            REApi::ManagedObject* transform = REInvokePtr<REApi::ManagedObject>(transformComponents, "get_Item", { (void*)i });
            if (!transform) continue;

            REApi::ManagedObject* gameObject = REInvokePtr<REApi::ManagedObject>(transform, "get_GameObject", {});
            if (!gameObject) continue;

            std::string name = REInvokeStr(gameObject, "get_Name", {});

            if (name.starts_with(hunter.female ? transformPrefixXX : transformPrefixXY)) {
                outInfo.pointers.Transform = transform;
                hunterTransformCache = transform;
                hunterFound = true;
            }
            else if (name == sceneControllerName) {
                REApi::ManagedObject* controller = getComponent(gameObject, componentTypeName);
                controllerOut = controller;
                sceneControllerCache = controller;
                sceneControllerFound = true;
            }

            if (hunterFound && sceneControllerFound) break;
        }

        return hunterFound;
    }


    std::optional<ArmorSetID> PlayerTracker::resolveSaveInnerArmour(
        REApi::ManagedObject* save,
        ArmorParts part)
    {
        if (part == ArmorParts::HELM)
            return ArmorSetID{};  // matches previous DEFAULT behaviour

        REApi::ManagedObject* edit =
            REInvokePtr<REApi::ManagedObject>(save, "get_CharacterEdit_Hunter", {});
        if (!edit)
            return std::nullopt;

        const char* fn = nullptr;
        switch (part) {
        case ArmorParts::BODY: fn = "get_ChestInner"; break;
        case ArmorParts::ARMS: fn = "get_ArmsInner";  break;
        case ArmorParts::COIL: fn = "get_WaistInner"; break;
        case ArmorParts::LEGS: fn = "get_LegsInner";  break;
        default: return std::nullopt;
        }

        size_t innerIdx = REInvoke<size_t>(edit, fn, {}, InvokeReturnType::DWORD);

        return REInvokeStatic<ArmorSetID>(
            "app.ArmorUtil",
            "getArmorSetIDFromInnerStyle(app.characteredit.Definition.INNER_STYLE)",
            { (void*)innerIdx },
            InvokeReturnType::WORD);
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
        if (idx < 0 || idx >= playerListSize) return REFRAMEWORK_HOOK_CALL_ORIGINAL;

        playersToFetch[static_cast<size_t>(idx)] = true;

        return REFRAMEWORK_HOOK_CALL_ORIGINAL;
    }

    void PlayerTracker::clearPlayerSlot(size_t index) {
        if (index >= playerInfos.size()) return;
        if (playerInfos[index]) {
            playerSlotTable.erase(playerInfos[index]->playerData);
            occupiedNormalGameplaySlots[index] = false;
            playerInfos[index]           = std::nullopt;
            persistentPlayerInfos[index] = std::nullopt;
        }
    }

}