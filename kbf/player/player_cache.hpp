#pragma once

#include <reframework/API.hpp>

#include <kbf/data/player/player_data.hpp>
#include <kbf/util/re_engine/check_re_ptr_validity.hpp>

namespace kbf {

	struct NormalGameplayPlayerCache {
		bool cacheIsEmpty = false;

		reframework::API::ManagedObject* Transform;
		reframework::API::ManagedObject* Motion;
		reframework::API::ManagedObject* HunterCharacter;

		PlayerData playerData;

		bool isEmpty() const { return cacheIsEmpty; }

		bool isValid() const {
			if (isEmpty()) return true;

			static const reframework::API::TypeDefinition* def_ViaTransform       = reframework::API::get()->tdb()->find_type("via.Transform");
			static const reframework::API::TypeDefinition* def_ViaMotionAnimation = reframework::API::get()->tdb()->find_type("via.motion.Motion");
			static const reframework::API::TypeDefinition* def_HunterCharacter    = reframework::API::get()->tdb()->find_type("app.HunterCharacter");

			if (!checkREPtrValidity(Transform, def_ViaTransform))          return false;
			if (!checkREPtrValidity(Motion, def_ViaMotionAnimation))       return false;
			if (!checkREPtrValidity(HunterCharacter, def_HunterCharacter)) return false;

			return true;
		}
	};

}