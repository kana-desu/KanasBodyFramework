#pragma once 

#include <kbf/watchers/fs_watch_listener.hpp>
#include <kbf/util/functional/invoke_callback.hpp>

#include <filesystem>
#include <functional>
#include <string>

namespace kbf {

    class KbfDllUpdateListener : public watchers::FsWatchListener {
    public:
        KbfDllUpdateListener(
            std::filesystem::path dllPath
        );

        void onHotReload(std::function<void()> callback) {
            onHotReloadCallback = std::move(callback);
		}

        void handleFileAction(
            watchers::FsWatchAction action,
            const std::string& dir,
            const std::string& filename
        ) override;

    private:
        std::filesystem::path dllPath;
        std::function<void()> onHotReloadCallback;
    };

}