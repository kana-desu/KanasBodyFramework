#pragma once

#include <kbf/data/preset/preset_group.hpp>

#include <kbf/gui/shared/sex_marker.hpp>
#include <kbf/gui/shared/alignment.hpp>

#include <kbf/util/functional/invoke_callback.hpp>
#include <kbf/util/font/default_font_sizes.hpp>

#include <kbf/cimgui/cimgui_funcs.hpp>

#include <string>

namespace kbf {

    inline void drawPresetSelectTableEntry(
        ImFont* symbolFont,
        const std::string& strId,
        const std::string& entryName,
        const Preset* preset,
        std::function<void()> onEdit,
        const float selectableHeight = 60.0f
    ) {
        CImGui::TableNextRow();
        CImGui::TableNextColumn();

        ImVec2 pos = CImGui::GetCursorScreenPos();
        if (CImGui::Selectable(("##Selectable_" + entryName).c_str(), false, 0, ImVec2(0.0f, selectableHeight))) {
            INVOKE_REQUIRED_CALLBACK(onEdit);
        }

        std::string presetGroupStr = preset ? preset->name : "Default";

        // Sex Mark ONLY if using defaults i.e. "Male" / "Female" label
        ImVec2 sexMarkerPos = pos;
        ImVec2 sexMarkerSize = { 0.0f, 0.0f };
        float sexMarkerSpacingAfter = 0.0f;

        if (entryName == "Male" || entryName == "Female") {
            // Sex Mark
            bool female = entryName == "Female";
            std::string sexMarkSymbol = female ? WS_FONT_FEMALE : WS_FONT_MALE;
            ImVec4 sexMarkerCol = female ? ImVec4(0.76f, 0.50f, 0.24f, 1.0f) : ImVec4(0.50f, 0.70f, 0.33f, 1.0f);

            CImGui::PushFont(symbolFont, FONT_SIZE_DEFAULT_WILDS_SYMBOLS);
            sexMarkerSpacingAfter = 20.0f;
            sexMarkerSize = CImGui::CalcTextSize(sexMarkSymbol.c_str());
            sexMarkerPos.x = pos.x + CImGui::GetStyle().ItemSpacing.x;
            sexMarkerPos.y = pos.y + (selectableHeight - sexMarkerSize.y) * 0.5f;
            CImGui::GetWindowDrawList()->AddText(sexMarkerPos, CImGui::GetColorU32(sexMarkerCol), sexMarkSymbol.c_str());
            CImGui::PopFont();
        }

        ImVec2 playerNameSize = CImGui::CalcTextSize(entryName.c_str());
        ImVec2 playerNamePos;
        playerNamePos.x = sexMarkerPos.x + sexMarkerSpacingAfter;
        playerNamePos.y = pos.y + (selectableHeight - playerNameSize.y) * 0.5f;
        CImGui::GetWindowDrawList()->AddText(playerNamePos, CImGui::GetColorU32(ImGuiCol_Text), entryName.c_str());

        // Preset Group Sex Mark
        bool female = false;
        float presetGroupSexMarkSpacing = CImGui::GetStyle().ItemSpacing.x;
        float presetGroupSexMarkSpacingBefore = 0.0f;
        float endPos = CImGui::GetCursorScreenPos().x + CImGui::GetContentRegionAvail().x;
        if (preset) {
            bool female = preset->female;
            presetGroupSexMarkSpacing = 15.0f;
            presetGroupSexMarkSpacingBefore = 10.0f;

            std::string presetGroupSexMarkSymbol = female ? WS_FONT_FEMALE : WS_FONT_MALE;
            ImVec4 presetGroupSexMarkCol = female ? ImVec4(0.76f, 0.50f, 0.24f, 1.0f) : ImVec4(0.50f, 0.70f, 0.33f, 1.0f);

            ImVec2 presetGroupSexMarkSize = CImGui::CalcTextSize(presetGroupSexMarkSymbol.c_str());
            ImVec2 presetGroupSexMarkPos;
            presetGroupSexMarkPos.x = endPos - presetGroupSexMarkSpacingBefore - CImGui::GetStyle().ItemSpacing.x;
            presetGroupSexMarkPos.y = pos.y + (selectableHeight - presetGroupSexMarkSize.y) * 0.5f;

            CImGui::PushFont(symbolFont, FONT_SIZE_DEFAULT_WILDS_SYMBOLS);
            CImGui::GetWindowDrawList()->AddText(presetGroupSexMarkPos, CImGui::GetColorU32(presetGroupSexMarkCol), presetGroupSexMarkSymbol.c_str());
            CImGui::PopFont();
        }

        // Preset Group Name
        ImVec2 presetGroupNameSize = CImGui::CalcTextSize(presetGroupStr.c_str());
        ImVec2 presetGroupNamePos;
        presetGroupNamePos.x = endPos - (presetGroupNameSize.x + presetGroupSexMarkSpacing + presetGroupSexMarkSpacingBefore);
        presetGroupNamePos.y = pos.y + (selectableHeight - presetGroupNameSize.y) * 0.5f;

        ImVec4 presetGroupNameCol = preset == nullptr ? ImVec4(0.365f, 0.678f, 0.886f, 0.8f) : ImVec4(1.0f, 1.0f, 1.0f, 0.5f);
        CImGui::PushStyleColor(ImGuiCol_Text, presetGroupNameCol);
        CImGui::GetWindowDrawList()->AddText(presetGroupNamePos, CImGui::GetColorU32(ImGuiCol_Text), presetGroupStr.c_str());
        CImGui::PopStyleColor();
    }

