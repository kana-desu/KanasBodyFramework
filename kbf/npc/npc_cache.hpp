#pragma once

#include <reframework/API.hpp>

#include <kbf/util/re_engine/check_re_ptr_validity.hpp>

namespace kbf {

	struct MainMenuNpcCache {
		reframework::API::ManagedObject* Transform;
		reframework::API::ManagedObject* VolumeOccludee;
		reframework::API::ManagedObject* MeshBoundary;

		bool isValid() const {
			static const reframework::API::TypeDefinition* def_ViaTransform       = REApi::get()->tdb()->find_type("via.Transform");
			static const reframework::API::TypeDefinition* def_VolumeOcludee      = REApi::get()->tdb()->find_type("via.render.VolumeOccludee");
			static const reframework::API::TypeDefinition* def_MeshBoundary       = REApi::get()->tdb()->find_type("ace.MeshBoundary");

			if (!checkREPtrValidity(Transform,      def_ViaTransform))       return false;
			if (!checkREPtrValidity(VolumeOccludee, def_VolumeOcludee))      return false;
			if (!checkREPtrValidity(MeshBoundary,   def_MeshBoundary))       return false;

			return true;
		}

	};

	struct NormalGameplayNpcCache {
		bool cacheIsEmpty = false;

		reframework::API::ManagedObject* Transform;
		reframework::API::ManagedObject* Motion;
		reframework::API::ManagedObject* HunterCharacter;

		bool female;
		std::string prefabPath;

		bool isEmpty() const { return cacheIsEmpty; }

		bool isValid() const {
			if (isEmpty()) return true;

			static const reframework::API::TypeDefinition* def_ViaTransform       = REApi::get()->tdb()->find_type("via.Transform");
			static const reframework::API::TypeDefinition* def_ViaMotionAnimation = REApi::get()->tdb()->find_type("via.motion.Motion");
			static const reframework::API::TypeDefinition* def_NpcCharacter       = REApi::get()->tdb()->find_type("app.NpcCharacter");
			static const reframework::API::TypeDefinition* def_HunterCharacter    = REApi::get()->tdb()->find_type("app.HunterCharacter");

			if (!checkREPtrValidity(Transform,    def_ViaTransform))       return false;
			if (!checkREPtrValidity(Motion,       def_ViaMotionAnimation)) return false;

			// HunterCharacter is valid as both HunterCharacter and NpcCharacter (latter more common)
			if (!checkREPtrValidity(HunterCharacter, def_NpcCharacter)) {
				return checkREPtrValidity(HunterCharacter, def_HunterCharacter);
			}

			return true;
		}
	};

}