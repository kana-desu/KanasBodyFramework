#pragma once

#include <string>

namespace kbf {

	enum class KbfFileType {
		UNKNOWN,
		SETTINGS,
		ALMA_CONFIG,
		ERIK_CONFIG,
		GEMMA_CONFIG,
		SUPPORT_HUNTER_CONFIG,
		NPC_CONFIG,
		PLAYER_CONFIG,
		DOT_KBF,
		FBS_PRESET,
		PRESET,
		PRESET_GROUP,
		PLAYER_OVERRIDE,
		ARMOUR_LIST,
		BONE_CACHE,
		PART_CACHE,
		MATERIAL_CACHE
	};

	inline std::string kbfFileTypeToString(KbfFileType fileType) {
		switch (fileType) {
		case KbfFileType::SETTINGS:              return "Settings";
		case KbfFileType::ALMA_CONFIG:           return "Alma Config";
		case KbfFileType::ERIK_CONFIG:           return "Erik Config";
		case KbfFileType::GEMMA_CONFIG:          return "Gemma Config";
		case KbfFileType::SUPPORT_HUNTER_CONFIG: return "Support Hunter Config";
		case KbfFileType::NPC_CONFIG:            return "NPC Config";
		case KbfFileType::PLAYER_CONFIG:         return "Player Config";
		case KbfFileType::DOT_KBF:               return ".kbf File";
		case KbfFileType::FBS_PRESET:            return "FBS Preset";
		case KbfFileType::PRESET:                return "Preset";
		case KbfFileType::PRESET_GROUP:          return "Preset Group";
		case KbfFileType::PLAYER_OVERRIDE:       return "Player Override";
		case KbfFileType::ARMOUR_LIST:           return "Armour List";
		case KbfFileType::BONE_CACHE:            return "Bone Cache";
		case KbfFileType::PART_CACHE:            return "Part Cache";
		case KbfFileType::MATERIAL_CACHE:        return "Material Cache";
		default:                                 return "Unknown";
		}
	}

}