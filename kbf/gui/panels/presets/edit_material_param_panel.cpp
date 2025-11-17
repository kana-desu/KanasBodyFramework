#include <kbf/gui/panels/presets/edit_material_param_panel.hpp>

#include <kbf/gui/shared/toggle_colours.hpp>
#include <kbf/gui/components/toggle/imgui_toggle.h>
#include <kbf/util/functional/invoke_callback.hpp>
#include <kbf/gui/shared/styling_consts.hpp>
#include <kbf/util/string/to_lower.hpp>
#include <kbf/gui/shared/alignment.hpp>

#define EDIT_PRESET_GROUP_PANEL_LOG_TAG "[EditPresetGroupPanel]"

namespace kbf {

    EditMaterialParamPanel::EditMaterialParamPanel(
        const std::string& name,
        const std::string& strID,
        const OverrideMaterial& material,
        ArmourPiece piece,
        ArmourSetWithCharacterSex armour,
        KBFDataManager& dataManager,
        ImFont* wsSymbolFont
    ) : iPanel(name, strID),
        materialBefore{ material },
		materialAfter{ material },
        piece{ piece },
        armour{ armour },
        dataManager{ dataManager },
        wsSymbolFont{ wsSymbolFont }
    {}

    bool EditMaterialParamPanel::draw() {
        bool open = true;
        processFocus();

        CImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, ImVec2(0.5f, 0.5f));

        constexpr ImVec2 minWindowSize = ImVec2(600, 700);

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

        CImGui::Spacing();
        CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
        const char* hintText = "Note: Red params are not in this material's cache, so may not be present in-game.";
        preAlignCellContentHorizontal(hintText);
        CImGui::Text(hintText);
        CImGui::PopStyleColor();
        CImGui::Spacing();

        float reserve = CImGui::GetFrameHeight() * 2 + CImGui::GetStyle().ItemSpacing.y * 4;
        CImGui::BeginChild("MaterialParamListChild", ImVec2(0, -(reserve)), ImGuiChildFlags_Borders);

        constexpr ImGuiTableFlags matParamTableFlags =
            ImGuiTableFlags_RowBg
            | ImGuiTableFlags_PadOuterX
            | ImGuiTableFlags_BordersInnerH
            | ImGuiTableFlags_ScrollY;
        CImGui::BeginTable("##MatParamList", 3, matParamTableFlags);

        CImGui::TableSetupColumn("",               ImGuiTableColumnFlags_WidthFixed, 0.0f);
        CImGui::TableSetupColumn("Parameter",      ImGuiTableColumnFlags_WidthFixed, 0.0f);
        CImGui::TableSetupColumn("Override Value", ImGuiTableColumnFlags_WidthStretch, 0.0f);
        CImGui::TableSetupScrollFreeze(0, 1);
        CImGui::TableHeadersRow();

        CImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(LIST_PADDING.x, tableVpad));

        // Get corresponding Material to one passed in
        const std::vector<MeshMaterial> mats = dataManager.materialCacheManager().getCache(armour)->getPieceCache(piece).getMaterials();

        const MeshMaterial* mat = nullptr;
        for (const MeshMaterial& m : mats) {
            if (m.name == materialAfter.material.name) {
                mat = &m;
                break;
            }
        }

        std::set<std::string> presentNames{};
        size_t drawCnt = 0;
        bool changed = false;

        // Draw all present params
        if (mat != nullptr) {
            for (const auto& [idx, param] : mat->params) {
                if (!filterLower.empty()) {
                    std::string nameLower = toLower(param.name);
                    if (nameLower.find(filterLower) == std::string::npos) continue;
                }

			    changed |= drawMaterialParamEditorRow(param);
                presentNames.insert(param.name);
                drawCnt++;
            }
        }

        // Draw any remaining ones in the preset that aren't in the cache
        for (const auto& [name, param] : materialAfter.paramOverrides) {
            if (presentNames.contains(name)) continue;
            if (!filterLower.empty()) {
                std::string nameLower = toLower(name);
                if (nameLower.find(filterLower) == std::string::npos) continue;
            }

            changed |= drawMissingMaterialParamRow(name);
            drawCnt++;
        }
        
		CImGui::PopStyleVar();
        CImGui::EndTable();

        // Hint when no params drawn
        if (drawCnt == 0) {
            CImGui::Spacing();
            CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
            const char* noParamStr = filterLower.empty()
                ? "No Parameter editors found."
                : "Parameter cache is empty, try equipping the armour in-game.";
            preAlignCellContentHorizontal(noParamStr);
            CImGui::Text(noParamStr);
            CImGui::PopStyleColor();
        }

        CImGui::EndChild();

        CImGui::PushItemWidth(-1);
        CImGui::InputTextWithHint("##Search", "Search...", filterBuffer, IM_ARRAYSIZE(filterBuffer));
        CImGui::PopItemWidth();

        // Buttons
        CImGui::Spacing();
        CImGui::Spacing();

        static constexpr const char* kRevertLabel = "Revert";
        bool cantRevert = materialBefore.isExactlyEqual(materialAfter);

        CImGui::BeginDisabled(cantRevert);
        if (CImGui::Button(kRevertLabel)) {
            materialAfter = materialBefore;
            changed = true;
        }
        CImGui::EndDisabled();
        CImGui::SameLine();

        static constexpr const char* kCancelLabel = "Cancel";
        static constexpr const char* kUpdateLabel = "Done";

        float spacing = CImGui::GetStyle().ItemSpacing.x;
        float buttonWidth1 = CImGui::CalcTextSize(kCancelLabel).x + CImGui::GetStyle().FramePadding.x * 2;
        float buttonWidth2 = CImGui::CalcTextSize(kUpdateLabel).x + CImGui::GetStyle().FramePadding.x * 2;
        float buttonWidth3 = CImGui::CalcTextSize(kRevertLabel).x + CImGui::GetStyle().FramePadding.x * 2;
        float totalWidth = buttonWidth1 + buttonWidth2 - buttonWidth3;

        float availableWidth = CImGui::GetContentRegionAvail().x;
        CImGui::SetCursorPosX(availableWidth - totalWidth + 8.0f); // Align to the right

        CImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
        CImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.20f, 0.20f, 1.0f));
        CImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.10f, 0.10f, 0.10f, 1.0f));

        if (CImGui::Button(kCancelLabel)) {
            changed = true;
            INVOKE_REQUIRED_CALLBACK(closeCallback);
        }

        CImGui::PopStyleColor(3);

        CImGui::SameLine();

        if (CImGui::Button(kUpdateLabel)) {
            changed = true;
            INVOKE_REQUIRED_CALLBACK(closeCallback);
        }

        CImGui::End();
        CImGui::PopStyleVar();

        if (isFirstDraw()) needsWidthStretch = true;

        if (changed) {
            INVOKE_REQUIRED_CALLBACK(updateCallback, materialAfter);
        }

        return open;
    }

    bool EditMaterialParamPanel::drawMaterialParamEditorRow(const MeshMaterialParam& param) {
        bool changed = false;
        bool enabled = materialAfter.paramOverrides.contains(param.name);

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
            changed = true;
            if (localEnabled) {
                switch (param.type) {
                case MeshMaterialParamType::MAT_TYPE_FLOAT: materialAfter.setParamOverride(param.name, 0.0f); break;
                case MeshMaterialParamType::MAT_TYPE_FLOAT4: materialAfter.setParamOverride(param.name, glm::vec4{}); break;
                }
            }
            else {
                materialAfter.removeParamOverride(param.name);
            }
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

        // TODO: This code is fucking horrible.
        switch (param.type) {

        case MeshMaterialParamType::MAT_TYPE_FLOAT: {
            float value = 0.0f;
            bool hasValue = true;

            if (materialAfter.paramOverrides.contains(param.name)) {
                auto& paramVal = materialAfter.paramOverrides.at(param.name);
                if (std::holds_alternative<float>(paramVal.value)) 
                    value = paramVal.asFloat();
                else                                               
                    hasValue = false;
            }

            if (CImGui::DragFloat(("##slider_" + param.name).c_str(), &value, 0.001f)) {
                changed = true;
                if (hasValue) materialAfter.paramOverrides.at(param.name).value = value;
            }
        } break;

        case MeshMaterialParamType::MAT_TYPE_FLOAT4: {
            glm::vec4 value{};  // default dummy
            bool hasValue = true;

            if (materialAfter.paramOverrides.contains(param.name)) {
                auto& paramVal = materialAfter.paramOverrides.at(param.name);
                if (std::holds_alternative<glm::vec4>(paramVal.value))
                    value = paramVal.asVec4();
                else
                    hasValue = false; // param exists but wrong type, treat as dummy
            }

            std::string nameLower = toLower(param.name);
            bool isColor = nameLower.find("color") != nameLower.npos;

            if (isColor) {
                constexpr ImGuiColorEditFlags colFlags =
                    ImGuiColorEditFlags_NoOptions |
                    ImGuiColorEditFlags_Float |
                    ImGuiColorEditFlags_DisplayRGB |
                    ImGuiColorEditFlags_PickerHueWheel |
                    ImGuiColorEditFlags_AlphaBar;

                if (CImGui::ColorEdit4(("##Color_" + param.name).c_str(), &value[0], colFlags)) {
                    changed = true;
                    if (hasValue)
                        materialAfter.paramOverrides.at(param.name).value = value;
                }
            }
            else {
                if (CImGui::DragFloat4(("##Float4_" + param.name).c_str(), &value[0], 0.001f)) {
                    changed = true;
                    if (hasValue)
                        materialAfter.paramOverrides.at(param.name).value = value;
                }
            }
        } break;

        }

        CImGui::EndDisabled();

        return changed;
    }

    bool EditMaterialParamPanel::drawMissingMaterialParamRow(const std::string& paramName) {
        bool changed = false;
        bool isEnabled = materialAfter.paramOverrides.contains(paramName);

        // --- Row background (always red) ---
        const ImVec4 rowBg(0.8f, 0.2f, 0.2f, 0.3f);
        CImGui::TableNextRow();

        CImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, CImGui::GetColorU32(rowBg));

        // --- Vertical centering ---
        float lineH = CImGui::GetTextLineHeight();
        float rowH = 30.0f;
        float yOff = (rowH - lineH) * 0.5f + 5.0f;

        // ---------------- COLUMN 1: toggle ----------------
        CImGui::TableNextColumn();
        CImGui::SetCursorPosY(CImGui::GetCursorPosY() + yOff);

        pushToggleColors(isEnabled);
        bool localEnabled = isEnabled;

        if (CImGui::Toggle(("##" + paramName).c_str(), &localEnabled, ImGuiToggleFlags_Animated)) {
            if (!localEnabled && isEnabled) {
                changed = true;
                materialAfter.removeParamOverride(paramName);
                isEnabled = false;
            }
            // No toggle-on action needed
        }
        popToggleColors();

        // ---------------- COLUMN 2: param name ----------------
        CImGui::TableNextColumn();
        CImGui::SetCursorPosY(CImGui::GetCursorPosY() + yOff);
        CImGui::PushStyleColor(ImGuiCol_Text, CImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
        CImGui::TextUnformatted(paramName.c_str());
        CImGui::PopStyleColor();

        // ---------------- COLUMN 3: slider / color (disabled) ----------------
        CImGui::TableNextColumn();
        CImGui::SetCursorPosY(CImGui::GetCursorPosY() + yOff);
        CImGui::BeginDisabled(true); // slider always disabled
        CImGui::SetNextItemWidth(-1);

        if (isEnabled) {
            // Param exists: draw actual value in read-only
            MaterialParamValue& paramVal = materialAfter.paramOverrides.at(paramName);
            if (std::holds_alternative<float>(paramVal.value)) {
                float v = paramVal.asFloat();
                CImGui::DragFloat(("##slider_" + paramName).c_str(), &v, 0.001f);
            }
            else if (std::holds_alternative<glm::vec4>(paramVal.value)) {
                glm::vec4 v = paramVal.asVec4();
                CImGui::ColorEdit4(("##Color_" + paramName).c_str(), &v[0],
                    ImGuiColorEditFlags_NoOptions |
                    ImGuiColorEditFlags_Float |
                    ImGuiColorEditFlags_DisplayRGB);
            }
        }
        else {
            // Param missing: draw dummy slider
            float dummyFloat = 0.0f;
            glm::vec4 dummyVec4{};
            if (paramName.find("color") != std::string::npos) {
                CImGui::ColorEdit4(("##dummy_" + paramName).c_str(), &dummyVec4[0],
                    ImGuiColorEditFlags_NoOptions |
                    ImGuiColorEditFlags_Float |
                    ImGuiColorEditFlags_DisplayRGB);
            }
            else {
                CImGui::DragFloat(("##dummy_" + paramName).c_str(), &dummyFloat, 0.001f);
            }
        }

        CImGui::EndDisabled();

        return changed;
    }

}