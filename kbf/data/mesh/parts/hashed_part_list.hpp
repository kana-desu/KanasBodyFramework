#pragma once

#include <kbf/data/mesh/parts/mesh_part.hpp>
#include <kbf/util/hash/hash_combine.hpp>

#include <vector>
#include <string>

namespace kbf {

	class HashedPartList {
	public:
		HashedPartList(std::vector<MeshPart> parts = {}) : parts{ parts }, hash{ hashParts(parts) } {}
		HashedPartList(std::vector<MeshPart> parts, size_t hash) : parts{ parts }, hash{ hash } {}

		size_t getHash() const { return hash; }
		const std::vector<MeshPart>& getParts() const { return parts; }

		static const size_t hashParts(const std::vector<MeshPart>& parts) {
			std::size_t h = 0;
			MeshPartHash meshHasher;

			for (const auto& part : parts) {
				hashCombine(h, meshHasher(part));
			}

			return h;
		}

	private:
		size_t hash;
		std::vector<MeshPart> parts;
	};

}