#pragma once

#include <string>
#include <set>

namespace kbf {

	// ======================================================================= Base Bones (To compose sets from)===================================

	inline std::set<std::string> BASE_NECK_BONES = {
		"Neck_0",
		"Neck_1",
		"Neck_0_HJ_00",
		"Neck_1_HJ_00",
	};

	inline std::set<std::string> BASE_HEAD_BONES = {
		"Head",
		"HeadRX_HJ_01",
	};

	inline std::set<std::string> BASE_EYE_BONES = {
		"L_Eye",
		"R_Eye",
	};

	inline std::set<std::string> BASE_SPINE_BONES = {
		"Spine_0",
		"Spine_1",
		"Spine_2",
		"Spine_0_HJ_00",
		"Spine_1_HJ_00",
		"Spine_2_HJ_00",
	};

	inline std::set<std::string> BASE_DELTOID_BONES = {
		"L_Deltoid_HJ_00",
		"L_Deltoid_HJ_01",
		"L_Deltoid_HJ_02",
		"R_Deltoid_HJ_00",
		"R_Deltoid_HJ_01",
		"R_Deltoid_HJ_02",
	};

	inline std::set<std::string> BASE_PEC_BONES = {
		"L_Pec_HJ_00",
		"L_Pec_HJ_01",
		"R_Pec_HJ_00",
		"R_Pec_HJ_01",
	};

	inline std::set<std::string> BASE_BUST_BONES = {
		"L_Bust_HJ_00",
		"L_Bust_HJ_01",
		"R_Bust_HJ_00",
		"R_Bust_HJ_01",
	};

	inline std::set<std::string> BASE_SHOULDER_BONES = {
		"L_Shoulder",
		"R_Shoulder",
		"L_Shoulder_HJ_00",
		"R_Shoulder_HJ_00",
	};

	inline std::set<std::string> BASE_TRAPS_BONES = {
		"L_Traps_HJ_00",
		"L_Traps_HJ_01",
		"R_Traps_HJ_00",
		"R_Traps_HJ_01",
	};

	inline std::set<std::string> BASE_LATS_BONES = {
		"L_Lats_HJ_00",
		"L_Lats_HJ_01",
		"R_Lats_HJ_00",
		"R_Lats_HJ_01",
	};

	inline std::set<std::string> BASE_UPPER_ARM_BONES = {
		"L_UpperArm",
		"R_UpperArm",
		"L_UpperArm_HJ_00",
		"R_UpperArm_HJ_00",
		"L_UpperArmTwist_HJ_00",
		"L_UpperArmTwist_HJ_01",
		"L_UpperArmTwist_HJ_02",
		"R_UpperArmTwist_HJ_00",
		"R_UpperArmTwist_HJ_01",
		"R_UpperArmTwist_HJ_02",
		"L_UpperArmDouble_HJ_00",
		"R_UpperArmDouble_HJ_00",
	};

	inline std::set<std::string> BASE_BICEPS_BONES = {
		"L_Biceps_HJ_00",
		"L_Biceps_HJ_01",
		"R_Biceps_HJ_00",
		"R_Biceps_HJ_01",
	};

	inline std::set<std::string> BASE_TRICEPS_BONES = {
		"L_Triceps_HJ_00",
		"R_Triceps_HJ_00",
	};

	inline std::set<std::string> BASE_FOREARM_BONES = {
		"L_Forearm",
		"R_Forearm",
		"L_Forearm_HJ_00",
		"R_Forearm_HJ_00",
		"R_ForearmTwist_HJ_00",
		"R_ForearmTwist_HJ_01",
		"R_ForearmTwist_HJ_02",
		"L_ForearmTwist_HJ_02",
		"L_ForearmRY_HJ_00",
		"L_ForearmRY_HJ_01",
		"R_ForearmRY_HJ_00",
		"R_ForearmRY_HJ_01",
		"L_ForearmTwist_HJ_01",
		"L_ForearmTwist_HJ_00",
		"L_ForearmDouble_HJ_00",
		"R_ForearmDouble_HJ_00",
	};

	inline std::set<std::string> BASE_ELBOW_BONES = {
		"L_Elbow_HJ_00",
		"R_Elbow_HJ_00",
	};

	inline std::set<std::string> BASE_WEAPON_BONES = {
		"L_Wep",
		"R_Wep",
		"L_Wep_Sub",
		"R_Wep_Sub",
		"R_Shield",
	};

