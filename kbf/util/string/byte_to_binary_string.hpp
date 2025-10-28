#pragma once

#include <cstdint>
#include <string>

namespace kbf {

	inline std::string byteToBinaryString(uint8_t byte) {
        std::string result;
        for (int i = 7; i >= 0; --i) {
            result.push_back(((byte >> i) & 1) ? '1' : '0');
        }
        return result;
    }

}