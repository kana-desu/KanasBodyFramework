#pragma once

#include <kbf/data/armour/armour_set.hpp>
#include <kbf/data/armour/armour_piece.hpp>
#include <reframework/API.hpp>

#include <format>
#include <optional>

namespace kbf {

	struct ArmourInfo {
		std::optional<ArmourSet> helm    = std::nullopt;
		std::optional<ArmourSet> body    = std::nullopt;
		std::optional<ArmourSet> arms    = std::nullopt;
		std::optional<ArmourSet> coil    = std::nullopt;
		std::optional<ArmourSet> legs    = std::nullopt;
		std::optional<ArmourSet> slinger = std::nullopt;

		bool operator==(const ArmourInfo& other) const {
			return helm    == other.helm
				&& body    == other.body
				&& arms    == other.arms
				&& coil    == other.coil
				&& legs    == other.legs
				&& slinger == other.slinger;
		}

		std::optional<ArmourSet>& getPiece(ArmourPiece piece) {
			switch (piece) {
			case ArmourPiece::AP_HELM:    return helm;
			case ArmourPiece::AP_BODY:    return body;
			case ArmourPiece::AP_ARMS:    return arms;
			case ArmourPiece::AP_COIL:    return coil;
			case ArmourPiece::AP_LEGS:    return legs;
			case ArmourPiece::AP_SLINGER: return slinger;
			default: throw std::format("Invalid ArmourPiece enum value: {}", static_cast<int>(piece));
			}
		}

		bool isFullyPopulated() const {
			// Slinger is truly optional, so don't check it.
			return helm.has_value() && body.has_value() && arms.has_value() && coil.has_value() && legs.has_value();
		}
	};

}