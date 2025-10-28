#pragma once

#include <kbf/data/kbf_data_manager.hpp>
#include <kbf/player/player_tracker.hpp>
#include <kbf/gui/tabs/kbf_tab.hpp>
#include <kbf/gui/tabs/player/player_tab.hpp>
#include <kbf/gui/tabs/npc/npc_tab.hpp>
#include <kbf/gui/tabs/presets/presets_tab.hpp>
#include <kbf/gui/tabs/preset_groups/preset_groups_tab.hpp>
#include <kbf/gui/tabs/editor/editor_tab.hpp>
#include <kbf/gui/tabs/share/share_tab.hpp>
#include <kbf/gui/tabs/debug/debug_tab.hpp>
#include <kbf/gui/tabs/settings/settings_tab.hpp>
#include <kbf/gui/tabs/about/about_tab.hpp>
#include <kbf/util/io/kbf_asset_path.hpp>

#include <kbf/cimgui/cimgui_funcs.hpp>

#include <string>

namespace kbf {

	class KBFWindow {
	public:
		KBFWindow(
			KBFDataManager& dataManager,
			PlayerTracker& playerTracker,
			NpcTracker& npcTracker
		) : dataManager{ dataManager },
			playerTracker{ playerTracker },
			npcTracker{ npcTracker } {}

		~KBFWindow() = default;

		KBFWindow(const KBFWindow&) = delete;
		KBFWindow& operator=(const KBFWindow&) = delete;

		void initialize();
		void draw(bool* open);

	private:
		void initializeFonts();

		void drawKbfMenuItem(const std::string& label, const KBFTab tabId);
		void drawMenuBar();

		void drawTab();
		void drawPopouts();

		void cleanupTab(KBFTab tab);

		KBFDataManager& dataManager;
		PlayerTracker& playerTracker;
		NpcTracker& npcTracker;

		PlayerTab       playerTab{ dataManager, playerTracker };
		NpcTab          npcTab{ dataManager };
		PresetGroupsTab presetGroupsTab{ dataManager };
		PresetsTab      presetsTab{ dataManager };
		EditorTab       editorTab{ dataManager };
		ShareTab        shareTab{ dataManager };
		SettingsTab     settingsTab{ dataManager };
		DebugTab        debugTab{ dataManager, playerTracker, npcTracker };
		AboutTab        aboutTab{ dataManager };

		KBFTab tab = KBFTab::About;

		ImFont* mainFont         = nullptr;
		ImFont* wildsSymbolsFont = nullptr;
		ImFont* wildsArmourFont  = nullptr;
		ImFont* monoFont         = nullptr;
		ImFont* monoFontTiny     = nullptr;
	};

}