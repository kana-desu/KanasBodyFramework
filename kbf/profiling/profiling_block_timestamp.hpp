#pragma once

#include <chrono>

namespace kbf {

    struct ProfilingBlockTimestamp {
        std::chrono::steady_clock::time_point start{};
        std::chrono::steady_clock::time_point end{};
    };

}