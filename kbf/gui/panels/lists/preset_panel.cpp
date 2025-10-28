#include <kbf/gui/panels/lists/preset_panel.hpp>

#include <kbf/data/ids/font_symbols.hpp>
#include <kbf/data/ids/special_armour_ids.hpp>
#include <kbf/gui/shared/alignment.hpp>
#include <kbf/util/string/to_lower.hpp>
#include <kbf/util/functional/invoke_callback.hpp>
#include <kbf/util/font/default_font_sizes.hpp>

#include <kbf/cimgui/cimgui_funcs.hpp>

#include <algorithm>

namespace kbf {

    PresetPanel::PresetPanel(
        const std::string& name,
        const std::string& strID,
        const KBFDataManager& dataManager,
        ImFont* wsSymbolFont,
        ImFont* wsArmourFont,
        bool showDefaultAsOption
    ) : iPanel(name, strID), 
        dataManager{ dataManager }, 
        wsSymbolFont{ wsSymbolFont }, 
        wsArmourFont{ wsArmourFont },
        showDefaultAsOption{ showDefaultAsOption } {}

    bool PresetPanel::draw() {
        bool open = true;
        processFocus();
        CImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, ImVec2(0.5f, 0.5f));

        constexpr ImVec2 minWindowSize = ImVec2(600, 350);

        if (isFirstDraw()) { CImGui::SetNextWindowSize(minWindowSize); }
        if (needsWidthStretch) { CImGui::SetNextWindowSize(ImVec2{ widthStretch, minWindowSize.y }); needsWidthStretch = false; }
        CImGui::SetNextWindowSizeConstraints(minWindowSize, ImVec2(FLT_MAX, FLT_MAX));
        CImGui::PushFont(wsArmourFont, FONT_SIZE_DEFAULT_WILDS_ARMOUR);
        CImGui::Begin(nameWithID.c_str(), &open, ImGuiWindowFlags_NoCollapse);

        float width = CImGui::GetWindowSize().x;

        static char filterBuffer[128] = "";
        std::string filterStr{ filterBuffer };

        drawPresetList(dataManager.getPresets(filterStr, ArmourPieceFlagBits::APF_NONE, true));

        CImGui::PushFont(dataManager.getRegularFontOverride(), FONT_SIZE_DEFAULT_MAIN);
        CImGui::PushItemWidth(-1);
        CImGui::InputTextWithHint("##Search", "Search...", filterBuffer, IM_ARRAYSIZE(filterBuffer));
        CImGui::PopItemWidth();
        CImGui::PopFont();

        CImGui::End();
        CImGui::PopFont();
        CImGui::PopStyleVar();

        if (isFirstDraw()) needsWidthStretch = true;

