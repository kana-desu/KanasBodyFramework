#include <kbf/npc/npc_tracker.hpp>
#include <kbf/situation/situation_watcher.hpp>

#include <kbf/hook/hook_manager.hpp>

#include <kbf/npc/armour_id_to_npc_id.hpp>
#include <kbf/data/armour/armour_list.hpp>
#include <kbf/data/armour/find_object_armours.hpp>
#include <kbf/util/re_engine/reinvoke.hpp>
#include <kbf/util/string/ptr_to_hex_string.hpp>
#include <kbf/npc/get_npc_name_from_armour.hpp>
#include <kbf/util/re_engine/print_re_object.hpp>
#include <kbf/data/armour/format_full_armour_id.hpp>
#include <kbf/util/re_engine/find_transform.hpp>
#include <kbf/util/re_engine/dump_components.hpp>
#include <kbf/util/re_engine/re_memory_ptr.hpp>
#include <kbf/debug/debug_stack.hpp>

#include <kbf/profiling/cpu_profiler.hpp>

#define NPC_TRACKER_LOG_TAG "[NpcTracker]"

using REApi = reframework::API;

namespace kbf {

	NpcTracker* NpcTracker::g_instance = nullptr;

    void NpcTracker::initialize() {
        g_instance = this;

        auto& api = REApi::get();

        // Main Menu Singletons
        sceneManager = api->get_native_singleton("via.SceneManager");
        assert(sceneManager != nullptr && "Could not get sceneManager!");

        // Normal Gameplay Singletons
        npcManager = api->get_managed_singleton("app.NpcManager");
        assert(npcManager != nullptr && "Could not get NPC Manager");

        kbf::HookManager::add_tdb("app.NpcCharacterCore", "onWarp",        onNpcChangeStateHook, nullptr, false);
        kbf::HookManager::add_tdb("app.NpcCharacterCore", "setupHeadCtrl", onNpcChangeStateHook, nullptr, false);
    }

    const std::vector<size_t> NpcTracker::getNpcList() const {
		return std::vector<size_t>(npcSlotTable.begin(), npcSlotTable.end());
    }

    void NpcTracker::updateNpcs() {
        fetchNpcs();
        updateApplyDelays();
    }

    void NpcTracker::applyPresets() {
        bool inQuest = SituationWatcher::inSituation(isinQuestPlayingasGuest) || SituationWatcher::inSituation(isinQuestPlayingasHost);
        if (dataManager.settings().enableDuringQuestsOnly && !inQuest) return;

        // Additionally consider one extra 'preview preset' for those currently being edited in the GUI
        const Preset* previewedPreset = dataManager.getPreviewedPreset();
        const bool hasPreview = previewedPreset != nullptr;
        const bool applyPreviewUnconditional = hasPreview && previewedPreset->armour == ArmourList::DefaultArmourSet();

		// Only apply to first n NPCs based on distance to camera
        const int maxNPCsToApply = std::max<int>(dataManager.settings().maxConcurrentApplications, 0);

        // Copy slot IDs into a sortable vector
        std::vector<size_t> npcs;
        npcs.reserve(npcSlotTable.size());

        for (size_t idx : npcSlotTable)
            npcs.emplace_back(idx);

        if (maxNPCsToApply > 0) {
            // Partially select closest N NPCs by distance
            std::nth_element(
                npcs.begin(),
                npcs.begin() + std::min<size_t>(maxNPCsToApply, npcs.size()),
                npcs.end(),
                [&](size_t a, size_t b) {
                    return npcInfos[a]->distanceFromCameraSq < npcInfos[b]->distanceFromCameraSq;
                }
            );
        }

        size_t npcLimit = maxNPCsToApply > 0 ? std::min<size_t>(maxNPCsToApply, npcs.size()) : npcs.size();

        // Apply only to the N closest
        for (size_t i = 0; i < npcLimit; ++i) {
            size_t idx = npcs[i];

            if (npcInfos[idx] == nullptr) continue;
            if (npcApplyDelays[idx] && npcApplyDelays[idx].has_value()) continue;

			const NpcInfo* infoPtr = npcInfos[idx].get();
            if (infoPtr == nullptr) {
                npcApplyDelays.erase(idx);
                continue;
            }

            const NpcInfo& info = *infoPtr;
            if (!info.visible) continue;

            PersistentNpcInfo* pInfo = persistentNpcInfos[idx].get();
			if (pInfo == nullptr) continue;
            if (!pInfo->areSetPointersValid()) {
                persistentNpcInfos[idx] = nullptr;
                continue;
            }

            if (pInfo->boneManager && pInfo->partManager) {
                // Always apply base presets when they are present, but refrain from re-applying the same base preset multiple times.
                std::unordered_set<std::string> presetBasesApplied{};

                for (ArmourPiece piece = ArmourPiece::AP_MIN_EXCLUDING_SET; piece <= ArmourPiece::AP_MAX_EXCLUDING_SLINGER; piece = static_cast<ArmourPiece>(static_cast<int>(piece) + 1)) {
                    std::optional<ArmourSet>& armourPiece = pInfo->armourInfo.getPiece(piece);

                    if (armourPiece.has_value()) {
                        const Preset* preset = dataManager.getActivePreset(pInfo->npcID, info.female, armourPiece.value(), piece);

                        // TODO: Part enables are persistent until transform change, so these *could* be set along with pInfo fetch.
                        bool usePreview = hasPreview && (applyPreviewUnconditional || previewedPreset->armour == armourPiece.value());
                        if (preset == nullptr && !usePreview) continue;

                        const Preset* activePreset = usePreview ? previewedPreset : preset;

                        BoneManager::BoneApplyStatusFlag applyFlag = pInfo->boneManager->applyPreset(activePreset, piece);
                        bool invalidBones = applyFlag == BoneManager::BoneApplyStatusFlag::BONE_APPLY_ERROR_INVALID_BONE;
                        if (invalidBones) { clearNpcSlot(idx); npcsToFetch[idx] = true; break; }

                        pInfo->partManager->applyPreset(activePreset, piece);
						pInfo->materialManager->applyPreset(activePreset, piece);

                        if (!invalidBones && activePreset->set.hasModifiers() && !presetBasesApplied.contains(activePreset->uuid)) {
                            presetBasesApplied.insert(activePreset->uuid);
                            BoneManager::BoneApplyStatusFlag baseApplyFlag = pInfo->boneManager->applyPreset(activePreset, AP_SET);
                            bool invalidBaseBones = baseApplyFlag == BoneManager::BoneApplyStatusFlag::BONE_APPLY_ERROR_INVALID_BONE;
                            if (invalidBaseBones) { clearNpcSlot(idx); npcsToFetch[idx] = true; break; }
                        }
                    }
                }
            }
        }
	}

