#include <kbf/gui/kbf_window.hpp>

#include <kbf/gui/shared/tab_bar_separator.hpp>
#include <kbf/gui/shared/preset_selectors.hpp>
#include <kbf/gui/shared/styling_consts.hpp>
#include <kbf/gui/shared/alignment.hpp>

#include <kbf/debug/debug_stack.hpp>
#include <kbf/data/ids/font_symbols.hpp>
#include <kbf/data/ids/special_armour_ids.hpp>
#include <kbf/data/preset/preset_group.hpp>
#include <kbf/data/preset/player_override.hpp>
#include <kbf/util/string/to_lower.hpp>

#include <vector>
#include <algorithm>
#include <ctime>

#define KBF_WINDOW_LOG_TAG "[KBFWindow]"

namespace kbf {

    void KBFWindow::initialize() {
        initializeFonts();

        // Set required callbacks
        presetsTab.onOpenPresetInEditor([&](std::string uuid) { 
            editorTab.editPreset(dataManager.getPresetByUUID(uuid)); 
            tab = KBFTab::Editor; });
        presetGroupsTab.onOpenPresetGroupInEditor([&](std::string uuid) { 
            editorTab.editPresetGroup(dataManager.getPresetGroupByUUID(uuid)); 
            tab = KBFTab::Editor; });

        // Presets may be deleted, so avoid hanging ptr
        settingsTab.setOnReloadDataCallback([&]() { editorTab.editNone(); });

		DEBUG_STACK.push(std::format("{} Hello from Kana! ^o^ - Framework initialized.", KBF_WINDOW_LOG_TAG), DebugStack::Color::INFO);
    }