        return open;
    }

    void PresetPanel::drawPresetList(const std::vector<const Preset*>& presets) {
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

            if (presets.size() > 0) CImGui::Separator();
        }
        else if (presets.size() == 0) {
            CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
            constexpr char const* noPresetStr = "No Existing Presets";
            preAlignCellContentHorizontal(noPresetStr);
            CImGui::Text(noPresetStr);
            CImGui::PopStyleColor();
        }

        float maxDrawSize = 0.0f;
        for (const Preset* preset : presets)
        {
            ImVec2 pos = CImGui::GetCursorScreenPos();
            if (CImGui::Selectable(("##Selectable_Preset_" + preset->name).c_str(), false, 0, ImVec2(0.0f, selectableHeight))) {
                INVOKE_REQUIRED_CALLBACK(selectCallback, preset->uuid);
            }

            // Sex Mark
            std::string sexMarkSymbol = preset->female ? WS_FONT_FEMALE : WS_FONT_MALE;
            ImVec4 sexMarkerCol = preset->female ? ImVec4(0.76f, 0.50f, 0.24f, 1.0f) : ImVec4(0.50f, 0.70f, 0.33f, 1.0f);

            CImGui::PushFont(wsSymbolFont, FONT_SIZE_DEFAULT_WILDS_SYMBOLS);

            constexpr float sexMarkerSpacingAfter = 5.0f;
            constexpr float sexMarkerVerticalAlignOffset = 5.0f;
            ImVec2 sexMarkerSize = CImGui::CalcTextSize(sexMarkSymbol.c_str());
            ImVec2 sexMarkerPos;
            sexMarkerPos.x = pos.x + CImGui::GetStyle().ItemSpacing.x;
            sexMarkerPos.y = pos.y + (selectableHeight - sexMarkerSize.y) * 0.5f + sexMarkerVerticalAlignOffset;
            CImGui::GetWindowDrawList()->AddText(sexMarkerPos, CImGui::GetColorU32(sexMarkerCol), sexMarkSymbol.c_str());

            CImGui::PopFont();

            CImGui::PushFont(dataManager.getRegularFontOverride(), FONT_SIZE_DEFAULT_MAIN);

            // Group name... floating because imgui has no vertical alignment STILL :(
            constexpr float playerNameSpacingAfter = 5.0f;
            ImVec2 playerNameSize = CImGui::CalcTextSize(preset->name.c_str());
            ImVec2 playerNamePos;
            playerNamePos.x = sexMarkerPos.x + sexMarkerSize.x + sexMarkerSpacingAfter;
            playerNamePos.y = pos.y + (selectableHeight - playerNameSize.y) * 0.5f;
            CImGui::GetWindowDrawList()->AddText(playerNamePos, CImGui::GetColorU32(ImGuiCol_Text), preset->name.c_str());

            std::string bundleStr = "(" + preset->bundle + ")";
            ImVec2 bundleStrSize = CImGui::CalcTextSize(bundleStr.c_str());
            ImVec2 bundleStrPos;
            bundleStrPos.x = playerNamePos.x + playerNameSize.x + playerNameSpacingAfter;
            bundleStrPos.y = pos.y + (selectableHeight - bundleStrSize.y) * 0.5f;
            CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
            CImGui::GetWindowDrawList()->AddText(bundleStrPos, CImGui::GetColorU32(ImGuiCol_Text), bundleStr.c_str());
            CImGui::PopStyleColor();

			CImGui::PopFont();

            // Armour Marks
            constexpr ImVec4 armourMissingCol{ 1.0f, 1.0f, 1.0f, 0.1f };
            constexpr ImVec4 armourPresentCol{ 1.0f, 1.0f, 1.0f, 1.0f };
            constexpr float armourVerticalAlignOffset = 2.5f;
            CImGui::PushFont(wsSymbolFont, FONT_SIZE_DEFAULT_WILDS_SYMBOLS);

            // Legs Mark
            ImVec2 legMarkSize = CImGui::CalcTextSize(WS_FONT_LEGS);
            ImVec2 legMarkPos;
            legMarkPos.x = CImGui::GetCursorScreenPos().x + contentRegionWidth - legMarkSize.x - CImGui::GetStyle().ItemSpacing.x;
            legMarkPos.y = pos.y + (selectableHeight - legMarkSize.y) * 0.5f + armourVerticalAlignOffset;
            CImGui::PushStyleColor(ImGuiCol_Text, preset->hasModifiers(ArmourPiece::AP_LEGS) ? armourPresentCol : armourMissingCol);
            CImGui::GetWindowDrawList()->AddText(legMarkPos, CImGui::GetColorU32(ImGuiCol_Text), WS_FONT_LEGS);
            CImGui::PopStyleColor();

            // Coil Mark
            ImVec2 coilMarkSize = CImGui::CalcTextSize(WS_FONT_COIL);
            ImVec2 coilMarkPos;
            coilMarkPos.x = legMarkPos.x - coilMarkSize.x - CImGui::GetStyle().ItemSpacing.x;
            coilMarkPos.y = pos.y + (selectableHeight - coilMarkSize.y) * 0.5f + armourVerticalAlignOffset;
            CImGui::PushStyleColor(ImGuiCol_Text, preset->hasModifiers(ArmourPiece::AP_COIL) ? armourPresentCol : armourMissingCol);
            CImGui::GetWindowDrawList()->AddText(coilMarkPos, CImGui::GetColorU32(ImGuiCol_Text), WS_FONT_COIL);
            CImGui::PopStyleColor();

            // Arms Mark
            ImVec2 armsMarkSize = CImGui::CalcTextSize(WS_FONT_ARMS);
            ImVec2 armsMarkPos;
            armsMarkPos.x = coilMarkPos.x - armsMarkSize.x - CImGui::GetStyle().ItemSpacing.x;
            armsMarkPos.y = pos.y + (selectableHeight - armsMarkSize.y) * 0.5f + armourVerticalAlignOffset;
            CImGui::PushStyleColor(ImGuiCol_Text, preset->hasModifiers(ArmourPiece::AP_ARMS) ? armourPresentCol : armourMissingCol);
            CImGui::GetWindowDrawList()->AddText(armsMarkPos, CImGui::GetColorU32(ImGuiCol_Text), WS_FONT_ARMS);
            CImGui::PopStyleColor();

            // Body Mark
            ImVec2 bodyMarkSize = CImGui::CalcTextSize(WS_FONT_BODY);
            ImVec2 bodyMarkPos;
            bodyMarkPos.x = armsMarkPos.x - bodyMarkSize.x - CImGui::GetStyle().ItemSpacing.x;
            bodyMarkPos.y = pos.y + (selectableHeight - bodyMarkSize.y) * 0.5f + armourVerticalAlignOffset;
            CImGui::PushStyleColor(ImGuiCol_Text, preset->hasModifiers(ArmourPiece::AP_BODY) ? armourPresentCol : armourMissingCol);
            CImGui::GetWindowDrawList()->AddText(bodyMarkPos, CImGui::GetColorU32(ImGuiCol_Text), WS_FONT_BODY);
            CImGui::PopStyleColor();

            // Helm Mark
            ImVec2 helmMarkSize = CImGui::CalcTextSize(WS_FONT_HELM);
            ImVec2 helmMarkPos;
            helmMarkPos.x = bodyMarkPos.x - helmMarkSize.x - CImGui::GetStyle().ItemSpacing.x;
            helmMarkPos.y = pos.y + (selectableHeight - helmMarkSize.y) * 0.5f + armourVerticalAlignOffset;
            CImGui::PushStyleColor(ImGuiCol_Text, preset->hasModifiers(ArmourPiece::AP_HELM) ? armourPresentCol : armourMissingCol);
            CImGui::GetWindowDrawList()->AddText(helmMarkPos, CImGui::GetColorU32(ImGuiCol_Text), WS_FONT_HELM);
            CImGui::PopStyleColor();

            // Set Mark
            ImVec2 setMarkSize = CImGui::CalcTextSize(WS_FONT_SET);
            ImVec2 setMarkPos;
            setMarkPos.x = helmMarkPos.x - setMarkSize.x - CImGui::GetStyle().ItemSpacing.x;
            setMarkPos.y = pos.y + (selectableHeight - setMarkSize.y) * 0.5f + armourVerticalAlignOffset;
            CImGui::PushStyleColor(ImGuiCol_Text, preset->hasModifiers(ArmourPiece::AP_SET) ? armourPresentCol : armourMissingCol);
            CImGui::GetWindowDrawList()->AddText(setMarkPos, CImGui::GetColorU32(ImGuiCol_Text), WS_FONT_SET);
            CImGui::PopStyleColor();

            // Armour Sex Mark
            std::string armourSexMarkSymbol = preset->armour.female ? WS_FONT_FEMALE : WS_FONT_MALE;
            ImVec4 armourSexMarkerCol = preset->armour.female ? ImVec4(0.76f, 0.50f, 0.24f, 1.0f) : ImVec4(0.50f, 0.70f, 0.33f, 1.0f);

            //constexpr float armourSexMarkerSpacingAfter = 5.0f;
            constexpr float armourSexMarkerVerticalAlignOffset = 5.0f;
            ImVec2 armourSexMarkerSize = CImGui::CalcTextSize(armourSexMarkSymbol.c_str());
            ImVec2 armourSexMarkerPos;
            armourSexMarkerPos.x = setMarkPos.x - armourSexMarkerSize.x - CImGui::GetStyle().ItemSpacing.x;
            armourSexMarkerPos.y = pos.y + (selectableHeight - armourSexMarkerSize.y) * 0.5f + armourSexMarkerVerticalAlignOffset;
            CImGui::GetWindowDrawList()->AddText(armourSexMarkerPos, CImGui::GetColorU32(armourSexMarkerCol), armourSexMarkSymbol.c_str());

            CImGui::PopFont();

            CImGui::PushFont(wsArmourFont, FONT_SIZE_DEFAULT_WILDS_ARMOUR);

            // Armour Name
            ImVec2 armourNameSize = CImGui::CalcTextSize(preset->armour.name.c_str());
            ImVec2 armourNamePos;
            armourNamePos.x = armourSexMarkerPos.x - armourNameSize.x - CImGui::GetStyle().ItemSpacing.x;
            armourNamePos.y = pos.y + (selectableHeight - armourNameSize.y) * 0.5f;

            ImVec4 armourNameCol = preset->armour.name == ANY_ARMOUR_ID ? ImVec4(0.365f, 0.678f, 0.886f, 0.8f) : ImVec4(1.0f, 1.0f, 1.0f, 0.5f);
            CImGui::PushStyleColor(ImGuiCol_Text, armourNameCol);
            CImGui::GetWindowDrawList()->AddText(armourNamePos, CImGui::GetColorU32(ImGuiCol_Text), preset->armour.name.c_str());
            CImGui::PopStyleColor();

            CImGui::PopFont();

            maxDrawSize = std::max<float>(
                maxDrawSize,
                sexMarkerSize.x + playerNameSize.x + bundleStrSize.x + legMarkSize.x + coilMarkSize.x + armsMarkSize.x + bodyMarkSize.x + helmMarkSize.x + setMarkSize.x + armourSexMarkerSize.x + armourNameSize.x
            );
        }

        CImGui::EndChild();
        CImGui::PopStyleVar();
        CImGui::PopStyleColor();

        widthStretch = maxDrawSize + 150.0f; // Const for spacing and stuff
    }

}