#pragma once

#include <kbf/data/armour/armour_piece.hpp>

#include <string>

namespace kbf {

	struct ArmourID {
		std::string helm;
		std::string body;
		std::string arms;
		std::string coil;
		std::string legs;
		std::string slinger;

		std::string getPiece(ArmourPiece piece) const {
			switch (piece) {
			case ArmourPiece::AP_HELM:    return helm;
			case ArmourPiece::AP_BODY:    return body;
			case ArmourPiece::AP_ARMS:    return arms;
			case ArmourPiece::AP_COIL:    return coil;
			case ArmourPiece::AP_LEGS:    return legs;
			case ArmourPiece::AP_SLINGER: return slinger;
			}

			return "";
		}

		bool hasPiece(ArmourPiece piece) const {
			std::string p = getPiece(piece);
			return !p.empty();
		}

	};

}