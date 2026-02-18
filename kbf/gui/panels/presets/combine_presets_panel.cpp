#include <kbf/gui/panels/presets/combine_presets_panel.hpp>

#include <kbf/gui/shared/tab_bar_separator.hpp>
#include <kbf/data/bones/common_bones.hpp>
#include <kbf/data/ids/font_symbols.hpp>
#include <kbf/data/ids/special_armour_ids.hpp>
#include <kbf/data/ids/preset_ids.hpp>
#include <kbf/util/id/uuid_generator.hpp>
#include <kbf/util/functional/invoke_callback.hpp>
#include <kbf/debug/debug_stack.hpp>
#include <kbf/util/font/default_font_sizes.hpp>

#include <format>

#define COMBINE_PRESETS_PANEL_LOG_TAG "[CombinePresetsPanel]"

namespace kbf {

    CombinePresetsPanel::CombinePresetsPanel(
        const std::string& name,
        const std::string& strID,
        const KBFDataManager& dataManager,
        ImFont* wsSymbolFont,
        ImFont* wsArmourFont,
        std::string defaultBundle
    ) : iPanel(name, strID), dataManager{ dataManager }, wsSymbolFont{ wsSymbolFont }, wsArmourFont{ wsArmourFont } {
        preset = Preset{};
        preset.name = "New Combined Preset";
        preset.uuid = uuid::v4::UUID::New().String();
        preset.bundle = defaultBundle.empty() ? PRESET_DEFAULT_BUNDLE : defaultBundle;
        preset.female = true;
        preset.armour = ArmourSet{ ANY_ARMOUR_ID, false };

        initializeBuffers();
    }

    void CombinePresetsPanel::initializeBuffers() {
        std::strcpy(presetNameBuffer, preset.name.c_str());
        std::strcpy(presetBundleBuffer, preset.bundle.c_str());
    }

    bool CombinePresetsPanel::draw() {
        bool open = true;
        processFocus();

        presetSelectorPanel.draw();

        CImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, ImVec2(0.5f, 0.5f));

        constexpr ImVec2 minWindowSize = ImVec2(600, 600);

        if (isFirstDraw()) { CImGui::SetNextWindowSize(minWindowSize); }
        CImGui::SetNextWindowSizeConstraints(minWindowSize, ImVec2(FLT_MAX, FLT_MAX));
        CImGui::Begin(nameWithID.c_str(), &open, ImGuiWindowFlags_NoCollapse);

        if (CImGui::BeginTabBar("CombinedPresetsTabs")) {
            if (CImGui::BeginTabItem("Properties")) {
                CImGui::Spacing();
                drawPropertiesTab();
                CImGui::EndTabItem();
            }
            if (CImGui::IsItemClicked()) presetSelectorPanel.close();

            if (CImGui::BeginTabItem("Combined Presets")) {
                CImGui::Spacing();
                drawCombinedPresetsTab();
                CImGui::EndTabItem();
            }
            if (CImGui::IsItemClicked()) presetSelectorPanel.close();

            CImGui::EndTabBar();
        }

        CImGui::Spacing();

        static constexpr const char* kCancelLabel = "Cancel";
        static constexpr const char* kCombineLabel = "Combine";

        float spacing = CImGui::GetStyle().ItemSpacing.x;
        float buttonWidth1 = CImGui::CalcTextSize(kCancelLabel).x + CImGui::GetStyle().FramePadding.x * 2;
        float buttonWidth2 = CImGui::CalcTextSize(kCombineLabel).x + CImGui::GetStyle().FramePadding.x * 2;
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
        if (CImGui::Button(kCombineLabel)) {
            INVOKE_REQUIRED_CALLBACK(combineCallback, getFinalCombinedPreset());
        }
        if (disableCreateButton) CImGui::EndDisabled();
        if (nameEmpty) CImGui::SetItemTooltip("Please provide a preset name");
        if (bundleEmpty) CImGui::SetItemTooltip("Please provide a bundle name");
        else if (alreadyExists) CImGui::SetItemTooltip("Preset name already taken");

        CImGui::End();
        CImGui::PopStyleVar();

