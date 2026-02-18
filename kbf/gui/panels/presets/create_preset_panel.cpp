#include <kbf/gui/panels/presets/create_preset_panel.hpp>

#include <kbf/data/ids/font_symbols.hpp>
#include <kbf/data/ids/special_armour_ids.hpp>
#include <kbf/data/ids/preset_ids.hpp>
#include <kbf/util/id/uuid_generator.hpp>
#include <kbf/util/functional/invoke_callback.hpp>
#include <kbf/debug/debug_stack.hpp>
#include <kbf/util/font/default_font_sizes.hpp>

#include <format>

#define CREATE_PRESET_PANEL_LOG_TAG "[CreatePresetPanel]"

namespace kbf {

    CreatePresetPanel::CreatePresetPanel(
        const std::string& name,
        const std::string& strID,
        const KBFDataManager& dataManager,
        ImFont* wsSymbolFont,
        ImFont* wsArmourFont,
        std::string defaultBundle
    ) : iPanel(name, strID), dataManager{ dataManager }, wsSymbolFont{ wsSymbolFont }, wsArmourFont{ wsArmourFont } {
        preset        = Preset{};
        preset.name   = "New Preset";
        preset.uuid   = uuid::v4::UUID::New().String();
        preset.bundle = defaultBundle.empty() ? PRESET_DEFAULT_BUNDLE : defaultBundle;
        preset.female = true;
        preset.armour = ArmourSet{ ANY_ARMOUR_ID, false };

        initializeBuffers();
    }

    void CreatePresetPanel::initializeBuffers() {
        std::strcpy(presetNameBuffer, preset.name.c_str());
        std::strcpy(presetBundleBuffer, preset.bundle.c_str());
    }

    bool CreatePresetPanel::draw() {
        bool open = true;
        processFocus();

        copyPresetPanel.draw();

        CImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, ImVec2(0.5f, 0.5f));

        constexpr ImVec2 minWindowSize = ImVec2(450, 600);

        if (isFirstDraw()) { CImGui::SetNextWindowSize(minWindowSize); }
        CImGui::SetNextWindowSizeConstraints(minWindowSize, ImVec2(FLT_MAX, FLT_MAX));
        CImGui::Begin(nameWithID.c_str(), &open, ImGuiWindowFlags_NoCollapse);

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

		const bool nameEmpty = preset.name.empty();
        const bool bundleEmpty = preset.bundle.empty();
        const bool alreadyExists = dataManager.presetExists(preset.name);
		const bool disableCreateButton = nameEmpty || bundleEmpty || alreadyExists;
        if (disableCreateButton) CImGui::BeginDisabled();
        if (CImGui::Button(kCreateLabel)) {
            INVOKE_REQUIRED_CALLBACK(createCallback, preset);
        }
        if (disableCreateButton) CImGui::EndDisabled();
        if (nameEmpty) CImGui::SetItemTooltip("Please provide a preset name");
        if (bundleEmpty) CImGui::SetItemTooltip("Please provide a bundle name");
        else if (alreadyExists) CImGui::SetItemTooltip("Preset name already taken");

        CImGui::End();
        CImGui::PopStyleVar();

        return open;
    }

    void CreatePresetPanel::drawArmourList(const std::string& filter) {
        std::vector<ArmourSet> armours = ArmourDataManager::get().getFilteredArmourSets(filter);

        // Fixed-height, scrollable region
        CImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.02f, 0.02f, 0.02f, 1.0f));
        CImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 10));

        const float height = -(2.0f * CImGui::GetFrameHeightWithSpacing() + 10.0f); // Leave space for the search bar
        CImGui::BeginChild("ArmourListChild", ImVec2(0, height), true, ImGuiWindowFlags_HorizontalScrollbar);

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

    void CreatePresetPanel::drawArmourSetName(const ArmourSet& armourSet, const float offsetBefore, const float offsetAfter) {
        // Sex Mark
        std::string symbol  = armourSet.female ? WS_FONT_FEMALE : WS_FONT_MALE;
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

    void CreatePresetPanel::openCopyPresetPanel() {
        copyPresetPanel.openNew("Copy Existing Preset", "CreatePresetPanel_CopyPanel", dataManager, wsSymbolFont, wsArmourFont, false);
        copyPresetPanel.get()->focus();

        copyPresetPanel.get()->onSelectPreset([&](std::string uuid) {
            const Preset* copyPreset = dataManager.getPresetByUUID(uuid);
            if (copyPreset) {
                std::string nameBefore = preset.name;
                preset = *copyPreset;
                preset.uuid = uuid::v4::UUID::New().String(); // Make sure to change the UUID.
                preset.name = nameBefore;
                initializeBuffers();
            }
            else {
                DEBUG_STACK.push(std::format("{} Could not find preset with UUID {} while trying to make a copy.", CREATE_PRESET_PANEL_LOG_TAG, uuid), DebugStack::Color::COL_ERROR);
            }
            copyPresetPanel.close();
        });
    }

}