	inline std::set<std::string> BASE_HAND_BONES = {
		"L_Hand",
		"R_Hand",
		"L_Hand_HJ_00",
		"L_Hand_HJ_01",
		"R_Hand_HJ_00",
		"R_Hand_HJ_01",
		"L_HandRZ_HJ_00",
		"R_HandRZ_HJ_00",
		"L_Palm",
		"R_Palm",
		"L_Thumb1",
		"L_Thumb2",
		"L_Thumb3",
		"R_Thumb1",
		"R_Thumb2",
		"R_Thumb3",
		"L_Thumb_HJ_00",
		"L_Thumb_HJ_01",
		"L_Thumb_HJ_02",
		"L_Thumb_HJ_03",
		"R_Thumb_HJ_00",
		"R_Thumb_HJ_01",
		"R_Thumb_HJ_02",
		"R_Thumb_HJ_03",
		"L_IndexF1",
		"L_IndexF2",
		"L_IndexF3",
		"R_IndexF1",
		"R_IndexF2",
		"R_IndexF3",
		"L_IndexF_HJ_00",
		"L_IndexF_HJ_01",
		"L_IndexF_HJ_02",
		"L_IndexF_HJ_03",
		"L_IndexF_HJ_04",
		"R_IndexF_HJ_00",
		"R_IndexF_HJ_01",
		"R_IndexF_HJ_02",
		"R_IndexF_HJ_03",
		"R_IndexF_HJ_04",
		"L_MiddleF1",
		"L_MiddleF2",
		"L_MiddleF3",
		"R_MiddleF1",
		"R_MiddleF2",
		"R_MiddleF3",
		"L_MiddleF_HJ_00",
		"L_MiddleF_HJ_01",
		"L_MiddleF_HJ_02",
		"L_MiddleF_HJ_03",
		"L_MiddleF_HJ_04",
		"R_MiddleF_HJ_00",
		"R_MiddleF_HJ_01",
		"R_MiddleF_HJ_02",
		"R_MiddleF_HJ_03",
		"R_MiddleF_HJ_04",
		"L_RingF1",
		"L_RingF2",
		"L_RingF3",
		"R_RingF1",
		"R_RingF2",
		"R_RingF3",
		"L_RingF_HJ_00",
		"L_RingF_HJ_01",
		"L_RingF_HJ_02",
		"L_RingF_HJ_03",
		"L_RingF_HJ_04",
		"R_RingF_HJ_00",
		"R_RingF_HJ_01",
		"R_RingF_HJ_02",
		"R_RingF_HJ_03",
		"R_RingF_HJ_04",
		"L_PinkyF1",
		"L_PinkyF2",
		"L_PinkyF3",
		"R_PinkyF1",
		"R_PinkyF2",
		"R_PinkyF3",
		"L_PinkyF_HJ_00",
		"L_PinkyF_HJ_01",
		"L_PinkyF_HJ_02",
		"L_PinkyF_HJ_03",
		"L_PinkyF_HJ_04",
		"R_PinkyF_HJ_03",
		"R_PinkyF_HJ_02",
		"R_PinkyF_HJ_00",
		"R_PinkyF_HJ_01",
		"R_PinkyF_HJ_04",
	};

	inline std::set<std::string> BASE_HIP_BONES = {
		"Hip",
		"Hip_HJ_00",
		"L_Hip_HJ_00",
		"L_Hip_HJ_01",
		"R_Hip_HJ_00",
		"R_Hip_HJ_01",
	};

	inline std::set<std::string> BASE_THIGH_BONES = {
		"L_Thigh",
		"R_Thigh",
		"L_ThighTwist_HJ_00",
		"L_ThighTwist_HJ_01",
		"L_ThighTwist_HJ_02",
		"R_ThighTwist_HJ_00",
		"R_ThighTwist_HJ_01",
		"R_ThighTwist_HJ_02",
		"L_ThighRX_HJ_00",
		"L_ThighRX_HJ_01",
		"R_ThighRX_HJ_00",
		"R_ThighRX_HJ_01",
		"L_ThighRZ_HJ_00",
		"L_ThighRZ_HJ_01",
		"R_ThighRZ_HJ_00",
		"R_ThighRZ_HJ_01",
	};

	inline std::set<std::string> BASE_KNEE_BONES = {
		"L_Knee",
		"R_Knee",
		"L_KneeRX_HJ_00",
		"R_KneeRX_HJ_00",
		"L_Knee_HJ_00",
		"R_Knee_HJ_00",
		"L_KneeDouble_HJ_00",
		"R_KneeDouble_HJ_00",
	};

	inline std::set<std::string> BASE_SHIN_BONES = {
		"L_Shin",
		"R_Shin",
		"L_Shin_HJ_00",
		"L_Shin_HJ_01",
		"R_Shin_HJ_00",
		"R_Shin_HJ_01",
	};

