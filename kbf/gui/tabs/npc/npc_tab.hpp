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
		const std::function<void(std::string)> setAlmaHandlersOutfitCb       = [&](std::string uuid) { dataManager.setAlmaConfig_HandlersOutfit(uuid); };
		const std::function<void(std::string)> setAlmaNewWorldCommissionCb   = [&](std::string uuid) { dataManager.setAlmaConfig_NewWorldCommission(uuid); };
		const std::function<void(std::string)> setAlmaScrivenersCoatCb       = [&](std::string uuid) { dataManager.setAlmaConfig_ScrivenersCoat(uuid); };
		const std::function<void(std::string)> setAlmaSpringBlossomKimonoCb  = [&](std::string uuid) { dataManager.setAlmaConfig_SpringBlossomKimono(uuid); };
		const std::function<void(std::string)> setAlmaChunLiOutfitCb         = [&](std::string uuid) { dataManager.setAlmaConfig_ChunLiOutfit(uuid); };
		const std::function<void(std::string)> setAlmaCammyOutfitCb          = [&](std::string uuid) { dataManager.setAlmaConfig_CammyOutfit(uuid); };
		const std::function<void(std::string)> setAlmaSummerPonchoCb         = [&](std::string uuid) { dataManager.setAlmaConfig_SummerPoncho(uuid); };
		const std::function<void(std::string)> setAlmaAutumnWitchCb          = [&](std::string uuid) { dataManager.setAlmaConfig_AutumnWitch(uuid); };
		const std::function<void()>            editAlmaHandlersOutfitCb      = [&]() { openEditPanel(setAlmaHandlersOutfitCb     ); };
		const std::function<void()>            editAlmaNewWorldCommissionCb  = [&]() { openEditPanel(setAlmaNewWorldCommissionCb ); };
		const std::function<void()>            editAlmaScrivenersCoatCb      = [&]() { openEditPanel(setAlmaScrivenersCoatCb     ); };
		const std::function<void()>            editAlmaSpringBlossomKimonoCb = [&]() { openEditPanel(setAlmaSpringBlossomKimonoCb); };
		const std::function<void()>            editAlmaChunLiOutfitCb        = [&]() { openEditPanel(setAlmaChunLiOutfitCb       ); };
		const std::function<void()>            editAlmaCammyOutfitCb         = [&]() { openEditPanel(setAlmaCammyOutfitCb        ); };
		const std::function<void()>            editAlmaSummerPonchoCb        = [&]() { openEditPanel(setAlmaSummerPonchoCb       ); };
		const std::function<void()>            editAlmaAutumnWitchCb         = [&]() { openEditPanel(setAlmaAutumnWitchCb        ); };

		// Gemma
		const std::function<void(std::string)> setGemmaSmithysOutfitCb       = [&](std::string uuid) { dataManager.setGemmaConfig_SmithysOutfit(uuid); };
		const std::function<void(std::string)> setGemmaSummerCoverallsCb     = [&](std::string uuid) { dataManager.setGemmaConfig_SummerCoveralls(uuid); };
		const std::function<void()>            editGemmaSmithysOutfitCb      = [&]() { openEditPanel(setGemmaSmithysOutfitCb   ); };
		const std::function<void()>            editGemmaSummerCoverallsCb    = [&]() { openEditPanel(setGemmaSummerCoverallsCb); };

		// Erik
		const std::function<void(std::string)> setErikHandlersOutfitCb       = [&](std::string uuid) { dataManager.setErikConfig_HandlersOutfit(uuid); };
		const std::function<void(std::string)> setErikSummerHatCb            = [&](std::string uuid) { dataManager.setErikConfig_SummerHat(uuid); };
		const std::function<void(std::string)> setErikAutumnTherianCb        = [&](std::string uuid) { dataManager.setErikConfig_AutumnTherian(uuid); };
		const std::function<void()>            editErikHandlersOutfitCb      = [&]() { openEditPanel(setErikHandlersOutfitCb); };
		const std::function<void()>            editErikSummerHatCb           = [&]() { openEditPanel(setErikSummerHatCb     ); };
		const std::function<void()>            editErikAutumnTherianCb       = [&]() { openEditPanel(setErikAutumnTherianCb ); };

	};

}