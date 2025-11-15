#pragma once

#include <kbf/data/mesh/materials/mesh_material.hpp>

#include <unordered_map>

#include <glm/glm.hpp>

namespace kbf {

	struct MaterialParamValue {
		MeshMaterialParamType type;
		union {
			float     floatValue;
			glm::vec4 vec4Value;
		} value;
	};

	struct OverrideMaterial {

		OverrideMaterial() = default;
		OverrideMaterial(MeshMaterial material, bool shown = true)
			: material{ material }, shown{ shown } {
		}

		bool operator==(const OverrideMaterial& other) const {
			return material == other.material && shown == other.shown;
		}

		bool operator==(const MeshMaterial& other) const {
			return material == other;
		}

		bool operator<(const OverrideMaterial& other) const {
			return material < other.material;
		}

		bool operator<(const MeshMaterial& other) const {
			return material < other;
		}

		MeshMaterial material;
		bool shown = true;

		void setParamOverride(const std::string& paramName, float value) {
			paramOverrides[paramName] = MaterialParamValue{ 
				MeshMaterialParamType::MAT_TYPE_FLOAT, 
				{ .floatValue = value  }
			};
		}

		void setParamOverride(const std::string& paramName, const glm::vec4& value) {
			paramOverrides[paramName] = MaterialParamValue{
				MeshMaterialParamType::MAT_TYPE_FLOAT4, 
				{ .vec4Value = value }
			};
		}

		void removeParamOverride(const std::string& paramName) {
			paramOverrides.erase(paramName);
		}

		std::unordered_map<std::string, MaterialParamValue> paramOverrides;
	};

}