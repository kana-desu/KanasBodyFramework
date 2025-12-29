#include <kbf/gui/tabs/npc/npc_tab.hpp>

#include <kbf/gui/shared/tab_bar_separator.hpp>
#include <kbf/gui/shared/preset_selectors.hpp>
#include <kbf/gui/shared/styling_consts.hpp>

namespace kbf {

	void NpcTab::draw() {

        CImGui::PushStyleVar(ImGuiStyleVar_CellPadding, LIST_PADDING);

        if (CImGui::BeginTabBar("NpcTabs")) {
            if (CImGui::BeginTabItem("Main Npcs")) {
                CImGui::Spacing();
                CImGui::Separator();
                CImGui::Spacing();
                drawCoreNpcsTab();
                CImGui::EndTabItem();
            }
            if (CImGui::IsItemClicked()) closePopouts();

            if (CImGui::BeginTabItem("Support Hunters")) {
                CImGui::Spacing();
                CImGui::Separator();
                CImGui::Spacing();                
                drawSupportHuntersTab();
                CImGui::EndTabItem();
            }
            if (CImGui::IsItemClicked()) closePopouts();

            if (CImGui::BeginTabItem("Unnamed/Other Npcs")) {
                CImGui::Spacing();
                CImGui::Separator();
                CImGui::Spacing();                
                drawOtherNpcsTab();
                CImGui::EndTabItem();
            }
            if (CImGui::IsItemClicked()) closePopouts();

            CImGui::EndTabBar();
        }

        CImGui::PopStyleVar(1);
	}

    void NpcTab::drawCoreNpcsTab() {
        // Alma / Erik / Gemma tab
        if (CImGui::BeginTabBar("CoreNpcTabs")) {
			drawNpcListTabItem("Alma",  [&]() { drawAlmaTabLists(); });
			drawNpcListTabItem("Gemma", [&]() { drawGemmaTabLists(); });
			drawNpcListTabItem("Erik",  [&]() { drawErikTabLists(); });
            CImGui::EndTabBar();
        }

    }

    void NpcTab::drawAlmaTabLists() {
        CImGui::BeginTable("##AlmaPresetList", 1, tableFlags);
        drawPresetSelectTableEntry(wsSymbolFont,
            "##AlmaPresetCombo_HandlersOutfit", "Handler's Outfit",
            dataManager.getPresetByUUID(dataManager.almaConfig().handlersOutfit),
            editAlmaHandlersOutfitCb,
            namedNpcSelectableHeight);
        drawPresetSelectTableEntry(wsSymbolFont,
            "##AlmaPresetCombo_NewWorldCommission", "New World Commission",
            dataManager.getPresetByUUID(dataManager.almaConfig().newWorldCommission),
            editAlmaNewWorldCommissionCb,
            namedNpcSelectableHeight);
        drawPresetSelectTableEntry(wsSymbolFont,
            "##AlmaPresetCombo_ScrivenersCoat", "Scrivener's Coat",
            dataManager.getPresetByUUID(dataManager.almaConfig().scrivenersCoat),
            editAlmaScrivenersCoatCb,
            namedNpcSelectableHeight);
        drawPresetSelectTableEntry(wsSymbolFont,
            "##AlmaPresetCombo_SpringBlossomKimono", "Spring Blossom Kimono",
            dataManager.getPresetByUUID(dataManager.almaConfig().springBlossomKimono),
            editAlmaSpringBlossomKimonoCb,
            namedNpcSelectableHeight);
        drawPresetSelectTableEntry(wsSymbolFont,
            "##AlmaPresetCombo_ChunLiOutfit", "Chun-li Outfit",
            dataManager.getPresetByUUID(dataManager.almaConfig().chunLiOutfit),
            editAlmaChunLiOutfitCb,
            namedNpcSelectableHeight);
        drawPresetSelectTableEntry(wsSymbolFont,
            "##AlmaPresetCombo_CammyOutfit", "Cammy Outfit",
            dataManager.getPresetByUUID(dataManager.almaConfig().cammyOutfit),
            editAlmaCammyOutfitCb,
            namedNpcSelectableHeight);
        drawPresetSelectTableEntry(wsSymbolFont,
            "##AlmaPresetCombo_SummerPoncho", "Summer Poncho",
            dataManager.getPresetByUUID(dataManager.almaConfig().summerPoncho),
            editAlmaSummerPonchoCb,
            namedNpcSelectableHeight);
        drawPresetSelectTableEntry(wsSymbolFont,
            "##AlmaPresetCombo_AutumnWitch", "Autumn Witch",
            dataManager.getPresetByUUID(dataManager.almaConfig().autumnWitch),
            editAlmaAutumnWitchCb,
            namedNpcSelectableHeight);
        drawPresetSelectTableEntry(wsSymbolFont,
            "##AlmaPresetCombo_FeatherskirtSeikretDress", "Featherskirt Seikret Dress",
            dataManager.getPresetByUUID(dataManager.almaConfig().featherskirtSeikretDress),
            editAlmaFeatherskirtSeikretDressCb,
            namedNpcSelectableHeight);
        CImGui::EndTable();
	}

