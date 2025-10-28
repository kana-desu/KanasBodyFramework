#pragma once 

#include <kbf/util/functional/invoke_callback.hpp>

#include <kbf/cimgui/cimgui_funcs.hpp>

#include <functional>

namespace kbf {

	class BoneModifierInfoWidget {
    public:

        void onAddBoneModifier(std::function<void(void)> callback) { addBoneModifierCb = callback; }

		bool draw(
            bool* compactMode,
            bool* categorizeBones,
            bool* symmetry,
            float* modLimit
        ) {
            if (compactMode)     m_compactMode = *compactMode;
            if (categorizeBones) m_categorizeBones = *categorizeBones;
            if (symmetry)        m_symmetry = *symmetry;
            if (modLimit)        m_modLimit = *modLimit;

            if (CImGui::Button("Add Bone Modifier", ImVec2(CImGui::GetContentRegionAvail().x, 0))) {
                INVOKE_REQUIRED_CALLBACK(addBoneModifierCb);
            }

            CImGui::Spacing();
            CImGui::Separator();
            CImGui::Spacing();

            CImGui::Checkbox(" Compact ", &m_compactMode);
            CImGui::SetItemTooltip("Compact: per-bone, vertical sliders in a single line.\nStandard: per-bone, horizontal sliders in 3 lines, and you can type exact values.");
            if (compactMode) *compactMode = m_compactMode;

            CImGui::SameLine();
            CImGui::Checkbox(" Categorize ", &m_categorizeBones);
            CImGui::SetItemTooltip("Categorize bones into separate tables based on body part (e.g. chest, arms, spine).");
            if (categorizeBones) *categorizeBones = m_categorizeBones;

            CImGui::SameLine();
            CImGui::Checkbox(" L/R Symmetry ", &m_symmetry);
            CImGui::SetItemTooltip("When enabled, bones with left (L_) & right (R_) pairs will be combined into one set of sliders.\n Any modifiers applied to the set will be reflected on the opposite bones.");
            if (symmetry) *symmetry = m_symmetry;
            
            CImGui::SameLine();
            constexpr const char* modLimitLabel = " Mod Limit ";
            float reservedWidth = CImGui::CalcTextSize(modLimitLabel).x;
            CImGui::SetNextItemWidth(CImGui::GetContentRegionAvail().x - reservedWidth);
            CImGui::DragFloat(modLimitLabel, &m_modLimit, 0.01f, 0.01f, 5.0f, "%.2f");
            if (m_modLimit < 0.0f) m_modLimit = 0;
            CImGui::SetItemTooltip("The maximum value of any bone modifier for this preset.\nSet this to your expected max modifier to see differences more clearly.");
            if (modLimit) *modLimit = m_modLimit;
            
            CImGui::Spacing();

            constexpr const char* hintText = "Note: Right click a modifier to set it to zero. Hold shift to edit x,y,z all at once.";
            const float hintWidth = CImGui::CalcTextSize(hintText).x;
            CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
            CImGui::SetCursorPosX(CImGui::GetCursorPosX() + (CImGui::GetContentRegionAvail().x - hintWidth) * 0.5f);
            CImGui::Text(hintText);
            CImGui::PopStyleColor();

            constexpr const char* hintText2 = "Bones in red don't exist in the current cache and may not actually be present in-game.";
            const float hintWidth2 = CImGui::CalcTextSize(hintText2).x;
            CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
            CImGui::SetCursorPosX(CImGui::GetCursorPosX() + (CImGui::GetContentRegionAvail().x - hintWidth2) * 0.5f);
            CImGui::Text(hintText2);
            CImGui::PopStyleColor();

            CImGui::Spacing();
            CImGui::Separator();
            CImGui::Spacing();

            return true;
		}

    private:
        bool m_compactMode = true;
        bool m_categorizeBones = true;
        bool m_symmetry = true;
        float m_modLimit = 1.0f;

        std::function<void(void)> addBoneModifierCb;

	};

}