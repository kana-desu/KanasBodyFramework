#include <kbf/gui/panels/presets/edit_player_override_panel.hpp>

#include <kbf/debug/debug_stack.hpp>
#include <kbf/data/ids/font_symbols.hpp>

#include <kbf/util/string/to_lower.hpp>
#include <kbf/util/functional/invoke_callback.hpp>
#include <kbf/util/font/default_font_sizes.hpp>

#include <algorithm>

#define EDIT_PLAYER_OVERRIDE_PANEL_LOG_TAG "[EditPlayerOverridePanel]"

namespace kbf {

    EditPlayerOverridePanel::EditPlayerOverridePanel(
        const PlayerData& playerData,
        const std::string& label,
        const std::string& strID,
        const KBFDataManager& dataManager,
        ImFont* wsSymbolFont
    ) : iPanel(label, strID), dataManager{ dataManager }, wsSymbolFont{ wsSymbolFont } {
        const PlayerOverride* overridePtr = dataManager.getPlayerOverride(playerData);
        playerOverrideBefore = *overridePtr;
        playerOverride       = *overridePtr;

        std::strcpy(playerNameBuffer, playerOverride.player.name.c_str());
        std::strcpy(hunterIdBuffer,   playerOverride.player.hunterId.c_str());
    }

    bool EditPlayerOverridePanel::draw() {
        bool open = true;
        processFocus();
        CImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, ImVec2(0.5f, 0.5f));
        CImGui::Begin(nameWithID.c_str(), &open, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);

        std::string playerNameStr{ playerNameBuffer };
        CImGui::InputText(" Name ", playerNameBuffer, IM_ARRAYSIZE(playerNameBuffer));
        playerOverride.player.name = playerNameStr;

        CImGui::Spacing();
        std::string hunterIdStr{ hunterIdBuffer };
        CImGui::InputText(" Hunter ID ", hunterIdBuffer, IM_ARRAYSIZE(hunterIdBuffer));
        playerOverride.player.hunterId = hunterIdStr;

        CImGui::Spacing();
        std::string sexComboValue = playerOverride.player.female ? "Female" : "Male";
        if (CImGui::BeginCombo(" Sex ", sexComboValue.c_str())) {
            if (CImGui::Selectable("Male")) {
                playerOverride.player.female = false;
            }
            if (CImGui::Selectable("Female")) {
                playerOverride.player.female = true;
            };
            CImGui::EndCombo();
        }

        CImGui::Spacing();

        const PresetGroup* activeGroup = dataManager.getPresetGroupByUUID(playerOverride.presetGroup);

        std::string activeGroupStr = playerOverride.presetGroup.empty() || activeGroup == nullptr ? "Default" : activeGroup->name;
        static char activeGroupStrBuffer[128] = "";
        std::strcpy(activeGroupStrBuffer, activeGroupStr.c_str());

        CImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, 1.0f); // Prevent fading
        CImGui::BeginDisabled();
        CImGui::InputText("Preset Group", activeGroupStrBuffer, IM_ARRAYSIZE(activeGroupStrBuffer));
        CImGui::EndDisabled();
        CImGui::PopStyleVar();

        CImGui::Spacing();

        static char filterBuffer[128] = "";
        std::string filterStr{ filterBuffer };

        drawPresetGroupList(dataManager.getPresetGroups(filterStr, true));

        CImGui::PushItemWidth(-1);
        CImGui::InputTextWithHint("##Search", "Search...", filterBuffer, IM_ARRAYSIZE(filterBuffer));
        CImGui::PopItemWidth();

        CImGui::Spacing();
        CImGui::Spacing();

        constexpr const char* kDeleteLabel = "Delete";
        constexpr const char* kCancelLabel = "Cancel";
		constexpr const char* kUpdateLabel = "Update";

        float spacing = CImGui::GetStyle().ItemSpacing.x;
        float buttonWidth1 = CImGui::CalcTextSize(kDeleteLabel).x + CImGui::GetStyle().FramePadding.x * 2;
        float buttonWidth2 = CImGui::CalcTextSize(kCancelLabel).x + CImGui::GetStyle().FramePadding.x * 2;
        float buttonWidth3 = CImGui::CalcTextSize(kUpdateLabel).x + CImGui::GetStyle().FramePadding.x * 2;
        float totalWidth = buttonWidth1 + buttonWidth2 + buttonWidth3 + spacing * 2.0f;

        float availableWidth = CImGui::GetContentRegionAvail().x;
        CImGui::SetCursorPosX(availableWidth - totalWidth + 8.0f); // Align to the right

        CImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f)); // Muted red
        CImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
        CImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.1f, 0.1f, 1.0f));
        if (CImGui::Button(kDeleteLabel)) {
            INVOKE_REQUIRED_CALLBACK(deleteCallback, playerOverrideBefore.player);
        }
        CImGui::PopStyleColor(3);

        CImGui::SameLine();

        CImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));       
        CImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.20f, 0.20f, 1.0f));
        CImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.10f, 0.10f, 0.10f, 1.0f)); 
        if (CImGui::Button(kCancelLabel)) {
            INVOKE_REQUIRED_CALLBACK(cancelCallback);
        }
        CImGui::PopStyleColor(3);

        CImGui::SameLine();

        bool cantUpdate = playerOverride.player != playerOverrideBefore.player && dataManager.playerOverrideExists(playerOverride.player);

        if (cantUpdate) CImGui::BeginDisabled();
        if (CImGui::Button(kUpdateLabel)) {
            // Validate preset group can be found
            if (!playerOverride.presetGroup.empty() && dataManager.getPresetGroupByUUID(playerOverride.presetGroup) == nullptr) {
                DEBUG_STACK.push(std::format("{} Updated player override: {} uses an invalid preset group: {}. Reverting to default...", EDIT_PLAYER_OVERRIDE_PANEL_LOG_TAG, playerOverride.player.string(), playerOverride.presetGroup), DebugStack::Color::WARNING);
            }
            INVOKE_REQUIRED_CALLBACK(updateCallback, playerOverrideBefore.player, playerOverride);
        }
        if (cantUpdate) CImGui::EndDisabled();
        if (cantUpdate) CImGui::SetItemTooltip("An override for the specified player already exists.");

        CImGui::End();
        CImGui::PopStyleVar();

        return open;
    }

    void EditPlayerOverridePanel::drawPresetGroupList(const std::vector<const PresetGroup*>& presetGroups) {
        // Fixed-height, scrollable region
        CImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.02f, 0.02f, 0.02f, 1.0f));
        CImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 10));
        CImGui::BeginChild("PresetGroupChild", ImVec2(0, 150), true, ImGuiWindowFlags_HorizontalScrollbar);

        float contentRegionWidth = CImGui::GetContentRegionAvail().x;
        const float selectableHeight = CImGui::GetTextLineHeight();

        CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.365f, 0.678f, 0.886f, 0.8f));
        if (CImGui::Selectable("Default", playerOverride.presetGroup.empty(), 0, ImVec2(0.0f, selectableHeight))) {
            playerOverride.presetGroup = "";
        }
        CImGui::PopStyleColor();

        if (presetGroups.size() > 0) CImGui::Separator();

        for (const PresetGroup* group : presetGroups)
        {
            ImVec2 pos = CImGui::GetCursorScreenPos();

            if (CImGui::Selectable(("##" + group->name).c_str(), playerOverride.presetGroup == group->uuid, 0, ImVec2(0.0f, selectableHeight))) {
                playerOverride.presetGroup = group->uuid;
            }

            // Sex Mark
            std::string sexMarkSymbol = group->female ? WS_FONT_FEMALE : WS_FONT_MALE;
            ImVec4 sexMarkerCol = group->female ? ImVec4(0.76f, 0.50f, 0.24f, 1.0f) : ImVec4(0.50f, 0.70f, 0.33f, 1.0f);

            CImGui::PushFont(wsSymbolFont, FONT_SIZE_DEFAULT_WILDS_SYMBOLS);

            constexpr float sexMarkerSpacingAfter = 5.0f;
            constexpr float sexMarkerVerticalAlignOffset = 5.0f;
            ImVec2 sexMarkerSize = CImGui::CalcTextSize(sexMarkSymbol.c_str());
            ImVec2 sexMarkerPos;
            sexMarkerPos.x = pos.x + CImGui::GetStyle().ItemSpacing.x;
            sexMarkerPos.y = pos.y + (selectableHeight - sexMarkerSize.y) * 0.5f + sexMarkerVerticalAlignOffset;
            CImGui::GetWindowDrawList()->AddText(sexMarkerPos, CImGui::GetColorU32(sexMarkerCol), sexMarkSymbol.c_str());

            CImGui::PopFont();

            // Group name
            ImVec2 presetGroupNameSize = CImGui::CalcTextSize(group->name.c_str());
            ImVec2 presetGroupNamePos;
            presetGroupNamePos.x = sexMarkerPos.x + sexMarkerSize.x + sexMarkerSpacingAfter;
            presetGroupNamePos.y = pos.y + (selectableHeight - presetGroupNameSize.y) * 0.5f;
            CImGui::GetWindowDrawList()->AddText(presetGroupNamePos, CImGui::GetColorU32(ImGuiCol_Text), group->name.c_str());

            std::string presetCountStr;
            if (group->size() == 0) presetCountStr = "Empty";
            else if (group->size() == 1) presetCountStr = "1 Preset";
            else presetCountStr = std::to_string(group->size()) + " Presets";

            ImVec2 rightTextSize = CImGui::CalcTextSize(presetCountStr.c_str());
            float hunterIdCursorPosX = CImGui::GetCursorScreenPos().x + contentRegionWidth - rightTextSize.x - CImGui::GetStyle().ItemSpacing.x;
            float hunterIdCursorPosY = CImGui::GetItemRectMin().y + 4.0f;  // Same Y as the selectable item, plus vertical alignment

            CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
            CImGui::GetWindowDrawList()->AddText(ImVec2(hunterIdCursorPosX, hunterIdCursorPosY), CImGui::GetColorU32(ImGuiCol_Text), presetCountStr.c_str());
            CImGui::PopStyleColor();
        }

        CImGui::EndChild();
        CImGui::PopStyleVar();
        CImGui::PopStyleColor();

    }

}