    void KBFWindow::initializeFonts() {
        // TODO: Japanese tilde not loaded
        // TODO: certain european symbols, e.g. capital s with v hatnot loaded, g with v hat not loaded.
        // TODO: Very tall i not supported

        const std::string mainFontPathJP      = (dataManager.dataBasePath / "assets/fonts/NotoSansCJKjp-Regular.otf").string();
        const std::string mainFontPathKR      = (dataManager.dataBasePath / "assets/fonts/NotoSansCJKkr-Regular.otf").string();
        const std::string mainFontPathTC      = (dataManager.dataBasePath / "assets/fonts/NotoSansCJKtc-Regular.otf").string();
        const std::string mainFontPathSC      = (dataManager.dataBasePath / "assets/fonts/NotoSansCJKsc-Regular.otf").string();
        const std::string wildsSymbolFontPath = (dataManager.dataBasePath / "assets/fonts/Roboto-Regular-WS-Symbols.ttf").string();
        const std::string wildsArmourFontPath = (dataManager.dataBasePath / "assets/fonts/Roboto-Regular-WS-Armour.ttf").string();
        const std::string monoFontPathJP      = (dataManager.dataBasePath / "assets/fonts/NotoSansMonoCJKjp-Regular.otf").string();
        const std::string monoFontPathKR      = (dataManager.dataBasePath / "assets/fonts/NotoSansMonoCJKkr-Regular.otf").string();
        const std::string monoFontPathTC      = (dataManager.dataBasePath / "assets/fonts/NotoSansMonoCJKtc-Regular.otf").string();
        const std::string monoFontPathSC      = (dataManager.dataBasePath / "assets/fonts/NotoSansMonoCJKsc-Regular.otf").string();

        std::vector<std::string> unfoundFonts = {};
        if (!std::filesystem::exists(mainFontPathJP))      unfoundFonts.push_back(mainFontPathJP);
        if (!std::filesystem::exists(mainFontPathKR))      unfoundFonts.push_back(mainFontPathKR);
        if (!std::filesystem::exists(mainFontPathTC))      unfoundFonts.push_back(mainFontPathTC);
		if (!std::filesystem::exists(mainFontPathSC))      unfoundFonts.push_back(mainFontPathSC);
        if (!std::filesystem::exists(wildsSymbolFontPath)) unfoundFonts.push_back(wildsSymbolFontPath);
        if (!std::filesystem::exists(wildsArmourFontPath)) unfoundFonts.push_back(wildsArmourFontPath);
        if (!std::filesystem::exists(monoFontPathJP))      unfoundFonts.push_back(monoFontPathJP);
        if (!std::filesystem::exists(monoFontPathKR))      unfoundFonts.push_back(monoFontPathKR);
        if (!std::filesystem::exists(monoFontPathTC))      unfoundFonts.push_back(monoFontPathTC);
		if (!std::filesystem::exists(monoFontPathSC))      unfoundFonts.push_back(monoFontPathSC);

        if (unfoundFonts.size() > 0) {
            std::string errMsg = "Failed to find required font files:\n";
            for (size_t i = 0; i < unfoundFonts.size(); i++) {
                errMsg += " - " + unfoundFonts[i];
                if (i < unfoundFonts.size() - 1) errMsg += "\n";
            }
            DEBUG_STACK.push(std::format("{} {}", KBF_WINDOW_LOG_TAG, errMsg), DebugStack::Color::ERROR);
            return;
        }

		ImGuiIO* io = CImGui::GetIO();
        assert(io != nullptr && "Failed to get IO");
		ImFontAtlas* fonts = io->Fonts;
        assert(fonts != nullptr && "Fonts not initialized??");

        wildsSymbolsFont = CImGui::AddFontFromFileTTF(fonts, wildsSymbolFontPath.c_str());
        wildsArmourFont  = CImGui::AddFontFromFileTTF(fonts, wildsArmourFontPath.c_str());

        ImFontConfig mergeScCfg{}; mergeScCfg.MergeMode = true; mergeScCfg.FontDataOwnedByAtlas = false;
		ImFontConfig mergeKrCfg{}; mergeKrCfg.MergeMode = true; mergeKrCfg.FontDataOwnedByAtlas = false;
		ImFontConfig mergeJpCfg{}; mergeJpCfg.MergeMode = true; mergeJpCfg.FontDataOwnedByAtlas = false;

        mainFont = CImGui::AddFontFromFileTTF(fonts, mainFontPathTC.c_str(), 0.0f);
                   CImGui::AddFontFromFileTTF(fonts, mainFontPathSC.c_str(), 0.0f, &mergeScCfg);
                   CImGui::AddFontFromFileTTF(fonts, mainFontPathKR.c_str(), 0.0f, &mergeKrCfg);
                   CImGui::AddFontFromFileTTF(fonts, mainFontPathJP.c_str(), 0.0f, &mergeJpCfg);

        ImFontConfig mergeMonoScCfg{}; mergeMonoScCfg.MergeMode = true; mergeMonoScCfg.FontDataOwnedByAtlas = false;
		ImFontConfig mergeMonoKrCfg{}; mergeMonoKrCfg.MergeMode = true; mergeMonoKrCfg.FontDataOwnedByAtlas = false;
		ImFontConfig mergeMonoJpCfg{}; mergeMonoJpCfg.MergeMode = true; mergeMonoJpCfg.FontDataOwnedByAtlas = false;

        monoFont = CImGui::AddFontFromFileTTF(fonts, monoFontPathTC.c_str(), 0.0f);
                   CImGui::AddFontFromFileTTF(fonts, monoFontPathSC.c_str(), 0.0f, &mergeMonoScCfg);
                   CImGui::AddFontFromFileTTF(fonts, monoFontPathKR.c_str(), 0.0f, &mergeMonoKrCfg);
                   CImGui::AddFontFromFileTTF(fonts, monoFontPathJP.c_str(), 0.0f, &mergeMonoJpCfg);

        monoFontTiny = monoFont; // deal with legacy 

        // Deferred initialization of any tabs that need special fonts.
        playerTab.setSymbolFont(wildsSymbolsFont);
        playerTab.setArmourFont(wildsArmourFont);
        npcTab.setSymbolFont(wildsSymbolsFont);
        npcTab.setArmourFont(wildsArmourFont);
        presetGroupsTab.setSymbolFont(wildsSymbolsFont);
        presetsTab.setSymbolFont(wildsSymbolsFont);
        presetsTab.setArmourFont(wildsArmourFont);
        editorTab.setSymbolFont(wildsSymbolsFont);
        editorTab.setArmourFont(wildsArmourFont);
		shareTab.setSymbolFont(wildsSymbolsFont);
		shareTab.setArmourFont(wildsArmourFont);
		debugTab.setArmourFont(wildsArmourFont);
		debugTab.setSymbolFont(wildsSymbolsFont);
        debugTab.setMonoFont(monoFont);
        aboutTab.setMonoFontTiny(monoFontTiny);
        aboutTab.setMonoFont(monoFont);

        dataManager.setRegularFontOverride(mainFont);
    }