    void NpcTab::drawGemmaTabLists() {
        CImGui::BeginTable("##GemmaPresetList", 1, tableFlags);
        drawPresetSelectTableEntry(wsSymbolFont,
            "##GemmaPresetCombo_SmithysOutfit", "Smithy's Outfit",
            dataManager.getPresetByUUID(dataManager.gemmaConfig().smithysOutfit),
            editGemmaSmithysOutfitCb,
            namedNpcSelectableHeight);
        drawPresetSelectTableEntry(wsSymbolFont,
            "##GemmaPresetCombo_SummerCoveralls", "Summer Coveralls",
            dataManager.getPresetByUUID(dataManager.gemmaConfig().summerCoveralls),
            editGemmaSummerCoverallsCb,
            namedNpcSelectableHeight);
        drawPresetSelectTableEntry(wsSymbolFont,
            "##GemmaPresetCombo_RedveilSeikretDress", "Redveil Seikret Dress",
            dataManager.getPresetByUUID(dataManager.gemmaConfig().redveilSeikretDress),
            editGemmaRedveilSeikretDressCb,
            namedNpcSelectableHeight);
        CImGui::EndTable();

    }
    void NpcTab::drawErikTabLists() {
        CImGui::BeginTable("##ErikPresetList", 1, tableFlags);
        drawPresetSelectTableEntry(wsSymbolFont,
            "##ErikPresetCombo_HandlersOutfit", "Handler's Outfit",
            dataManager.getPresetByUUID(dataManager.erikConfig().handlersOutfit),
            editErikHandlersOutfitCb,
            namedNpcSelectableHeight);
        drawPresetSelectTableEntry(wsSymbolFont,
            "##ErikPresetCombo_SummerHat", "Summer Hat",
            dataManager.getPresetByUUID(dataManager.erikConfig().summerHat),
            editErikSummerHatCb,
            namedNpcSelectableHeight);
        drawPresetSelectTableEntry(wsSymbolFont,
            "##ErikPresetCombo_AutumnTherian", "Autumn Therian",
            dataManager.getPresetByUUID(dataManager.erikConfig().autumnTherian),
            editErikAutumnTherianCb,
            namedNpcSelectableHeight);
        drawPresetSelectTableEntry(wsSymbolFont,
            "##ErikPresetCombo_CrestcollarSeikretSuit", "Crestcollar Seikret Suit",
            dataManager.getPresetByUUID(dataManager.erikConfig().crestcollarSeikretSuit),
            editErikCrestcollarSeikretSuitCb,
            namedNpcSelectableHeight);
        CImGui::EndTable();
    }

    void NpcTab::drawSupportHuntersTab() {
        if (CImGui::BeginTabBar("CoreNpcTabs")) {
			drawNpcListTabItem("Olivia",    [&]() { drawOliviaTabLists(); });
			drawNpcListTabItem("Rosso",     [&]() { drawRossoTabLists(); });
			drawNpcListTabItem("Alessa",    [&]() { drawAlessaTabLists(); });
			drawNpcListTabItem("Mina",      [&]() { drawMinaTabLists(); });
			drawNpcListTabItem("Kai",       [&]() { drawKaiTabLists(); });
			drawNpcListTabItem("Griffin",   [&]() { drawGriffinTabLists(); });
			drawNpcListTabItem("Nightmist", [&]() { drawNightmistTabLists(); });
			drawNpcListTabItem("Fabius",    [&]() { drawFabiusTabLists(); });
			drawNpcListTabItem("Nadia",     [&]() { drawNadiaTabLists(); });
            CImGui::EndTabBar();
        }
    }

    void NpcTab::drawOliviaTabLists() {
        CImGui::BeginTable("##OliviaPresetList", 1, tableFlags);
        drawPresetSelectTableEntry(wsSymbolFont,
            "##OliviaPresetCombo_DefaultOutfit", "Olivia's Outfit",
            dataManager.getPresetByUUID(dataManager.supportHunterConfigs().olivia.defaultOutfit),
            editOliviaDefaultOutfitCb,
            namedNpcSelectableHeight);
        CImGui::EndTable();
	}

    void NpcTab::drawRossoTabLists() {
        CImGui::BeginTable("##RossoPresetList", 1, tableFlags);
        drawPresetSelectTableEntry(wsSymbolFont,
            "##RossoPresetCombo_DefaultOutfit", "Quematrice Armour",
            dataManager.getPresetByUUID(dataManager.supportHunterConfigs().rosso.quematrice),
            editRossoQuematriceCb,
            namedNpcSelectableHeight);
        CImGui::EndTable();
    }

    void NpcTab::drawAlessaTabLists() {
        CImGui::BeginTable("##AlessaPresetList", 1, tableFlags);
        drawPresetSelectTableEntry(wsSymbolFont,
            "##AlessaPresetCombo_DefaultOutfit", "Balahara Armour",
            dataManager.getPresetByUUID(dataManager.supportHunterConfigs().alessa.balahara),
            editAlessaBalaharaCb,
            namedNpcSelectableHeight);
        CImGui::EndTable();
	}

