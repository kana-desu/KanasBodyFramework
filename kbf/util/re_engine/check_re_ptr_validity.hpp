#pragma once 

#include <reframework/API.hpp>

namespace kbf {

	inline bool checkREPtrValidity(
		const reframework::API::ManagedObject* ptr,
		const reframework::API::TypeDefinition* requiredType
	) {
		if (ptr == nullptr) return true;
		if (!reframework::API::get()->sdk()->managed_object->is_managed_object((void*)ptr)) return false;
		return ptr->get_type_definition() == requiredType;
	}

}