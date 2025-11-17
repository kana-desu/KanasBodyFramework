#pragma once

#include <kbf/data/mesh/materials/mesh_material.hpp>

#include <unordered_map>

#include <glm/glm.hpp>

#include <variant>

namespace kbf {

	struct MaterialParamValue {
		using ValueVariant = std::variant<float, glm::vec4>;

		MeshMaterialParamType type;  // keep if you need it for UI/serialization
		ValueVariant value;

		MaterialParamValue() = default;

		MaterialParamValue(float v)
			: type(MeshMaterialParamType::MAT_TYPE_FLOAT), value(v) {
		}

		MaterialParamValue(const glm::vec4& v)
			: type(MeshMaterialParamType::MAT_TYPE_FLOAT4), value(v) {
		}

		bool operator==(const MaterialParamValue& other) const {
			return type == other.type && value == other.value;
		}

		// --- Getters (with reference return) ---
		float& asFloat() { return std::get<float>(value); }
		const float& asFloat() const { return std::get<float>(value); }

		glm::vec4& asVec4() { return std::get<glm::vec4>(value); }
		const glm::vec4& asVec4()  const { return std::get<glm::vec4>(value); }
	};


    struct OverrideMaterial {

        OverrideMaterial() = default;
        OverrideMaterial(MeshMaterial material, bool shown = true)
            : material(material), shown(shown) {
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

        // --- Setters (now construct variant safely) ---
        void setParamOverride(const std::string& paramName, float value) {
            paramOverrides[paramName] = MaterialParamValue(value);
        }

        void setParamOverride(const std::string& paramName, const glm::vec4& value) {
            paramOverrides[paramName] = MaterialParamValue(value);
        }

        void removeParamOverride(const std::string& paramName) {
            paramOverrides.erase(paramName);
        }

        // --- EXACT equality comparing parameter overrides ---
        bool isExactlyEqual(const OverrideMaterial& other) const {
            return material == other.material &&
                shown == other.shown &&
                paramOverrides == other.paramOverrides;
        }

        MeshMaterial material;
        bool shown = true;

        // map of overrides
        std::unordered_map<std::string, MaterialParamValue> paramOverrides;
    };

}