#pragma once 

#include <kbf/gui/panels/i_panel.hpp>
#include <kbf/gui/panels/unique_panel.hpp>
#include <kbf/gui/panels/lists/preset_panel.hpp>
#include <kbf/data/preset/preset.hpp>
#include <kbf/data/kbf_data_manager.hpp>

#include <kbf/cimgui/cimgui_funcs.hpp>

#include <functional>

namespace kbf {

	class CombinePresetsPanel : public iPanel {
	public:
		CombinePresetsPanel(
			const std::string& name,
			const std::string& strID,
			const KBFDataManager& dataManager,
			ImFont* wsSymbolFont,
			ImFont* wsArmourFont,
			std::string defaultBundle = "");

		bool draw() override;
		void onCombine(std::function<void(Preset)> callback) { combineCallback = callback; }
		void onCancel(std::function<void()> callback) { cancelCallback = callback; }

	private:
		const KBFDataManager& dataManager;
		Preset preset;

		Preset combine_head;
		Preset combine_body;
		Preset combine_arms;
		Preset combine_coil;
		Preset combine_legs;

		void drawPropertiesTab();
		void drawCombinedPresetsTab();
		void drawCombinePresetSelector(const std::string& label, Preset& presetOut);

		void initializeBuffers();
		char presetNameBuffer[128];
		char presetBundleBuffer[128];

		void drawArmourList(const std::string& filter);
		void drawArmourSetName(const ArmourSet& armourSet, const float offsetBefore, const float offsetAfter);

		Preset getFinalCombinedPreset();
		
		UniquePanel<PresetPanel> presetSelectorPanel;
		void openPresetSelectorPanel(Preset& presetOut);

		std::function<void(Preset)> combineCallback;
		std::function<void()> cancelCallback;

		ImFont* wsSymbolFont;
		ImFont* wsArmourFont;
	};

}