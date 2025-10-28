#pragma once

#include <stdexcept>

#define INVOKE_REQUIRED_CALLBACK(callback, ...) (callback) ? (callback)(__VA_ARGS__) : throw std::runtime_error("Required callback \"" #callback "\" not set.")
#define INVOKE_OPTIONAL_CALLBACK(callback, ...) (callback) ? (callback)(__VA_ARGS__) : void()
#define INVOKE_OPTIONAL_CALLBACK_TYPED(type, defaultRet, callback, ...) (callback) ? static_cast<type>((callback)(__VA_ARGS__)) : type(defaultRet)