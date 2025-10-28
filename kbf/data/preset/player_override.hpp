#pragma once

#include <kbf/data/formats/format_metadata.hpp>
#include <kbf/data/player/player_data.hpp>
#include <kbf/data/preset/preset_group.hpp>

namespace kbf {

	struct PlayerOverride {
		PlayerData player;
		std::string presetGroup; // uuid
		FormatMetadata metadata;
	};

}