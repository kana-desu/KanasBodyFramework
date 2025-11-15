#include <kbf/mesh/material_manager.hpp>

#include <kbf/data/armour/armour_list.hpp>
#include <kbf/util/re_engine/reinvoke.hpp>
#include <kbf/debug/debug_stack.hpp>
#include <kbf/util/re_engine/re_object_properties_to_string.hpp>
#include <kbf/util/string/ptr_to_hex_string.hpp>
#include <kbf/util/string/byte_to_binary_string.hpp>
#include <kbf/util/re_engine/find_transform.hpp>

#include <kbf/data/mesh/parts/part_cache_manager.hpp>

#define KBF_BONE_MANAGER_LOG_TAG "[PartManager]"

namespace kbf {

	MaterialManager::MaterialManager(
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
		initialized = loadMaterials();
	}

	bool MaterialManager::applyPreset(const Preset* preset, ArmourPiece piece) {
		if (preset == nullptr) return false;
		if (piece == ArmourPiece::AP_SET) return true; // SET does not have materials to modify

		const std::set<OverrideMaterial> matOverrides = preset->getPieceSettings(piece).materialOverrides;
		// TODO: Should probably check that the parts being removed actually exist in the mesh.

		REApi::ManagedObject* mesh = nullptr;
		switch (piece) {
		case ArmourPiece::AP_HELM: mesh = helmMesh; break;
		case ArmourPiece::AP_BODY: mesh = bodyMesh; break;
		case ArmourPiece::AP_ARMS: mesh = armsMesh; break;
		case ArmourPiece::AP_COIL: mesh = coilMesh; break;
		case ArmourPiece::AP_LEGS: mesh = legsMesh; break;
		}
		if (mesh == nullptr) return false;

		const std::vector<MeshMaterial>* targetMaterials = nullptr;
		switch (piece) {
		case ArmourPiece::AP_HELM: targetMaterials = &helmMaterials; break;
		case ArmourPiece::AP_BODY: targetMaterials = &bodyMaterials; break;
		case ArmourPiece::AP_ARMS: targetMaterials = &armsMaterials; break;
		case ArmourPiece::AP_COIL: targetMaterials = &coilMaterials; break;
		case ArmourPiece::AP_LEGS: targetMaterials = &legsMaterials; break;
		}
		if (targetMaterials == nullptr) return false;

		// Only mask out visible items
		for (const MeshMaterial& mat : *targetMaterials) {
			if (matOverrides.find(mat) == matOverrides.end()) continue;

			bool vis = matOverrides.find(mat)->shown;

			REInvokeVoid(mesh, "setMaterialsEnable(System.UInt64, System.Boolean)", { (void*)mat.index, (void*)vis });
		}

		return true;
	}

	bool MaterialManager::loadMaterials() {
		bool hasBaseMesh = getMesh(baseTransform, &baseMesh);
		bool hasHelmMesh = getMesh(helmTransform, &helmMesh);
		bool hasBodyMesh = getMesh(bodyTransform, &bodyMesh);
		bool hasArmsMesh = getMesh(armsTransform, &armsMesh);
		bool hasCoilMesh = getMesh(coilTransform, &coilMesh);
		bool hasLegsMesh = getMesh(legsTransform, &legsMesh);

		if (hasHelmMesh) {
			getMaterials(helmMesh, helmMaterials);
			ArmourSetWithCharacterSex armourWithSex{ armourInfo.helm.value(), female };
			dataManager.materialCacheManager().cache(armourWithSex, helmMaterials, ArmourPiece::AP_HELM);
		}
		if (hasBodyMesh) {
			getMaterials(bodyMesh, bodyMaterials);
			ArmourSetWithCharacterSex armourWithSex{ armourInfo.body.value(), female };
			dataManager.materialCacheManager().cache(armourWithSex, bodyMaterials, ArmourPiece::AP_BODY);
		}
		if (hasArmsMesh) {
			getMaterials(armsMesh, armsMaterials);
			ArmourSetWithCharacterSex armourWithSex{ armourInfo.arms.value(), female };
			dataManager.materialCacheManager().cache(armourWithSex, armsMaterials, ArmourPiece::AP_ARMS);
		}
		if (hasCoilMesh) {
			getMaterials(coilMesh, coilMaterials);
			ArmourSetWithCharacterSex armourWithSex{ armourInfo.coil.value(), female };
			dataManager.materialCacheManager().cache(armourWithSex, coilMaterials, ArmourPiece::AP_COIL);
		}
		if (hasLegsMesh) {
			getMaterials(legsMesh, legsMaterials);
			ArmourSetWithCharacterSex armourWithSex{ armourInfo.legs.value(), female };
			dataManager.materialCacheManager().cache(armourWithSex, legsMaterials, ArmourPiece::AP_LEGS);
		}

		return bodyMaterials.size() > 0;
	}

	bool MaterialManager::getMesh(REApi::ManagedObject* transform, REApi::ManagedObject** outMesh) const {
		if (transform == nullptr || outMesh == nullptr) return false;

		// Get GameObject first
		REApi::ManagedObject* GameObject = REInvokePtr<REApi::ManagedObject>(transform, "get_GameObject", {});
		if (GameObject == nullptr) return false;

		static REApi::ManagedObject* meshType = REApi::get()->typeof("via.render.Mesh");
		*outMesh = REInvokePtr<REApi::ManagedObject>(GameObject, "getComponent(System.Type)", { (void*)meshType });

		return *outMesh != nullptr;
	}

	void MaterialManager::getMaterials(REApi::ManagedObject* mesh, std::vector<MeshMaterial>& out) const {
		out.clear();
		
		uint32_t materialCount = REInvoke<uint32_t>(mesh, "get_MaterialNum", {}, InvokeReturnType::DWORD);

		for (uint32_t i = 0 ; i < materialCount; i++) {
			std::string materialName = REInvokeStr(mesh, "getMaterialName(System.UInt32)", { (void*)i });
			std::uint32_t matVariableCount = REInvoke<std::uint32_t>(mesh, "getMaterialVariableNum(System.UInt32)", { (void*)i }, InvokeReturnType::DWORD);

			std::unordered_map<uint32_t, MeshMaterialParam> params = {};
			for (uint32_t paramIdx = 0; paramIdx < matVariableCount; paramIdx++) {
				MeshMaterialParam param{};
				uint32_t rawType = REInvoke<uint32_t>(mesh, "getMaterialVariableType(System.UInt32, System.UInt32)", { (void*)i, (void*)paramIdx }, InvokeReturnType::DWORD);
				param.name = REInvokeStr(mesh, "getMaterialVariableName(System.UInt32, System.UInt32)", { (void*)i, (void*)paramIdx });
				param.type = static_cast<MeshMaterialParamType>(rawType);

				//DEBUG_STACK.push(std::format("Material {}: Param [{}]: Name={}, Type={}", materialName, paramIdx, param.name, static_cast<uint32_t>(param.type)));
				
				params.emplace(paramIdx, param);
			}

			MeshMaterial mat{};
			mat.index  = i;
			mat.name   = materialName;
			mat.params = std::move(params);

			out.emplace_back(std::move(mat));
		}
	}

}