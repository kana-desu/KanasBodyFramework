#pragma once 

#include <kbf/util/re_engine/reinvoke.hpp>
#include <kbf/util/re_engine/re_memory_ptr.hpp>

#include <unordered_map>

namespace kbf {

	// Returns Game Object -> app.MeshSetting
	// Pass this function the address of app.MeshManager :: _MeshSettingRegister :: _ObjectArray
	std::unordered_map<REApi::ManagedObject*, REApi::ManagedObject*> dumpVisibleMeshes(uintptr_t objectArrayAddress, bool forceAll) {
		REApi::ManagedObject* objectArray = reinterpret_cast<REApi::ManagedObject*>(objectArrayAddress);

		//int32_t count = REInvoke<int32_t>(meshSettingRegister, "get_Capacity", {}, InvokeReturnType::DWORD);
		std::unordered_map<REApi::ManagedObject*, REApi::ManagedObject*> map{};

		for (int32_t i = 0; i < 8000; i++) {
			REApi::ManagedObject* meshSetting = REInvokePtr<REApi::ManagedObject>(objectArray, "get_Item(System.Int32)", { (void*)i });
			if (!meshSetting) continue;

			bool visible = forceAll || REInvoke<bool>(meshSetting, "get_IsVisible()", {}, InvokeReturnType::BOOL); //
			if (!visible) continue;

			REApi::ManagedObject* gameObj = REInvokePtr<REApi::ManagedObject>(meshSetting, "get_GameObject", {});
			if (!gameObj) continue;

			map.emplace(gameObj, meshSetting);
		}	

		return map;
	}

}