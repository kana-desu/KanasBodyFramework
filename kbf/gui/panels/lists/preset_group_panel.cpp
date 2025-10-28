#include <kbf/gui/panels/lists/preset_group_panel.hpp>

#include <kbf/data/ids/font_symbols.hpp>
#include <kbf/util/string/to_lower.hpp>
#include <kbf/util/functional/invoke_callback.hpp>
#include <kbf/gui/shared/alignment.hpp>
#include <kbf/util/font/default_font_sizes.hpp>

#include <kbf/cimgui/cimgui_funcs.hpp>

#include <algorithm>

namespace kbf {

    PresetGroupPanel::PresetGroupPanel(
        const std::string& name,
        const std::string& strID,
        const KBFDataManager& dataManager,
        ImFont* wsSymbolFont,
        bool showDefaultAsOption
    ) : iPanel(name, strID), 
        dataManager{ dataManager }, 
        wsSymbolFont{ wsSymbolFont }, 
        showDefaultAsOption{ showDefaultAsOption } {}

    bool PresetGroupPanel::draw() {
        bool open = true;
        processFocus();
        CImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, ImVec2(0.5f, 0.5f));

        constexpr ImVec2 minWindowSize = ImVec2(550, 350);

        if (isFirstDraw()) { CImGui::SetNextWindowSize(minWindowSize); }
        CImGui::SetNextWindowSizeConstraints(minWindowSize, ImVec2(FLT_MAX, FLT_MAX));
        CImGui::Begin(nameWithID.c_str(), &open, ImGuiWindowFlags_NoCollapse);

        float width = CImGui::GetWindowSize().x;

        static char filterBuffer[128] = "";
        std::string filterStr{ filterBuffer };

        drawPresetGroupList(dataManager.getPresetGroups(filterStr, true));

        CImGui::PushItemWidth(-1);
        CImGui::InputTextWithHint("##Search", "Search...", filterBuffer, IM_ARRAYSIZE(filterBuffer));
        CImGui::PopItemWidth();

        CImGui::End();
        CImGui::PopStyleVar();

        return open;
    }

    void PresetGroupPanel::drawPresetGroupList(const std::vector<const PresetGroup*>& presetGroups) {
        // Fixed-height, scrollable region
        CImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.02f, 0.02f, 0.02f, 1.0f));
        CImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 10));

        const float height = -(CImGui::GetFrameHeightWithSpacing() + 10.0f); // Leave space for the search bar
        CImGui::BeginChild("PresetGroupChild", ImVec2(0, height), true, ImGuiWindowFlags_HorizontalScrollbar);

        float contentRegionWidth = CImGui::GetContentRegionAvail().x;
        const float selectableHeight = CImGui::GetTextLineHeight();

        if (showDefaultAsOption) {
            CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.365f, 0.678f, 0.886f, 0.8f));
            if (CImGui::Selectable("Default", false, 0, ImVec2(0.0f, selectableHeight))) {
                INVOKE_REQUIRED_CALLBACK(selectCallback, "");
            }
            CImGui::PopStyleColor();

            if (presetGroups.size() > 0) CImGui::Separator();
        } 
        else if (presetGroups.size() == 0) {
            CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
            constexpr char const* noPresetGroupStr = "No Existing Preset Groups";
            preAlignCellContentHorizontal(noPresetGroupStr);
            CImGui::Text(noPresetGroupStr);
            CImGui::PopStyleColor();
        }

        for (const PresetGroup* group : presetGroups)
        {
            ImVec2 pos = CImGui::GetCursorScreenPos();

            if (CImGui::Selectable(("##" + group->name).c_str(), false, 0, ImVec2(0.0f, selectableHeight))) {
                INVOKE_REQUIRED_CALLBACK(selectCallback, group->uuid);
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

            // Group name
            ImVec2 presetGroupNameSize = CImGui::CalcTextSize(group->name.c_str());
            ImVec2 presetGroupNamePos;
            presetGroupNamePos.x = sexMarkerPos.x + sexMarkerSize.x + sexMarkerSpacingAfter;
            presetGroupNamePos.y = pos.y + (selectableHeight - presetGroupNameSize.y) * 0.5f;
            CImGui::GetWindowDrawList()->AddText(presetGroupNamePos, CImGui::GetColorU32(ImGuiCol_Text), group->name.c_str());

            std::string presetCountStr;
            if (group->size() == 0) presetCountStr = "Empty";
            else if (group->size() == 1) presetCountStr = "1 Preset";
            else presetCountStr = std::to_string(group->size()) + " Presets";

            ImVec2 rightTextSize = CImGui::CalcTextSize(presetCountStr.c_str());
            float hunterIdCursorPosX = CImGui::GetCursorScreenPos().x + contentRegionWidth - rightTextSize.x - CImGui::GetStyle().ItemSpacing.x;
            float hunterIdCursorPosY = CImGui::GetItemRectMin().y + 4.0f;  // Same Y as the selectable item, plus vertical alignment

            CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
            CImGui::GetWindowDrawList()->AddText(ImVec2(hunterIdCursorPosX, hunterIdCursorPosY), CImGui::GetColorU32(ImGuiCol_Text), presetCountStr.c_str());
            CImGui::PopStyleColor();
        }

        CImGui::EndChild();
        CImGui::PopStyleVar();
        CImGui::PopStyleColor();

    }

}