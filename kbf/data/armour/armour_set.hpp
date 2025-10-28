#pragma once

#include <string>
#include <unordered_map>

namespace kbf {

	struct ArmourSet {
		std::string name;
		bool female = true;

		bool operator==(const ArmourSet& other) const {
			return female == other.female && name == other.name;
		}

		bool operator<(const ArmourSet& other) const {
			return std::tie(female, name) < std::tie(other.female, other.name);
		}
	};


	struct ArmourSetWithCharacterSex {
		ArmourSet set;
		bool characterFemale = true;

		bool operator==(const ArmourSetWithCharacterSex& other) const {
			return characterFemale == other.characterFemale && set == other.set;
		}

		bool operator<(const ArmourSetWithCharacterSex& other) const {
			return std::tie(characterFemale, set) < std::tie(other.characterFemale, other.set);
		}
	};

}

namespace std {

	template <>
	struct hash<kbf::ArmourSet> {
		std::size_t operator()(const kbf::ArmourSet& a) const {
			std::size_t h = std::hash<std::string>()(a.name);
			h ^= std::hash<bool>()(a.female) + 0x9e3779b9 + (h << 6) + (h >> 2);
			return h;
		}
	};

	template <>
	struct hash<kbf::ArmourSetWithCharacterSex> {
		std::size_t operator()(const kbf::ArmourSetWithCharacterSex& a) const {
			std::size_t h = std::hash<kbf::ArmourSet>()(a.set);
			h ^= std::hash<bool>()(a.characterFemale) + 0x9e3779b9 + (h << 6) + (h >> 2);
			return h;
		}
	};

}