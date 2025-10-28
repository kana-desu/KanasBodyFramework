#pragma once

#include <string>
#include <Windows.h>

#include <stdio.h>

namespace kbf {

    // Ported from kananlib
    inline std::string narrow(std::wstring_view str) {
        auto length = WideCharToMultiByte(CP_UTF8, 0, str.data(), (int)str.length(), nullptr, 0, nullptr, nullptr);
        std::string narrowStr{};

        narrowStr.resize(length);
        WideCharToMultiByte(CP_UTF8, 0, str.data(), (int)str.length(), (LPSTR)narrowStr.c_str(), length, nullptr, nullptr);

        return narrowStr;
    }

    inline std::wstring widen(std::string_view str) {
        auto length = MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.length(), nullptr, 0);
        std::wstring wideStr{};

        wideStr.resize(length);
        MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.length(), (LPWSTR)wideStr.c_str(), length);

        return wideStr;
    }


    inline std::string cvt_utf16_to_utf8(const std::u16string& input) {
        if (input.empty()) return {};

        // WideCharToMultiByte wants wchar_t* on Windows (UTF-16)
        auto* utf16 = reinterpret_cast<LPCWCH>(input.data());

        // First call: figure out required size in UTF-8 bytes
        int utf8Len = WideCharToMultiByte(
            CP_UTF8,
            0,
            utf16,
            static_cast<int>(input.size()),
            nullptr,
            0,
            nullptr,
            nullptr
        );

        if (utf8Len <= 0) {
            return {};
        }

        // Second call: actually convert
        std::string utf8(utf8Len, '\0');
        WideCharToMultiByte(
            CP_UTF8,
            0,
            utf16,
            static_cast<int>(input.size()),
            utf8.data(),
            utf8Len,
            nullptr,
            nullptr
        );

        return utf8;
    }

    inline std::string cvt_utf16_to_utf8(const std::wstring & input) {
        if (input.empty()) return {};

        // First call: get required UTF-8 buffer size
        int utf8Len = WideCharToMultiByte(
            CP_UTF8,
            0,
            input.data(),
            static_cast<int>(input.size()),
            nullptr,
            0,
            nullptr,
            nullptr
        );

        if (utf8Len <= 0) {
            return {};
        }

        // Second call: actually convert
        std::string utf8(utf8Len, '\0');
        WideCharToMultiByte(
            CP_UTF8,
            0,
            input.data(),
            static_cast<int>(input.size()),
            utf8.data(),
            utf8Len,
            nullptr,
            nullptr
        );

        return utf8;
	}

    inline std::wstring cvt_utf8_to_utf16(const std::string& input) {
        if (input.empty()) {
            return {};
        }

        int wideLen = MultiByteToWideChar(
            CP_UTF8,
            0,
            input.data(),
            static_cast<int>(input.size()),
            nullptr,
            0
        );

        if (wideLen <= 0) {
            return {};
        }

        std::wstring output(wideLen, L'\0');
        MultiByteToWideChar(
            CP_UTF8,
            0,
            input.data(),
            static_cast<int>(input.size()),
            output.data(),
            wideLen
        );

        return output;
    }

}