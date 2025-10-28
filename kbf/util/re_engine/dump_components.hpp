#pragma once

#include <reframework/API.hpp>
#include <kbf/util/re_engine/reinvoke.hpp>

using REApi = reframework::API;

namespace kbf {

    // Public API
    inline std::vector<std::string> dumpComponents(REApi::ManagedObject* obj) {
        std::vector<std::string> result;
        
        REApi::ManagedObject* componentArr = REInvokePtr<REApi::ManagedObject>(obj, "get_Components", {});
        if (componentArr == nullptr) return {};

        const int numComponents = REInvoke<int>(componentArr, "GetLength", { (void*)0 }, InvokeReturnType::DWORD);

        for (int i = 0; i < numComponents; i++) {
            REApi::ManagedObject* component = REInvokePtr<REApi::ManagedObject>(componentArr, "get_Item", { (void*)i });
            if (component == nullptr) continue;

            std::string name = REInvokeStr(component, "ToString", {});
            result.push_back(name);
        }

        return result;
    }

}