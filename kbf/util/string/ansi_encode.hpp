#pragma once

#include <sstream>
#include <iomanip>

namespace kbf {

    inline std::string AnsiPercentEncode(const std::string& utf8) {
        std::ostringstream oss;
        for (unsigned char c : utf8) {
            if (c >= 32 && c <= 126) {
                oss << c; // printable ASCII
            }
            else {
                oss << '%' << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << (int)c;
            }
        }
        return oss.str();
    }

    inline std::string AnsiPercentDecode(const std::string& str) {
        std::ostringstream oss;
        for (size_t i = 0; i < str.size(); ++i) {
            if (str[i] == '%' && i + 2 < str.size()) {
                int value;
                std::istringstream(str.substr(i + 1, 2)) >> std::hex >> value;
                oss << static_cast<char>(value);
                i += 2;
            }
            else {
                oss << str[i];
            }
        }
        return oss.str();
    }

}