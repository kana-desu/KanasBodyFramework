#pragma once

#include <kbf/cimgui/cimgui_funcs.hpp>
#include <glm/glm.hpp>

namespace kbf {

    inline bool DragSelectable(
        const char* label,
        bool selected,
        float itemHeight = 0.0f,
        bool* wasHovered = nullptr)
    {
        ImVec2 cusrsorPos = CImGui::GetCursorScreenPos();
        glm::vec2 start = glm::vec2(cusrsorPos.x, cusrsorPos.y);
        float height = itemHeight > 0 ? itemHeight : CImGui::GetFontSize();
        glm::vec2 size = glm::vec2(CImGui::GetContentRegionAvail().x, height);
        glm::vec2 end = start + size;

        // Reserve space & check interaction
        CImGui::InvisibleButton(label, ImVec2(std::max(size.x, 1.0f), std::max(size.y, 1.0f)));
        bool clicked = CImGui::IsItemClicked();

        // Internal hover detection (supports dragging over items)
        bool hovered = CImGui::IsItemHovered() || CImGui::IsMouseHoveringRect(ImVec2(start.x, start.y), ImVec2(end.x, end.y));
        if (wasHovered) *wasHovered = hovered;

        // Determine color based on state
        ImU32 col = 0;
        if (selected)
            col = CImGui::GetColorU32(ImGuiCol_Header);
        else if (hovered)
            col = CImGui::GetColorU32(ImGuiCol_HeaderHovered);

        // Draw background and text
        if (col)
            CImGui::GetWindowDrawList()->AddRectFilled(ImVec2(start.x, start.y), ImVec2(end.x, end.y), col);

        return clicked;
    }

}