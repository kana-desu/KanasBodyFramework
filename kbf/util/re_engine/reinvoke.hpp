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

using REApi = reframework::API;

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
            DEBUG_STACK.push(std::format("{} REInvokePtr: {} threw an exception!", REINVOKE_LOG_TAG, methodName), DebugStack::Color::COL_DEBUG);
        }

        return (castType*)(ret.ptr); // I *think* this is ok, but may need (castType*)&ret? ??
	}

    template<typename castType>
    inline castType REInvokeStatic(
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
        if (callerType == nullptr) return castType{};

        reframework::API::Method* callerMethod = callerType->find_method(methodName);
        #if defined(ENABLE_REINVOKE_LOGGING) && defined(REINVOKE_LOGGING_LEVEL_NULL) && defined(REINVOKE_LOGGING_LEVEL_ERROR)
        if (callerMethod == nullptr) {
            DEBUG_STACK.push(std::format("Failed to find method {}. {} has the following properties:\n{}", methodName, callerTypeName, reTypePropertiesToString(callerType)), DebugStack::Color::COL_ERROR);
        }
        #endif
        if (callerMethod == nullptr) return castType{};
        
        reframework::InvokeRet ret = callerMethod->invoke(nullptr, args);

        if (ret.exception_thrown) {
            DEBUG_STACK.push(std::format("{} REInvokeStatic: {} threw an exception!", REINVOKE_LOG_TAG, methodName), DebugStack::Color::COL_DEBUG);
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

        return castType{};
    }

    template<typename castType>
    inline castType* REFieldPtr(
        reframework::API::ManagedObject* caller, 
        std::string fieldName
    ) {
        REApi::TypeDefinition* callerTypeDef = caller->get_type_definition();
        if (callerTypeDef == nullptr) {
            DEBUG_STACK.push(std::format("Failed to fetch caller type definition for field {}", fieldName), DebugStack::Color::COL_ERROR);
            return nullptr;
        }

		bool isValueType_caller = callerTypeDef->is_valuetype();
        bool callerIsManagedObj = caller->is_managed_object();

		REApi::Field* field = callerTypeDef->find_field(fieldName);
        if (field == nullptr) {
            DEBUG_STACK.push(std::format("Failed to find field {}. Caller object has the following fields and methods:\n{}", fieldName, reObjectPropertiesToString(caller)), DebugStack::Color::COL_ERROR);
            return nullptr;
        }

		REApi::TypeDefinition* fieldTypeDef = field->get_type();
		if (fieldTypeDef == nullptr) {
            DEBUG_STACK.push(std::format("Failed to fetch field type definition for field {}", fieldName), DebugStack::Color::COL_ERROR);
            return nullptr;
        }

		// Not sure if the managed object check is necessary here, but better safe than sorry
        uint32_t offset = field->get_offset_from_fieldptr();
		if (!isValueType_caller || callerIsManagedObj) offset += callerTypeDef->get_fieldptr_offset();

        bool isValueType_test = fieldTypeDef->is_valuetype();

		castType* data = (castType*)((uintptr_t)caller + offset);

        if (isValueType_test) return data;
		return *(castType**)data;
	}

    // TODO: I think this can cause a massive memory leak when the length returned is valid, but huge.
    // Unwrap ManagedObject* to std::string
    inline std::string REFieldStr(
        reframework::API::ManagedObject* caller,
        std::string fieldName,
        REStringType stringType
    ) {
        void* data = REFieldPtr<void>(caller, fieldName);
		if (data == nullptr) return "ERROR: REFieldStr Field Returned Nullptr!";

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
            DEBUG_STACK.push(std::format("Failed to fetch caller type definition for method {}", methodName), DebugStack::Color::COL_ERROR);
        }
        else if (caller->get_type_definition()->find_method(methodName) == nullptr) {
            DEBUG_STACK.push(std::format("Failed to find method {}. Caller object has the following fields and methods:\n{}", methodName, reObjectPropertiesToString(caller)), DebugStack::Color::COL_ERROR);
        }
        #endif

        reframework::InvokeRet ret = caller->invoke(methodName, args);

        if (ret.exception_thrown) {
            DEBUG_STACK.push(std::format("{} REInvoke: {} threw an exception!", REINVOKE_LOG_TAG, methodName), DebugStack::Color::COL_DEBUG);
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
            DEBUG_STACK.push(std::format("Failed to fetch caller type definition for method {}", methodName), DebugStack::Color::COL_ERROR);
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
            DEBUG_STACK.push(std::format("{} REInvokePtr: {} threw an exception!", REINVOKE_LOG_TAG, methodName), DebugStack::Color::COL_DEBUG);
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

    inline std::string REInvokeStaticStr(
        std::string callerTypeName,
        std::string methodName,
        std::vector<void*> args
    ) {
        REApi::Method* fn = REApi::get()->tdb()->find_method(callerTypeName, methodName);
        if (fn == nullptr) {
            DEBUG_STACK.push(std::format("{} REInvokeStaticStr: Failed to find method {}::{}", REINVOKE_LOG_TAG, callerTypeName, methodName), DebugStack::Color::COL_ERROR);
			return "ERR: Null Method!";
        }

        reframework::InvokeRet ret = fn->invoke(nullptr, args);

        REApi::ManagedObject* managedStr = (REApi::ManagedObject*)ret.ptr;
        if (managedStr == nullptr) {
            DEBUG_STACK.push(std::format("{} Return value was nullptr for call: {}::{}", REINVOKE_LOG_TAG, callerTypeName, methodName), DebugStack::Color::COL_WARNING);
            return "ERR: Null ManagedStr!";
        }

        int32_t length = *(int32_t*)((uint8_t*)managedStr + /*offset to m_stringLength*/ 0x10);
        const char16_t* chars = (const char16_t*)((uint8_t*)managedStr + /*offset to firstChar*/ 0x14);
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
            DEBUG_STACK.push(std::format("Failed to fetch caller type definition for method {}", methodName), DebugStack::Color::COL_ERROR);
        }
        else if (caller->get_type_definition()->find_method(methodName) == nullptr) {
            DEBUG_STACK.push(std::format("Failed to find method {}. Caller object has the following fields and methods:\n{}", methodName, reObjectPropertiesToString(caller)), DebugStack::Color::COL_ERROR);
        }
        #endif

        reframework::InvokeRet ret = caller->invoke(methodName, args);

        if (ret.exception_thrown) {
            DEBUG_STACK.push(std::format("{} REInvokeVoid: {} threw an exception!", REINVOKE_LOG_TAG, methodName), DebugStack::Color::COL_DEBUG);
        }
	}

    inline int REEnum(
        reframework::API::TypeDefinition* typeDef,
        std::string name,
        bool& success
    ) {
        if (typeDef == nullptr) {
            DEBUG_STACK.push(std::format("Recieved null type definition for enum value @ {}", name), DebugStack::Color::COL_ERROR);
            success = false;
            return 0;
        }

        bool isEnumType = typeDef->is_enum();
        if (!isEnumType) {
            DEBUG_STACK.push(std::format("Attempted to fetch enum {} from {}, but the type is not an enum", name, typeDef->get_full_name()), DebugStack::Color::COL_ERROR);
            success = false;
            return 0;
		}

        REApi::Field* field = typeDef->find_field(name);
        if (field == nullptr) {
            DEBUG_STACK.push(std::format("Failed to find enum value {} for enum {}", name, typeDef->get_full_name()), DebugStack::Color::COL_ERROR);
            success = false;
            return 0;
        }

        void* data = field->get_init_data();
        int castData = 0;

        switch (typeDef->get_underlying_type()->get_valuetype_size()) {
        case 1: { castData = static_cast<int>(*(int8_t*)data);  break; }
        case 2: { castData = static_cast<int>(*(int16_t*)data); break; }
        case 4: { castData = static_cast<int>(*(int32_t*)data); break; }
        case 8: { castData = static_cast<int>(*(int64_t*)data); break; }
        default: DEBUG_STACK.push(std::format("Enum value {} of {} has unsupported underlying data size, it will be read as 0.", name, typeDef->get_full_name()), DebugStack::Color::COL_WARNING);
        }

        success = true;
        return castData;
    }


}