#include <kbf/profiling/cpu_profiler.hpp>

#include <chrono>
#include <cassert>

namespace kbf {

    // Profiler reserved for tracking a global timeline of events. i.e. no overlapping between profiling blocks.
    std::unique_ptr<CpuProfiler> CpuProfiler::GlobalTimelineProfiler = nullptr;
    // Profiler reserved for arbitrary usage. i.e. allow overlapping profiling blocks.
    std::unique_ptr<CpuProfiler> CpuProfiler::GlobalMultiScopeProfiler = nullptr;

    // ---- Builder ----------------------------------------------------------------------------------------------
    CpuProfiler::Builder& CpuProfiler::Builder::addBlock(std::string name) {
        profilingBlocks.emplace(name, ProfilingBlock{ currentBlockIdx });
        currentBlockIdx += 1u;

        return *this;
    };

    CpuProfiler::Builder& CpuProfiler::Builder::setWindowSize(double seconds) {
        windowSize = seconds;
        return *this;
	}

    std::unique_ptr<CpuProfiler> CpuProfiler::Builder::build() const {
        return std::make_unique<CpuProfiler>(profilingBlocks, windowSize);
    }

    // ---- Profiler ---------------------------------------------------------------------------------------------
    CpuProfiler::CpuProfiler(
        NamedProfilingBlockMap profilingBlocks,
        double windowSize
    ) : namedProfilingBlocks{ profilingBlocks }, windowSize{ windowSize } {
        for (auto& kv : namedProfilingBlocks) {
            recordedTimestamps.emplace(kv.first, ProfilingBlockTimestamp{});
			namedSampleHistories.emplace(kv.first, std::deque<ProfilingSample>{});
        }
    }

    double CpuProfiler::getMs(const std::string& name) const {
        auto it = namedProfilingBlocks.find(name);
        if (it == namedProfilingBlocks.end()) return 0.0;
        return it->second.ms;
    }

    double CpuProfiler::getAccumulatedMs(const std::string& name) const {
        auto it = namedProfilingBlocks.find(name);
        if (it == namedProfilingBlocks.end()) return 0.0;
        return it->second.totalMs;
    }

    double CpuProfiler::getAverageMs(const std::string& name) const {
        auto it = namedProfilingBlocks.find(name);
        if (it == namedProfilingBlocks.end() || it->second.count == 0) return 0.0;
        return it->second.totalMs / static_cast<double>(it->second.count);
    }

    void CpuProfiler::resetAccumulated(const std::string& name) {
        auto it = namedProfilingBlocks.find(name);
        if (it != namedProfilingBlocks.end()) {
            it->second.totalMs = 0.0;
            it->second.count = 0;
        }
    }

    void CpuProfiler::resetAccumulatedAll() {
        for (auto& kv : namedProfilingBlocks) {
            kv.second.totalMs = 0.0;
            kv.second.count = 0;
        }
    }

    void CpuProfiler::setBlockMillis(const std::string& name, double ms) {
        namedProfilingBlocks[name].ms = ms;
    }

    void CpuProfiler::beginBlock(const std::string& name) {
        assert(namedProfilingBlocks.find(name) != namedProfilingBlocks.end()
            && "Tried to begin a block not specified when building the CpuProfiler.");
        ProfilingBlock& block = namedProfilingBlocks[name];
        recordedTimestamps[name].start = std::chrono::high_resolution_clock::now();
    }

    void CpuProfiler::endBlock(const std::string& name) {
        assert(namedProfilingBlocks.find(name) != namedProfilingBlocks.end()
            && "Tried to end a block not specified when building the CpuProfiler.");

        ProfilingBlock& block = namedProfilingBlocks[name];

        auto now = std::chrono::high_resolution_clock::now();
        auto& ts = recordedTimestamps[name];

        ts.end = now;

        double durationMs = std::chrono::duration<double, std::milli>(ts.end - ts.start).count();
        block.ms = durationMs;

        // accumulate
        block.totalMs += durationMs;
        block.count++;

        double nowSec = std::chrono::duration<double>(now.time_since_epoch()).count();

        // ------------------ duration (existing) ------------------
        auto& dq = namedSampleHistories[name];

        // 1. Remove samples from the back that are <= current duration
        while (!dq.empty() && dq.back().durationMs <= durationMs) {
            dq.pop_back();
        }

        // 2. Add new sample
        dq.push_back({ durationMs, nowSec });

        // 3. Remove samples outside window
        while (!dq.empty() && nowSec - dq.front().timestampSec > windowSize) {
            dq.pop_front();
        }

        // 4. Max duration in window
        block.maxMs = dq.front().durationMs;


        // ------------------ totalMs ------------------------
        auto& tq = namedTotalSampleHistories[name];

        // 1. Remove samples from the back that are <= current total
        while (!tq.empty() && tq.back().durationMs <= block.totalMs) {
            tq.pop_back();
        }

        // 2. Add new sample
        tq.push_back({ block.totalMs, nowSec });

        // 3. Remove samples outside window
        while (!tq.empty() && nowSec - tq.front().timestampSec > windowSize) {
            tq.pop_front();
        }

        // 4. Max total in window
        block.maxTotalMs = tq.front().durationMs;
    }

}