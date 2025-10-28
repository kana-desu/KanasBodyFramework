#include <kbf/gui/tabs/player/player_tab.hpp>

#include <kbf/data/ids/font_symbols.hpp>

#include <kbf/gui/shared/tab_bar_separator.hpp>
#include <kbf/gui/shared/styling_consts.hpp>
#include <kbf/gui/shared/preset_selectors.hpp>
#include <kbf/gui/shared/alignment.hpp>

#include <kbf/util/id/uuid_generator.hpp>
#include <kbf/util/string/to_lower.hpp>
#include <kbf/util/font/default_font_sizes.hpp>

#include <vector>
#include <algorithm>

namespace kbf {

    void PlayerTab::draw() {
        drawTabBarSeparator("Defaults", "PlayerTabDefaults");

        CImGui::PushStyleVar(ImGuiStyleVar_CellPadding, LIST_PADDING);

        drawDefaults();
        CImGui::Spacing();
        drawTabBarSeparator("Overrides", "PlayerTabOverrides");
        drawOverrideList();

        CImGui::PopStyleVar();
    }

    void PlayerTab::drawPopouts() {
        editDefaultPanel.draw();
        addPlayerOverridePanel.draw();
        editPlayerOverridePanel.draw();
    }

    void PlayerTab::closePopouts() {
        editDefaultPanel.close();
        addPlayerOverridePanel.close();
        editPlayerOverridePanel.close();
	}

    void PlayerTab::drawDefaults() {
        constexpr ImGuiTableFlags tableFlags = ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_PadOuterX;

        CImGui::BeginTable("##PlayerDefaultPresetGroupList", 1, tableFlags);
        CImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(LIST_PADDING.x, 0.0f));

        drawPresetGroupSelectTableEntry(wsSymbolFont, 
            "##PlayerPresetGroup_Male", "Male",  
            dataManager.getPresetGroupByUUID(dataManager.playerDefaults().male), 
            editMaleCb);
        drawPresetGroupSelectTableEntry(wsSymbolFont, 
            "##PlayerPresetGroup_Female", "Female", 
            dataManager.getPresetGroupByUUID(dataManager.playerDefaults().female),
            editFemaleCb);