	inline std::set<std::string> BASE_CALF_BONES = {
		"L_Calf_HJ_00",
		"R_Calf_HJ_00",
	};

	inline std::set<std::string> BASE_FOOT_BONES = {
		"L_Foot",
		"R_Foot",
		"L_Foot_HJ_00",
		"R_Foot_HJ_00",
		"L_Instep",
		"R_Instep",
	};

	inline std::set<std::string> BASE_TOE_BONES = {
		"L_Toe",
		"R_Toe",
	};

	// ======================================================================= Male Default Bones ===================================================

	inline std::set<std::string> DEFAULT_BONES_MALE_HEAD([] {
		std::set<std::string> bones;
		bones.insert(BASE_NECK_BONES.begin(), BASE_NECK_BONES.end());
		bones.insert(BASE_HEAD_BONES.begin(), BASE_HEAD_BONES.end());
		bones.insert(BASE_EYE_BONES.begin(), BASE_EYE_BONES.end());
		bones.insert(BASE_SPINE_BONES.begin(), BASE_SPINE_BONES.end());
		return bones;
	}());

	inline std::set<std::string> DEFAULT_BONES_MALE_BODY([] {
		std::set<std::string> bones;
		bones.insert(BASE_SPINE_BONES.begin(), BASE_SPINE_BONES.end());
		bones.insert(BASE_DELTOID_BONES.begin(), BASE_DELTOID_BONES.end());
		bones.insert(BASE_PEC_BONES.begin(), BASE_PEC_BONES.end());
		bones.insert(BASE_SHOULDER_BONES.begin(), BASE_SHOULDER_BONES.end());
		bones.insert(BASE_TRAPS_BONES.begin(), BASE_TRAPS_BONES.end());
		bones.insert(BASE_LATS_BONES.begin(), BASE_LATS_BONES.end());

		bones.insert(BASE_UPPER_ARM_BONES.begin(), BASE_UPPER_ARM_BONES.end());
		bones.insert(BASE_BICEPS_BONES.begin(), BASE_BICEPS_BONES.end());
		bones.insert(BASE_TRICEPS_BONES.begin(), BASE_TRICEPS_BONES.end());
		bones.insert(BASE_FOREARM_BONES.begin(), BASE_FOREARM_BONES.end());
		bones.insert(BASE_ELBOW_BONES.begin(), BASE_ELBOW_BONES.end());

		bones.insert(BASE_NECK_BONES.begin(), BASE_NECK_BONES.end());

		bones.insert(BASE_WEAPON_BONES.begin(), BASE_WEAPON_BONES.end());
		return bones;
	}());

	inline std::set<std::string> DEFAULT_BONES_MALE_ARMS([] {
		std::set<std::string> bones;
		bones.insert(BASE_SHOULDER_BONES.begin(), BASE_SHOULDER_BONES.end());
		bones.insert(BASE_DELTOID_BONES.begin(), BASE_DELTOID_BONES.end());
		bones.insert(BASE_UPPER_ARM_BONES.begin(), BASE_UPPER_ARM_BONES.end());
		bones.insert(BASE_BICEPS_BONES.begin(), BASE_BICEPS_BONES.end());
		bones.insert(BASE_TRICEPS_BONES.begin(), BASE_TRICEPS_BONES.end());
		bones.insert(BASE_FOREARM_BONES.begin(), BASE_FOREARM_BONES.end());
		bones.insert(BASE_ELBOW_BONES.begin(), BASE_ELBOW_BONES.end());
		bones.insert(BASE_HAND_BONES.begin(), BASE_HAND_BONES.end());
		return bones;
	}());

	inline std::set<std::string> DEFAULT_BONES_MALE_WAIST([] {
		std::set<std::string> bones;
		bones.insert(BASE_HIP_BONES.begin(), BASE_HIP_BONES.end());
		bones.insert(BASE_SPINE_BONES.begin(), BASE_SPINE_BONES.end());
		bones.insert(BASE_THIGH_BONES.begin(), BASE_THIGH_BONES.end());
		return bones;
	}());

	inline std::set<std::string> DEFAULT_BONES_MALE_LEGS([] {
		std::set<std::string> bones;
		bones.insert(BASE_HIP_BONES.begin(), BASE_HIP_BONES.end());
		bones.insert(BASE_THIGH_BONES.begin(), BASE_THIGH_BONES.end());
		bones.insert(BASE_KNEE_BONES.begin(), BASE_KNEE_BONES.end());
		bones.insert(BASE_SHIN_BONES.begin(), BASE_SHIN_BONES.end());
		bones.insert(BASE_CALF_BONES.begin(), BASE_CALF_BONES.end());
		bones.insert(BASE_FOOT_BONES.begin(), BASE_FOOT_BONES.end());
		bones.insert(BASE_TOE_BONES.begin(), BASE_TOE_BONES.end());
		return bones;
	}());

