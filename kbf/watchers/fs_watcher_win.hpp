#pragma once

#ifdef _WIN32

#include <kbf/watchers/fs_watch_listener.hpp>
#include <kbf/watchers/fs_watch_action.hpp>

// Windows includes, excluding some annoying macros.
#include <Windows.h>

#include <string>
#include <vector>
#include <array>
#include <thread>

namespace kbf::watchers {

    class FsWatcherWin {
    public:
        FsWatcherWin(
            const std::string& dir, FsWatchListener* listener, bool recursive);
        ~FsWatcherWin();

        void watch();
        void endWatch();

    private:
        constexpr static int NOTIF_BUFFER_ALLOC_SIZE = 4096;

        struct DirWatchHandle {
            HANDLE handle;
            std::array<BYTE, NOTIF_BUFFER_ALLOC_SIZE> buffer{};
        };

        DirWatchHandle getWatchHandle();
        static FsWatchAction toFsWatchAction(DWORD action);
        static std::string toString(const std::wstring& wstr);

        void asyncWatch();
        std::unique_ptr<std::thread> watcherThread;

        const std::string watchDir;
        DirWatchHandle watchHandle;
        FsWatchListener* listener;
        bool recursive;

        // TODO: atomic?
        bool watching = false;
    };

}

#endif