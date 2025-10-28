#pragma once

#include <kbf/data/bones/bone_modifier.hpp>
#include <kbf/data/mesh/mesh_part.hpp>

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
		std::set<MeshPart> removedParts;

		bool operator==(const PresetPieceSettings& other) const {
			return (
				modifiers == other.modifiers &&
				modLimit == other.modLimit &&
				useSymmetry == other.useSymmetry &&
				removedParts == other.removedParts
			);
		}

		bool hasModifiers() const { return !modifiers.empty(); }
		bool hasPartRemovers() const { return !removedParts.empty(); }
	};

}