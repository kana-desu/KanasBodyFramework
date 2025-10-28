#include <kbf/gui/tabs/share/share_tab.hpp>

#include <kbf/gui/shared/tab_bar_separator.hpp>
#include <kbf/gui/shared/styling_consts.hpp>
#include <kbf/gui/shared/delete_button.hpp>
#include <kbf/gui/shared/alignment.hpp>

#include <kbf/util/io/noc_impl.hpp>

#include <thread>

namespace kbf {

	void ShareTab::draw() {
        CImGui::PushStyleVar(ImGuiStyleVar_CellPadding, LIST_PADDING);

        processCallbacks();

        const ImVec2 buttonSize = ImVec2(CImGui::GetContentRegionAvail().x, 0.0f);
        if (importDialogOpen) CImGui::BeginDisabled();
        if (CImGui::Button("Import KBF File", buttonSize)) {
            std::thread([this]() {
                std::string filepath = getImportFileDialog();
                if (!filepath.empty()) {
                    postToMainThread([this, filepath]() {
                        size_t nConflicts = 0;
                        bool success = dataManager.importKBF(filepath, &nConflicts);
                        openImportInfoPanel(success, nConflicts);
                    });
                }
                importDialogOpen = false;
            }).detach();
        }
        CImGui::SetItemTooltip("Import Presets / Preset Groups / Player Overrides via a .kbf file");
        if (importDialogOpen) CImGui::EndDisabled();

        CImGui::Spacing();
        CImGui::Separator();
        CImGui::Spacing();

        if (CImGui::Button("Export KBF File", buttonSize)) {
            openExportFilePanel();
        }
		CImGui::SetItemTooltip("Generate a .kbf file to share specific presets, preset groups, and/or player overrides. Best for single-time use");

        if (CImGui::Button("Export Mod Archive", buttonSize)) {
            openExportModArchivePanel();
        }
        CImGui::SetItemTooltip("Generate a .zip file for presets, etc. that can be installed by extracting into the game directory");

        CImGui::Spacing();

        drawTabBarSeparator("Installed Mod Archives", "ModArchiveSep");

        drawModArchiveList();

        CImGui::PopStyleVar();
	}

    void ShareTab::drawModArchiveList() {
        std::unordered_map<std::string, KBFDataManager::ModArchiveCounts> modInfos = dataManager.getModArchiveInfo();

        if (modInfos.size() == 0) {
            CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
            constexpr char const* noPresetStr = "\nNo Installed Mod Archives Found.";
            CImGui::Spacing();
            preAlignCellContentHorizontal(noPresetStr);
            CImGui::Text(noPresetStr);
            CImGui::PopStyleColor();
            return;
        }


        std::vector<std::string> modNames;
		for (const auto& [name, _] : modInfos) modNames.push_back(name);
        
        constexpr float deleteButtonScale = 1.2f;
        constexpr float linkButtonScale = 1.0f;
        constexpr float sliderWidth = 80.0f;
        constexpr float sliderSpeed = 0.01f;
        constexpr float tableVpad = 2.5f;

        constexpr ImGuiTableFlags modTableFlags =
            ImGuiTableFlags_PadOuterX
            | ImGuiTableFlags_BordersInnerH
            | ImGuiTableFlags_Sortable;
        CImGui::BeginTable("##ModArchiveList", 2, modTableFlags);

        constexpr ImGuiTableColumnFlags stretchSortFlags =
            ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthStretch;
        constexpr ImGuiTableColumnFlags fixedNoSortFlags =
            ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthFixed;

        CImGui::TableSetupColumn("", fixedNoSortFlags, 0.0f);
        CImGui::TableSetupColumn("Mod", stretchSortFlags, 0.0f);
        CImGui::TableHeadersRow();

        CImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(LIST_PADDING.x, tableVpad));

        float contentRegionWidth = CImGui::GetContentRegionAvail().x;

        // Sort - this is kinda horrible.
        static bool sortDirAscending = true;
        static bool sort = false;

        if (sort) std::sort(modNames.begin(), modNames.end());

        if (ImGuiTableSortSpecs* sort_specs = CImGui::TableGetSortSpecs()) {
            if (sort_specs->SpecsDirty && sort_specs->SpecsCount > 0) {
                const ImGuiTableColumnSortSpecs& sort_spec = sort_specs->Specs[0];
                sortDirAscending = sort_spec.SortDirection == ImGuiSortDirection_Ascending;

                switch (sort_spec.ColumnIndex)
                {
                case 1: sort = true;
                }

                sort_specs->SpecsDirty = false;
            }
        }

