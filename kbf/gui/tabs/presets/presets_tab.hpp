#pragma once 

#include <kbf/gui/tabs/i_tab.hpp>

#include <kbf/gui/panels/unique_panel.hpp>
#include <kbf/gui/panels/info/info_popup_panel.hpp>
#include <kbf/gui/panels/presets/create_preset_panel.hpp>
#include <kbf/gui/panels/presets/combine_presets_panel.hpp>
#include <kbf/gui/panels/presets/edit_preset_panel.hpp>
#include <kbf/gui/panels/presets/import_fbs_presets_panel.hpp>

#include <kbf/cimgui/cimgui_funcs.hpp>

namespace kbf {

	class PresetsTab : public iTab {
	public:
		PresetsTab(
			KBFDataManager& dataManager,
			ImFont* wsSymbolFont = nullptr,
			ImFont* wsArmourFont = nullptr
		) : iTab(), dataManager{ dataManager }, wsSymbolFont{ wsSymbolFont }, wsArmourFont{ wsArmourFont } {}

		void setSymbolFont(ImFont* font) { wsSymbolFont = font; }
		void setArmourFont(ImFont* font) { wsArmourFont = font; }

		void draw() override;
		void drawPopouts() override;
		void closePopouts() override;

		void onOpenPresetInEditor(std::function<void(std::string)> callback) { openPresetInEditorCb = callback; }

	private:
		void drawBundleTab();
		void drawBundleList();
		void drawPresetList(const std::string& bundleFilter = "");
		std::string bundleViewed = "";

		UniquePanel<CreatePresetPanel>     createPresetPanel;
		UniquePanel<CombinePresetsPanel>   combinePresetsPanel;
		UniquePanel<EditPresetPanel>       editPresetPanel;
		UniquePanel<ImportFbsPresetsPanel> importFBSPresetsPanel;
		UniquePanel<InfoPopupPanel>        warnDeleteBundlePanel;
		void openCreatePresetPanel();
		void openCombinePresetsPanel();
		void openEditPresetPanel(const std::string& presetUUID);
		void openImportFBSPresetsPanel();
		void openWarnDeleteBundlePanel(const std::string& bundleName);

		KBFDataManager& dataManager;
		ImFont* wsSymbolFont;
		ImFont* wsArmourFont;

		std::function<void(std::string)> openPresetInEditorCb;

		std::string getBundleViewed();
		bool inBundlesTab = true;
	};

}