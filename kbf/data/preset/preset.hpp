#pragma once

#include <kbf/data/formats/format_metadata.hpp>
#include <kbf/data/armour/armour_set.hpp>
#include <kbf/data/preset/preset_piece_settings.hpp>
#include <kbf/data/armour/armour_piece.hpp>
#include <kbf/data/preset/quick_material_override.hpp>

#include <glm/glm.hpp>

#include <algorithm>

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

		std::unordered_map<std::string, QuickMaterialOverride<float>> quickMaterialOverridesFloat{
			{ "wetness",       QuickMaterialOverride<float>{ false, "skin", "...", 0.0f } },
			{ "wet_roughness", QuickMaterialOverride<float>{ false, "skin", "...", 0.0f } },
		};
		std::unordered_map<std::string, QuickMaterialOverride<glm::vec4>> quickMaterialOverridesVec4{};
		
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
			match &= quickMaterialOverridesFloat == other.quickMaterialOverridesFloat;
			match &= quickMaterialOverridesVec4 == other.quickMaterialOverridesVec4;
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

		bool hasPartOverrides(ArmourPiece piece) const {
			switch (piece)
			{
			case ArmourPiece::AP_SET:  return set.hasPartOverrides();
			case ArmourPiece::AP_HELM: return helm.hasPartOverrides();
			case ArmourPiece::AP_BODY: return body.hasPartOverrides();
			case ArmourPiece::AP_ARMS: return arms.hasPartOverrides();
			case ArmourPiece::AP_COIL: return coil.hasPartOverrides();
			case ArmourPiece::AP_LEGS: return legs.hasPartOverrides();
			default:                   return false;
			}
		}

		bool hasMaterialOverrides(ArmourPiece piece) const {
			switch (piece)
			{
			case ArmourPiece::AP_SET:  return set.hasMaterialOverrides();
			case ArmourPiece::AP_HELM: return helm.hasMaterialOverrides();
			case ArmourPiece::AP_BODY: return body.hasMaterialOverrides();
			case ArmourPiece::AP_ARMS: return arms.hasMaterialOverrides();
			case ArmourPiece::AP_COIL: return coil.hasMaterialOverrides();
			case ArmourPiece::AP_LEGS: return legs.hasMaterialOverrides();
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

		bool hasAnyPartOverrides() const {
			return hideSlinger || hideWeapon ||
				   hasPartOverrides(ArmourPiece::AP_SET) ||
				   hasPartOverrides(ArmourPiece::AP_HELM) ||
				   hasPartOverrides(ArmourPiece::AP_BODY) ||
				   hasPartOverrides(ArmourPiece::AP_ARMS) ||
				   hasPartOverrides(ArmourPiece::AP_COIL) ||
				   hasPartOverrides(ArmourPiece::AP_LEGS);
		}

		bool hasAnyMaterialOverrides() const {
			bool anyQuickOverridesFloat = std::ranges::any_of(quickMaterialOverridesFloat, [](auto& p) { return p.second.enabled; });
			bool anyQuickOverridesVec4 = std::ranges::any_of(quickMaterialOverridesVec4, [](auto& p) { return p.second.enabled; });

			return anyQuickOverridesFloat || 
				   anyQuickOverridesVec4 ||
				   hasMaterialOverrides(ArmourPiece::AP_SET) ||
				   hasMaterialOverrides(ArmourPiece::AP_HELM) ||
				   hasMaterialOverrides(ArmourPiece::AP_BODY) ||
				   hasMaterialOverrides(ArmourPiece::AP_ARMS) ||
				   hasMaterialOverrides(ArmourPiece::AP_COIL) ||
				   hasMaterialOverrides(ArmourPiece::AP_LEGS);
		}

	};

}