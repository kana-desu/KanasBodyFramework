#pragma once

#include <kbf/situation/known_situation.hpp>
#include <kbf/situation/custom_situation.hpp>

#include <reframework/API.hpp>

#include <unordered_set>

using REApi = reframework::API;

namespace kbf {

    class SituationWatcher {
    public:
        static SituationWatcher& get();

        static bool isMultiplayerSafe() { return get().multiplayerSafe; }
        static bool inSituation(KnownSituation situation) { return get().currentSituations.contains(situation); }
        static bool inCustomSituation(CustomSituation situation) { return get().checkCustomSituation(situation); }

    private:
        SituationWatcher() { initialize(); }
        void initialize();

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
        REApi::ManagedObject* MasterFieldManager = nullptr;
        REApi::ManagedObject* stageController_GuildCard  = nullptr;
        REApi::ManagedObject* stageController_CharaMake  = nullptr;
        REApi::ManagedObject* stageController_SaveSelect = nullptr;

        // Cutscene Tracker Singleton
        REApi::ManagedObject* CutScenePropsControllerManager = nullptr;
        int currentCutsceneId = -1;
    };

}