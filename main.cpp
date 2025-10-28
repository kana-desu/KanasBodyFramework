#include <plugin.hpp>

#include <kbf/hook/hook_manager.hpp>
#include <kbf/debug/log_string.hpp>

#include <Windows.h>

#define HOT_RELOAD_EXPORT __declspec(dllexport)

extern "C" {

    // Hot reload dll exports
    HOT_RELOAD_EXPORT void initialize_kbf() { kbf::preInitialize(); }
    HOT_RELOAD_EXPORT void kbf_on_draw_ui() { kbf::onDrawUi(); }
    HOT_RELOAD_EXPORT void kbf_on_pre_update_motion() { kbf::onPreUpdateMotion(); }
    HOT_RELOAD_EXPORT void kbf_on_post_update_motion() { kbf::onPostUpdateMotion(); }
	HOT_RELOAD_EXPORT void kbf_on_post_late_update_behavior() { kbf::onPostLateUpdateBehavior(); }
    HOT_RELOAD_EXPORT void kbf_on_unload() { kbf::HookManager::remove_all(); }
    HOT_RELOAD_EXPORT void kbf_force_initialize_reframework(const REFrameworkPluginInitializeParam* param) {
        kbf::forceInitializeReframework(param);
	}

    // Release dll exports
    RE_EXPORT void reframework_plugin_required_version(REFrameworkPluginVersion* version) {
        version->major = REFRAMEWORK_PLUGIN_VERSION_MAJOR;
		version->minor = REFRAMEWORK_PLUGIN_VERSION_MINOR;
        version->patch = REFRAMEWORK_PLUGIN_VERSION_PATCH;
        version->game_name = "MHWILDS";
    }

    RE_EXPORT bool reframework_plugin_initialize(const REFrameworkPluginInitializeParam* param) {
        kbf::preInitialize();

        reframework::API::initialize(param);
        reframework::API::get()->log_info(LOG_STRING("Initializing..."));

        try {
            const auto onPreUpdateMotionHook  = []() { kbf::onPreUpdateMotion(); };
            const auto onPostUpdateMotionHook = []() { kbf::onPostUpdateMotion(); };
			const auto onPostLateUpdateBehaviorHook = []() { kbf::onPostLateUpdateBehavior(); };
            const auto onImguiDrawUiHook = [](REFImGuiFrameCbData* data) { kbf::onDrawUi(); };

            const REFrameworkPluginFunctions* functions = param->functions;
            functions->on_imgui_draw_ui(onImguiDrawUiHook);
            functions->on_pre_application_entry("UpdateMotion", onPreUpdateMotionHook);
            functions->on_post_application_entry("UpdateMotion", onPostUpdateMotionHook);
			functions->on_post_application_entry("LateUpdateBehavior", onPostLateUpdateBehaviorHook);

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