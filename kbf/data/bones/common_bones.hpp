#pragma once

#include <string>
#include <set>
#include <unordered_map>

#include <kbf/data/bones/default_bones.hpp>

// Upper Body
#define COMMON_BONE_CATEGORY_HEAD       "Head Bones"
#define COMMON_BONE_CATEGORY_CHEST      "Chest Bones"
#define COMMON_BONE_CATEGORY_BACK       "Back Bones"
#define COMMON_BONE_CATEGORY_UPPER_ARMS "Upper Arm Bones"
#define COMMON_BONE_CATEGORY_LOWER_ARMS "Lower Arm Bones"
#define COMMON_BONE_CATEGORY_HANDS      "Hand Bones"
#define COMMON_BONE_CATEGORY_SPINE      "Spine Bones"
// Lower Body
#define COMMON_BONE_CATEGORY_HIP       "Hip Bones"
#define COMMON_BONE_CATEGORY_THIGH     "Thigh Bones"
#define COMMON_BONE_CATEGORY_LOWER_LEG "Lower Leg Bones"
#define COMMON_BONE_CATEGORY_FEET      "Foot Bones"
// Other
#define COMMON_BONE_CATEGORY_CUSTOM "Common Custom Bones"
#define COMMON_BONE_CATEGORY_OTHER  "Other Bones"

namespace kbf {

	inline std::set<std::string> COMMON_HEAD_BONES([] {
		std::set<std::string> bones;
		bones.insert(BASE_NECK_BONES.begin(), BASE_NECK_BONES.end());
		bones.insert(BASE_HEAD_BONES.begin(), BASE_HEAD_BONES.end());
		bones.insert(BASE_EYE_BONES.begin(), BASE_EYE_BONES.end());
		return bones;
	}());

	inline std::set<std::string> COMMON_CHEST_BONES([] {
		std::set<std::string> bones;
		bones.insert(BASE_PEC_BONES.begin(), BASE_PEC_BONES.end());
		bones.insert(BASE_BUST_BONES.begin(), BASE_BUST_BONES.end());
		bones.insert("L_Bust_CH_00");
		bones.insert("R_Bust_CH_00");
		bones.insert("L_BustJGL_CH_00");
		bones.insert("R_BustJGL_CH_00");
		bones.insert("L_Bust_JGL_HJ_02");
		bones.insert("R_Bust_JGL_HJ_02");
		return bones;
	}());

	inline std::set<std::string> COMMON_BACK_BONES([] {
		std::set<std::string> bones;
		bones.insert(BASE_LATS_BONES.begin(), BASE_LATS_BONES.end());
		bones.insert(BASE_TRAPS_BONES.begin(), BASE_TRAPS_BONES.end());
		return bones;
	}());

	inline std::set<std::string> COMMON_UPPER_ARM_BONES([] {
		std::set<std::string> bones;
		bones.insert(BASE_SHOULDER_BONES.begin(), BASE_SHOULDER_BONES.end());
		bones.insert(BASE_DELTOID_BONES.begin(), BASE_DELTOID_BONES.end());
		bones.insert(BASE_UPPER_ARM_BONES.begin(), BASE_UPPER_ARM_BONES.end());
		bones.insert(BASE_BICEPS_BONES.begin(), BASE_BICEPS_BONES.end());
		bones.insert(BASE_TRICEPS_BONES.begin(), BASE_TRICEPS_BONES.end());
		return bones;
	}());

	inline std::set<std::string> COMMON_LOWER_ARM_BONES([] {
		std::set<std::string> bones;
		bones.insert(BASE_ELBOW_BONES.begin(), BASE_ELBOW_BONES.end());
		bones.insert(BASE_FOREARM_BONES.begin(), BASE_FOREARM_BONES.end());
		return bones;
	}());

	inline std::set<std::string> COMMON_HAND_BONES = BASE_HAND_BONES;

	inline std::set<std::string> COMMON_SPINE_BONES([] {
		std::set<std::string> bones;
		bones.insert(BASE_SPINE_BONES.begin(), BASE_SPINE_BONES.end());
		bones.insert("Spine_1_WpConst");
		bones.insert("Spine_2_WpConst");
		return bones;
	}());

	inline std::set<std::string> COMMON_HIP_BONES([] {
		std::set<std::string> bones;
		bones.insert(BASE_HIP_BONES.begin(), BASE_HIP_BONES.end());
		bones.insert("C Hip_HJ_00");
		return bones;
	}());

	inline std::set<std::string> COMMON_THIGH_BONES = BASE_THIGH_BONES;

	inline std::set<std::string> COMMON_LOWER_LEG_BONES([] {
		std::set<std::string> bones;
		bones.insert(BASE_KNEE_BONES.begin(), BASE_KNEE_BONES.end());
		bones.insert(BASE_SHIN_BONES.begin(), BASE_SHIN_BONES.end());
		bones.insert(BASE_CALF_BONES.begin(), BASE_CALF_BONES.end());
		return bones;
	}());

