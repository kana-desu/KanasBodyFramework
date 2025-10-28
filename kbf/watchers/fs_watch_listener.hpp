#pragma once 

#include <kbf/watchers/fs_watch_action.hpp>

#include <string>

namespace kbf::watchers {

    class FsWatchListener {
    public:
        // Note: This function should be thread safe as it will potentially
        //       be invoked from multiple listener threads.
        virtual void handleFileAction(
            FsWatchAction action,
            const std::string& dir,
            const std::string& filename
        ) = 0;

    private:
    };

}