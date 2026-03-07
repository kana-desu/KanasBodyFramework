#pragma once

#include <kbf/data/kbf_data_manager.hpp>
#include <kbf/player/player_tracker.hpp>
#include <kbf/npc/npc_tracker.hpp>
#include <kbf/situation/camera_manager.hpp>
#include <kbf/gui/tabs/editor/editor_tab.hpp>
#include <kbf/cimgui/cimgui_funcs.hpp>

namespace kbf {

    class BoneVisualizer {
    public:
    BoneVisualizer(
        KBFDataManager& dataManager,
        PlayerTracker& playerTracker,
        NpcTracker& npcTracker,
        EditorTab& editorTab);

        void draw();

    private:
        KBFDataManager& dataManager;
        PlayerTracker& playerTracker;
        NpcTracker& npcTracker;
        CameraManager& cam;
        EditorTab& editorTab;

        void drawBoneDots(
            CImGui::ImDrawListTransparent* drawList,
            const BoneManager& bm,
            const PresetPieceSettings& settings,
            const glm::mat4& viewProjMatrix,
            const glm::vec2& displaySize,
            ArmourPiece ap,
            ImVec4 baseCol,
            ImVec4 highlightCol,
            const HoveredBone& hoveredBone
        );

        bool armourSetMatchesPreview(const ArmourSet& previewSet, ArmourInfo& checkArmour, ArmourPiece ap);
    };

}
