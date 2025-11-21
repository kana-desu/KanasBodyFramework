#pragma once

#include <kbf/util/re_engine/reinvoke.hpp>

#include <reframework/API.hpp>

using REApi = reframework::API;

namespace kbf {

	// Recursive helper if find(System.String) fails
	static inline REApi::ManagedObject* searchTransforms(
		REApi::ManagedObject* node,
		const std::string& targetName,
		size_t depth, size_t maxDepth,
		size_t breadth, size_t maxBreadth
	) {
		if (!node) return nullptr;
		if (depth > maxDepth) return nullptr;
		if (breadth > maxBreadth) return nullptr;

		// Add this node's name
		REApi::ManagedObject* gameObject = REInvokePtr<REApi::ManagedObject>(node, "get_GameObject", {});
		if (gameObject == nullptr) return nullptr;

		std::string name = REInvokeStr(gameObject, "get_Name", {});
		DEBUG_STACK.push(std::format("Searching Transforms: {} | [{}]-[{}] | {}", targetName, depth, breadth, name), DebugStack::Color::COL_DEBUG);
		if (name == targetName) return node;

		// Recurse into siblings (More likely to find it breadth-wise first)
		REApi::ManagedObject* siblingRet = searchTransforms(
			REInvokePtr<REApi::ManagedObject>(node, "get_Next", {}), 
			targetName, 
			depth, maxDepth, 
			breadth + 1, maxBreadth);
		if (siblingRet != nullptr) return siblingRet;

		// Recurse into children
		return searchTransforms(
			REInvokePtr<REApi::ManagedObject>(node, "get_Child", {}),
			targetName, 
			depth + 1, maxDepth,
			0, maxBreadth);
	}

	inline REApi::ManagedObject* findTransform(
		REApi::ManagedObject* rootTransform, 
		const std::string& name
	) {
		if (rootTransform == nullptr) return nullptr;

		// Convert to managed string
		REApi::ManagedObject* param = REApi::get()->create_managed_string_normal(name.c_str());

		// Call the "find(System.String)" method
		REApi::ManagedObject* ptr = REInvokePtr<REApi::ManagedObject>(
			rootTransform,
			"find(System.String)",
			{ (void*)param }
		);
		return ptr;
	}


}