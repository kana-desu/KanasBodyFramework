#pragma once

#include <string>
#include <sstream>
#include <iomanip>

template <typename T>
inline std::string ptrToHexString(T* ptr) {
    std::ostringstream oss;
    oss << "0x"
        << std::hex << std::setw(sizeof(void*) * 2) << std::setfill('0')
        << reinterpret_cast<uintptr_t>(ptr);
    return oss.str();
}