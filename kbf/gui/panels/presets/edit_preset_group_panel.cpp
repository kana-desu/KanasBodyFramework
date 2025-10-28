#include <kbf/gui/panels/presets/edit_preset_group_panel.hpp>

#include <kbf/data/ids/font_symbols.hpp>
#include <kbf/data/ids/special_armour_ids.hpp>
#include <kbf/data/ids/preset_ids.hpp>
#include <kbf/data/armour/armour_list.hpp>
#include <kbf/util/id/uuid_generator.hpp>
#include <kbf/util/functional/invoke_callback.hpp>
#include <kbf/debug/debug_stack.hpp>

#include <format>

#define EDIT_PRESET_GROUP_PANEL_LOG_TAG "[EditPresetGroupPanel]"

namespace kbf {

    EditPresetGroupPanel::EditPresetGroupPanel(
        const std::string& presetGroupUUID,
        const std::string& name,
        const std::string& strID,
        const KBFDataManager& dataManager,
        ImFont* wsSymbolFont
    ) : iPanel(name, strID), 
        presetGroupUUID{ presetGroupUUID },
        dataManager{ dataManager }, 
        wsSymbolFont{ wsSymbolFont } 
    {
        const PresetGroup* presetPtr = dataManager.getPresetGroupByUUID(presetGroupUUID);
        presetGroupBefore = *presetPtr;
        presetGroup       = *presetPtr;

        initializeBuffers();
    }

    void EditPresetGroupPanel::initializeBuffers() {
        std::strcpy(presetGroupNameBuffer, presetGroup.name.c_str());
    }

    bool EditPresetGroupPanel::draw() {
        bool open = true;
        processFocus();

        copyPresetGroupPanel.draw();

        CImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, ImVec2(0.5f, 0.5f));
        CImGui::Begin(nameWithID.c_str(), &open, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);

        if (CImGui::Button("Copy Existing Preset Group", ImVec2(CImGui::GetContentRegionAvail().x, 0))) {
            openCopyPresetGroupPanel();
        }

        CImGui::Spacing();
        CImGui::Separator();
        CImGui::Spacing();

        CImGui::InputText(" Name ", presetGroupNameBuffer, IM_ARRAYSIZE(presetGroupNameBuffer));
        presetGroup.name = std::string{ presetGroupNameBuffer };

        CImGui::Spacing();
        std::string sexComboValue = presetGroup.female ? "Female" : "Male";
        if (CImGui::BeginCombo(" Sex ", sexComboValue.c_str())) {
            if (CImGui::Selectable("Male")) {
                presetGroup.female = false;
            }
            if (CImGui::Selectable("Female")) {
                presetGroup.female = true;
            };
            CImGui::EndCombo();
        }
        CImGui::SetItemTooltip("Suggested character sex to use with (not a hard restriction)");

        CImGui::Spacing();
        CImGui::Spacing();
        static constexpr const char* kDeleteLabel = "Delete";
        static constexpr const char* kCancelLabel = "Cancel";
        static constexpr const char* kEditorLabel = "Open In Editor";
        static constexpr const char* kUpdateLabel = "Update";

        CImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f)); // Muted red
        CImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
        CImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.1f, 0.1f, 1.0f));
        if (CImGui::Button(kDeleteLabel)) {
            INVOKE_REQUIRED_CALLBACK(deleteCallback, presetGroupUUID);
        }
        CImGui::PopStyleColor(3);

        float availableWidth = CImGui::GetContentRegionAvail().x;
        float spacing = CImGui::GetStyle().ItemSpacing.x;
        float cancelButtonWidth = CImGui::CalcTextSize(kCancelLabel).x + CImGui::GetStyle().FramePadding.x * 2;
        float editorButtonWidth = CImGui::CalcTextSize(kEditorLabel).x + CImGui::GetStyle().FramePadding.x * 2;
        float updateButtonWidth = CImGui::CalcTextSize(kUpdateLabel).x + CImGui::GetStyle().FramePadding.x * 2;
        float totalWidth = updateButtonWidth + editorButtonWidth + cancelButtonWidth + spacing;

        // Cancel Button
        CImGui::SameLine();

        float cancelButtonPos = availableWidth - totalWidth;
        CImGui::SetCursorPosX(cancelButtonPos);
        CImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
        CImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.20f, 0.20f, 1.0f));
        CImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.10f, 0.10f, 0.10f, 1.0f));

        if (CImGui::Button(kCancelLabel)) {
            INVOKE_REQUIRED_CALLBACK(cancelCallback);
        }

        CImGui::PopStyleColor(3);

        // Editor Button
        CImGui::SameLine();

        if (CImGui::Button(kEditorLabel)) {
            INVOKE_REQUIRED_CALLBACK(openEditorCallback, presetGroupUUID);
        }
        CImGui::SetItemTooltip("Edit presets used per armour set, etc.");

        // Update Button
        CImGui::SameLine();

        const bool nameEmpty = presetGroup.name.empty();
        const bool alreadyExists = presetGroup.name != presetGroupBefore.name && dataManager.presetGroupExists(presetGroup.name);
        const bool disableUpdateButton = nameEmpty || alreadyExists;
        if (disableUpdateButton) CImGui::BeginDisabled();
        if (CImGui::Button(kUpdateLabel)) {
            INVOKE_REQUIRED_CALLBACK(updateCallback, presetGroupUUID, presetGroup);
        }
        if (disableUpdateButton) CImGui::EndDisabled();
        if (nameEmpty) CImGui::SetItemTooltip("Please provide a group name");
        else if (alreadyExists) CImGui::SetItemTooltip("Group name already taken");

        CImGui::End();
        CImGui::PopStyleVar();

        return open;
    }

    void EditPresetGroupPanel::openCopyPresetGroupPanel() {
        copyPresetGroupPanel.openNew("Copy Existing Preset Group", "CreatePresetPanel_CopyPanel", dataManager, wsSymbolFont, false);
        copyPresetGroupPanel.get()->focus();

        copyPresetGroupPanel.get()->onSelectPresetGroup([&](std::string uuid) {
            const PresetGroup* copyPresetGroup = dataManager.getPresetGroupByUUID(uuid);
            if (copyPresetGroup) {
                std::string nameBefore = presetGroup.name;
                presetGroup = *copyPresetGroup;
                presetGroup.uuid = presetGroupBefore.uuid; // Make sure UUID remains the same.
                presetGroup.name = nameBefore;
                initializeBuffers();

                initializeBuffers();
            }
            else {
                DEBUG_STACK.push(std::format("{} Could not find preset group with UUID {} while trying to make a copy.", EDIT_PRESET_GROUP_PANEL_LOG_TAG, uuid), DebugStack::Color::ERROR);
            }
            copyPresetGroupPanel.close();
        });
    }

}