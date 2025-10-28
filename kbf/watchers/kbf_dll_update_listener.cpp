#include <kbf/watchers/kbf_dll_update_listener.hpp>

#include <iostream>
#include <filesystem>

#include <Windows.h>

namespace kbf {

    KbfDllUpdateListener::KbfDllUpdateListener(std::filesystem::path dllPath) : dllPath{ dllPath } {}

    void KbfDllUpdateListener::handleFileAction(
        watchers::FsWatchAction action,
        const std::string& dir,
        const std::string& filename
    ) {
        switch (action) {
        // Code generating actions
        case watchers::FsWatchAction::FS_ADD:
        case watchers::FsWatchAction::FS_RENAME_NEW:
        case watchers::FsWatchAction::FS_MODIFIED: {
            std::filesystem::path fsDir = dir;
			std::filesystem::path fsFilename = filename;
            std::filesystem::path targetFilepath = std::filesystem::absolute(fsDir / fsFilename);
            bool isTargetDll = dllPath == targetFilepath;

            if (isTargetDll) {
                INVOKE_REQUIRED_CALLBACK(onHotReloadCallback);
            }

        } break;
        //Others for debug logging
        //case watchers::FsWatchAction::FS_RENAME_OLD: {
        //    MessageBoxA(0, ("INFO: File renamed from: <" + filename + ">.").c_str(), "KBF DEBUG WINDOW", 0);
        //} break;
        //case watchers::FsWatchAction::FS_DELETE: {
        //    MessageBoxA(0, ("INFO: File deleted: <" + filename + ">.").c_str(), "KBF DEBUG WINDOW", 0);
        //} break;
        default:
            break;
        }
    }

}