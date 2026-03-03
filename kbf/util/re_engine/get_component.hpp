#pragma once

#include <kbf/util/re_engine/reinvoke.hpp>
#include <kbf/util/string/to_lower.hpp>

#include <reframework/API.hpp>

using REApi = reframework::API;

namespace kbf {

    // Returns the managed array of components for the given type, or nullptr.
    static inline REApi::ManagedObject* getComponentArray(
        REApi::ManagedObject* gameObject,
        const std::string& type
    ) {
        REApi::ManagedObject* t = REApi::get()->typeof(type.c_str());
        if (t == nullptr) return nullptr;
        return REInvokePtr<REApi::ManagedObject>(gameObject, "findComponents(System.Type)", { (void*)t });
    }

    inline REApi::ManagedObject* getComponent(
        REApi::ManagedObject* gameObject,
        const std::string& type
    ) {
        REApi::ManagedObject* t = REApi::get()->typeof(type.c_str());
        if (t == nullptr) return nullptr;
        return REInvokePtr<REApi::ManagedObject>(gameObject, "getComponent(System.Type)", { (void*)t });
    }

    inline std::vector<REApi::ManagedObject*> findComponents(
        REApi::ManagedObject* gameObject,
        const std::string& type,
        const std::vector<std::string>& filters = {}
    ) {
        REApi::ManagedObject* comps = getComponentArray(gameObject, type);
        if (comps == nullptr) return {};

        int arrSize = REInvoke<int>(comps, "GetLength(System.Int32)", { (void*)0 }, InvokeReturnType::DWORD);
        if (arrSize <= 0) return {};

        std::vector<std::string> filtersLower;
        filtersLower.reserve(filters.size());
        for (const auto& f : filters)
            filtersLower.push_back(toLower(f));

        const bool noFilter = filtersLower.empty() ||
            std::any_of(filtersLower.begin(), filtersLower.end(), [](const std::string& f) { return f.empty(); });

        std::vector<REApi::ManagedObject*> objs;
        objs.reserve(arrSize);

        for (int i = 0; i < arrSize; i++) {
            REApi::ManagedObject* c = REInvokePtr<REApi::ManagedObject>(comps, "get_Item(System.Int32)", { (void*)i });
            if (!c) continue;

            if (noFilter) { objs.push_back(c); continue; }

            REApi::ManagedObject* obj = REInvokePtr<REApi::ManagedObject>(c, "get_GameObject", {});
            if (!obj) continue; 

            const std::string compName = toLower(REInvokeStr(obj, "get_Name", {}));
            if (compName.empty()) continue;

            for (const auto& f : filtersLower) {
                if (compName.find(f) != std::string::npos) { objs.push_back(c); break; }
            }
        }

        return objs;
    }

    inline REApi::ManagedObject* findComponent(
        REApi::ManagedObject* gameObject,
        const std::string& type,
        const std::string& filter = ""
    ) {
        auto results = findComponents(gameObject, type, { filter });
        return results.empty() ? nullptr : results.front();
    }

}