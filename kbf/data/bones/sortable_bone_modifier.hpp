#pragma once

#include <kbf/data/preset/preset.hpp>

#include <string>

namespace kbf {

    struct SortableBoneModifier {
        std::string name;
		bool isSymmetryProxy = false;    // True if this is a symmetry proxy modifier (e.g. "L_Bone" and "R_Bone" are combined into slider)
        BoneModifier* modifier;          // L_Bone.
		BoneModifier* reflectedModifier; // R_Bone, if this is a symmetry proxy.
        std::string boneName;
        std::string reflectedBoneName;
    };

}