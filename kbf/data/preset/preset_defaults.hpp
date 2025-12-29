#pragma once

#include <kbf/data/formats/format_metadata.hpp>
#include <kbf/data/preset/preset.hpp>

namespace kbf {

	// Note: If adding new entires, check ALL references to these variables to see where updates are needed.

	// --- Core Npcs ---

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

	// --- Support Hunters ---

	struct OliviaDefaults {
		std::string defaultOutfit;
		FormatMetadata metadata;
	};

	struct RossoDefaults {
		std::string quematrice;
		FormatMetadata metadata;
	};

	struct AlessaDefaults {
		std::string balahara;
		FormatMetadata metadata;
	};

	struct MinaDefaults {
		std::string chatacabra;
		FormatMetadata metadata;
	};

	struct KaiDefaults {
		std::string ingot;
		FormatMetadata metadata;
	};

	struct GriffinDefaults {
		std::string conga;
		FormatMetadata metadata;
	};

	struct NightmistDefaults {
		std::string ingot;
		FormatMetadata metadata;
	};

	struct FabiusDefaults {
		std::string defaultOutfit;
		FormatMetadata metadata;
	};

	struct NadiaDefaults {
		std::string defaultOutfit;
		FormatMetadata metadata;
	};

	struct SupportHunterDefaults {
		OliviaDefaults    olivia;
		RossoDefaults     rosso;
		AlessaDefaults    alessa;
		MinaDefaults      mina;
		KaiDefaults       kai;
		GriffinDefaults   griffin;
		NightmistDefaults nightmist;
		FabiusDefaults    fabius;
		NadiaDefaults     nadia;
		FormatMetadata metadata;
	};

	// --- Top Level Struct ---

	struct PresetDefaults {
		AlmaDefaults  alma;
		GemmaDefaults gemma;
		ErikDefaults  erik;
		SupportHunterDefaults supportHunters;
		FormatMetadata metadata;
	};

}