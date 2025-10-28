#pragma once

#include <glm/glm.hpp>
#include <kbf/cimgui/cimgui_funcs.hpp>

namespace kbf {

    inline bool ImBoneSlider(const char* label, const ImVec2& size, float* value, float maxExtent, const char* fmt = "", const char* tooltipFmt = "%.2f") {
        ImGuiStyle& style = CImGui::GetStyle();

        // Define target pastel colors (in RGB)
        constexpr float pastelness  = 0.4f;
        const glm::vec3 pastelGreenRGB = glm::vec3(pastelness, 1.0f, pastelness);
        const glm::vec3 pastelRedRGB   = glm::vec3(1.0f, pastelness, pastelness);

        // Convert to HSV
		ImVec4 greenHSV{ 0, 0, 0, 0 }, redHSV{ 0, 0, 0, 0 };
        CImGui::ColorConvertRGBtoHSV(pastelGreenRGB.r, pastelGreenRGB.g, pastelGreenRGB.b, &greenHSV.x, &greenHSV.y, &greenHSV.z);
        CImGui::ColorConvertRGBtoHSV(pastelRedRGB.r, pastelRedRGB.g, pastelRedRGB.b, &redHSV.x, &redHSV.y, &redHSV.z);
        greenHSV.w = redHSV.w = 1.0f;

        // Compute normalized deviation
        float absDeviation = glm::clamp(std::abs(*value) / maxExtent, 0.0f, 1.0f);
        float ramp = glm::pow(absDeviation, 0.4f); // non-linear ramp for better low-end distinction

        // Choose base HSV (white has same hue, s = 0, v = 1)
        ImVec4 targetHSV = (*value >= 0.0f) ? greenHSV : redHSV;
        ImVec4 mixedHSV = {
            targetHSV.x,                                 // keep hue constant
            targetHSV.y * ramp,                          // boost saturation fade
            1.0f - (1.0f - targetHSV.z) * ramp,          // tweak brightness fade if desired
            1.0f
        };

        // Convert interpolated HSV back to RGB
        float r, g, b;
        CImGui::ColorConvertHSVtoRGB(mixedHSV.x, mixedHSV.y, mixedHSV.z, &r, &g, &b);
        glm::vec3 finalRGB(r, g, b);

        // Affected ImGui color indices
        const ImGuiCol affectedCols[] = {
            ImGuiCol_SliderGrab,
            ImGuiCol_SliderGrabActive,
            ImGuiCol_FrameBg,
            ImGuiCol_FrameBgHovered,
            ImGuiCol_FrameBgActive
        };

        // Push interpolated colors
        for (ImGuiCol col : affectedCols) {
            float alpha = style.Colors[col].w;
            CImGui::PushStyleColor(col, ImVec4(finalRGB.r, finalRGB.g, finalRGB.b, alpha));
        }

        // Draw the slider
        bool changed = CImGui::VSliderFloat(label, size, value, -maxExtent, +maxExtent, fmt);

        if (CImGui::IsItemActive() || CImGui::IsItemHovered())
            CImGui::SetTooltip(tooltipFmt, *value);

        if (CImGui::IsItemHovered() && CImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            *value = 0.0f;
            changed = true;
        }

        // Restore style
        CImGui::PopStyleColor(IM_ARRAYSIZE(affectedCols));

        return changed;
    }

    inline bool ImBoneSliderH(const char* label, float width, float* value, float speed, float maxExtent, const char* fmt = "%.2f") {
        ImGuiStyle& style = CImGui::GetStyle();

        // Define target pastel colors (in RGB)
        constexpr float pastelness = 0.4f;
        const glm::vec3 pastelGreenRGB = glm::vec3(pastelness, 1.0f, pastelness);
        const glm::vec3 pastelRedRGB = glm::vec3(1.0f, pastelness, pastelness);

        // Convert to HSV
        ImVec4 greenHSV{ 0, 0, 0, 0 }, redHSV{ 0, 0, 0, 0 };
        CImGui::ColorConvertRGBtoHSV(pastelGreenRGB.r, pastelGreenRGB.g, pastelGreenRGB.b, &greenHSV.x, &greenHSV.y, &greenHSV.z);
        CImGui::ColorConvertRGBtoHSV(pastelRedRGB.r, pastelRedRGB.g, pastelRedRGB.b, &redHSV.x, &redHSV.y, &redHSV.z);
        greenHSV.w = redHSV.w = 1.0f;

        // Compute normalized deviation
        float absDeviation = glm::clamp(std::abs(*value) / maxExtent, 0.0f, 1.0f);
        float ramp = glm::pow(absDeviation, 0.4f); // non-linear ramp for better low-end distinction

        // Choose base HSV (white has same hue, s = 0, v = 1)
        ImVec4 targetHSV = (*value >= 0.0f) ? greenHSV : redHSV;
        ImVec4 mixedHSV = {
            targetHSV.x,                                 // keep hue constant
            targetHSV.y * ramp,                          // boost saturation fade
            1.0f - (1.0f - targetHSV.z) * ramp,          // tweak brightness fade if desired
            1.0f
        };

        // Convert interpolated HSV back to RGB
        float r, g, b;
        CImGui::ColorConvertHSVtoRGB(mixedHSV.x, mixedHSV.y, mixedHSV.z, &r, &g, &b);
        glm::vec3 finalRGB(r, g, b);

        // Affected ImGui color indices
        const ImGuiCol affectedCols[] = {
            ImGuiCol_SliderGrab,
            ImGuiCol_SliderGrabActive,
            ImGuiCol_FrameBg,
            ImGuiCol_FrameBgHovered,
            ImGuiCol_FrameBgActive
        };

        // Push interpolated colors
        for (ImGuiCol col : affectedCols) {
            float alpha = style.Colors[col].w;
            CImGui::PushStyleColor(col, ImVec4(finalRGB.r, finalRGB.g, finalRGB.b, alpha));
        }

        if (CImGui::IsKeyDown(ImGuiKey_LeftShift)) speed /= 10.0f; // Revert internal imgui fast tweak behaviour

        // Draw the slider
        CImGui::PushItemWidth(width);
        bool changed = CImGui::DragFloat(label, value, speed, -maxExtent, +maxExtent, fmt);
        CImGui::PopItemWidth();

        if (CImGui::IsItemHovered() && CImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            *value = 0.0f;
            changed = true;
        }

        // Restore style
        CImGui::PopStyleColor(IM_ARRAYSIZE(affectedCols));

        return changed;
    }

}