#pragma once

#include <kbf/data/formats/format_metadata.hpp>
#include <kbf/data/armour/armour_set.hpp>
#include <kbf/data/preset/preset_piece_settings.hpp>
#include <kbf/data/armour/armour_piece.hpp>

#include <glm/glm.hpp>

namespace kbf {

	struct Preset {
		std::string uuid;
		std::string name;
		std::string bundle;
		bool female;
		ArmourSet armour;

		PresetPieceSettings set;
		PresetPieceSettings helm;
		PresetPieceSettings body;
		PresetPieceSettings arms;
		PresetPieceSettings coil;
		PresetPieceSettings legs;

		bool hideSlinger = false;
		bool hideWeapon = false;

		FormatMetadata metadata;

		bool operator==(const Preset& other) const {
			bool match = true;
			match &= uuid == other.uuid;
			match &= name == other.name;
			match &= bundle == other.bundle;
			match &= female == other.female;
			match &= armour == other.armour;
			match &= set == other.set;
			match &= helm == other.helm;
			match &= body == other.body;
			match &= arms == other.arms;
			match &= coil == other.coil;
			match &= legs == other.legs;
			match &= hideSlinger == other.hideSlinger;
			match &= hideWeapon == other.hideWeapon;
			return match;
		}

		PresetPieceSettings getPieceSettings(ArmourPiece piece) const {
			switch (piece)
			{
			case ArmourPiece::AP_SET:  return set;
			case ArmourPiece::AP_HELM: return helm;
			case ArmourPiece::AP_BODY: return body;
			case ArmourPiece::AP_ARMS: return arms;
			case ArmourPiece::AP_COIL: return coil;
			case ArmourPiece::AP_LEGS: return legs;
			default:                   return {};
			}
		}

		bool hasModifiers(ArmourPiece piece) const {
			switch (piece)
			{
			case ArmourPiece::AP_SET:  return set.hasModifiers();
			case ArmourPiece::AP_HELM: return helm.hasModifiers();
			case ArmourPiece::AP_BODY: return body.hasModifiers();
			case ArmourPiece::AP_ARMS: return arms.hasModifiers();
			case ArmourPiece::AP_COIL: return coil.hasModifiers();
			case ArmourPiece::AP_LEGS: return legs.hasModifiers();
			default:                   return false;
			}
		}

		bool hasRemovers(ArmourPiece piece) const {
			switch (piece)
			{
			case ArmourPiece::AP_SET:  return set.hasPartRemovers();
			case ArmourPiece::AP_HELM: return helm.hasPartRemovers();
			case ArmourPiece::AP_BODY: return body.hasPartRemovers();
			case ArmourPiece::AP_ARMS: return arms.hasPartRemovers();
			case ArmourPiece::AP_COIL: return coil.hasPartRemovers();
			case ArmourPiece::AP_LEGS: return legs.hasPartRemovers();
			default:                   return false;
			}
		}

		bool hasAnyModifiers() const {
			return hasModifiers(ArmourPiece::AP_SET) ||
				   hasModifiers(ArmourPiece::AP_HELM) ||
				   hasModifiers(ArmourPiece::AP_BODY) ||
				   hasModifiers(ArmourPiece::AP_ARMS) ||
				   hasModifiers(ArmourPiece::AP_COIL) ||
				   hasModifiers(ArmourPiece::AP_LEGS);
		}

		bool hasAnyRemovers() const {
			return hasRemovers(ArmourPiece::AP_SET) ||
				   hasRemovers(ArmourPiece::AP_HELM) ||
				   hasRemovers(ArmourPiece::AP_BODY) ||
				   hasRemovers(ArmourPiece::AP_ARMS) ||
				   hasRemovers(ArmourPiece::AP_COIL) ||
				   hasRemovers(ArmourPiece::AP_LEGS);
		}
	};

}