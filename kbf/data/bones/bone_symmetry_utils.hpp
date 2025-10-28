#pragma once

#include <kbf/data/bones/sortable_bone_modifier.hpp>

#include <string>
#include <unordered_set>

namespace kbf {

	inline bool isLeftBone(const std::string& boneName) {
		return boneName.starts_with("L_");
	}

	inline bool isRightBone(const std::string& boneName) {
		return boneName.starts_with("R_");
	}

	inline bool isLeftOrRightBone(const std::string& boneName) {
		return isLeftBone(boneName) || isRightBone(boneName);
	}

	inline std::string getBoneComplement(const std::string& boneName) {
		if (isLeftBone(boneName)) {
			return "R_" + boneName.substr(2); // Remove "L_" prefix
		}
		else if (isRightBone(boneName)) {
			return "L_" + boneName.substr(2); // Remove "R_" prefix
		}
		return boneName; // No complement if not L/R
	}

	inline std::string getBoneStem(const std::string& boneName) {
		if (isLeftOrRightBone(boneName)) {
			return boneName.substr(2); // Remove "L_" or "R_" prefix
		}
		return boneName; // No stem if not L/R
	}

	inline SortableBoneModifier getSymmetryProxyModifier(
		const std::string& boneName,
		BoneModifierMap& boneModifiers,
		std::string* boneComplementOut = nullptr
	) {
		std::string stem = getBoneStem(boneName);

		if (isLeftOrRightBone(boneName)) {
			std::string complement = getBoneComplement(boneName);
			bool complementExists = boneModifiers.find(complement) != boneModifiers.end();

			if (complementExists) {
				bool complementIsLeft = isLeftBone(complement);
				std::string leftBone  = complementIsLeft ? complement : boneName;
				std::string rightBone = complementIsLeft ? boneName : complement;

				if (boneComplementOut) *boneComplementOut = complement;
				return SortableBoneModifier{ stem, true, &boneModifiers.at(leftBone), &boneModifiers.at(rightBone), leftBone, rightBone };
			}
			else {
				if (boneComplementOut) *boneComplementOut = boneName;
				return SortableBoneModifier{ boneName, false, &boneModifiers.at(boneName), nullptr, boneName, "" };
			}
		}

		if (boneComplementOut) *boneComplementOut = boneName;
		return SortableBoneModifier{ boneName, false, &boneModifiers.at(boneName), nullptr, boneName, "" };
	}

}