#pragma once

#define NOC_FILE_DIALOG_WIN32
#define NOC_FILE_DIALOG_IMPLEMENTATION
#include <kbf/util/io/noc_file_dialog.h>

// Windows implementation of folder browser for noc as it does not natively support this.
#ifdef NOC_FILE_DIALOG_WIN32

#include <windows.h>
#include <shlobj.h>
#include <string>

//inline std::string noc_browse_folder_open(std::string saved_path) {
//    TCHAR path[MAX_PATH];
//
//    // Callback function to set default folder
//    auto callback = [](HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData) -> int {
//        if (uMsg == BFFM_INITIALIZED) {
//            // Set the initial directory
//            SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
//        }
//        return 0;
//    };
//
//    BROWSEINFO bi = { 0 };
//    bi.lpszTitle = TEXT("Browse for folder...");
//    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE | BIF_EDITBOX;
//    bi.lpfn = callback;
//    bi.lParam = (LPARAM)saved_path.c_str(); // path to preselect
//
//    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
//
//    if (pidl != nullptr) {
//        SHGetPathFromIDList(pidl, path);
//
//        // Free memory used
//        IMalloc* imalloc = nullptr;
//        if (SUCCEEDED(SHGetMalloc(&imalloc))) {
//            imalloc->Free(pidl);
//            imalloc->Release();
//        }
//
//        return path;
//    }
//
//    return "";
//}

inline std::string noc_browse_folder_open(const std::string& defaultPath = "") {
    std::string result;

    // Initialize COM
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr)) return "";

    IFileDialog* pFileDialog = nullptr;
    hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&pFileDialog));

    if (SUCCEEDED(hr)) {
        DWORD options;
        pFileDialog->GetOptions(&options);
        pFileDialog->SetOptions(options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);

        // Set default folder if provided
        if (!defaultPath.empty()) {
            IShellItem* pItem = nullptr;
            std::wstring wpath(defaultPath.begin(), defaultPath.end());
            hr = SHCreateItemFromParsingName(wpath.c_str(), nullptr, IID_PPV_ARGS(&pItem));
            if (SUCCEEDED(hr)) {
                pFileDialog->SetFolder(pItem);  // Sets the folder and focuses it
                pItem->Release();
            }
        }

        // Show dialog
        hr = pFileDialog->Show(nullptr);
        if (SUCCEEDED(hr)) {
            IShellItem* pResult;
            hr = pFileDialog->GetResult(&pResult);
            if (SUCCEEDED(hr)) {
                PWSTR pszFilePath = nullptr;
                hr = pResult->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
                if (SUCCEEDED(hr)) {
                    char path[MAX_PATH];
                    wcstombs(path, pszFilePath, MAX_PATH);
                    result = path;
                    CoTaskMemFree(pszFilePath);
                }
                pResult->Release();
            }
        }
        pFileDialog->Release();
    }

    CoUninitialize();
    return result;
}

#else

inline std::string noc_folder_dialog_open(std::string saved_path) {
    const char* pth = noc_file_dialog_open(NOC_FILE_DIALOG_DIR, nullptr, saved_path.c_str(), NULL);
    return (pth == nullptr) ? "" : std::string{ pth };
}

#endif