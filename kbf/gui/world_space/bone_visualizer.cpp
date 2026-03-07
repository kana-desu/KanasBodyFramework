#include "kbf/gui/world_space/bone_visualizer.hpp"
#include <kbf/debug/debug_stack.hpp>
#include <glm/vec3.hpp>

#define BONE_VIS_LOG_TAG "[BoneVisualizer]"

namespace kbf {

    BoneVisualizer::BoneVisualizer(KBFDataManager& dataManager, PlayerTracker& playerTracker, NpcTracker& npcTracker, EditorTab& editorTab)
        : dataManager{ dataManager }, playerTracker{ playerTracker }, npcTracker{ npcTracker }, cam{ CameraManager::instance() }, editorTab{ editorTab } {}

    void BoneVisualizer::draw() {
        const Preset* preview = dataManager.getPreviewedPreset();
        if (!preview) return;
        if (!dataManager.settings().showJointsWhenEditing) return;

        std::optional<glm::mat4> viewProjMatrix = cam.getViewProjMatrix();
        if (!viewProjMatrix.has_value()) return;

        // Only draw bones for the armour piece tab currently active in the editor
        const std::optional<ArmourPiece>& activePiece = editorTab.getActiveArmourTab();
        if (!activePiece.has_value()) return;
        ArmourPiece ap = activePiece.value();
        PresetPieceSettings settings = preview->getPieceSettings(ap);

        auto ImIO = CImGui::GetIO();
        if (!ImIO) return;

        auto dl = CImGui::GetBackgroundDrawList();
        ImVec4 baseCol      = { 1.0f, 1.0f, 1.0f, 1.0f };
        ImVec4 highlightCol = { 0.0f, 1.0f, 1.0f, 1.0f };
        glm::vec2 dispSize = { ImIO->DisplaySize.x, ImIO->DisplaySize.y };

        // Draw dots for tracked players
        auto players = playerTracker.getPlayerList();
        for (const auto& pd : players) {
            try {
                // Don't show bone visualizers for players that aren't visible or too far away to be tracked
                const PlayerInfo& info = playerTracker.getPlayerInfo(pd);
                if (!info.visible) continue;

                std::optional<PersistentPlayerInfo>& pinfoOpt = playerTracker.getPersistentPlayerInfo(pd);
                if (!pinfoOpt.has_value()) continue;
                PersistentPlayerInfo& pinfo = *pinfoOpt;
                if (!pinfo.areSetPointersValid()) continue;
                if (!pinfo.boneManager.has_value()) continue;

                // Only show when the armour set matches the preset
                if (!armourSetMatchesPreview(preview->armour, pinfo.armourInfo, ap)) continue;

                const BoneManager& bm = pinfo.boneManager.value();
                drawBoneDots(dl.get(), bm, settings, viewProjMatrix.value(), dispSize, ap, baseCol, highlightCol, editorTab.getHoveredBone());
            }
            catch (...) {
                continue;
            }
        }

        // Draw dots for NPCs
        auto npcList = npcTracker.getNpcList();
        for (const auto idx : npcList) {
            try {
                const NpcInfo& info = npcTracker.getNpcInfo(idx);
                if (!info.visible) continue;

                std::optional<PersistentNpcInfo>& ninfoOpt = npcTracker.getPersistentNpcInfo(idx);
                if (!ninfoOpt.has_value()) continue;
                PersistentNpcInfo& ninfo = ninfoOpt.value();
                if (!ninfo.areSetPointersValid()) continue;
                if (!ninfo.boneManager.has_value()) continue;

                // Only show when the armour set matches the preset
                if (!armourSetMatchesPreview(preview->armour, ninfo.armourInfo, ap)) continue;

                const BoneManager& bm = *ninfo.boneManager;
                drawBoneDots(dl.get(), bm, settings, viewProjMatrix.value(), dispSize, ap, baseCol, highlightCol, editorTab.getHoveredBone());
            }
            catch (...) {
                continue;
            }
        }

    }