    void KBFWindow::draw(bool* open) {
        //CImGui::ShowDemoWindow();

        CImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 7));
        CImGui::PushStyleVar(ImGuiStyleVar_TabBarBorderSize, 2.0f);

        CImGui::SetNextWindowSize(ImVec2(700, 800), ImGuiCond_FirstUseEver);
        CImGui::SetNextWindowSizeConstraints(ImVec2(700, 500), ImVec2(FLT_MAX, FLT_MAX));
        CImGui::Begin("Kana's Body Framework", open, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse);
        CImGui::PushFont(mainFont, FONT_SIZE_DEFAULT_MAIN);

        drawMenuBar();
        CImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 5));
        drawTab();
        drawPopouts();
        CImGui::PopStyleVar();

        CImGui::PopFont();

        CImGui::End();
        CImGui::PopStyleVar(2);
    }

    void KBFWindow::drawKbfMenuItem(const std::string& label, const KBFTab tabId) {
        if (CImGui::MenuItem(label.c_str(), nullptr, tab == tabId)) { 
			cleanupTab(tab); // Cleanup previous tab's popouts
            tab = tabId; 
        }
    }

    void KBFWindow::drawMenuBar() {
        if (CImGui::BeginMenuBar()) {
            // Deal with the padding fucking the alignment up (why imgui why)
            CImGui::SetCursorPosY(CImGui::GetCursorPosY() - 2.5f);
            drawKbfMenuItem("Players",       KBFTab::Players);
            drawKbfMenuItem("NPCs",          KBFTab::NPCs);
            drawKbfMenuItem("Preset Groups", KBFTab::PresetGroups);
            drawKbfMenuItem("Presets",       KBFTab::Presets);
            drawKbfMenuItem("Editor",        KBFTab::Editor);
			drawKbfMenuItem("Share",         KBFTab::Share);
            drawKbfMenuItem("Settings",      KBFTab::Settings);
            drawKbfMenuItem("Debug",         KBFTab::Debug);
            drawKbfMenuItem("About",         KBFTab::About);
            CImGui::EndMenuBar();
        }
    }

    void KBFWindow::drawTab() {
        switch (tab) {
        case KBFTab::Players:
            playerTab.draw();
            break;
        case KBFTab::NPCs:
            npcTab.draw();
            break;
        case KBFTab::PresetGroups:
            presetGroupsTab.draw();
            break;
        case KBFTab::Presets:
            presetsTab.draw();
            break;
        case KBFTab::Editor:
            editorTab.draw();
            break;
        case KBFTab::Share:
            shareTab.draw();
            break;
        case KBFTab::Settings:
            settingsTab.draw();
            break;
        case KBFTab::Debug:
            debugTab.draw();
            break;
        case KBFTab::About:
            aboutTab.draw();
            break;
        }
    }

    void KBFWindow::drawPopouts() {
        playerTab.drawPopouts();
        npcTab.drawPopouts();
        presetsTab.drawPopouts();
        presetGroupsTab.drawPopouts();
        editorTab.drawPopouts();
        shareTab.drawPopouts();
        settingsTab.drawPopouts();
        debugTab.drawPopouts();
        aboutTab.drawPopouts();
    }

    void KBFWindow::cleanupTab(KBFTab tab) {
        switch (tab) {
        case KBFTab::Players:
            playerTab.closePopouts();
            break;
        case KBFTab::NPCs:
            npcTab.closePopouts();
            break;
        case KBFTab::PresetGroups:
            presetGroupsTab.closePopouts();
            break;
        case KBFTab::Presets:
            presetsTab.closePopouts();
            break;
        case KBFTab::Editor:
            editorTab.closePopouts();
            break;
        case KBFTab::Share:
            shareTab.closePopouts();
            break;
        case KBFTab::Settings:
            settingsTab.closePopouts();
            break;
        case KBFTab::Debug:
            debugTab.closePopouts();
            break;
        case KBFTab::About:
            aboutTab.closePopouts();
            break;
		}
    }

}