    void NpcTracker::reset() {
        npcSlotTable.clear();
        npcApplyDelays.clear();
        for (auto& p : npcInfos)           p.reset();
        for (auto& p : persistentNpcInfos) p.reset();

        npcsToFetch.fill(false);

        mainMenuAlmaCache = std::nullopt;
        mainMenuErikCache = std::nullopt;

        characterCreatorNamedNpcTransformCache = nullptr;
        characterCreatorHashedArmourTransformsCache = std::nullopt;
    }

    void NpcTracker::fetchNpcs() {
        bool inQuest = SituationWatcher::inSituation(isinQuestPlayingasGuest) || SituationWatcher::inSituation(isinQuestPlayingasHost);
        if (dataManager.settings().enableDuringQuestsOnly && !inQuest) return;

        frameBoneFetchCount = 0;
        std::optional<CustomSituation> thisUpdateSituation = std::nullopt;

        const bool mainMenu         = SituationWatcher::inCustomSituation(CustomSituation::isInMainMenuScene);
        const bool saveSelect       = SituationWatcher::inCustomSituation(CustomSituation::isInSaveSelectGUI);
        const bool characterCreator = SituationWatcher::inCustomSituation(CustomSituation::isInCharacterCreator);
        const bool guildCard        = SituationWatcher::inCustomSituation(CustomSituation::isInHunterGuildCard);
        const bool cutscene         = SituationWatcher::inCustomSituation(CustomSituation::isInCutscene);

        // Try refetch once after cutscene ends/begins to avoid being untracked.
        needsAllNpcFetch |= (frameIsCutscene && !cutscene) || (!frameIsCutscene && cutscene);
        frameIsCutscene = cutscene;

        if      (mainMenu        ) thisUpdateSituation = CustomSituation::isInMainMenuScene;   
        else if (saveSelect      ) thisUpdateSituation = CustomSituation::isInSaveSelectGUI;   
        else if (characterCreator) thisUpdateSituation = CustomSituation::isInCharacterCreator;
        else if (guildCard       ) thisUpdateSituation = CustomSituation::isInHunterGuildCard; 
        else if (cutscene        ) thisUpdateSituation = CustomSituation::isInCutscene;        

        if (thisUpdateSituation != lastSituation) {
            lastSituation = thisUpdateSituation;
            reset();
		}

        if      (mainMenu        ) fetchNpcs_MainMenu();
        else if (saveSelect      ) return;
        else if (characterCreator) return; // Npcs still show up in the list when in outfit selector so just ignore.
        else if (guildCard       ) return;
        else if (cutscene        ) fetchNpcs_NormalGameplay();
        else                       fetchNpcs_NormalGameplay();
    }

    void NpcTracker::fetchNpcs_MainMenu() {
        NpcInfo almaInfo{};
        NpcInfo erikInfo{};

        bool fetched = fetchNpcs_MainMenu_BasicInfo(almaInfo, erikInfo);
        if (!fetched) return;

        int almaIdx = almaInfo.index;
        int erikIdx = erikInfo.index;

        // ---- Alma ----------------------------------------------------------------------------------
        if (persistentNpcInfos[almaIdx] == nullptr) {
            PersistentNpcInfo almaPInfo{};
            almaPInfo.index = almaIdx;

            bool fetchedPInfo = fetchNpcs_MainMenu_PersistentInfo(almaInfo, almaPInfo);
            if (fetchedPInfo) {
                npcApplyDelays[almaIdx] = std::chrono::high_resolution_clock::now();
                persistentNpcInfos[almaIdx] = std::make_unique<PersistentNpcInfo>(std::move(almaPInfo));
            }
        }

        if (!npcSlotTable.contains(almaIdx)) npcSlotTable.insert(almaIdx);
        npcInfos[almaIdx] = std::make_unique<NpcInfo>(std::move(almaInfo));

        // ---- Erik ----------------------------------------------------------------------------------
        if (persistentNpcInfos[erikIdx] == nullptr) {
            PersistentNpcInfo erikPInfo{};
            erikPInfo.index = erikIdx;

            bool fetchedPInfo = fetchNpcs_MainMenu_PersistentInfo(erikInfo, erikPInfo);
            if (fetchedPInfo) {
                npcApplyDelays[erikIdx] = std::chrono::high_resolution_clock::now();
                persistentNpcInfos[erikIdx] = std::make_unique<PersistentNpcInfo>(std::move(erikPInfo));
            }
        }

        if (!npcSlotTable.contains(erikIdx)) npcSlotTable.insert(erikIdx);
        npcInfos[erikIdx] = std::make_unique<NpcInfo>(std::move(erikInfo));
    }

