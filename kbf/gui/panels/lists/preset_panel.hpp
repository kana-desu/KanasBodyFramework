#pragma once 

#include <kbf/gui/panels/i_panel.hpp>
#include <kbf/data/preset/preset.hpp>
#include <kbf/data/kbf_data_manager.hpp>

#include <kbf/cimgui/cimgui_funcs.hpp>

#include <functional>

namespace kbf {

	class PresetPanel : public iPanel {
	public:
		PresetPanel(
			const std::string& label,
			const std::string& strID,
			const KBFDataManager& dataManager,
			ImFont* wsSymbolFont,
			ImFont* wsArmourFont,
			bool showDefaultAsOption = true);

		bool draw() override;
		void onSelectPreset(std::function<void(std::string)> callback) { selectCallback = callback; }

	private:
		void drawPresetList(const std::vector<const Preset*>& presets);

		bool needsWidthStretch = false;
		float widthStretch = 0.0f;

		std::function<void(std::string)> selectCallback;

		const KBFDataManager& dataManager;
		ImFont* wsSymbolFont;
		ImFont* wsArmourFont;
		bool showDefaultAsOption;
	};

}