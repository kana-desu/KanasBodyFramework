#pragma once

#include <kbf/data/armour/armor_set_id.hpp>
#include <kbf/data/armour/armour_piece.hpp>
#include <kbf/util/re_engine/reinvoke.hpp>
#include <kbf/util/re_engine/guid_to_string.hpp>
#include <kbf/util/re_engine/re_singleton.hpp>

#include <unordered_set>

#define ARMOUR_DATA_FETCH_CAP 1000

namespace kbf {

	// Look at natives\STM\GameDesign\Common\Equip ...\ArmorData.user.3 and ...\ArmorSeriesData.user.3 for mappings
	// we can probably extract this info at runtime to auto generate the mapping.

	enum ArmorSeriesModelVariety {
		INVALID = 0b00,
		MALE    = 0b01,
		FEMALE  = 0b10,
		BOTH    = 0b11
	};

	typedef uint8_t ArmorSeriesDisplayRank;
	enum ArmorSeriesDisplayRankFlags {
		RANK_NONE  = 0b000,
		RANK_ALPHA = 0b001,
		RANK_BETA  = 0b010,
		RANK_GAMMA = 0b100
	};

	struct ArmorSeriesData {
		std::string name;
		bool female;
		ArmorSeriesDisplayRank ranks = ArmorSeriesDisplayRankFlags::RANK_NONE;
		ArmourPieceFlags residentPieces = ArmourPieceFlagBits::APF_NONE;
	};

	struct NpcPrefabData {
		std::string name;
		bool femaleCanUse = false;
		bool maleCanUse = false;
	};

	// Note, two ArmorSetIDs can map to the same Armor due to alpha/beta sets.
	using ArmorSeriesIDMap       = std::unordered_map<ArmorSetID, ArmorSeriesData>;
	using NpcPrefabToArmorSetMap = std::unordered_map<std::string, NpcPrefabData>;
	using ArmorSetToSetIDMap     = std::unordered_map<ArmourSet, ArmorSetID>; 
	using ArmorSetToNpcPrefabMap = std::unordered_map<ArmourSet, std::string>;
	using ArmorSetResidentPiecesMap = std::unordered_map<ArmourSet, ArmourPieceFlags>;

	class ArmourDataManager {
	public:
		static ArmourDataManager& get();

		void initialize();
		void uninitialize() { initialized = false; }
		void reinitialize() { uninitialize(); initialize(); }
		std::vector<ArmourSet> getFilteredArmourSets(const std::string& filter);
		static ArmorSetID getArmourSetIDFromArmourSeries(uint32_t series, bool female);
		ArmourSet getArmourSetFromArmourID(const ArmorSetID& setId) const;
		ArmourSet getArmourSetFromNpcPrefab(const std::string& npcPrefabPath, bool female) const;
		std::string getNpcPrefabFromAlias(const std::string& alias) const;
		std::string getNpcPrefabPrimaryTransformName(const std::string& prefabPath);
		REApi::ManagedObject* getNpcPrefabPrimaryTransform(const std::string& prefabPath, REApi::ManagedObject* baseTransform);
		std::vector<std::string> getPartnerCostumePrefabs(size_t partnerId) const;
		std::string getPartnerCostumePrefab(size_t partnerId, size_t costumeId) const;

		bool hasArmourSetMapping(const ArmourSet& set) const;
		ArmourPieceFlags getResidentArmourPieces(const ArmourSet& set) const;
		//bool armourSetHasPiece(const ArmourSet& set, const ArmourPiece& piece) const;
		std::optional<ArmourSet> getArmourSetFromPrefabName(const std::string& prefabName);
		std::optional<ArmorSetID> getArmourSetIDFromArmourSet(const ArmourSet& set) const;
		static std::optional<ArmorSetID> getArmourSetIDFromPrefabName(const std::string& prefabName);
		static std::string getPrefabNameFromArmourSetID(const ArmorSetID& setId, ArmourPiece piece, bool characterFemale);

	private:
		ArmourDataManager() = default;
		bool initialized = false;

		ArmorSeriesIDMap armourSeriesIDMappings;
		NpcPrefabToArmorSetMap npcPrefabToArmourSetMap;
		ArmorSetToSetIDMap knownArmourSeries;
		ArmorSetToNpcPrefabMap knownNpcPrefabs;
		std::unordered_map<std::string, std::string> npcPrefabToPrimaryTransformNameMap;
		std::unordered_map<size_t, std::unordered_map<size_t, std::string>> partnerIdToCostumePrefabMap;

		void getArmourMappings();

		ArmourPieceFlags getResidentArmourPieces(size_t armorSeries) const;

		ArmorSeriesIDMap getArmorSeriesData();
		ArmorSeriesIDMap getHunterArmorData();
		ArmorSeriesIDMap getInnersArmorData();

		NpcPrefabToArmorSetMap getNpcArmorData();
		size_t getNpcArmorDataDefaultSelectors(REApi::ManagedObject* cNpcCatalogHolder, NpcPrefabToArmorSetMap& map, bool verbose = false);
		void getNpcArmorDataUniqueSelectors(REApi::ManagedObject* cNpcCatalogHolder, NpcPrefabToArmorSetMap& map);
		bool addPrefabToArmorSetMap(
			NpcPrefabToArmorSetMap& map, 
			const std::string& npcName,
			const std::string& npcStrID,
			const std::string& prefabPath, 
			size_t variant,
			bool female,
			bool verbose = false);

		static ArmorSeriesDisplayRank getArmorSeriesDisplayRank(const std::string& fullName);
		static std::string getArmorSeriesNameStem(const std::string& fullName);
		static void postProcessArmorSeriesData(ArmorSeriesIDMap& map);

		static std::string getPrefabFromVisualSetting(REApi::ManagedObject* visualSetting);

		RESingleton npcManager{ "app.NpcManager" };

		inline static constexpr FixedString LOG_TAG{ "[ArmourDataManager]" };
	};

}