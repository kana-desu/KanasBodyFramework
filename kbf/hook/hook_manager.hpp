#pragma once

#include <reframework/API.hpp>

#include <unordered_map>
#include <string_view>
#include <format>

#include <Windows.h>

namespace kbf {

    class HookManager {
    public:
        HookManager() = delete;

        static void add_tdb(
            std::string_view typeName,
            std::string_view name,
            REFPreHookFn pre_fn, REFPostHookFn post_fn, bool ignore_jmp
        ) {
            reframework::API::Method* method = reframework::API::get()->tdb()->find_method(typeName, name);
            if (method == nullptr) {
                MessageBoxA(0, std::format("Failed to find method to hook: {}::{}", typeName, name).c_str(), "KBF DEBUG WINDOW", 0);
                return;
            }

			unsigned int hookHandle = method->add_hook(pre_fn, post_fn, ignore_jmp);
            
            if (registeredHooks.contains(method)) {
                registeredHooks[method].push_back(hookHandle);
            }
            else {
                registeredHooks[method] = std::vector<unsigned int>{ hookHandle };
			}
        }

        static void remove_all() {
            for (const auto& [method, hookIds] : registeredHooks) {
                for (const auto& id : hookIds) {
                    if (method) method->remove_hook(id);
                }
            }
            registeredHooks.clear();
		}

    private:
        inline static std::unordered_map<reframework::API::Method*, std::vector<unsigned int>> registeredHooks{};
    };

}