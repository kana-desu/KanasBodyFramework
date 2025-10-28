﻿#include <kbf/data/armour/armour_list.hpp>

#include <kbf/util/string/to_lower.hpp>

#include <algorithm>

namespace kbf {

    std::vector<ArmourSet> ArmourList::getFilteredSets(const std::string& filter) {
		const bool noFilter = filter.empty();
        std::string filterLower = toLower(filter);

        std::vector<ArmourSet> filteredSets;
        for (const auto& [set, id] : ACTIVE_MAPPING) {
            std::string setLower = toLower(set.name);

            if (noFilter || setLower.find(filterLower) != std::string::npos) {
                 filteredSets.push_back(set);
            }
        }

        std::sort(filteredSets.begin(), filteredSets.end(),
            [](const ArmourSet& a, const ArmourSet& b) {
                if (a.name == ANY_ARMOUR_ID) return true;
                if (b.name == ANY_ARMOUR_ID) return false;
                return a.name < b.name;
            });

        return filteredSets;
	}

    bool ArmourList::isValidArmourSet(const std::string& name, bool female) {
        return ACTIVE_MAPPING.find(ArmourSet{ name, female }) != ACTIVE_MAPPING.end();
    }

    ArmourSet ArmourList::getArmourSetFromId(const std::string& id, ArmourPiece* piece) {
        if (id.empty()) return DefaultArmourSet();

        for (const auto& [set, armourId] : ACTIVE_MAPPING) {
            bool found = false;
            // TODO: Can probably make this more efficient by checking last char, or inverse mapping
            if (armourId.helm    == id) { found = true; if (piece != nullptr) *piece = ArmourPiece::AP_HELM; }
            if (armourId.body    == id) { found = true; if (piece != nullptr) *piece = ArmourPiece::AP_BODY; }
			if (armourId.arms    == id) { found = true; if (piece != nullptr) *piece = ArmourPiece::AP_ARMS; }
			if (armourId.coil    == id) { found = true; if (piece != nullptr) *piece = ArmourPiece::AP_COIL; }
            if (armourId.legs    == id) { found = true; if (piece != nullptr) *piece = ArmourPiece::AP_LEGS; }
			if (armourId.slinger == id) { found = true; if (piece != nullptr) *piece = ArmourPiece::AP_SLINGER; }
                
            if (found) return set;
        }
        return DefaultArmourSet();
	}

    ArmourID ArmourList::getArmourIdFromSet(const ArmourSet& set) {
        if (ACTIVE_MAPPING.find(set) != ACTIVE_MAPPING.end()) {
            return ACTIVE_MAPPING.at(set);
        }
        return { ANY_ARMOUR_ID, ANY_ARMOUR_ID };
	}

    std::string ArmourList::getArmourId(const ArmourSet& set, ArmourPiece piece, bool female) {
        if (ACTIVE_MAPPING.find(set) == ACTIVE_MAPPING.end()) return ANY_ARMOUR_ID;

		const ArmourID& armourId = ACTIVE_MAPPING.at(set);

        std::string femaleId = "";
        switch (piece)
        {
        case ArmourPiece::AP_HELM:    femaleId = armourId.helm; break;
		case ArmourPiece::AP_BODY:    femaleId = armourId.body; break;
		case ArmourPiece::AP_ARMS:    femaleId = armourId.arms; break;
		case ArmourPiece::AP_COIL:    femaleId = armourId.coil; break;
		case ArmourPiece::AP_LEGS:    femaleId = armourId.legs; break;
		case ArmourPiece::AP_SLINGER: femaleId = armourId.slinger; break;
        }

        if (female) return femaleId;

        // ch03 needs to be switched to ch02 for male players, when a non-npc set is provided (i.e. not ch04)
        if (femaleId.size() >= 4 && femaleId[3] != '4')
            femaleId[3] = '2';

        return femaleId;
    }

