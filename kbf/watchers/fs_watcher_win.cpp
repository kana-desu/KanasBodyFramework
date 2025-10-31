#ifdef _WIN32

#include <kbf/watchers/fs_watcher_win.hpp>

#include <Windows.h>
#include <cassert>
#include <chrono>
#include <iostream>
#include <locale>
#include <codecvt>
#include <set>

namespace kbf::watchers {

    FsWatcherWin::FsWatcherWin(
        const std::string& dir, FsWatchListener* listener, bool recursive
    ) : watchDir{ dir },
        listener{ listener },
        recursive{ recursive },
        watchHandle{getWatchHandle()} 
    {}

    FsWatcherWin::~FsWatcherWin() {
        if (watching) {
            endWatch();
        }
    }

    void FsWatcherWin::watch() {
        std::cout << "[KBF::FsWatcherWin] INFO: Beginning watch of dir " << watchDir << std::endl;
        watching = true;

        watcherThread = std::make_unique<std::thread>(& FsWatcherWin::asyncWatch, this);
    }
    
    void FsWatcherWin::asyncWatch() {
        std::set<std::string> modifiedFiles;
        // ~Human reaction time. Not the best solution and may cause a missed update
        //   But quite frankly if a pipeline recreates in less than 200ms, updating it twice is fine. 
        constexpr float MIN_WAIT_INTERVAL_MS = 200.0f;

        auto start = std::chrono::steady_clock::now();

        while (watching) {
            DWORD bytesReturned = 0;
            BOOL res = ReadDirectoryChangesW(
                watchHandle.handle,
                watchHandle.buffer.data(),
                NOTIF_BUFFER_ALLOC_SIZE,
                recursive,
                FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME,
                &bytesReturned,
                NULL,
                NULL
            );

            // Handle watch callbacks
            if (res != 0) {
                FILE_NOTIFY_INFORMATION* info =
                    reinterpret_cast<FILE_NOTIFY_INFORMATION*>(watchHandle.buffer.data());

                do {
                    // Get notif data
                    FsWatchAction action = toFsWatchAction(info->Action);
                    std::wstring w_Filename{ info->FileName, info->FileNameLength / sizeof(wchar_t) };
                    std::string filename = toString(w_Filename);

                    // Handle action immediately if it is not a modification
                    // Modification handling is deferred a few ms to prevent double notifications from Windows.
                    if (action != FsWatchAction::FS_MODIFIED) {
                        listener->handleFileAction(
                            action,
                            watchDir,
                            filename
                        );
                    }
                    else {
                        modifiedFiles.insert(filename);
                    }

                    // Next notif
                    info = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(
                        reinterpret_cast<BYTE*>(info) + info->NextEntryOffset
                    );

                } while (info->NextEntryOffset > 0);
            }

            // Deferred handling of double modification notifications
            auto now = std::chrono::steady_clock::now();
            size_t elapsed = static_cast<size_t>(
                std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count());

            if (elapsed > MIN_WAIT_INTERVAL_MS && !modifiedFiles.empty()) {
                for (auto& filename : modifiedFiles) {
                    listener->handleFileAction(
                        FsWatchAction::FS_MODIFIED,
                        watchDir,
                        filename
                    );
                }
                modifiedFiles.clear();
                start = now;
            }
            else {
                modifiedFiles.clear();
            }
        }
    }

    void FsWatcherWin::endWatch() {
        watching = false;

        std::cout << "[KBF::FsWatcherWin] INFO: Ending watch of dir " << watchDir << std::endl;
        CancelIoEx(watchHandle.handle, NULL);
        watcherThread->join();
        std::cout << "[KBF::FsWatcherWin] INFO: Watch thread finished for dir " << watchDir << std::endl;

        watcherThread = nullptr;
    }

    FsWatcherWin::DirWatchHandle FsWatcherWin::getWatchHandle() {
        DirWatchHandle watchHandle{};
        watchHandle.handle = CreateFileA(
            watchDir.c_str(),
            FILE_LIST_DIRECTORY,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS,
            NULL
        );

        return std::move(watchHandle);
    }

    FsWatchAction FsWatcherWin::toFsWatchAction(DWORD action) {
        switch (action) {
        case FILE_ACTION_ADDED:            return FsWatchAction::FS_ADD;
        case FILE_ACTION_MODIFIED:         return FsWatchAction::FS_MODIFIED;
        case FILE_ACTION_REMOVED:          return FsWatchAction::FS_DELETE;
        case FILE_ACTION_RENAMED_NEW_NAME: return FsWatchAction::FS_RENAME_NEW;
        case FILE_ACTION_RENAMED_OLD_NAME: return FsWatchAction::FS_RENAME_OLD;
        default:
            throw std::runtime_error("[KBF::FsWatcherWin] Tried to convert invalid FILE_ACTION.");
        }

        // This should never run
        return FS_ADD;
    }

    std::string FsWatcherWin::toString(const std::wstring& wstr) {
        using convert_typeX = std::codecvt_utf8<wchar_t>;
        std::wstring_convert<convert_typeX, wchar_t> converterX;

        return converterX.to_bytes(wstr);
    }
}

#endif