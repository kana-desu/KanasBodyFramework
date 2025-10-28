#pragma once

#include <reframework/API.hpp>

#include <cassert>

namespace kbf {

    template <typename T>
    inline void write_re_memory_no_check(reframework::API::ManagedObject* obj, int32_t offset, T value) {
		assert(obj != nullptr);
        *(T*)((uintptr_t)obj + offset) = value;
    }

}