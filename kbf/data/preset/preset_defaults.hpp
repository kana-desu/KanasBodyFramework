#pragma once

#include <kbf/data/formats/format_metadata.hpp>
#include <kbf/data/preset/preset.hpp>

namespace kbf {

	// Note: If adding new entires, check ALL references to these variables to see where updates are needed.

	struct AlmaDefaults {
		std::string handlersOutfit;
		std::string newWorldCommission;
		std::string scrivenersCoat;
		std::string springBlossomKimono;
		std::string chunLiOutfit;
		std::string cammyOutfit;
		std::string summerPoncho;
		std::string autumnWitch;
		std::string featherskirtSeikretDress;
		FormatMetadata metadata;
	};

	struct GemmaDefaults {
		std::string smithysOutfit;
		std::string summerCoveralls;
		std::string redveilSeikretDress;
		FormatMetadata metadata;
	};

	struct ErikDefaults {
		std::string handlersOutfit;
		std::string summerHat;
		std::string autumnTherian;
		std::string crestcollarSeikretSuit;
		FormatMetadata metadata;
	};

	struct PresetDefaults {
		AlmaDefaults  alma;
		GemmaDefaults gemma;
		ErikDefaults  erik;
		FormatMetadata metadata;
	};

}