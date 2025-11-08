#pragma once

#include <kbf/data/kbf_data_manager.hpp>
#include <kbf/player/player_cache.hpp>
#include <kbf/player/player_info.hpp>
#include <kbf/player/persistent_player_info.hpp>
#include <kbf/player/player_fetch_flags.hpp>
#include <kbf/situation/lobby_type.hpp>
#include <kbf/situation/situation_watcher.hpp>

#include <unordered_map>
#include <unordered_set>

#define PLAYER_LIST_SIZE 103

namespace kbf {

    class PlayerTracker {
    public:
        PlayerTracker(KBFDataManager& dataManager) : dataManager{ dataManager } { initialize(); };

        void updatePlayers();
        void applyPresets();
        void reset();

        const std::vector<PlayerData> getPlayerList() const;

        const PlayerInfo& getPlayerInfo(const PlayerData& playerData) const { return *playerInfos.at(playerSlotTable.at(playerData)); }
        PlayerInfo& getPlayerInfo(const PlayerData& playerData) { return *playerInfos.at(playerSlotTable.at(playerData)); }

		const PersistentPlayerInfo* getPersistentPlayerInfo(const PlayerData& playerData) const { return persistentPlayerInfos.at(playerSlotTable.at(playerData)).get(); }
		PersistentPlayerInfo* getPersistentPlayerInfo(const PlayerData& playerData) { return persistentPlayerInfos.at(playerSlotTable.at(playerData)).get(); }

    private:
        void initialize();
        KBFDataManager& dataManager;
		static PlayerTracker* g_instance;

        void fetchPlayers();
        void fetchPlayers_MainMenu();
		bool fetchPlayers_MainMenu_BasicInfo(PlayerInfo& outInfo, int& outSaveIdx);
		bool fetchPlayers_MainMenu_WeaponObjects(const PlayerInfo& info, PersistentPlayerInfo& outPInfo);
		void fetchPlayers_SaveSelect();
		bool fetchPlayers_SaveSelect_BasicInfo(PlayerInfo& outInfo);
		bool fetchPlayers_SaveSelect_WeaponObjects(const PlayerInfo& info, PersistentPlayerInfo& outPInfo);
		void fetchPlayers_CharacterCreator();
        bool fetchPlayers_CharacterCreator_BasicInfo(PlayerInfo& outInfo);
		void fetchPlayers_HunterGuildCard();
        bool fetchPlayers_HunterGuildCard_BasicInfo(PlayerInfo& outInfo);
        void fetchPlayers_NormalGameplay();
        void fetchPlayers_NormalGameplay_SinglePlayer(size_t i, bool useCache, bool inQuest, bool online);
        PlayerFetchFlags fetchPlayer_BasicInfo(size_t i, bool inQuest, bool online, PlayerInfo& outInfo);
        bool fetchPlayer_PersistentInfo(size_t i, const PlayerInfo& info, PersistentPlayerInfo& pInfo);
		void fetchPlayer_Visibility(PlayerInfo& info);
        bool fetchPlayer_EquippedArmours(const PlayerInfo& info, PersistentPlayerInfo& pInfo);
		bool fetchPlayer_ArmourTransforms(const PlayerInfo& info, PersistentPlayerInfo& pInfo);
		bool fetchPlayer_WeaponObjects(PersistentPlayerInfo& pInfo);
        bool fetchPlayer_Bones(const PlayerInfo& info, PersistentPlayerInfo& pInfo);
		bool fetchPlayer_Parts(const PlayerInfo& info, PersistentPlayerInfo& pInfo);
        void clearPlayerSlot(size_t index);

        static int onIsEquipBuildEndHook(int argc, void** argv, REFrameworkTypeDefinitionHandle* arg_tys, unsigned long long ret_addr);
        int onIsEquipBuildEnd(int argc, void** argv, REFrameworkTypeDefinitionHandle* arg_tys, unsigned long long ret_addr);
        static int onWarpHook(int argc, void** argv, REFrameworkTypeDefinitionHandle* arg_tys, unsigned long long ret_addr);
		int onWarp(int argc, void** argv, REFrameworkTypeDefinitionHandle* arg_tys, unsigned long long ret_addr);
        static int saveSelectListSelectHook(int argc, void** argv, REFrameworkTypeDefinitionHandle* arg_tys, unsigned long long ret_addr);

        int detectPlayer(void* hunterCharacterPtr, const std::string& logStrSuffix);

        bool getSavePlayerData(int saveIdx, PlayerData& out) const;
        bool getActiveSavePlayerData(PlayerData& out) const;
        REApi::ManagedObject* getCurrentScene() const;

        void updateApplyDelays();

        //std::unordered_set<size_t> playersToFetch{};
        std::unordered_map<PlayerData, size_t> playerSlotTable{};
        std::unordered_map<PlayerData, std::optional<std::chrono::steady_clock::time_point>> playerApplyDelays{};

        std::array<bool, PLAYER_LIST_SIZE> playersToFetch;
        std::array<bool, PLAYER_LIST_SIZE> occupiedNormalGameplaySlots{};
        std::array<std::unique_ptr<PlayerInfo>, PLAYER_LIST_SIZE> playerInfos;
		std::array<std::unique_ptr<PersistentPlayerInfo>, PLAYER_LIST_SIZE> persistentPlayerInfos;

        std::array<std::optional<NormalGameplayPlayerCache>, PLAYER_LIST_SIZE> playerInfoCaches;

        // Main Menu Refs
        void* sceneManager = nullptr;
        reframework::API::ManagedObject* saveDataManager = nullptr;

        // Save Select Refs
		int lastSelectedSaveIdx = -1;
        reframework::API::ManagedObject* saveSelectHunterTransformCache = nullptr;
        std::optional<size_t> saveSelectHashedArmourTransformsCache = std::nullopt;

        // Character Creator Refs
        reframework::API::ManagedObject* characterCreatorHunterTransformCache = nullptr;
        std::optional<size_t> characterCreatorHashedArmourTransformsCache = std::nullopt;

        // Guild Card Refs
        reframework::API::ManagedObject* guiManager = nullptr;
        reframework::API::ManagedObject* guildCardHunterTransformCache = nullptr;
        std::optional<size_t> guildCardHashedArmourTransformsCache = std::nullopt;

        // Normal Gameplay Refs
        reframework::API::ManagedObject* playerManager  = nullptr;
        reframework::API::ManagedObject* networkManager = nullptr;
        reframework::API::ManagedObject* netUserInfoManager = nullptr;
        reframework::API::ManagedObject* netContextManager = nullptr;
		reframework::API::ManagedObject* Net_UserInfoList = nullptr;

        bool needsAllPlayerFetch = false;

        std::optional<CustomSituation> lastSituation = std::nullopt;
        size_t frameBoneFetchCount = 0;

        // Cutscene end tracking
        bool frameIsCutscene = false;

    };

}