#pragma once

#include <kbf/util/re_engine/reinvoke.hpp>
#include <kbf/util/string/to_lower.hpp>

namespace kbf {

    std::vector<std::pair<size_t, REApi::ManagedObject*>> searchSceneForObjects(std::string nameContainsStr, std::string typeDef = "via.Transform") {
        RENativeSingleton sceneManager{ "via.SceneManager" };

        static auto sceneManagerTypeDefinition = REApi::get()->tdb()->find_type("via.SceneManager");
        static auto getCurrentSceneMethodDefinition = sceneManagerTypeDefinition->find_method("get_CurrentScene");

        REApi::ManagedObject* currentScene = getCurrentSceneMethodDefinition->call<REApi::ManagedObject*>(REApi::get()->get_vm_context(), sceneManager);
        if (!currentScene) return {};

        REApi::ManagedObject* type = REApi::get()->typeof(typeDef.c_str());

        REApi::ManagedObject* transformComponents = REInvokePtr<REApi::ManagedObject>(currentScene, "findComponents(System.Type)", { (void*)type });
        if (!transformComponents) return {};

        const int numComponents = REInvoke<int>(transformComponents, "GetLength", { (void*)0 }, InvokeReturnType::DWORD);

        std::vector<std::pair<size_t, REApi::ManagedObject*>> objs{};
        std::string filterLower = toLower(nameContainsStr);

        // Build pointer -> offset lookup once
        std::unordered_map<REApi::ManagedObject*, size_t> pointerOffsets{};

        const uint32_t sceneSize = currentScene->get_type_definition()->get_size();
        uint8_t* sceneBase = reinterpret_cast<uint8_t*>(currentScene);

        for (uint32_t offset = sizeof(void*); offset < sceneSize; offset += sizeof(void*)) {
            auto candidate = *reinterpret_cast<REApi::ManagedObject**>(sceneBase + offset);
            if (candidate) {
                pointerOffsets.emplace(candidate, offset);
            }
        }

        for (int i = 0; i < numComponents; i++) {
            REApi::ManagedObject* transform = REInvokePtr<REApi::ManagedObject>(transformComponents, "get_Item", { (void*)i });
            if (!transform) continue;

            REApi::ManagedObject* gameObject = REInvokePtr<REApi::ManagedObject>(transform, "get_GameObject", {});
            if (!gameObject) continue;

            std::string name = REInvokeStr(gameObject, "get_Name", {});
            if (!nameContainsStr.empty() && toLower(name).find(filterLower) == std::string::npos) continue;

            auto it = pointerOffsets.find(gameObject);
            if (it != pointerOffsets.end()) {
                objs.emplace_back(it->second, gameObject);
            }
        }

        return objs;
    }

}