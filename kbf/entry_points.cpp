#include <kbf/entry_points.hpp>
#include <string_view>

namespace kbf {

    // ===== Helper for handle encoding =====
    inline EntryPointHandle makeHandle(uint32_t entry, uint32_t binding) {
        return (static_cast<uint64_t>(entry) << 32) | binding;
    }

    inline std::pair<uint32_t, uint32_t> decodeHandle(EntryPointHandle handle) {
        uint32_t entry = static_cast<uint32_t>(handle >> 32);
        uint32_t binding = static_cast<uint32_t>(handle & 0xFFFFFFFF);
        return { entry, binding };
    }

    inline bool EntryPoints::isValid(uint32_t entry, uint32_t binding) const {
        return entry < ENTRY_POINT_NAMES.size() &&
            binding < m_bindings[entry].size();
    }

    // ===== EntryPoints Implementation =====

    EntryPointHandle EntryPoints::addBinding(EntryTiming timing, const char* name, EntryCallback fn, bool active) {
        if (!name || !fn) return INVALID_HANDLE;

        // Find entry index
        int entryIndex = -1;
        for (size_t i = 0; i < ENTRY_POINT_NAMES.size(); ++i) {
            if (std::string_view(ENTRY_POINT_NAMES[i]) == name) {
                entryIndex = static_cast<int>(i);
                break;
            }
        }
        if (entryIndex < 0) return INVALID_HANDLE;

        auto& vec = m_bindings[entryIndex];
        vec.push_back(Binding{ std::move(fn), timing, active });
        return makeHandle(entryIndex, static_cast<uint32_t>(vec.size() - 1));
    }

    void EntryPoints::removeBinding(EntryPointHandle handle) {
        auto [entryIndex, bindingIndex] = decodeHandle(handle);
        if (!isValid(entryIndex, bindingIndex)) return;

        auto& b = m_bindings[entryIndex][bindingIndex];
        b.active = false;
        b.callback = nullptr;
    }

    void EntryPoints::setActive(EntryPointHandle handle, bool active) {
        auto [entryIndex, bindingIndex] = decodeHandle(handle);
        if (!isValid(entryIndex, bindingIndex)) return;

        m_bindings[entryIndex][bindingIndex].active = active;
    }

    void EntryPoints::dispatch(int index, EntryTiming timing) {
        if (index < 0 || index >= static_cast<int>(m_bindings.size())) return;
        auto& vec = m_bindings[index];
        for (auto& b : vec) {
            if (b.active && b.callback && b.timing == timing)
                b.callback();
        }
    }

    void EntryPoints::dispatch(const char* name, EntryTiming timing) {
        if (!name) return;

        int entryIndex = -1;
        for (size_t i = 0; i < ENTRY_POINT_NAMES.size(); ++i) {
            if (std::string_view(ENTRY_POINT_NAMES[i]) == name) {
                entryIndex = static_cast<int>(i);
                break;
            }
        }
        if (entryIndex >= 0) dispatch(entryIndex, timing);
    }

    std::vector<EntryPointHandle> EntryPoints::getActiveHandles(const char* name) const {
        std::vector<EntryPointHandle> result;
        if (!name) return result;

        int entryIndex = -1;
        for (size_t i = 0; i < ENTRY_POINT_NAMES.size(); ++i) {
            if (std::string_view(ENTRY_POINT_NAMES[i]) == name) {
                entryIndex = static_cast<int>(i);
                break;
            }
        }
        if (entryIndex < 0) return result;

        const auto& vec = m_bindings[entryIndex];
        for (uint32_t i = 0; i < vec.size(); ++i) {
            if (vec[i].active && vec[i].callback)
                result.push_back(makeHandle(entryIndex, i));
        }

        return result;
    }

}