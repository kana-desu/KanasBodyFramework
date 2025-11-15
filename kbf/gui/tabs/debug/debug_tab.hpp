#pragma once 

#include <kbf/gui/tabs/i_tab.hpp>
#include <kbf/player/player_tracker.hpp>
#include <kbf/npc/npc_tracker.hpp>
#include <kbf/data/mesh/materials/mesh_material.hpp>

#include <kbf/cimgui/cimgui_funcs.hpp>

namespace kbf {

	class DebugTab : public iTab {
	public:
		DebugTab(
			KBFDataManager& dataManager,
			PlayerTracker& playerTracker, 
			NpcTracker& npcTracker,
			ImFont* wsArmourFont = nullptr,
			ImFont* wsSymbolFont = nullptr,
			ImFont* monoFont = nullptr
		) : dataManager{ dataManager },
			playerTracker{ playerTracker },
			npcTracker{ npcTracker },
			wsArmourFont{ wsArmourFont },
			wsSymbolFont{ wsSymbolFont },
			monoFont{ monoFont } {}

		void setArmourFont(ImFont* font) { wsArmourFont = font; }
		void setSymbolFont(ImFont* font) { wsSymbolFont = font; }
		void setMonoFont(ImFont* font) { monoFont = font; }

		void draw() override;
		void drawPopouts() override;
		void closePopouts() override;

	private:
		void drawDebugTab();
		void drawPerformanceTab();
		void drawPerformanceTab_TimingRow(std::string blockName, double t, const double* max_t = nullptr);
		void drawSituationTab();
		void drawSituationTab_Row(std::string name, bool active, bool colorBg);
		void drawPlayerList();
		void drawPlayerListRow(PlayerInfo info, const PersistentPlayerInfo* pInfo);
		void drawNpcList();
		void drawNpcListRow(NpcInfo info, const PersistentNpcInfo* pInfo);
		void drawCacheTab();
		void drawBoneCacheTab();
		void drawBoneCacheTab_BoneList(const std::string& label, const std::vector<std::string>& bones);
		void drawPartCacheTab();
		void drawPartCacheTab_PartList(const std::string& label, const std::vector<MeshPart>& parts);
		void drawMatCacheTab();
		void drawMatCacheTab_MaterialList(const std::string& label, const std::vector<MeshMaterial>& mats);

		static ImVec4 getTimingColour(double ms);

		KBFDataManager& dataManager;
		PlayerTracker& playerTracker;
		NpcTracker& npcTracker;
		ImFont* wsArmourFont;
		ImFont* wsSymbolFont;
		ImFont* monoFont;

		bool consoleAutoscroll = true;
		bool showInfo = true;
		bool showSuccess = true;
		bool showWarn = true;
		bool showError = true;
		bool showDebug = false;
	};

}