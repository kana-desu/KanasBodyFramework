#include <kbf/situation/situation_watcher.hpp>

#include <kbf/hook/hook_manager.hpp>
#include <kbf/util/re_engine/reinvoke.hpp>
#include <kbf/util/re_engine/re_memory_ptr.hpp>
#include <kbf/enums/cutscene_id.hpp>

#include <kbf/util/re_engine/print_re_object.hpp>

#include <string>

#define SITUATION_WATCHER_LOG_TAG "[SituationWatcher]"

namespace kbf {

    SituationWatcher& SituationWatcher::get() {
        static SituationWatcher instance;
        return instance;
    }

    void SituationWatcher::initialize() {
        if (initialized) return;

        std::unique_ptr<reframework::API>& api = reframework::API::get();

        // Get Scene Controllers
        MasterFieldManager = api->get_managed_singleton("app.MasterFieldManager");
        assert(MasterFieldManager != nullptr && "Could not get MasterFieldManager!");
        CutScenePropsControllerManager = api->get_managed_singleton("app.CutScenePropsControllerManager");
        assert(CutScenePropsControllerManager != nullptr && "Could not get CutScenePropsControllerManager");

        stageController_GuildCard = REInvokePtr<REApi::ManagedObject>(MasterFieldManager, "get_GuildCard", {});
        assert(stageController_GuildCard != nullptr && "Could not get GuildCard stage controller!");
        stageController_CharaMake = REInvokePtr<REApi::ManagedObject>(MasterFieldManager, "get_CharaMake", {});
        assert(stageController_CharaMake != nullptr && "Could not get CharaMake stage controller!");
        stageController_SaveSelect = REInvokePtr<REApi::ManagedObject>(MasterFieldManager, "get_SaveSelect", {});
        assert(stageController_SaveSelect != nullptr && "Could not get SaveSelect stage controller!");

        kbf::HookManager::add_tdb(
            "System.Collections.Generic.List`1<app.cGUIMaskContentsManager.SITUATION>", "ToArray",
            situationPreStart, situationPostStart, false);

        // Custom Situation Hooks
        kbf::HookManager::add_tdb(
            "app.cSimpleStageController", "doActivate",
            stageControllerPreActivate, nullptr, false);

        kbf::HookManager::add_tdb(
            "app.cSimpleStageController", "doDeactivate",
            stageControllerPreDeactivate, nullptr, false);

        // Main Menu Open
        kbf::HookManager::add_tdb(
			"app.GUI010100", ".ctor",
            mainMenuGUIOpenPreStart, nullptr, false);

        //Main Menu GUI
        kbf::HookManager::add_tdb(
            "app.GUI010101", "onOpen",
            mainMenuGUIOpenPreStart, nullptr, false);

        kbf::HookManager::add_tdb(
            "app.CutScenePropsControllerManagerBase`1<app.CutScenePropsController>", "startCurrentCutscene(System.Int32)",
            nullptr, cutsceneStartPostStart, false);

        kbf::HookManager::add_tdb(
            "app.CutScenePropsControllerManagerBase`1<app.CutScenePropsController>", "endCurrentCutscene",
            cutsceneEndPreStart, nullptr, false);

        initialized = true;
    }

