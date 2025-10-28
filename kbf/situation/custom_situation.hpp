#pragma once

#include <unordered_map>

namespace kbf {

	enum CustomSituation {
		isInMainMenuScene    = 1,
		isInSaveSelectGUI    = 2,
		isInCharacterCreator = 3,
		isInHunterGuildCard  = 4,
		isInCutscene         = 5,
		isInGame             = 6,
		isInTitleMenus       = 7
	};

    const std::unordered_map<size_t, const char*> CUSTOM_SITUATION_NAMES = {
		{ 1, "isInMainMenuScene"    },
		{ 2, "isInSaveSelectGUI"    },
		{ 3, "isInCharacterCreator" },
		{ 4, "isInHunterGuildCard"  },
		{ 5, "isInCutscene"         },
		{ 6, "isInGame"             },
		{ 7, "isInTitleMenus"       },
    };

}