#pragma once

#include <string>

namespace kbf {

    struct ProfilingBlock {
        uint32_t idx;
        double ms         = 0.0; // Last measured duration
        double maxMs      = 0.0; // Max duration over sliding window
        double totalMs    = 0.0; // Accumulated duration
        double maxTotalMs = 0.0; // Max observed accumulated duration
        size_t count      = 0;   // Number of times block was executed    
    };

}