	inline std::set<std::string> COMMON_FOOT_BONES = BASE_FOOT_BONES;

	inline std::set<std::string> COMMON_CUSTOM_BONES = {
		// Body
		"CustomBone1",
		"CustomBone2",
		"CustomBone3",
		"CustomBone4",
		"CustomBone5",
		"CustomBone6",
		"CustomBone7",
		"CustomBone8",
		// Legs
		"CustomBone9",
		"CustomBone10",
		"CustomBone11",
		"CustomBone12",
		"CustomBone13",
		"CustomBone14",
		"CustomBone15",
		"CustomBone16",
	};

	inline const std::unordered_map<std::string, const std::set<std::string>*> BONE_CATEGORIES = {
		{ COMMON_BONE_CATEGORY_HEAD,       &COMMON_HEAD_BONES },
		{ COMMON_BONE_CATEGORY_BACK,       &COMMON_BACK_BONES },
		{ COMMON_BONE_CATEGORY_CHEST,      &COMMON_CHEST_BONES },
		{ COMMON_BONE_CATEGORY_UPPER_ARMS, &COMMON_UPPER_ARM_BONES },
		{ COMMON_BONE_CATEGORY_LOWER_ARMS, &COMMON_LOWER_ARM_BONES },
		{ COMMON_BONE_CATEGORY_HANDS,      &COMMON_HAND_BONES },
		{ COMMON_BONE_CATEGORY_SPINE,      &COMMON_SPINE_BONES },
		{ COMMON_BONE_CATEGORY_HIP,        &COMMON_HIP_BONES },
		{ COMMON_BONE_CATEGORY_THIGH,      &COMMON_THIGH_BONES },
		{ COMMON_BONE_CATEGORY_LOWER_LEG,  &COMMON_LOWER_LEG_BONES },
		{ COMMON_BONE_CATEGORY_FEET,       &COMMON_FOOT_BONES },
		{ COMMON_BONE_CATEGORY_CUSTOM,     &COMMON_CUSTOM_BONES }
	};

	inline std::string getCommonBoneCategory(const std::string& boneName) {
		for (const auto& [category, boneSet] : BONE_CATEGORIES) {
			if (boneSet->find(boneName) != boneSet->end()) {
				return category;
			}
		}
		return COMMON_BONE_CATEGORY_OTHER;
	}

	inline bool isHeadBone(const std::string& boneName) {
		bool isHead = false;
		isHead |= COMMON_HEAD_BONES.find(boneName) != COMMON_HEAD_BONES.end();
		return isHead;
	}

	inline bool isBodyBone(const std::string& boneName) {
		bool isBody = false;
		isBody |= COMMON_CHEST_BONES.find(boneName) != COMMON_CHEST_BONES.end();
		isBody |= COMMON_BACK_BONES.find(boneName) != COMMON_BACK_BONES.end();
		isBody |= COMMON_SPINE_BONES.find(boneName) != COMMON_SPINE_BONES.end();
		isBody |= COMMON_HIP_BONES.find(boneName) != COMMON_HIP_BONES.end();
		return isBody;
	}

	inline bool isArmsBone(const std::string& boneName) {
		bool isArms = false;
		isArms |= COMMON_UPPER_ARM_BONES.find(boneName) != COMMON_UPPER_ARM_BONES.end();
		isArms |= COMMON_LOWER_ARM_BONES.find(boneName) != COMMON_LOWER_ARM_BONES.end();
		isArms |= COMMON_HAND_BONES.find(boneName) != COMMON_HAND_BONES.end();
		return isArms;
	}

	inline bool isLegsBone(const std::string& boneName) {
		bool isLegs = false;
		isLegs |= COMMON_HIP_BONES.find(boneName) != COMMON_HIP_BONES.end();
		isLegs |= COMMON_THIGH_BONES.find(boneName) != COMMON_THIGH_BONES.end();
		isLegs |= COMMON_LOWER_LEG_BONES.find(boneName) != COMMON_LOWER_LEG_BONES.end();
		isLegs |= COMMON_FOOT_BONES.find(boneName) != COMMON_FOOT_BONES.end();
		return isLegs;
	}

	inline bool isCustomOrUncommonBone(const std::string& boneName) {
		bool isCustomOrUncommon = false;
		isCustomOrUncommon |= COMMON_CUSTOM_BONES.find(boneName) != COMMON_CUSTOM_BONES.end();
		isCustomOrUncommon |= getCommonBoneCategory(boneName) == COMMON_BONE_CATEGORY_OTHER;
		return isCustomOrUncommon;
	}

}