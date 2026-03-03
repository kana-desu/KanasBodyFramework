#pragma once

#include <kbf/debug/debug_stack.hpp>
#include <kbf/npc/npc_info.hpp>
#include <kbf/npc/persistent_npc_info.hpp>
#include <kbf/npc/npc_cache.hpp>
#include <kbf/npc/npc_fetch_flags.hpp>
#include <kbf/situation/lobby_type.hpp>
#include <kbf/data/npc/quest_clear_npc_type.hpp>
#include <kbf/situation/custom_situation.hpp>
#include <kbf/situation/known_situation.hpp>

#include <unordered_set>

#include <kbf/util/re_engine/re_singleton.hpp>

// UPDATE NOTE: This size may change with future updates.

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

		const std::optional<PersistentNpcInfo>& getPersistentNpcInfo(size_t idx) const { return persistentNpcInfos.at(idx); }
        std::optional<PersistentNpcInfo>& getPersistentNpcInfo(size_t idx) { return persistentNpcInfos.at(idx); }

    private:
        void initialize();
        void setupLists();
        bool getNpcListSize(size_t& out);

		KBFDataManager& dataManager;
        static NpcTracker* g_instance;
		size_t npcListSize = 0;

        void fetchNpcs();
        void fetchNpcs_MainMenu();
        bool fetchNpcs_MainMenu_BasicInfo(NpcInfo& almaInfo, NpcInfo& erikInfo);
        bool fetchNpcs_MainMenu_PersistentInfo(const NpcInfo& info, PersistentNpcInfo& pInfo);
        bool fetchNpcs_MainMenu_EquippedArmourSet(const NpcInfo& outInfo, PersistentNpcInfo& pInfo);
        void fetchNpcs_NormalGameplay();
        void fetchNpcs_NormalGameplay_SingleNpc(size_t i, bool useCache);
        void fetchNpcs_QuestClearCutscene();
        bool fetchNpcs_QuestClearCutscene_BasicInfo(QuestClearNpcType npcType, NpcInfo& outInfo);
        bool fetchNpcs_QuestClearCutscene_PersistentInfo(const NpcInfo& info, PersistentNpcInfo& pInfo);
        NpcFetchFlags fetchNpc_BasicInfo(size_t i, NpcInfo& out);
        bool fetchNpc_PersistentInfo(size_t i, const NpcInfo& info, PersistentNpcInfo& pInfo);
		void fetchNpc_Visibility(NpcInfo& info);
        bool fetchNpc_EquippedArmourSet(const NpcInfo& info, PersistentNpcInfo& pInfo);
		bool fetchNpc_ArmourTransforms(const NpcInfo& info, PersistentNpcInfo& pInfo, bool searchTransforms = false);
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
        std::vector<bool> npcsToFetch;
        std::vector<size_t> tryFetchCountTable{};
        std::unordered_map<size_t, std::optional<std::chrono::steady_clock::time_point>> npcApplyDelays{};
        std::vector<std::optional<NpcInfo>> npcInfos;
		std::vector<std::optional<PersistentNpcInfo>> persistentNpcInfos;

        std::vector<std::optional<NormalGameplayNpcCache>> npcInfoCaches;

        // Main Menu Refs
        RENativeSingleton sceneManager{ "via.SceneManager" };
        RESingleton saveDataManager{ "app.SaveDataManager" };
        std::optional<MainMenuNpcCache> mainMenuAlmaCache;
        std::optional<MainMenuNpcCache> mainMenuErikCache;

        // Character Creator Refs
        reframework::API::ManagedObject* characterCreatorNamedNpcTransformCache = nullptr;
        std::optional<size_t> characterCreatorHashedArmourTransformsCache = std::nullopt;

        // Normal Gameplay Refs
        RESingleton npcManager{ "app.NpcManager" };
        bool needsAllNpcFetch = false;

        std::optional<std::variant<CustomSituation, KnownSituation>> lastSituation = std::nullopt;
        size_t frameBoneFetchCount = 0;

        // Quest Clear animation Refs
        RESingleton playerManager{ "app.PlayerManager" };
        std::unordered_map<QuestClearNpcType, QuestClearNpcCache> questClearCaches;

        // Cutscene end tracking
        bool frameIsCutscene    = false;

        inline static constexpr FixedString LOG_TAG{ "[NpcTracker]" };
    };

}