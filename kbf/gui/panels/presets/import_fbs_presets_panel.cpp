#include <kbf/gui/panels/presets/import_fbs_presets_panel.hpp>

#include <kbf/data/ids/font_symbols.hpp>
#include <kbf/data/ids/special_armour_ids.hpp>
#include <kbf/data/ids/preset_ids.hpp>
#include <kbf/util/id/uuid_generator.hpp>
#include <kbf/util/functional/invoke_callback.hpp>
#include <kbf/util/string/to_lower.hpp>
#include <kbf/debug/debug_stack.hpp>
#include <kbf/util/font/default_font_sizes.hpp>
#include <kbf/gui/shared/alignment.hpp>

#include <format>

namespace kbf {

    ImportFbsPresetsPanel::ImportFbsPresetsPanel(
        const std::string& name,
        const std::string& strID,
        const KBFDataManager& dataManager,
        ImFont* wsSymbolFont,
        ImFont* wsArmourFont
    ) : iPanel(name, strID), dataManager{ dataManager }, wsSymbolFont{ wsSymbolFont }, wsArmourFont{ wsArmourFont } {
        initializeBuffers();
    }

    void ImportFbsPresetsPanel::postToMainThread(std::function<void()> func) {
        std::lock_guard<std::mutex> lock(callbackMutex);
        callbackQueue.push(std::move(func));
    }

    void ImportFbsPresetsPanel::processCallbacks() {
        std::lock_guard<std::mutex> lock(callbackMutex);
        while (!callbackQueue.empty()) {
            auto fn = std::move(callbackQueue.front());
            callbackQueue.pop();
            fn();
        }
    }

    void ImportFbsPresetsPanel::initializeBuffers() {
        std::strcpy(presetBundleBuffer, bundleName.c_str());
        std::strcpy(filterBuffer, "");
    }

    bool ImportFbsPresetsPanel::draw() {
        bool open = true;
        processFocus();
        processCallbacks();

        CImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, ImVec2(0.5f, 0.5f));

        constexpr ImVec2 minWindowSize = ImVec2(600, 450);
		constexpr ImVec2 loadingMinWindowSize = ImVec2(600, 100);

        if (isFirstDraw()) { CImGui::SetNextWindowSize(minWindowSize); }
		if (loadInProgress) CImGui::SetNextWindowSize(loadingMinWindowSize);

        CImGui::SetNextWindowSizeConstraints(loadInProgress ? loadingMinWindowSize : minWindowSize, ImVec2(FLT_MAX, FLT_MAX));
        CImGui::Begin(nameWithID.c_str(), &open, ImGuiWindowFlags_NoCollapse);

        float width = CImGui::GetWindowSize().x;

        if (!loadAttempted && !loadInProgress) {
            loadInProgress = true;
            progressFraction = 0.0f;

            std::thread([this]() {
                bool success = dataManager.getFBSpresets(&presets, true, "", &progressFraction);  // Long-running
                postToMainThread([this, success]() {
                    presetLoadFailed = !success;
                    loadAttempted = true;
                    loadInProgress = false;
                });
            }).detach();
        }

        if (!loadAttempted) {
            drawLoadingBar(progressFraction);
        }
        else {
            drawContent();
        }

        CImGui::End();
        CImGui::PopStyleVar();

