#pragma once

#include <kbf/data/kbf_data_manager.hpp>
#include <kbf/data/armour/armour_info.hpp>
#include <kbf/data/preset/preset.hpp>

#include <reframework/API.hpp>

#include <array>

using REApi = reframework::API;

namespace kbf {

	class BoneManager {
	public:
		BoneManager(
			KBFDataManager& datamanager, 
			ArmourInfo armour, 
			REApi::ManagedObject* baseTransform,
			REApi::ManagedObject* helmTransform,
			REApi::ManagedObject* bodyTransform,
			REApi::ManagedObject* armsTransform,
			REApi::ManagedObject* coilTransform,
			REApi::ManagedObject* legsTransform,
			bool female);

		enum BoneApplyStatusFlag {
			BONE_APPLY_SUCCESS            = 0x00000000,
			BONE_APPLY_ERROR_NULL_PRESET  = 0x00000001,
			BONE_APPLY_ERROR_INVALID_BONE = 0x00000002
		};

		BoneApplyStatusFlag applyPreset(const Preset* preset, ArmourPiece piece);
		bool loadBones();
		bool loadTransformBones(
			ArmourPiece piece,
			REApi::ManagedObject* transform,
			std::unordered_map<std::string, REApi::ManagedObject*>& outMap);

		bool isInitialized() const { return initialized; }

	private:
		bool modifyBone(REApi::ManagedObject* bone, const BoneModifier& modifier);

		std::unordered_map<std::string, REApi::ManagedObject*> getBoneNames(REApi::ManagedObject* jointArr) const;
		void DEBUG_printBoneList(REApi::ManagedObject* jointArr, std::string message) const;

		KBFDataManager& dataManager;
		ArmourInfo armourInfo;
		bool female;
		bool initialized = false;

		std::array<std::unordered_map<std::string, REApi::ManagedObject*>, 6> partBones;
		std::array<REApi::ManagedObject*, 6> partTransforms;
	};

}