#pragma once

#include <string>

namespace kbf {

	template<typename T>
	struct QuickMaterialOverride {
		bool enabled             = false;
		std::string materialName = "";
		std::string paramName    = "";
		T value;

		const bool operator==(const QuickMaterialOverride<T>& other) const {
			return enabled == other.enabled
				&& materialName == other.materialName
				&& paramName == other.paramName
				&& value == other.value;
		}

	};

}