    // ============ Hooks =======================================================================================================================
    int SituationWatcher::situationPreStart(int argc, void** argv, REFrameworkTypeDefinitionHandle* arg_tys, unsigned long long ret_addr) {
        SituationWatcher& instance = get();
        
        if (argc < 2) return REFRAMEWORK_HOOK_CALL_ORIGINAL;

        auto& api = reframework::API::get();

        reframework::API::ManagedObject* object = static_cast<reframework::API::ManagedObject*>(argv[1]);
        if (!object->is_managed_object()) return REFRAMEWORK_HOOK_CALL_ORIGINAL;

        int count = REInvoke<int>(object, "get_Count", {}, InvokeReturnType::DWORD);

        instance.currentSituations.clear();
        std::string infoSituationStr = "";
        for (int i = 0; i < count; i++) {
            uint32_t rawSituationId = REInvoke<uint32_t>(object, "get_Item", { (void*)i }, InvokeReturnType::DWORD); // this upcast to void* might be sus
            KnownSituation situation = static_cast<KnownSituation>(static_cast<int>(rawSituationId));
            
            if      (situation == KnownSituation::DUPLICATE_isinQuestPlayingasGuest) situation = KnownSituation::isinQuestPlayingasGuest;
            else if (situation == KnownSituation::DUPLICATE_isinTrainingArea)        situation = KnownSituation::isinTrainingArea;

            instance.currentSituations.insert(situation);

            std::string situationName = "UNKNOWN";
            if (SITUATION_NAMES.contains(i)) situationName = SITUATION_NAMES.at(i);
            infoSituationStr += "\n   - [" + std::to_string(i) + "] " + situationName;
        }

        instance.multiplayerSafe = checkMultiplayerSafe();

        // Lastly, a situation change will tell us that we've loaded out of the main menu
        //   This is necessary as these situations don't have a limit on their life-time otherwise.
        //   Lobby search also triggers this, but does not set any flags, so we check that first.
        if (!(instance.currentSituations.size() == 1 && instance.currentSituations.contains(KnownSituation::isAlwaysOn))) {
		    instance.customSituations.erase(CustomSituation::isInMainMenuScene);
		    instance.customSituations.erase(CustomSituation::isInSaveSelectGUI);
            instance.customSituations.erase(CustomSituation::isInTitleMenus);
            instance.customSituations.insert(CustomSituation::isInGame);
        }

        DEBUG_STACK.push(std::format("{} Internal Situations Changed:{}", SITUATION_WATCHER_LOG_TAG, instance.currentSituations.empty() ? "EMPTY" : infoSituationStr));

        return REFRAMEWORK_HOOK_CALL_ORIGINAL;
    }

    void SituationWatcher::situationPostStart(void** ret_val, REFrameworkTypeDefinitionHandle ret_ty, unsigned long long ret_addr) {}

    int SituationWatcher::stageControllerPreActivate(int argc, void** argv, REFrameworkTypeDefinitionHandle* arg_tys, unsigned long long ret_addr) {
        if (argc < 2) return REFRAMEWORK_HOOK_CALL_ORIGINAL;
        SituationWatcher::get().updateCustomSituations();

        return REFRAMEWORK_HOOK_CALL_ORIGINAL;
    }

    int SituationWatcher::stageControllerPreDeactivate(int argc, void** argv, REFrameworkTypeDefinitionHandle* arg_tys, unsigned long long ret_addr) {
        if (argc < 2) return REFRAMEWORK_HOOK_CALL_ORIGINAL;
        SituationWatcher::get().updateCustomSituations();

        return REFRAMEWORK_HOOK_CALL_ORIGINAL;
    }

    int SituationWatcher::mainMenuCutsceneOpenPreStart(int argc, void** argv, REFrameworkTypeDefinitionHandle* arg_tys, unsigned long long ret_addr) {
        SituationWatcher& instance = get();
        
        instance.customSituations.insert(CustomSituation::isInMainMenuScene);
        instance.customSituations.insert(CustomSituation::isInTitleMenus);
		instance.customSituations.erase(CustomSituation::isInSaveSelectGUI);
        instance.customSituations.erase(CustomSituation::isInCutscene);
        instance.customSituations.erase(CustomSituation::isInGame);
        return REFRAMEWORK_HOOK_CALL_ORIGINAL;
	}

    int SituationWatcher::mainMenuGUIOpenPreStart(int argc, void** argv, REFrameworkTypeDefinitionHandle* arg_tys, unsigned long long ret_addr) {
        SituationWatcher& instance = get();
        
        instance.customSituations.insert(CustomSituation::isInMainMenuScene);
        instance.customSituations.insert(CustomSituation::isInTitleMenus);
        instance.customSituations.erase(CustomSituation::isInSaveSelectGUI);
        instance.customSituations.erase(CustomSituation::isInCutscene);
        instance.customSituations.erase(CustomSituation::isInGame);
        return REFRAMEWORK_HOOK_CALL_ORIGINAL;
    }