	inline std::set<std::string> DEFAULT_BONES_MALE_SET([] {
		std::set<std::string> allBones;
		allBones.insert(DEFAULT_BONES_MALE_HEAD.begin(), DEFAULT_BONES_MALE_HEAD.end());
		allBones.insert(DEFAULT_BONES_MALE_BODY.begin(), DEFAULT_BONES_MALE_BODY.end());
		allBones.insert(DEFAULT_BONES_MALE_ARMS.begin(), DEFAULT_BONES_MALE_ARMS.end());
		allBones.insert(DEFAULT_BONES_MALE_WAIST.begin(), DEFAULT_BONES_MALE_WAIST.end());
		allBones.insert(DEFAULT_BONES_MALE_LEGS.begin(), DEFAULT_BONES_MALE_LEGS.end());
		return allBones;
	}());

	// ===================================================================== Female Default Bones ===================================================

	inline std::set<std::string> DEFAULT_BONES_FEMALE_HEAD = DEFAULT_BONES_MALE_HEAD;
	inline std::set<std::string> DEFAULT_BONES_FEMALE_BODY([] {
		std::set<std::string> bones = DEFAULT_BONES_MALE_BODY;
		bones.insert(BASE_BUST_BONES.begin(), BASE_BUST_BONES.end());
		return bones;
	}());
	inline std::set<std::string> DEFAULT_BONES_FEMALE_ARMS  = DEFAULT_BONES_MALE_ARMS;
	inline std::set<std::string> DEFAULT_BONES_FEMALE_WAIST = DEFAULT_BONES_MALE_WAIST;
	inline std::set<std::string> DEFAULT_BONES_FEMALE_LEGS  = DEFAULT_BONES_MALE_LEGS;
	inline std::set<std::string> DEFAULT_BONES_FEMALE_SET([] {
		std::set<std::string> allBones;
		allBones.insert(DEFAULT_BONES_FEMALE_HEAD.begin(), DEFAULT_BONES_FEMALE_HEAD.end());
		allBones.insert(DEFAULT_BONES_FEMALE_BODY.begin(), DEFAULT_BONES_FEMALE_BODY.end());
		allBones.insert(DEFAULT_BONES_FEMALE_ARMS.begin(), DEFAULT_BONES_FEMALE_ARMS.end());
		allBones.insert(DEFAULT_BONES_FEMALE_WAIST.begin(), DEFAULT_BONES_FEMALE_WAIST.end());
		allBones.insert(DEFAULT_BONES_FEMALE_LEGS.begin(), DEFAULT_BONES_FEMALE_LEGS.end());
		return allBones;
	}());

	// ======================================================================= Utility Functions ===================================================

	inline std::set<std::string> DEFAULT_BONES_ERROR = {};

	inline const std::set<std::string>& getDefaultBones(ArmourPiece piece, bool female) {
		if (female) {
			switch (piece) {
			case ArmourPiece::AP_SET: return DEFAULT_BONES_FEMALE_SET;
			case ArmourPiece::AP_HELM: return DEFAULT_BONES_FEMALE_HEAD;
			case ArmourPiece::AP_BODY: return DEFAULT_BONES_FEMALE_BODY;
			case ArmourPiece::AP_ARMS: return DEFAULT_BONES_FEMALE_ARMS;
			case ArmourPiece::AP_COIL: return DEFAULT_BONES_FEMALE_WAIST;
			case ArmourPiece::AP_LEGS: return DEFAULT_BONES_FEMALE_LEGS;
			}
		}
		else {
			switch (piece) {
			case ArmourPiece::AP_SET: return DEFAULT_BONES_MALE_SET;
			case ArmourPiece::AP_HELM: return DEFAULT_BONES_MALE_HEAD;
			case ArmourPiece::AP_BODY: return DEFAULT_BONES_MALE_BODY;
			case ArmourPiece::AP_ARMS: return DEFAULT_BONES_MALE_ARMS;
			case ArmourPiece::AP_COIL: return DEFAULT_BONES_MALE_WAIST;
			case ArmourPiece::AP_LEGS: return DEFAULT_BONES_MALE_LEGS;
			}
		}

		return DEFAULT_BONES_ERROR;
	}

}