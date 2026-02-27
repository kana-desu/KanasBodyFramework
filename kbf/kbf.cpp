#include <kbf/kbf.hpp>

#include <kbf/situation/situation_watcher.hpp>
#include <kbf/debug/debug_stack.hpp>
#include <kbf/cimgui/cimgui_funcs.hpp>

#include <windows.h>
#include <dbghelp.h>
#include <psapi.h>  

#include <iostream>

namespace kbf {

	bool KBF::pluginDisabled = false;

    KBF::KBF() {
        SymInitialize(GetCurrentProcess(), nullptr, TRUE);

        setInitializationTriggers();
    }

    void KBF::setInitializationTriggers() {
        // Do this on a separate thread to avoid freezing the game during the main menu load
        // This is a one time off -> on trigger, so no need to post anything back to the main thread :)
        auto initFn = [&]() {
            // Pre check to avoid unnecessary thread creation
            if (instance.isInitializing() || instance.isInitialized()) return; 
            std::thread([&]() {
                instance.initialize();
            }).detach();
        };

        // Trigger on all situations that are inside of the loaded game loop.
        //  Unfortunately we have to define these manually since c++ has no way of checking whether a certain enum value exists
        //  (i.e. if we wanted to do all but a few 'exceptions')

        SituationWatcher& watcher = SituationWatcher::get();

        watcher.onEnterSituation(isOnline						     , initFn);
        watcher.onEnterSituation(isSoloOnline					     , initFn);
        watcher.onEnterSituation(isOfflineorMainMenu				 , initFn);
        watcher.onEnterSituation(isinQuestPreparing				     , initFn);
        watcher.onEnterSituation(isinQuestReady					     , initFn);
        watcher.onEnterSituation(isinQuestPlayingasHost			     , initFn);
        watcher.onEnterSituation(isinQuestPlayingasGuest			 , initFn);
        watcher.onEnterSituation(isinQuestPlayingfromFieldSurvey	 , initFn);
        watcher.onEnterSituation(DUPLICATE_isinQuestPlayingasGuest   , initFn);
        watcher.onEnterSituation(isinArenaQuestPlayingasHost		 , initFn);
        watcher.onEnterSituation(isinQuestPressSelectToEnd		     , initFn);
        watcher.onEnterSituation(isinQuestEndAnnounce			     , initFn);
        watcher.onEnterSituation(isinQuestResultScreen			     , initFn);
        watcher.onEnterSituation(isinQuestLoadingResult			     , initFn);
        watcher.onEnterSituation(isinLinkPartyAsGuest			     , initFn);
        watcher.onEnterSituation(isinTrainingArea				     , initFn);
        watcher.onEnterSituation(isinJunctionArea				     , initFn);
        watcher.onEnterSituation(isinSuja						     , initFn);
        watcher.onEnterSituation(isinGrandHub					     , initFn);
        watcher.onEnterSituation(DUPLICATE_isinTrainingArea		     , initFn);
        watcher.onEnterSituation(isinBowlingGame					 , initFn);
        watcher.onEnterSituation(isinArmWrestling				     , initFn);
        watcher.onEnterSituation(isatTable						     , initFn);
        //watcher.onEnterSituation(isAlwaysOn						 , initFn); // outside of loaded game loop

        watcher.onEnterSituation(isInMainMenuScene    , initFn);
        watcher.onEnterSituation(isInSaveSelectGUI    , initFn);
        watcher.onEnterSituation(isInCharacterCreator , initFn);
        watcher.onEnterSituation(isInHunterGuildCard  , initFn);
        watcher.onEnterSituation(isInCutscene         , initFn);
        watcher.onEnterSituation(isInGame             , initFn);
        //watcher.onEnterSituation(isInTitleMenus     , initFn); // outside of loaded game loop
    }

    KBF::~KBF() {
		SymCleanup(GetCurrentProcess());
    }

    KBF& KBF::get() {
        static KBF kbf;
        return kbf;
    }

    int KBF::handleException(const char* line, EXCEPTION_POINTERS * ep) {
        KBF::get().condStackTrace(line, ep);
        return EXCEPTION_EXECUTE_HANDLER;
	}

    void KBF::fetch() {
        if (pluginDisabled) return;

        __try {
            get().instance.fetch();
        }
        __except (handleException("fetch", GetExceptionInformation())) {
            pluginDisabled = true;
        }
    }

	void KBF::apply() {
        if (pluginDisabled) return;

        __try {
		    get().instance.apply();
        }
        __except (handleException("apply", GetExceptionInformation())) {
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
            __except (handleException("drawUI", GetExceptionInformation())) {
                pluginDisabled = true;
		    }
        }

    }

    void KBF::condStackTrace(const char* line, EXCEPTION_POINTERS* ep) {
        if (!pluginDisabled) {
            reframework::API::get()->log_error(std::format("KBF Encountered a crash in function: {}. Stack Trace:", line).c_str());
            logStackTrace(ep);
			logKbfDebugLog();
		}
    }

    void KBF::logStackTrace(EXCEPTION_POINTERS* ep) {
        CONTEXT context = *ep->ContextRecord;
        STACKFRAME64 stackFrame = {};
        HANDLE process = GetCurrentProcess();
        HANDLE thread = GetCurrentThread();

        DWORD machineType = IMAGE_FILE_MACHINE_AMD64;

        stackFrame.AddrPC.Offset = context.Rip; // x64
        stackFrame.AddrPC.Mode = AddrModeFlat;
        stackFrame.AddrFrame.Offset = context.Rbp;
        stackFrame.AddrFrame.Mode = AddrModeFlat;
        stackFrame.AddrStack.Offset = context.Rsp;
        stackFrame.AddrStack.Mode = AddrModeFlat;

        while (StackWalk64(machineType, process, thread, &stackFrame, &context, nullptr,
            SymFunctionTableAccess64, SymGetModuleBase64, nullptr)) {
            if (stackFrame.AddrPC.Offset == 0) break;

            HMODULE module = nullptr;
            DWORD64 moduleBase = 0;
            if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                reinterpret_cast<LPCSTR>(stackFrame.AddrPC.Offset),
                &module)) {
                MODULEINFO info = {};
                if (GetModuleInformation(process, module, &info, sizeof(info))) {
                    moduleBase = reinterpret_cast<DWORD64>(info.lpBaseOfDll);
                }
            }

            const char* moduleName = module ? "" : "UnknownModule";
            char moduleFileName[MAX_PATH] = {};
            if (module) GetModuleFileNameA(module, moduleFileName, MAX_PATH);
            if (moduleFileName[0]) moduleName = moduleFileName;

            DWORD64 offset = stackFrame.AddrPC.Offset - moduleBase;

            // Log the current frame
            reframework::API::get()->log_error(
                std::format("Frame:    {} + 0x{:016x}", moduleName, offset).c_str()
            );
        }
    }

    void KBF::logKbfDebugLog() {
        reframework::API::get()->log_error(std::format("KBF Debug Log Start (VERSION={}):", KBF_VERSION).c_str());
		reframework::API::get()->log_error(DEBUG_STACK.string().c_str());
		reframework::API::get()->log_error("KBF Debug Log End");
    }
}