#pragma once

#include <reframework/API.hpp>

#include <kbf/debug/debug_stack.hpp>
#include <kbf/util/re_engine/string_types.hpp>
#include <kbf/util/re_engine/re_object_properties_to_string.hpp>
#include <kbf/util/string/cvt_utf16_utf8.hpp>
#include <kbf/util/string/ptr_to_hex_string.hpp>

#include <string_view>
#include <cstdint>

#define REINVOKE_LOG_TAG "[REInvoke]"
#define ENABLE_REINVOKE_LOGGING
#define REINVOKE_LOGGING_LEVEL_NULL
#define REINVOKE_LOGGING_LEVEL_ERROR
//#define REINVOKE_LOGGING_LEVEL_WARNING

// Screw you, windows api.
#undef ERROR

namespace kbf {

    enum class InvokeReturnType {
        BYTES,
        BOOL,
        BYTE,
        WORD,
        DWORD,
        FLOAT,
        QWORD,
        DOUBLE
    };

    template<typename castType>
    inline castType* REInvokeStaticPtr(
        std::string callerTypeName,
        std::string methodName,
        std::vector<void*> args
    ) {
        reframework::API::TypeDefinition* callerType = reframework::API::get()->tdb()->find_type(callerTypeName);
        #if defined(ENABLE_REINVOKE_LOGGING) && defined(REINVOKE_LOGGING_LEVEL_NULL) && defined(REINVOKE_LOGGING_LEVEL_ERROR)
        if (callerType == nullptr) {
            DEBUG_STACK.push(std::format("Failed to fetch caller type definition: {}", callerTypeName), DebugStack::Color::COL_ERROR);
        }
        #endif
        if (callerType == nullptr) return nullptr;

        reframework::API::Method* callerMethod = callerType->find_method(methodName);
        #if defined(ENABLE_REINVOKE_LOGGING) && defined(REINVOKE_LOGGING_LEVEL_NULL) && defined(REINVOKE_LOGGING_LEVEL_ERROR)
        if (callerMethod == nullptr) {
            DEBUG_STACK.push(std::format("Failed to find method {}. {} has the following properties:\n{}", methodName, callerTypeName, reTypePropertiesToString(callerType)), DebugStack::Color::COL_ERROR);
        }
        #endif
		if (callerMethod == nullptr) return nullptr;
        
        reframework::InvokeRet ret = callerMethod->invoke(nullptr, args);

        #if defined(ENABLE_REINVOKE_LOGGING) && defined(REINVOKE_LOGGING_LEVEL_NULL) && defined(REINVOKE_LOGGING_LEVEL_WARNING)
        if (ret.ptr == nullptr) {
            DEBUG_STACK.push(std::format("{} REInvokePtr: {} returned nullptr", REINVOKE_LOG_TAG, methodName), DebugStack::Color::WARNING);
        }
        #endif

        if (ret.exception_thrown) {
            DEBUG_STACK.push(std::format("{} REInvokePtr: {} threw an exception!", REINVOKE_LOG_TAG, methodName), DebugStack::Color::COL_ERROR);
        }

        return (castType*)(ret.ptr); // I *think* this is ok, but may need (castType*)&ret? ??
	}

    template<typename castType>
    inline castType* REInvokeStatic(
        std::string callerTypeName,
        std::string methodName,
        std::vector<void*> args,
        InvokeReturnType returnType
    ) {
        reframework::API::TypeDefinition* callerType = reframework::API::get()->tdb()->find_type(callerTypeName);
        #if defined(ENABLE_REINVOKE_LOGGING) && defined(REINVOKE_LOGGING_LEVEL_NULL) && defined(REINVOKE_LOGGING_LEVEL_ERROR)
        if (callerType == nullptr) {
            DEBUG_STACK.push(std::format("Failed to fetch caller type definition: {}", callerTypeName), DebugStack::Color::COL_ERROR);
        }
        #endif
        if (callerType == nullptr) return nullptr;

        reframework::API::Method* callerMethod = callerType->find_method(methodName);
        #if defined(ENABLE_REINVOKE_LOGGING) && defined(REINVOKE_LOGGING_LEVEL_NULL) && defined(REINVOKE_LOGGING_LEVEL_ERROR)
        if (callerMethod == nullptr) {
            DEBUG_STACK.push(std::format("Failed to find method {}. {} has the following properties:\n{}", methodName, callerTypeName, reTypePropertiesToString(callerType)), DebugStack::Color::COL_ERROR);
        }
        #endif
		if (callerMethod == nullptr) return nullptr;
        
        reframework::InvokeRet ret = callerMethod->invoke(nullptr, args);

        if (ret.exception_thrown) {
            DEBUG_STACK.push(std::format("{} REInvokeStatic: {} threw an exception!", REINVOKE_LOG_TAG, methodName), DebugStack::Color::COL_ERROR);
        }

        switch (returnType) {
        case InvokeReturnType::BYTES:
            return *reinterpret_cast<castType*>(&ret.bytes);
        case InvokeReturnType::BOOL:
            return *reinterpret_cast<castType*>(&ret.byte);
        case InvokeReturnType::BYTE:
            return *reinterpret_cast<castType*>(&ret.byte);
        case InvokeReturnType::WORD:
            return *reinterpret_cast<castType*>(&ret.word);
        case InvokeReturnType::DWORD:
            return *reinterpret_cast<castType*>(&ret.dword);
        case InvokeReturnType::FLOAT:
            return *reinterpret_cast<castType*>(&ret.f);
        case InvokeReturnType::QWORD:
            return *reinterpret_cast<castType*>(&ret.qword);
        case InvokeReturnType::DOUBLE:
            return *reinterpret_cast<castType*>(&ret.d);
        }

        return *reinterpret_cast<castType*>((void*)0);
    }

