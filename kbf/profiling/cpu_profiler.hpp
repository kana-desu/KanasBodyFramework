#pragma once

#include <kbf/profiling/profiling_block.hpp>
#include <kbf/profiling/profiling_block_timestamp.hpp>
#include <kbf/profiling/profiling_sample.hpp>

#include <memory>
#include <map>
#include <deque>

#ifdef KBF_DEBUG_BUILD
    #define BEGIN_CPU_PROFILING_BLOCK(profiler, blockName)  \
       if ((profiler)) (profiler)->beginBlock((blockName));

    #define END_CPU_PROFILING_BLOCK(profiler, blockName)  \
       if ((profiler)) (profiler)->endBlock((blockName));

    #define PROFILED_FLOW_OP(profiler, blockName, op)           \
        { 					                                    \
            END_CPU_PROFILING_BLOCK((profiler), (blockName))    \
            op;                                                 \
        }
#else
    #define BEGIN_CPU_PROFILING_BLOCK(profiler, blockName)
    #define END_CPU_PROFILING_BLOCK(profiler, blockName)
#endif

namespace kbf {

    class CpuProfiler {
    public:

        static std::unique_ptr<CpuProfiler> GlobalTimelineProfiler;
        static std::unique_ptr<CpuProfiler> GlobalMultiScopeProfiler;
        typedef std::map<std::string, ProfilingBlock> NamedProfilingBlockMap;
        typedef std::map<std::string, std::deque<ProfilingSample>> NamedSampleHistoryMap;

        class Builder {
        public:
            Builder() {}

            Builder& addBlock(std::string name);
			Builder& setWindowSize(double seconds);
            std::unique_ptr<CpuProfiler> build() const;

        private:
            double windowSize = 5.0;
            uint32_t currentBlockIdx = 0u;
            NamedProfilingBlockMap profilingBlocks;
        };

        CpuProfiler(
            NamedProfilingBlockMap profilingBlocks,
            double windowSize
        );
        ~CpuProfiler() = default;

        double getMs(const std::string& name) const;
        double getAccumulatedMs(const std::string& name) const;
        double getAverageMs(const std::string& name) const;
        void resetAccumulated(const std::string& name);
        void resetAccumulatedAll();

        void setBlockMillis(const std::string& name, double ms);
        void beginBlock(const std::string& name);
        void endBlock(const std::string& name);

        const NamedProfilingBlockMap& getNamedBlocks() const {
            return namedProfilingBlocks;
        }

    private:
        NamedProfilingBlockMap namedProfilingBlocks;
        NamedSampleHistoryMap namedSampleHistories;
        NamedSampleHistoryMap namedTotalSampleHistories;
        std::map<std::string, ProfilingBlockTimestamp> recordedTimestamps;
        double windowSize = 1.0f;
    };

}