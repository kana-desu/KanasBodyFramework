#pragma once

#include <kbf/data/armour/armour_info.hpp>
#include <kbf/data/player/player_data.hpp>

#include <reframework/API.hpp>
#include <glm/glm.hpp>

#include <string>
#include <memory>

namespace kbf {

	struct PlayerPointers {
		reframework::API::ManagedObject* Transform;

		bool hasNull() const {
			return Transform == nullptr;
		}
	};

	struct PlayerOptionalPointers {
		reframework::API::ManagedObject* cPlayerManageInfo;
		reframework::API::ManagedObject* HunterCharacter;
		reframework::API::ManagedObject* Motion;
		reframework::API::ManagedObject* cHunterCreateInfo;
		reframework::API::ManagedObject* EventModelSetupper;
		reframework::API::ManagedObject* VolumeOccludee;
	};

	struct PlayerInfo {
		PlayerData playerData; // this is a unique identifier
		size_t index;
		PlayerPointers         pointers;
		PlayerOptionalPointers optionalPointers;

		bool visible = false;
		bool weaponDrawn = false;
		bool inCombat = false;

		float distanceFromCameraSq;
	};

}