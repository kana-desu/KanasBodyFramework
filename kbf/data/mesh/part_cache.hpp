#pragma once

#include <kbf/data/armour/armour_set.hpp>
#include <kbf/data/mesh/hashed_part_list.hpp>

#include <set>

namespace kbf {

	struct PartCache {
		ArmourSetWithCharacterSex armour;
		HashedPartList set;
		HashedPartList helm;
		HashedPartList body;
		HashedPartList arms;
		HashedPartList coil;
		HashedPartList legs;
	};

}