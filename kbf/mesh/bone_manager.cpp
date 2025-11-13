#include <kbf/mesh/bone_manager.hpp>

#include <kbf/data/armour/armour_list.hpp>
#include <kbf/util/re_engine/reinvoke.hpp>
#include <kbf/debug/debug_stack.hpp>
#include <kbf/util/string/ptr_to_hex_string.hpp>
#include <kbf/util/re_engine/check_re_ptr_validity.hpp>
#include <kbf/util/re_engine/find_transform.hpp>

#include <kbf/util/re_engine/write_re_memory.hpp>
#include <kbf/util/re_engine/read_re_memory.hpp>
#include <kbf/util/re_engine/re_memory_ptr.hpp>

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

	bool BoneManager::modifyBone(REApi::ManagedObject* bone, const BoneModifier& modifier) {
		if (bone == nullptr) return false;

		static reframework::API::TypeDefinition* def_ViaJoint = reframework::API::get()->tdb()->find_type("via.Joint");
		if (!checkREPtrValidity(bone, def_ViaJoint)) return false;

		if (modifier.hasScale()) {
			float* scaleX = re_memory_ptr<float>(bone, 0x40);
			float* scaleY = re_memory_ptr<float>(bone, 0x44);
			float* scaleZ = re_memory_ptr<float>(bone, 0x48);

			write_re_memory_no_check<float>(bone, 0x40, *scaleX + modifier.scale.x);
			write_re_memory_no_check<float>(bone, 0x44, *scaleY + modifier.scale.y);
			write_re_memory_no_check<float>(bone, 0x48, *scaleZ + modifier.scale.z);
		}

		if (modifier.hasPosition()) {
			float* posX = re_memory_ptr<float>(bone, 0x20);
			float* posY = re_memory_ptr<float>(bone, 0x24);
			float* posZ = re_memory_ptr<float>(bone, 0x28);

			write_re_memory_no_check<float>(bone, 0x20, *posX + modifier.position.x);
			write_re_memory_no_check<float>(bone, 0x24, *posY + modifier.position.y);
			write_re_memory_no_check<float>(bone, 0x28, *posZ + modifier.position.z);
		}

		if (modifier.hasRotation()) {
			glm::fquat rotation{
				read_re_memory_no_check<float>(bone, 0x3C),
				read_re_memory_no_check<float>(bone, 0x30),
				read_re_memory_no_check<float>(bone, 0x34),
				read_re_memory_no_check<float>(bone, 0x38)
			};

			rotation *= modifier.getQuaternionRotation();

			write_re_memory_no_check<float>(bone, 0x30, rotation.x);
			write_re_memory_no_check<float>(bone, 0x34, rotation.y);
			write_re_memory_no_check<float>(bone, 0x38, rotation.z);
			write_re_memory_no_check<float>(bone, 0x3C, rotation.w);
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