    bool NpcTracker::fetchNpcs_MainMenu_BasicInfo(NpcInfo& almaInfo, NpcInfo& erikInfo) {
        // This screen is totally horrible. It took many hours for me to find how to distinguish which handler is present here.
        //   Wherever this is in the save data is obfuscated and not immediately apparent, but fortunately their child MeshBoundary objects seems to control visibility. 

        almaInfo = NpcInfo{};
        erikInfo = NpcInfo{};

        bool usedCache = false;
        if (mainMenuAlmaCache.has_value() && mainMenuErikCache.has_value()) {
            // Check the cache hasn't been invalidated
            if (mainMenuAlmaCache.value().isValid() && mainMenuErikCache.value().isValid()) {
                almaInfo.pointers.Transform              = mainMenuAlmaCache.value().Transform;
                almaInfo.optionalPointers.VolumeOccludee = mainMenuAlmaCache.value().VolumeOccludee;
                almaInfo.optionalPointers.MeshBoundary   = mainMenuAlmaCache.value().MeshBoundary;

                erikInfo.pointers.Transform              = mainMenuErikCache.value().Transform;
                erikInfo.optionalPointers.VolumeOccludee = mainMenuErikCache.value().VolumeOccludee;
                erikInfo.optionalPointers.MeshBoundary   = mainMenuErikCache.value().MeshBoundary;

                usedCache = true;
            }
        }

        if (!usedCache) {
            REApi::ManagedObject* currentScene = getCurrentScene();
            if (currentScene == nullptr) return false;

            static const REApi::ManagedObject* transformType = REApi::get()->typeof("via.Transform");
            REApi::ManagedObject* transformComponents = REInvokePtr<REApi::ManagedObject>(currentScene, "findComponents(System.Type)", { (void*)transformType });

            constexpr const char* almaTransformNamePrefix = "NPC102_00_001_00";
            constexpr const char* erikTransformNamePrefix = "NPC101_00_002_00";
            const int numComponents = REInvoke<int>(transformComponents, "GetLength", { (void*)0 }, InvokeReturnType::DWORD);

            bool foundAlma = false;
            bool foundErik = false;
            for (int i = 0; i < numComponents; i++) {
                REApi::ManagedObject* transform = REInvokePtr<REApi::ManagedObject>(transformComponents, "get_Item", { (void*)i });
                if (transform == nullptr) continue;

                REApi::ManagedObject* gameObject = REInvokePtr<REApi::ManagedObject>(transform, "get_GameObject", {});
                if (gameObject == nullptr) continue;

                std::string name = REInvokeStr(gameObject, "get_Name", {});

                static const REApi::ManagedObject* volumeocludeeType = REApi::get()->typeof("via.render.VolumeOccludee");
                static const REApi::ManagedObject* meshBoundaryType = REApi::get()->typeof("ace.MeshBoundary");

                if (!foundAlma && name.starts_with(almaTransformNamePrefix)) {
                    REApi::ManagedObject* volumeOccludee = REInvokePtr<REApi::ManagedObject>(gameObject, "getComponent(System.Type)", { (void*)volumeocludeeType });

                    if (volumeocludeeType != nullptr) {
                        REApi::ManagedObject* meshBoundary = REInvokePtr<REApi::ManagedObject>(gameObject, "getComponent(System.Type)", { (void*)meshBoundaryType });

                        almaInfo.pointers.Transform              = transform;
                        almaInfo.optionalPointers.VolumeOccludee = volumeOccludee;
                        almaInfo.optionalPointers.MeshBoundary   = meshBoundary;
                        mainMenuAlmaCache = MainMenuNpcCache{ transform, volumeOccludee, meshBoundary };
                    }
                }

                if (!foundErik && name.starts_with(erikTransformNamePrefix)) {
                    REApi::ManagedObject* volumeOccludee = REInvokePtr<REApi::ManagedObject>(gameObject, "getComponent(System.Type)", { (void*)volumeocludeeType });

                    if (volumeocludeeType != nullptr) {
                        REApi::ManagedObject* meshBoundary = REInvokePtr<REApi::ManagedObject>(gameObject, "getComponent(System.Type)", { (void*)meshBoundaryType });

                        erikInfo.pointers.Transform              = transform;
                        erikInfo.optionalPointers.VolumeOccludee = volumeOccludee;
                        erikInfo.optionalPointers.MeshBoundary   = meshBoundary;
                        mainMenuErikCache = MainMenuNpcCache{ transform, volumeOccludee, meshBoundary };
                    }
                }
            }
        }

        if (almaInfo.optionalPointers.VolumeOccludee && almaInfo.optionalPointers.MeshBoundary) {
            bool meshEnabled = REInvoke<bool>(almaInfo.optionalPointers.MeshBoundary, "get_IsVisible", {}, InvokeReturnType::BOOL);
            int visible      = REInvoke<int>(almaInfo.optionalPointers.VolumeOccludee, "get_Visibility", {}, InvokeReturnType::DWORD);
            almaInfo.visible = meshEnabled && (visible == 1);
        }
        almaInfo.index = 0;

        if (erikInfo.optionalPointers.VolumeOccludee && erikInfo.optionalPointers.MeshBoundary) {
            bool meshEnabled = REInvoke<bool>(erikInfo.optionalPointers.MeshBoundary, "get_IsVisible", {}, InvokeReturnType::BOOL);
            int visible      = REInvoke<int>(erikInfo.optionalPointers.VolumeOccludee, "get_Visibility", {}, InvokeReturnType::DWORD);
            erikInfo.visible = meshEnabled && (visible == 1);
        }
        erikInfo.index = 1;

        return true;
    }

