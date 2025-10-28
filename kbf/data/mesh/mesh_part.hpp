#pragma once

#include <kbf/util/hash/hash_combine.hpp>

#include <string>
#include <cstdint>

namespace kbf {

	enum class MeshPartType {
		PART_GROUP,
		MATERIAL
	};

	struct MeshPart {
		std::string name;
		uint64_t index;
		MeshPartType type;

		bool operator==(const MeshPart& other) const {
			return name == other.name && index == other.index && type == other.type;
		}

		bool operator<(const MeshPart& other) const {
			if (name != other.name) return name < other.name;
			if (index != other.index) return index < other.index;
			return type < other.type;
		}
	};

	// Hash for single MeshPart
	struct MeshPartHash {
		std::size_t operator()(const MeshPart& part) const noexcept {
			std::size_t h = 0;

			std::hash<std::string> hashString;
			std::hash<uint64_t> hashUInt64;
			std::hash<int> hashInt;

			hashCombine(h, hashString(part.name));
			hashCombine(h, hashUInt64(part.index));
			hashCombine(h, hashInt(static_cast<int>(part.type)));

			return h;
		}
	};

}