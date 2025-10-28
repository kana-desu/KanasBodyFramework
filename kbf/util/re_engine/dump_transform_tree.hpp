#pragma once

#include <reframework/API.hpp>
#include <kbf/util/re_engine/reinvoke.hpp>

namespace kbf {

    // Recursive helper
    static inline void collectNames(
        reframework::API::ManagedObject* node, 
        std::vector<std::string>& out, 
        size_t depth, size_t maxDepth,
        size_t breadth, size_t maxBreadth
    ) {
        if (!node) return;
		if (depth > maxDepth) return;
		if (breadth > maxBreadth) return;

        // Add this node's name
		reframework::API::ManagedObject* gameObject = REInvokePtr<reframework::API::ManagedObject>(node, "get_GameObject", {});
		if (gameObject == nullptr) return;

		std::string name = REInvokeStr(gameObject, "get_Name", {});
        if (!name.empty()) out.emplace_back(name);

        // Recurse into children
        collectNames(REInvokePtr<reframework::API::ManagedObject>(node, "get_Child", {}), out, depth + 1, maxDepth, 0, maxBreadth);

        // Recurse into siblings
        collectNames(REInvokePtr<reframework::API::ManagedObject>(node, "get_Next", {}), out, depth, maxDepth, breadth + 1, maxBreadth);
    }

    // Public API
    inline std::vector<std::string> dumpTransformTree(reframework::API::ManagedObject* root, size_t maxDepth = 20, size_t maxBreadth = 100) {
        std::vector<std::string> result;
        collectNames(root, result, 0, maxDepth, 0, maxBreadth);
        return result;
    }

    // Recursive helper
    static inline void collectTreeString(
        reframework::API::ManagedObject* node,
        std::string& out,
        std::string prefix,  // keeps track of tree drawing characters
        bool isLast,         // whether this node is the last child
        size_t depth, size_t maxDepth,
        size_t breadth, size_t maxBreadth
    ) {
        if (!node) return;
        if (depth > maxDepth) return;
        if (breadth > maxBreadth) return;

        // Add this node's name
        reframework::API::ManagedObject* gameObject = REInvokePtr<reframework::API::ManagedObject>(node, "get_GameObject", {});
        if (gameObject == nullptr) return;

        std::string name = REInvokeStr(gameObject, "get_Name", {});
        if (!name.empty()) {
            out.append(prefix);
            out.append(isLast ? "+-- " : "+-- ");
            out.append(name);
            out.push_back('\n');
        }

        // Get first child
        auto* child = REInvokePtr<reframework::API::ManagedObject>(node, "get_Child", {});
        size_t childIndex = 0;

        while (child != nullptr && childIndex <= maxBreadth) {
            // Determine if this is the last sibling
            auto* next = REInvokePtr<reframework::API::ManagedObject>(child, "get_Next", {});
            bool childIsLast = (next == nullptr) || (childIndex + 1 > maxBreadth);

            // Extend prefix depending on whether this node was last
            std::string newPrefix = prefix + (isLast ? "    " : "|   ");

            collectTreeString(child, out, newPrefix, childIsLast, depth + 1, maxDepth, 0, maxBreadth);

            child = next;
            ++childIndex;
        }
    }

    // Public API
    inline std::string dumpTransformTreeString(
        reframework::API::ManagedObject* root,
        size_t maxDepth = 20, size_t maxBreadth = 100
    ) {
        std::string result;
        collectTreeString(root, result, "", true, 0, maxDepth, 0, maxBreadth);
        return result;
    }

}