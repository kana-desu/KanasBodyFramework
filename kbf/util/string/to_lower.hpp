#pragma once

#include <string>
#include <algorithm>

namespace kbf {

	inline std::string toLower(const std::string& str) {
		std::string lowerStr = str;
		std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), ::tolower);
		return lowerStr;
	}

}