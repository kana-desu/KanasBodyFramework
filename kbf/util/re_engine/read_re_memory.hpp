#pragma once

#include <reframework/API.hpp>

#include <cassert>

namespace kbf {

    template <typename T>
    inline T read_re_memory_no_check(reframework::API::ManagedObject* obj, int32_t offset) {
        return *(T*)((uintptr_t)obj + offset);
    }

}