    void SituationWatcher::cutsceneStartPostStart(void** ret_val, REFrameworkTypeDefinitionHandle ret_ty, unsigned long long ret_addr) {
        SituationWatcher& instance = get();

        instance.customSituations.insert(CustomSituation::isInCutscene);
        int* cutsceneId = re_memory_ptr<int>(instance.CutScenePropsControllerManager, 0xB0);

        instance.currentCutsceneId = cutsceneId ? *cutsceneId : -1;
        DEBUG_STACK.push(std::format("{} Started Cutscene: [{}]", SITUATION_WATCHER_LOG_TAG, instance.currentCutsceneId));
    }

    int SituationWatcher::cutsceneEndPreStart(int argc, void** argv, REFrameworkTypeDefinitionHandle* arg_tys, unsigned long long ret_addr) {
        SituationWatcher& instance = get();
        
        DEBUG_STACK.push(std::format("{} Finished Cutscene: [{}]", SITUATION_WATCHER_LOG_TAG, instance.currentCutsceneId));

        instance.customSituations.erase(CustomSituation::isInCutscene);
        instance.currentCutsceneId = -1;
        return REFRAMEWORK_HOOK_CALL_ORIGINAL;
    }

    bool SituationWatcher::checkMultiplayerSafe() {
        SituationWatcher& instance = get();
        
        if (instance.currentSituations.contains(KnownSituation::isinArenaQuestPlayingasHost)) return false;

        return instance.currentSituations.contains(KnownSituation::isOfflineorMainMenu)
            || instance.currentSituations.contains(KnownSituation::isSoloOnline)
            || instance.currentSituations.contains(KnownSituation::isinTrainingArea);
    }

    bool SituationWatcher::checkCustomSituation(CustomSituation situation) {
        // Do a bit of post-processing here to make states more mutually exclusive;
        switch (situation) {
        case CustomSituation::isInMainMenuScene: {
            return currentCutsceneId == CutsceneID::MAIN_MENU_PROLOGUE;
        }
        case CustomSituation::isInSaveSelectGUI: {
            bool inSaveSelect = customSituations.contains(CustomSituation::isInSaveSelectGUI);
            inSaveSelect &= !customSituations.contains(CustomSituation::isInGame);
            inSaveSelect &= !customSituations.contains(CustomSituation::isInCharacterCreator);
            inSaveSelect &= !customSituations.contains(CustomSituation::isInHunterGuildCard);
            return inSaveSelect;
        }
        case CustomSituation::isInCutscene: {
            bool inCutscene = customSituations.contains(CustomSituation::isInCutscene);
            inCutscene &= currentCutsceneId != CutsceneID::MAIN_MENU_PROLOGUE;
            return inCutscene;
        }
        default:
            return customSituations.contains(situation);
        }
    }

    void SituationWatcher::updateCustomSituations() {
        const bool guildCardIsActive  = REInvoke<bool>(stageController_GuildCard,  "get_IsActive", {}, InvokeReturnType::BOOL);
        const bool charaMakeIsActive  = REInvoke<bool>(stageController_CharaMake,  "get_IsActive", {}, InvokeReturnType::BOOL);
        const bool saveSelectIsActive = REInvoke<bool>(stageController_SaveSelect, "get_IsActive", {}, InvokeReturnType::BOOL);

        if (guildCardIsActive)  customSituations.insert(CustomSituation::isInHunterGuildCard);
        else                    customSituations.erase(CustomSituation::isInHunterGuildCard);
        if (charaMakeIsActive)  customSituations.insert(CustomSituation::isInCharacterCreator);
        else                    customSituations.erase(CustomSituation::isInCharacterCreator);
        if (saveSelectIsActive) customSituations.insert(CustomSituation::isInSaveSelectGUI);
        else                    customSituations.erase(CustomSituation::isInSaveSelectGUI);
    }

}