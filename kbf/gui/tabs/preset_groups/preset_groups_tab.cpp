#include <kbf/gui/tabs/preset_groups/preset_groups_tab.hpp>

#include <kbf/gui/shared/styling_consts.hpp>
#include <kbf/gui/shared/alignment.hpp>
#include <kbf/data/ids/font_symbols.hpp>
#include <kbf/data/preset/preset_group.hpp>
#include <kbf/util/string/to_lower.hpp>
#include <kbf/util/functional/invoke_callback.hpp>
#include <kbf/util/font/default_font_sizes.hpp>

#include <vector>
#include <algorithm>

namespace kbf {

	void PresetGroupsTab::draw() {

        const ImVec2 buttonSize = ImVec2(CImGui::GetContentRegionAvail().x, 0.0f);
        if (CImGui::Button("Create Preset Group", buttonSize)) {
            openCreatePresetGroupPanel();
        }

        if (CImGui::Button("Create From Preset Bundle", buttonSize)) {
            openCreatePresetGroupFromBundlePanel();
        }
        CImGui::SetItemTooltip("Automatically create a preset group using the assigned armour sets for presets within a specific bundle.");
        CImGui::Spacing();

        CImGui::BeginChild("PresetGroupListChild");
        drawPresetGroupList();
        CImGui::EndChild();
	}

	void PresetGroupsTab::drawPopouts() {
        createPresetGroupPanel.draw();
		createPresetGroupFromBundlePanel.draw();
        editPresetGroupPanel.draw();
    };

    void PresetGroupsTab::closePopouts() {
        createPresetGroupPanel.close();
		createPresetGroupFromBundlePanel.close();
        editPresetGroupPanel.close();
	}

