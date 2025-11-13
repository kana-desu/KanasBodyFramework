#include <kbf/gui/panels/presets/create_preset_group_panel.hpp>

#include <kbf/data/ids/font_symbols.hpp>
#include <kbf/data/ids/special_armour_ids.hpp>
#include <kbf/data/ids/preset_ids.hpp>
#include <kbf/data/armour/armour_list.hpp>
#include <kbf/util/id/uuid_generator.hpp>
#include <kbf/util/functional/invoke_callback.hpp>
#include <kbf/debug/debug_stack.hpp>

#include <format>

#define CREATE_PRESET_GROUP_PANEL_LOG_TAG "[CreatePresetGroupPanel]"

namespace kbf {

    CreatePresetGroupPanel::CreatePresetGroupPanel(
        const std::string& name,
        const std::string& strID,
        const KBFDataManager& dataManager,
        ImFont* wsSymbolFont
    ) : iPanel(name, strID), dataManager{ dataManager }, wsSymbolFont{ wsSymbolFont } {
        presetGroup = PresetGroup{};
        presetGroup.name = "New Preset Group";
        presetGroup.uuid = uuid::v4::UUID::New().String();
        presetGroup.female = true;

        initializeBuffers();
    }

    void CreatePresetGroupPanel::initializeBuffers() {
        std::strcpy(presetGroupNameBuffer, presetGroup.name.c_str());
    }

    bool CreatePresetGroupPanel::draw() {
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

        static constexpr const char* kCancelLabel = "Cancel";
        static constexpr const char* kCreateLabel = "Create";

        float spacing = CImGui::GetStyle().ItemSpacing.x;
        float buttonWidth1 = CImGui::CalcTextSize(kCancelLabel).x + CImGui::GetStyle().FramePadding.x * 2;
        float buttonWidth2 = CImGui::CalcTextSize(kCreateLabel).x + CImGui::GetStyle().FramePadding.x * 2;
        float totalWidth = buttonWidth1 + buttonWidth2 + spacing;

        float availableWidth = CImGui::GetContentRegionAvail().x;
        CImGui::SetCursorPosX(availableWidth - totalWidth + 8.0f); // Align to the right

        CImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
        CImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.20f, 0.20f, 1.0f));
        CImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.10f, 0.10f, 0.10f, 1.0f));

        if (CImGui::Button(kCancelLabel)) {
            INVOKE_REQUIRED_CALLBACK(cancelCallback);
        }

        CImGui::PopStyleColor(3);

        CImGui::SameLine();

        const bool nameEmpty = presetGroup.name.empty();
        const bool alreadyExists = dataManager.presetGroupExists(presetGroup.name);
        const bool disableCreateButton = nameEmpty || alreadyExists;
        if (disableCreateButton) CImGui::BeginDisabled();
        if (CImGui::Button(kCreateLabel)) {
            INVOKE_REQUIRED_CALLBACK(createCallback, presetGroup);
        }
        if (disableCreateButton) CImGui::EndDisabled();
        if (nameEmpty) CImGui::SetItemTooltip("Please provide a preset group name");
        else if (alreadyExists) CImGui::SetItemTooltip("Preset group name already taken");

        CImGui::End();
        CImGui::PopStyleVar();

        return open;
    }

    void CreatePresetGroupPanel::openCopyPresetGroupPanel() {
        copyPresetGroupPanel.openNew("Copy Existing Preset Group", "CreatePresetPanel_CopyPanel", dataManager, wsSymbolFont, false);
        copyPresetGroupPanel.get()->focus();

        copyPresetGroupPanel.get()->onSelectPresetGroup([&](std::string uuid) {
            const PresetGroup* copyPresetGroup = dataManager.getPresetGroupByUUID(uuid);
            if (copyPresetGroup) {
                std::string nameBefore = presetGroup.name;
                presetGroup = *copyPresetGroup;
                presetGroup.uuid = uuid::v4::UUID::New().String(); // Make sure to change the UUID.
                presetGroup.name = nameBefore;
                initializeBuffers();
            }
            else {
                DEBUG_STACK.push(std::format("{} Could not find preset group with UUID {} while trying to make a copy.", CREATE_PRESET_GROUP_PANEL_LOG_TAG, uuid), DebugStack::Color::COL_ERROR);
            }
            copyPresetGroupPanel.close();
        });
    }

}