        return open;
    }

    void CombinePresetsPanel::drawPropertiesTab() {
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
    }

    void CombinePresetsPanel::drawCombinedPresetsTab() {
        static char dummyStrBuffer[8] = "";

        CImGui::BeginChild("CombineTabContents", ImVec2(0, -52));

        drawCombinePresetSelector(" Head "         , combine_head);
        drawCombinePresetSelector(" Body "         , combine_body);
        drawCombinePresetSelector(" Arms "         , combine_arms);
        drawCombinePresetSelector(" Waist "        , combine_coil);
        drawCombinePresetSelector(" Legs "         , combine_legs);

        CImGui::EndChild();

        CImGui::Spacing();
    }

    void CombinePresetsPanel::drawCombinePresetSelector(const std::string& label, Preset& presetOut) {
        static char strBuffer[255] = "";
        std::strcpy(strBuffer, presetOut.name.c_str());

        if (CImGui::Button(("Select##" + label).c_str())) {
            openPresetSelectorPanel(presetOut);
        }
        CImGui::SameLine();

        CImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, 1.0f);
        CImGui::BeginDisabled();
        CImGui::InputText(label.c_str(), strBuffer, IM_ARRAYSIZE(strBuffer));
        CImGui::EndDisabled();
        CImGui::PopStyleVar();
    }

    void CombinePresetsPanel::drawArmourList(const std::string& filter) {
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

    void CombinePresetsPanel::drawArmourSetName(const ArmourSet& armourSet, const float offsetBefore, const float offsetAfter) {
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

    Preset CombinePresetsPanel::getFinalCombinedPreset() {
        Preset combined = preset;

        bool hideSlinger = false;
        hideSlinger |= combine_head.hideSlinger;
        hideSlinger |= combine_body.hideSlinger;
        hideSlinger |= combine_arms.hideSlinger;
        hideSlinger |= combine_coil.hideSlinger;
        hideSlinger |= combine_legs.hideSlinger;
        combined.hideSlinger;

        bool hideWeapon = false;
        hideWeapon |= combine_head.hideSlinger;
        hideWeapon |= combine_body.hideSlinger;
        hideWeapon |= combine_arms.hideSlinger;
        hideWeapon |= combine_coil.hideSlinger;
        hideWeapon |= combine_legs.hideSlinger;
        combined.hideWeapon = hideWeapon;

        //combined.set = ; // TODO: Combine these from the relevant bones in all the rest of the sets
        auto mergeSetModifiers = [&](const auto& source, auto isPrimaryBone) {
            for (const auto& [k, v] : source.set.modifiers) {
                if (isPrimaryBone(k) || isCustomOrUncommonBone(k))
                    combined.set.modifiers[k] = v;
            }
        };

        mergeSetModifiers(combine_head, isHeadBone);
        mergeSetModifiers(combine_body, isBodyBone);
        mergeSetModifiers(combine_arms, isArmsBone);
        mergeSetModifiers(combine_legs, isLegsBone);

        combined.helm = combine_head.helm;
        combined.body = combine_body.body;
        combined.arms = combine_arms.arms;
        combined.coil = combine_coil.coil;
        combined.legs = combine_legs.legs;

        // (Badly) combine quick override material maps
        // Float
        for (const auto& [k, v] : combine_head.quickMaterialOverridesFloat) combined.quickMaterialOverridesFloat[k] = v;
        for (const auto& [k, v] : combine_body.quickMaterialOverridesFloat) combined.quickMaterialOverridesFloat[k] = v;
        for (const auto& [k, v] : combine_arms.quickMaterialOverridesFloat) combined.quickMaterialOverridesFloat[k] = v;
        for (const auto& [k, v] : combine_coil.quickMaterialOverridesFloat) combined.quickMaterialOverridesFloat[k] = v;
        for (const auto& [k, v] : combine_legs.quickMaterialOverridesFloat) combined.quickMaterialOverridesFloat[k] = v;

        // Vec4
        for (const auto& [k, v] : combine_head.quickMaterialOverridesVec4) combined.quickMaterialOverridesVec4[k] = v;
        for (const auto& [k, v] : combine_body.quickMaterialOverridesVec4) combined.quickMaterialOverridesVec4[k] = v;
        for (const auto& [k, v] : combine_arms.quickMaterialOverridesVec4) combined.quickMaterialOverridesVec4[k] = v;
        for (const auto& [k, v] : combine_coil.quickMaterialOverridesVec4) combined.quickMaterialOverridesVec4[k] = v;
        for (const auto& [k, v] : combine_legs.quickMaterialOverridesVec4) combined.quickMaterialOverridesVec4[k] = v;

        return combined;
    }

    void CombinePresetsPanel::openPresetSelectorPanel(Preset& presetOut) {
        presetSelectorPanel.openNew("Select Preset to Combine", "CombinePresetPanel_PresetSelector", dataManager, wsSymbolFont, wsArmourFont, false);
        presetSelectorPanel.get()->focus();

        presetSelectorPanel.get()->onSelectPreset([&](std::string uuid) {
            const Preset* copyPreset = dataManager.getPresetByUUID(uuid);
            if (copyPreset) {
                presetOut = *copyPreset;
            }
            else {
                DEBUG_STACK.push(std::format("{} Could not find preset with UUID {} while trying to combine.", COMBINE_PRESETS_PANEL_LOG_TAG, uuid), DebugStack::Color::COL_ERROR);
            }
            presetSelectorPanel.close();
        });
    }

}