#include <kbf/mesh/bone_manager.hpp>

#include <kbf/util/re_engine/reinvoke.hpp>
#include <kbf/debug/debug_stack.hpp>
#include <kbf/util/string/ptr_to_hex_string.hpp>
#include <kbf/util/re_engine/check_re_ptr_validity.hpp>

#include <kbf/data/bones/bone_cache_manager.hpp>
#include <glm/gtc/quaternion.hpp>

#define KBF_BONE_MANAGER_LOG_TAG "[BoneManager]"

namespace kbf {

	BoneManager::BoneManager(
		KBFDataManager& dataManager,
		ArmourInfo armour, 
		REApi::ManagedObject* baseTransform,
		REApi::ManagedObject* helmTransform,
		REApi::ManagedObject* bodyTransform,
		REApi::ManagedObject* armsTransform,
		REApi::ManagedObject* coilTransform,
		REApi::ManagedObject* legsTransform,
		bool female
	) : dataManager{ dataManager }, 
		armourInfo{ armour }, 
		female{ female }
	{
		// This is a LUT, so make sure these are in the correct order.
		partTransforms = {
			baseTransform,
			armsTransform,
			bodyTransform,
			helmTransform,
			legsTransform,
			coilTransform,
		};

		initialized = loadBones();
	}

	BoneManager::BoneApplyStatusFlag BoneManager::applyPreset(const Preset* preset, ArmourPiece piece) {
		if (preset == nullptr) return BoneApplyStatusFlag::BONE_APPLY_ERROR_NULL_PRESET;

		const BoneModifierMap& pieceModifiers = preset->getPieceSettings(piece).modifiers;
		const std::unordered_map<std::string, REApi::ManagedObject*>& targetBones = partBones[piece];
		
		for (const auto& [boneName, modifier] : pieceModifiers) {
			auto it = targetBones.find(boneName);
			if (it != targetBones.end()) {
				bool applySuccess = modifyBone(it->second, modifier);
				if (!applySuccess) {
					return BoneApplyStatusFlag::BONE_APPLY_ERROR_INVALID_BONE;
				}
			}
		}

		return BoneApplyStatusFlag::BONE_APPLY_SUCCESS;
	}

	// UPDATE NOTE: This func is reverse engineered from get_LocalXXX ASM. It will need updating frequently.
	template<int64_t Offset>
	inline uintptr_t getJointTransformPtr(REApi::ManagedObject*& bone) {
		uint64_t rax = *(uint64_t*)(bone + 0x10); //qword ptr [r8 + 0x10]
		if (rax == 0) return 0;

		int64_t rcx = (int64_t)*(int32_t*)(bone + 0x18);
		rax = *(uint64_t*)(rax + Offset);
		rcx = rcx << 0x04;

		return rax + rcx;
	}

	bool BoneManager::modifyBone(REApi::ManagedObject* bone, const BoneModifier& modifier) {
		if (bone == nullptr) return false;
		
		if (modifier.hasScale()) {
			// Direct read for best performance
			glm::vec3* scalePtr = (glm::vec3*)getJointTransformPtr<0x38>(bone);
			if (scalePtr != nullptr) *scalePtr = *scalePtr + modifier.scale;
		}

		if (modifier.hasPosition()) {
			glm::vec3* posPtr = (glm::vec3*)getJointTransformPtr<0x18>(bone);
			if (posPtr != nullptr) *posPtr = *posPtr + modifier.position;
		}

		if (modifier.hasRotation()) {
			glm::fquat* rotPtr = (glm::fquat*)getJointTransformPtr<0x28>(bone);
			if (rotPtr != nullptr) *rotPtr *= modifier.getQuaternionRotation();
		}

		return true;
	}

