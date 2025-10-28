#pragma once

#include <vector>
#include <string>

namespace kbf {

	class HashedBoneList {
	public:
		HashedBoneList(std::vector<std::string> bones = {}) : bones{ bones }, hash{ hashBones(bones) } {}
		HashedBoneList(std::vector<std::string> bones, size_t hash) : bones{ bones }, hash{ hash } {}

		size_t getHash() const { return hash; }
		const std::vector<std::string>& getBones() const { return bones; }
		bool hasBone(const std::string& boneName) const { return std::find(bones.begin(), bones.end(), boneName) != bones.end(); }

		static const size_t hashBones(const std::vector<std::string>& bones) {
			size_t hash = 0;
			std::hash<std::string> hasher;

			for (const std::string& bone : bones) {
				hash ^= hasher(bone) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
			}

			return hash;
		}

	private:
		size_t hash;
		std::vector<std::string> bones;
	};

}