#pragma once

#include <kbf/data/preset/preset_group.hpp>
#include <kbf/data/preset/preset.hpp>
#include <kbf/data/preset/player_override.hpp>
#include <kbf/data/formats/format_metadata.hpp>

#include <vector>

namespace kbf {

	struct KBFFileData {
		std::vector<PresetGroup> presetGroups;
		std::vector<Preset> presets;
		std::vector<PlayerOverride> playerOverrides;
		FormatMetadata metadata;
	};

}