        std::vector<std::string> modsToDelete{};
        for (const std::string& name : modNames) {
            const auto& counts = modInfos.at(name);

            CImGui::TableNextRow();

            CImGui::TableSetColumnIndex(0);
            CImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
            CImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.1f, 0.1f, 1.0f));
            // Note: This doesn't seem to work without the manual adjustment
            CImGui::SetCursorPosY(CImGui::GetCursorPosY() + (CImGui::GetFrameHeight() - CImGui::GetFontSize() * deleteButtonScale) * 0.5f + 16.5f);
            if (ImDeleteButton(("##del_" + name).c_str(), deleteButtonScale)) {
                modsToDelete.push_back(name);
            }
            CImGui::PopStyleColor(2);

            CImGui::TableSetColumnIndex(1);

            constexpr float selectableHeight = 60.0f;
            float endPos = CImGui::GetCursorScreenPos().x + CImGui::GetContentRegionAvail().x;
            ImVec2 pos = CImGui::GetCursorScreenPos();

            CImGui::Selectable(("##Selectable_" + name).c_str(), false, 0, ImVec2(0.0f, selectableHeight));

            // Player name
            constexpr float playerNameSpacingAfter = 5.0f;
            ImVec2 playerNameSize = CImGui::CalcTextSize(name.c_str());
            ImVec2 playerNamePos;
            playerNamePos.x = pos.x;
            playerNamePos.y = pos.y + (selectableHeight - playerNameSize.y) * 0.5f;
            CImGui::GetWindowDrawList()->AddText(playerNamePos, CImGui::GetColorU32(ImGuiCol_Text), name.c_str());

            // Hunter ID
            std::string countsStr = std::format("({} / {} / {})", counts.presetGroups, counts.presets, counts.playerOverrides);
            ImVec2 countsStrSize = CImGui::CalcTextSize(countsStr.c_str());
            ImVec2 countsStrPos;
            countsStrPos.x = endPos - (countsStrSize.x + CImGui::GetStyle().ItemSpacing.x);
            countsStrPos.y = pos.y + (selectableHeight - countsStrSize.y) * 0.5f;

            CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
            CImGui::GetWindowDrawList()->AddText(countsStrPos, CImGui::GetColorU32(ImGuiCol_Text), countsStr.c_str());
            CImGui::PopStyleColor();
        }

        for (const std::string& mod : modsToDelete) {
            dataManager.deleteLocalModArchive(mod);
        }

        CImGui::PopStyleVar();
        CImGui::EndTable();
    }

	void ShareTab::drawPopouts() {
        exportPanel.draw();
        importInfoPanel.draw();
        exportInfoPanel.draw();
    }

	void ShareTab::closePopouts() {
        exportPanel.close();
        importInfoPanel.close();
        exportInfoPanel.close();
    }

    std::string ShareTab::getImportFileDialog() {
        importDialogOpen = true;
        const char* pth = noc_file_dialog_open(NOC_FILE_DIALOG_OPEN, "kbf\0*.kbf\0\0", dataManager.exportsPath.string().c_str(), "");

        if (pth == nullptr) return "";
        return std::string{ pth };
    }

    void ShareTab::postToMainThread(std::function<void()> func) {
        std::lock_guard<std::mutex> lock(callbackMutex);
        callbackQueue.push(std::move(func));
    }

    void ShareTab::processCallbacks() {
        std::lock_guard<std::mutex> lock(callbackMutex);
        while (!callbackQueue.empty()) {
            auto func = std::move(callbackQueue.front());
            callbackQueue.pop();
            func();
        }
    }

    void ShareTab::openExportFilePanel() {
		exportPanel.openNew("Export KBF File", "ExportFilePanel", false, dataManager, wsSymbolFont, wsArmourFont);
        exportPanel.get()->focus();
        exportPanel.get()->onCancel([&]() {
            exportPanel.close();
        });
        exportPanel.get()->onCreate([&](std::string filepath, KBFFileData data) {
            bool success = dataManager.writeKBF(filepath, data);
            exportPanel.close();
			openExportInfoPanel(success);
        });
    }

    void ShareTab::openExportModArchivePanel() {
        exportPanel.openNew("Export Mod Archive", "ExportFilePanel", true, dataManager, wsSymbolFont, wsArmourFont);
        exportPanel.get()->focus();
        exportPanel.get()->onCancel([&]() {
            exportPanel.close();
        });
        exportPanel.get()->onCreate([&](std::string filepath, KBFFileData data) {
            dataManager.writeModArchive(filepath, data);
            exportPanel.close();
        });
    }

    void ShareTab::openImportInfoPanel(bool success, size_t nConflicts) {
        std::vector<std::string> messages = {};

        if (success) {
            messages.push_back(".KBF Imported Successfully.");
            if (nConflicts > 0)
                messages.push_back("Note: " + std::to_string(nConflicts) + " items were not imported as matching items already exist. Please check Debug > Log for details.");
        }
        else {
            messages.push_back("Failed to Import .KBF File. Please check Debug > Log and ensure the file is valid.");
        }

        importInfoPanel.openNew(
            success ? "Import Success" : "Import Error",
            "ImportInfoPanel", 
			messages
        );
        importInfoPanel.get()->focus();
        importInfoPanel.get()->onOk([&]() {
            importInfoPanel.close();
		});
    }

    void ShareTab::openExportInfoPanel(bool success) {
        std::vector<std::string> messages = {};
        if (success) {
            messages.push_back("Exported Successfully.");
        }
        else {
            messages.push_back("Failed to Export. Please check Debug > Log for details.");
        }
        exportInfoPanel.openNew(
            success ? "Export Success" : "Export Error",
            "ExportInfoPanel",
            messages
        );
        exportInfoPanel.get()->focus();
        exportInfoPanel.get()->onOk([&]() {
            exportInfoPanel.close();
        });
    }
}