    void BoneVisualizer::drawBoneDots(
        CImGui::ImDrawListTransparent* drawList,
        const BoneManager& bm,
        const PresetPieceSettings& settings,
        const glm::mat4& viewProjMatrix,
        const glm::vec2& displaySize,
        ArmourPiece ap,
        ImVec4 baseCol,
        ImVec4 highlightCol,
        const HoveredBone& hoveredBone
    ) {
        bool hasHoveredBone = hoveredBone.primary.has_value() || hoveredBone.secondary.has_value();
        float normalBoneOpacity = hasHoveredBone ? 0.4f : 0.85f;

        ImU32 baseColU32      = CImGui::GetColorU32({ baseCol.x, baseCol.y, baseCol.z, baseCol.w * normalBoneOpacity });
        ImU32 highlightColU32 = CImGui::GetColorU32(highlightCol);

        // Draw all non-hovered bones first
        for (const auto& [boneName, modifier] : settings.modifiers) {
            // Defer hovered bones to end of draw calls
            if (hoveredBone.primary.value_or("") == boneName)   continue;
            if (hoveredBone.secondary.value_or("") == boneName) continue;

            auto posOpt = bm.getBoneWorldPosition(ap, boneName);
            if (!posOpt) continue;
            glm::vec3 pos = *posOpt;
            auto screenOpt = cam.worldToScreen(pos, viewProjMatrix, displaySize);
            if (!screenOpt) continue;
            glm::vec2 sc = *screenOpt;

            drawList->AddCircleFilled(ImVec2(sc.x, sc.y), 3.0f, baseColU32);
            if (dataManager.settings().showJointNamesWhenEditing) {
                ImVec2 textPos(sc.x + 4.0f, sc.y - 6.0f);
                drawList->AddText(textPos, baseColU32, boneName.c_str());
            }
        }

        // Now draw hovered bones last (on top)
        if (hoveredBone.primary.has_value()) {
            const std::string& bn = *hoveredBone.primary;
            auto posOpt = bm.getBoneWorldPosition(ap, bn);
            if (posOpt) {
                glm::vec3 pos = *posOpt;
                auto screenOpt = cam.worldToScreen(pos, glm::uvec2{ static_cast<uint32_t>(displaySize.x), static_cast<uint32_t>(displaySize.y) });
                if (screenOpt) {
                    glm::vec2 sc = *screenOpt;
                    drawList->AddCircleFilled(ImVec2(sc.x, sc.y), 5.0f, highlightColU32);
                    if (dataManager.settings().showJointNamesWhenEditing) {
                        ImU32 outlineCol = CImGui::GetColorU32(ImVec4(0, 0, 0, 255));
                        ImVec2 textPos(sc.x + 6.0f, sc.y - 8.0f);
                        drawList->AddText(ImVec2(textPos.x - 1, textPos.y - 1), outlineCol, bn.c_str());
                        drawList->AddText(ImVec2(textPos.x + 1, textPos.y - 1), outlineCol, bn.c_str());
                        drawList->AddText(ImVec2(textPos.x - 1, textPos.y + 1), outlineCol, bn.c_str());
                        drawList->AddText(ImVec2(textPos.x + 1, textPos.y + 1), outlineCol, bn.c_str());
                        drawList->AddText(textPos, highlightColU32, bn.c_str());
                    }
                }
            }
        }

        if (hoveredBone.secondary.has_value()) {
            const std::string& bn2 = *hoveredBone.secondary;
            auto posOpt2 = bm.getBoneWorldPosition(ap, bn2);
            if (posOpt2) {
                glm::vec3 pos = *posOpt2;
                auto screenOpt = cam.worldToScreen(pos, glm::uvec2{ static_cast<uint32_t>(displaySize.x), static_cast<uint32_t>(displaySize.y) });
                if (screenOpt) {
                    glm::vec2 sc = *screenOpt;
                    drawList->AddCircleFilled(ImVec2(sc.x, sc.y), 5.0f, highlightColU32);
                    if (dataManager.settings().showJointNamesWhenEditing) {
                        ImU32 outlineCol = CImGui::GetColorU32(ImVec4(0, 0, 0, 255));
                        ImVec2 textPos(sc.x + 6.0f, sc.y - 8.0f);
                        drawList->AddText(ImVec2(textPos.x - 1, textPos.y - 1), outlineCol, bn2.c_str());
                        drawList->AddText(ImVec2(textPos.x + 1, textPos.y - 1), outlineCol, bn2.c_str());
                        drawList->AddText(ImVec2(textPos.x - 1, textPos.y + 1), outlineCol, bn2.c_str());
                        drawList->AddText(ImVec2(textPos.x + 1, textPos.y + 1), outlineCol, bn2.c_str());
                        drawList->AddText(textPos, highlightColU32, bn2.c_str());
                    }
                }
            }
        }
    }

    bool BoneVisualizer::armourSetMatchesPreview(const ArmourSet& previewSet, ArmourInfo& checkArmour, ArmourPiece ap) {
        if (previewSet == ArmourSet::DEFAULT) return true;
        if (ap == ArmourPiece::AP_SET) {
            // Allow any armour loadout that contains AT LEAST ONE piece from this set.
            size_t minAP = static_cast<size_t>(ArmourPiece::AP_MIN_EXCLUDING_SET);
            size_t maxAP = static_cast<size_t>(ArmourPiece::AP_MAX_EXCLUDING_SLINGER);
            for (size_t piece = minAP; piece < maxAP; piece++) {
                std::optional<ArmourSet> pieceSet = checkArmour.getPiece(static_cast<ArmourPiece>(piece));
                if (pieceSet.has_value() && pieceSet == previewSet) return true;
            }
            return false;
        } 
        else {
            // For a specific piece, check that direct armour set mapping only
            std::optional<ArmourSet> pieceSet = checkArmour.getPiece(ap);
            return (pieceSet.has_value() && pieceSet == previewSet);
        }

        return false;
    }


}