        CImGui::PopStyleVar();
        CImGui::EndTable();

    }

    void PlayerTab::drawOverrideList() {
        const ImVec2 buttonSize = ImVec2(CImGui::GetContentRegionAvail().x, 0.0f);
        if (CImGui::Button("Add Player Override", buttonSize)) {
            openAddPlayerOverridePanel();
        }

        std::vector<const PlayerOverride*> playerOverrides = dataManager.getPlayerOverrides();

        static bool sortDirAscending;
        static enum class SortCol {
            NONE,
            NAME
        } sortCol;

        // Sort
        switch (sortCol)
        {
        case SortCol::NAME:
            std::sort(playerOverrides.begin(), playerOverrides.end(), [&](const PlayerOverride* a, const PlayerOverride* b) {
                std::string lowa = toLower(a->player.name); std::string lowb = toLower(b->player.name);
                return sortDirAscending ? lowa < lowb : lowa > lowb;
                });
        }

        if (playerOverrides.size() == 0) {
            CImGui::Spacing();
            CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
            constexpr char const* noOverrideStr = "No Existing Overrides";
            preAlignCellContentHorizontal(noOverrideStr);
            CImGui::Text(noOverrideStr);
            CImGui::PopStyleColor();
        }
        else {
            constexpr ImGuiTableFlags playerOverrideTableFlags =
                ImGuiTableFlags_BordersInnerH
                | ImGuiTableFlags_BordersInnerV
                | ImGuiTableFlags_PadOuterX
                | ImGuiTableFlags_Sortable
                | ImGuiTableFlags_ScrollY;
            CImGui::BeginTable("##PlayerOverrideList", 1, playerOverrideTableFlags);

            constexpr ImGuiTableColumnFlags stretchSortFlags =
                ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthStretch;

            CImGui::TableSetupColumn("Player", stretchSortFlags, 0.0f);
            CImGui::TableSetupScrollFreeze(0, 1);
            CImGui::TableHeadersRow();

            // Sorting for preset group name
            if (ImGuiTableSortSpecs* sort_specs = CImGui::TableGetSortSpecs()) {
                if (sort_specs->SpecsDirty && sort_specs->SpecsCount > 0) {
                    const ImGuiTableColumnSortSpecs& sort_spec = sort_specs->Specs[0];
                    sortDirAscending = sort_spec.SortDirection == ImGuiSortDirection_Ascending;

                    switch (sort_spec.ColumnIndex)
                    {
                    case 0:  sortCol = SortCol::NAME; break;
                    default: sortCol = SortCol::NONE; break;
                    }

                    sort_specs->SpecsDirty = false;
                }
            }

            CImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(LIST_PADDING.x, 0.0f));

            float contentRegionWidth = CImGui::GetContentRegionAvail().x;
            for (const PlayerOverride* override : playerOverrides) {
                CImGui::TableNextRow();

                CImGui::TableNextColumn();

                constexpr float selectableHeight = 60.0f;
                ImVec2 pos = CImGui::GetCursorScreenPos();
                if (CImGui::Selectable(("##Selectable_" + override->player.string()).c_str(), false, 0, ImVec2(0.0f, selectableHeight))) {
                    openEditPlayerOverridePanel(override->player);
                }

                // Sex Mark
                std::string sexMarkSymbol = override->player.female ? WS_FONT_FEMALE : WS_FONT_MALE;
                ImVec4 sexMarkerCol = override->player.female ? ImVec4(0.76f, 0.50f, 0.24f, 1.0f) : ImVec4(0.50f, 0.70f, 0.33f, 1.0f);

                CImGui::PushFont(wsSymbolFont, FONT_SIZE_DEFAULT_WILDS_SYMBOLS);

                constexpr float sexMarkerSpacingAfter = 5.0f;
                constexpr float sexMarkerVerticalAlignOffset = 5.0f;
                ImVec2 sexMarkerSize = CImGui::CalcTextSize(sexMarkSymbol.c_str());
                ImVec2 sexMarkerPos;
                sexMarkerPos.x = pos.x + CImGui::GetStyle().ItemSpacing.x;
                sexMarkerPos.y = pos.y + (selectableHeight - sexMarkerSize.y) * 0.5f + sexMarkerVerticalAlignOffset;
                CImGui::GetWindowDrawList()->AddText(sexMarkerPos, CImGui::GetColorU32(sexMarkerCol), sexMarkSymbol.c_str());

                CImGui::PopFont();

                // Player name
                constexpr float playerNameSpacingAfter = 5.0f;
                ImVec2 playerNameSize = CImGui::CalcTextSize(override->player.name.c_str());
                ImVec2 playerNamePos;
                playerNamePos.x = sexMarkerPos.x + sexMarkerSize.x + sexMarkerSpacingAfter;
                playerNamePos.y = pos.y + (selectableHeight - playerNameSize.y) * 0.5f;
                CImGui::GetWindowDrawList()->AddText(playerNamePos, CImGui::GetColorU32(ImGuiCol_Text), override->player.name.c_str());

                // Preset Group
                const PresetGroup* activeGroup = dataManager.getPresetGroupByUUID(override->presetGroup);

                // Sex Mark
                bool female = false;
                float presetGroupSexMarkSpacing = CImGui::GetStyle().ItemSpacing.x;
                float presetGroupSexMarkSpacingBefore = 0.0f;
                float endPos = CImGui::GetCursorScreenPos().x + CImGui::GetContentRegionAvail().x;
                if (activeGroup) {
                    bool female = activeGroup->female;
                    presetGroupSexMarkSpacing = 15.0f;
                    presetGroupSexMarkSpacingBefore = 10.0f;

                    std::string presetGroupSexMarkSymbol = female ? WS_FONT_FEMALE : WS_FONT_MALE;
                    ImVec4 presetGroupSexMarkerCol = female ? ImVec4(0.76f, 0.50f, 0.24f, 1.0f) : ImVec4(0.50f, 0.70f, 0.33f, 1.0f);

                    ImVec2 presetGroupSexMarkerSize = CImGui::CalcTextSize(presetGroupSexMarkSymbol.c_str());
                    ImVec2 presetGroupSexMarkerPos;
                    presetGroupSexMarkerPos.x = endPos - presetGroupSexMarkSpacingBefore - CImGui::GetStyle().ItemSpacing.x;
                    presetGroupSexMarkerPos.y = pos.y + (selectableHeight - presetGroupSexMarkerSize.y) * 0.5f;

                    CImGui::PushFont(wsSymbolFont, FONT_SIZE_DEFAULT_WILDS_SYMBOLS);
                    CImGui::GetWindowDrawList()->AddText(presetGroupSexMarkerPos, CImGui::GetColorU32(presetGroupSexMarkerCol), presetGroupSexMarkSymbol.c_str());
                    CImGui::PopFont();
                }

                // Group Name
                std::string presetGroupName = override->presetGroup.empty() || activeGroup == nullptr ? "Default" : activeGroup->name;
                ImVec2 currentGroupStrSize = CImGui::CalcTextSize(presetGroupName.c_str());
                ImVec2 currentGroupStrPos;
                currentGroupStrPos.x = endPos - (currentGroupStrSize.x + presetGroupSexMarkSpacing + presetGroupSexMarkSpacingBefore);
                currentGroupStrPos.y = pos.y + (selectableHeight - currentGroupStrSize.y) * 0.5f;

                ImVec4 presetGroupCol = activeGroup == nullptr ? ImVec4(0.365f, 0.678f, 0.886f, 0.8f) : ImVec4(1.0f, 1.0f, 1.0f, 0.5f);
                CImGui::PushStyleColor(ImGuiCol_Text, presetGroupCol);
                CImGui::GetWindowDrawList()->AddText(currentGroupStrPos, CImGui::GetColorU32(ImGuiCol_Text), presetGroupName.c_str());
                CImGui::PopStyleColor();

                // Hunter ID
                std::string hunterIdStr = "(" + override->player.hunterId + ")";
                ImVec2 hunterIdStrSize = CImGui::CalcTextSize(hunterIdStr.c_str());
                ImVec2 hunterIdStrPos;
                hunterIdStrPos.x = playerNamePos.x + playerNameSize.x + playerNameSpacingAfter;
                hunterIdStrPos.y = pos.y + (selectableHeight - hunterIdStrSize.y) * 0.5f;
                CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
                CImGui::GetWindowDrawList()->AddText(hunterIdStrPos, CImGui::GetColorU32(ImGuiCol_Text), hunterIdStr.c_str());
                CImGui::PopStyleColor();
            }

            CImGui::PopStyleVar();
            CImGui::EndTable();
        }
    }

    void PlayerTab::openEditDefaultPanel(const std::function<void(std::string)>& onSelect) {
        editDefaultPanel.openNew("Select Default Preset Group", "EditDefaultPanel_PlayerTab", dataManager, wsSymbolFont);
        editDefaultPanel.get()->focus();

        editDefaultPanel.get()->onSelectPresetGroup([&](std::string uuid) {
            INVOKE_REQUIRED_CALLBACK(onSelect, uuid);
            editDefaultPanel.close();
        });
    }

    void PlayerTab::openAddPlayerOverridePanel() {
        addPlayerOverridePanel.openNew("Select Player to Override", "AddPlayerOverridePanel", playerTracker, wsSymbolFont);
        addPlayerOverridePanel.get()->focus();

        addPlayerOverridePanel.get()->onSelectPlayer([&](PlayerData playerData) {
            PlayerOverride newOverride{};
            newOverride.player = playerData;
            newOverride.presetGroup = "";

            dataManager.addPlayerOverride(newOverride);
            addPlayerOverridePanel.close();
        });

        addPlayerOverridePanel.get()->onCheckDisablePlayer([&](const PlayerData& playerData) {
            return dataManager.playerOverrideExists(playerData);
        });

        addPlayerOverridePanel.get()->onRequestDisabledPlayerTooltip([&]() { return "An override already exists for this player"; });
    }

    void PlayerTab::openEditPlayerOverridePanel(const PlayerData& playerData) {
        std::string windowName = std::format("Edit Player Override - {}", playerData.name);
        editPlayerOverridePanel.openNew(playerData, windowName, "EditPlayerOverridePanel", dataManager, wsSymbolFont);

        editPlayerOverridePanel.get()->focus();

        editPlayerOverridePanel.get()->onCancel([&]() {
            editPlayerOverridePanel.close();
        });

        editPlayerOverridePanel.get()->onDelete([&](const PlayerData& playerData) {
            dataManager.deletePlayerOverride(playerData);
            editPlayerOverridePanel.close();
        });

        editPlayerOverridePanel.get()->onUpdate([&](const PlayerData& playerData, PlayerOverride newOverride) {
            dataManager.updatePlayerOverride(playerData, newOverride);
            editPlayerOverridePanel.close();
        });

    }

}