#pragma once

#include <kbf/cimgui/cimgui_funcs.hpp>
#include <glm/glm.hpp>

namespace kbf {


    inline void ImTickMark(ImVec2 center, float scale = 1.0f, float thickness = 1.0f, ImU32 color = CImGui::GetColorU32(ImGuiCol_Text)) {
        float size = CImGui::GetFontSize() * scale;     // Final size based on font size and scale

        // Define points relative to center
        ImVec2 p1 = ImVec2(center.x - size * 0.35f, center.y + size * 0.05f);  // lower left
        ImVec2 p2 = ImVec2(center.x - size * 0.1f, center.y + size * 0.35f);  // joint
        ImVec2 p3 = ImVec2(center.x + size * 0.45f, center.y - size * 0.35f);  // upper right

        CImGui::GetWindowDrawList()->AddLine(p1, p2, color, thickness);
        CImGui::GetWindowDrawList()->AddLine(p2, p3, color, thickness);
    }

}