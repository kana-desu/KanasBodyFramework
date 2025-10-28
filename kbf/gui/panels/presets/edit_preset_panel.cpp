#include <kbf/gui/panels/presets/edit_preset_panel.hpp>

#include <kbf/data/ids/font_symbols.hpp>
#include <kbf/data/ids/special_armour_ids.hpp>
#include <kbf/data/armour/armour_list.hpp>
#include <kbf/util/id/uuid_generator.hpp>
#include <kbf/util/functional/invoke_callback.hpp>
#include <kbf/debug/debug_stack.hpp>
#include <kbf/util/font/default_font_sizes.hpp>

#include <format>

#define EDIT_PRESET_PANEL_LOG_TAG "[EditPresetPanel]"

namespace kbf {

    EditPresetPanel::EditPresetPanel(
        const std::string& presetUUID,
        const std::string& name,
        const std::string& strID,
        const KBFDataManager& dataManager,
        ImFont* wsSymbolFont,
        ImFont* wsArmourFont
    ) : iPanel(name, strID),
        presetUUID{ presetUUID },
        dataManager{ dataManager },
        wsSymbolFont{ wsSymbolFont },
        wsArmourFont{ wsArmourFont } 
    {
        const Preset* presetPtr = dataManager.getPresetByUUID(presetUUID);
        presetBefore = *presetPtr;
        preset       = *presetPtr;

        initializeBuffers();
    }

    void EditPresetPanel::initializeBuffers() {
        std::strcpy(presetNameBuffer, preset.name.c_str());
        std::strcpy(presetBundleBuffer, preset.bundle.c_str());
    }

    bool EditPresetPanel::draw() {
        bool open = true;
        processFocus();

        copyPresetPanel.draw();

        CImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, ImVec2(0.5f, 0.5f));
        CImGui::Begin(nameWithID.c_str(), &open, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);

        if (CImGui::Button("Copy Existing Preset", ImVec2(CImGui::GetContentRegionAvail().x, 0))) {
            openCopyPresetPanel();
        }

        CImGui::Spacing();
        CImGui::Separator();
        CImGui::Spacing();

        CImGui::InputText(" Name ", presetNameBuffer, IM_ARRAYSIZE(presetNameBuffer));
        preset.name = std::string{ presetNameBuffer };

        CImGui::Spacing();
        CImGui::InputText(" Bundle ", presetBundleBuffer, IM_ARRAYSIZE(presetBundleBuffer));
        preset.bundle = std::string{ presetBundleBuffer };
        CImGui::SetItemTooltip("Enables sorting similar presets under one title");

        CImGui::Spacing();
        std::string sexComboValue = preset.female ? "Female" : "Male";
        if (CImGui::BeginCombo(" Sex ", sexComboValue.c_str())) {
            if (CImGui::Selectable("Male")) {
                preset.female = false;
            }
            if (CImGui::Selectable("Female")) {
                preset.female = true;
            };
            CImGui::EndCombo();
        }
        CImGui::SetItemTooltip("Suggested character sex to use with (not a hard restriction)");

        CImGui::Spacing();
        CImGui::Separator();
        CImGui::Spacing();

        static char dummyStrBuffer[8] = "";

        CImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, 1.0f); // Prevent fading
        CImGui::BeginDisabled();
        CImGui::InputText(" Armour ", dummyStrBuffer, IM_ARRAYSIZE(dummyStrBuffer));
        CImGui::EndDisabled();
        CImGui::PopStyleVar();
        CImGui::SetItemTooltip("Suggested armour set to use with (not a hard restriction)");

        drawArmourSetName(preset.armour, 10.0f, 17.5f);

        CImGui::Spacing();

        static char filterBuffer[128] = "";
        std::string filterStr{ filterBuffer };

        drawArmourList(filterStr);

        CImGui::PushItemWidth(-1);
        CImGui::InputTextWithHint("##Search", "Search...", filterBuffer, IM_ARRAYSIZE(filterBuffer));
        CImGui::PopItemWidth();

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
            INVOKE_REQUIRED_CALLBACK(deleteCallback, presetUUID);
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
            INVOKE_REQUIRED_CALLBACK(openEditorCallback, presetUUID);
        }
        CImGui::SetItemTooltip("Edit all values, e.g. bone modifiers");

        // Update Button
        CImGui::SameLine();

        const bool nameEmpty = preset.name.empty();
        const bool bundleEmpty = preset.bundle.empty();
        const bool alreadyExists = preset.name != presetBefore.name && dataManager.presetExists(preset.name);
        const bool disableUpdateButton = nameEmpty || bundleEmpty || alreadyExists;
        if (disableUpdateButton) CImGui::BeginDisabled();
        if (CImGui::Button(kUpdateLabel)) {
            INVOKE_REQUIRED_CALLBACK(updateCallback, presetUUID, preset);
        }
        if (disableUpdateButton) CImGui::EndDisabled();
        if (nameEmpty) CImGui::SetItemTooltip("Please provide a preset name");
        if (bundleEmpty) CImGui::SetItemTooltip("Please provide a bundle name");
        else if (alreadyExists) CImGui::SetItemTooltip("Preset name already taken");

        CImGui::End();
        CImGui::PopStyleVar();

        return open;
    }

    void EditPresetPanel::drawArmourList(const std::string& filter) {
        std::vector<ArmourSet> armours = ArmourList::getFilteredSets(filter);

        // Fixed-height, scrollable region
        CImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.02f, 0.02f, 0.02f, 1.0f));
        CImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 10));
        CImGui::BeginChild("ArmourListChild", ImVec2(0, 150), true, ImGuiWindowFlags_HorizontalScrollbar);

        float contentRegionWidth = CImGui::GetContentRegionAvail().x;

        if (armours.size() == 0) {
            const char* noneFoundStr = "No Armours Found";

            CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
            CImGui::SetCursorPosX(CImGui::GetCursorPosX() + (CImGui::GetColumnWidth() - CImGui::CalcTextSize(noneFoundStr).x) * 0.5f);
            CImGui::Text(noneFoundStr);
            CImGui::PopStyleColor();
        }
        else {
            for (const auto& armourSet : armours)
            {
                std::string selectableId = std::format("##{}_{}", armourSet.name, armourSet.female ? "f" : "m");
                if (CImGui::Selectable(selectableId.c_str(), preset.armour == armourSet)) {
                    preset.armour = armourSet;
                }

                drawArmourSetName(armourSet, 5.0f, 17.5f);

                if (armours.size() > 1 && armourSet.name == ANY_ARMOUR_ID) CImGui::Separator();
            }
        }

        CImGui::EndChild();
        CImGui::PopStyleVar();
        CImGui::PopStyleColor();
    }

    void EditPresetPanel::drawArmourSetName(const ArmourSet& armourSet, const float offsetBefore, const float offsetAfter) {
        // Sex Mark
        std::string symbol = armourSet.female ? WS_FONT_FEMALE : WS_FONT_MALE;
        std::string tooltip = armourSet.female ? "Female" : "Male";
        ImVec4 colour = armourSet.female ? ImVec4(0.76f, 0.50f, 0.24f, 1.0f) : ImVec4(0.50f, 0.70f, 0.33f, 1.0f);

        CImGui::PushFont(wsSymbolFont, FONT_SIZE_DEFAULT_WILDS_SYMBOLS);
        CImGui::PushStyleColor(ImGuiCol_Text, colour);

        float sexMarkerCursorPosX = CImGui::GetCursorScreenPos().x + offsetBefore;
        float sexMarkerCursorPosY = CImGui::GetItemRectMin().y + 4.0f;  // Same Y as the selectable item, plus vertical alignment
        CImGui::GetWindowDrawList()->AddText(
            ImVec2(sexMarkerCursorPosX, sexMarkerCursorPosY),
            CImGui::GetColorU32(ImGuiCol_Text),
            symbol.c_str());

        CImGui::PopStyleColor();
        CImGui::PopFont();

        // Name
        float armourNameCursorPosX = CImGui::GetCursorScreenPos().x + offsetBefore + offsetAfter;
        float armourNameCursorPosY = CImGui::GetItemRectMin().y + 4.0f;  // Same Y as the selectable item, plus vertical alignment

        CImGui::PushFont(wsArmourFont, FONT_SIZE_DEFAULT_WILDS_ARMOUR);
        CImGui::GetWindowDrawList()->AddText(ImVec2(armourNameCursorPosX, armourNameCursorPosY), CImGui::GetColorU32(ImGuiCol_Text), armourSet.name.c_str());
        CImGui::PopFont();
    }

    void EditPresetPanel::openCopyPresetPanel() {
        copyPresetPanel.openNew("Copy Existing Preset", "CreatePresetPanel_CopyPanel", dataManager, wsSymbolFont, wsArmourFont, false);
        copyPresetPanel.get()->focus();

        copyPresetPanel.get()->onSelectPreset([&](std::string uuid) {
            const Preset* copyPreset = dataManager.getPresetByUUID(uuid);
            if (copyPreset) {
                std::string nameBefore = preset.name;
                preset = *copyPreset;
                preset.uuid = presetBefore.uuid; // Make sure UUID remains the same.
                preset.name = nameBefore;
                initializeBuffers();
            }
            else {
                DEBUG_STACK.push(std::format("{} Could not find preset with UUID {} while trying to make a copy.", EDIT_PRESET_PANEL_LOG_TAG, uuid), DebugStack::Color::ERROR);
            }
            copyPresetPanel.close();
            });
    }

}