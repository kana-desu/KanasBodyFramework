#pragma once 

#include <kbf/gui/panels/i_panel.hpp>
#include <kbf/data/preset/preset.hpp>
#include <kbf/data/kbf_data_manager.hpp>
#include <kbf/gui/panels/unique_panel.hpp>
#include <kbf/gui/panels/lists/preset_panel.hpp>

#include <kbf/cimgui/cimgui_funcs.hpp>

#include <functional>

namespace kbf {

	class EditPresetPanel : public iPanel {
	public:
		EditPresetPanel(
			const std::string& presetUUID,
			const std::string& name,
			const std::string& strID,
			const KBFDataManager& dataManager,
			ImFont* wsSymbolFont,
			ImFont* wsArmourFont);

		bool draw() override;
		void onDelete(std::function<void(const std::string)> callback) { deleteCallback = callback; }
		void onUpdate(std::function<void(const std::string&, Preset)> callback) { updateCallback = callback; }
		void onCancel(std::function<void()> callback) { cancelCallback = callback; }
		void onOpenEditor(std::function<void(std::string)> callback) { openEditorCallback = callback; }

	private:
		const KBFDataManager& dataManager;
		std::string presetUUID;
		Preset presetBefore;
		Preset preset;

		void initializeBuffers();
		char presetNameBuffer[128];
		char presetBundleBuffer[128];

		void drawArmourList(const std::string& filter);
		void drawArmourSetName(const ArmourSet& armourSet, const float offsetBefore, const float offsetAfter);

		UniquePanel<PresetPanel> copyPresetPanel;
		void openCopyPresetPanel();


		std::function<void(const std::string&)>         deleteCallback;
		std::function<void(const std::string&, Preset)> updateCallback;
		std::function<void()>                           cancelCallback;
		std::function<void(std::string)>                openEditorCallback;

		ImFont* wsSymbolFont;
		ImFont* wsArmourFont;
	};

}