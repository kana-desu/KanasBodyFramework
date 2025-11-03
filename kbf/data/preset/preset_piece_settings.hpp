#pragma once

#include <kbf/data/bones/bone_modifier.hpp>
#include <kbf/data/preset/override_mesh_part.hpp>

#include <set>
#include <map>
#include <unordered_map>

namespace kbf {

	typedef std::map<std::string, BoneModifier> BoneModifierMap;

	struct PresetPieceSettings {
		// Bones
		float modLimit = 1.0f;
		bool useSymmetry = true;
		BoneModifierMap modifiers;

		// Parts
		std::set<OverrideMeshPart> partOverrides;

		bool operator==(const PresetPieceSettings& other) const {
			return (
				modifiers == other.modifiers &&
				modLimit == other.modLimit &&
				useSymmetry == other.useSymmetry &&
				partOverrides == other.partOverrides
			);
		}

		bool hasModifiers() const { return !modifiers.empty(); }
		bool hasPartOverrides() const { return !partOverrides.empty(); }
	};

}