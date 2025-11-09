#include <kbf/kbf.hpp>

#include <kbf/situation/situation_watcher.hpp>
#include <kbf/debug/debug_stack.hpp>
#include <kbf/cimgui/cimgui_funcs.hpp>

#include <windows.h>
#include <psapi.h>

#include <iostream>

namespace kbf {

	bool KBF::pluginDisabled = false;

    KBF::KBF() {
        instance.initialize();
    }

    KBF& KBF::get() {
        static KBF kbf;
        return kbf;
    }

    void KBF::onPreUpdateMotion() {
        if (pluginDisabled) return;

        __try {
            get().instance.onPreUpdateMotion();
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
			condStackTrace("onPreUpdateMotion");
            pluginDisabled = true;
        }
    }

    void KBF::onPostUpdateMotion() { }

	void KBF::onPostLateUpdateBehavior() {
        if (pluginDisabled) return;

        __try {
		    get().instance.onPostLateUpdateBehavior();
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
			condStackTrace("onPostLateUpdateBehavior");
            pluginDisabled = true;
        }
    }

    void KBF::drawUI() {
        if (pluginDisabled) {
            if (reframework::API::get()->reframework()->is_drawing_ui()) {
                instance.drawDisabled();
            }
        }
        else {
            __try {
                if (reframework::API::get()->reframework()->is_drawing_ui()) {
                    instance.draw();
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
			    condStackTrace("drawUI");
                pluginDisabled = true;
		    }
        }

    }

    void KBF::condStackTrace(const char* line) {
        if (!pluginDisabled) {
            reframework::API::get()->log_error(std::format("KBF Encountered a crash in function: {}. Stack Trace:", line).c_str());
            logStackTrace();
			logKbfDebugLog();
		}
    }

    void KBF::logStackTrace() {
        void* stack[62];
        unsigned short frames = CaptureStackBackTrace(0, 62, stack, nullptr);


        for (unsigned short i = 0; i < frames; i++) {
            HMODULE module = nullptr;
            DWORD64 moduleBase = 0;

            // get the module handle for this address
            if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                reinterpret_cast<LPCSTR>(stack[i]), &module)) {
                MODULEINFO info = {};
                if (GetModuleInformation(GetCurrentProcess(), module, &info, sizeof(info))) {
                    moduleBase = reinterpret_cast<DWORD64>(info.lpBaseOfDll);
                }
            }

            const char* moduleName = module ? "" : "UnknownModule";

            char moduleFileName[MAX_PATH] = {};
            if (module) GetModuleFileNameA(module, moduleFileName, MAX_PATH);
            if (moduleFileName[0]) moduleName = moduleFileName;

            DWORD64 offset = reinterpret_cast<DWORD64>(stack[i]) - moduleBase;

            reframework::API::get()->log_error(std::format("Frame {}: {} + 0x{:016x}", i, moduleName, offset).c_str());
        }
    }

    void KBF::logKbfDebugLog() {
        reframework::API::get()->log_error("KBF Debug Log Start:");
		reframework::API::get()->log_error(DEBUG_STACK.string().c_str());
		reframework::API::get()->log_error("KBF Debug Log End");
    }
}