    template<typename castType>
    inline castType* REFieldPtr(
        reframework::API::ManagedObject* caller, 
        std::string fieldName,
		bool isValueType = false
    ) {
        // TODO: Sometimes this seems to fail. I think this might be due to not considering an fieldPtr offset, or valueType
        //         (although the reframework api should handle this???). If doesn't work, just read the raw memory ptr for now.

        #if defined(ENABLE_REINVOKE_LOGGING) && defined(REINVOKE_LOGGING_LEVEL_NULL) && defined(REINVOKE_LOGGING_LEVEL_ERROR)
        if (caller->get_type_definition() == nullptr) {
            DEBUG_STACK.push(std::format("Failed to fetch caller type definition for field {}", fieldName), DebugStack::Color::COL_ERROR);
        }
        else if (caller->get_type_definition()->find_field(fieldName) == nullptr) {
            DEBUG_STACK.push(std::format("Failed to find field {}. Caller object has the following fields and methods:\n{}", fieldName, reObjectPropertiesToString(caller)), DebugStack::Color::COL_ERROR);
        }
        #endif

        castType* ret = caller->get_field<castType>(fieldName, isValueType);
        #if defined(ENABLE_REINVOKE_LOGGING) && defined(REINVOKE_LOGGING_LEVEL_NULL) && defined(REINVOKE_LOGGING_LEVEL_WARNING)
        if (ret == nullptr) {
            DEBUG_STACK.push(std::format("{} REField: {} returned nullptr", REINVOKE_LOG_TAG, fieldName), DebugStack::Color::WARNING);
        }
        #endif

        if (ret != nullptr && !isValueType) ret = *(castType**)ret;

        return ret;
	}

    // TODO: I think this can cause a massive memory leak when the length returned is valid, but huge.
    // Unwrap ManagedObject* to std::string
    inline std::string REFieldStr(
        reframework::API::ManagedObject* caller,
        std::string fieldName,
        REStringType stringType,
        bool isValueType = false
    ) {
        void* data = REFieldPtr<void>(caller, fieldName, isValueType);
		if (data == nullptr) return "ERROR: REFieldStr Field Returned Nullptr!";

        //if (!isValueType) data = *(void**)data;

        switch (stringType) {
        case REStringType::SYSTEM_STRING: {
			SystemString retStr = *reinterpret_cast<SystemString*>(data);
            return narrow(get_view(retStr));
        }
        case REStringType::UNMANAGED_STRING: {
            UnmanagedString retStr = *reinterpret_cast<UnmanagedString*>(data);
            return narrow(get_view(retStr));
        }
		default: return "ERROR: REFieldStr Invalid StringType Enum!";
        }

    }

    template<typename castType>
    inline castType REInvoke(
        reframework::API::ManagedObject* caller, 
        std::string methodName,
        std::vector<void*> args,
        InvokeReturnType returnType
    ) {
        #if defined(ENABLE_REINVOKE_LOGGING) && defined(REINVOKE_LOGGING_LEVEL_NULL) && defined(REINVOKE_LOGGING_LEVEL_ERROR)
        if (caller->get_type_definition() == nullptr) {
            DEBUG_STACK.push(std::format("Failed to fetch function type definition for method {}", methodName), DebugStack::Color::COL_ERROR);
        }
        else if (caller->get_type_definition()->find_method(methodName) == nullptr) {
            DEBUG_STACK.push(std::format("Failed to find method {}. Caller object has the following fields and methods:\n{}", methodName, reObjectPropertiesToString(caller)), DebugStack::Color::COL_ERROR);
        }
        #endif

        reframework::InvokeRet ret = caller->invoke(methodName, args);

        if (ret.exception_thrown) {
            DEBUG_STACK.push(std::format("{} REInvoke: {} threw an exception!", REINVOKE_LOG_TAG, methodName), DebugStack::Color::COL_ERROR);
        }

        switch (returnType) {
        case InvokeReturnType::BYTES:
            return *reinterpret_cast<castType*>(&ret.bytes);
        case InvokeReturnType::BOOL:
            return *reinterpret_cast<castType*>(&ret.byte);
        case InvokeReturnType::BYTE:
            return *reinterpret_cast<castType*>(&ret.byte);
        case InvokeReturnType::WORD:
            return *reinterpret_cast<castType*>(&ret.word);
        case InvokeReturnType::DWORD:
            return *reinterpret_cast<castType*>(&ret.dword);
        case InvokeReturnType::FLOAT:
            return *reinterpret_cast<castType*>(&ret.f);
        case InvokeReturnType::QWORD:
            return *reinterpret_cast<castType*>(&ret.qword);
        case InvokeReturnType::DOUBLE:
            return *reinterpret_cast<castType*>(&ret.d);
        }

        return *reinterpret_cast<castType*>((void*)0);
    }

