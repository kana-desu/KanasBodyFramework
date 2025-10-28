#pragma once

#include <kbf/util/re_engine/re_object.hpp>
#include <cstdint>
#include <string_view>

namespace kbf {

	enum class REStringType {
		UNMANAGED_STRING,
		SYSTEM_STRING  // so called "managed string"
	};

	class SystemString : public REManagedObject
	{
	public:
		int32_t size; //0x0010
		wchar_t data[256]; //0x0014
	}; //Size: 0x0214
    static_assert(sizeof(SystemString) == 0x214);

	class UnmanagedString
	{
	public:
		char pad_0000[24]; //0x0000
		int32_t length; //0x0018 if len >= 12, is a pointer
		int32_t maxLength; //0x001C
	}; //Size: 0x0020
    static_assert(sizeof(UnmanagedString) == 0x0020);

    inline std::wstring_view get_view(const UnmanagedString& str) {
        if (&str == nullptr) {
            return L"";
        }

        auto length = str.length;

        if (length <= 0) {
            return L"";
        }

        std::wstring_view raw_name;

        if (length >= 12) {
            auto name_ptr = *(wchar_t**)&str;

            if (name_ptr == nullptr) {
                return L"";
            }

            raw_name = name_ptr;
        }
        else {
            if (length <= 0) {
                return L"";
            }

            raw_name = (wchar_t*)&str;
        }

        return raw_name;
    }

    inline std::wstring_view get_view(const SystemString& str) {
        if (&str == nullptr) {
            return L"";
        }

        return std::wstring_view{ str.data };
    }

}