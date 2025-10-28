#pragma once 

#include <kbf/player/player_tracker.hpp>
#include <kbf/gui/panels/i_panel.hpp>
#include <kbf/data/player/player_data.hpp>

#include <kbf/cimgui/cimgui_funcs.hpp>

#include <functional>

namespace kbf {

	class PlayerListPanel : public iPanel {
	public:
		PlayerListPanel(
			const std::string& label, 
			const std::string& strID,
			PlayerTracker& playerTracker,
			ImFont* wsSymbolFont);

		bool draw() override;
		void onSelectPlayer(std::function<void(PlayerData)> callback) { selectCallback = callback; }
		void onCheckDisablePlayer(std::function<bool(const PlayerData&)> callback) { checkDisablePlayerCallback = callback; }
		void onRequestDisabledPlayerTooltip(std::function<std::string()> callback) { requestDisabledPlayerTooltipCallback = callback; }

	private:
		std::vector<PlayerData> filterPlayerList(
			const std::string& filter, 
			const std::vector<PlayerData>& playerList);
		void drawPlayerList(const std::vector<PlayerData>& playerList);

		std::function<void(PlayerData)> selectCallback;
		std::function<bool(PlayerData)> checkDisablePlayerCallback;
		std::function<std::string()> requestDisabledPlayerTooltipCallback;

		PlayerTracker& playerTracker;
		ImFont* wsSymbolFont;
	};

}