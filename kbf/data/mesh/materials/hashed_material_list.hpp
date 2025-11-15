#pragma once

#include <kbf/data/mesh/materials/mesh_material.hpp>
#include <kbf/util/hash/hash_combine.hpp>

#include <vector>
#include <string>

namespace kbf {

	class HashedMaterialList {
	public:
		HashedMaterialList(std::vector<MeshMaterial> materials = {}) : materials{ materials }, hash{ hashMaterials(materials) } {}
		HashedMaterialList(std::vector<MeshMaterial> materials, size_t hash) : materials{ materials }, hash{ hash } {}

		size_t getHash() const { return hash; }
		const std::vector<MeshMaterial>& getMaterials() const { return materials; }

		static const size_t hashMaterials(const std::vector<MeshMaterial>& materials) {
			std::size_t h = 0;
			MeshMaterialHash meshHasher;

			for (const auto& mat : materials) {
				hashCombine(h, meshHasher(mat));
			}

			return h;
		}

	private:
		size_t hash;
		std::vector<MeshMaterial> materials;
	};

}