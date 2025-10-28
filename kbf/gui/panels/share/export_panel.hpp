#pragma once

#include <kbf/gui/panels/i_panel.hpp>
#include <kbf/data/preset/preset.hpp>
#include <kbf/gui/panels/unique_panel.hpp>
#include <kbf/gui/panels/info/info_popup_panel.hpp>
#include <kbf/data/kbf_data_manager.hpp>

#include <kbf/cimgui/cimgui_funcs.hpp>

#include <functional>
#include <unordered_set>
#include <queue>
#include <mutex>

namespace kbf {

	class ExportPanel : public iPanel {
	public:
		ExportPanel(
			const std::string& name,
			const std::string& strID,
			const bool archive,
			const KBFDataManager& dataManager,
			ImFont* wsSymbolFont,
			ImFont* wsArmourFont);

		bool draw() override;
		void onCreate(std::function<void(std::string, KBFFileData)> callback) { createCallback = callback; }
		void onCancel(std::function<void()> callback) { cancelCallback = callback; }

	private:
		const bool archive;
		const KBFDataManager& dataManager;
		std::string exportName = archive ? "New Mod Archive" : "New Export";
		std::filesystem::path exportPath = dataManager.exportsPath;

		std::unordered_set<std::string> selectedPresetGroups;
		std::unordered_set<std::string> selectedPresets;
		std::unordered_set<std::string> selectedBundles;
		std::unordered_set<PlayerData> selectedPlayerOverrides;

		void initializeBuffers();
		char exportNameBuffer[128];
		char exportPathBuffer[256];
		char filterBuffer[128];

		constexpr static float selectableHeight = 35.0f;
		constexpr static float listPaddingX = 10.0f;
		constexpr static ImVec2 childWindowPadding = ImVec2(0, 0);
		constexpr static ImVec2 childItemSpacing = ImVec2(8, 0);
		void drawPresetList();
		void drawPresetSelector(const Preset* preset, ImVec2 pos, float selectableHeight) const;
		bool checkPresetSelected(const Preset* preset) const;
		void selectPreset(const Preset* preset);
		void deselectPreset(const Preset* preset);

		void drawPresetGroupList();
		void drawPresetGroupSelector(const PresetGroup* presetGroup, ImVec2 pos, float selectableHeight) const;
		bool checkPresetGroupSelected(const PresetGroup* presetGroup) const;
		void selectPresetGroup(const PresetGroup* presetGroup);
		void deselectPresetGroup(const PresetGroup* presetGroup);

		void drawPresetBundleList();
		void drawPresetBundleSelector(const std::string& bundleName, ImVec2 pos, float selectableHeight) const;
		bool checkPresetBundleSelected(const std::vector<std::string>& bundleIds) const;
		void selectPresetBundle(const std::vector<std::string>& bundleIds);
		void deselectPresetBundle(const std::vector<std::string>& bundleIds);

		void drawPlayerOverrideList();
		void drawPlayerOverrideSelector(const PlayerOverride* override, ImVec2 pos, float selectableHeight) const;
		bool checkPlayerOverrideSelected(const PlayerOverride* override) const;
		void selectPlayerOverride(const PlayerOverride* override);
		void deselectPlayerOverride(const PlayerOverride* override);

		KBFFileData getKbfFileData() const;
		void addModArchiveTags(KBFFileData& data) const;

		// Path prompt 
		std::string getExportPathFileDialog();

		// Thread-safe task queue
		std::queue<std::function<void()>> callbackQueue;
		std::mutex callbackMutex;

		std::atomic<bool> exportPathDialogOpen = false;

		// Helper to post callback to run on main thread
		void postToMainThread(std::function<void()> func);
		void processCallbacks();

		std::function<void(std::string, KBFFileData)> createCallback;
		std::function<void()> cancelCallback;

		ImFont* wsSymbolFont;
		ImFont* wsArmourFont;
	};

}