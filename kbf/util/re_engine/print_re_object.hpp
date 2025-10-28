#pragma once 

#include <kbf/util/string/ptr_to_hex_string.hpp>
#include <kbf/util/re_engine/re_object_properties_to_string.hpp>

#include <reframework/API.hpp>

#include <string>
#include <format>

namespace kbf {

	inline std::string printReObject(reframework::API::ManagedObject* obj, std::string name) {
		return std::format("{} PTR: {} - Properties:\n{}",
			name,
			ptrToHexString(obj),
			reObjectPropertiesToString(obj)
		);
	}

}