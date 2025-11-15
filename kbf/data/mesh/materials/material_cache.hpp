#pragma once

#include <kbf/data/armour/armour_set.hpp>
#include <kbf/data/mesh/materials/hashed_material_list.hpp>

#include <set>

namespace kbf {

	struct MaterialCache {
		ArmourSetWithCharacterSex armour;
		HashedMaterialList helm;
		HashedMaterialList body;
		HashedMaterialList arms;
		HashedMaterialList coil;
		HashedMaterialList legs;
	};

}