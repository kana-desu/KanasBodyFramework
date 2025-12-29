#pragma once 

#include <kbf/gui/tabs/i_tab.hpp>

#include <kbf/gui/panels/unique_panel.hpp>
#include <kbf/gui/panels/lists/preset_group_panel.hpp>
#include <kbf/gui/panels/lists/preset_panel.hpp>

#include <kbf/cimgui/cimgui_funcs.hpp>

namespace kbf {

	class NpcTab : public iTab {
	public:
		NpcTab(
			KBFDataManager& dataManager,
			ImFont* wsSymbolFont = nullptr,
			ImFont* wsArmourFont = nullptr
		) : iTab(), dataManager{ dataManager }, wsSymbolFont{ wsSymbolFont }, wsArmourFont{ wsArmourFont } {}

		void setSymbolFont(ImFont* font) { wsSymbolFont = font; }
		void setArmourFont(ImFont* font) { wsArmourFont = font; }

		void draw() override;
		void drawPopouts() override;
		void closePopouts() override;

	private:
		static constexpr ImGuiTableFlags tableFlags = ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_PadOuterX;
		static constexpr float namedNpcSelectableHeight = 40.0f;

		void drawCoreNpcsTab();
		void drawAlmaTabLists();
		void drawGemmaTabLists();
		void drawErikTabLists();
		void drawSupportHuntersTab();
		void drawOliviaTabLists();
		void drawRossoTabLists();
		void drawAlessaTabLists();
		void drawMinaTabLists();
		void drawKaiTabLists();
		void drawGriffinTabLists();
		void drawNightmistTabLists();
		void drawFabiusTabLists();
		void drawNadiaTabLists();
		void drawOtherNpcsTab();

		void drawNpcListTabItem(const std::string& tabLabel, const std::function<void()>& drawFunc);

		UniquePanel<PresetGroupPanel> editDefaultPanel;
		UniquePanel<PresetPanel>      editPanel;
		void openEditDefaultPanel(const std::function<void(std::string)>& onSelect);
		void openEditPanel(const std::function<void(std::string)>& onSelect);

		KBFDataManager& dataManager;
		ImFont* wsSymbolFont;
		ImFont* wsArmourFont;

		// Callbacks for setting presets / groups
		const std::function<void(std::string)> setMaleCb    = [&](std::string uuid) { dataManager.setNpcConfig_Male(uuid); };
		const std::function<void(std::string)> setFemaleCb  = [&](std::string uuid) { dataManager.setNpcConfig_Female(uuid); };
		const std::function<void()>            editMaleCb   = [&]() { openEditDefaultPanel(setMaleCb); };
		const std::function<void()>            editFemaleCb = [&]() { openEditDefaultPanel(setFemaleCb); };

		// Callbacks for setting individual presets
		// Alma
		const std::function<void(std::string)> setAlmaHandlersOutfitCb            = [&](std::string uuid) { dataManager.setAlmaConfig_HandlersOutfit(uuid); };
		const std::function<void(std::string)> setAlmaNewWorldCommissionCb        = [&](std::string uuid) { dataManager.setAlmaConfig_NewWorldCommission(uuid); };
		const std::function<void(std::string)> setAlmaScrivenersCoatCb            = [&](std::string uuid) { dataManager.setAlmaConfig_ScrivenersCoat(uuid); };
		const std::function<void(std::string)> setAlmaSpringBlossomKimonoCb       = [&](std::string uuid) { dataManager.setAlmaConfig_SpringBlossomKimono(uuid); };
		const std::function<void(std::string)> setAlmaChunLiOutfitCb              = [&](std::string uuid) { dataManager.setAlmaConfig_ChunLiOutfit(uuid); };
		const std::function<void(std::string)> setAlmaCammyOutfitCb               = [&](std::string uuid) { dataManager.setAlmaConfig_CammyOutfit(uuid); };
		const std::function<void(std::string)> setAlmaSummerPonchoCb              = [&](std::string uuid) { dataManager.setAlmaConfig_SummerPoncho(uuid); };
		const std::function<void(std::string)> setAlmaAutumnWitchCb               = [&](std::string uuid) { dataManager.setAlmaConfig_AutumnWitch(uuid); };
		const std::function<void(std::string)> setAlmaFeatherskirtSeikretDressCb  = [&](std::string uuid) { dataManager.setAlmaConfig_FeatherskirtSeikretDress(uuid); };
		const std::function<void()>            editAlmaHandlersOutfitCb           = [&]() { openEditPanel(setAlmaHandlersOutfitCb          ); };
		const std::function<void()>            editAlmaNewWorldCommissionCb       = [&]() { openEditPanel(setAlmaNewWorldCommissionCb      ); };
		const std::function<void()>            editAlmaScrivenersCoatCb           = [&]() { openEditPanel(setAlmaScrivenersCoatCb          ); };
		const std::function<void()>            editAlmaSpringBlossomKimonoCb      = [&]() { openEditPanel(setAlmaSpringBlossomKimonoCb     ); };
		const std::function<void()>            editAlmaChunLiOutfitCb             = [&]() { openEditPanel(setAlmaChunLiOutfitCb            ); };
		const std::function<void()>            editAlmaCammyOutfitCb              = [&]() { openEditPanel(setAlmaCammyOutfitCb             ); };
		const std::function<void()>            editAlmaSummerPonchoCb             = [&]() { openEditPanel(setAlmaSummerPonchoCb            ); };
		const std::function<void()>            editAlmaAutumnWitchCb              = [&]() { openEditPanel(setAlmaAutumnWitchCb             ); };
		const std::function<void()>            editAlmaFeatherskirtSeikretDressCb = [&]() { openEditPanel(setAlmaFeatherskirtSeikretDressCb); };

