#pragma once

#include <reframework/API.hpp>

namespace kbf {

    template <typename T>
    inline T* re_memory_ptr(reframework::API::ManagedObject* obj, int32_t offset) {
        return (T*)((uintptr_t)obj + offset);
    }

}