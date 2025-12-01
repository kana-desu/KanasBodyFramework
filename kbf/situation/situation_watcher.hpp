#pragma once

#include <kbf/situation/known_situation.hpp>
#include <kbf/situation/custom_situation.hpp>

#include <kbf/util/re_engine/check_re_ptr_validity.hpp>

#include <reframework/API.hpp>

#include <unordered_set>

#include <kbf/util/re_engine/re_singleton.hpp>

using REApi = reframework::API;

namespace kbf {

    static inline constexpr const char AppMasterFieldManagerTypeStr[]             = "app.MasterFieldManager";
    static inline constexpr const char AppCutscenePropsControllerManagerTypeStr[] = "app.CutScenePropsControllerManager";
    static inline constexpr const char AppSimpleStageControllerTypeStr[]          = "app.cSimpleStageController";

    class SituationWatcher {
    public:
        static SituationWatcher& get();

        static bool isMultiplayerSafe() { return get().multiplayerSafe; }
        static bool inSituation(KnownSituation situation) { return get().currentSituations.contains(situation); }
        static bool inCustomSituation(CustomSituation situation) { return get().checkCustomSituation(situation); }

    private:
        SituationWatcher() { initialize(); }
        void initialize();
        bool getSingletons();

        static bool checkMultiplayerSafe();
        bool checkCustomSituation(CustomSituation situation);
        void updateCustomSituations();

        // Known Situation Hooks
        static int  situationPreStart(int argc, void** argv, REFrameworkTypeDefinitionHandle* arg_tys, unsigned long long ret_addr);
        static void situationPostStart(void** ret_val, REFrameworkTypeDefinitionHandle ret_ty, unsigned long long ret_addr);

        static int stageControllerPreActivate(int argc, void** argv, REFrameworkTypeDefinitionHandle* arg_tys, unsigned long long ret_addr);
        static int stageControllerPreDeactivate(int argc, void** argv, REFrameworkTypeDefinitionHandle* arg_tys, unsigned long long ret_addr);

        // Custom Situation Hooks
		static int  mainMenuCutsceneOpenPreStart (int argc, void** argv, REFrameworkTypeDefinitionHandle* arg_tys, unsigned long long ret_addr);
        static int  mainMenuGUIOpenPreStart      (int argc, void** argv, REFrameworkTypeDefinitionHandle* arg_tys, unsigned long long ret_addr);
        static void cutsceneStartPostStart       (void** ret_val, REFrameworkTypeDefinitionHandle ret_ty, unsigned long long ret_addr);
		static int  cutsceneEndPreStart          (int argc, void** argv, REFrameworkTypeDefinitionHandle* arg_tys, unsigned long long ret_addr);

        bool initialized     = false;
        bool multiplayerSafe = false;
        std::unordered_set<KnownSituation> currentSituations{};
        std::unordered_set<CustomSituation> customSituations{};

        // Pointers to some app.cSimpleStageControllers for various scenes that we need to track
        RESingleton MasterFieldManager{ "app.MasterFieldManager" };
        REApi::ManagedObject* stageController_GuildCard  = nullptr;
        REApi::ManagedObject* stageController_CharaMake  = nullptr;
        REApi::ManagedObject* stageController_SaveSelect = nullptr;

        // Cutscene Tracker Singleton
        RESingleton CutScenePropsControllerManager{ "app.CutScenePropsControllerManager" };
        int currentCutsceneId = -1;
    };

}