#pragma once

#include <kbf/util/hash/hash_combine.hpp>

#include <string>
#include <cstdint>
#include <unordered_map>

namespace kbf {

	enum MeshMaterialParamType {
		MAT_TYPE_FLOAT  = 1,
		MAT_TYPE_FLOAT4 = 4,
	};

	struct MeshMaterialParam {
		std::string name;
		MeshMaterialParamType type;
        uint64_t index = 0;
        
        bool operator==(const MeshMaterialParam& other) const {
            return name == other.name && type == other.type && index == other.index;
		}

        bool operator<(const MeshMaterialParam& other) const {
            if (name != other.name) return name < other.name;
            if (index != other.index) return index < other.index;
            return type < other.type;
		}
	};

	struct MeshMaterial {
		std::string name;
		uint64_t index;
		std::unordered_map<std::string, MeshMaterialParam> params;

		bool operator==(const MeshMaterial& other) const {
            return name == other.name; //&& index == other.index && params == other.params;
		}

		bool operator<(const MeshMaterial& other) const {
            return name < other.name;
			//if (name != other.name) return name < other.name;
			//return index < other.index;
		}
        
	};

	// ---- Hash Funcs -----------------------------------------------------
    struct MeshMaterialParamHash {
        std::size_t operator()(const MeshMaterialParam& param) const noexcept {
            std::size_t h = 0;
            std::hash<std::string> hashString;
            std::hash<int> hashInt;

            hashCombine(h, hashString(param.name));
            hashCombine(h, hashInt(static_cast<int>(param.type)));

            return h;
        }
    };

    struct MeshMaterialHash {
        std::size_t operator()(const MeshMaterial& mat) const noexcept {
            std::size_t h = 0;

            std::hash<std::string> hashString;
            std::hash<uint64_t> hashUInt64;

            hashCombine(h, hashString(mat.name));
            hashCombine(h, hashUInt64(mat.index));

            // Combine hashes for params (unordered_map)
            MeshMaterialParamHash paramHasher;
            for (const auto& [_, value] : mat.params) {
                std::size_t paramHash = 0;
                hashCombine(paramHash, hashUInt64(value.index));
                hashCombine(paramHash, paramHasher(value));
                hashCombine(h, paramHash);
            }

            return h;
        }
    };

}