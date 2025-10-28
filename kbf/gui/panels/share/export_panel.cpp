#include <kbf/gui/panels/share/export_panel.hpp>

#include <kbf/gui/shared/alignment.hpp>
#include <kbf/gui/shared/drag_selectable.hpp>
#include <kbf/data/ids/font_symbols.hpp>
#include <kbf/data/ids/special_armour_ids.hpp>
#include <kbf/util/functional/invoke_callback.hpp>
#include <kbf/debug/debug_stack.hpp>
#include <kbf/gui/shared/tick_mark.hpp>
#include <kbf/util/io/noc_impl.hpp>
#include <kbf/util/font/default_font_sizes.hpp>

namespace kbf {

    ExportPanel::ExportPanel(
        const std::string& name,
        const std::string& strID,
        const bool archive,
        const KBFDataManager& dataManager,
        ImFont* wsSymbolFont,
        ImFont* wsArmourFont
    ) : iPanel(name, strID), 
        archive{ archive },
        dataManager{ dataManager },
        wsSymbolFont{ wsSymbolFont }, 
		wsArmourFont{ wsArmourFont }
    {
        initializeBuffers();
    }

    void ExportPanel::initializeBuffers() {
        std::strcpy(exportNameBuffer, exportName.c_str());
        std::strcpy(exportPathBuffer, exportPath.string().c_str());
        std::strcpy(filterBuffer, "");
    }

    bool ExportPanel::draw() {
        bool open = true;
        processFocus();
        processCallbacks();

        CImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, ImVec2(0.5f, 0.5f));

        constexpr ImVec2 minWindowSize = ImVec2(600, 450);

        if (isFirstDraw()) { CImGui::SetNextWindowSize(minWindowSize); }
        CImGui::SetNextWindowSizeConstraints(minWindowSize, ImVec2(FLT_MAX, FLT_MAX));
        CImGui::Begin(nameWithID.c_str(), &open, ImGuiWindowFlags_NoCollapse);

        float width = CImGui::GetWindowSize().x;

        const char* nameLabel = archive ? " Mod Name " : " Filename ";
        CImGui::InputText(" Export Name ", exportNameBuffer, IM_ARRAYSIZE(exportNameBuffer));
        exportName = std::string{ exportNameBuffer };

        // Export Path Widget
		CImGui::InputTextWithHint("##PathInput", "Export Path...", exportPathBuffer, IM_ARRAYSIZE(exportPathBuffer), ImGuiInputTextFlags_ReadOnly);
        CImGui::SetItemTooltip("Export path");
        CImGui::SameLine();
        if (exportPathDialogOpen) CImGui::BeginDisabled();
        if (CImGui::Button("Browse")) {
            std::thread([this]() {
                std::string filepath = getExportPathFileDialog();
                if (!filepath.empty()) {
                    postToMainThread([this, filepath]() {
                        exportPath = filepath;
						std::strcpy(exportPathBuffer, exportPath.string().c_str());
                    });
                }
                exportPathDialogOpen = false;
            }).detach();
        }
        CImGui::SetItemTooltip("Set Export path");
        if (exportPathDialogOpen) CImGui::EndDisabled();

        CImGui::Spacing();
        CImGui::Separator();
        CImGui::Spacing();

        if (CImGui::BeginTabBar("ExportTabs")) {
            if (CImGui::BeginTabItem("Preset Groups")) {
                drawPresetGroupList();
                CImGui::EndTabItem();
            }
            if (CImGui::BeginTabItem("Preset Bundles")) {
                drawPresetBundleList();
                CImGui::EndTabItem();
            }
            if (CImGui::BeginTabItem("Presets")) {
                drawPresetList();
                CImGui::EndTabItem();
            }
            if (CImGui::BeginTabItem("Player Override")) {
                drawPlayerOverrideList();
                CImGui::EndTabItem();
            }
            CImGui::EndTabBar();
        }

        CImGui::Spacing();
        CImGui::Spacing();

        static constexpr const char* kCancelLabel = "Cancel";
        const char* kCreateLabel = archive ? "Create Archive" : "Create .KBF";

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

        bool nameEmpty = exportName.empty();
        bool pathEmpty = exportPath.empty();
        bool pathNotExists = pathEmpty || !std::filesystem::exists(exportPath); // TODO Don't check this each frame
		bool noContent = selectedPresetGroups.size() == 0 && selectedPresets.size() == 0 && selectedPlayerOverrides.size() == 0;

        const bool disableCreateButton = nameEmpty || pathEmpty || pathNotExists || noContent;
        if (disableCreateButton) CImGui::BeginDisabled();
        if (CImGui::Button(kCreateLabel)) {
            KBFFileData data = getKbfFileData();
            addModArchiveTags(data);

			std::string filename = archive ? exportName + ".zip" : exportName + ".kbf";
			std::filesystem::path filePath = exportPath / filename;
			INVOKE_REQUIRED_CALLBACK(createCallback, filePath.string(), data);
        }
        if (disableCreateButton) CImGui::EndDisabled();
        if (nameEmpty) CImGui::SetItemTooltip("Please provide a name");
		else if (pathEmpty) CImGui::SetItemTooltip("Please provide an export path");
        else if (pathNotExists) CImGui::SetItemTooltip("Export path does not exist");
        else if (noContent) CImGui::SetItemTooltip("Please select some data to include in the export");

        CImGui::End();
        CImGui::PopStyleVar();

