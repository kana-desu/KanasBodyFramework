#pragma once

#include <glm/glm.hpp>

#include <string>
#include <array>
#include <chrono>

namespace kbf {

	struct LogData {
		std::string data;
		glm::vec3 colour;
		std::chrono::system_clock::time_point timestamp;
	};

}