#pragma once

#include <kbf/gui/panels/i_panel.hpp>
#include <kbf/data/preset/preset.hpp>
#include <kbf/gui/panels/unique_panel.hpp>
#include <kbf/gui/panels/info/info_popup_panel.hpp>
#include <kbf/data/kbf_data_manager.hpp>

#include <kbf/cimgui/cimgui_funcs.hpp>

#include <functional>

namespace kbf {

	class CreatePresetGroupFromBundlePanel : public iPanel {
	public:
		CreatePresetGroupFromBundlePanel(
			const std::string& name,
			const std::string& strID,
			const KBFDataManager& dataManager,
			ImFont* wsSymbolFont);

		bool draw() override;
		void onCreate(std::function<void(PresetGroup)> callback) { createCallback = callback; }
		void onCancel(std::function<void()> callback) { cancelCallback = callback; }

	private:
		const KBFDataManager& dataManager;
		std::string selectedBundle = "";
		PresetGroup presetGroup;

		void initializeBuffers();
		char presetGroupNameBuffer[128];

		void drawBundleList(const std::vector<std::pair<std::string, size_t>>& bundleList);

		size_t assignPresetsToGroup(const std::vector<std::string>& presetUUIDs);

		UniquePanel<InfoPopupPanel> conflictInfoPanel;
		void openConflictInfoPanel(size_t nConflicts);

		std::function<void(PresetGroup)> createCallback;
		std::function<void()> cancelCallback;

		ImFont* wsSymbolFont;
	};

}