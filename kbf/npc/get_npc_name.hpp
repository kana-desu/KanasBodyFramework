#pragma once

#include <kbf/npc/npc_id.hpp>
#include <kbf/data/armour/armour_list.hpp>
#include <kbf/data/ids/special_armour_ids.hpp>

#include <string>
#include <unordered_map>

namespace kbf {

    static const std::unordered_map<NpcID, std::string> NPC_ID_TO_NAME_MAP{
        { NpcID::NPC_ID_ALMA      , "Alma"      },
        { NpcID::NPC_ID_GEMMA     , "Gemma"     },
        { NpcID::NPC_ID_ERIK      , "Erik"      },
        { NpcID::NPC_ID_OLIVIA    , "Olivia"    },
        { NpcID::NPC_ID_ROSSO     , "Rosso"     },
        { NpcID::NPC_ID_ALESSA    , "Alessa"    },
        { NpcID::NPC_ID_MINA      , "Mina"      },
        { NpcID::NPC_ID_KAI       , "Kai"       },
        { NpcID::NPC_ID_GRIFFIN   , "Griffin"   },
        { NpcID::NPC_ID_NIGHTMIST , "Nightmist" },
        { NpcID::NPC_ID_FABIUS    , "Fabius"    },
        { NpcID::NPC_ID_NADIA     , "Nadia"     },
	};

