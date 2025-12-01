#pragma once

#include <kbf/npc/npc_info.hpp>
#include <kbf/npc/persistent_npc_info.hpp>
#include <kbf/npc/npc_cache.hpp>
#include <kbf/npc/npc_fetch_flags.hpp>
#include <kbf/situation/lobby_type.hpp>
#include <kbf/situation/custom_situation.hpp>

#include <unordered_set>

#include <kbf/util/re_engine/re_singleton.hpp>

// UPDATE NOTE: This size may change with future updates.

#define NPC_LIST_SIZE 681
#define TRY_FETCH_LIMIT 100

namespace kbf {

    class NpcTracker {
    public:
        NpcTracker(KBFDataManager& dataManager) : dataManager{ dataManager } { initialize(); };

        void updateNpcs();
        void applyPresets();
        void reset();

        const std::vector<size_t> getNpcList() const;

        const NpcInfo& getNpcInfo(size_t idx) const { return *npcInfos.at(idx); }
        NpcInfo& getNpcInfo(size_t idx) { return *npcInfos.at(idx); }

		const PersistentNpcInfo* getPersistentNpcInfo(size_t idx) const { return persistentNpcInfos.at(idx).get(); }
		PersistentNpcInfo* getPersistentNpcInfo(size_t idx) { return persistentNpcInfos.at(idx).get(); }

    private:
        void initialize();
		KBFDataManager& dataManager;
        static NpcTracker* g_instance;

        void fetchNpcs();
        void fetchNpcs_MainMenu();
        bool fetchNpcs_MainMenu_BasicInfo(NpcInfo& almaInfo, NpcInfo& erikInfo);
        bool fetchNpcs_MainMenu_PersistentInfo(const NpcInfo& info, PersistentNpcInfo& pInfo);
        bool fetchNpcs_MainMenu_EquippedArmourSet(const NpcInfo& outInfo, PersistentNpcInfo& pInfo);
        void fetchNpcs_NormalGameplay();
        void fetchNpcs_NormalGameplay_SingleNpc(size_t i, bool useCache);
        NpcFetchFlags fetchNpc_BasicInfo(size_t i, NpcInfo& out);
        bool fetchNpc_PersistentInfo(size_t i, const NpcInfo& info, PersistentNpcInfo& pInfo);
		void fetchNpc_Visibility(NpcInfo& info);
        bool fetchNpc_EquippedArmourSet(const NpcInfo& info, PersistentNpcInfo& pInfo);
		bool fetchNpc_ArmourTransforms(const NpcInfo& info, PersistentNpcInfo& pInfo);
		bool fetchNpc_Bones(const NpcInfo& info, PersistentNpcInfo& pInfo);
        bool fetchNpc_Parts(const NpcInfo& info, PersistentNpcInfo& pInfo);
        bool fetchNpc_Materials(const NpcInfo& info, PersistentNpcInfo& pInfo);

		static std::string armourIdFromPrefabPath(const std::string& prefabPath);
        void clearNpcSlot(size_t index);

        REApi::ManagedObject* getVolumeOccludeeComponentExhaustive(REApi::ManagedObject* obj, const char* nameFilter) const;
        REApi::ManagedObject* getCurrentScene() const;

		void updateApplyDelays();

        static int onNpcChangeStateHook(int argc, void** argv, REFrameworkTypeDefinitionHandle* arg_tys, unsigned long long ret_addr);
        int onNpcChangeState(REApi::ManagedObject* app_NpcCharacterCore);

        std::mutex fetchListMutex;

        std::unordered_set<size_t> npcSlotTable{};
        std::array<bool, NPC_LIST_SIZE> npcsToFetch;
        std::array<size_t, NPC_LIST_SIZE> tryFetchCountTable{};
        std::unordered_map<size_t, std::optional<std::chrono::steady_clock::time_point>> npcApplyDelays{};
        std::array<std::unique_ptr<NpcInfo>, NPC_LIST_SIZE> npcInfos;
		std::array<std::unique_ptr<PersistentNpcInfo>, NPC_LIST_SIZE> persistentNpcInfos;

        std::array<std::optional<NormalGameplayNpcCache>, NPC_LIST_SIZE> npcInfoCaches;

        // Main Menu Refs
        RENativeSingleton sceneManager{ "via.SceneManager" };
        std::optional<MainMenuNpcCache> mainMenuAlmaCache;
        std::optional<MainMenuNpcCache> mainMenuErikCache;

        // Character Creator Refs
        reframework::API::ManagedObject* characterCreatorNamedNpcTransformCache = nullptr;
        std::optional<size_t> characterCreatorHashedArmourTransformsCache = std::nullopt;

        // Normal Gameplay Refs
        RESingleton npcManager{ "app.NpcManager" };
        bool needsAllNpcFetch = false;

        std::optional<CustomSituation> lastSituation = std::nullopt;
        size_t frameBoneFetchCount = 0;

        // Cutscene end tracking
        bool frameIsCutscene    = false;
    };

}