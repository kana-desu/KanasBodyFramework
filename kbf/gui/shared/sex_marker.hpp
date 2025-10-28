#pragma once

#include <kbf/data/ids/font_symbols.hpp>
#include <kbf/util/font/default_font_sizes.hpp>

#include <kbf/cimgui/cimgui_funcs.hpp>

#include <string>

namespace kbf {

    inline void drawSexMarker(ImFont* symbolFont, const bool male, const bool sameline, const bool center) {
        std::string symbol = male ? WS_FONT_MALE : WS_FONT_FEMALE;
        std::string tooltip = male ? "Male" : "Female";
        ImVec4 colour = male ? ImVec4(0.50f, 0.70f, 0.33f, 1.0f) : ImVec4(0.76f, 0.50f, 0.24f, 1.0f);

        CImGui::PushFont(symbolFont, FONT_SIZE_DEFAULT_WILDS_SYMBOLS);
        CImGui::PushStyleColor(ImGuiCol_Text, colour);

        if (center) CImGui::SetCursorPosX(CImGui::GetCursorPosX() + (CImGui::GetColumnWidth() - CImGui::CalcTextSize(symbol.c_str()).x) * 0.5f);

        CImGui::Text(symbol.c_str());
        if (sameline) CImGui::SameLine();
        CImGui::PopStyleColor(1);
        CImGui::PopFont();

        if (CImGui::IsItemHovered()) CImGui::SetTooltip(tooltip.c_str());
    }

}