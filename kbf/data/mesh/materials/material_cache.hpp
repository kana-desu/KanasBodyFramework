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

		HashedMaterialList& getPieceCache(ArmourPiece piece) {
			switch (piece)
			{
			case ArmourPiece::AP_HELM: return helm;
			case ArmourPiece::AP_BODY: return body;
			case ArmourPiece::AP_ARMS: return arms;
			case ArmourPiece::AP_COIL: return coil;
			case ArmourPiece::AP_LEGS: return legs;
			}

			throw std::runtime_error("[MaterialCache] Invalid cache request - piece not supported.");
		}

		const HashedMaterialList& getPieceCache(ArmourPiece piece) const {
			switch (piece)
			{
			case ArmourPiece::AP_HELM: return helm;
			case ArmourPiece::AP_BODY: return body;
			case ArmourPiece::AP_ARMS: return arms;
			case ArmourPiece::AP_COIL: return coil;
			case ArmourPiece::AP_LEGS: return legs;
			}

			throw std::runtime_error("[MaterialCache] Invalid cache request - piece not supported.");
		}

	};

}