    static const std::unordered_map<std::string, std::string> NPC_ARMOUR_ID_TO_NAME_MAP{
        { ANY_ARMOUR_ID   , "Unknown Hunter" },
        { "ch03_002_0002" , "Hunter NPC" },
        { "ch03_002_0012" , "Hunter NPC" },
        { "ch03_002_1002" , "Hunter NPC" },
        { "ch03_002_1012" , "Hunter NPC" },
        { "ch03_001_0002" , "Hunter NPC" },
        { "ch03_001_0012" , "Hunter NPC" },
        { "ch03_003_0002" , "Hunter NPC" },
        { "ch03_003_0012" , "Hunter NPC" },
        { "ch03_008_0002" , "Hunter NPC" },
        { "ch03_008_0012" , "Hunter NPC" },
        { "ch03_014_0002" , "Hunter NPC" },
        { "ch03_014_0012" , "Hunter NPC" },
        { "ch03_005_0002" , "Hunter NPC" },
        { "ch03_005_0012" , "Hunter NPC" },
        { "ch03_006_0002" , "Hunter NPC" },
        { "ch03_006_0012" , "Hunter NPC" },
        { "ch03_004_0002" , "Hunter NPC" },
        { "ch03_004_0012" , "Hunter NPC" },
        { "ch03_009_0002" , "Hunter NPC" },
        { "ch03_009_0012" , "Hunter NPC" },
        { "ch03_010_0002" , "Hunter NPC" },
        { "ch03_010_0012" , "Hunter NPC" },
        { "ch03_012_0002" , "Hunter NPC" },
        { "ch03_012_0012" , "Hunter NPC" },
        { "ch03_028_0002" , "Hunter NPC" },
        { "ch03_028_0012" , "Hunter NPC" },
        { "ch03_013_0002" , "Hunter NPC" },
        { "ch03_013_0012" , "Hunter NPC" },
        { "ch03_017_0002" , "Hunter NPC" },
        { "ch03_017_0012" , "Hunter NPC" },
        { "ch03_015_0002" , "Hunter NPC" },
        { "ch03_015_0012" , "Hunter NPC" },
        { "ch03_020_0002" , "Hunter NPC" },
        { "ch03_020_0012" , "Hunter NPC" },
        { "ch03_018_0002" , "Hunter NPC" },
        { "ch03_018_0012" , "Hunter NPC" },
        { "ch03_021_0002" , "Hunter NPC" },
        { "ch03_021_0012" , "Hunter NPC" },
        { "ch03_022_0002" , "Hunter NPC" },
        { "ch03_022_0012" , "Hunter NPC" },
        { "ch03_023_0002" , "Hunter NPC" },
        { "ch03_023_0012" , "Hunter NPC" },
        { "ch03_024_0002" , "Hunter NPC" },
        { "ch03_024_0012" , "Hunter NPC" },
        { "ch03_027_0002" , "Hunter NPC" },
        { "ch03_027_0012" , "Hunter NPC" },
        { "ch03_027_3002" , "Hunter NPC" },
        { "ch03_027_3012" , "Hunter NPC" },
        { "ch03_003_5002" , "Hunter NPC" },
        { "ch03_003_5012" , "Hunter NPC" },
        { "ch03_036_5002" , "Hunter NPC" },
        { "ch03_036_5012" , "Hunter NPC" },
        { "ch03_030_6002" , "Hunter NPC" },
        { "ch03_030_6012" , "Hunter NPC" },
        { "ch03_031_0002" , "Hunter NPC" },
        { "ch03_031_0012" , "Hunter NPC" },
        { "ch03_032_5002" , "Hunter NPC" },
        { "ch03_032_5012" , "Hunter NPC" },
        { "ch03_053_0002" , "Hunter NPC" },
        { "ch03_053_0012" , "Hunter NPC" },
        { "ch03_032_0002" , "Hunter NPC" },
        { "ch03_032_0012" , "Hunter NPC" },
        { "ch03_051_0002" , "Hunter NPC" },
        { "ch03_051_0012" , "Hunter NPC" },
        { "ch03_047_0002" , "Hunter NPC" },
        { "ch03_057_0002" , "Hunter NPC" },
        { "ch03_057_0012" , "Hunter NPC" },
        { "ch03_041_0002" , "Hunter NPC" },
        { "ch03_041_0012" , "Hunter NPC" },
        { "ch03_054_0002" , "Hunter NPC" },
        { "ch03_054_0012" , "Hunter NPC" },
        { "ch03_050_0002" , "Hunter NPC" },
        { "ch03_050_0012" , "Hunter NPC" },
        { "ch03_029_0002" , "Hunter NPC" },
        { "ch03_029_0012" , "Hunter NPC" },
        { "ch03_025_0002" , "Hunter NPC" },
        { "ch03_025_0012" , "Hunter NPC" },
        { "ch03_019_0002" , "Hunter NPC" },
        { "ch03_019_0012" , "Hunter NPC" },
        { "ch03_042_0002" , "Hunter NPC" },
        { "ch03_042_0012" , "Hunter NPC" },
        { "ch03_040_0002" , "Hunter NPC" },
        { "ch03_040_0012" , "Hunter NPC" },
        { "ch03_043_6002" , "Hunter NPC" },
        { "ch03_043_6012" , "Hunter NPC" },
        { "ch03_062_0002" , "Hunter NPC" },
        { "ch03_060_0002" , "Hunter NPC" },
        { "ch03_073_0002" , "Hunter NPC" },
        { "ch03_037_0002" , "Hunter NPC" },
        { "ch03_037_0012" , "Hunter NPC" },
        { "ch03_056_0002" , "Hunter NPC" },
        { "ch03_056_0012" , "Hunter NPC" },
        { "ch03_055_0002" , "Hunter NPC" },
        { "ch03_055_0012" , "Hunter NPC" },
        { "ch03_046_0002" , "Hunter NPC" },
        { "ch03_034_0002" , "Hunter NPC" },
        { "ch03_034_0012" , "Hunter NPC" },
        { "ch03_058_0002" , "Hunter NPC" },
        { "ch03_058_0012" , "Hunter NPC" },
        { "ch03_036_0002" , "Hunter NPC" },
        { "ch03_036_0012" , "Hunter NPC" },
        { "ch03_035_0002" , "Hunter NPC" },
        { "ch03_035_0012" , "Hunter NPC" },
        { "ch03_048_0002" , "Hunter NPC" },
        { "ch03_059_0102" , "Hunter NPC" },
        { "ch03_059_0112" , "Hunter NPC" },
        { "ch03_066_0002" , "Hunter NPC" },
        { "ch03_066_0012" , "Hunter NPC" },
        { "ch03_074_0002" , "Hunter NPC" },
        { "ch03_074_0012" , "Hunter NPC" },
        { "ch03_069_0002" , "Hunter NPC" },
        { "ch03_069_0012" , "Hunter NPC" },
        { "ch03_075_0002" , "Hunter NPC" },
        { "ch03_075_0012" , "Hunter NPC" },
        { "ch03_021_3002" , "Hunter NPC" },
        { "ch03_021_3012" , "Hunter NPC" },
        { "ch03_039_0002" , "Hunter NPC" },
        { "ch03_039_0012" , "Hunter NPC" },
        { "ch03_038_0002" , "Hunter NPC" },
        { "ch03_038_0012" , "Hunter NPC" },
        { "ch03_089_0002" , "Hunter NPC" },
        { "ch03_090_0002" , "Hunter NPC" },
        { "ch03_090_0012" , "Hunter NPC" },
        { "ch03_017_3002" , "Hunter NPC" },
        { "ch03_017_3012" , "Hunter NPC" },
        { "ch03_071_0002" , "Hunter NPC" },
        { "ch03_071_0012" , "Hunter NPC" },
        { "ch03_081_0002" , "Hunter NPC" },
        { "ch03_081_0012" , "Hunter NPC" },
        { "ch03_080_0002" , "Hunter NPC" },
        { "ch03_080_0012" , "Hunter NPC" },
        { "ch03_067_0002" , "Hunter NPC" },
        { "ch03_061_0002" , "Hunter NPC" },
        { "ch03_085_0002" , "Hunter NPC" },
        { "ch03_085_0012" , "Hunter NPC" },
        { "ch03_086_0002" , "Hunter NPC" },
        { "ch03_086_0012" , "Hunter NPC" },
        { "ch03_100_0002" , "Hunter NPC" },
        { "ch03_100_0012" , "Hunter NPC" },
        { "ch03_100_1002" , "Hunter NPC" },
        { "ch03_096_0002" , "Hunter NPC" },

        { "ch04_000_0000" , "Alma" },       // Note: These are made redundant by the NpcID check and *shouldn't* need updating 
        { "ch04_000_0002" , "Alma" },       //
        { "ch04_000_0070" , "Alma" },       //
        { "ch04_000_0071" , "Alma" },       //
        { "ch04_000_0074" , "Alma" },       //
        { "ch04_000_0075" , "Alma" },       //
        { "ch04_000_0076" , "Alma" },       //
        { "ch04_000_0076" , "Alma" },       //
        { "ch04_000_0072" , "Alma" },       //
        { "ch04_000_0073" , "Alma" },       //
                                            //
        { "ch04_004_0000" , "Gemma" },      //
        { "ch04_004_0072" , "Gemma" },      //
		{ "ch04_004_0073" , "Gemma" },      //
                                            //
        { "ch04_010_0000" , "Erik" },       //
        { "ch04_010_0071" , "Erik" },       //
        { "ch04_010_0072" , "Erik" },       //
        { "ch04_010_0073" , "Erik" },       //

        { "ch04_001_0000"             , "Y'sai"                     }, //(NPC) Y'sai's Outfit"            
        { "ch04_001_0050"             , "Windward Plains NPC"       }, //(NPC) Windward Plains Hood 1"    
        { "ch04_002_0000"             , "Windward Plains NPC"       }, //(NPC) Windward Plains Hood 2"    
        { "ch04_003_0000"             , "Zatoh"                     }, //(NPC) Zatoh's Outfit"            
        { "ch04_005_0000"             , "Sild NPC"                  }, //(NPC) Sild Poncho 1"             
        { "ch04_006_0000"             , "Nata"                      }, //(NPC) Nata's Outfit"                         
        { "ch04_006_0001"             , "Nata"                      }, //(NPC) Nata's Hunter Outfit"                  
        { "ch04_006_0050"             , "Nata"                      }, //(NPC) Nata's Winter Outfit"      
        { "ch04_007_0000"             , "Sild NPC"                  }, //(NPC) Sild Poncho 2"             
        { "ch04_008_0000"             , "Fabius"                    }, //(NPC) Fabius's Outfit"           
        { "ch04_009_0000"             , "Olivia"                    }, //(NPC) Olivia's Outfit"           
        { "ch04_009_0000_eating_inner", "Olivia"                    }, //(NPC) Olivia's Inner Outfit"     
        { "ch04_011_0000"             , "Werner"                    }, //(NPC) Werner's Outfit"           
        { "ch04_011_0050"             , "Werner"                    }, //(NPC) Werner's Winter Outfit"    
        { "ch04_012_0000"             , "The Allhearken"            }, //(NPC) The Allhearken's Outfit"   
        { "ch04_013_0000"             , "Elder Ela"                 }, //(NPC) Elder Ela's Outfit"                         
        { "ch04_013_0050"             , "Elder Ela"                 }, //(NPC) Elder Ela's Hood"          
        { "ch04_014_0000"             , "Oilwell Basin NPC"         }, //(NPC) Forge Outfit 1"            
        { "ch04_015_0000"             , "Maki"                      }, //(NPC) Maki's Outfit"             
        { "ch04_016_0000"             , "Dogard"                    }, //(NPC) Dogard's Outfit"           
        { "ch04_017_0000"             , "Aida"                      }, //(NPC) Aida's Outfit"             
        { "ch04_018_0000"             , "Tetsuzan"                  }, //(NPC) Tetsuzuan's Outfit"        
        { "ch04_019_0000"             , "Santiago"                  }, //(NPC) Santiago's Outfit"         
        { "ch04_020_0000"             , "Vio"                       }, //(NPC) Vio's Outfit"              
        { "ch04_022_0000"             , "Legendary Artisan"         }, //(NPC) Legendary Artisan's Outfit"
        { "ch04_023_0000"             , "Diva"                      }, //(NPC) Diva's Outfit"      
        { "ch04_025_0000"             , "Nadia"                     }, //(NPC) Nadia's Outfit"
        { "ch04_200_0000"             , "Windward Plains NPC"       }, //(NPC) Windward Plains Outfit 1"                             
        { "ch04_200_0001"             , "Windward Plains NPC"       }, //(NPC) Windward Plains Outfit 2"                             
        { "ch04_200_0100"             , "Windward Plains NPC"       }, //(NPC) Windward Plains Outfit 3"  
        { "ch04_200_0200"             , "Provisions NPC"            }, //(NPC) Smithy Outfit 1"           
        { "ch04_200_0201"             , "Defender NPC"              }, //(NPC) Defender Armour"           
        { "ch04_200_0202"             , "Explorer NPC"              }, //(NPC) Explorer"                                   
        { "ch04_200_0203"             , "Clerk NPC"                 }, //(NPC) Clerk"                                      
        { "ch04_200_0204"             , "Clerk NPC"                 }, //(NPC) Inner"                                      
        { "ch04_200_0205"             , "Clerk NPC"                 }, //(NPC) Blossomdance"                               
        { "ch04_200_0206"             , "Clerk NPC"                 }, //(NPC) Flamefete"                 
        { "ch04_200_0207"             , "Clerk NPC"                 }, //(NPC) Dreamspell"                 
        { "ch04_200_0208"             , "Clerk NPC"                 }, //(NPC) Lumenhymn"                 
        { "ch04_200_0300"             , "Provisions NPC"            }, //(NPC) Forge Outfit 2"                                  
        { "ch04_200_0301"             , "Provisions NPC"            }, //(NPC) Forge Outfit 3"                                  
        { "ch04_200_0302"             , "Provisions NPC"            }, //(NPC) Forge Outfit 4"            
        { "ch04_200_0400"             , "Sild NPC"                  }, //(NPC) Sild Poncho 3"             
        { "ch04_201_0000"             , "Amone"                     }, //(NPC) Amone's Outfit"            
        { "ch04_201_0002"             , "Windward Plains NPC"       }, //(NPC) Windward Plains Outfit 4"  
        { "ch04_201_0100"             , "Windward Plains NPC"       }, //(NPC) Windward Plains Outfit 5"  
        { "ch04_201_0200"             , "Provisions NPC"            }, //(NPC) Smithy Outfit 2"           
        { "ch04_201_0201"             , "Defender NPC"              }, //(NPC) Defender Armour"           
        { "ch04_201_0202"             , "Explorer NPC"              }, //(NPC) Explorer"                                   
        { "ch04_201_0203"             , "Clerk NPC"                 }, //(NPC) Clerk"                                      
        { "ch04_201_0204"             , "Clerk NPC"                 }, //(NPC) Inner"                                      
        { "ch04_201_0205"             , "Clerk NPC"                 }, //(NPC) Blossomdance"                               
        { "ch04_201_0206"             , "Clerk NPC"                 }, //(NPC) Flamefete"                 
        { "ch04_201_0207"             , "Clerk NPC"                 }, //(NPC) Dreamspell"                 
        { "ch04_201_0208"             , "Clerk NPC"                 }, //(NPC) Lumenhymn"                 
        { "ch04_201_0300"             , "Oilwell Basin NPC"         }, //(NPC) Forge Outfit 2"                                      
        { "ch04_201_0301"             , "Oilwell Basin NPC"         }, //(NPC) Forge Outfit 3"                                      
        { "ch04_201_0302"             , "Oilwell Basin NPC"         }, //(NPC) Forge Outfit 4"            
        { "ch04_201_0400"             , "Sild NPC"                  }, //(NPC) Sild Poncho 3"             
        { "ch04_204_0000"             , "Windward Plains Child NPC" }, //(NPC) Windward Plains Child Outfit
        { "ch04_204_0001"             , "Windward Plains Child NPC" }, //(NPC) Windward Plains Child Outfit
        { "ch04_204_0300"             , "Oilwell Basin Child NPC"   }, //(NPC) Forge Child Outfit 2"      
        { "ch04_205_0000"             , "Windward Plains Elder NPC" }, //(NPC) Windward Plains Elder Outfit
        { "ch04_205_0001"             , "Windward Plains Elder NPC" }, //(NPC) Windward Plains Elder Outfit
        { "ch04_205_0300"             , "Oilwell Basin NPC"         }, //(NPC) Forge Elder Outfit 2"      
        { "ch04_207_0100"             , "Elder Researcher NPC"      }, //(NPC) Elder Researcher Outfit"                            
        { "ch04_207_0200"             , "Suja Elder NPC"            }, //(NPC) Suja Elder Outfit A"                            
        { "ch04_208_0100"             , "Suja Elder NPC"            }, //(NPC) Suja Elder Outfit B"       
        { "ch04_208_0101"             , "Provisions NPC"            }, //(NPC) Smithy Outfit 3"           
        { "ch04_208_0100"             , "Researcher NPC"            }, //(NPC) Suja Outfit 1"             
        { "ch04_208_0200"             , "Suja NPC"                  }, //(NPC) Suja Outfit 2"             
        { "ch04_208_0201"             , "Suja Guard"                }, //(NPC) Suja Guard"           
        { "ch04_209_0100"             , "Researcher NPC"            }, //(NPC) Suja Outfit 1"             
        { "ch04_209_0101"             , "Provisions NPC"            }, //(NPC) Smithy Outfit 3"           
        { "ch04_209_0200"             , "Suja NPC"                  }, //(NPC) Suja Outfit 2"             
        { "ch04_209_0201"             , "Suja Guard"                }, //(NPC) Suja Guard"             
    };

