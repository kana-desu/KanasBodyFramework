#pragma once

#include <kbf/debug/debug_stack.hpp>
#include <reframework/API.hpp>

#define RE_SINGLETON_LOG_TAG "[RESingleton]"
#define RE_NATIVE_SINGLETON_LOG_TAG "[RENativeSingleton]"

namespace kbf {

    class RESingleton {
    public:
        explicit RESingleton(std::string name, bool native = false)
            : m_name{ std::move(name) },
            m_typedef{ reframework::API::get()->tdb()->find_type(m_name) } 
        {}

        reframework::API::ManagedObject* operator->() {
            if (!ensureInstance()) {
                throw std::runtime_error(std::format(
                    "{} Attempted to dereference null singleton '{}' (-> operator)",
                    RE_SINGLETON_LOG_TAG, m_name
                ));
            }
            return m_instance;
        }

        reframework::API::ManagedObject& operator*() {
            if (!ensureInstance()) {
                throw std::runtime_error(std::format(
                    "{} Attempted to dereference null singleton '{}' (* operator)",
                    RE_SINGLETON_LOG_TAG, m_name
                ));
            }
            return *m_instance;
        }

        reframework::API::ManagedObject* get() {
            ensureInstance();
            return m_instance;
        }

    private:
        bool ensureInstance() {
            if (m_instance == nullptr) {
                m_instance = reframework::API::get()->get_managed_singleton(m_name.c_str());
                if (m_instance == nullptr) return false;

                if (!checkREPtrValidity(m_instance, m_typedef)) {
                    DEBUG_STACK.push(std::format(
                        "{} Failed to get singleton instance for {}",
                        RE_SINGLETON_LOG_TAG, m_name
                    ));

                    m_instance = nullptr;
                    return false;
                }
            }

            return true;
        }

    private:
        std::string m_name;
        reframework::API::ManagedObject* m_instance = nullptr;
        reframework::API::TypeDefinition* m_typedef = nullptr;
    };

    class RENativeSingleton {
    public:
        explicit RENativeSingleton(std::string name)
            : m_name(std::move(name)) {}

        void* get() {
            ensureInstance();
            return m_instance;
        }

    private:
        bool ensureInstance() {
            if (m_instance == nullptr) {
                m_instance = reframework::API::get()->get_native_singleton(m_name.c_str());

                if (m_instance == nullptr) {
                    DEBUG_STACK.push(std::format(
                        "{} Failed to get native singleton instance for {}",
                        RE_NATIVE_SINGLETON_LOG_TAG, m_name
                    ));
                    return false;
                }
            }
            return true;
        }

    private:
        std::string m_name;
        void* m_instance = nullptr;
    };

}