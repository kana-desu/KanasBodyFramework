#include <kbf/mesh/material_manager.hpp>

#include <kbf/data/armour/armour_list.hpp>
#include <kbf/util/re_engine/reinvoke.hpp>
#include <kbf/debug/debug_stack.hpp>
#include <kbf/util/re_engine/check_re_ptr_validity.hpp>
#include <kbf/util/re_engine/re_object_properties_to_string.hpp>
#include <kbf/util/string/ptr_to_hex_string.hpp>
#include <kbf/util/string/byte_to_binary_string.hpp>
#include <kbf/util/re_engine/find_transform.hpp>

#include <kbf/profiling/cpu_profiler.hpp>

#include <kbf/util/string/to_lower.hpp>
#include <kbf/util/hash/pair_hash.hpp>

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

		BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Material Apply - Fetch Piece Info");
		const std::set<OverrideMaterial> matOverrides = preset->getPieceSettings(piece).materialOverrides;
		// TODO: I Hate literally all of this OverrideMaterial code, but i cba to refactor it
		// Do a shitty map based on name so subsequent searches are faster - this is a big performance bottleneck
		const std::unordered_map<std::pair<size_t, std::string>, const OverrideMaterial*, PairHash> matOverridesLUT = [&matOverrides]() {
			std::unordered_map<std::pair<size_t, std::string>, const OverrideMaterial*, PairHash> lut;
			for (const auto& mat : matOverrides) {
				lut[{ mat.material.index, mat.material.name }] = &mat;
			}
			return lut;
		}();

		REApi::ManagedObject* mesh = nullptr;
		switch (piece) {
		case ArmourPiece::AP_HELM: mesh = helmMesh; break;
		case ArmourPiece::AP_BODY: mesh = bodyMesh; break;
		case ArmourPiece::AP_ARMS: mesh = armsMesh; break;
		case ArmourPiece::AP_COIL: mesh = coilMesh; break;
		case ArmourPiece::AP_LEGS: mesh = legsMesh; break;
		}
		END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Material Apply - Fetch Piece Info");
		if (mesh == nullptr) return false;
		
		BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Material Apply - Check Mesh Validity");
		// Check the mesh is valid before trying to apply anything
		static const reframework::API::TypeDefinition* def_renderMesh = reframework::API::get()->tdb()->find_type("via.render.Mesh");
		if (!checkREPtrValidity(mesh, def_renderMesh)) return false;
		END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Material Apply - Check Mesh Validity");

		BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Material Apply - Fetch Material Info");
		std::unordered_map<std::string, MeshMaterial>* targetMaterials = nullptr;
		switch (piece) {
		case ArmourPiece::AP_HELM: targetMaterials = &helmMaterials; break;
		case ArmourPiece::AP_BODY: targetMaterials = &bodyMaterials; break;
		case ArmourPiece::AP_ARMS: targetMaterials = &armsMaterials; break;
		case ArmourPiece::AP_COIL: targetMaterials = &coilMaterials; break;
		case ArmourPiece::AP_LEGS: targetMaterials = &legsMaterials; break;
		}
		END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Material Apply - Fetch Material Info");
		if (targetMaterials == nullptr) return false;

		BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Material Apply - Process Overrides");
		// Only mask out visible items
		for (const auto& [_, mat] : *targetMaterials) {
			BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Material Apply - Process Overrides - Mat Search");
			const auto it = matOverridesLUT.find({ mat.index, mat.name });
			END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Material Apply - Process Overrides - Mat Search");
			if (it == matOverridesLUT.end()) continue;

			bool vis = it->second->shown;
			
			BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Material Apply - Process Overrides - Set Visibility");
			REInvokeVoid(mesh, "setMaterialsEnable(System.UInt64, System.Boolean)", { (void*)mat.index, (void*)vis });
			END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Material Apply - Process Overrides - Set Visibility");

			if (!vis) continue;

			BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Material Apply - Process Overrides - Apply Params");
			// Apply params.
			//  All my bad design choices coming to bite me in the ass here :)
			for (const auto& [name, value] : it->second->paramOverrides) {
				const auto paramIt = mat.params.find(name);
				if (paramIt != mat.params.end()) {
					uint32_t matIndex   = static_cast<uint32_t>(mat.index);
					uint32_t paramIndex = static_cast<uint32_t>(paramIt->second.index);
					
					switch (value.type) {
					case MeshMaterialParamType::MAT_TYPE_FLOAT: {
						BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Material Apply - Process Overrides - Apply Params - Set Float");
						// For whatever dumbass reason, System.Single is actually a double?????????????????????????????????????????????
						double v = static_cast<double>(value.asFloat());
						uint64_t vAsUint = *reinterpret_cast<uint64_t*>(&v);

						REInvokeVoid(mesh, "setMaterialFloat(System.UInt32, System.UInt32, System.Single)", { (void*)matIndex, (void*)paramIndex, (void*)vAsUint });
						END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Material Apply - Process Overrides - Apply Params - Set Float");
					} break;
					case MeshMaterialParamType::MAT_TYPE_FLOAT4: {
						BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Material Apply - Process Overrides - Apply Params - Set Float4");
						glm::vec4 v = value.asVec4();
						REInvokeVoid(mesh, "setMaterialFloat4(System.UInt32, System.UInt32, via.Float4)", { (void*)matIndex, (void*)paramIndex, (void*)&v });
						END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Material Apply - Process Overrides - Apply Params - Set Float4");
					} break;
					}

				}
			}
			END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Material Apply - Process Overrides - Apply Params");
		}
		END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Material Apply - Process Overrides");

		QuickOverrideMatMatchLUT* targetOverrideMatches = nullptr;
		switch (piece) {
		case ArmourPiece::AP_HELM: targetOverrideMatches = &helmQuickOverrideMatches; break;
		case ArmourPiece::AP_BODY: targetOverrideMatches = &bodyQuickOverrideMatches; break;
		case ArmourPiece::AP_ARMS: targetOverrideMatches = &armsQuickOverrideMatches; break;
		case ArmourPiece::AP_COIL: targetOverrideMatches = &coilQuickOverrideMatches; break;
		case ArmourPiece::AP_LEGS: targetOverrideMatches = &legsQuickOverrideMatches; break;
		}
		if (targetOverrideMatches == nullptr) return false;

		BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Material Apply - Quick Overrides");
		// Apply quick overrides.
		const QuickOverrideMatMatchLUT& matches = *targetOverrideMatches;
		applyQuickOverrides(preset, mesh, matches);
		END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler, "Material Apply - Quick Overrides");

		return true;
	}

	void MaterialManager::applyQuickOverrides(
		const Preset* preset, 
		REApi::ManagedObject* mesh, 
		const QuickOverrideMatMatchLUT& matches
	) {
		// Note: Template this func if any more types get added.

		for (const auto& [paramKey, qOverride] : preset->quickMaterialOverridesFloat) {
			if (!qOverride.enabled) continue;

			// Check this mesh has matching material (cached)
			auto matIt = matches.find(qOverride.materialName);
			if (matIt == matches.end()) continue;
			for (const MeshMaterial* foundMat : matIt->second) {
				// Check matching material has param we want to edit
				const auto& matParams = foundMat->params;
				auto paramIt = matParams.find(qOverride.paramName);
				if (paramIt == matParams.end()) continue;
				const MeshMaterialParam& foundParam = paramIt->second;

				double v = static_cast<double>(qOverride.value);
				uint64_t vAsUint = *reinterpret_cast<uint64_t*>(&v);

				uint32_t matIdx32   = static_cast<uint32_t>(foundMat->index);
				uint32_t paramIdx32 = static_cast<uint32_t>(foundParam.index);
				REInvokeVoid(mesh, "setMaterialFloat(System.UInt32, System.UInt32, System.Single)", { (void*)foundMat->index, (void*)foundParam.index, (void*)vAsUint });
			}
		}

		for (const auto& [paramKey, qOverride] : preset->quickMaterialOverridesVec4) {
			if (!qOverride.enabled) continue;

			// Check this mesh has matching material (cached)
			auto matIt = matches.find(qOverride.materialName);
			if (matIt == matches.end()) continue;

			for (const MeshMaterial* foundMat : matIt->second) {
				// Check matching material has param we want to edit
				const auto& matParams = foundMat->params;
				auto paramIt = matParams.find(qOverride.paramName);
				if (paramIt == matParams.end()) continue;
				const MeshMaterialParam& foundParam = paramIt->second;

				glm::vec4 v = qOverride.value;
				REInvokeVoid(mesh, "setMaterialFloat4(System.UInt32, System.UInt32, via.Float4)", { (void*)foundMat->index, (void*)foundParam.index, (void*)&v });
			}

		}
	}

	bool MaterialManager::loadMaterials() {
		bool hasHelmMesh = getMesh(helmTransform, &helmMesh);
		bool hasBodyMesh = getMesh(bodyTransform, &bodyMesh);
		bool hasArmsMesh = getMesh(armsTransform, &armsMesh);
		bool hasCoilMesh = getMesh(coilTransform, &coilMesh);
		bool hasLegsMesh = getMesh(legsTransform, &legsMesh);

		if (hasHelmMesh) {
			helmMaterials = getMaterials(helmMesh, &helmQuickOverrideMatches);
			ArmourSetWithCharacterSex armourWithSex{ armourInfo.helm.value(), female };
			dataManager.materialCacheManager().cache(armourWithSex, helmMaterials, ArmourPiece::AP_HELM);
		}
		if (hasBodyMesh) {
			bodyMaterials = getMaterials(bodyMesh, &bodyQuickOverrideMatches);
			ArmourSetWithCharacterSex armourWithSex{ armourInfo.body.value(), female };
			dataManager.materialCacheManager().cache(armourWithSex, bodyMaterials, ArmourPiece::AP_BODY);
		}
		if (hasArmsMesh) {
			armsMaterials = getMaterials(armsMesh, &armsQuickOverrideMatches);
			ArmourSetWithCharacterSex armourWithSex{ armourInfo.arms.value(), female };
			dataManager.materialCacheManager().cache(armourWithSex, armsMaterials, ArmourPiece::AP_ARMS);
		}
		if (hasCoilMesh) {
			coilMaterials = getMaterials(coilMesh, &coilQuickOverrideMatches);
			ArmourSetWithCharacterSex armourWithSex{ armourInfo.coil.value(), female };
			dataManager.materialCacheManager().cache(armourWithSex, coilMaterials, ArmourPiece::AP_COIL);
		}
		if (hasLegsMesh) {
			legsMaterials = getMaterials(legsMesh, &legsQuickOverrideMatches);
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

	std::unordered_map<std::string, MeshMaterial> MaterialManager::getMaterials(
		REApi::ManagedObject* mesh, 
		QuickOverrideMatMatchLUT* quickOverrideMatchesOut
	) const {

		std::unordered_map<std::string, MeshMaterial> out{};
		
		uint32_t materialCount = REInvoke<uint32_t>(mesh, "get_MaterialNum", {}, InvokeReturnType::DWORD);

		for (uint32_t i = 0 ; i < materialCount; i++) {
			std::string materialName = REInvokeStr(mesh, "getMaterialName(System.UInt32)", { (void*)i });
			std::uint32_t matVariableCount = REInvoke<std::uint32_t>(mesh, "getMaterialVariableNum(System.UInt32)", { (void*)i }, InvokeReturnType::DWORD);

			std::unordered_map<std::string, MeshMaterialParam> params = {};
			for (uint32_t paramIdx = 0; paramIdx < matVariableCount; paramIdx++) {
				MeshMaterialParam param{};
				uint32_t rawType = REInvoke<uint32_t>(mesh, "getMaterialVariableType(System.UInt32, System.UInt32)", { (void*)i, (void*)paramIdx }, InvokeReturnType::DWORD);
				param.name = REInvokeStr(mesh, "getMaterialVariableName(System.UInt32, System.UInt32)", { (void*)i, (void*)paramIdx });
				param.type = static_cast<MeshMaterialParamType>(rawType);
				param.index = paramIdx;

				//DEBUG_STACK.push(std::format("Material [({})]: {} | Param [{}]: Name={}, Type={}", i, materialName, paramIdx, param.name, static_cast<uint32_t>(param.type)));
				
				params.emplace(param.name, param);
			}

			MeshMaterial mat{};
			mat.index  = i;
			mat.name   = materialName;
			mat.params = std::move(params);

			// we make the assumption that all match strings should be lower case to save computational resources

			out.emplace(materialName, std::move(mat));

			// Accumulate values into the out LUT
			if (quickOverrideMatchesOut) {
				auto&& result = getQuickOverrideMatches(out.at(materialName));

				for (auto& [key, vec] : result) {
					// Append to existing vector in the output map
					auto& targetVec = (*quickOverrideMatchesOut)[key];
					targetVec.insert(targetVec.end(), vec.begin(), vec.end());
				}
			}
		}

		return out;
	}

	MaterialManager::QuickOverrideMatMatchLUT MaterialManager::getQuickOverrideMatches(
		const MeshMaterial& mat
	) const {
		std::string lowercaseName = toLower(mat.name);

		// TODO: Might be more than one match??
		QuickOverrideMatMatchLUT matches;
		for (const std::string& name : QuickOverrideKeys::ALL_MATERIALS) {
			if (lowercaseName.find(name) != lowercaseName.npos) {
				matches[name].push_back(&mat);
			}
		}

		return matches;
	}

}