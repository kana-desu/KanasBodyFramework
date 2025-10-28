#pragma once 

#include <kbf/gui/panels/i_panel.hpp>
#include <kbf/data/preset/preset_group.hpp>
#include <kbf/data/kbf_data_manager.hpp>

#include <kbf/cimgui/cimgui_funcs.hpp>

#include <functional>

namespace kbf {

	class PresetGroupPanel : public iPanel {
	public:
		PresetGroupPanel(
			const std::string& label,
			const std::string& strID,
			const KBFDataManager& dataManager,
			ImFont* wsSymbolFont,
			bool showDefaultAsOption = true);

		bool draw() override;
		void onSelectPresetGroup(std::function<void(std::string)> callback) { selectCallback = callback; }

	private:
		void drawPresetGroupList(const std::vector<const PresetGroup*>& presetGroups);

		std::function<void(std::string)> selectCallback;

		const KBFDataManager& dataManager;
		ImFont* wsSymbolFont;
		ImFont* wsArmourFont;
		bool showDefaultAsOption;
	};

}