	template<typename castType>
    inline castType* REInvokePtr(
        reframework::API::ManagedObject* caller,
        std::string methodName,
        std::vector<void*> args
    ) {
        #if defined(ENABLE_REINVOKE_LOGGING) && defined(REINVOKE_LOGGING_LEVEL_NULL) && defined(REINVOKE_LOGGING_LEVEL_ERROR)
        if (caller->get_type_definition() == nullptr) {
            DEBUG_STACK.push(std::format("Failed to fetch function type definition for method {}", methodName), DebugStack::Color::COL_ERROR);
        }
        else if (caller->get_type_definition()->find_method(methodName) == nullptr) {
            DEBUG_STACK.push(std::format("Failed to find method {}. Caller object has the following fields and methods:\n{}", methodName, reObjectPropertiesToString(caller)), DebugStack::Color::COL_ERROR);
        }
        #endif

        reframework::InvokeRet ret = caller->invoke(methodName, args);

        #if defined(ENABLE_REINVOKE_LOGGING) && defined(REINVOKE_LOGGING_LEVEL_NULL) && defined(REINVOKE_LOGGING_LEVEL_WARNING)
        if (ret.ptr == nullptr) {
            DEBUG_STACK.push(std::format("{} REInvokePtr: {} returned nullptr", REINVOKE_LOG_TAG, methodName), DebugStack::Color::COL_WARNING);
        }
        #endif

        if (ret.exception_thrown) {
            DEBUG_STACK.push(std::format("{} REInvokePtr: {} threw an exception!", REINVOKE_LOG_TAG, methodName), DebugStack::Color::COL_ERROR);
        }

        return (castType*)(ret.ptr); // I *think* this is ok, but may need (castType*)&ret? ??
	}

    // Unwrap ManagedObject* to std::string
    inline std::string REInvokeStr(
        reframework::API::ManagedObject* caller,
        std::string methodName,
        std::vector<void*> args
    ) {
		reframework::API::ManagedObject* ret = REInvokePtr<reframework::API::ManagedObject>(caller, methodName, args);
		if (ret == nullptr) return "ERROR: REInvokeStr Returned NULLPTR!";

        /*
		Note that the layout of a managed string in Il2Cpp is as follows (I think):

            [0x00] Il2CppClass* klass
            [0x08] MonitorData* monitor
            [0x10] int32_t m_stringLength
            [0x14] char16_t m_firstChar
                   char16_t char[1..256]

        */

        int32_t length = *(int32_t*)((uint8_t*)ret + /*offset to m_stringLength*/ 0x10);
        const char16_t* chars = (const char16_t*)((uint8_t*)ret + /*offset to firstChar*/ 0x14);
        if (!chars || length <= 0) return {};

        // Convert UTF-16 -> UTF-8
        std::u16string utf16(chars, chars + length);
		return cvt_utf16_to_utf8(utf16);
    }

    inline void REInvokeVoid(
        reframework::API::ManagedObject* caller,
        std::string methodName,
        std::vector<void*> args
    ) {
        #if defined(ENABLE_REINVOKE_LOGGING) && defined(REINVOKE_LOGGING_LEVEL_NULL) && defined(REINVOKE_LOGGING_LEVEL_ERROR)
        if (caller->get_type_definition() == nullptr) {
            DEBUG_STACK.push(std::format("Failed to fetch function type definition for method {}", methodName), DebugStack::Color::COL_ERROR);
        }
        else if (caller->get_type_definition()->find_method(methodName) == nullptr) {
            DEBUG_STACK.push(std::format("Failed to find method {}. Caller object has the following fields and methods:\n{}", methodName, reObjectPropertiesToString(caller)), DebugStack::Color::COL_ERROR);
        }
        #endif

        reframework::InvokeRet ret = caller->invoke(methodName, args);

        if (ret.exception_thrown) {
            DEBUG_STACK.push(std::format("{} REInvokeVoid: {} threw an exception!", REINVOKE_LOG_TAG, methodName), DebugStack::Color::COL_ERROR);
        }
	}

}