	bool BoneManager::loadBones() {
		bool hasBase = loadTransformBones(ArmourPiece::AP_SET,  partTransforms[ArmourPiece::AP_SET],  partBones[ArmourPiece::AP_SET] );
		bool hasHelm = loadTransformBones(ArmourPiece::AP_HELM, partTransforms[ArmourPiece::AP_HELM], partBones[ArmourPiece::AP_HELM]);
		bool hasBody = loadTransformBones(ArmourPiece::AP_BODY, partTransforms[ArmourPiece::AP_BODY], partBones[ArmourPiece::AP_BODY]);
		bool hasArms = loadTransformBones(ArmourPiece::AP_ARMS, partTransforms[ArmourPiece::AP_ARMS], partBones[ArmourPiece::AP_ARMS]);
		bool hasCoil = loadTransformBones(ArmourPiece::AP_COIL, partTransforms[ArmourPiece::AP_COIL], partBones[ArmourPiece::AP_COIL]);
		bool hasLegs = loadTransformBones(ArmourPiece::AP_LEGS, partTransforms[ArmourPiece::AP_LEGS], partBones[ArmourPiece::AP_LEGS]);
		
		return partBones[ArmourPiece::AP_BODY].size() > 0; // all characteres must have at least body bones
	}

	bool BoneManager::loadTransformBones(
		ArmourPiece piece,
		REApi::ManagedObject* transform,
		std::unordered_map<std::string, REApi::ManagedObject*>& outMap
	) {
		if (transform == nullptr) return false;

		REApi::ManagedObject* joints = REInvokePtr<REApi::ManagedObject>(transform, "get_Joints", {});
		outMap = getBoneNames(joints);

		// Cache bones
		if (outMap.size() > 0) {
			std::vector<std::string> keys;
			keys.reserve(outMap.size()); // optional, avoids reallocations
			for (const auto& pair : outMap) keys.push_back(pair.first);


			if (piece != ArmourPiece::AP_SET) {
				// Don't cache base bones as not tied to a specific armour set
				ArmourSetWithCharacterSex armourWithSex{ armourInfo.getPiece(piece).value(), female};
				dataManager.boneCacheManager().cache(armourWithSex, keys, piece);
			}
		}

		return outMap.size() > 0;
	}

	std::unordered_map<std::string, REApi::ManagedObject*> BoneManager::getBoneNames(REApi::ManagedObject* jointArr) const {
		int arrSize = REInvoke<int>(jointArr, "GetLength(System.Int32)", { (void*)0 }, InvokeReturnType::DWORD);

		std::unordered_map<std::string, REApi::ManagedObject*> bones;
		for (size_t i = 0; i < arrSize; i++) {
			REApi::ManagedObject* joint = REInvokePtr<REApi::ManagedObject>(jointArr, "get_Item(System.Int32)", { (void*)i });
			if (joint) {
				std::string jointName = REInvokeStr(joint, "get_Name", {});
				bool isValid = REInvoke<bool>(joint, "get_Valid", {}, InvokeReturnType::BOOL);
				if (isValid) bones.emplace(jointName, joint);
			}
		}
		return bones;
	}

	void BoneManager::DEBUG_printBoneList(REApi::ManagedObject* jointArr, std::string message) const {
		int arrSize = REInvoke<int>(jointArr, "GetLength(System.Int32)", { (void*)0 }, InvokeReturnType::DWORD);

		for (size_t i = 0; i < arrSize; i++) {
			REApi::ManagedObject* joint = REInvokePtr<REApi::ManagedObject>(jointArr, "get_Item(System.Int32)", { (void*)i });
			if (joint) {
				std::string jointName = REInvokeStr(joint, "get_Name", {});
				bool isValid = REInvoke<bool>(joint, "get_Valid", {}, InvokeReturnType::BOOL);
				DEBUG_STACK.push(std::format("{} [{}] {} {} ({}) [{}]", KBF_BONE_MANAGER_LOG_TAG, i, message, jointName, ptrToHexString(joint), isValid ? "VALID" : "INVALID"), DebugStack::Color::COL_DEBUG);
			}
		}
	}

}