        return open;
    }

    void ExportPanel::drawPresetList() {
        std::string filterStr{ filterBuffer };

        CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
        constexpr char const* exportHintStr = "Click & Drag to select items to include in the export.";
        CImGui::Spacing();
        preAlignCellContentHorizontal(exportHintStr);
        CImGui::Text(exportHintStr);
        CImGui::Spacing();
        CImGui::PopStyleColor();

        // Fixed-height, scrollable region
        CImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.02f, 0.02f, 0.02f, 1.0f));
        CImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, childItemSpacing);
        CImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, childWindowPadding);

        const float height = -(2.0f * CImGui::GetFrameHeightWithSpacing() + 20.0f); // Leave space for the search bar
        CImGui::BeginChild("ItemSelectorGroupChild", ImVec2(0, height), true, ImGuiWindowFlags_HorizontalScrollbar);

        float contentRegionWidth = CImGui::GetContentRegionAvail().x;
        static bool isDragging = false;
        static bool dragStarted = false;
        static std::unordered_set<std::string> selectedDuringDrag{};

        ImVec2 dragStart, dragEnd;
        ImGuiIO* io = CImGui::GetIO();

        // Handle drag logic outside the list box
        if (CImGui::IsMouseClicked(0) && CImGui::IsWindowHovered()) {
            dragStart = io->MousePos;
            isDragging = true;
            dragStarted = true;
        }
        if (isDragging && CImGui::IsMouseReleased(0)) {
            isDragging = false;
            selectedDuringDrag.clear();
        }

		const std::vector<const Preset*> presets = dataManager.getPresets(filterStr, ArmourPieceFlagBits::APF_NONE, true);

        if (presets.size() == 0) {
            CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
            constexpr char const* noPresetStr = "\nNo Presets Found.";
            CImGui::Spacing();
            preAlignCellContentHorizontal(noPresetStr);
            CImGui::Text(noPresetStr);
            CImGui::PopStyleColor();
        }

        for (const Preset* preset : presets)
        {
            ImVec2 pos = CImGui::GetCursorScreenPos();

            bool selected = checkPresetSelected(preset);

            bool hovered = false;
            if (DragSelectable(("##Selectable_Preset_" + preset->uuid).c_str(), selected, selectableHeight, &hovered)) {
                if (selected) deselectPreset(preset);
                else          selectPreset(preset);
                selected = !selected;
                selectedDuringDrag.insert(preset->uuid);
            }

            bool alreadySelectedThisDrag = selectedDuringDrag.find(preset->uuid) != selectedDuringDrag.end();
            if (isDragging && hovered && !alreadySelectedThisDrag) {
                if (selected) deselectPreset(preset);
                else          selectPreset(preset);
                selected = !selected;
                selectedDuringDrag.insert(preset->uuid);
            }

            ImVec2 tickMarkPos;
            tickMarkPos.x = pos.x + listPaddingX + CImGui::GetStyle().ItemSpacing.x;
            tickMarkPos.y = pos.y + (selectableHeight) * 0.5f;

            if (selected) {
                CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
                ImTickMark(tickMarkPos, 1.2f, 2.0f);
                CImGui::PopStyleColor();
            }

            drawPresetSelector(preset, pos, selectableHeight);
        }

        CImGui::EndChild();
        CImGui::PopStyleVar(2);
        CImGui::PopStyleColor();

        CImGui::Spacing();
        if (CImGui::Button("Clear")) selectedPresets.clear();
        CImGui::SameLine();
        if (CImGui::Button("Select All")) {
            for (const Preset* preset : presets) selectPreset(preset);
        }
        CImGui::SameLine();
        CImGui::PushItemWidth(-1);
        CImGui::InputTextWithHint("##Search", "Search...", filterBuffer, IM_ARRAYSIZE(filterBuffer));
        CImGui::PopItemWidth();
    }

    void ExportPanel::drawPresetSelector(const Preset* preset, ImVec2 pos, float selectableHeight) const {
        const float contentRegionWidth = CImGui::GetContentRegionAvail().x;

        // Sex Mark
        std::string sexMarkSymbol = preset->female ? WS_FONT_FEMALE : WS_FONT_MALE;
        ImVec4 sexMarkerCol = preset->female ? ImVec4(0.76f, 0.50f, 0.24f, 1.0f) : ImVec4(0.50f, 0.70f, 0.33f, 1.0f);

        CImGui::PushFont(wsSymbolFont, FONT_SIZE_DEFAULT_WILDS_SYMBOLS);

        constexpr float sexMarkerSpacingAfter = 5.0f;
        constexpr float sexMarkerVerticalAlignOffset = 5.0f;
        ImVec2 sexMarkerSize = CImGui::CalcTextSize(sexMarkSymbol.c_str());
        ImVec2 sexMarkerPos;
        sexMarkerPos.x = pos.x + listPaddingX +  3.0f * CImGui::GetStyle().ItemSpacing.x;
        sexMarkerPos.y = pos.y + (selectableHeight - sexMarkerSize.y) * 0.5f + sexMarkerVerticalAlignOffset;
        CImGui::GetWindowDrawList()->AddText(sexMarkerPos, CImGui::GetColorU32(sexMarkerCol), sexMarkSymbol.c_str());

        CImGui::PopFont();

        CImGui::PushFont(dataManager.getRegularFontOverride(), FONT_SIZE_DEFAULT_MAIN);

        // Group name... floating because imgui has no vertical alignment STILL :(
        constexpr float playerNameSpacingAfter = 5.0f;
        ImVec2 playerNameSize = CImGui::CalcTextSize(preset->name.c_str());
        ImVec2 playerNamePos;
        playerNamePos.x = sexMarkerPos.x + sexMarkerSize.x + sexMarkerSpacingAfter;
        playerNamePos.y = pos.y + (selectableHeight - playerNameSize.y) * 0.5f;
        CImGui::GetWindowDrawList()->AddText(playerNamePos, CImGui::GetColorU32(ImGuiCol_Text), preset->name.c_str());

        std::string bundleStr = "(" + preset->bundle + ")";
        ImVec2 bundleStrSize = CImGui::CalcTextSize(bundleStr.c_str());
        ImVec2 bundleStrPos;
        bundleStrPos.x = playerNamePos.x + playerNameSize.x + playerNameSpacingAfter;
        bundleStrPos.y = pos.y + (selectableHeight - bundleStrSize.y) * 0.5f;
        CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
        CImGui::GetWindowDrawList()->AddText(bundleStrPos, CImGui::GetColorU32(ImGuiCol_Text), bundleStr.c_str());
        CImGui::PopStyleColor();

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
        CImGui::PushStyleColor(ImGuiCol_Text, preset->hasModifiers(ArmourPiece::AP_LEGS) ? armourPresentCol : armourMissingCol);
        CImGui::GetWindowDrawList()->AddText(legMarkPos, CImGui::GetColorU32(ImGuiCol_Text), WS_FONT_LEGS);
        CImGui::PopStyleColor();

        // Coil Mark
        ImVec2 coilMarkSize = CImGui::CalcTextSize(WS_FONT_COIL);
        ImVec2 coilMarkPos;
        coilMarkPos.x = legMarkPos.x - coilMarkSize.x - CImGui::GetStyle().ItemSpacing.x;
        coilMarkPos.y = pos.y + (selectableHeight - coilMarkSize.y) * 0.5f + armourVerticalAlignOffset;
        CImGui::PushStyleColor(ImGuiCol_Text, preset->hasModifiers(ArmourPiece::AP_COIL) ? armourPresentCol : armourMissingCol);
        CImGui::GetWindowDrawList()->AddText(coilMarkPos, CImGui::GetColorU32(ImGuiCol_Text), WS_FONT_COIL);
        CImGui::PopStyleColor();

        // Arms Mark
        ImVec2 armsMarkSize = CImGui::CalcTextSize(WS_FONT_ARMS);
        ImVec2 armsMarkPos;
        armsMarkPos.x = coilMarkPos.x - armsMarkSize.x - CImGui::GetStyle().ItemSpacing.x;
        armsMarkPos.y = pos.y + (selectableHeight - armsMarkSize.y) * 0.5f + armourVerticalAlignOffset;
        CImGui::PushStyleColor(ImGuiCol_Text, preset->hasModifiers(ArmourPiece::AP_ARMS) ? armourPresentCol : armourMissingCol);
        CImGui::GetWindowDrawList()->AddText(armsMarkPos, CImGui::GetColorU32(ImGuiCol_Text), WS_FONT_ARMS);
        CImGui::PopStyleColor();

        // Body Mark
        ImVec2 bodyMarkSize = CImGui::CalcTextSize(WS_FONT_BODY);
        ImVec2 bodyMarkPos;
        bodyMarkPos.x = armsMarkPos.x - bodyMarkSize.x - CImGui::GetStyle().ItemSpacing.x;
        bodyMarkPos.y = pos.y + (selectableHeight - bodyMarkSize.y) * 0.5f + armourVerticalAlignOffset;
        CImGui::PushStyleColor(ImGuiCol_Text, preset->hasModifiers(ArmourPiece::AP_BODY) ? armourPresentCol : armourMissingCol);
        CImGui::GetWindowDrawList()->AddText(bodyMarkPos, CImGui::GetColorU32(ImGuiCol_Text), WS_FONT_BODY);
        CImGui::PopStyleColor();

        // Helm Mark
        ImVec2 helmMarkSize = CImGui::CalcTextSize(WS_FONT_HELM);
        ImVec2 helmMarkPos;
        helmMarkPos.x = bodyMarkPos.x - helmMarkSize.x - CImGui::GetStyle().ItemSpacing.x;
        helmMarkPos.y = pos.y + (selectableHeight - helmMarkSize.y) * 0.5f + armourVerticalAlignOffset;
        CImGui::PushStyleColor(ImGuiCol_Text, preset->hasModifiers(ArmourPiece::AP_HELM) ? armourPresentCol : armourMissingCol);
        CImGui::GetWindowDrawList()->AddText(helmMarkPos, CImGui::GetColorU32(ImGuiCol_Text), WS_FONT_HELM);
        CImGui::PopStyleColor();

        // Set Mark
        ImVec2 setMarkSize = CImGui::CalcTextSize(WS_FONT_SET);
        ImVec2 setMarkPos;
        setMarkPos.x = helmMarkPos.x - setMarkSize.x - CImGui::GetStyle().ItemSpacing.x;
        setMarkPos.y = pos.y + (selectableHeight - setMarkSize.y) * 0.5f + armourVerticalAlignOffset;
        CImGui::PushStyleColor(ImGuiCol_Text, preset->hasModifiers(ArmourPiece::AP_SET) ? armourPresentCol : armourMissingCol);
        CImGui::GetWindowDrawList()->AddText(setMarkPos, CImGui::GetColorU32(ImGuiCol_Text), WS_FONT_SET);
        CImGui::PopStyleColor();

        // Armour Sex Mark
        std::string armourSexMarkSymbol = preset->armour.female ? WS_FONT_FEMALE : WS_FONT_MALE;
        ImVec4 armourSexMarkerCol = preset->armour.female ? ImVec4(0.76f, 0.50f, 0.24f, 1.0f) : ImVec4(0.50f, 0.70f, 0.33f, 1.0f);

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
        ImVec2 armourNameSize = CImGui::CalcTextSize(preset->armour.name.c_str());
        ImVec2 armourNamePos;
        armourNamePos.x = armourSexMarkerPos.x - armourNameSize.x - CImGui::GetStyle().ItemSpacing.x;
        armourNamePos.y = pos.y + (selectableHeight - armourNameSize.y) * 0.5f;

        ImVec4 armourNameCol = preset->armour.name == ANY_ARMOUR_ID ? ImVec4(0.365f, 0.678f, 0.886f, 0.8f) : ImVec4(1.0f, 1.0f, 1.0f, 0.5f);
        CImGui::PushStyleColor(ImGuiCol_Text, armourNameCol);
        CImGui::GetWindowDrawList()->AddText(armourNamePos, CImGui::GetColorU32(ImGuiCol_Text), preset->armour.name.c_str());
        CImGui::PopStyleColor();

        CImGui::PopFont();
    }

    bool ExportPanel::checkPresetSelected(const Preset* preset) const {
        return selectedPresets.find(preset->uuid) != selectedPresets.end();
	}

    void ExportPanel::selectPreset(const Preset* preset) {
        selectedPresets.insert(preset->uuid);
	}

    void ExportPanel::deselectPreset(const Preset* preset) {
        selectedPresets.erase(preset->uuid);
    }

    void ExportPanel::drawPresetGroupList() {
        std::string filterStr{ filterBuffer };

        CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
        constexpr char const* exportHintStr = "Click & Drag to select items to include in the export.";
        CImGui::Spacing();
        preAlignCellContentHorizontal(exportHintStr);
        CImGui::Text(exportHintStr);
        CImGui::Spacing();
        CImGui::PopStyleColor();

        // Fixed-height, scrollable region
        CImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.02f, 0.02f, 0.02f, 1.0f));
        CImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, childItemSpacing);
        CImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, childWindowPadding);

        const float height = -(2.0f * CImGui::GetFrameHeightWithSpacing() + 20.0f); // Leave space for the search bar
        CImGui::BeginChild("ItemSelectorGroupChild", ImVec2(0, height), true, ImGuiWindowFlags_HorizontalScrollbar);

        float contentRegionWidth = CImGui::GetContentRegionAvail().x;
        static bool isDragging = false;
        static bool dragStarted = false;
        static std::unordered_set<std::string> selectedDuringDrag{};

        ImVec2 dragStart, dragEnd;
        ImGuiIO* io = CImGui::GetIO();

        // Handle drag logic outside the list box
        if (CImGui::IsMouseClicked(0) && CImGui::IsWindowHovered()) {
            dragStart = io->MousePos;
            isDragging = true;
            dragStarted = true;
        }
        if (isDragging && CImGui::IsMouseReleased(0)) {
            isDragging = false;
            selectedDuringDrag.clear();
        }

        std::vector<const PresetGroup*> presetGroups = dataManager.getPresetGroups(filterBuffer);

        if (presetGroups.size() == 0) {
            CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
            constexpr char const* noPresetStr = "\nNo Preset Groups Found.";
            CImGui::Spacing();
            preAlignCellContentHorizontal(noPresetStr);
            CImGui::Text(noPresetStr);
            CImGui::PopStyleColor();
        }

        for (const PresetGroup* presetGroup : presetGroups)
        {
            ImVec2 pos = CImGui::GetCursorScreenPos();

            bool selected = checkPresetGroupSelected(presetGroup);

            bool hovered = false;
            if (DragSelectable(("##Selectable_Preset_" + presetGroup->uuid).c_str(), selected, selectableHeight, &hovered)) {
                if (selected) deselectPresetGroup(presetGroup);
				else          selectPresetGroup(presetGroup);
                selected = !selected;
                selectedDuringDrag.insert(presetGroup->uuid);
            }

            bool alreadySelectedThisDrag = selectedDuringDrag.find(presetGroup->uuid) != selectedDuringDrag.end();
            if (isDragging && hovered && !alreadySelectedThisDrag) {
                if (selected) deselectPresetGroup(presetGroup);
                else          selectPresetGroup(presetGroup);
                selected = !selected;
                selectedDuringDrag.insert(presetGroup->uuid);
            }

            ImVec2 tickMarkPos;
            tickMarkPos.x = pos.x + listPaddingX + CImGui::GetStyle().ItemSpacing.x;
            tickMarkPos.y = pos.y + (selectableHeight) * 0.5f;

            if (selected) {
                CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
                ImTickMark(tickMarkPos, 1.2f, 2.0f);
                CImGui::PopStyleColor();
            }

            drawPresetGroupSelector(presetGroup, pos, selectableHeight);
        }

        CImGui::EndChild();
        CImGui::PopStyleVar(2);
        CImGui::PopStyleColor();

        CImGui::Spacing();
        if (CImGui::Button("Clear")) selectedPresetGroups.clear();
        CImGui::SameLine();
        if (CImGui::Button("Select All")) {
            for (const PresetGroup* presetGroup : presetGroups) selectPresetGroup(presetGroup);
        }
        CImGui::SameLine();
        CImGui::PushItemWidth(-1);
        CImGui::InputTextWithHint("##Search", "Search...", filterBuffer, IM_ARRAYSIZE(filterBuffer));
        CImGui::PopItemWidth();
    }

    void ExportPanel::drawPresetGroupSelector(const PresetGroup* presetGroup, ImVec2 pos, float selectableHeight) const {
        const float contentRegionWidth = CImGui::GetContentRegionAvail().x;

        // Sex Mark
        std::string sexMarkSymbol = presetGroup->female ? WS_FONT_FEMALE : WS_FONT_MALE;
        ImVec4 sexMarkerCol = presetGroup->female ? ImVec4(0.76f, 0.50f, 0.24f, 1.0f) : ImVec4(0.50f, 0.70f, 0.33f, 1.0f);

        CImGui::PushFont(wsSymbolFont, FONT_SIZE_DEFAULT_WILDS_SYMBOLS);

        constexpr float sexMarkerSpacingAfter = 5.0f;
        constexpr float sexMarkerVerticalAlignOffset = 5.0f;
        ImVec2 sexMarkerSize = CImGui::CalcTextSize(sexMarkSymbol.c_str());
        ImVec2 sexMarkerPos;
        sexMarkerPos.x = pos.x + listPaddingX + CImGui::GetStyle().ItemSpacing.x * 3.0f;
        sexMarkerPos.y = pos.y + (selectableHeight - sexMarkerSize.y) * 0.5f + sexMarkerVerticalAlignOffset;
        CImGui::GetWindowDrawList()->AddText(sexMarkerPos, CImGui::GetColorU32(sexMarkerCol), sexMarkSymbol.c_str());

        CImGui::PopFont();

        // Group name
        ImVec2 presetGroupNameSize = CImGui::CalcTextSize(presetGroup->name.c_str());
        ImVec2 presetGroupNamePos;
        presetGroupNamePos.x = sexMarkerPos.x + sexMarkerSize.x + sexMarkerSpacingAfter;
        presetGroupNamePos.y = pos.y + (selectableHeight - presetGroupNameSize.y) * 0.5f;
        CImGui::GetWindowDrawList()->AddText(presetGroupNamePos, CImGui::GetColorU32(ImGuiCol_Text), presetGroup->name.c_str());

        std::string presetCountStr;
        if (presetGroup->size() == 0) presetCountStr = "Empty";
        else if (presetGroup->size() == 1) presetCountStr = "1 Preset";
        else presetCountStr = std::to_string(presetGroup->size()) + " Presets";

        ImVec2 rightTextSize = CImGui::CalcTextSize(presetCountStr.c_str());
        float hunterIdCursorPosX = CImGui::GetCursorScreenPos().x + contentRegionWidth - rightTextSize.x - CImGui::GetStyle().ItemSpacing.x - listPaddingX;
        float hunterIdCursorPosY = CImGui::GetItemRectMin().y + (selectableHeight - rightTextSize.y) * 0.5f;;  // Same Y as the selectable item, plus vertical alignment

        CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
        CImGui::GetWindowDrawList()->AddText(ImVec2(hunterIdCursorPosX, hunterIdCursorPosY), CImGui::GetColorU32(ImGuiCol_Text), presetCountStr.c_str());
        CImGui::PopStyleColor();
    }

    bool ExportPanel::checkPresetGroupSelected(const PresetGroup* presetGroup) const {
        bool selected = selectedPresetGroups.find(presetGroup->uuid) != selectedPresetGroups.end();
        if (selected) {
            for (const auto& [_, id] : presetGroup->bodyPresets) {
                if (selectedPresets.find(id) == selectedPresets.end()) {
                    selected = false;
                    break;
                }
            }
            for (const auto& [_, id] : presetGroup->legsPresets) {
                if (selectedPresets.find(id) == selectedPresets.end()) {
                    selected = false;
                    break;
                }
            }
        }

        return selected;
    }

    void ExportPanel::selectPresetGroup(const PresetGroup* presetGroup) {
        selectedPresetGroups.insert(presetGroup->uuid);
        std::ranges::for_each(presetGroup->bodyPresets, [&](auto& entry) { selectedPresets.insert(entry.second); });
        std::ranges::for_each(presetGroup->legsPresets, [&](auto& entry) { selectedPresets.insert(entry.second); });
    }

    void ExportPanel::deselectPresetGroup(const PresetGroup* presetGroup) {
        selectedPresetGroups.erase(presetGroup->uuid);
    }

    void ExportPanel::drawPresetBundleList() {
        std::string filterStr{ filterBuffer };

        CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
        constexpr char const* exportHintStr = "Click & Drag to select items to include in the export.";
        CImGui::Spacing();
        preAlignCellContentHorizontal(exportHintStr);
        CImGui::Text(exportHintStr);
        CImGui::Spacing();
        CImGui::PopStyleColor();

        // Fixed-height, scrollable region
        CImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.02f, 0.02f, 0.02f, 1.0f));
        CImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, childItemSpacing);
        CImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, childWindowPadding);

        const float height = -(2.0f * CImGui::GetFrameHeightWithSpacing() + 20.0f); // Leave space for the search bar
        CImGui::BeginChild("ItemSelectorGroupChild", ImVec2(0, height), true, ImGuiWindowFlags_HorizontalScrollbar);

        float contentRegionWidth = CImGui::GetContentRegionAvail().x;
        static bool isDragging = false;
        static bool dragStarted = false;
        static std::unordered_set<std::string> selectedDuringDrag{};

        ImVec2 dragStart, dragEnd;
        ImGuiIO* io = CImGui::GetIO();

        // Handle drag logic outside the list box
        if (CImGui::IsMouseClicked(0) && CImGui::IsWindowHovered()) {
            dragStart = io->MousePos;
            isDragging = true;
            dragStarted = true;
        }
        if (isDragging && CImGui::IsMouseReleased(0)) {
            isDragging = false;
            selectedDuringDrag.clear();
        }

		std::vector<std::pair<std::string, size_t>> bundles = dataManager.getPresetBundlesWithCounts(filterBuffer);


        if (bundles.size() == 0) {
            CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
            constexpr char const* noPresetStr = "\nNo Bundles Found.";
            CImGui::Spacing();
            preAlignCellContentHorizontal(noPresetStr);
            CImGui::Text(noPresetStr);
            CImGui::PopStyleColor();
        }

        for (const auto& bundleInfo : bundles)
        {
			std::vector<std::string> bundleIds = dataManager.getPresetsInBundle(bundleInfo.first);

			std::string bundleName = bundleInfo.first;
            size_t bundleCount     = bundleInfo.second;

            ImVec2 pos = CImGui::GetCursorScreenPos();

			bool selected = checkPresetBundleSelected(bundleIds);

            bool hovered = false;
            if (DragSelectable(("##Selectable_Preset_" + bundleName).c_str(), selected, selectableHeight, &hovered)) {
                if (selected) deselectPresetBundle(bundleIds);
                else          selectPresetBundle(bundleIds);
                selected = !selected;
                selectedDuringDrag.insert(bundleName);
            }

            bool alreadySelectedThisDrag = selectedDuringDrag.find(bundleName) != selectedDuringDrag.end();
            if (isDragging && hovered && !alreadySelectedThisDrag) {
                if (selected) deselectPresetBundle(bundleIds);
                else          selectPresetBundle(bundleIds);
                selected = !selected;
                selectedDuringDrag.insert(bundleName);
            }

            ImVec2 tickMarkPos;
            tickMarkPos.x = pos.x + listPaddingX + CImGui::GetStyle().ItemSpacing.x;
            tickMarkPos.y = pos.y + (selectableHeight) * 0.5f;

            if (selected) {
                CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
                ImTickMark(tickMarkPos, 1.2f, 2.0f);
                CImGui::PopStyleColor();
            }

			drawPresetBundleSelector(bundleName, pos, selectableHeight);
        }

        CImGui::EndChild();
        CImGui::PopStyleVar(2);
        CImGui::PopStyleColor();

        CImGui::Spacing();
        if (CImGui::Button("Clear")) selectedPresets.clear();
        CImGui::SameLine();
        if (CImGui::Button("Select All")) {
            for (const auto& bundleInfo : bundles) {
                std::vector<std::string> bundleIds = dataManager.getPresetsInBundle(bundleInfo.first);
                std::ranges::for_each(bundleIds, [&](auto& id) { selectedPresets.insert(id); });
            }
        }
        CImGui::SameLine();
        CImGui::PushItemWidth(-1);
        CImGui::InputTextWithHint("##Search", "Search...", filterBuffer, IM_ARRAYSIZE(filterBuffer));
        CImGui::PopItemWidth();
    }

    void ExportPanel::drawPresetBundleSelector(const std::string& bundleName, ImVec2 pos, float selectableHeight) const {
        size_t count = dataManager.getPresetBundleCount(bundleName);

        // Group name... floating because imgui has no vertical alignment STILL :(
        ImVec2 labelSize = CImGui::CalcTextSize(bundleName.c_str());
        ImVec2 labelPos;
        labelPos.x = pos.x + listPaddingX + CImGui::GetStyle().ItemSpacing.x * 3.0f;
        labelPos.y = pos.y + (selectableHeight - labelSize.y) * 0.5f;

        CImGui::GetWindowDrawList()->AddText(labelPos, CImGui::GetColorU32(ImGuiCol_Text), bundleName.c_str());

        // Similarly for preset count
        const size_t nPresets = count;
        std::string rightText = std::to_string(nPresets);
        if (nPresets == 0) rightText = "Empty";
        else if (nPresets == 1) rightText += " Preset";
        else rightText += " Presets";

        ImVec2 rightTextSize = CImGui::CalcTextSize(rightText.c_str());
        float contentRegionWidth = CImGui::GetContentRegionAvail().x;
        float cursorPosX = CImGui::GetCursorScreenPos().x + contentRegionWidth - rightTextSize.x - CImGui::GetStyle().ItemSpacing.x - listPaddingX;

        CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
        CImGui::GetWindowDrawList()->AddText(ImVec2(cursorPosX, labelPos.y), CImGui::GetColorU32(ImGuiCol_Text), rightText.c_str());
        CImGui::PopStyleColor();
    }

    bool ExportPanel::checkPresetBundleSelected(const std::vector<std::string>& bundleIds) const {
        bool selected = true;
        for (const auto& id : bundleIds) {
            if (selectedPresets.find(id) == selectedPresets.end()) {
                selected = false;
                break;
            }
        }
        return selected;
	}

    void ExportPanel::selectPresetBundle(const std::vector<std::string>& bundleIds) {
        std::ranges::for_each(bundleIds, [&](auto& id) { selectedPresets.insert(id); });
    }

    void ExportPanel::deselectPresetBundle(const std::vector<std::string>& bundleIds) {
        std::ranges::for_each(bundleIds, [&](auto& id) { selectedPresets.erase(id); });
    }

    void ExportPanel::drawPlayerOverrideList() {
        std::string filterStr{ filterBuffer };

        CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
        constexpr char const* exportHintStr = "Click & Drag to select items to include in the export.";
        CImGui::Spacing();
        preAlignCellContentHorizontal(exportHintStr);
        CImGui::Text(exportHintStr);
        CImGui::Spacing();
        CImGui::PopStyleColor();

        // Fixed-height, scrollable region
        CImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.02f, 0.02f, 0.02f, 1.0f));
        CImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, childItemSpacing);
        CImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, childWindowPadding);

        const float height = -(2.0f * CImGui::GetFrameHeightWithSpacing() + 20.0f); // Leave space for the search bar
        CImGui::BeginChild("ItemSelectorGroupChild", ImVec2(0, height), true, ImGuiWindowFlags_HorizontalScrollbar);

        float contentRegionWidth = CImGui::GetContentRegionAvail().x;
        static bool isDragging = false;
        static bool dragStarted = false;
        static std::unordered_set<PlayerData> selectedDuringDrag{};

        ImVec2 dragStart, dragEnd;
        ImGuiIO* io = CImGui::GetIO();

        // Handle drag logic outside the list box
        if (CImGui::IsMouseClicked(0) && CImGui::IsWindowHovered()) {
            dragStart = io->MousePos;
            isDragging = true;
            dragStarted = true;
        }
        if (isDragging && CImGui::IsMouseReleased(0)) {
            isDragging = false;
            selectedDuringDrag.clear();
        }

        std::vector<const PlayerOverride*> playerOverrides = dataManager.getPlayerOverrides(filterBuffer, true);

        if (playerOverrides.size() == 0) {
            CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
            constexpr char const* noPresetStr = "\nNo Overrides Found.";
            preAlignCellContentHorizontal(noPresetStr);
            CImGui::Text(noPresetStr);
            CImGui::PopStyleColor();
        }

        for (const PlayerOverride* override : playerOverrides)
        {
            ImVec2 pos = CImGui::GetCursorScreenPos();

            bool selected = checkPlayerOverrideSelected(override);

            // Need to add the preset if added, and just remove override if not.
            bool hovered = false;
            if (DragSelectable(("##Selectable_Preset_" + override->player.string()).c_str(), selected, selectableHeight, &hovered)) {
                if (selected) deselectPlayerOverride(override);
                else          selectPlayerOverride(override);
                selected = !selected;
                selectedDuringDrag.insert(override->player);
            }

            bool alreadySelectedThisDrag = selectedDuringDrag.find(override->player) != selectedDuringDrag.end();
            if (isDragging && hovered && !alreadySelectedThisDrag) {
                const PresetGroup* presetGroup = dataManager.getPresetGroupByUUID(override->presetGroup);
                if (selected) deselectPlayerOverride(override);
				else          selectPlayerOverride(override);
                selected = !selected;
                selectedDuringDrag.insert(override->player);
            }

            ImVec2 tickMarkPos;
            tickMarkPos.x = pos.x + listPaddingX + CImGui::GetStyle().ItemSpacing.x;
            tickMarkPos.y = pos.y + (selectableHeight) * 0.5f;

            if (selected) {
                CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
                ImTickMark(tickMarkPos, 1.2f, 2.0f);
                CImGui::PopStyleColor();
            }

            drawPlayerOverrideSelector(override, pos, selectableHeight);
        }

        CImGui::EndChild();
        CImGui::PopStyleVar(2);
        CImGui::PopStyleColor();

        CImGui::Spacing();
        if (CImGui::Button("Clear")) selectedPlayerOverrides.clear();
        CImGui::SameLine();
        if (CImGui::Button("Select All")) {
            for (const PlayerOverride* override : playerOverrides) selectPlayerOverride(override);
        }
        CImGui::SameLine();
        CImGui::PushItemWidth(-1);
        CImGui::InputTextWithHint("##Search", "Search...", filterBuffer, IM_ARRAYSIZE(filterBuffer));
        CImGui::PopItemWidth();
    }

    void ExportPanel::drawPlayerOverrideSelector(const PlayerOverride* override, ImVec2 pos, float selectableHeight) const {
        // Sex Mark
        std::string sexMarkSymbol = override->player.female ? WS_FONT_FEMALE : WS_FONT_MALE;
        ImVec4 sexMarkerCol = override->player.female ? ImVec4(0.76f, 0.50f, 0.24f, 1.0f) : ImVec4(0.50f, 0.70f, 0.33f, 1.0f);

        CImGui::PushFont(wsSymbolFont, FONT_SIZE_DEFAULT_WILDS_SYMBOLS);

        constexpr float sexMarkerSpacingAfter = 5.0f;
        constexpr float sexMarkerVerticalAlignOffset = 5.0f;
        ImVec2 sexMarkerSize = CImGui::CalcTextSize(sexMarkSymbol.c_str());
        ImVec2 sexMarkerPos;
        sexMarkerPos.x = pos.x + listPaddingX + 3.0f * CImGui::GetStyle().ItemSpacing.x;
        sexMarkerPos.y = pos.y + (selectableHeight - sexMarkerSize.y) * 0.5f + sexMarkerVerticalAlignOffset;
        CImGui::GetWindowDrawList()->AddText(sexMarkerPos, CImGui::GetColorU32(sexMarkerCol), sexMarkSymbol.c_str());

        CImGui::PopFont();

        // Player name
        constexpr float playerNameSpacingAfter = 5.0f;
        ImVec2 playerNameSize = CImGui::CalcTextSize(override->player.name.c_str());
        ImVec2 playerNamePos;
        playerNamePos.x = sexMarkerPos.x + sexMarkerSize.x + sexMarkerSpacingAfter;
        playerNamePos.y = pos.y + (selectableHeight - playerNameSize.y) * 0.5f;
        CImGui::GetWindowDrawList()->AddText(playerNamePos, CImGui::GetColorU32(ImGuiCol_Text), override->player.name.c_str());

        // Preset Group
        const PresetGroup* activeGroup = dataManager.getPresetGroupByUUID(override->presetGroup);

        // Sex Mark
        bool female = false;
        float presetGroupSexMarkSpacing = CImGui::GetStyle().ItemSpacing.x;
        float presetGroupSexMarkSpacingBefore = 0.0f;
        float endPos = CImGui::GetCursorScreenPos().x + CImGui::GetContentRegionAvail().x;
        if (activeGroup) {
            bool female = activeGroup->female;
            presetGroupSexMarkSpacing = 15.0f;
            presetGroupSexMarkSpacingBefore = 10.0f;

            std::string presetGroupSexMarkSymbol = female ? WS_FONT_FEMALE : WS_FONT_MALE;
            ImVec4 presetGroupSexMarkerCol = female ? ImVec4(0.76f, 0.50f, 0.24f, 1.0f) : ImVec4(0.50f, 0.70f, 0.33f, 1.0f);

            ImVec2 presetGroupSexMarkerSize = CImGui::CalcTextSize(presetGroupSexMarkSymbol.c_str());
            ImVec2 presetGroupSexMarkerPos;
            presetGroupSexMarkerPos.x = endPos - presetGroupSexMarkSpacingBefore - CImGui::GetStyle().ItemSpacing.x - listPaddingX;
            presetGroupSexMarkerPos.y = pos.y + (selectableHeight - presetGroupSexMarkerSize.y) * 0.5f;

            CImGui::PushFont(wsSymbolFont, FONT_SIZE_DEFAULT_WILDS_SYMBOLS);
            CImGui::GetWindowDrawList()->AddText(presetGroupSexMarkerPos, CImGui::GetColorU32(presetGroupSexMarkerCol), presetGroupSexMarkSymbol.c_str());
            CImGui::PopFont();
        }

        // Group Name
        std::string presetGroupName = override->presetGroup.empty() || activeGroup == nullptr ? "Default" : activeGroup->name;
        ImVec2 currentGroupStrSize = CImGui::CalcTextSize(presetGroupName.c_str());
        ImVec2 currentGroupStrPos;
        currentGroupStrPos.x = endPos - (currentGroupStrSize.x + presetGroupSexMarkSpacing + presetGroupSexMarkSpacingBefore + listPaddingX);
        currentGroupStrPos.y = pos.y + (selectableHeight - currentGroupStrSize.y) * 0.5f;

        ImVec4 presetGroupCol = activeGroup == nullptr ? ImVec4(0.365f, 0.678f, 0.886f, 0.8f) : ImVec4(1.0f, 1.0f, 1.0f, 0.5f);
        CImGui::PushStyleColor(ImGuiCol_Text, presetGroupCol);
        CImGui::GetWindowDrawList()->AddText(currentGroupStrPos, CImGui::GetColorU32(ImGuiCol_Text), presetGroupName.c_str());
        CImGui::PopStyleColor();

        // Hunter ID
        std::string hunterIdStr = "(" + override->player.hunterId + ")";
        ImVec2 hunterIdStrSize = CImGui::CalcTextSize(hunterIdStr.c_str());
        ImVec2 hunterIdStrPos;
        hunterIdStrPos.x = playerNamePos.x + playerNameSize.x + playerNameSpacingAfter;
        hunterIdStrPos.y = pos.y + (selectableHeight - hunterIdStrSize.y) * 0.5f;
        CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
        CImGui::GetWindowDrawList()->AddText(hunterIdStrPos, CImGui::GetColorU32(ImGuiCol_Text), hunterIdStr.c_str());
        CImGui::PopStyleColor();
    }

    bool ExportPanel::checkPlayerOverrideSelected(const PlayerOverride* override) const {
        bool selected = selectedPlayerOverrides.find(override->player) != selectedPlayerOverrides.end();
        if (selected && !override->presetGroup.empty() && selectedPresetGroups.find(override->presetGroup) == selectedPresetGroups.end()) {
            selected = false;
        }

        return selected;
    }

    void ExportPanel::selectPlayerOverride(const PlayerOverride* override) {
        selectedPlayerOverrides.insert(override->player);
        if (!override->presetGroup.empty()) {
            const PresetGroup* presetGroup = dataManager.getPresetGroupByUUID(override->presetGroup);
            selectedPresetGroups.insert(presetGroup->uuid);
            std::ranges::for_each(presetGroup->bodyPresets, [&](auto& entry) { selectedPresets.insert(entry.second); });
            std::ranges::for_each(presetGroup->legsPresets, [&](auto& entry) { selectedPresets.insert(entry.second); });
        }
	}

    void ExportPanel::deselectPlayerOverride(const PlayerOverride* override) {
        selectedPlayerOverrides.erase(override->player);
	}

    KBFFileData ExportPanel::getKbfFileData() const {
        KBFFileData kbfData;
        for (const auto& presetGroupId : selectedPresetGroups) {
            const PresetGroup* presetGroup = dataManager.getPresetGroupByUUID(presetGroupId);
            if (presetGroup) kbfData.presetGroups.push_back(*presetGroup);
        }
        for (const auto& presetId : selectedPresets) {
            const Preset* preset = dataManager.getPresetByUUID(presetId);
            if (preset) kbfData.presets.push_back(*preset);
		}
        for (const auto& playerData : selectedPlayerOverrides) {
            const PlayerOverride* override = dataManager.getPlayerOverride(playerData);
            if (override) kbfData.playerOverrides.push_back(*override);
        }
		return kbfData;
    }

    void ExportPanel::addModArchiveTags(KBFFileData& data) const {
        if (!archive) return;

        data.metadata.MOD_ARCHIVE = exportName;
        for (PresetGroup& presetGroup : data.presetGroups)          presetGroup.metadata.MOD_ARCHIVE = exportName;
        for (Preset& preset : data.presets)                         preset.metadata.MOD_ARCHIVE = exportName;
		for (PlayerOverride& playerOverride : data.playerOverrides) playerOverride.metadata.MOD_ARCHIVE = exportName;
    }

    std::string ExportPanel::getExportPathFileDialog() {
        exportPathDialogOpen = true;
        std::string pth = noc_browse_folder_open(exportPath.string());

        return pth;
    }

    void ExportPanel::postToMainThread(std::function<void()> func) {
        std::lock_guard<std::mutex> lock(callbackMutex);
        callbackQueue.push(std::move(func));
    }

    void ExportPanel::processCallbacks() {
        std::lock_guard<std::mutex> lock(callbackMutex);
        while (!callbackQueue.empty()) {
            auto func = std::move(callbackQueue.front());
            callbackQueue.pop();
            func();
        }
    }

}