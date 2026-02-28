#include <kbf/entry_points.hpp>
#include <string_view>

namespace kbf {

    // Pointer that can be set by external module to unify the EntryPoints
    // singleton across DLL boundaries (hot-reload helper). If null, the
    // local internal singleton will be used.
    static EntryPoints* g_instance_override = nullptr;


    // ===== Helper for handle encoding =====
    inline EntryPointHandle makeHandle(uint32_t entry, uint32_t binding) {
        return (static_cast<uint64_t>(entry) << 32) | binding;
    }
    
// Exported helper for other DLLs to call into this module and set their
// local instance override pointer to a host-provided EntryPoints pointer.
extern "C" __declspec(dllexport) void kbf_set_entrypoints_override(kbf::EntryPoints* inst) {
    kbf::EntryPoints::setInstanceOverride(inst);
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

    // Instance management (defined here to avoid inline static in header)
    EntryPoints& EntryPoints::instance() {
        if (g_instance_override) return *g_instance_override;
        static EntryPoints s_instance;
        return s_instance;
    }

    void EntryPoints::setInstanceOverride(EntryPoints* inst) {
        g_instance_override = inst;
    }

    EntryPoints* EntryPoints::getInstanceOverride() {
        return g_instance_override;
    }

    // ===== EntryPoints Implementation =====

    EntryPointHandle EntryPoints::addBinding(EntryTiming timing, const char* name, EntryCallback fn, bool active) {
        return addBinding(timing, name, std::move(fn), active, nullptr);
    }

    EntryPointHandle EntryPoints::addBinding(EntryTiming timing, const char* name, EntryCallback fn, bool active, const char* tag) {
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
        Binding b{ std::move(fn), timing, active };
        if (tag) b.tag = std::string(tag);
        vec.push_back(std::move(b));
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
        if (!name) return {};

        int entryIndex = -1;
        for (size_t i = 0; i < ENTRY_POINT_NAMES.size(); ++i) {
            if (std::string_view(ENTRY_POINT_NAMES[i]) == name) {
                entryIndex = static_cast<int>(i);
                break;
            }
        }
        return getActiveHandles(entryIndex);
    }

    std::vector<EntryPointHandle> EntryPoints::getActiveHandles(int entryIndex) const {
        std::vector<EntryPointHandle> result;
        if (entryIndex < 0) return result;

        const auto& vec = m_bindings[entryIndex];
        for (uint32_t i = 0; i < vec.size(); ++i) {
            if (vec[i].active && vec[i].callback)
                result.push_back(makeHandle(entryIndex, i));
        }

        return result;
    }

    EntryTiming EntryPoints::getBindingTiming(EntryPointHandle handle) const {
        auto [entryIndex, bindingIndex] = decodeHandle(handle);
        if (!isValid(entryIndex, bindingIndex)) return EntryTiming::PRE_FUNCTION;
        return m_bindings[entryIndex][bindingIndex].timing;
    }

    void* EntryPoints::getBindingFunctionPointer(EntryPointHandle handle) const {
        auto [entryIndex, bindingIndex] = decodeHandle(handle);
        if (!isValid(entryIndex, bindingIndex)) return nullptr;
        const auto& fn = m_bindings[entryIndex][bindingIndex].callback;
        if (!fn) return nullptr;
        // Try to extract a plain function pointer target
        using fn_ptr_t = void(*)();
        if (auto target = fn.template target<fn_ptr_t>()) {
            return reinterpret_cast<void*>(*target);
        }
        return nullptr;
    }


    std::string EntryPoints::getBindingTag(EntryPointHandle handle) const {
        auto [entryIndex, bindingIndex] = decodeHandle(handle);
        if (!isValid(entryIndex, bindingIndex)) return std::string();
        return m_bindings[entryIndex][bindingIndex].tag;
    }

}