		// Gemma
		const std::function<void(std::string)> setGemmaSmithysOutfitCb        = [&](std::string uuid) { dataManager.setGemmaConfig_SmithysOutfit(uuid); };
		const std::function<void(std::string)> setGemmaSummerCoverallsCb      = [&](std::string uuid) { dataManager.setGemmaConfig_SummerCoveralls(uuid); };
		const std::function<void(std::string)> setGemmaRedveilSeikretDressCb  = [&](std::string uuid) { dataManager.setGemmaConfig_RedveilSeikretDress(uuid); };
		const std::function<void()>            editGemmaSmithysOutfitCb       = [&]() { openEditPanel(setGemmaSmithysOutfitCb      ); };
		const std::function<void()>            editGemmaSummerCoverallsCb     = [&]() { openEditPanel(setGemmaSummerCoverallsCb    ); };
		const std::function<void()>            editGemmaRedveilSeikretDressCb = [&]() { openEditPanel(setGemmaRedveilSeikretDressCb); };

		// Erik
		const std::function<void(std::string)> setErikHandlersOutfitCb          = [&](std::string uuid) { dataManager.setErikConfig_HandlersOutfit(uuid); };
		const std::function<void(std::string)> setErikSummerHatCb               = [&](std::string uuid) { dataManager.setErikConfig_SummerHat(uuid); };
		const std::function<void(std::string)> setErikAutumnTherianCb           = [&](std::string uuid) { dataManager.setErikConfig_AutumnTherian(uuid); };
		const std::function<void(std::string)> setErikCrestcollarSeikretSuitCb  = [&](std::string uuid) { dataManager.setErikConfig_CrestcollarSeikretSuit(uuid); };
		const std::function<void()>            editErikHandlersOutfitCb         = [&]() { openEditPanel(setErikHandlersOutfitCb        ); };
		const std::function<void()>            editErikSummerHatCb              = [&]() { openEditPanel(setErikSummerHatCb             ); };
		const std::function<void()>            editErikAutumnTherianCb          = [&]() { openEditPanel(setErikAutumnTherianCb         ); };
		const std::function<void()>            editErikCrestcollarSeikretSuitCb = [&]() { openEditPanel(setErikCrestcollarSeikretSuitCb); };

		// Support Hunters
		const std::function<void(std::string)> setOliviaDefaultOutfitCb = [&](std::string uuid) { dataManager.setOliviaConfig_DefaultOutfit(uuid); };
		const std::function<void(std::string)> setRossoQuematriceCb     = [&](std::string uuid) { dataManager.setRossoConfig_Quematrice(uuid); };
		const std::function<void(std::string)> setAlessaBalaharaCb      = [&](std::string uuid) { dataManager.setAlessaConfig_Balahara(uuid); };
		const std::function<void(std::string)> setMinaChatacabraCb      = [&](std::string uuid) { dataManager.setMinaConfig_Chatacabra(uuid); };
		const std::function<void(std::string)> setKaiIngotCb            = [&](std::string uuid) { dataManager.setKaiConfig_Ingot(uuid); };
		const std::function<void(std::string)> setGriffinCongaCb        = [&](std::string uuid) { dataManager.setGriffinConfig_Conga(uuid); };
		const std::function<void(std::string)> setNightmistIngotCb      = [&](std::string uuid) { dataManager.setNightmistConfig_Ingot(uuid); };
		const std::function<void(std::string)> setFabiusDefaultOutfitCb = [&](std::string uuid) { dataManager.setFabiusConfig_DefaultOutfit(uuid); };
		const std::function<void(std::string)> setNadiaDefaultOutfitCb  = [&](std::string uuid) { dataManager.setNadiaConfig_DefaultOutfit(uuid); };
		const std::function<void()>            editOliviaDefaultOutfitCb = [&]() { openEditPanel(setOliviaDefaultOutfitCb); };
		const std::function<void()>            editRossoQuematriceCb     = [&]() { openEditPanel(setRossoQuematriceCb    ); };
		const std::function<void()>            editAlessaBalaharaCb      = [&]() { openEditPanel(setAlessaBalaharaCb     ); };
		const std::function<void()>            editMinaChatacabraCb      = [&]() { openEditPanel(setMinaChatacabraCb     ); };
		const std::function<void()>            editKaiIngotCb            = [&]() { openEditPanel(setKaiIngotCb           ); };
		const std::function<void()>            editGriffinCongaCb        = [&]() { openEditPanel(setGriffinCongaCb       ); };
		const std::function<void()>            editNightmistIngotCb      = [&]() { openEditPanel(setNightmistIngotCb     ); };
		const std::function<void()>            editFabiusDefaultOutfitCb = [&]() { openEditPanel(setFabiusDefaultOutfitCb); };
		const std::function<void()>            editNadiaDefaultOutfitCb  = [&]() { openEditPanel(setNadiaDefaultOutfitCb ); };

	};

}