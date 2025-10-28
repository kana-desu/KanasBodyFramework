#pragma once

#include <reframework/API.hpp>

#include <kbf/debug/debug_stack.hpp>

#include <string>
#include <sstream>

namespace kbf {

    inline std::string reObjectPropertiesToString(reframework::API::ManagedObject* obj) {
        if (!obj) {
            return "[DUMP ERROR] Object is null.";
        }

        auto klass = obj->get_type_definition();
        if (!klass) {
            return "[DUMP ERROR] Could not get type definition.";
        }

        std::ostringstream oss;

        // Fields
        oss << "[Fields]\n";
        if (klass->get_num_fields() == 0) {
            oss << "  No Fields Found\n";
        }
        else {
            for (auto f : klass->get_fields()) {
                if (!f) continue;
                oss << "  " << f->get_name()
                    << " : " << f->get_type()->get_full_name()
                    << "\n";
            }
        }

        // Methods
        oss << "[Methods]\n";
        if (klass->get_num_methods() == 0) {
            oss << "  No Methods Found\n";
        }
        else {
            for (auto m : klass->get_methods()) {
                if (!m) continue;
                oss << "  " << m->get_name() << "(";

                if (m->get_num_params() > 0) {
                    // Print parameter types
                    auto params = m->get_params();
                    for (size_t i = 0; i < params.size(); ++i) {

                        const auto param_type = (reframework::API::TypeDefinition*)params[i].t;

                        auto param_type_name = param_type->get_full_name();

                        // Getting param names seems very difficult unless building the entire SDK alongside this... so, just use a placeholder
                        oss << param_type_name;
                        if (i + 1 < params.size()) oss << ", ";
                    }
                }

				auto retType = m->get_return_type();
                if (retType) {
                    oss << ") -> " << retType->get_full_name() << "\n";
                } else {
                    oss << ") -> void\n";
				}
            }
        }

        return oss.str();
    }

    inline std::string reTypePropertiesToString(reframework::API::TypeDefinition* typeDef) {
        auto klass = typeDef;
        if (!klass) {
            return "[DUMP ERROR] Could not get type definition.";
        }

        std::ostringstream oss;

        // Fields
        oss << "[Fields]\n";
        if (klass->get_num_fields() == 0) {
            oss << "  No Fields Found\n";
        }
        else {
            for (auto f : klass->get_fields()) {
                if (!f) continue;
                oss << "  " << f->get_name()
                    << " : " << f->get_type()->get_full_name()
                    << "\n";
            }
        }

        // Methods
        oss << "[Methods]\n";
        if (klass->get_num_methods() == 0) {
            oss << "  No Methods Found\n";
        }
        else {
            for (auto m : klass->get_methods()) {
                if (!m) continue;
                oss << "  " << m->get_name() << "(";

                if (m->get_num_params() > 0) {
                    // Print parameter types
                    auto params = m->get_params();
                    for (size_t i = 0; i < params.size(); ++i) {

                        const auto param_type = (reframework::API::TypeDefinition*)params[i].t;

                        auto param_type_name = param_type->get_full_name();

                        // Getting param names seems very difficult unless building the entire SDK alongside this... so, just use a placeholder
                        oss << param_type_name;
                        if (i + 1 < params.size()) oss << ", ";
                    }
                }

                auto retType = m->get_return_type();
                if (retType) {
                    oss << ") -> " << retType->get_full_name() << "\n";
                }
                else {
                    oss << ") -> void\n";
                }
            }
        }

        return oss.str();
    }

}