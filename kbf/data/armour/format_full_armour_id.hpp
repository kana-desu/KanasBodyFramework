#pragma once

#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>

namespace kbf {

    inline std::string formatFullArmourID(std::string prefix, int id1, int id2) {
        std::ostringstream oss;
        oss << prefix
            << std::setw(3) << std::setfill('0') << id1 << "_"
            << std::setw(4) << std::setfill('0') << id2;
        return oss.str();
    }

}