        return open;
    }

    void ImportFbsPresetsPanel::drawLoadingBar(float fraction) {
		CImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
        CImGui::ProgressBar(fraction);
        CImGui::PopStyleColor();
    }

    void ImportFbsPresetsPanel::drawContent() {
        if (presetLoadFailed) {
            const char* failureStr = "Failed to load FBS presets.";
			CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
            CImGui::SetCursorPosX(CImGui::GetCursorPosX() + (CImGui::GetColumnWidth() - CImGui::CalcTextSize(failureStr).x) * 0.5f);
            CImGui::Text(failureStr);
            CImGui::PopStyleColor();
            CImGui::BeginDisabled();

            CImGui::Spacing();
            CImGui::Separator();
            CImGui::Spacing();
        }

        CImGui::InputText(" Bundle Name ", presetBundleBuffer, IM_ARRAYSIZE(presetBundleBuffer));
        bundleName = std::string{ presetBundleBuffer };
        CImGui::SetItemTooltip("Enables sorting similar presets under one title");

        CImGui::Spacing();
        std::string sexComboValue = presetsFemale ? "Female" : "Male";
        if (CImGui::BeginCombo(" Sex ", sexComboValue.c_str())) {
            if (CImGui::Selectable("Male")) {
                presetsFemale = false;
            }
            if (CImGui::Selectable("Female")) {
                presetsFemale = true;
            };
            CImGui::EndCombo();
        }
        CImGui::SetItemTooltip("Suggested character sex to use with (not a hard restriction)");


        CImGui::Spacing();
        CImGui::Separator();
        CImGui::Spacing();

		static bool autoswitchPresetsOnly = true;
        CImGui::Checkbox("Import Autoswitch Presets Only", &autoswitchPresetsOnly);

        CImGui::Spacing();

        CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
        constexpr char const* exportHintStr = "Click to select presets to import.";
        preAlignCellContentHorizontal(exportHintStr);
        CImGui::Text(exportHintStr);
        CImGui::Spacing();
        CImGui::PopStyleColor();

        const bool noSelection = selectedPresets.empty();

        drawPresetList(presets, autoswitchPresetsOnly, presetsFemale);

        static constexpr const char* kCancelLabel = "Cancel";
        static constexpr const char* kClearLabel = "Clear";
        const std::string selectedStr = std::format("Import {} Selected", selectedPresets.size());
        const char* kImportLabel = noSelection ? "Import All" : selectedStr.c_str();

        CImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(CImGui::GetStyle().ItemSpacing.x, 10));

        // A button to clear the selection
        if (noSelection) CImGui::BeginDisabled();
        if (CImGui::Button(kClearLabel)) selectedPresets.clear();
        if (noSelection) CImGui::EndDisabled();

        CImGui::SameLine();

        CImGui::PushItemWidth(-1);
        CImGui::InputTextWithHint("##Search", "Search...", filterBuffer, IM_ARRAYSIZE(filterBuffer));
        CImGui::PopItemWidth();

        CImGui::PopStyleVar();

        float spacingy = CImGui::GetStyle().ItemSpacing.y;
        float spacing = CImGui::GetStyle().ItemSpacing.x;
        float buttonWidth1 = CImGui::CalcTextSize(kCancelLabel).x + CImGui::GetStyle().FramePadding.x * 2;
        float buttonWidth2 = CImGui::CalcTextSize(kImportLabel).x + CImGui::GetStyle().FramePadding.x * 2;
        float totalWidth = buttonWidth1 + buttonWidth2 + spacing;

        float availableWidth = CImGui::GetContentRegionAvail().x;
        CImGui::SetCursorPosX(availableWidth - totalWidth + 8.0f); // Align to the right

        CImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
        CImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.20f, 0.20f, 1.0f));
        CImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.10f, 0.10f, 0.10f, 1.0f));

        if (presetLoadFailed) CImGui::EndDisabled();

        if (CImGui::Button(kCancelLabel)) {
            INVOKE_REQUIRED_CALLBACK(cancelCallback);
        }

        if (presetLoadFailed) CImGui::BeginDisabled();

        CImGui::PopStyleColor(3);

        CImGui::SameLine();

        const bool bundleEmpty = bundleName.empty();
        if (bundleEmpty) CImGui::BeginDisabled();
        if (CImGui::Button(kImportLabel)) {
			std::vector<Preset> presetsToCreate = createPresetList(autoswitchPresetsOnly, !noSelection);
            INVOKE_REQUIRED_CALLBACK(createCallback, presetsToCreate);
        }
        if (bundleEmpty) CImGui::EndDisabled();
        if (bundleEmpty) CImGui::SetItemTooltip("Please provide a bundle name");

        if (presetLoadFailed) CImGui::EndDisabled();
    }

    void ImportFbsPresetsPanel::drawPresetList(const std::vector<FBSPreset>& presets, bool autoSwitchOnly, const bool female) {
        // Fixed-height, scrollable region
        CImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.02f, 0.02f, 0.02f, 1.0f));
        CImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 10));

        const float height = -(CImGui::GetFrameHeightWithSpacing() * 2); // Leave space for the search bar
        CImGui::BeginChild("PresetGroupChild", ImVec2(0, height), true, ImGuiWindowFlags_HorizontalScrollbar);

        float contentRegionWidth = CImGui::GetContentRegionAvail().x;
        const float selectableHeight = CImGui::GetTextLineHeight();

        bool anyShown = false;
        for (size_t it = 0; it < presets.size(); ++it)
        {
            const FBSPreset& fbspreset = presets[it];
			const Preset& preset = fbspreset.preset;
			bool autoSwitch = fbspreset.autoswitchingEnabled;

			if (!autoSwitch && autoSwitchOnly) continue;

            std::string filterStr{ filterBuffer };
            const auto valor = toLower(preset.name).find(toLower(filterStr));
            if (!filterStr.empty() && valor == std::string::npos) continue;

            //A variable for showing the nothing found
            anyShown = true;

            bool selected = selectedPresets.contains(preset.uuid);

            ImVec2 pos = CImGui::GetCursorScreenPos();
            if (CImGui::Selectable(("##Selectable_Preset_" + preset.name).c_str(), selected, ImGuiSelectableFlags_SelectOnClick, ImVec2(0.0f, selectableHeight))) {
                if (selected) selectedPresets.erase(preset.uuid);
                else selectedPresets.insert(preset.uuid);
                selected = !selected;
            };

            // Sex Mark
            std::string sexMarkSymbol = female ? WS_FONT_FEMALE : WS_FONT_MALE;
            ImVec4 sexMarkerCol = female ? ImVec4(0.76f, 0.50f, 0.24f, 1.0f) : ImVec4(0.50f, 0.70f, 0.33f, 1.0f);

            CImGui::PushFont(wsSymbolFont, FONT_SIZE_DEFAULT_WILDS_SYMBOLS);

            constexpr float sexMarkerSpacingAfter = 5.0f;
            constexpr float sexMarkerVerticalAlignOffset = 5.0f;
            ImVec2 sexMarkerSize = CImGui::CalcTextSize(sexMarkSymbol.c_str());
            ImVec2 sexMarkerPos;
            sexMarkerPos.x = pos.x + CImGui::GetStyle().ItemSpacing.x;
            sexMarkerPos.y = pos.y + (selectableHeight - sexMarkerSize.y) * 0.5f + sexMarkerVerticalAlignOffset;
            CImGui::GetWindowDrawList()->AddText(sexMarkerPos, CImGui::GetColorU32(sexMarkerCol), sexMarkSymbol.c_str());

            CImGui::PopFont();

            CImGui::PushFont(dataManager.getRegularFontOverride(), FONT_SIZE_DEFAULT_MAIN);

            // Group name... floating because imgui has no vertical alignment STILL :(
            constexpr float playerNameSpacingAfter = 5.0f;
            ImVec2 playerNameSize = CImGui::CalcTextSize(preset.name.c_str());
            ImVec2 playerNamePos;
            playerNamePos.x = sexMarkerPos.x + sexMarkerSize.x + sexMarkerSpacingAfter;
            playerNamePos.y = pos.y + (selectableHeight - playerNameSize.y) * 0.5f;
            CImGui::GetWindowDrawList()->AddText(playerNamePos, CImGui::GetColorU32(ImGuiCol_Text), preset.name.c_str());

            CImGui::PopFont();

            // Armour Marks
            constexpr float armourVerticalAlignOffset = 2.5f;
            constexpr ImVec4 armourMissingCol{ 1.0f, 1.0f, 1.0f, 0.1f };
            constexpr ImVec4 armourPresentCol{ 1.0f, 1.0f, 1.0f, 1.0f };

            CImGui::PushFont(wsSymbolFont, FONT_SIZE_DEFAULT_WILDS_SYMBOLS);

            // Legs Mark
            ImVec2 legMarkSize = CImGui::CalcTextSize(WS_FONT_LEGS);
            ImVec2 legMarkPos;
            legMarkPos.x = CImGui::GetCursorScreenPos().x + contentRegionWidth - legMarkSize.x - CImGui::GetStyle().ItemSpacing.x;
            legMarkPos.y = pos.y + (selectableHeight - legMarkSize.y) * 0.5f + armourVerticalAlignOffset;
            CImGui::PushStyleColor(ImGuiCol_Text, preset.hasModifiers(ArmourPiece::AP_LEGS) ? armourPresentCol : armourMissingCol);
            CImGui::GetWindowDrawList()->AddText(legMarkPos, CImGui::GetColorU32(ImGuiCol_Text), WS_FONT_LEGS);
            CImGui::PopStyleColor();

            // Coil Mark
            ImVec2 coilMarkSize = CImGui::CalcTextSize(WS_FONT_COIL);
            ImVec2 coilMarkPos;
            coilMarkPos.x = legMarkPos.x - coilMarkSize.x - CImGui::GetStyle().ItemSpacing.x;
            coilMarkPos.y = pos.y + (selectableHeight - coilMarkSize.y) * 0.5f + armourVerticalAlignOffset;
            CImGui::PushStyleColor(ImGuiCol_Text, preset.hasModifiers(ArmourPiece::AP_COIL) ? armourPresentCol : armourMissingCol);
            CImGui::GetWindowDrawList()->AddText(coilMarkPos, CImGui::GetColorU32(ImGuiCol_Text), WS_FONT_COIL);
            CImGui::PopStyleColor();

            // Arms Mark
            ImVec2 armsMarkSize = CImGui::CalcTextSize(WS_FONT_ARMS);
            ImVec2 armsMarkPos;
            armsMarkPos.x = coilMarkPos.x - armsMarkSize.x - CImGui::GetStyle().ItemSpacing.x;
            armsMarkPos.y = pos.y + (selectableHeight - armsMarkSize.y) * 0.5f + armourVerticalAlignOffset;
            CImGui::PushStyleColor(ImGuiCol_Text, preset.hasModifiers(ArmourPiece::AP_ARMS) ? armourPresentCol : armourMissingCol);
            CImGui::GetWindowDrawList()->AddText(armsMarkPos, CImGui::GetColorU32(ImGuiCol_Text), WS_FONT_ARMS);
            CImGui::PopStyleColor();

            // Body Mark
            ImVec2 bodyMarkSize = CImGui::CalcTextSize(WS_FONT_BODY);
            ImVec2 bodyMarkPos;
            bodyMarkPos.x = armsMarkPos.x - bodyMarkSize.x - CImGui::GetStyle().ItemSpacing.x;
            bodyMarkPos.y = pos.y + (selectableHeight - bodyMarkSize.y) * 0.5f + armourVerticalAlignOffset;
            CImGui::PushStyleColor(ImGuiCol_Text, preset.hasModifiers(ArmourPiece::AP_BODY) ? armourPresentCol : armourMissingCol);
            CImGui::GetWindowDrawList()->AddText(bodyMarkPos, CImGui::GetColorU32(ImGuiCol_Text), WS_FONT_BODY);
            CImGui::PopStyleColor();

            // Helm Mark
            ImVec2 helmMarkSize = CImGui::CalcTextSize(WS_FONT_HELM);
            ImVec2 helmMarkPos;
            helmMarkPos.x = bodyMarkPos.x - helmMarkSize.x - CImGui::GetStyle().ItemSpacing.x;
            helmMarkPos.y = pos.y + (selectableHeight - helmMarkSize.y) * 0.5f + armourVerticalAlignOffset;
            CImGui::PushStyleColor(ImGuiCol_Text, preset.hasModifiers(ArmourPiece::AP_HELM) ? armourPresentCol : armourMissingCol);
            CImGui::GetWindowDrawList()->AddText(helmMarkPos, CImGui::GetColorU32(ImGuiCol_Text), WS_FONT_HELM);
            CImGui::PopStyleColor();

            // Set Mark
            ImVec2 setMarkSize = CImGui::CalcTextSize(WS_FONT_SET);
            ImVec2 setMarkPos;
            setMarkPos.x = helmMarkPos.x - setMarkSize.x - CImGui::GetStyle().ItemSpacing.x;
            setMarkPos.y = pos.y + (selectableHeight - setMarkSize.y) * 0.5f + armourVerticalAlignOffset;
            CImGui::PushStyleColor(ImGuiCol_Text, preset.hasModifiers(ArmourPiece::AP_SET) ? armourPresentCol : armourMissingCol);
            CImGui::GetWindowDrawList()->AddText(setMarkPos, CImGui::GetColorU32(ImGuiCol_Text), WS_FONT_SET);
            CImGui::PopStyleColor();

            // Armour Sex Mark
            std::string armourSexMarkSymbol = preset.armour.female ? WS_FONT_FEMALE : WS_FONT_MALE;
            ImVec4 armourSexMarkerCol = preset.armour.female ? ImVec4(0.76f, 0.50f, 0.24f, 1.0f) : ImVec4(0.50f, 0.70f, 0.33f, 1.0f);

            //constexpr float armourSexMarkerSpacingAfter = 5.0f;
            constexpr float armourSexMarkerVerticalAlignOffset = 5.0f;
            ImVec2 armourSexMarkerSize = CImGui::CalcTextSize(armourSexMarkSymbol.c_str());
            ImVec2 armourSexMarkerPos;
            armourSexMarkerPos.x = setMarkPos.x - armourSexMarkerSize.x - CImGui::GetStyle().ItemSpacing.x;
            armourSexMarkerPos.y = pos.y + (selectableHeight - armourSexMarkerSize.y) * 0.5f + armourSexMarkerVerticalAlignOffset;
            CImGui::GetWindowDrawList()->AddText(armourSexMarkerPos, CImGui::GetColorU32(armourSexMarkerCol), armourSexMarkSymbol.c_str());

            CImGui::PopFont();

            CImGui::PushFont(wsArmourFont, FONT_SIZE_DEFAULT_WILDS_ARMOUR);

            // Armour Name
            ImVec2 armourNameSize = CImGui::CalcTextSize(preset.armour.name.c_str());
            ImVec2 armourNamePos;
            armourNamePos.x = armourSexMarkerPos.x - armourNameSize.x - CImGui::GetStyle().ItemSpacing.x;
            armourNamePos.y = pos.y + (selectableHeight - armourNameSize.y) * 0.5f;

            ImVec4 armourNameCol = preset.armour.name == ANY_ARMOUR_ID ? ImVec4(0.365f, 0.678f, 0.886f, 0.8f) : ImVec4(1.0f, 1.0f, 1.0f, 0.5f);
            CImGui::PushStyleColor(ImGuiCol_Text, armourNameCol);
            CImGui::GetWindowDrawList()->AddText(armourNamePos, CImGui::GetColorU32(ImGuiCol_Text), preset.armour.name.c_str());
            CImGui::PopStyleColor();

            CImGui::PopFont();

            //// Legs Mark
            //constexpr ImVec4 armourMissingCol{ 1.0f, 1.0f, 1.0f, 0.1f };
            //constexpr ImVec4 armourPresentCol{ 1.0f, 1.0f, 1.0f, 1.0f };
            //constexpr float armourVerticalAlignOffset = 2.5f;
            //CImGui::PushFont(wsSymbolFont, FONT_SIZE_DEFAULT_WILDS_SYMBOLS);

            //ImVec2 legMarkSize = CImGui::CalcTextSize(WS_FONT_LEGS);
            //ImVec2 legMarkPos;
            //legMarkPos.x = CImGui::GetCursorScreenPos().x + contentRegionWidth - legMarkSize.x - CImGui::GetStyle().ItemSpacing.x;
            //legMarkPos.y = pos.y + (selectableHeight - legMarkSize.y) * 0.5f + armourVerticalAlignOffset;

            //CImGui::PushStyleColor(ImGuiCol_Text, preset.hasLegs() ? armourPresentCol : armourMissingCol);
            //CImGui::GetWindowDrawList()->AddText(legMarkPos, CImGui::GetColorU32(ImGuiCol_Text), WS_FONT_LEGS);
            //CImGui::PopStyleColor();

            //// Body Mark
            //ImVec2 bodyMarkSize = CImGui::CalcTextSize(WS_FONT_BODY);
            //ImVec2 bodyMarkPos;
            //bodyMarkPos.x = legMarkPos.x - bodyMarkSize.x - CImGui::GetStyle().ItemSpacing.x;
            //bodyMarkPos.y = pos.y + (selectableHeight - bodyMarkSize.y) * 0.5f + armourVerticalAlignOffset;

            //CImGui::PushStyleColor(ImGuiCol_Text, preset.hasBody() ? armourPresentCol : armourMissingCol);
            //CImGui::GetWindowDrawList()->AddText(bodyMarkPos, CImGui::GetColorU32(ImGuiCol_Text), WS_FONT_BODY);
            //CImGui::PopStyleColor();

            //// Armour Sex Mark
            //std::string armourSexMarkSymbol = preset.armour.female ? WS_FONT_FEMALE : WS_FONT_MALE;
            //ImVec4 armourSexMarkerCol = preset.armour.female ? ImVec4(0.76f, 0.50f, 0.24f, 1.0f) : ImVec4(0.50f, 0.70f, 0.33f, 1.0f);

            //constexpr float armourSexMarkerVerticalAlignOffset = 5.0f;
            //ImVec2 armourSexMarkerSize = CImGui::CalcTextSize(armourSexMarkSymbol.c_str());
            //ImVec2 armourSexMarkerPos;
            //armourSexMarkerPos.x = bodyMarkPos.x - armourSexMarkerSize.x - CImGui::GetStyle().ItemSpacing.x;
            //armourSexMarkerPos.y = pos.y + (selectableHeight - armourSexMarkerSize.y) * 0.5f + armourSexMarkerVerticalAlignOffset;
            //CImGui::GetWindowDrawList()->AddText(armourSexMarkerPos, CImGui::GetColorU32(armourSexMarkerCol), armourSexMarkSymbol.c_str());

            //CImGui::PopFont();

            //CImGui::PushFont(wsArmourFont, FONT_SIZE_DEFAULT_WILDS_ARMOUR);

            //// Armour Name
            //ImVec2 armourNameSize = CImGui::CalcTextSize(preset.armour.name.c_str());
            //ImVec2 armourNamePos;
            //armourNamePos.x = armourSexMarkerPos.x - armourNameSize.x - CImGui::GetStyle().ItemSpacing.x;
            //armourNamePos.y = pos.y + (selectableHeight - armourNameSize.y) * 0.5f;

            //ImVec4 armourNameCol = preset.armour.name == ANY_ARMOUR_ID ? ImVec4(0.365f, 0.678f, 0.886f, 0.8f) : ImVec4(1.0f, 1.0f, 1.0f, 0.5f);
            //CImGui::PushStyleColor(ImGuiCol_Text, armourNameCol);
            //CImGui::GetWindowDrawList()->AddText(armourNamePos, CImGui::GetColorU32(ImGuiCol_Text), preset.armour.name.c_str());
            //CImGui::PopStyleColor();

            //CImGui::PopFont();
        }

        if (presets.size() == 0 || !anyShown) {
            const char* noneFoundStr = "No FBS Presets Found";
            CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
            CImGui::SetCursorPosX(CImGui::GetCursorPosX() + (CImGui::GetColumnWidth() - CImGui::CalcTextSize(noneFoundStr).x) * 0.5f);
            CImGui::Text(noneFoundStr);
            CImGui::PopStyleColor();
		}

        CImGui::EndChild();
        CImGui::PopStyleVar();
        CImGui::PopStyleColor();

    }

    std::vector<Preset> ImportFbsPresetsPanel::createPresetList(bool autoswitchOnly, bool selectionsOnly) const {
        std::vector<Preset> presetsToCreate{};
        for (const FBSPreset& fbspreset : presets) {
            if (autoswitchOnly && !fbspreset.autoswitchingEnabled) continue;
            if (selectionsOnly && !selectedPresets.contains(fbspreset.preset.uuid)) continue;
            Preset processedPreset = fbspreset.preset;
            processedPreset.bundle = bundleName;
            presetsToCreate.push_back(processedPreset);
        }
        dataManager.resolvePresetNameConflicts(presetsToCreate);

        return presetsToCreate;
    }

}