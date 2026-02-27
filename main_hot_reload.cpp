#include "plugin.hpp"
#include <kbf/entry_points.hpp>

#include <kbf/debug/log_string.hpp>
#include <kbf/util/io/kbf_asset_path.hpp>

#include <kbf/watchers/kbf_dll_update_listener.hpp>
#include <kbf/watchers/fs_watcher_win.hpp>

#include <Windows.h>

#include <memory>

static const std::filesystem::path hotReloadDirRelative{ KBF_ASSET_PATH("KBF/HotReload") };
static const std::filesystem::path hotReloadDirAbsolute{ std::filesystem::absolute(hotReloadDirRelative) };
static const std::filesystem::path hotReloadDllPathRelative{ hotReloadDirRelative / LOGIC_DLL_NAME };
static const std::filesystem::path hotReloadDllPathAbsolute{ std::filesystem::absolute(hotReloadDllPathRelative) };
static const std::filesystem::path dllPathRelative{ KBF_ASSET_PATH("KBF/") LOGIC_DLL_NAME };
static const std::filesystem::path dllPathAbsolute{ std::filesystem::absolute(dllPathRelative) };

static std::unique_ptr<kbf::KbfDllUpdateListener> dllUpdateListener = std::make_unique<kbf::KbfDllUpdateListener>(hotReloadDllPathAbsolute);
static kbf::watchers::FsWatcherWin fsWatcher{ hotReloadDirAbsolute.string(), dllUpdateListener.get(), false};

static HMODULE g_logicDll;
static const REFrameworkPluginInitializeParam* g_param;

// Hooks
static void (*kbfUnloadFn)()  = nullptr;
static void (*kbfDrawUIFn)() = nullptr;
static void (*kbfFetchFn)()  = nullptr;
static void (*kbfApplyFn)()  = nullptr;
static void kbfDrawUI(REFImGuiFrameCbData* data) { if(g_logicDll && kbfDrawUIFn) kbfDrawUIFn(); }
static void kbfFetch() { if (g_logicDll && kbfFetchFn) kbfFetchFn(); }
static void kbfApply() { if (g_logicDll && kbfApplyFn) kbfApplyFn(); }

static bool copyHotReloadableDll() {
    // Create HotReload directory if it doesn't exist
    if (!std::filesystem::exists(hotReloadDirAbsolute)) {
        if (!std::filesystem::create_directories(hotReloadDirAbsolute)) {
            reframework::API::get()->log_error(LOG_STRING("Failed to create HotReload directory: {}"), hotReloadDirAbsolute.string());
            return false;
        }
    }

    // If there's a target DLL in the HotReload directory, copy it to the expected dll location, while freeing current library if loaded
    if (std::filesystem::exists(hotReloadDllPathAbsolute)) {
        bool reloaded = false;
        if (g_logicDll) {
            if (kbfUnloadFn) kbfUnloadFn();
            FreeLibrary(g_logicDll);
            g_logicDll = nullptr;
        }
        try {
            std::filesystem::copy_file(hotReloadDllPathAbsolute, dllPathAbsolute, std::filesystem::copy_options::overwrite_existing);
            reframework::API::get()->log_info(std::format("{} Copied hot-reloadable DLL to: {}", LOG_STRING_PREFIX, dllPathAbsolute.string()).c_str());

            // copy the pdb if it exists too
            const std::filesystem::path expectedHotReloadPdbPath = hotReloadDirAbsolute / (LOGIC_DLL_NAME ".pdb");
            if (std::filesystem::exists(expectedHotReloadPdbPath)) {
                std::filesystem::copy_file(expectedHotReloadPdbPath, dllPathAbsolute.parent_path() / (LOGIC_DLL_NAME ".pdb"), std::filesystem::copy_options::overwrite_existing);
			}

            reloaded = true;
        } catch (const std::exception& e) {
            reframework::API::get()->log_error(std::format("{} Failed to copy hot-reloadable DLL: {}", e.what(), LOG_STRING_PREFIX).c_str());
            return false;
        }
        
        return reloaded;
	}

    return false;
}

