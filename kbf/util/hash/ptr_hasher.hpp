#pragma once

#include <cstdint>
#include <functional>

namespace kbf {

    struct PtrHasher {
        template <typename... Ptrs>
        size_t operator()(Ptrs... ptrs) const noexcept {
            size_t seed = 0;
            (..., combine(seed, ptrs));
            return seed;
        }

    private:
        template <typename T>
        void combine(std::size_t& seed, T* ptr) const noexcept {
            std::uintptr_t addr = reinterpret_cast<std::uintptr_t>(ptr);
            std::size_t h = std::hash<std::uintptr_t>{}(addr);
            // A simple hash combine
            seed ^= h + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
        }
    };
}