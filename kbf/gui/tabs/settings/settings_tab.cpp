#include <kbf/gui/tabs/settings/settings_tab.hpp>

#include <kbf/gui/shared/tab_bar_separator.hpp>
#include <kbf/debug/debug_stack.hpp>

#include <kbf/gui/components/toggle/imgui_toggle.h>
#include <kbf/util/functional/invoke_callback.hpp>

#define SETTINGS_TAB_LOG_TAG "[SettingsTab]"

namespace kbf {

	std::chrono::steady_clock::time_point SettingsTab::lastWriteTime = std::chrono::steady_clock::now();

	void SettingsTab::draw() {
		KBFSettings& settings = dataManager.settings();
		bool settingsChanged = false;

		drawTabBarSeparator("Data", "SettingsTabData");
		CImGui::Spacing();
		const ImVec2 buttonSize = ImVec2(CImGui::GetContentRegionAvail().x, 0.0f);
		if (CImGui::Button("Reload Data", buttonSize)) {
			dataManager.reloadData();
			INVOKE_OPTIONAL_CALLBACK(onReloadData);
		}
		CImGui::SetItemTooltip("Reload Preset Groups, Presets, & Player Overrides.");

		CImGui::Spacing();
		CImGui::Spacing();
		drawTabBarSeparator("General", "SettingsTabGeneral");
		CImGui::Spacing();

		pushToggleColors(settings.enabled);
		bool enabledBefore = settings.enabled;
		settingsChanged |= CImGui::Toggle(" Enable KBF", &settings.enabled, ImGuiToggleFlags_Animated);
		bool enabledAfter = settings.enabled;
		popToggleColors();
		CImGui::SetItemTooltip("Turn this plugin on/off.");

		pushToggleColors(settings.enablePlayers);
		settingsChanged |= CImGui::Toggle(" Enable KBF for Players", &settings.enablePlayers, ImGuiToggleFlags_Animated);
		popToggleColors();
		CImGui::SetItemTooltip("Enable / Disable tracking & application for players.");

		pushToggleColors(settings.enableNpcs);
		settingsChanged |= CImGui::Toggle(" Enable KBF for NPCs", &settings.enableNpcs, ImGuiToggleFlags_Animated);
		popToggleColors();
		CImGui::SetItemTooltip("Enable / Disable tracking & application for NPCs");

		// Force turning off "Enable KBF" slider if both players and NPCs are disabled.
		/// Similarly, force turning on "Enable KBF" slider if either players or NPCs are enabled.

		if (!settings.enablePlayers && !settings.enableNpcs && settings.enabled) {
			settings.enabled = false;
			settingsChanged = true;
		}
		else if ((settings.enablePlayers || settings.enableNpcs) && !settings.enabled) {
			settings.enabled = true;
			settingsChanged = true;
		}

		if (enabledAfter != enabledBefore) {
			if (enabledAfter) {
				settings.enableNpcs = true;
				settings.enablePlayers = true;
			}
			else {
				settings.enableNpcs = false;
				settings.enablePlayers = false;
			}
			settingsChanged = true;
		}

		// --------------------------------

		CImGui::Spacing();
		CImGui::Spacing();
		drawTabBarSeparator("Scenarios", "SettingsTabScenarios");
		CImGui::Spacing();

		CImGui::Spacing();
		pushToggleColors(settings.enableDuringQuestsOnly);
		settingsChanged |= CImGui::Toggle(" Enable During Quests Only", &settings.enableDuringQuestsOnly, ImGuiToggleFlags_Animated);
		popToggleColors();
		CImGui::SetItemTooltip("Only apply modifiers in quests where the number applications is minimal.");

		pushToggleColors(settings.hideWeaponsOutsideOfCombatOnly);
		settingsChanged |= CImGui::Toggle(" Hide Weapons Outside of Combat Only", &settings.hideWeaponsOutsideOfCombatOnly, ImGuiToggleFlags_Animated);
		popToggleColors();
		CImGui::SetItemTooltip("Enabling this will ensure your weapon is always visible during combat.");

		pushToggleColors(settings.hideSlingerOutsideOfCombatOnly);
		settingsChanged |= CImGui::Toggle(" Hide Slinger Outside of Combat Only", &settings.hideSlingerOutsideOfCombatOnly, ImGuiToggleFlags_Animated);
		popToggleColors();
		CImGui::SetItemTooltip("Enabling this will ensure your slinger is always visible during combat.");

		// --------------------------------

		CImGui::Spacing();
		CImGui::Spacing();
		drawTabBarSeparator("Performance", "SettingsTabPerformance");
		CImGui::Spacing();

		CImGui::PushItemWidth(-1);
		settingsChanged |= CImGui::DragFloat("##Slider1", &settings.delayOnEquip,     0.001f, 0.0f, 2.0f, "Delay on Equip: %.3fs", ImGuiSliderFlags_AlwaysClamp);
		CImGui::SetItemTooltip(
			"Delay appling bone modifiers for this amount of time when a model is loaded.\n\n"
			"Setting this value lower will reduce pop-in, but may break physics (particularly when equipping a loadout) if set too low.\n"
			"It is recommended to set this value to the minimum value possible which does not break physics upon equipping an armour loadout.\n\n"
			"Suggested Range: 0.01 ~ 0.1, depending on speed of your CPU.");

		settingsChanged |= CImGui::DragFloat("##Slider2", &settings.applicationRange, 0.1f, 0.0f, 300.0f, "Application Range: %.1fm", ImGuiSliderFlags_AlwaysClamp);
		CImGui::SetItemTooltip(
			"Characters further away from the camera than this value will not be modified.\n\n"
			"This can improve performance in player-dense areas (e.g. full grand hub lobbies).\n"
			"Note that you aren't likely to notice subtle modifiers on any far-away players.\n\n"
			"Setting this value too low may introduce noticeable pop-in if you have unsubtle modifiers.\n\n"
			"Suggested Range For High Performance: 15 ~ 30 (As low as possible until you notice pop-in)\n"
			"Note: When set to 0, the application range will be unlimited.");

		settingsChanged |= CImGui::SliderInt("##Slider3", &settings.maxConcurrentApplications, 0, 99, "Max Concurrent Applications: %d", ImGuiSliderFlags_AlwaysClamp);
		CImGui::SetItemTooltip(
			"The maximum number of models which will modified at once (prioritised by distance to camera).\n\n"
			"This can improve performance in player-dense areas (e.g. full grand hub lobbies)\n\n"
			"Typical presets will add only a fraction of a millisecond to frame-time (per application).\n"
			"If you have many active presets, or presets with many modifiers, setting an application limit is recommended.\n\n"
			"Suggested Range For High Performance: < 10\n"
			"Note: When set to 0, modifiers will be unconditionally applied to all players.");

		settingsChanged |= CImGui::SliderInt("##Slider4", &settings.maxBoneFetchesPerFrame, 0, 100, "Max Bone Fetches Per Frame: %d", ImGuiSliderFlags_AlwaysClamp);
		CImGui::SetItemTooltip(
			"The maximum number of models that we can fetch bones for per-frame.\n\n"
			"Reducing this limit can improve frame-lows when loading into a new area by distributing the computation over more frames, but may introduce slight pop-in.\n\n"
			"Suggested Range For High Performance: 1 ~ 5\n"
			"Note: When set to 0, all tracked bones may be fetched on a single frame. This reduces pop-in but may cause slight frame-dips when loading into a scene.");

		CImGui::PopItemWidth();

		if (settingsChanged) needsWrite = true;

		auto durationSec = std::chrono::duration_cast<std::chrono::duration<float>>(std::chrono::steady_clock::now() - lastWriteTime);
		if (needsWrite && durationSec.count() >= writeRateLimit) {
			DEBUG_STACK.push(std::format("{} Settings Changed, writing to disk...", SETTINGS_TAB_LOG_TAG), DebugStack::Color::DEBUG);
			needsWrite = !dataManager.writeSettings();
			lastWriteTime = std::chrono::steady_clock::now();
		}

	}


	void SettingsTab::drawPopouts() {};
	void SettingsTab::closePopouts() {};

	void SettingsTab::pushToggleColors(bool enabled) {
		if (enabled) {
			CImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.5f, 0.1f, 1.0f));
			CImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.8f, 0.3f, 1.0f));
		}
		else {
			CImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.5f, 0.1f, 0.1f, 1.0f));
			CImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.8f, 0.3f, 0.3f, 1.0f));
		}
	}

	void SettingsTab::popToggleColors() {
		CImGui::PopStyleColor(2);
	}

}