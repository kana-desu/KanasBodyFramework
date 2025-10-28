#pragma once

#include <cstdint>
#include <string>

namespace kbf {

	enum ArmourPiece {
		AP_MIN  = 0,
		AP_SET  = 0, // This is reserved for the player's base transform
		AP_MIN_EXCLUDING_SET = 1,
		AP_ARMS = 1,
		AP_BODY = 2,
		AP_HELM = 3,
		AP_LEGS = 4,
		AP_COIL = 5,
		AP_MAX_EXCLUDING_SLINGER  = 5,
		AP_SLINGER = 6,
		AP_MAX = 6
	};

	typedef enum ArmourPieceFlagBits {
		APF_NONE    = 0b0000000,
		APF_SET     = 0b0000001,
		APF_HELM    = 0b0000010,
		APF_BODY    = 0b0000100,
		APF_ARMS    = 0b0001000,
		APF_COIL    = 0b0010000,
		APF_LEGS    = 0b0100000,
		APF_SLINGER = 0b0000000,
		APF_ALL     = 0b1111111
	} ArmourPieceFlagBits;

	typedef uint32_t ArmourPieceFlags;

	inline std::string armourPieceToString(ArmourPiece piece) {
		switch (piece) {
		case ArmourPiece::AP_SET:     return "Set";
		case ArmourPiece::AP_HELM:    return "Helm";
		case ArmourPiece::AP_BODY:    return "Body";
		case ArmourPiece::AP_ARMS:    return "Arms";
		case ArmourPiece::AP_COIL:    return "Coil";
		case ArmourPiece::AP_LEGS:    return "Legs";
		case ArmourPiece::AP_SLINGER: return "Slinger";
		default: return "Unknown";
		}
	}
	
}