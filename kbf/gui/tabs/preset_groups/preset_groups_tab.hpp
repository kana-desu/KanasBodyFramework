#pragma once 

#include <kbf/gui/tabs/i_tab.hpp>
#include <kbf/gui/panels/unique_panel.hpp>
#include <kbf/gui/panels/presets/create_preset_group_panel.hpp>
#include <kbf/gui/panels/presets/create_preset_group_from_bundle_panel.hpp>
#include <kbf/gui/panels/presets/edit_preset_group_panel.hpp>

#include <kbf/cimgui/cimgui_funcs.hpp>

namespace kbf {

	class PresetGroupsTab : public iTab {
	public:
		PresetGroupsTab(
			KBFDataManager& dataManager,
			ImFont* wsSymbolFont = nullptr
		) : iTab(), dataManager{ dataManager }, wsSymbolFont{ wsSymbolFont } {}

		void setSymbolFont(ImFont* font) { wsSymbolFont = font; }

		void draw() override;
		void drawPopouts() override;
		void closePopouts() override;

		void onOpenPresetGroupInEditor(std::function<void(std::string)> callback) { openPresetGroupInEditorCb = callback; }

	private:
		void drawPresetGroupList();

		UniquePanel<CreatePresetGroupPanel>           createPresetGroupPanel;
		UniquePanel<CreatePresetGroupFromBundlePanel> createPresetGroupFromBundlePanel;
		UniquePanel<EditPresetGroupPanel>             editPresetGroupPanel;
		void openCreatePresetGroupPanel();
		void openCreatePresetGroupFromBundlePanel();
		void openEditPresetGroupPanel(const std::string& presetUUID);

		KBFDataManager& dataManager;
		ImFont* wsSymbolFont;

		std::function<void(std::string)> openPresetGroupInEditorCb;
	};

}