	inline std::string getNpcNameFromNpcID(NpcID npcID, bool& success) {
		return NPC_ID_TO_NAME_MAP.find(npcID) != NPC_ID_TO_NAME_MAP.end() 
            ? (success = true,  NPC_ID_TO_NAME_MAP.at(npcID)) 
            : (success = false, "FLAG_NPC_ID_TO_NAME_FETCH_FAILURE");
    }

    inline std::string getNpcNameFromArmourID(ArmourID armour) {
        return NPC_ARMOUR_ID_TO_NAME_MAP.find(armour.body) != NPC_ARMOUR_ID_TO_NAME_MAP.end() ? NPC_ARMOUR_ID_TO_NAME_MAP.at(armour.body) : "NPC (UNKNOWN SET!)";
    }

    inline std::string getNpcNameFromArmourSet(ArmourSet set) {
        if (ArmourList::ACTIVE_MAPPING.find(set) == ArmourList::ACTIVE_MAPPING.end()) {
            return "NPC (INVALID SET!)";
		}

        return getNpcNameFromArmourID(ArmourList::ACTIVE_MAPPING.at(set));
	}

    inline std::string getNpcName(NpcID npcID, ArmourSet set) {
        // Try get named npc name from npcID first
        bool npcIdSuccess = false;
        std::string npcName = getNpcNameFromNpcID(npcID, npcIdSuccess);
        if (npcIdSuccess) return npcName;

        // Fallback to generic name from armour set
        return getNpcNameFromArmourSet(set);
    }

}