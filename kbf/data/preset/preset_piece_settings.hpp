#pragma once

#include <kbf/data/bones/bone_modifier.hpp>
#include <kbf/data/preset/override_mesh_part.hpp>
#include <kbf/data/preset/override_material.hpp>

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
		std::set<OverrideMaterial> materialOverrides;

		bool operator==(const PresetPieceSettings& other) const {
			return (
				modifiers == other.modifiers &&
				modLimit == other.modLimit &&
				useSymmetry == other.useSymmetry &&
				partOverrides == other.partOverrides &&
				matOverridesExactlyEqual(materialOverrides, other.materialOverrides)
			);
		}

		static bool matOverridesExactlyEqual(
			const std::set<OverrideMaterial>& a,
			const std::set<OverrideMaterial>& b
		) {
			if (a.size() != b.size()) return false;

			auto itA = a.begin();
			auto itB = b.begin();

			while (itA != a.end()) {
				if (!itA->isExactlyEqual(*itB)) return false;
				++itA;
				++itB;
			}

			return true;
		}

		bool hasMatOverride(MeshMaterial material) {
			for (const auto& matOverride : materialOverrides) {
				if (matOverride.material.name == material.name) return true;
			}
			return false;
		}

		bool hasModifiers() const { return !modifiers.empty(); }
		bool hasPartOverrides() const { return !partOverrides.empty(); }
		bool hasMaterialOverrides() const { return !materialOverrides.empty(); }
	};

}