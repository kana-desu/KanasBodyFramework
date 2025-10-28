#pragma once

#include <kbf/data/armour/armour_info.hpp>

#include <reframework/API.hpp>
#include <glm/glm.hpp>

#include <string>

namespace kbf {

	struct NpcPointers {
		reframework::API::ManagedObject* Transform;

		bool hasNull() const {
			return Transform == nullptr;
		}
	};

	struct NpcOptionalPointers {
		reframework::API::ManagedObject* cNpcManageInfo;
		reframework::API::ManagedObject* GameObject;
		reframework::API::ManagedObject* Motion;
		reframework::API::ManagedObject* HunterCharacter;
		reframework::API::ManagedObject* NpcAccessor;
		reframework::API::ManagedObject* NpcParamHolder;
		reframework::API::ManagedObject* NpcVisualSetting;
		reframework::API::ManagedObject* EventModelSetupper;
		reframework::API::ManagedObject* VolumeOccludee;
		reframework::API::ManagedObject* MeshBoundary;
	};

	struct NpcInfo {
		size_t index;
		bool female = true;

		NpcPointers         pointers;
		NpcOptionalPointers optionalPointers;
		
		std::string prefabPath;

		bool visible;
		
		float distanceFromCameraSq;
		glm::vec3 position;
	};

}