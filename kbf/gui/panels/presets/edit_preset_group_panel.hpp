#pragma once 

#include <kbf/gui/panels/i_panel.hpp>
#include <kbf/data/preset/preset.hpp>
#include <kbf/data/kbf_data_manager.hpp>
#include <kbf/gui/panels/unique_panel.hpp>
#include <kbf/gui/panels/lists/preset_group_panel.hpp>

#include <kbf/cimgui/cimgui_funcs.hpp>

#include <functional>

namespace kbf {

	class EditPresetGroupPanel : public iPanel {
	public:
		EditPresetGroupPanel(
			const std::string& presetGroupUUID,
			const std::string& name,
			const std::string& strID,
			const KBFDataManager& dataManager,
			ImFont* wsSymbolFont);

		bool draw() override;
		void onDelete(std::function<void(const std::string&)> callback) { deleteCallback = callback; }
		void onUpdate(std::function<void(const std::string&, PresetGroup)> callback) { updateCallback = callback; }
		void onCancel(std::function<void()> callback) { cancelCallback = callback; }
		void onOpenEditor(std::function<void(std::string)> callback) { openEditorCallback = callback; }

	private:
		const KBFDataManager& dataManager;
		std::string presetGroupUUID;
		PresetGroup presetGroupBefore;
		PresetGroup presetGroup;

		void initializeBuffers();
		char presetGroupNameBuffer[128];

		UniquePanel<PresetGroupPanel> copyPresetGroupPanel;
		void openCopyPresetGroupPanel();

		std::function<void(const std::string&)>              deleteCallback;
		std::function<void(const std::string&, PresetGroup)> updateCallback;
		std::function<void()>                                cancelCallback;
		std::function<void(std::string)>                     openEditorCallback;

		ImFont* wsSymbolFont;
	};

}