    inline void drawPresetGroupSelectTableEntry(
        ImFont* symbolFont, 
        const std::string& strId, 
        const std::string& entryName,
        const PresetGroup* presetGroup,
        std::function<void()> onEdit,
        const float selectableHeight = 60.0f
    ) {
        CImGui::TableNextRow();
        CImGui::TableNextColumn();

        ImVec2 pos = CImGui::GetCursorScreenPos();
        if (CImGui::Selectable(("##Selectable_" + entryName).c_str(), false, 0, ImVec2(0.0f, selectableHeight))) {
            INVOKE_REQUIRED_CALLBACK(onEdit);
        }

        std::string presetGroupStr = presetGroup ? presetGroup->name : "Default";

        // Sex Mark ONLY if using defaults i.e. "Male" / "Female" label
        ImVec2 sexMarkerPos = pos;
        ImVec2 sexMarkerSize = { 0.0f, 0.0f };
        float sexMarkerSpacingAfter = 0.0f;

        if (entryName == "Male" || entryName == "Female") {
            // Sex Mark
            bool female = entryName == "Female";
            std::string sexMarkSymbol = female ? WS_FONT_FEMALE : WS_FONT_MALE;
            ImVec4 sexMarkerCol = female ? ImVec4(0.76f, 0.50f, 0.24f, 1.0f) : ImVec4(0.50f, 0.70f, 0.33f, 1.0f);

            sexMarkerSpacingAfter = 20.0f;
            sexMarkerSize = CImGui::CalcTextSize(sexMarkSymbol.c_str());
            sexMarkerPos.x = pos.x + CImGui::GetStyle().ItemSpacing.x;
            sexMarkerPos.y = pos.y + (selectableHeight - sexMarkerSize.y) * 0.5f;
            CImGui::PushFont(symbolFont, FONT_SIZE_DEFAULT_WILDS_SYMBOLS);
            CImGui::GetWindowDrawList()->AddText(sexMarkerPos, CImGui::GetColorU32(sexMarkerCol), sexMarkSymbol.c_str());
            CImGui::PopFont();
        }

        ImVec2 playerNameSize = CImGui::CalcTextSize(entryName.c_str());
        ImVec2 playerNamePos;
        playerNamePos.x = sexMarkerPos.x + sexMarkerSpacingAfter;
        playerNamePos.y = pos.y + (selectableHeight - playerNameSize.y) * 0.5f;
        CImGui::GetWindowDrawList()->AddText(playerNamePos, CImGui::GetColorU32(ImGuiCol_Text), entryName.c_str());

        // Preset Group Sex Mark
        bool female = false;
        float presetGroupSexMarkSpacing = CImGui::GetStyle().ItemSpacing.x;
        float presetGroupSexMarkSpacingBefore = 0.0f;
        float endPos = CImGui::GetCursorScreenPos().x + CImGui::GetContentRegionAvail().x;
        if (presetGroup) {
            bool female = presetGroup->female;
            presetGroupSexMarkSpacing = 15.0f;
            presetGroupSexMarkSpacingBefore = 10.0f;

            std::string presetGroupSexMarkSymbol = female ? WS_FONT_FEMALE : WS_FONT_MALE;
            ImVec4 presetGroupSexMarkCol = female ? ImVec4(0.76f, 0.50f, 0.24f, 1.0f) : ImVec4(0.50f, 0.70f, 0.33f, 1.0f);

            ImVec2 presetGroupSexMarkSize = CImGui::CalcTextSize(presetGroupSexMarkSymbol.c_str());
            ImVec2 presetGroupSexMarkPos;
            presetGroupSexMarkPos.x = endPos - presetGroupSexMarkSpacingBefore - CImGui::GetStyle().ItemSpacing.x;
            presetGroupSexMarkPos.y = pos.y + (selectableHeight - presetGroupSexMarkSize.y) * 0.5f;

            CImGui::PushFont(symbolFont, FONT_SIZE_DEFAULT_WILDS_SYMBOLS);
            CImGui::GetWindowDrawList()->AddText(presetGroupSexMarkPos, CImGui::GetColorU32(presetGroupSexMarkCol), presetGroupSexMarkSymbol.c_str());
            CImGui::PopFont();
        }

        // Preset Group Name
        ImVec2 presetGroupNameSize = CImGui::CalcTextSize(presetGroupStr.c_str());
        ImVec2 presetGroupNamePos;
        presetGroupNamePos.x = endPos - (presetGroupNameSize.x + presetGroupSexMarkSpacing + presetGroupSexMarkSpacingBefore);
        presetGroupNamePos.y = pos.y + (selectableHeight - presetGroupNameSize.y) * 0.5f;

        ImVec4 presetGroupNameCol = presetGroup == nullptr ? ImVec4(0.365f, 0.678f, 0.886f, 0.8f) : ImVec4(1.0f, 1.0f, 1.0f, 0.5f);
        CImGui::PushStyleColor(ImGuiCol_Text, presetGroupNameCol);
        CImGui::GetWindowDrawList()->AddText(presetGroupNamePos, CImGui::GetColorU32(ImGuiCol_Text), presetGroupStr.c_str());
        CImGui::PopStyleColor();
    }

}