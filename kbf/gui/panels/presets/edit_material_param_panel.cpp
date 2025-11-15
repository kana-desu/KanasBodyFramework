#include <kbf/gui/panels/presets/edit_material_param_panel.hpp>

#include <kbf/gui/shared/toggle_colours.hpp>
#include <kbf/gui/components/toggle/imgui_toggle.h>
#include <kbf/util/functional/invoke_callback.hpp>
#include <kbf/gui/shared/styling_consts.hpp>
#include <kbf/util/string/to_lower.hpp>

#define EDIT_PRESET_GROUP_PANEL_LOG_TAG "[EditPresetGroupPanel]"

namespace kbf {

    EditMaterialParamPanel::EditMaterialParamPanel(
        const std::string& name,
        const std::string& strID,
        const OverrideMaterial& material,
        const KBFDataManager& dataManager,
        ImFont* wsSymbolFont
    ) : iPanel(name, strID),
        materialBefore{ material },
		materialAfter{ material },
        dataManager{ dataManager },
        wsSymbolFont{ wsSymbolFont }
    {}

    bool EditMaterialParamPanel::draw() {
        bool open = true;
        processFocus();

        CImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, ImVec2(0.5f, 0.5f));

        constexpr ImVec2 minWindowSize = ImVec2(600, 600);

        if (isFirstDraw()) { CImGui::SetNextWindowSize(minWindowSize); }
        if (needsWidthStretch) { CImGui::SetNextWindowSize(ImVec2{ widthStretch, minWindowSize.y }); needsWidthStretch = false; }
        CImGui::SetNextWindowSizeConstraints(minWindowSize, ImVec2(FLT_MAX, FLT_MAX));
        CImGui::Begin(nameWithID.c_str(), &open, ImGuiWindowFlags_NoCollapse);

        float width = CImGui::GetWindowSize().x;
        constexpr float rowHeight = 30.0f;
		constexpr float tableVpad = 0.0f;

        static char filterBuffer[128] = "";
        std::string filterStr{ filterBuffer };
        std::string filterLower = toLower(filterStr);

        float reserve = CImGui::GetFrameHeight() * 2 + CImGui::GetStyle().ItemSpacing.y * 4;
        CImGui::BeginChild("MaterialParamListChild", ImVec2(0, -(reserve)), ImGuiChildFlags_Borders);

        constexpr ImGuiTableFlags boneModTableFlags =
            ImGuiTableFlags_PadOuterX
            | ImGuiTableFlags_BordersInnerH;
        CImGui::BeginTable("##MatParamList", 3, boneModTableFlags);

        CImGui::TableSetupColumn("",               ImGuiTableColumnFlags_WidthFixed, 0.0f);
        CImGui::TableSetupColumn("Parameter",      ImGuiTableColumnFlags_WidthFixed, 0.0f);
        CImGui::TableSetupColumn("Override Value", ImGuiTableColumnFlags_WidthStretch, 0.0f);
        CImGui::TableSetupScrollFreeze(0, 1);
        CImGui::TableHeadersRow();

        CImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(LIST_PADDING.x, tableVpad));

        for (const auto& [idx, param] : materialBefore.material.params) {
            if (!filterLower.empty()) {
                std::string nameLower = toLower(param.name);
                if (nameLower.find(filterLower) == std::string::npos) continue;
            }

			drawMaterialParamEditorRow(param);
        }

		CImGui::PopStyleVar();
        CImGui::EndTable();

        CImGui::EndChild();

        CImGui::PushItemWidth(-1);
        CImGui::InputTextWithHint("##Search", "Search...", filterBuffer, IM_ARRAYSIZE(filterBuffer));
        CImGui::PopItemWidth();

        CImGui::Spacing();
        CImGui::Spacing();

        static constexpr const char* kCancelLabel = "Cancel";
        static constexpr const char* kUpdateLabel = "Update";

        float spacing = CImGui::GetStyle().ItemSpacing.x;
        float buttonWidth1 = CImGui::CalcTextSize(kCancelLabel).x + CImGui::GetStyle().FramePadding.x * 2;
        float buttonWidth2 = CImGui::CalcTextSize(kUpdateLabel).x + CImGui::GetStyle().FramePadding.x * 2;
        float totalWidth = buttonWidth1 + buttonWidth2 + spacing;

        float availableWidth = CImGui::GetContentRegionAvail().x;
        CImGui::SetCursorPosX(availableWidth - totalWidth + 8.0f); // Align to the right

        CImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
        CImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.20f, 0.20f, 1.0f));
        CImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.10f, 0.10f, 0.10f, 1.0f));

        if (CImGui::Button(kCancelLabel)) {
            INVOKE_REQUIRED_CALLBACK(cancelCallback);
        }

        CImGui::PopStyleColor(3);

        CImGui::SameLine();

        if (CImGui::Button(kUpdateLabel)) {
            INVOKE_REQUIRED_CALLBACK(updateCallback, materialAfter);
        }

        CImGui::End();
        CImGui::PopStyleVar();

        if (isFirstDraw()) needsWidthStretch = true;

        return open;
    }

    void EditMaterialParamPanel::drawMaterialParamEditorRow(const MeshMaterialParam& param) {
        bool enabled = materialAfter.paramOverrides.contains(param.name);

        // ----- DIM ROW BACKGROUND IF DISABLED -----
        if (!enabled) {
            const ImVec4 bg = CImGui::GetStyle().Colors[ImGuiCol_TableRowBg];
            const ImVec4 bga = CImGui::GetStyle().Colors[ImGuiCol_TableRowBgAlt];
            CImGui::PushStyleColor(ImGuiCol_TableRowBg, ImVec4(bg.x, bg.y, bg.z, bg.w * 0.25f));
            CImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, ImVec4(bga.x, bga.y, bga.z, bga.w * 0.25f));
        }

        CImGui::TableNextRow();

        // ------ VERTICAL CENTERING OF CONTENT ------
        float lineH = CImGui::GetTextLineHeight();
        float rowH = 30.0f;   // your constant row height
        float yOff = (rowH - lineH) * 0.5f + 5.0f;   // center inside fixed-height row

        // -------------------------------------------
        CImGui::TableNextColumn();
        CImGui::SetCursorPosY(CImGui::GetCursorPosY() + yOff);

        // toggle
        pushToggleColors(enabled);
        bool localEnabled = enabled;
        if (CImGui::Toggle(("##" + param.name).c_str(), &localEnabled, ImGuiToggleFlags_Animated)) {
            if (localEnabled)
                materialAfter.setParamOverride(param.name, 0.0f);
            else
                materialAfter.removeParamOverride(param.name);
        }
        popToggleColors();

        // -------------------------------------------
        CImGui::TableNextColumn();
        CImGui::SetCursorPosY(CImGui::GetCursorPosY() + yOff);

        // disabled-style text
        if (!enabled)
            CImGui::PushStyleColor(ImGuiCol_Text, CImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);

        CImGui::TextUnformatted(param.name.c_str());

        if (!enabled)
            CImGui::PopStyleColor();

        // -------------------------------------------
        CImGui::TableNextColumn();
        CImGui::SetCursorPosY(CImGui::GetCursorPosY() + yOff);

        CImGui::BeginDisabled(!enabled);
        CImGui::SetNextItemWidth(-1);

        switch (param.type) {
        case MeshMaterialParamType::MAT_TYPE_FLOAT: {
            bool hasParam = materialAfter.paramOverrides.contains(param.name);
            float value = 0.0f;
            if (hasParam) value = materialAfter.paramOverrides.at(param.name).value.floatValue;

            if (CImGui::DragFloat(("##slider_" + param.name).c_str(), &value)) {
                if (hasParam) materialAfter.paramOverrides.at(param.name).value.floatValue = value;
            }
        } break;
        case MeshMaterialParamType::MAT_TYPE_FLOAT4: {
            bool hasParam = materialAfter.paramOverrides.contains(param.name);
            glm::vec4 value{};
            if (hasParam) value = materialAfter.paramOverrides.at(param.name).value.vec4Value;

            DEBUG_STACK.push("CHECKING FOR COL!!");
            std::string nameLower = toLower(param.name);
            DEBUG_STACK.push(nameLower);
            bool isColor = nameLower.find("color") != nameLower.npos;

            if (isColor) {
                constexpr ImGuiColorEditFlags colFlags =
                    ImGuiColorEditFlags_DisplayRGB |
                    ImGuiColorEditFlags_PickerHueWheel |  // Use color wheel instead of bars
                    ImGuiColorEditFlags_AlphaBar;

                if (CImGui::ColorEdit4(("##Color_" + param.name).c_str(), &value[0], colFlags)) {
                    if (hasParam) materialAfter.paramOverrides.at(param.name).value.vec4Value = value;
                }
            }
            else {
                if (CImGui::DragFloat4(("##Float4_" + param.name).c_str(), &value[0])) {
                    if (hasParam) materialAfter.paramOverrides.at(param.name).value.vec4Value = value;
                }
            }
        } break;
        }

        CImGui::EndDisabled();

        // Pop row background colors
        if (!enabled)
            CImGui::PopStyleColor(2);
    }

}