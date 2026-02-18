#pragma once 

#include <kbf/gui/panels/i_panel.hpp>
#include <kbf/gui/panels/unique_panel.hpp>
#include <kbf/gui/panels/lists/preset_panel.hpp>
#include <kbf/data/preset/preset.hpp>
#include <kbf/data/kbf_data_manager.hpp>

#include <kbf/cimgui/cimgui_funcs.hpp>

#include <functional>

namespace kbf {

	class CreatePresetPanel : public iPanel {
	public:
		CreatePresetPanel(
			const std::string& name,
			const std::string& strID,
			const KBFDataManager& dataManager,
			ImFont* wsSymbolFont,
			ImFont* wsArmourFont,
			std::string defaultBundle = "");

		bool draw() override;
		void onCreate(std::function<void(Preset)> callback) { createCallback = callback; }
		void onCancel(std::function<void()> callback) { cancelCallback = callback; }

	private:
		const KBFDataManager& dataManager;
		Preset preset;

		void initializeBuffers();
		char presetNameBuffer[128];
		char presetBundleBuffer[128];

		void drawArmourList(const std::string& filter);
		void drawArmourSetName(const ArmourSet& armourSet, const float offsetBefore, const float offsetAfter);

		UniquePanel<PresetPanel> copyPresetPanel;
		void openCopyPresetPanel();

		std::function<void(Preset)> createCallback;
		std::function<void()> cancelCallback;

		ImFont* wsSymbolFont;
		ImFont* wsArmourFont;
	};

}