#pragma once

#include <kbf/data/formats/format_metadata.hpp>
#include <kbf/data/preset/preset.hpp>

#include <unordered_map>
#include <string>

namespace kbf {

	struct PresetGroup {
		std::string uuid;
		std::string name;
		bool female;
		std::unordered_map<ArmourSet, std::string> setPresets;
		std::unordered_map<ArmourSet, std::string> helmPresets;
		std::unordered_map<ArmourSet, std::string> bodyPresets;
		std::unordered_map<ArmourSet, std::string> armsPresets;
		std::unordered_map<ArmourSet, std::string> coilPresets;
		std::unordered_map<ArmourSet, std::string> legsPresets;

		FormatMetadata metadata;

		bool operator==(const PresetGroup& other) const {
			return (
				uuid == other.uuid &&
				name == other.name &&
				female == other.female &&
				setPresets == other.setPresets &&
				helmPresets == other.helmPresets &&
				bodyPresets == other.bodyPresets &&
				armsPresets == other.armsPresets &&
				coilPresets == other.coilPresets &&
				legsPresets == other.legsPresets);
		}

		std::unordered_map<ArmourSet, std::string>* getPresetMap(ArmourPiece piece) {
			switch (piece)
			{
			case ArmourPiece::AP_SET:  return &setPresets;
			case ArmourPiece::AP_HELM: return &helmPresets;
			case ArmourPiece::AP_BODY: return &bodyPresets;
			case ArmourPiece::AP_ARMS: return &armsPresets;
			case ArmourPiece::AP_COIL: return &coilPresets;
			case ArmourPiece::AP_LEGS: return &legsPresets;
			default:                   return nullptr;
			}
		}

		bool armourHasPresetUUID(const ArmourSet& armour, ArmourPiece piece) const {
			switch (piece)
			{
			case ArmourPiece::AP_SET:  return setPresets.find(armour) != setPresets.end();
			case ArmourPiece::AP_HELM: return helmPresets.find(armour) != helmPresets.end();
			case ArmourPiece::AP_BODY: return bodyPresets.find(armour) != bodyPresets.end();
			case ArmourPiece::AP_ARMS: return armsPresets.find(armour) != armsPresets.end();
			case ArmourPiece::AP_COIL: return coilPresets.find(armour) != coilPresets.end();
			case ArmourPiece::AP_LEGS: return legsPresets.find(armour) != legsPresets.end();
			default:                   return false;
			}
		}

		size_t size() const {
			return setPresets.size() + helmPresets.size() + bodyPresets.size() + armsPresets.size() + coilPresets.size() + legsPresets.size();
		}
	};

}