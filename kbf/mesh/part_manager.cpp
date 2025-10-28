#include <kbf/mesh/part_manager.hpp>

#include <kbf/data/armour/armour_list.hpp>
#include <kbf/util/re_engine/reinvoke.hpp>
#include <kbf/debug/debug_stack.hpp>
#include <kbf/util/re_engine/re_object_properties_to_string.hpp>
#include <kbf/util/string/ptr_to_hex_string.hpp>
#include <kbf/util/string/byte_to_binary_string.hpp>
#include <kbf/util/re_engine/find_transform.hpp>

#include <kbf/data/mesh/part_cache_manager.hpp>

#define KBF_BONE_MANAGER_LOG_TAG "[PartManager]"

namespace kbf {

	PartManager::PartManager(
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
		baseTransform{ baseTransform },
		helmTransform{ helmTransform },
		bodyTransform{ bodyTransform },
		armsTransform{ armsTransform },
		coilTransform{ coilTransform },
		legsTransform{ legsTransform },
		female{ female }
	{
		initialized = loadParts();
	}

	bool PartManager::applyPreset(const Preset* preset, ArmourPiece piece) {
		if (preset == nullptr) return false;

		const std::set<MeshPart> removedParts = preset->getPieceSettings(piece).removedParts;
		// TODO: Should probably check that the parts being removed actually exist in the mesh.

		REApi::ManagedObject* mesh = nullptr;
		switch (piece) {
		case ArmourPiece::AP_SET:  mesh = baseMesh; break;
		case ArmourPiece::AP_HELM: mesh = helmMesh; break;
		case ArmourPiece::AP_BODY: mesh = bodyMesh; break;
		case ArmourPiece::AP_ARMS: mesh = armsMesh; break;
		case ArmourPiece::AP_COIL: mesh = coilMesh; break;
		case ArmourPiece::AP_LEGS: mesh = legsMesh; break;
		}
		if (mesh == nullptr) return false;

		const std::vector<MeshPart>* targetParts = nullptr;
		switch (piece) {
		case ArmourPiece::AP_SET:  targetParts = &baseParts; break;
		case ArmourPiece::AP_HELM: targetParts = &helmParts; break;
		case ArmourPiece::AP_BODY: targetParts = &bodyParts; break;	
		case ArmourPiece::AP_ARMS: targetParts = &armsParts; break;
		case ArmourPiece::AP_COIL: targetParts = &coilParts; break;
		case ArmourPiece::AP_LEGS: targetParts = &legsParts; break;
		}
		if (targetParts == nullptr) return false;

		for (const MeshPart& part : *targetParts) {
			bool enable = removedParts.find(part) == removedParts.end();
			if (part.type == MeshPartType::PART_GROUP) {
				REInvokeVoid(mesh, "setPartsEnable(System.UInt64, System.Boolean)", { (void*)part.index, (void*)enable });
			}
			else if (part.type == MeshPartType::MATERIAL) {
				REInvokeVoid(mesh, "setMaterialsEnable(System.UInt64, System.Boolean)", { (void*)part.index, (void*)enable });
			}
		}

		return true;
	}

	bool PartManager::loadParts() {
		bool hasBaseMesh = getMesh(baseTransform, &baseMesh);
		bool hasHelmMesh = getMesh(helmTransform, &helmMesh);
		bool hasBodyMesh = getMesh(bodyTransform, &bodyMesh);
		bool hasArmsMesh = getMesh(armsTransform, &armsMesh);
		bool hasCoilMesh = getMesh(coilTransform, &coilMesh);
		bool hasLegsMesh = getMesh(legsTransform, &legsMesh);

		if (hasBaseMesh) {
			getParts(baseMesh, baseParts);
			ArmourSetWithCharacterSex armourWithSex{ ArmourList::DefaultArmourSet(), female};
			dataManager.partCache().cacheParts(armourWithSex, baseParts, ArmourPiece::AP_SET);
		}
		if (hasHelmMesh) {
			getParts(helmMesh, helmParts);
			ArmourSetWithCharacterSex armourWithSex{ armourInfo.helm.value(), female };
			dataManager.partCache().cacheParts(armourWithSex, helmParts, ArmourPiece::AP_HELM);
		}
		if (hasBodyMesh) {
			getParts(bodyMesh, bodyParts);
			ArmourSetWithCharacterSex armourWithSex{ armourInfo.body.value(), female };
			dataManager.partCache().cacheParts(armourWithSex, bodyParts, ArmourPiece::AP_BODY);
		}
		if (hasArmsMesh) {
			getParts(armsMesh, armsParts);
			ArmourSetWithCharacterSex armourWithSex{ armourInfo.arms.value(), female };
			dataManager.partCache().cacheParts(armourWithSex, armsParts, ArmourPiece::AP_ARMS);
		}
		if (hasCoilMesh) {
			getParts(coilMesh, coilParts);
			ArmourSetWithCharacterSex armourWithSex{ armourInfo.coil.value(), female };
			dataManager.partCache().cacheParts(armourWithSex, coilParts, ArmourPiece::AP_COIL);
		}
		if (hasLegsMesh) {
			getParts(legsMesh, legsParts);
			ArmourSetWithCharacterSex armourWithSex{ armourInfo.legs.value(), female };
			dataManager.partCache().cacheParts(armourWithSex, legsParts, ArmourPiece::AP_LEGS);
		}

		return bodyParts.size() > 0;
	}

	bool PartManager::getMesh(REApi::ManagedObject* transform, REApi::ManagedObject** outMesh) const {
		if (transform == nullptr || outMesh == nullptr) return false;

		// Get GameObject first
		REApi::ManagedObject* GameObject = REInvokePtr<REApi::ManagedObject>(transform, "get_GameObject", {});
		if (GameObject == nullptr) return false;

		static REApi::ManagedObject* meshType = REApi::get()->typeof("via.render.Mesh");
		*outMesh = REInvokePtr<REApi::ManagedObject>(GameObject, "getComponent(System.Type)", { (void*)meshType });

		return *outMesh != nullptr;
	}

	void PartManager::getParts(REApi::ManagedObject* mesh, std::vector<MeshPart>& out) const {
		out.clear();
		// The finesty granularity for parts appears to be somewhere between submesh and material level.
		//  The 'parts' at run-time do not seem to correspond with submeshes within the .mesh file nor is there a 1-to-1 mapping with materials.
		//  Consequently, just expose both the materials and parts here for the user to toggle. 

		uint64_t partIndicesCount = REInvoke<uint64_t>(mesh, "getPartsEnableIndicesCount", {}, InvokeReturnType::QWORD);

		for (uint64_t i = 0 ; i < partIndicesCount; i++) {
			uint8_t indices = REInvoke<uint8_t>(mesh, "getPartsEnableIndices(System.UInt64)", { (void*)i }, InvokeReturnType::BYTE);
		
			uint64_t indices_u64 = static_cast<uint64_t>(indices);

			out.emplace_back(MeshPart{
				.name = std::format("Part Group {}", i),
				.index = indices_u64,
				.type = MeshPartType::PART_GROUP
			});

			//REInvokeVoid(mesh, "setPartsEnable(System.UInt64, System.Boolean)", { (void*)indices_u64, (void*)true });
		}

		uint32_t materialCount = REInvoke<uint32_t>(mesh, "get_MaterialNum", {}, InvokeReturnType::DWORD);

		for (uint32_t i = 0 ; i < materialCount; i++) {
			std::string materialName = REInvokeStr(mesh, "getMaterialName(System.UInt32)", { (void*)i });

			out.emplace_back(MeshPart{
				.name = std::format("Material \"{}\"", materialName),
				.index = static_cast<uint64_t>(i),
				.type = MeshPartType::MATERIAL
			});

			//REInvokeVoid(mesh, "setMaterialsEnable(System.UInt64, System.Boolean)", { (void*)(static_cast<uint64_t>(i)), (void*)false });
		}
	}

	//std::set<std::string> BoneManager::getBoneNames(REApi::ManagedObject* jointArr) const {
	//	int arrSize = REInvoke<int>(jointArr, "GetLength(System.Int32)", { (void*)0 }, InvokeReturnType::DWORD);

	//	std::set<std::string> boneNames;
	//	for (size_t i = 0; i < arrSize; i++) {
	//		REApi::ManagedObject* joint = REInvokePtr<REApi::ManagedObject>(jointArr, "get_Item(System.Int32)", { (void*)i });
	//		if (joint) {
	//			std::string jointName = REInvokeStr(joint, "get_Name", {});
	//			bool isValid = REInvoke<bool>(joint, "get_Valid", {}, InvokeReturnType::BOOL);
	//			if (isValid) boneNames.insert(jointName);
	//		}
	//	}
	//	return boneNames;
	//}

	//void BoneManager::DEBUG_printBoneList(REApi::ManagedObject* jointArr, std::string message) const {
	//	int arrSize = REInvoke<int>(jointArr, "GetLength(System.Int32)", { (void*)0 }, InvokeReturnType::DWORD);

	//	for (size_t i = 0; i < arrSize; i++) {
	//		REApi::ManagedObject* joint = REInvokePtr<REApi::ManagedObject>(jointArr, "get_Item(System.Int32)", { (void*)i });
	//		if (joint) {
	//			std::string jointName = REInvokeStr(joint, "get_Name", {});
	//			bool isValid = REInvoke<bool>(joint, "get_Valid", {}, InvokeReturnType::BOOL);
	//			DEBUG_STACK.push(std::format("{} [{}] {} {} ({}) [{}]", KBF_BONE_MANAGER_LOG_TAG, i, message, jointName, ptrToHexString(joint), isValid ? "VALID" : "INVALID"), DebugStack::Color::DEBUG);
	//		}
	//	}
	//}

}