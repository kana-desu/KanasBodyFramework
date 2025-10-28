#pragma once

#include <windows.h>
#include <string>
#include <iostream>

namespace kbf {

    inline bool copyToClipboard(const std::string& text) {
        if (!OpenClipboard(nullptr)) return false;
        if (!EmptyClipboard()) return false;

        // Allocate a global memory object for the text
        HGLOBAL hGlob = GlobalAlloc(GMEM_MOVEABLE, text.size() + 1);
        if (!hGlob) {
            CloseClipboard();
            return false;
        }

        // Copy the string into the allocated memory
        memcpy(GlobalLock(hGlob), text.c_str(), text.size() + 1);
        GlobalUnlock(hGlob);

        // Place the handle on the clipboard
        if (!SetClipboardData(CF_TEXT, hGlob)) {
            GlobalFree(hGlob);
            CloseClipboard();
            return false;
        }

        CloseClipboard();
        return true;
    }

}