    void PresetGroupsTab::drawPresetGroupList() {
        static bool sortDirAscending;
        static enum class SortCol {
            NONE,
            NAME
        } sortCol;

        std::vector<const PresetGroup*> presetGroups = dataManager.getPresetGroups("");

        // Sort
        switch (sortCol)
        {
        case SortCol::NAME:
            std::sort(presetGroups.begin(), presetGroups.end(), [&](const PresetGroup* a, const PresetGroup* b) {
                std::string lowa = toLower(a->name); std::string lowb = toLower(b->name);
                return sortDirAscending ? lowa < lowb : lowa > lowb;
            });
        }

        if (presetGroups.size() == 0) {
            CImGui::Spacing();
            CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
            constexpr char const* noPresetStr = "No Existing Preset Groups";
            preAlignCellContentHorizontal(noPresetStr);
            CImGui::Text(noPresetStr);
            CImGui::PopStyleColor();
        }
        else {
            CImGui::PushStyleVar(ImGuiStyleVar_CellPadding, LIST_PADDING);

            constexpr ImGuiTableFlags presetGroupTableFlags =
                ImGuiTableFlags_BordersInnerH
                | ImGuiTableFlags_BordersInnerV
                | ImGuiTableFlags_PadOuterX
                | ImGuiTableFlags_Sortable
                | ImGuiTableFlags_ScrollY;
            CImGui::BeginTable("##PresetGroupList", 1, presetGroupTableFlags);

            constexpr ImGuiTableColumnFlags stretchSortFlags =
                ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthStretch;

            CImGui::TableSetupColumn("Preset Group", stretchSortFlags, 0.0f);
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

            CImGui::PopStyleVar();
            CImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(LIST_PADDING.x, 0.0f));

            for (const PresetGroup* group : presetGroups) {
                CImGui::TableNextRow();
                CImGui::TableNextColumn();

                constexpr float selectableHeight = 60.0f;
                ImVec2 pos = CImGui::GetCursorScreenPos();
                if (CImGui::Selectable(("##Selectable_" + group->name).c_str(), false, 0, ImVec2(0.0f, selectableHeight))) {
                    openEditPresetGroupPanel(group->uuid);
                }

                // Sex Mark
                std::string sexMarkSymbol = group->female ? WS_FONT_FEMALE : WS_FONT_MALE;
                ImVec4 sexMarkerCol = group->female ? ImVec4(0.76f, 0.50f, 0.24f, 1.0f) : ImVec4(0.50f, 0.70f, 0.33f, 1.0f);

                CImGui::PushFont(wsSymbolFont, FONT_SIZE_DEFAULT_WILDS_SYMBOLS);

                constexpr float sexMarkerSpacingAfter = 5.0f;
                constexpr float sexMarkerVerticalAlignOffset = 5.0f;
                ImVec2 sexMarkerSize = CImGui::CalcTextSize(sexMarkSymbol.c_str());
                ImVec2 sexMarkerPos;
                sexMarkerPos.x = pos.x + CImGui::GetStyle().ItemSpacing.x;
                sexMarkerPos.y = pos.y + (selectableHeight - sexMarkerSize.y) * 0.5f + sexMarkerVerticalAlignOffset;
                CImGui::GetWindowDrawList()->AddText(sexMarkerPos, CImGui::GetColorU32(sexMarkerCol), sexMarkSymbol.c_str());

                CImGui::PopFont();

                // Group name... floating because imgui has no vertical alignment STILL :(
                constexpr float groupNameSpacingAfter = 5.0f;
                ImVec2 groupNameSize = CImGui::CalcTextSize(group->name.c_str());
                ImVec2 groupNamePos;
                groupNamePos.x = sexMarkerPos.x + sexMarkerSize.x + sexMarkerSpacingAfter;
                groupNamePos.y = pos.y + (selectableHeight - groupNameSize.y) * 0.5f;
                CImGui::GetWindowDrawList()->AddText(groupNamePos, CImGui::GetColorU32(ImGuiCol_Text), group->name.c_str());

                // Similarly for preset count
                const size_t nPresets = group->size();
                std::string rightText = std::to_string(nPresets);
                if (nPresets == 0) rightText = "Empty";
                else if (nPresets == 1) rightText += " Preset";
                else rightText += " Presets";

                ImVec2 rightTextSize = CImGui::CalcTextSize(rightText.c_str());
                float contentRegionWidth = CImGui::GetContentRegionAvail().x;
                float cursorPosX = CImGui::GetCursorScreenPos().x + contentRegionWidth - rightTextSize.x - CImGui::GetStyle().ItemSpacing.x;

                CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
                CImGui::GetWindowDrawList()->AddText(ImVec2(cursorPosX, groupNamePos.y), CImGui::GetColorU32(ImGuiCol_Text), rightText.c_str());
                CImGui::PopStyleColor();
            }

            CImGui::EndTable();
            CImGui::PopStyleVar();
        }
    }

    void PresetGroupsTab::openCreatePresetGroupPanel() {
        createPresetGroupPanel.openNew("Create Preset Group", "CreatePresetGroupPanel", dataManager, wsSymbolFont);
        createPresetGroupPanel.get()->focus();

        createPresetGroupPanel.get()->onCancel([&]() {
            createPresetGroupPanel.close();
        });

        createPresetGroupPanel.get()->onCreate([&](const PresetGroup& presetGroup) {
            dataManager.addPresetGroup(presetGroup);
            createPresetGroupPanel.close();
        });
    }

    void PresetGroupsTab::openCreatePresetGroupFromBundlePanel() {
        createPresetGroupFromBundlePanel.openNew("Create Preset Group From Bundle", "CreatePresetGroupFromBundlePanel", dataManager, wsSymbolFont);
        createPresetGroupFromBundlePanel.get()->focus();

        createPresetGroupFromBundlePanel.get()->onCancel([&]() {
            createPresetGroupFromBundlePanel.close();
        });

        createPresetGroupFromBundlePanel.get()->onCreate([&](const PresetGroup& presetGroup) {
            dataManager.addPresetGroup(presetGroup);
            createPresetGroupFromBundlePanel.close();
        });
    }

    void PresetGroupsTab::openEditPresetGroupPanel(const std::string& presetGroupUUID) {
        editPresetGroupPanel.openNew(presetGroupUUID, "Edit Preset Group", "EditPresetGroupPanel", dataManager, wsSymbolFont);
        editPresetGroupPanel.get()->focus();

        editPresetGroupPanel.get()->onDelete([&](const std::string& presetGroupUUID) {
            dataManager.deletePresetGroup(presetGroupUUID);
            editPresetGroupPanel.close();
        });

        editPresetGroupPanel.get()->onCancel([&]() {
            editPresetGroupPanel.close();
        });

        editPresetGroupPanel.get()->onUpdate([&](const std::string& presetGroupUUID, PresetGroup presetGroup) {
            dataManager.updatePresetGroup(presetGroupUUID, presetGroup);
            editPresetGroupPanel.close();
        });

        editPresetGroupPanel.get()->onOpenEditor([&](std::string presetGroupUUID) {
            editPresetGroupPanel.close();
            INVOKE_REQUIRED_CALLBACK(openPresetGroupInEditorCb, presetGroupUUID);
        });
    }

}