    void NpcTab::drawMinaTabLists() {
        CImGui::BeginTable("##MinaPresetList", 1, tableFlags);
        drawPresetSelectTableEntry(wsSymbolFont,
            "##MinaPresetCombo_Chatacabra", "Chatacabra Armour",
            dataManager.getPresetByUUID(dataManager.supportHunterConfigs().mina.chatacabra),
            editMinaChatacabraCb,
            namedNpcSelectableHeight);
        CImGui::EndTable();
    }

    void NpcTab::drawKaiTabLists() {
        CImGui::BeginTable("##KaiPresetList", 1, tableFlags);
        drawPresetSelectTableEntry(wsSymbolFont,
            "##KaiPresetCombo_Ingot", "Ingot Armour",
            dataManager.getPresetByUUID(dataManager.supportHunterConfigs().kai.ingot),
            editKaiIngotCb,
            namedNpcSelectableHeight);
        CImGui::EndTable();
	}

    void NpcTab::drawGriffinTabLists() {
        CImGui::BeginTable("##GriffinPresetList", 1, tableFlags);
        drawPresetSelectTableEntry(wsSymbolFont,
            "##GriffinPresetCombo_Conga", "Conga Armour",
            dataManager.getPresetByUUID(dataManager.supportHunterConfigs().griffin.conga),
            editGriffinCongaCb,
            namedNpcSelectableHeight);
        CImGui::EndTable();
	}

    void NpcTab::drawNightmistTabLists() {
        CImGui::BeginTable("##NightmistPresetList", 1, tableFlags);
        drawPresetSelectTableEntry(wsSymbolFont,
            "##NightmistPresetCombo_Ingot", "Ingot Armour",
            dataManager.getPresetByUUID(dataManager.supportHunterConfigs().nightmist.ingot),
            editNightmistIngotCb,
            namedNpcSelectableHeight);
        CImGui::EndTable();
    }

    void NpcTab::drawFabiusTabLists() {
        CImGui::BeginTable("##FabiusPresetList", 1, tableFlags);
        drawPresetSelectTableEntry(wsSymbolFont,
            "##FabiusPresetCombo_DefaultOutfit", "Fabius' Outfit",
            dataManager.getPresetByUUID(dataManager.supportHunterConfigs().fabius.defaultOutfit),
            editFabiusDefaultOutfitCb,
            namedNpcSelectableHeight);
        CImGui::EndTable();
    }

    void NpcTab::drawNadiaTabLists() {
        CImGui::BeginTable("##NadiaPresetList", 1, tableFlags);
        drawPresetSelectTableEntry(wsSymbolFont,
            "##NadiaPresetCombo_DefaultOutfit", "Nadia's Outfit",
            dataManager.getPresetByUUID(dataManager.supportHunterConfigs().nadia.defaultOutfit),
            editNadiaDefaultOutfitCb,
            namedNpcSelectableHeight);
        CImGui::EndTable();
	}

    void NpcTab::drawOtherNpcsTab() {
        CImGui::BeginTable("##UnnamedNpcPresetGroupList", 1, tableFlags);
        drawPresetGroupSelectTableEntry(wsSymbolFont,
            "##UnnamedNpcPresetGroup_Male", "Male",
            dataManager.getPresetGroupByUUID(dataManager.npcDefaults().male),
            editMaleCb);
        drawPresetGroupSelectTableEntry(wsSymbolFont,
            "##UnnamedNpcPresetGroup_Female", "Female",
            dataManager.getPresetGroupByUUID(dataManager.npcDefaults().female),
            editFemaleCb);
        CImGui::EndTable();
    }

    void NpcTab::drawNpcListTabItem(const std::string& tabLabel, const std::function<void()>& drawFunc) {
        if (CImGui::BeginTabItem(tabLabel.c_str())) {
            CImGui::Spacing();
            drawFunc();
            CImGui::EndTabItem();
        }
        if (CImGui::IsItemClicked()) closePopouts();
	}

	void NpcTab::drawPopouts() {
        editPanel.draw();
        editDefaultPanel.draw();
    };

    void NpcTab::closePopouts() {
        editPanel.close();
        editDefaultPanel.close();
    }

    void NpcTab::openEditDefaultPanel(const std::function<void(std::string)>& onSelect) {
        editPanel.close(); // Close the other panel if its open
        editDefaultPanel.openNew("Select Default Preset Group", "EditDefaultPanel_NpcTab", dataManager, wsSymbolFont);
        editDefaultPanel.get()->focus();

        editDefaultPanel.get()->onSelectPresetGroup([&](std::string uuid) {
            INVOKE_REQUIRED_CALLBACK(onSelect, uuid);
            editDefaultPanel.close();
        });
    }

    void NpcTab::openEditPanel(const std::function<void(std::string)>& onSelect) {
        editDefaultPanel.close(); // Close the other panel if its open
        editPanel.openNew("Select Preset", "EditPanel_NpcTab", dataManager, wsSymbolFont, wsArmourFont);
        editPanel.get()->focus();

        editPanel.get()->onSelectPreset([&](std::string uuid) {
            INVOKE_REQUIRED_CALLBACK(onSelect, uuid);
            editPanel.close();
        });
    }

}