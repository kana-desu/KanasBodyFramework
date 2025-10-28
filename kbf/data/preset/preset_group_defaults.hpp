#pragma once

#include <kbf/data/formats/format_metadata.hpp>
#include <kbf/data/preset/preset_group.hpp>

namespace kbf {

	struct PlayerDefaults {
		std::string male;
		std::string female;
		FormatMetadata metadata;
	};
	
	struct NpcDefaults {
		std::string male;
		std::string female;
		FormatMetadata metadata;
	};

	struct PresetGroupDefaults {
		PlayerDefaults player;
		NpcDefaults    npc;
		FormatMetadata metadata;
	};

}