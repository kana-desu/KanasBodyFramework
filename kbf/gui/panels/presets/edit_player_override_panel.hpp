#pragma once 

#include <kbf/gui/panels/i_panel.hpp>
#include <kbf/data/preset/player_override.hpp>
#include <kbf/data/preset/preset_group.hpp>
#include <kbf/data/kbf_data_manager.hpp>

#include <kbf/cimgui/cimgui_funcs.hpp>

#include <functional>

namespace kbf {

	class EditPlayerOverridePanel : public iPanel {
	public:
		EditPlayerOverridePanel(
			const PlayerData& playerData,
			const std::string& label,
			const std::string& strID,
			const KBFDataManager& dataManager,
			ImFont* wsSymbolFont);

		bool draw() override;
		void onUpdate(std::function<void(const PlayerData&, PlayerOverride)> callback) { updateCallback = callback; }
		void onDelete(std::function<void(const PlayerData&)> callback) { deleteCallback = callback; }
		void onCancel(std::function<void()> callback) { cancelCallback = callback; }

	private:
		const KBFDataManager& dataManager;
		ImFont* wsSymbolFont;

		PlayerOverride playerOverrideBefore;
		PlayerOverride playerOverride;

		char playerNameBuffer[128];
		char hunterIdBuffer[128];

		void drawPresetGroupList(const std::vector<const PresetGroup*>& presetGroups);

		std::function<void(const PlayerData&, PlayerOverride)> updateCallback;
		std::function<void(const PlayerData&)> deleteCallback;
		std::function<void()> cancelCallback;
	};

}