static void loadLogicDll(const REFrameworkPluginInitializeParam* param) {
    // Somestimes the file write wont be done quickly enough... we can afford to wait ~1s
    int attempts = 20;
    while (attempts--) {
	    g_logicDll = LoadLibraryW(dllPathAbsolute.c_str());
        if (g_logicDll) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    if (!g_logicDll) return reframework::API::get()->log_error(LOG_STRING("Failed to load KBF logic DLL: {}"), dllPathAbsolute.string());

    kbfDrawUIFn = reinterpret_cast<void(*)()>(GetProcAddress(g_logicDll, "kbf_draw_ui"));
    if (!kbfDrawUIFn) return reframework::API::get()->log_error(LOG_STRING("Failed to get kbf_draw_ui function from KBF logic DLL."));

    kbfFetchFn = reinterpret_cast<void(*)()>(GetProcAddress(g_logicDll, "kbf_fetch"));
    if (!kbfFetchFn) return reframework::API::get()->log_error(LOG_STRING("Failed to get kbf_fetch function from KBF logic DLL."));

	kbfApplyFn = reinterpret_cast<void(*)()>(GetProcAddress(g_logicDll, "kbf_apply"));
	if (!kbfApplyFn) return reframework::API::get()->log_error(LOG_STRING("Failed to get kbf_apply function from KBF logic DLL."));

	kbfUnloadFn = reinterpret_cast<void(*)()>(GetProcAddress(g_logicDll, "kbf_unload"));
	if (!kbfUnloadFn) return reframework::API::get()->log_error(LOG_STRING("Failed to get kbf_unload function from KBF logic DLL."));

	auto initializeFn = reinterpret_cast<void(*)()>(GetProcAddress(g_logicDll, "initialize_kbf"));
    if (!initializeFn) return reframework::API::get()->log_error(LOG_STRING("Failed to get initialize_kbf function from KBF logic DLL."));
    initializeFn();

	auto forceInitReframeworkFn = reinterpret_cast<void(*)(const REFrameworkPluginInitializeParam*)>(GetProcAddress(g_logicDll, "kbf_force_initialize_reframework"));
	if (!forceInitReframeworkFn) return reframework::API::get()->log_error(LOG_STRING("Failed to get kbf_force_initialize_reframework function from KBF logic DLL."));
    forceInitReframeworkFn(param);

    reframework::API::get()->log_info(LOG_STRING("KBF logic DLL loaded successfully."));
}

static bool g_doHotReload = false;
static void processHotReload() {
    if (g_doHotReload) {
        reframework::API::get()->log_info(LOG_STRING("Detected Hot-Reload, attempting to switch..."));
        if (copyHotReloadableDll()) {
            loadLogicDll(g_param);
		}
        g_doHotReload = false;
    }
}

extern "C" {

    RE_EXPORT void reframework_plugin_required_version(REFrameworkPluginVersion* version) {
        version->major = REFRAMEWORK_PLUGIN_VERSION_MAJOR;
        version->minor = REFRAMEWORK_PLUGIN_VERSION_MINOR;
        version->patch = REFRAMEWORK_PLUGIN_VERSION_PATCH;
        version->game_name = "MHWILDS";
    }

    RE_EXPORT bool reframework_plugin_initialize(const REFrameworkPluginInitializeParam* param) {
        g_param = param;
        reframework::API::initialize(param);
        reframework::API::get()->log_info(LOG_STRING("Initializing..."));

        // try to copy on first time load
        copyHotReloadableDll();
        loadLogicDll(param);

        fsWatcher.watch();
        dllUpdateListener->onHotReload([]() {
            reframework::API::get()->log_info(LOG_STRING("Detected Hot-Reload, attempting to switch..."));
            g_doHotReload = true;
		});

        try {
            const REFrameworkPluginFunctions* functions = param->functions;

            // Don't bind these functions through KBF's entry points as they should never be touched
            functions->on_imgui_draw_ui(kbfDrawUI);
            functions->on_post_application_entry("EndRendering", processHotReload);

            // This loop hooks EVERY entry point so that we have compile-time function definitions that we can hook into at runtime.
            //  This lets us choose where plugin code is called at runtime, rather than at compile time.
            for (size_t i = 0; i < kbf::EntryPoints::ENTRY_POINT_NAMES.size(); i++) {
                functions->on_pre_application_entry(kbf::EntryPoints::ENTRY_POINT_NAMES[i], kbf::EntryPoints::PRE_HOOKS[i]);
                functions->on_post_application_entry(kbf::EntryPoints::ENTRY_POINT_NAMES[i], kbf::EntryPoints::POST_HOOKS[i]);
            }

            // Set-up default kbf entry points
            kbf::EntryPoints::instance().addBinding(kbf::EntryTiming::PRE_FUNCTION,  "UpdateMotion",       kbfFetch);
            kbf::EntryPoints::instance().addBinding(kbf::EntryTiming::POST_FUNCTION, "LateUpdateBehavior", kbfApply);

            return true;
        }
        catch (const std::exception& e) {
            reframework::API::get()->log_error(LOG_STRING("exception during initialization: {}"), e.what());
            return false;
        }
        catch (...) {
            reframework::API::get()->log_error(LOG_STRING("unknown exception during initialization."));
            return false;
        }
    }

}