    bool NpcTracker::fetchNpcs_MainMenu_PersistentInfo(const NpcInfo& info, PersistentNpcInfo& pInfo) {
        bool fetchedArmour = fetchNpcs_MainMenu_EquippedArmourSet(info, pInfo);
        if (!fetchedArmour) return false;

        bool fetchedArmourTransforms = fetchNpc_ArmourTransforms(info, pInfo);
        if (!fetchedArmourTransforms) return false;

        bool fetchedBones = fetchNpc_Bones(info, pInfo);
        if (!fetchedBones) return false;

        bool fetchedParts = fetchNpc_Parts(info, pInfo);
        if (!fetchedParts) return false;

        bool fetchedMats = fetchNpc_Materials(info, pInfo);
        if (!fetchedMats) return false;

        return true;
    }

    bool NpcTracker::fetchNpcs_MainMenu_EquippedArmourSet(const NpcInfo& info, PersistentNpcInfo& pInfo) {
        if (info.pointers.Transform == nullptr) return false;

        pInfo.armourInfo.body = findFirstNPCArmourInObjectFromList(info.pointers.Transform);
        pInfo.npcID = getNpcIDFromArmourSet(pInfo.armourInfo.body.value());

        return pInfo.armourInfo.body.value() != ArmourList::DefaultArmourSet();
    }

    void NpcTracker::fetchNpcs_NormalGameplay() {
        std::unique_lock lock(fetchListMutex);

        const bool useCache = !needsAllNpcFetch;
        for (size_t i = 0; i < NPC_LIST_SIZE; i++) {
            if (needsAllNpcFetch || npcSlotTable.contains(i) || npcsToFetch[i]) {
                fetchNpcs_NormalGameplay_SingleNpc(i, useCache);
            }
        }

        needsAllNpcFetch = false;
    }

