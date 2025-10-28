#include <kbf/data/armour/armour_set.hpp>
#include <kbf/data/bones/hashed_bone_list.hpp>

namespace kbf {

	struct BoneCache {
		ArmourSetWithCharacterSex armour;
		HashedBoneList set;
		HashedBoneList helm;
		HashedBoneList body;
		HashedBoneList arms;
		HashedBoneList coil;
		HashedBoneList legs;
	};

}