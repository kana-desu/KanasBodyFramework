#pragma once

#include <kbf/data/armour/armour_info.hpp>
#include <kbf/data/player/player_data.hpp>
#include <kbf/mesh/bone_manager.hpp>
#include <kbf/mesh/part_manager.hpp>
#include <kbf/mesh/material_manager.hpp>
#include <kbf/util/re_engine/check_re_ptr_validity.hpp>

#include <reframework/API.hpp>
#include <glm/glm.hpp>

#include <format>
#include <string>
#include <memory>

namespace kbf {

	// Player info that isn't updated every frame
	struct PersistentPlayerInfo {
		PlayerData playerData;
		size_t index;
		
		ArmourInfo armourInfo;
		reframework::API::ManagedObject* Transform_base = nullptr;
		reframework::API::ManagedObject* Transform_helm = nullptr;
		reframework::API::ManagedObject* Transform_body = nullptr;
		reframework::API::ManagedObject* Transform_arms = nullptr;
		reframework::API::ManagedObject* Transform_coil = nullptr;
		reframework::API::ManagedObject* Transform_legs = nullptr;

		reframework::API::ManagedObject* Wp_Parent_GameObject           = nullptr;
		reframework::API::ManagedObject* WpSub_Parent_GameObject        = nullptr;
		reframework::API::ManagedObject* Wp_ReserveParent_GameObject    = nullptr;
		reframework::API::ManagedObject* WpSub_ReserveParent_GameObject = nullptr;

		reframework::API::ManagedObject* Wp_Insect        = nullptr;
		reframework::API::ManagedObject* Wp_ReserveInsect = nullptr;

		reframework::API::ManagedObject* Slinger_GameObject = nullptr;

		std::unique_ptr<BoneManager> boneManager         = nullptr;
		std::unique_ptr<PartManager> partManager         = nullptr;
		std::unique_ptr<MaterialManager> materialManager = nullptr;

		bool areSetPointersValid() const {
			static reframework::API::TypeDefinition* def_ViaTransform  = reframework::API::get()->tdb()->find_type("via.Transform");
			static reframework::API::TypeDefinition* def_ViaGameObject = reframework::API::get()->tdb()->find_type("via.GameObject");

			if (!checkREPtrValidity(Transform_base, def_ViaTransform)) return false;
			if (!checkREPtrValidity(Transform_helm, def_ViaTransform)) return false;
			if (!checkREPtrValidity(Transform_body, def_ViaTransform)) return false;
			if (!checkREPtrValidity(Transform_arms, def_ViaTransform)) return false;
			if (!checkREPtrValidity(Transform_coil, def_ViaTransform)) return false;
			if (!checkREPtrValidity(Transform_legs, def_ViaTransform)) return false;

			if (!checkREPtrValidity(Wp_Parent_GameObject          , def_ViaGameObject)) return false;
			if (!checkREPtrValidity(WpSub_Parent_GameObject       , def_ViaGameObject)) return false;
			if (!checkREPtrValidity(Wp_ReserveParent_GameObject   , def_ViaGameObject)) return false;
			if (!checkREPtrValidity(WpSub_ReserveParent_GameObject, def_ViaGameObject)) return false;

			if (!checkREPtrValidity(Slinger_GameObject, def_ViaGameObject)) return false;

			return true;
		}
	};

}