    void NpcTracker::fetchNpcs_NormalGameplay_SingleNpc(size_t i, bool useCache) {
        // -- Basic Info --------------------------------------------------------------------------------------------

        NpcInfo info{};
        bool usedCache = false;
        const bool cacheExists = npcInfoCaches[i].has_value();
        const bool cacheValid  = cacheExists && npcInfoCaches[i].value().isValid();
        if (useCache && cacheValid) {
            BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "NPC Fetch - Normal Gameplay - Basic Info");
            BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "NPC Fetch - Normal Gameplay - Basic Info - Cache Load");
            const NormalGameplayNpcCache& slotCache = npcInfoCaches[i].value();
            if (!slotCache.isEmpty()) {
                info.index                         = i;
                info.female                        = slotCache.female;
                info.prefabPath                    = slotCache.prefabPath;
                info.pointers.Transform            = slotCache.Transform;
                info.optionalPointers.Motion       = slotCache.Motion;
                info.optionalPointers.HunterCharacter = slotCache.HunterCharacter;
                usedCache = true;
            }
            END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "NPC Fetch - Normal Gameplay - Basic Info - Cache Load");
            END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "NPC Fetch - Normal Gameplay - Basic Info");
        }

        if (!usedCache) {
            BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "NPC Fetch - Normal Gameplay - Basic Info");
            NpcFetchFlags fetchFlags = fetchNpc_BasicInfo(i, info);
            if (fetchFlags == NpcFetchFlags::FETCH_ERROR_NULL) {
                if (!npcSlotTable.contains(i)) tryFetchCountTable[i] = 0;
                npcInfoCaches[i] = NormalGameplayNpcCache{ .cacheIsEmpty = true };
                END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "NPC Fetch - Normal Gameplay - Basic Info");
                return;
            }
            else if (fetchFlags == NpcFetchFlags::FETCH_UNSUPPORTED) {
                npcsToFetch[i] = false;
                return;
            }
            if (info.pointers.Transform == nullptr) {
                tryFetchCountTable[i]++; 
                npcInfoCaches[i] = NormalGameplayNpcCache{ .cacheIsEmpty = true };
                END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "NPC Fetch - Normal Gameplay - Basic Info");
                return;
            }
            // Update Cached Basic Info
            NormalGameplayNpcCache newCache{};
            newCache.female          = info.female;
            newCache.prefabPath      = info.prefabPath;
            newCache.Transform       = info.pointers.Transform;
            newCache.Motion          = info.optionalPointers.Motion;
            newCache.HunterCharacter = info.optionalPointers.HunterCharacter;
            npcInfoCaches[i] = newCache;
            END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "NPC Fetch - Normal Gameplay - Basic Info");
        }

        // -- Visibility --------------------------------------------------------------------------------------------
        BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "NPC Fetch - Normal Gameplay - Visibility");
        fetchNpc_Visibility(info);
        END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "NPC Fetch - Normal Gameplay - Visibility");
        // ----------------------------------------------------------------------------------------------------------

        if (info.visible && persistentNpcInfos[i] == nullptr && tryFetchCountTable[i] < TRY_FETCH_LIMIT) {
            BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "NPC Fetch - Normal Gameplay - Persistent Info");

            PersistentNpcInfo persistentInfo{};
            persistentInfo.index = i;

            bool fetchedPInfo = fetchNpc_PersistentInfo(i, info, persistentInfo);
            if (fetchedPInfo) {
                npcApplyDelays[i] = std::chrono::high_resolution_clock::now();
                persistentNpcInfos[i] = std::make_unique<PersistentNpcInfo>(std::move(persistentInfo));
                tryFetchCountTable[i] = 0; // Reset try count on success
            }

            END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "NPC Fetch - Normal Gameplay - Persistent Info");
        }

        if (!npcSlotTable.contains(i)) npcSlotTable.insert(i);
        npcInfos[i] = std::make_unique<NpcInfo>(std::move(info));
        npcsToFetch[i] = false; // TODO: Probs need to move this to PInfo loop
    }

    NpcFetchFlags NpcTracker::fetchNpc_BasicInfo(size_t i, NpcInfo& out) {
        // app.cNpcManageInfo
        REApi::ManagedObject* cNpcManageInfo = REInvokePtr<REApi::ManagedObject>(npcManager, "findNpcInfo_NpcId_NoCheck(System.Int32)", { (void*)i });
        if (cNpcManageInfo == nullptr) { clearNpcSlot(i); return NpcFetchFlags::FETCH_ERROR_NULL; } // Npc slot is empty, clear it

        REApi::ManagedObject* GameObject = REInvokePtr<REApi::ManagedObject>(cNpcManageInfo, "get_Object", {});    // via.GameObject
		if (GameObject == nullptr) { return NpcFetchFlags::FETCH_ERROR_NULL; } // It SEEMS like some NPC objects can be empty even if cNpcManageInfo exists...?

        REApi::ManagedObject* Transform = REInvokePtr<REApi::ManagedObject>(GameObject, "get_Transform", {}); // via.Transform

        static const REApi::ManagedObject* typeof_MotionAnimation = REApi::get()->typeof("via.motion.Animation");
        REApi::ManagedObject* Motion = REInvokePtr<REApi::ManagedObject>(GameObject, "getComponent(System.Type)", { (void*)typeof_MotionAnimation }); // via.motion.Animation

        REApi::ManagedObject* NpcAccessor = REInvokePtr<REApi::ManagedObject>(cNpcManageInfo, "get_NpcAccessor", {});
        if (NpcAccessor == nullptr) return NpcFetchFlags::FETCH_ERROR_NULL;

        REApi::ManagedObject* HunterCharacter = REInvokePtr<REApi::ManagedObject>(NpcAccessor, "get_Character", {});
        if (HunterCharacter == nullptr) return NpcFetchFlags::FETCH_ERROR_NULL;

        REApi::ManagedObject* NpcParamHolder = REInvokePtr<REApi::ManagedObject>(NpcAccessor, "get_ParamHolder", {});
        if (NpcParamHolder == nullptr) return NpcFetchFlags::FETCH_ERROR_NULL;

        REApi::ManagedObject* NpcVisualSetting = REInvokePtr<REApi::ManagedObject>(NpcParamHolder, "get_UsedVisualSetting", {});
        if (NpcVisualSetting == nullptr) return NpcFetchFlags::FETCH_ERROR_NULL;

        // Process anything but the palicoes - racist, ik :(
        int species = REInvoke<int>(NpcVisualSetting, "get_Species", {}, InvokeReturnType::DWORD);  // app.NpcDef.SPECIES
        if (species > 1) return NpcFetchFlags::FETCH_UNSUPPORTED; // Non-human npc

        int gender = REInvoke<int>(NpcVisualSetting, "get_Gender", {}, InvokeReturnType::DWORD);   // app.CharacterDef.GENDER

        REApi::ManagedObject* cNpcBaseModelData = REInvokePtr<REApi::ManagedObject>(NpcVisualSetting, "get_ModelData", {});
        if (cNpcBaseModelData == nullptr) return NpcFetchFlags::FETCH_ERROR_NULL;

        REApi::ManagedObject* cNpcVisualDataBase_cBaseModelInfo = REInvokePtr<REApi::ManagedObject>(cNpcBaseModelData, "get_ModelInfo", {});
        if (cNpcVisualDataBase_cBaseModelInfo == nullptr) return NpcFetchFlags::FETCH_ERROR_NULL;

        REApi::ManagedObject* baseModelPrefab = REInvokePtr<REApi::ManagedObject>(cNpcVisualDataBase_cBaseModelInfo, "get_Prefab", {});
        if (baseModelPrefab == nullptr) return NpcFetchFlags::FETCH_ERROR_NULL;
        std::string prefabPath = REInvokeStr(baseModelPrefab, "get_Path", {});

        bool isHunterNpc = prefabPath.empty();

        NpcPointers pointers{};
		pointers.Transform = Transform;

        NpcOptionalPointers optPointers{};
        optPointers.cNpcManageInfo   = cNpcManageInfo;
		optPointers.GameObject       = GameObject;
        optPointers.Motion           = Motion;
        optPointers.NpcAccessor      = NpcAccessor;
        optPointers.HunterCharacter  = HunterCharacter;
        optPointers.NpcParamHolder   = NpcParamHolder;
        optPointers.NpcVisualSetting = NpcVisualSetting;

        out.index            = i;
        out.female           = gender == 1;
        out.pointers         = pointers;
        out.optionalPointers = optPointers;
        out.prefabPath       = prefabPath;
        out.visible          = false;

        return NpcFetchFlags::FETCH_SUCCESS;
    }

    bool NpcTracker::fetchNpc_PersistentInfo(size_t i, const NpcInfo& info, PersistentNpcInfo& pInfo) {
        if (frameBoneFetchCount != 0 && frameBoneFetchCount >= dataManager.settings().maxBoneFetchesPerFrame) return false;

        BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "NPC Fetch - Normal Gameplay - Equipped Armours");
        bool fetchedArmour = fetchNpc_EquippedArmourSet(info, pInfo);
        END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "NPC Fetch - Normal Gameplay - Equipped Armours");
        if (!fetchedArmour) {
            tryFetchCountTable[i]++;
            if (tryFetchCountTable[i] >= TRY_FETCH_LIMIT) {
                DEBUG_STACK.push(std::format("{} Failed to find NPC [{}] Armour info {} times. The NPC is probably invalid, skipping for now...", NPC_TRACKER_LOG_TAG, i, TRY_FETCH_LIMIT), DebugStack::Color::COL_WARNING);
            }
            return false;
        }

        BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "NPC Fetch - Normal Gameplay - Armour Transforms");
        bool fetchedArmourTransforms = fetchNpc_ArmourTransforms(info, pInfo);
        END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "NPC Fetch - Normal Gameplay - Armour Transforms");
        if (!fetchedArmourTransforms) {
            tryFetchCountTable[i]++;
            if (tryFetchCountTable[i] >= TRY_FETCH_LIMIT) {
                DEBUG_STACK.push(std::format("{} Failed to find NPC [{}] Armour Transforms {} times. The NPC is probably invalid, skipping for now...", NPC_TRACKER_LOG_TAG, i, TRY_FETCH_LIMIT), DebugStack::Color::COL_WARNING);
            }
            return false;
        }

        BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "NPC Fetch - Normal Gameplay - Bones");
        bool fetchedBones = fetchNpc_Bones(info, pInfo);
        END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "NPC Fetch - Normal Gameplay - Bones");
        if (!fetchedBones) {
            tryFetchCountTable[i]++;
            if (tryFetchCountTable[i] >= TRY_FETCH_LIMIT) {
                DEBUG_STACK.push(std::format("{} Failed to find NPC [{}] Bones {} times. The NPC is probably invalid, skipping for now...", NPC_TRACKER_LOG_TAG, i, TRY_FETCH_LIMIT), DebugStack::Color::COL_WARNING);
            }
            return false;
        }

        BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "NPC Fetch - Normal Gameplay - Parts");
        bool fetchedParts = fetchNpc_Parts(info, pInfo);
        END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "NPC Fetch - Normal Gameplay - Parts");
        if (!fetchedParts) {
            tryFetchCountTable[i]++;
            if (tryFetchCountTable[i] >= TRY_FETCH_LIMIT) {
                DEBUG_STACK.push(std::format("{} Failed to find NPC [{}] Parts {} times. The NPC is probably invalid, skipping for now...", NPC_TRACKER_LOG_TAG, i, TRY_FETCH_LIMIT), DebugStack::Color::COL_WARNING);
            }
            return false;
        }

        BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "NPC Fetch - Normal Gameplay - Materials");
        bool fetchedMats = fetchNpc_Materials(info, pInfo);
        END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "NPC Fetch - Normal Gameplay - Materials");
        if (!fetchedMats) {
            tryFetchCountTable[i]++;
            if (tryFetchCountTable[i] >= TRY_FETCH_LIMIT) {
                DEBUG_STACK.push(std::format("{} Failed to find NPC [{}] Materials {} times. The NPC is probably invalid, skipping for now...", NPC_TRACKER_LOG_TAG, i, TRY_FETCH_LIMIT), DebugStack::Color::COL_WARNING);
            }
            return false;
        }

        frameBoneFetchCount++; // Consider moving this to top to limit effect of failed fetches - may make fetches inaccessible if there are enough errors though.
        return true;
    }

    void NpcTracker::fetchNpc_Visibility(NpcInfo& info) {
        info.visible = false;
        info.distanceFromCameraSq = FLT_MAX;

        bool motionSkipped = REInvoke<bool>(info.optionalPointers.Motion, "get_SkipUpdate", {}, InvokeReturnType::BOOL);
        if (motionSkipped) return;

        const float& distThreshold = dataManager.settings().applicationRange;
        double sqDist = REInvoke<double>(info.optionalPointers.HunterCharacter, "getCameraDistanceSqXZ", {}, InvokeReturnType::DOUBLE);
        if (distThreshold > 0 && sqDist > distThreshold * distThreshold) return;

        //if (info.optionalPointers.NpcCharacter == nullptr) return;
        //bool inFrustum = REInvoke<bool>(info.optionalPointers.NpcCharacter, "checkInCameraFrustum", {}, InvokeReturnType::BOOL);
        //if (!inFrustum) return;

        info.distanceFromCameraSq = sqDist;
        info.visible = true;
        return;
	}

    bool NpcTracker::fetchNpc_ArmourTransforms(const NpcInfo& info, PersistentNpcInfo& pInfo) {
        if (info.pointers.Transform == nullptr) return false;
        if (!pInfo.armourInfo.body.has_value()) return false;

        // Base transform is fetched every frame
        pInfo.Transform_base = info.pointers.Transform;

        // TODO: Can probably reduce to 1, 100
        constexpr size_t maxDepth = 1;
        constexpr size_t maxBreadth = 100;

        if (pInfo.armourInfo.helm.has_value()) {
            std::string helmId = ArmourList::getArmourId(pInfo.armourInfo.helm.value(), ArmourPiece::AP_HELM, info.female);
            pInfo.Transform_helm = findTransform(info.pointers.Transform, helmId);
        }
        if (pInfo.armourInfo.body.has_value()) {
            std::string bodyId = ArmourList::getArmourId(pInfo.armourInfo.body.value(), ArmourPiece::AP_BODY, info.female);
            pInfo.Transform_body = findTransform(info.pointers.Transform, bodyId);
        }
        if (pInfo.armourInfo.arms.has_value()) {
            std::string armsId = ArmourList::getArmourId(pInfo.armourInfo.arms.value(), ArmourPiece::AP_ARMS, info.female);
            pInfo.Transform_arms = findTransform(info.pointers.Transform, armsId);
        }
        if (pInfo.armourInfo.coil.has_value()) {
            std::string coilId = ArmourList::getArmourId(pInfo.armourInfo.coil.value(), ArmourPiece::AP_COIL, info.female);
            pInfo.Transform_coil = findTransform(info.pointers.Transform, coilId);
        }
        if (pInfo.armourInfo.legs.has_value()) {
            std::string legsId = ArmourList::getArmourId(pInfo.armourInfo.legs.value(), ArmourPiece::AP_LEGS, info.female);
            pInfo.Transform_legs = findTransform(info.pointers.Transform, legsId);
        }
        if (pInfo.armourInfo.slinger.has_value()) {
            std::string slingerId = ArmourList::getArmourId(pInfo.armourInfo.slinger.value(), ArmourPiece::AP_SLINGER, info.female);
            REApi::ManagedObject* slingerTransform = findTransform(info.pointers.Transform, slingerId);
            pInfo.Slinger_GameObject = (slingerTransform) ? REInvokePtr<REApi::ManagedObject>(slingerTransform, "get_GameObject", {}) : nullptr;
        }

        return pInfo.Transform_body;
	}

    bool NpcTracker::fetchNpc_EquippedArmourSet(const NpcInfo& info, PersistentNpcInfo& pInfo) {
        if (info.pointers.Transform == nullptr) return false;
        
        pInfo.armourInfo.body = ArmourList::DefaultArmourSet();

        // Npc will use a special npc armour set, i.e ch04_XXX_XXXX, that can be taken directly from this path.
        if (!info.prefabPath.empty()) {
            std::string strId = armourIdFromPrefabPath(info.prefabPath);
            pInfo.armourInfo.body = ArmourList::getArmourSetFromId(strId);
        }
        else {
            std::array<ArmourSet, 6> foundArmours = findAllArmoursInObjectFromList(info.pointers.Transform, info.female);
            pInfo.armourInfo.helm    = foundArmours[static_cast<size_t>(ArmourPiece::AP_HELM)    - 1];
            pInfo.armourInfo.body    = foundArmours[static_cast<size_t>(ArmourPiece::AP_BODY)    - 1];
            pInfo.armourInfo.arms    = foundArmours[static_cast<size_t>(ArmourPiece::AP_ARMS)    - 1];
            pInfo.armourInfo.coil    = foundArmours[static_cast<size_t>(ArmourPiece::AP_COIL)    - 1];
            pInfo.armourInfo.legs    = foundArmours[static_cast<size_t>(ArmourPiece::AP_LEGS)    - 1];
			pInfo.armourInfo.slinger = foundArmours[static_cast<size_t>(ArmourPiece::AP_SLINGER) - 1];

            // No npcs actually wear helms lol
            const ArmourSet helmPlaceholder{ "Alloy 0", false };
			if (pInfo.armourInfo.helm == helmPlaceholder) pInfo.armourInfo.helm = ArmourList::DefaultArmourSet();
        }

		pInfo.npcID = getNpcIDFromArmourSet(pInfo.armourInfo.body.value());

        return pInfo.armourInfo.body.value() != ArmourList::DefaultArmourSet();
    }

    bool NpcTracker::fetchNpc_Bones(const NpcInfo& info, PersistentNpcInfo& pInfo) {
        if (info.pointers.Transform == nullptr) return false;
        if (!pInfo.armourInfo.body.has_value()) return false;

        pInfo.boneManager = std::make_unique<BoneManager>(
            dataManager, 
            pInfo.armourInfo, 
            pInfo.Transform_base,
            pInfo.Transform_helm,
            pInfo.Transform_body,
            pInfo.Transform_arms,
            pInfo.Transform_coil,
            pInfo.Transform_legs,
            info.female);

        return pInfo.boneManager->isInitialized();
    }

    bool NpcTracker::fetchNpc_Parts(const NpcInfo& info, PersistentNpcInfo& pInfo) {
        if (info.pointers.Transform == nullptr) return false;
        if (!pInfo.Transform_body) return false;
        // Legs are optional for NPCs

        pInfo.partManager = std::make_unique<PartManager>(
            dataManager, 
            pInfo.armourInfo, 
            pInfo.Transform_base,
            pInfo.Transform_helm,
            pInfo.Transform_body,
            pInfo.Transform_arms,
            pInfo.Transform_coil,
            pInfo.Transform_legs,
            info.female);

		return pInfo.partManager->isInitialized();
    }

    bool NpcTracker::fetchNpc_Materials(const NpcInfo& info, PersistentNpcInfo& pInfo) {
        if (info.pointers.Transform == nullptr) return false;
        if (!pInfo.Transform_body) return false;
        // Legs are optional for NPCs

        pInfo.materialManager = std::make_unique<MaterialManager>(
            dataManager,
            pInfo.armourInfo,
            pInfo.Transform_base,
            pInfo.Transform_helm,
            pInfo.Transform_body,
            pInfo.Transform_arms,
            pInfo.Transform_coil,
            pInfo.Transform_legs,
            info.female);

        return pInfo.materialManager->isInitialized();
    }

    std::string NpcTracker::armourIdFromPrefabPath(const std::string& prefabPath) {
        if (prefabPath.empty()) return ANY_ARMOUR_ID;

        auto lastSlash = prefabPath.find_last_of('/');
        auto lastDot   = prefabPath.rfind(".pfb");

        return (lastDot != std::string::npos && lastDot > lastSlash)
            ? prefabPath.substr(lastSlash + 1, lastDot - (lastSlash + 1))
            : prefabPath.substr(lastSlash + 1);
	}

    void NpcTracker::clearNpcSlot(size_t index) {
        if (index >= npcInfos.size()) return;
        if (npcInfos[index]) {
            npcSlotTable.erase(index);
            tryFetchCountTable[index] = 0;
            npcInfoCaches[index]      = std::nullopt;
            npcInfos[index]           = nullptr;
            persistentNpcInfos[index] = nullptr;
        }
	}

    REApi::ManagedObject* NpcTracker::getVolumeOccludeeComponentExhaustive(REApi::ManagedObject* obj, const char* nameFilter) const {        
        static const REApi::ManagedObject* volumeOccludeeType = REApi::get()->typeof("via.render.VolumeOccludee");
        REApi::ManagedObject* volumeOccludeeComponents = REInvokePtr<REApi::ManagedObject>(obj, "findComponents(System.Type)", { (void*)volumeOccludeeType });

        const int numComponents = REInvoke<int>(volumeOccludeeComponents, "GetLength", { (void*)0 }, InvokeReturnType::DWORD);

        for (int i = 0; i < numComponents; i++) {
            REApi::ManagedObject* component = REInvokePtr<REApi::ManagedObject>(volumeOccludeeComponents, "get_Item", { (void*)i });
            if (component == nullptr) continue;

            REApi::ManagedObject* gameObject = REInvokePtr<REApi::ManagedObject>(component, "get_GameObject", {});
            if (gameObject == nullptr) continue;

            std::string name = REInvokeStr(gameObject, "get_Name", {});
            DEBUG_STACK.push(name);
            if (name.starts_with(nameFilter)) {
                return component;
            }
        }

        return nullptr;
    }


    REApi::ManagedObject* NpcTracker::getCurrentScene() const {
        static auto sceneManagerTypeDefinition = REApi::get()->tdb()->find_type("via.SceneManager");
        static auto getCurrentSceneMethodDefinition = sceneManagerTypeDefinition->find_method("get_CurrentScene");

        REApi::ManagedObject* scene = getCurrentSceneMethodDefinition->call<REApi::ManagedObject*>(REApi::get()->get_vm_context(), sceneManager);

        return scene;
    }

    void NpcTracker::updateApplyDelays() {
        std::vector<size_t> timestampsExpired{};
        std::chrono::steady_clock::time_point now = std::chrono::high_resolution_clock::now();

        for (const auto& [npcIdx, optTimestamp] : npcApplyDelays) {
            if (!optTimestamp.has_value()) continue;

            double durationMs = std::chrono::duration<double, std::milli>(now - optTimestamp.value()).count();
            if (durationMs >= dataManager.settings().delayOnEquip * 1000.0f) {
                npcApplyDelays[npcIdx] = std::nullopt;
            }
        }
    }

    int NpcTracker::onNpcChangeStateHook(int argc, void** argv, REFrameworkTypeDefinitionHandle* arg_tys, unsigned long long ret_addr) {
        if (argc < 2) return REFRAMEWORK_HOOK_CALL_ORIGINAL;

        if (g_instance) {
            REApi::ManagedObject* app_NpcCharacterCore = reinterpret_cast<REApi::ManagedObject*>(argv[1]);
            return g_instance->onNpcChangeState(app_NpcCharacterCore);
        }

        return REFRAMEWORK_HOOK_CALL_ORIGINAL;
    }

    int NpcTracker::onNpcChangeState(REApi::ManagedObject* app_NpcCharacterCore) {
        if (app_NpcCharacterCore == nullptr) return REFRAMEWORK_HOOK_CALL_ORIGINAL;

        // TODO: Why are all these the same ptr?

        bool inQuest = SituationWatcher::inSituation(isinQuestPlayingasGuest)
            || SituationWatcher::inSituation(isinQuestPlayingasHost);
        if (dataManager.settings().enableDuringQuestsOnly && !inQuest) return REFRAMEWORK_HOOK_CALL_ORIGINAL;

        REApi::ManagedObject* app_cNpcContextHolder = REFieldPtr<REApi::ManagedObject>(app_NpcCharacterCore, "_ContextHolder", false);
        if (app_cNpcContextHolder == nullptr) return REFRAMEWORK_HOOK_CALL_ORIGINAL;

        REApi::ManagedObject* app_cNpcContext = REInvokePtr<REApi::ManagedObject>(app_cNpcContextHolder, "get_Npc", {});
        if (app_cNpcContext == nullptr) return REFRAMEWORK_HOOK_CALL_ORIGINAL;

        int* idxPtr = re_memory_ptr<int>(app_cNpcContext, 0xEC); //REFieldPtr<int>(app_cNpcContext, "NpcID", true);
        if (idxPtr == nullptr) return REFRAMEWORK_HOOK_CALL_ORIGINAL;
        int idx = *idxPtr;

        if (idx < 0 || idx >= NPC_LIST_SIZE) return REFRAMEWORK_HOOK_CALL_ORIGINAL;

        {
            std::unique_lock lock(fetchListMutex);
            clearNpcSlot(idx);
            npcsToFetch[idx] = true;
        }

        return REFRAMEWORK_HOOK_CALL_ORIGINAL;
    }

}