#include "plugin.hpp"

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
static void (*onUnloadFn)() = nullptr;
static void (*onDrawUiFn)() = nullptr;
static void (*onPreUpdateMotionFn)()  = nullptr;
static void (*onPostUpdateMotionFn)() = nullptr;
static void (*onPostLateUpdateBehaviorFn)() = nullptr;
static void onDrawUiHook(REFImGuiFrameCbData* data) { if(g_logicDll && onDrawUiFn) onDrawUiFn(); }
static void onPreUpdateMotionHook() { if (g_logicDll && onPreUpdateMotionFn) onPreUpdateMotionFn(); }
static void onPostUpdateMotionHook() { if (g_logicDll && onPostUpdateMotionFn) onPostUpdateMotionFn(); }
static void onPostLateUpdateBehaviorHook() { if (g_logicDll && onPostLateUpdateBehaviorFn) onPostLateUpdateBehaviorFn(); }

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
            if (onUnloadFn) onUnloadFn();
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

    onDrawUiFn = reinterpret_cast<void(*)()>(GetProcAddress(g_logicDll, "kbf_on_draw_ui"));
    if (!onDrawUiFn) return reframework::API::get()->log_error(LOG_STRING("Failed to get kbf_on_draw_ui function from KBF logic DLL."));

    onPreUpdateMotionFn = reinterpret_cast<void(*)()>(GetProcAddress(g_logicDll, "kbf_on_pre_update_motion"));
    if (!onPreUpdateMotionFn) return reframework::API::get()->log_error(LOG_STRING("Failed to get kbf_on_pre_update_motion function from KBF logic DLL."));

    onPostUpdateMotionFn = reinterpret_cast<void(*)()>(GetProcAddress(g_logicDll, "kbf_on_post_update_motion"));
    if (!onPostUpdateMotionFn) return reframework::API::get()->log_error(LOG_STRING("Failed to get kbf_on_post_update_motion function from KBF logic DLL."));

	onPostLateUpdateBehaviorFn = reinterpret_cast<void(*)()>(GetProcAddress(g_logicDll, "kbf_on_post_late_update_behavior"));
	if (!onPostLateUpdateBehaviorFn) return reframework::API::get()->log_error(LOG_STRING("Failed to get kbf_on_post_late_update_behavior function from KBF logic DLL."));

	onUnloadFn = reinterpret_cast<void(*)()>(GetProcAddress(g_logicDll, "kbf_on_unload"));
	if (!onUnloadFn) return reframework::API::get()->log_error(LOG_STRING("Failed to get kbf_on_unload function from KBF logic DLL."));

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
            functions->on_imgui_draw_ui(onDrawUiHook);
            functions->on_pre_application_entry("UpdateMotion", onPreUpdateMotionHook);
            functions->on_post_application_entry("UpdateMotion", onPostUpdateMotionHook);
            functions->on_post_application_entry("LateUpdateBehavior", onPostLateUpdateBehaviorHook);
            functions->on_post_application_entry("EndRendering", processHotReload);

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