	const ArmourMapping ArmourList::FALLBACK_MAPPING = {
        // Name                          // Female?  // Helm ID         // Body ID       // Arms ID       // Coil ID       // Legs ID       // Slinger ID
        { ArmourList::DefaultArmourSet()           , { ANY_ARMOUR_ID  , ANY_ARMOUR_ID  , ANY_ARMOUR_ID  , ANY_ARMOUR_ID  , ANY_ARMOUR_ID  , ANY_ARMOUR_ID   } },
        { { "Innerwear 0"                , false } , { ""             , "ch03_002_0002", "ch03_002_0001", "ch03_002_0005", "ch03_002_0004", "ch03_002_0006" } },
        { { "Innerwear 0"                , true  } , { ""             , "ch03_002_0012", "ch03_002_0011", "ch03_002_0015", "ch03_002_0014", "ch03_002_0016" } },
        { { "Innerwear 1"                , false } , { ""             , "ch03_002_1002", "ch03_002_1001", "ch03_002_1005", "ch03_002_1004", "ch03_002_1006" } },
        { { "Innerwear 1"                , true  } , { ""             , "ch03_002_1012", "ch03_002_1011", "ch03_002_1015", "ch03_002_1014", "ch03_002_1016" } },
        { { "Hope 0"                     , false } , { "ch03_001_0003", "ch03_001_0002", "ch03_001_0001", "ch03_001_0005", "ch03_001_0004", "ch03_001_0006" } },
        { { "Hope 0"                     , true  } , { "ch03_001_0013", "ch03_001_0012", "ch03_001_0011", "ch03_001_0015", "ch03_001_0014", "ch03_001_0016" } },
        { { "Leather 0"                  , false } , { "ch03_005_0003", "ch03_005_0002", "ch03_005_0001", "ch03_005_0005", "ch03_005_0004", "ch03_005_0006" } },
        { { "Leather 0"                  , true  } , { "ch03_005_0013", "ch03_005_0012", "ch03_005_0011", "ch03_005_0015", "ch03_005_0014", "ch03_005_0016" } },
        { { "Chainmail 0"                , false } , { "ch03_006_0003", "ch03_006_0002", "ch03_006_0001", "ch03_006_0005", "ch03_006_0004", "ch03_006_0006" } },
        { { "Chainmail 0"                , true  } , { "ch03_006_0013", "ch03_006_0012", "ch03_006_0011", "ch03_006_0015", "ch03_006_0014", "ch03_006_0016" } },
        { { "Bone 0"                     , false } , { "ch03_004_0003", "ch03_004_0002", "ch03_004_0001", "ch03_004_0005", "ch03_004_0004", "ch03_004_0006" } },
        { { "Bone 0"                     , true  } , { "ch03_004_0013", "ch03_004_0012", "ch03_004_0011", "ch03_004_0015", "ch03_004_0014", "ch03_004_0016" } },
        { { "Alloy 0"                    , false } , { "ch03_010_0003", "ch03_010_0002", "ch03_010_0001", "ch03_010_0005", "ch03_010_0004", "ch03_010_0006" } },
        { { "Alloy 0"                    , true  } , { "ch03_010_0013", "ch03_010_0012", "ch03_010_0011", "ch03_010_0015", "ch03_010_0014", "ch03_010_0016" } },
        { { "Bulaqchi 0/1"               , false } , { "ch03_016_0003", "",              "",              "",              ""             , ""              } },
		{ { "Bulaqchi 0/1"               , true  } , { "ch03_016_0013", "",              "",              "",              ""             , ""              } },
        { { "Talioth 0/1"                , false } , { ""             , ""             , "ch03_007_0001", ""             , ""             , ""              } },
        { { "Talioth 0/1"                , true  } , { ""             , ""             , "ch03_007_0011", ""             , ""             , ""              } },
        { { "Piragill 0/1"               , false } , { ""             , ""             , ""             , ""             , "ch03_011_0004", ""              } },
        { { "Piragill 0/1"               , true  } , { ""             , ""             , ""             , ""             , "ch03_011_0014", ""              } },
        { { "Vespoid 0/1"                , false } , { "ch03_028_0003", "ch03_028_0002", "ch03_028_0001", "ch03_028_0005", "ch03_028_0004", "ch03_028_0006" } },
        { { "Vespoid 0/1"                , true  } , { "ch03_028_0013", "ch03_028_0012", "ch03_028_0011", "ch03_028_0015", "ch03_028_0014", "ch03_028_0016" } },
        { { "Kranodath 0/1"              , false } , { ""             , "ch03_020_0002", ""             , ""             , ""             , "ch03_020_0006" } },
        { { "Kranodath 0/1"              , true  } , { ""             , "ch03_020_0012", ""             , ""             , ""             , "ch03_020_0016" } },
        { { "Comaqchi 0/1"               , false } , { "ch03_026_0003", "",              "",              "",              ""             , ""              } },
        { { "Comaqchi 0/1"               , true  } , { "ch03_026_0013", "",              "",              "",              ""             , ""              } },
        { { "Kut-Ku 0/1"                 , false } , { "ch03_034_0003", "ch03_034_0002", "ch03_034_0001", "ch03_034_0005", "ch03_034_0004", "ch03_034_0006" } },
        { { "Kut-Ku 0/1"                 , true  } , { "ch03_034_0013", "ch03_034_0012", "ch03_034_0011", "ch03_034_0015", "ch03_034_0014", "ch03_034_0016" } },
        { { "Chatacabra 0/1"             , false } , { "ch03_008_0003", "ch03_008_0002", "ch03_008_0001", "ch03_008_0005", "ch03_008_0004", "ch03_008_0006" } },
        { { "Chatacabra 0/1"             , true  } , { "ch03_008_0013", "ch03_008_0012", "ch03_008_0011", "ch03_008_0015", "ch03_008_0014", "ch03_008_0016" } },
        { { "Quematrice 0/1"             , false } , { "ch03_009_0003", "ch03_009_0002", "ch03_009_0001", "ch03_009_0005", "ch03_009_0004", "ch03_009_0006" } },
        { { "Quematrice 0/1"             , true  } , { "ch03_009_0013", "ch03_009_0012", "ch03_009_0011", "ch03_009_0015", "ch03_009_0014", "ch03_009_0016" } },
        { { "Lala Barina 0/1"            , false } , { "ch03_012_0003", "ch03_012_0002", "ch03_012_0001", "ch03_012_0005", "ch03_012_0004", "ch03_012_0006" } },
        { { "Lala Barina 0/1"            , true  } , { "ch03_012_0013", "ch03_012_0012", "ch03_012_0011", "ch03_012_0015", "ch03_012_0014", "ch03_012_0016" } },
        { { "Conga 0/1"                  , false } , { "ch03_013_0003", "ch03_013_0002", "ch03_013_0001", "ch03_013_0005", "ch03_013_0004", "ch03_013_0006" } },
        { { "Conga 0/1"                  , true  } , { "ch03_013_0013", "ch03_013_0012", "ch03_013_0011", "ch03_013_0015", "ch03_013_0014", "ch03_013_0016" } },
        { { "Rompopolo 0/1"              , false } , { "ch03_018_0003", "ch03_018_0002", "ch03_018_0001", "ch03_018_0005", "ch03_018_0004", "ch03_018_0006" } },
        { { "Rompopolo 0/1"              , true  } , { "ch03_018_0013", "ch03_018_0012", "ch03_018_0011", "ch03_018_0015", "ch03_018_0014", "ch03_018_0016" } },
        { { "Gypceros 0/1"               , false } , { "ch03_037_0003", "ch03_037_0002", "ch03_037_0001", "ch03_037_0005", "ch03_037_0004", "ch03_037_0006" } },
        { { "Gypceros 0/1"               , true  } , { "ch03_037_0013", "ch03_037_0012", "ch03_037_0011", "ch03_037_0015", "ch03_037_0014", "ch03_037_0016" } },
        { { "Nerscylla 0/1"              , false } , { "ch03_022_0003", "ch03_022_0002", "ch03_022_0001", "ch03_022_0005", "ch03_022_0004", "ch03_022_0006" } },
        { { "Nerscylla 0/1"              , true  } , { "ch03_022_0013", "ch03_022_0012", "ch03_022_0011", "ch03_022_0015", "ch03_022_0014", "ch03_022_0016" } },
        { { "Balahara 0/1"               , false } , { "ch03_014_0003", "ch03_014_0002", "ch03_014_0001", "ch03_014_0005", "ch03_014_0004", "ch03_014_0006" } },
        { { "Balahara 0/1"               , true  } , { "ch03_014_0013", "ch03_014_0012", "ch03_014_0011", "ch03_014_0015", "ch03_014_0014", "ch03_014_0016" } },
        { { "Hirabami 0/1"               , false } , { "ch03_023_0003", "ch03_023_0002", "ch03_023_0001", "ch03_023_0005", "ch03_023_0004", "ch03_023_0006" } },
        { { "Hirabami 0/1"               , true  } , { "ch03_023_0013", "ch03_023_0012", "ch03_023_0011", "ch03_023_0015", "ch03_023_0014", "ch03_023_0016" } },
        { { "Rathian 0/1"                , false } , { "ch03_035_0003", "ch03_035_0002", "ch03_035_0001", "ch03_035_0005", "ch03_035_0004", "ch03_035_0006" } },
        { { "Rathian 0/1"                , true  } , { "ch03_035_0013", "ch03_035_0012", "ch03_035_0011", "ch03_035_0015", "ch03_035_0014", "ch03_035_0016" } },
        { { "Ingot 0"                    , false } , { "ch03_015_0003", "ch03_015_0002", "ch03_015_0001", "ch03_015_0005", "ch03_015_0004", "ch03_015_0006" } },
        { { "Ingot 0"                    , true  } , { "ch03_015_0013", "ch03_015_0012", "ch03_015_0011", "ch03_015_0015", "ch03_015_0014", "ch03_015_0016" } },
        { { "Guardian Seikret 0/1"       , false } , { ""             , ""             , ""             , "ch03_033_0005", ""             , ""              } },
        { { "Guardian Seikret 0/1"       , true  } , { ""             , ""             , ""             , "ch03_033_0015", ""             , ""              } },
        { { "Gajau 0"                    , false } , { ""             , ""             , ""             , ""             , "ch03_052_0004", ""              } },
        { { "Gajau 0"                    , true  } , { ""             , ""             , ""             , ""             , "ch03_052_0014", ""              } },
        { { "Guardian Fulgur 0/1"        , false } , { "ch03_043_6003", "ch03_043_6002", "ch03_043_6001", "ch03_043_6005", "ch03_043_6004", "ch03_043_6006" } },
        { { "Guardian Fulgur 0/1"        , true  } , { "ch03_043_6013", "ch03_043_6012", "ch03_043_6011", "ch03_043_6015", "ch03_043_6014", "ch03_043_6016" } },
        { { "Doshaguma 0/1"              , false } , { "ch03_003_0003", "ch03_003_0002", "ch03_003_0001", "ch03_003_0005", "ch03_003_0004", "ch03_003_0006" } },
        { { "Doshaguma 0/1"              , true  } , { "ch03_003_0013", "ch03_003_0012", "ch03_003_0011", "ch03_003_0015", "ch03_003_0014", "ch03_003_0016" } },
        { { "Guardian Doshaguma 0/1"     , false } , { "ch03_003_5003", "ch03_003_5002", "ch03_003_5001", "ch03_003_5005", "ch03_003_5004", "ch03_003_5006" } },
        { { "Guardian Doshaguma 0/1"     , true  } , { "ch03_003_5013", "ch03_003_5012", "ch03_003_5011", "ch03_003_5015", "ch03_003_5014", "ch03_003_5016" } },
        { { "Ajarakan 0/1"               , false } , { "ch03_024_0003", "ch03_024_0002", "ch03_024_0001", "ch03_024_0005", "ch03_024_0004", "ch03_024_0006" } },
        { { "Ajarakan 0/1"               , true  } , { "ch03_024_0013", "ch03_024_0012", "ch03_024_0011", "ch03_024_0015", "ch03_024_0014", "ch03_024_0016" } },
        { { "Guardian Ebony 0/1"         , false } , { "ch03_030_6003", "ch03_030_6002", "ch03_030_6001", "ch03_030_6005", "ch03_030_6004", "ch03_030_6006" } },
        { { "Guardian Ebony 0/1"         , true  } , { "ch03_030_6013", "ch03_030_6012", "ch03_030_6011", "ch03_030_6015", "ch03_030_6014", "ch03_030_6016" } },
        { { "Xu Wu 0/1"                  , false } , { "ch03_031_0003", "ch03_031_0002", "ch03_031_0001", "ch03_031_0005", "ch03_031_0004", "ch03_031_0006" } },
        { { "Xu Wu 0/1"                  , true  } , { "ch03_031_0013", "ch03_031_0012", "ch03_031_0011", "ch03_031_0015", "ch03_031_0014", "ch03_031_0016" } },
        { { "Rathalos 0/1"               , false } , { "ch03_036_0003", "ch03_036_0002", "ch03_036_0001", "ch03_036_0005", "ch03_036_0004", "ch03_036_0006" } },
        { { "Rathalos 0/1"               , true  } , { "ch03_036_0013", "ch03_036_0012", "ch03_036_0011", "ch03_036_0015", "ch03_036_0014", "ch03_036_0016" } },
        { { "Guardian Rathalos 0/1"      , false } , { "ch03_036_5003", "ch03_036_5002", "ch03_036_5001", "ch03_036_5005", "ch03_036_5004", "ch03_036_5006" } },
        { { "Guardian Rathalos 0/1"      , true  } , { "ch03_036_5013", "ch03_036_5012", "ch03_036_5011", "ch03_036_5015", "ch03_036_5014", "ch03_036_5016" } },
        { { "Gravios 0/1"                , false } , { "ch03_040_0003", "ch03_040_0002", "ch03_040_0001", "ch03_040_0005", "ch03_040_0004", "ch03_040_0006" } },
        { { "Gravios 0/1"                , true  } , { "ch03_040_0013", "ch03_040_0012", "ch03_040_0011", "ch03_040_0015", "ch03_040_0014", "ch03_040_0016" } },
        { { "Blango 0/1"                 , false } , { "ch03_041_0003", "ch03_041_0002", "ch03_041_0001", "ch03_041_0005", "ch03_041_0004", "ch03_041_0006" } },
        { { "Blango 0/1"                 , true  } , { "ch03_041_0013", "ch03_041_0012", "ch03_041_0011", "ch03_041_0015", "ch03_041_0014", "ch03_041_0016" } },
        { { "Commission 0"               , false } , { "ch03_050_0003", "ch03_050_0002", "ch03_050_0001", "ch03_050_0005", "ch03_050_0004", "ch03_050_0006" } },
        { { "Commission 0"               , true  } , { "ch03_050_0013", "ch03_050_0012", "ch03_050_0011", "ch03_050_0015", "ch03_050_0014", "ch03_050_0016" } },
        { { "Kunafa 0"                   , false } , { "ch03_046_0003", "ch03_046_0002", ""             , "ch03_046_0005", "ch03_046_0004", "ch03_046_0006" } },
        { { "Azuz 0"                     , false } , { "ch03_047_0003", "ch03_047_0002", "ch03_047_0001", "ch03_047_0005", "ch03_047_0004", "ch03_047_0006" } },
        { { "Suja 0"                     , false } , { ""             , ""             , ""             , "ch03_049_0005", ""             , ""              } },
        { { "Sild 0"                     , false } , { "ch03_048_0003", "ch03_048_0002", ""             , ""             , ""             , "ch03_048_0006" } },
        { { "Death Stench 0/1"           , false } , { "ch03_053_0003", "ch03_053_0002", "ch03_053_0001", "ch03_053_0005", "ch03_053_0004", "ch03_053_0006" } },
        { { "Death Stench 0/1"           , true  } , { "ch03_053_0013", "ch03_053_0012", "ch03_053_0011", "ch03_053_0015", "ch03_053_0014", "ch03_053_0016" } },
        { { "Butterfly 0"                , false } , { "ch03_054_0003", "ch03_054_0002", "ch03_054_0001", "ch03_054_0005", "ch03_054_0004", "ch03_054_0006" } },
        { { "Butterfly 0"                , true  } , { "ch03_054_0013", "ch03_054_0012", "ch03_054_0011", "ch03_054_0015", "ch03_054_0014", "ch03_054_0016" } },
        { { "King Beetle 0"              , false } , { "ch03_055_0003", "ch03_055_0002", "ch03_055_0001", "ch03_055_0005", "ch03_055_0004", "ch03_055_0006" } },
        { { "King Beetle 0"              , true  } , { "ch03_055_0013", "ch03_055_0012", "ch03_055_0011", "ch03_055_0015", "ch03_055_0014", "ch03_055_0016" } },
        { { "High Metal 0"               , false } , { "ch03_056_0003", "ch03_056_0002", "ch03_056_0001", "ch03_056_0005", "ch03_056_0004", "ch03_056_0006" } },
        { { "High Metal 0"               , true  } , { "ch03_056_0013", "ch03_056_0012", "ch03_056_0011", "ch03_056_0015", "ch03_056_0014", "ch03_056_0016" } },
        { { "Battle 0"                   , false } , { "ch03_057_0003", "ch03_057_0002", "ch03_057_0001", "ch03_057_0005", "ch03_057_0004", "ch03_057_0006" } },
        { { "Battle 0"                   , true  } , { "ch03_057_0013", "ch03_057_0012", "ch03_057_0011", "ch03_057_0015", "ch03_057_0014", "ch03_057_0016" } },
        { { "Melahoa 0"                  , false } , { "ch03_058_0003", "ch03_058_0002", "ch03_058_0001", "ch03_058_0005", "ch03_058_0004", "ch03_058_0006" } },
        { { "Melahoa 0"                  , true  } , { "ch03_058_0013", "ch03_058_0012", "ch03_058_0011", "ch03_058_0015", "ch03_058_0014", "ch03_058_0016" } },
        { { "Artian 0"                   , false } , { "ch03_051_0003", "ch03_051_0002", "ch03_051_0001", "ch03_051_0005", "ch03_051_0004", "ch03_051_0006" } },
        { { "Artian 0"                   , true  } , { "ch03_051_0013", "ch03_051_0012", "ch03_051_0011", "ch03_051_0015", "ch03_051_0014", "ch03_051_0016" } },
        { { "Dober 0"                    , false } , { "ch03_019_0003", "ch03_019_0002", "ch03_019_0001", "ch03_019_0005", "ch03_019_0004", "ch03_019_0006" } },
        { { "Dober 0"                    , true  } , { "ch03_019_0013", "ch03_019_0012", "ch03_019_0011", "ch03_019_0015", "ch03_019_0014", "ch03_019_0016" } },
        { { "Damascus 0"                 , false } , { "ch03_025_0003", "ch03_025_0002", "ch03_025_0001", "ch03_025_0005", "ch03_025_0004", "ch03_025_0006" } },
        { { "Damascus 0"                 , true  } , { "ch03_025_0013", "ch03_025_0012", "ch03_025_0011", "ch03_025_0015", "ch03_025_0014", "ch03_025_0016" } },
        { { "Dahaad 0/1"                 , false } , { "ch03_029_0003", "ch03_029_0002", "ch03_029_0001", "ch03_029_0005", "ch03_029_0004", "ch03_029_0006" } },
        { { "Dahaad 0/1"                 , true  } , { "ch03_029_0013", "ch03_029_0012", "ch03_029_0011", "ch03_029_0015", "ch03_029_0014", "ch03_029_0016" } },
        { { "Uth Duna 0/1"               , false } , { "ch03_017_0003", "ch03_017_0002", "ch03_017_0001", "ch03_017_0005", "ch03_017_0004", "ch03_017_0006" } },
        { { "Uth Duna 0/1"               , true  } , { "ch03_017_0013", "ch03_017_0012", "ch03_017_0011", "ch03_017_0015", "ch03_017_0014", "ch03_017_0016" } },
        { { "Rey Dau 0/1"                , false } , { "ch03_021_0003", "ch03_021_0002", "ch03_021_0001", "ch03_021_0005", "ch03_021_0004", "ch03_021_0006" } },
        { { "Rey Dau 0/1"                , true  } , { "ch03_021_0013", "ch03_021_0012", "ch03_021_0011", "ch03_021_0015", "ch03_021_0014", "ch03_021_0016" } },
        { { "Nu Udra 0/1"                , false } , { "ch03_027_0003", "ch03_027_0002", "ch03_027_0001", "ch03_027_0005", "ch03_027_0004", "ch03_027_0006" } },
        { { "Nu Udra 0/1"                , true  } , { "ch03_027_0013", "ch03_027_0012", "ch03_027_0011", "ch03_027_0015", "ch03_027_0014", "ch03_027_0016" } },
        { { "Nu Udra 2"                  , false } , { "ch03_027_3003", "ch03_027_3002", "ch03_027_3001", "ch03_027_3005", "ch03_027_3004", "ch03_027_3006" } },
        { { "Nu Udra 2"                  , true  } , { "ch03_027_3013", "ch03_027_3012", "ch03_027_3011", "ch03_027_3015", "ch03_027_3014", "ch03_027_3016" } },
        { { "Gore 0/1"                   , false } , { "ch03_042_0003", "ch03_042_0002", "ch03_042_0001", "ch03_042_0005", "ch03_042_0004", "ch03_042_0006" } },
        { { "Gore 0/1"                   , true  } , { "ch03_042_0013", "ch03_042_0012", "ch03_042_0011", "ch03_042_0015", "ch03_042_0014", "ch03_042_0016" } },
        { { "Arkveld 0/1"                , false } , { "ch03_032_0003", "ch03_032_0002", "ch03_032_0001", "ch03_032_0005", "ch03_032_0004", "ch03_032_0006" } },
        { { "Arkveld 0/1"                , true  } , { "ch03_032_0013", "ch03_032_0012", "ch03_032_0011", "ch03_032_0015", "ch03_032_0014", "ch03_032_0016" } },
        { { "Guardian Arkveld 0/1"       , false } , { "ch03_032_5003", "ch03_032_5002", "ch03_032_5001", "ch03_032_5005", "ch03_032_5004", "ch03_032_5006" } },
        { { "Guardian Arkveld 0/1"       , true  } , { "ch03_032_5013", "ch03_032_5012", "ch03_032_5011", "ch03_032_5015", "ch03_032_5014", "ch03_032_5016" } },
        { { "Guild Ace 0"                , false } , { "ch03_062_0003", "ch03_062_0002", "ch03_062_0001", "ch03_062_0005", "ch03_062_0004", "ch03_062_0006" } },
        { { "Dragonking 0"               , false } , { "ch03_044_0003", ""             , ""             , ""             , ""             , ""              } },
        { { "Dragonking 0"               , true  } , { "ch03_044_0013", ""             , ""             , ""             , ""             , ""              } },
        { { "Guild Cross 0"              , false } , { "ch03_073_0003", "ch03_073_0002", "ch03_073_0001", "ch03_073_0005", "ch03_073_0004", "ch03_073_0006" } },
        { { "Clerk 0"                    , false } , { "ch03_074_0003", "ch03_074_0002", "ch03_074_0001", "ch03_074_0005", "ch03_074_0004", "ch03_074_0006" } },
        { { "Clerk 0"                    , true  } , { "ch03_074_0013", "ch03_074_0012", "ch03_074_0011", "ch03_074_0015", "ch03_074_0014", "ch03_074_0016" } },
        { { "Gourmand's Earring 0"       , false } , { "ch03_076_1003", ""             , ""             , ""             , ""             , ""              } },
        { { "Gourmand's Earring 0"       , true  } , { "ch03_076_1013", ""             , ""             , ""             , ""             , ""              } },
        { { "Earrings of Dedication 0"   , false } , { "ch03_076_0003", ""             , ""             , ""             , ""             , ""              } },
        { { "Earrings of Dedication 0"   , true  } , { "ch03_076_0013", ""             , ""             , ""             , ""             , ""              } },
        { { "Mizutsune 0/1"              , false } , { "ch03_066_0003", "ch03_066_0002", "ch03_066_0001", "ch03_066_0005", "ch03_066_0004", "ch03_066_0006" } },
        { { "Mizutsune 0/1"              , true  } , { "ch03_066_0013", "ch03_066_0012", "ch03_066_0011", "ch03_066_0015", "ch03_066_0014", "ch03_066_0016" } },
        { { "Numinous 0/1"               , false } , { "ch03_059_0103", "ch03_059_0102", "ch03_059_0101", "ch03_059_0105", "ch03_059_0104", "ch03_059_0106" } },
        { { "Numinous 0/1"               , true  } , { "ch03_059_0113", "ch03_059_0112", "ch03_059_0111", "ch03_059_0115", "ch03_059_0114", "ch03_059_0116" } },
        { { "Lagiacrus 0/1"              , false } , { "ch03_039_0003", "ch03_039_0002", "ch03_039_0001", "ch03_039_0005", "ch03_039_0004", "ch03_039_0006" } },
        { { "Lagiacrus 0/1"              , true  } , { "ch03_039_0013", "ch03_039_0012", "ch03_039_0011", "ch03_039_0015", "ch03_039_0014", "ch03_039_0016" } },
        { { "Seregios 0/1"               , false } , { "ch03_038_0003", "ch03_038_0002", "ch03_038_0001", "ch03_038_0005", "ch03_038_0004", "ch03_038_0006" } },
        { { "Seregios 0/1"               , true  } , { "ch03_038_0013", "ch03_038_0012", "ch03_038_0011", "ch03_038_0015", "ch03_038_0014", "ch03_038_0016" } },
        { { "Rey Dau 2"                  , false } , { "ch03_021_3003", "ch03_021_3002", "ch03_021_3001", "ch03_021_3005", "ch03_021_3004", "ch03_021_3006" } },
        { { "Rey Dau 2"                  , true  } , { "ch03_021_3013", "ch03_021_3012", "ch03_021_3011", "ch03_021_3015", "ch03_021_3014", "ch03_021_3016" } },
        { { "Uth Duna 2"                 , false } , { "ch03_017_3003", "ch03_017_3002", "ch03_017_3001", "ch03_017_3005", "ch03_017_3004", "ch03_017_3006" } },
        { { "Uth Duna 2"                 , true  } , { "ch03_017_3013", "ch03_017_3012", "ch03_017_3011", "ch03_017_3015", "ch03_017_3014", "ch03_017_3016" } },
        { { "Sakuratide 0/1"             , false } , { "ch03_069_0003", "ch03_069_0002", "ch03_069_0001", "ch03_069_0005", "ch03_069_0004", "ch03_069_0006" } },
        { { "Sakuratide 0/1"             , true  } , { "ch03_069_0013", "ch03_069_0012", "ch03_069_0011", "ch03_069_0015", "ch03_069_0014", "ch03_069_0016" } },
        { { "Blossom 0"                  , false } , { "ch03_075_0003", "ch03_075_0002", "ch03_075_0001", "ch03_075_0005", "ch03_075_0004", "ch03_075_0006" } },
        { { "Blossom 0"                  , true  } , { "ch03_075_0013", "ch03_075_0012", "ch03_075_0011", "ch03_075_0015", "ch03_075_0014", "ch03_075_0016" } },
        { { "Afi 0"                      , false } , { "ch03_081_0003", "ch03_081_0002", "ch03_081_0001", "ch03_081_0005", "ch03_081_0004", "ch03_081_0006" } },
        { { "Afi 0"                      , true  } , { "ch03_081_0013", "ch03_081_0012", "ch03_081_0011", "ch03_081_0015", "ch03_081_0014", "ch03_081_0016" } },
        { { "Diver 0"                    , false } , { "ch03_071_0003", "ch03_071_0002", "ch03_071_0001", "ch03_071_0005", "ch03_071_0004", "ch03_071_0006" } },
        { { "Diver 0"                    , true  } , { "ch03_071_0013", "ch03_071_0012", "ch03_071_0011", "ch03_071_0015", "ch03_071_0014", "ch03_071_0016" } },
        { { "Mimiphyta 0"                , false } , { "ch03_063_0003", ""             , ""             , ""             , ""             , ""              } },
        { { "Expedition Headgear 0"      , false } , { "ch03_087_0003", ""             , ""             , ""             , ""             , ""              } },
        { { "Expedition Headgear 0"      , true  } , { "ch03_087_0013", ""             , ""             , ""             , ""             , ""              } },
        { { "Amstrigian 0"               , false } , { "ch03_078_0003", ""             , ""             , ""             , ""             , ""              } },
        { { "Sealed Dragon Cloth 0"      , false } , { "ch03_079_0003", ""             , ""             , ""             , ""             , ""              } },
        { { "Strategist Spectacles 0"    , false } , { "ch03_088_0003", ""             , ""             , ""             , ""             , ""              } },
        { { "Square Glasses 0"           , false } , { "ch03_088_1003", ""             , ""             , ""             , ""             , ""              } },
        { { "Sealed Eyepatch 0"          , false } , { "ch03_045_0003", ""             , ""             , ""             , ""             , ""              } },
        { { "Sealed Eyepatch 0"          , true  } , { "ch03_045_0013", ""             , ""             , ""             , ""             , ""              } },
        { { "Shadow Shades 0"            , false } , { "ch03_088_2003", ""             , ""             , ""             , ""             , ""              } },
        { { "Round Glasses 0"            , false } , { "ch03_088_3003", ""             , ""             , ""             , ""             , ""              } },
        { { "Faux Felyne 0"              , false } , { "ch03_077_0003", ""             , ""             , ""             , ""             , ""              } },
        { { "Faux Felyne 0"              , true  } , { "ch03_077_0013", ""             , ""             , ""             , ""             , ""              } },
        { { "Pinion Necklace 0"          , false } , { "ch03_089_0003", "ch03_089_0002", "ch03_089_0001", "ch03_089_0005", ""             , "ch03_089_0006" } },
        { { "Hawkheart Jacket 0"         , false } , { "ch03_090_0003", "ch03_090_0002", "ch03_090_0001", "ch03_090_0005", ""             , "ch03_090_0006" } },
        { { "Hawkheart Jacket 0"         , true  } , { "ch03_090_0013", "ch03_090_0012", "ch03_090_0011", "ch03_090_0015", ""             , "ch03_090_0016" } },
        { { "Half Rim Glasses 0"         , false } , { "ch03_088_5003", ""             , ""             , ""             , ""             , ""              } },
        { { "Lovely Shades 0"            , false } , { "ch03_088_4003", ""             , ""             , ""             , ""             , ""              } },
        { { "Toe Bean Mittens 0"         , false } , { ""             , ""             , "ch03_097_0001", ""             , ""             , "ch03_097_0006" } },
        { { "Guild Knight"               , false } , { "ch03_060_0003", "ch03_060_0002", "ch03_060_0001", "ch03_060_0005", "ch03_060_0004", "ch03_060_0006" } },
        { { "Feudal Soldier"             , false } , { "ch03_061_0003", "ch03_061_0002", "ch03_061_0001", "ch03_061_0005", "ch03_061_0004", "ch03_061_0006" } },
        { { "Fencer's Eyepatch 0"        , false } , { "ch03_065_0003", ""             , ""             , ""             , ""             , ""              } },
        { { "Fencer's Eyepatch 0"        , true  } , { "ch03_065_0013", ""             , ""             , ""             , ""             , ""              } },
        { { "Oni Horns Wig 0"            , false } , { "ch03_064_0003", ""             , ""             , ""             , ""             , ""              } },
        { { "Wyverian Ears 0"            , false } , { "ch03_072_0003", ""             , ""             , ""             , ""             , ""              } },
        { { "Wyverian Ears 0"            , true  } , { "ch03_072_0013", ""             , ""             , ""             , ""             , ""              } },
        { { "Florescent Circlet 0"       , false } , { "ch03_068_0003", ""             , ""             , ""             , ""             , ""              } },
        { { "Skull Mask 0"               , false } , { "ch03_098_0003", ""             , ""             , ""             , ""             , ""              } },
        { { "Aviator Shades 0"           , false } , { "ch03_088_6003", ""             , ""             , ""             , ""             , ""              } },
        { { "Kitten Frames 0"            , false } , { "ch03_088_7003", ""             , ""             , ""             , ""             , ""              } },
        { { "Fluffy Ears"                , false } , { "ch03_083_0003", ""             , ""             , ""             , ""             , ""              } },
        { { "Fluffy Tail"                , false } , { ""             , ""             , ""             , "ch03_084_0005", ""             , ""              } },
        { { "Noblesse"                   , false } , { "ch03_067_0003", "ch03_067_0002", "ch03_067_0001", "ch03_067_0005", "ch03_067_0004", "ch03_067_0006" } },
        { { "Cypurrpunk"                 , false } , { "ch03_080_0003", "ch03_080_0002", "ch03_080_0001", "ch03_080_0005", "ch03_080_0004", "ch03_080_0006" } },
        { { "Cypurrpunk"                 , true  } , { "ch03_080_0013", "ch03_080_0012", "ch03_080_0011", "ch03_080_0015", "ch03_080_0014", "ch03_080_0016" } },
        { { "Dreamwalker 0"              , false } , { "ch03_085_0003", "ch03_085_0002", "ch03_085_0001", "ch03_085_0005", "ch03_085_0004", "ch03_085_0006" } },
        { { "Dreamwalker 0"              , true  } , { "ch03_085_0013", "ch03_085_0012", "ch03_085_0011", "ch03_085_0015", "ch03_085_0014", "ch03_085_0016" } },
        { { "Harvest 0"                  , false } , { "ch03_086_0003", "ch03_086_0002", "ch03_086_0001", "ch03_086_0005", "ch03_086_0004", "ch03_086_0006" } },
        { { "Harvest 0"                  , true  } , { "ch03_086_0013", "ch03_086_0012", "ch03_086_0011", "ch03_086_0015", "ch03_086_0014", "ch03_086_0016" } },
        { { "Omega 0"                    , false } , { "ch03_100_0003", "ch03_100_0002", "ch03_100_0001", "ch03_100_0005", "ch03_100_0004", "ch03_100_0006" } },
        { { "Omega 0"                    , true  } , { "ch03_100_0013", "ch03_100_0012", "ch03_100_0011", "ch03_100_0015", "ch03_100_0014", "ch03_100_0016" } },
        { { "Bale 0"                     , false } , { "ch03_100_1003", "ch03_100_1002", "ch03_100_1001", "ch03_100_1005", "ch03_100_1004", "ch03_100_1006" } },
        { { "Gelidron 0"                 , false } , { "ch03_096_0003", "ch03_096_0002", ""             , ""             , ""             , ""              } },

        // Full Body Sets
        { { "Akuma 0"                    , false } , { "ch02_070_0002", "ch02_070_0002", "ch02_070_0002", "ch02_070_0002", "ch02_070_0002", "ch02_070_0002" } },

        // Alma
        { { "[NPC] Alma's Handler's Outfit"     , true  } , { "", "ch04_000_0000", "", "", "", "" } },
        { { "[NPC] Alma's Scrivener's Coat"     , true  } , { "", "ch04_000_0002", "", "", "", "" } },
        { { "[NPC] Alma's Spring Blossom Kimono", true  } , { "", "ch04_000_0070", "", "", "", "" } },
        { { "[NPC] Alma's Summer Poncho"        , true  } , { "", "ch04_000_0071", "", "", "", "" } },
        { { "[NPC] Alma's New World Commission" , true  } , { "", "ch04_000_0074", "", "", "", "" } },
        { { "[NPC] Alma's Chun Li Outfit"       , true  } , { "", "ch04_000_0075", "", "", "", "" } },
        { { "[NPC] Alma's Cammy Outfit"         , true  } , { "", "ch04_000_0076", "", "", "", "" } },
        { { "[NPC] Alma's Autumn Witch Outfit"  , true  } , { "", "ch04_000_0072", "", "", "", "" } },
                                                
        // Gemma
        { { "[NPC] Gemma's Smithy's Outfit"    , true  } , { "", "ch04_004_0000", "", "", "", "" } },
        { { "[NPC] Gemma's Summer Coveralls"   , true  } , { "", "ch04_004_0072", "", "", "", "" } },

        // Erik
        { { "[NPC] Erik's Handler's Outfit"    , false } , { "", "ch04_010_0000", "", "", "", "" } },
        { { "[NPC] Erik's Summer Hat"          , false } , { "", "ch04_010_0071", "", "", "", "" } },
        { { "[NPC] Erik's Autumn Therian"      , false } , { "", "ch04_010_0072", "", "", "", "" } },

        // Other NPCs
        { { "[NPC] Y'sai's Outfit"                , false } , { "", "ch04_001_0000",              "", "", "", "" } },
        { { "[NPC] Windward Plains Hood A"        , false } , { "", "ch04_001_0050",              "", "", "", "" } },
        { { "[NPC] Windward Plains Hood B"        , false } , { "", "ch04_002_0000",              "", "", "", "" } },
        { { "[NPC] Zatoh's Outfit"                , false } , { "", "ch04_003_0000",              "", "", "", "" } },
        { { "[NPC] Sild Poncho A"                 , false } , { "", "ch04_005_0000",              "", "", "", "" } },
        { { "[NPC] Nata's Outfit"                 , false } , { "", "ch04_006_0000",              "", "", "", "" } },
        { { "[NPC] Nata's Hunter Outfit"          , false } , { "", "ch04_006_0001",              "", "", "", "" } },
        { { "[NPC] Nata's Winter Outfit"          , false } , { "", "ch04_006_0050",              "", "", "", "" } }, // Maybe unneeded
        { { "[NPC] Sild Poncho B"                 , false } , { "", "ch04_007_0000",              "", "", "", "" } },
        { { "[NPC] Fabius's Outfit"               , false } , { "", "ch04_008_0000",              "", "", "", "" } },
        { { "[NPC] Olivia's Outfit"               , true  } , { "", "ch04_009_0000",              "", "", "", "" } },
        { { "[NPC] Olivia's Inner Outfit"         , true  } , { "", "ch04_009_0000_eating_inner", "", "", "", "" } },
        { { "[NPC] Werner's Outfit"               , false } , { "", "ch04_011_0000",              "", "", "", "" } },
        { { "[NPC] Werner's Winter Outfit"        , false } , { "", "ch04_011_0050",              "", "", "", "" } }, // maybe unneeded
        { { "[NPC] The Allhearken's Outfit"       , false } , { "", "ch04_012_0000",              "", "", "", "" } },
        { { "[NPC] Elder Ela's Outfit"            , false } , { "", "ch04_013_0000",              "", "", "", "" } },
        { { "[NPC] Elder Ela's Hood"              , false } , { "", "ch04_013_0050",              "", "", "", "" } },
        { { "[NPC] Oilwell Basin Outfit A"        , false } , { "", "ch04_014_0000",              "", "", "", "" } },
        { { "[NPC] Maki's Outfit"                 , true  } , { "", "ch04_015_0000",              "", "", "", "" } },
        { { "[NPC] Dogard's Outfit"               , false } , { "", "ch04_016_0000",              "", "", "", "" } },
        { { "[NPC] Aida's Outfit"                 , true  } , { "", "ch04_017_0000",              "", "", "", "" } },
        { { "[NPC] Tetsuzuan's Outfit"            , false } , { "", "ch04_018_0000",              "", "", "", "" } },
        { { "[NPC] Santiago's Outfit"             , false } , { "", "ch04_019_0000",              "", "", "", "" } },
        { { "[NPC] Vio's Outfit"                  , true  } , { "", "ch04_020_0000",              "", "", "", "" } },
        { { "[NPC] Legendary Artisan's Outfit"    , false } , { "", "ch04_022_0000",              "", "", "", "" } },
        { { "[NPC] Diva's Outfit"                 , true  } , { "", "ch04_023_0000",              "", "", "", "" } },
        { { "[NPC] Windward Plains Outfit A"      , false } , { "", "ch04_200_0000",              "", "", "", "" } },
        { { "[NPC] Windward Plains Outfit B"      , false } , { "", "ch04_200_0001",              "", "", "", "" } },
        { { "[NPC] Windward Plains Outfit C"      , false } , { "", "ch04_200_0100",              "", "", "", "" } },
        { { "[NPC] Provisions Outfit A"           , false } , { "", "ch04_200_0200",              "", "", "", "" } },
        { { "[NPC] Defender Armour"               , false } , { "", "ch04_200_0201",              "", "", "", "" } },
        { { "[NPC] Explorer"                      , false } , { "", "ch04_200_0202",              "", "", "", "" } },
        { { "[NPC] Clerk"                         , false } , { "", "ch04_200_0203",              "", "", "", "" } },
        { { "[NPC] Innerwear"                     , false } , { "", "ch04_200_0204",              "", "", "", "" } },
        { { "[NPC] Clerk (Blossomdance)"          , false } , { "", "ch04_200_0205",              "", "", "", "" } },
        { { "[NPC] Clerk (Flamefete)"             , false } , { "", "ch04_200_0206",              "", "", "", "" } },
        { { "[NPC] Clerk (Dreamspell)"            , false } , { "", "ch04_200_0207",              "", "", "", "" } },
        { { "[NPC] Oilwell Basin Outfit B"        , false } , { "", "ch04_200_0300",              "", "", "", "" } },
        { { "[NPC] Oilwell Basin Outfit C"        , false } , { "", "ch04_200_0301",              "", "", "", "" } },
        { { "[NPC] Oilwell Basin Outfit D"        , false } , { "", "ch04_200_0302",              "", "", "", "" } },
        { { "[NPC] Sild Poncho C"                 , false } , { "", "ch04_200_0400",              "", "", "", "" } },
        { { "[NPC] Amone's Outfit"                , true  } , { "", "ch04_201_0000",              "", "", "", "" } },
        { { "[NPC] Windward Plains Outfit D"      , true  } , { "", "ch04_201_0002",              "", "", "", "" } },
        { { "[NPC] Windward Plains Outfit E"      , false } , { "", "ch04_201_0100",              "", "", "", "" } },
        { { "[NPC] Provisions Outfit B"           , true  } , { "", "ch04_201_0200",              "", "", "", "" } },
        { { "[NPC] Defender Armour"               , true  } , { "", "ch04_201_0201",              "", "", "", "" } },
        { { "[NPC] Explorer"                      , true  } , { "", "ch04_201_0202",              "", "", "", "" } },
        { { "[NPC] Clerk"                         , true  } , { "", "ch04_201_0203",              "", "", "", "" } },
        { { "[NPC] Innerwear"                     , true  } , { "", "ch04_201_0204",              "", "", "", "" } },
        { { "[NPC] Clerk (Blossomdance)"          , true  } , { "", "ch04_201_0205",              "", "", "", "" } },
        { { "[NPC] Clerk (Flamefete)"             , true  } , { "", "ch04_201_0206",              "", "", "", "" } },
        { { "[NPC] Clerk (Dreamspell)"            , true  } , { "", "ch04_201_0207",              "", "", "", "" } },
        { { "[NPC] Oilwell Basin Outfit B"        , true  } , { "", "ch04_201_0300",              "", "", "", "" } },
        { { "[NPC] Oilwell Basin Outfit C"        , true  } , { "", "ch04_201_0301",              "", "", "", "" } },
        { { "[NPC] Oilwell Basin Outfit D"        , true  } , { "", "ch04_201_0302",              "", "", "", "" } },
        { { "[NPC] Sild Poncho C"                 , true  } , { "", "ch04_201_0400",              "", "", "", "" } },
        { { "[NPC] Windward Plains Child Outfit A", true  } , { "", "ch04_204_0000",              "", "", "", "" } },
        { { "[NPC] Windward Plains Child Outfit B", true  } , { "", "ch04_204_0001",              "", "", "", "" } },
        { { "[NPC] Oilwell Basin Child Outfit"    , true  } , { "", "ch04_204_0300",              "", "", "", "" } },
        { { "[NPC] Windward Plains Elder Outfit A", true  } , { "", "ch04_205_0000",              "", "", "", "" } },
        { { "[NPC] Windward Plains Elder Outfit B", true  } , { "", "ch04_205_0001",              "", "", "", "" } },
        { { "[NPC] Oilwell Basin Elder Outfit"    , false } , { "", "ch04_205_0300",              "", "", "", "" } },
        { { "[NPC] Elder Researcher's Outfit"     , false } , { "", "ch04_207_0100",              "", "", "", "" } },
        { { "[NPC] Suja Elder Outfit A"           , false } , { "", "ch04_207_0200",              "", "", "", "" } },
        { { "[NPC] Suja Elder Outfit B"           , false } , { "", "ch04_208_0100",              "", "", "", "" } },
        { { "[NPC] Provisions Outfit C"           , false } , { "", "ch04_208_0101",              "", "", "", "" } },
        { { "[NPC] Researcher's Outfit"           , false } , { "", "ch04_208_0100",              "", "", "", "" } },
        { { "[NPC] Suja Outfit"                   , false } , { "", "ch04_208_0200",              "", "", "", "" } },
        { { "[NPC] Suja Guard"                    , false } , { "", "ch04_208_0201",              "", "", "", "" } },
        { { "[NPC] Researcher's Outfit"           , true  } , { "", "ch04_209_0100",              "", "", "", "" } },
        { { "[NPC] Provisions Outfit C"           , true  } , { "", "ch04_209_0101",              "", "", "", "" } },
        { { "[NPC] Suja Outfit"                   , true  } , { "", "ch04_209_0200",              "", "", "", "" } },
        { { "[NPC] Suja Guard"                    , true  } , { "", "ch04_209_0201",              "", "", "", "" } },
    };

    ArmourMapping ArmourList::ACTIVE_MAPPING = ArmourList::FALLBACK_MAPPING;
}