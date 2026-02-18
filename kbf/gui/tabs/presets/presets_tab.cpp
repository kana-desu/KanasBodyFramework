#include <kbf/gui/tabs/presets/presets_tab.hpp>

#include <kbf/gui/shared/styling_consts.hpp>
#include <kbf/gui/shared/alignment.hpp>
#include <kbf/data/ids/font_symbols.hpp>
#include <kbf/data/ids/special_armour_ids.hpp>
#include <kbf/util/string/to_lower.hpp>
#include <kbf/util/functional/invoke_callback.hpp>
#include <kbf/util/font/default_font_sizes.hpp>

#include <format>

namespace kbf {

	void PresetsTab::draw() {
        CImGui::PushStyleVar(ImGuiStyleVar_CellPadding, LIST_PADDING);

        const ImVec2 buttonSize = ImVec2(CImGui::GetContentRegionAvail().x, 0.0f);
        if (CImGui::Button("Create Preset", buttonSize)) {
            openCreatePresetPanel();
        }

        if (CImGui::Button("Combine Presets", buttonSize)) {
            openCombinePresetsPanel();
        }

        bool canImportFBSpresets = dataManager.fbsDirectoryFound() && !importFBSPresetsPanel.isVisible();
        if (!canImportFBSpresets) CImGui::BeginDisabled();
        if (CImGui::Button("Import FBS Presets", buttonSize)) {
			openImportFBSPresetsPanel();
        }
		if (!canImportFBSpresets) CImGui::EndDisabled();
        if (!canImportFBSpresets) CImGui::SetItemTooltip("FBS Presets directory not found.");
        CImGui::Spacing();

        if (CImGui::BeginTabBar("PresetTabs")) {
            if (CImGui::BeginTabItem("Bundles")) {
                CImGui::Spacing();
                drawBundleTab();
                CImGui::EndTabItem();
            }
            if (CImGui::IsItemClicked()) { 
                closePopouts(); 
                inBundlesTab = true;
            }

            if (CImGui::BeginTabItem("All Presets")) {
                CImGui::Spacing();
                drawPresetList();
                CImGui::EndTabItem();
            }
            if (CImGui::IsItemClicked()) { 
                closePopouts(); 
                inBundlesTab = false; 
            }

            CImGui::EndTabBar();
        }

        CImGui::PopStyleVar();
	}

	void PresetsTab::drawPopouts() {
        editPresetPanel.draw();
        createPresetPanel.draw();
		combinePresetsPanel.draw();
        importFBSPresetsPanel.draw();
		warnDeleteBundlePanel.draw();
    };

    void PresetsTab::closePopouts() {
        editPresetPanel.close();
        createPresetPanel.close();
        combinePresetsPanel.close();
        importFBSPresetsPanel.close();
		warnDeleteBundlePanel.close();
	}

    void PresetsTab::drawBundleTab() {
        if (bundleViewed.empty()) {
            drawBundleList();
        }
        else {
            float availableWidth = CImGui::GetContentRegionAvail().x;

			bool needsDisable = warnDeleteBundlePanel.isVisible();
            if (needsDisable) CImGui::BeginDisabled();

            if (CImGui::ArrowButton("##bundleBackButton", ImGuiDir_Left)) {
                bundleViewed = "";
            }
            CImGui::SameLine();

            CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.75f));
            CImGui::Text(std::format("Viewing Bundle \"{}\"", bundleViewed).c_str());
            CImGui::PopStyleColor();

            // Delete Button
            CImGui::SameLine();

            static constexpr const char* kDeleteLabel = "Delete";

            float deleteButtonWidth = CImGui::CalcTextSize(kDeleteLabel).x + CImGui::GetStyle().FramePadding.x * 2.0f;
            float totalWidth = deleteButtonWidth;

            float deleteButtonPos = availableWidth - deleteButtonWidth;
            CImGui::SetCursorPosX(deleteButtonPos);
            CImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f)); // Muted red
            CImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
            CImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.1f, 0.1f, 1.0f));

            if (CImGui::Button(kDeleteLabel)) {
                openWarnDeleteBundlePanel(bundleViewed);
            }

            CImGui::PopStyleColor(3);

            CImGui::Spacing();
            CImGui::Separator();
            CImGui::Spacing();

            drawPresetList(bundleViewed);

            if (needsDisable) CImGui::EndDisabled();
        }
    }

    void PresetsTab::drawBundleList() {
        static bool sortDirAscending;
        static enum class SortCol {
            NONE,
            NAME
        } sortCol;

        std::vector<const Preset*> presets = dataManager.getPresets("");

        // Sort
        switch (sortCol)
        {
        case SortCol::NAME:
            std::sort(presets.begin(), presets.end(), [&](const Preset* a, const Preset* b) {
                std::string lowa = toLower(a->bundle); std::string lowb = toLower(b->bundle);
                return sortDirAscending ? lowa < lowb : lowa > lowb;
                });
        }

        if (presets.size() == 0) {
            CImGui::Spacing();
            CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
            constexpr char const* noPresetStr = "No Existing Bundles / Presets";
            preAlignCellContentHorizontal(noPresetStr);
            CImGui::Text(noPresetStr);
            CImGui::PopStyleColor();
        }
        else {
            constexpr ImGuiTableFlags bundleTableFlags =
                ImGuiTableFlags_BordersInnerH
                | ImGuiTableFlags_BordersInnerV
                | ImGuiTableFlags_PadOuterX
                | ImGuiTableFlags_Sortable
                | ImGuiTableFlags_ScrollY;
            CImGui::BeginTable("#PresetTab_BundleList", 1, bundleTableFlags);

            constexpr ImGuiTableColumnFlags stretchSortFlags =
                ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthStretch;

            CImGui::TableSetupColumn("Bundle", stretchSortFlags, 0.0f);
            CImGui::TableSetupScrollFreeze(0, 1);
            CImGui::TableHeadersRow();

            // Sorting for preset name
            if (ImGuiTableSortSpecs* sort_specs = CImGui::TableGetSortSpecs()) {
                if (sort_specs->SpecsDirty && sort_specs->SpecsCount > 0) {
                    const ImGuiTableColumnSortSpecs& sort_spec = sort_specs->Specs[0];
                    sortDirAscending = sort_spec.SortDirection == ImGuiSortDirection_Ascending;

                    switch (sort_spec.ColumnIndex)
                    {
                    case 0:  sortCol = SortCol::NAME; break;
                    default: sortCol = SortCol::NONE; break;
                    }

                    sort_specs->SpecsDirty = false;
                }
            }

            CImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(LIST_PADDING.x, 0.0f));

            // Sort bundles
            std::unordered_map<std::string, size_t> bundles{};
            for (const Preset* preset : presets) {
                if (bundles.find(preset->bundle) == bundles.end()) {
                    bundles.emplace(preset->bundle, 1);
                }
                else {
                    bundles.at(preset->bundle)++;
                }
            }

            float contentRegionWidth = CImGui::GetContentRegionAvail().x;
            for (const auto& [bundle, count] : bundles) {
                CImGui::TableNextRow();

                CImGui::TableNextColumn();

                constexpr float selectableHeight = 60.0f;
                ImVec2 pos = CImGui::GetCursorScreenPos();
                if (CImGui::Selectable(("##Selectable_" + bundle).c_str(), false, 0, ImVec2(0.0f, selectableHeight))) {
                    bundleViewed = bundle;
                }

                // Group name... floating because imgui has no vertical alignment STILL :(
                ImVec2 labelSize = CImGui::CalcTextSize(bundle.c_str());
                ImVec2 labelPos;
                labelPos.x = pos.x + CImGui::GetStyle().ItemSpacing.x;
                labelPos.y = pos.y + (selectableHeight - labelSize.y) * 0.5f;

                CImGui::GetWindowDrawList()->AddText(labelPos, CImGui::GetColorU32(ImGuiCol_Text), bundle.c_str());

                // Similarly for preset count
                const size_t nPresets = count;
                std::string rightText = std::to_string(nPresets);
                if (nPresets == 0) rightText = "Empty";
                else if (nPresets == 1) rightText += " Preset";
                else rightText += " Presets";

                ImVec2 rightTextSize = CImGui::CalcTextSize(rightText.c_str());
                float contentRegionWidth = CImGui::GetContentRegionAvail().x;
                float cursorPosX = CImGui::GetCursorScreenPos().x + contentRegionWidth - rightTextSize.x - CImGui::GetStyle().ItemSpacing.x;

                CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
                CImGui::GetWindowDrawList()->AddText(ImVec2(cursorPosX, labelPos.y), CImGui::GetColorU32(ImGuiCol_Text), rightText.c_str());
                CImGui::PopStyleColor();
            }

            CImGui::PopStyleVar();
            CImGui::EndTable();
        }

    }

    void PresetsTab::drawPresetList(const std::string& bundleFilter) {
        static bool mustHaveLegs = false;
        static bool mustHaveCoil = false;
        static bool mustHaveArms = false;
        static bool mustHaveBody = false;
        static bool mustHaveHelm = false;
        static bool mustHaveSet  = false;

        ImVec2 pos = CImGui::GetCursorPos();
		float frameHeight = CImGui::GetFrameHeight();
		const char* filterTextStr = "Filters:";
		const ImVec2 filterTextStrSize = CImGui::CalcTextSize(filterTextStr);
        CImGui::SetCursorPosY(pos.y + (frameHeight - filterTextStrSize.y) * 0.5f);
		CImGui::Text("Filters:");
		CImGui::SetCursorPos(ImVec2{ pos.x + filterTextStrSize.x + CImGui::GetStyle().ItemSpacing.x, pos.y });

		const ImVec2 iconSize{ frameHeight, frameHeight };
		const ImVec4* armourFilterActiveCol = CImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered);
        constexpr ImVec4 armourFilterDisabledCol{ 1.0f, 1.0f, 1.0f, 0.3f };

        constexpr auto getTextPos = [](ImVec2 pos, ImVec2 size, std::string text) -> ImVec2 {
            ImVec2 text_size = CImGui::CalcTextSize(text.c_str());
            return ImVec2(
                pos.x + (size.x - text_size.x) * 0.5f,
                pos.y + (size.y - text_size.y) * 0.5f + 5.0f
            );
		};

		CImGui::PushFont(wsSymbolFont, FONT_SIZE_DEFAULT_WILDS_SYMBOLS);
		CImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2.0f, 0.0f));

        ArmourPieceFlags hoveredButton = ArmourPieceFlagBits::APF_NONE;

        pos = CImGui::GetCursorScreenPos();
		if (CImGui::InvisibleButton("##SetFilter", iconSize)) mustHaveSet = !mustHaveSet;
        CImGui::GetWindowDrawList()->AddText(getTextPos(pos, iconSize, WS_FONT_SET), CImGui::GetColorU32(mustHaveSet ? *armourFilterActiveCol : armourFilterDisabledCol), "s");
		if (CImGui::IsItemHovered()) hoveredButton = ArmourPieceFlagBits::APF_SET;
        CImGui::SameLine();

		pos = CImGui::GetCursorScreenPos();
		if (CImGui::InvisibleButton("##HelmFilter", iconSize)) mustHaveHelm = !mustHaveHelm;
		CImGui::GetWindowDrawList()->AddText(getTextPos(pos, iconSize, WS_FONT_HELM), CImGui::GetColorU32(mustHaveHelm ? *armourFilterActiveCol : armourFilterDisabledCol), "h");
		if (CImGui::IsItemHovered()) hoveredButton = ArmourPieceFlagBits::APF_HELM;
        CImGui::SameLine();

        pos = CImGui::GetCursorScreenPos();
		if (CImGui::InvisibleButton("##BodyFilter", iconSize)) mustHaveBody = !mustHaveBody;
		if (CImGui::IsItemHovered()) hoveredButton = ArmourPieceFlagBits::APF_BODY;
		CImGui::GetWindowDrawList()->AddText(getTextPos(pos, iconSize, WS_FONT_BODY), CImGui::GetColorU32(mustHaveBody ? *armourFilterActiveCol : armourFilterDisabledCol), "b");
        CImGui::SameLine();

		pos = CImGui::GetCursorScreenPos();
		if (CImGui::InvisibleButton("##ArmsFilter", iconSize)) mustHaveArms = !mustHaveArms;
		if (CImGui::IsItemHovered()) hoveredButton = ArmourPieceFlagBits::APF_ARMS;
		CImGui::GetWindowDrawList()->AddText(getTextPos(pos, iconSize, WS_FONT_ARMS), CImGui::GetColorU32(mustHaveArms ? *armourFilterActiveCol : armourFilterDisabledCol), "a");
		CImGui::SameLine();

		pos = CImGui::GetCursorScreenPos();
		if (CImGui::InvisibleButton("##CoilFilter", iconSize)) mustHaveCoil = !mustHaveCoil;
		if (CImGui::IsItemHovered()) hoveredButton = ArmourPieceFlagBits::APF_COIL;
		CImGui::GetWindowDrawList()->AddText(getTextPos(pos, iconSize, WS_FONT_COIL), CImGui::GetColorU32(mustHaveCoil ? *armourFilterActiveCol : armourFilterDisabledCol), "c");
		CImGui::SameLine();

		pos = CImGui::GetCursorScreenPos();
		if (CImGui::InvisibleButton("##LegsFilter", iconSize)) mustHaveLegs = !mustHaveLegs;
		if (CImGui::IsItemHovered()) hoveredButton = ArmourPieceFlagBits::APF_LEGS;
		CImGui::GetWindowDrawList()->AddText(getTextPos(pos, iconSize, WS_FONT_LEGS), CImGui::GetColorU32(mustHaveLegs ? *armourFilterActiveCol : armourFilterDisabledCol), "l");
		
        if (hoveredButton != ArmourPieceFlagBits::APF_NONE) {
            std::string tooltip;
            switch (hoveredButton)
            {
            case ArmourPieceFlagBits::APF_SET:  tooltip = "Has base modifiers"; break;
            case ArmourPieceFlagBits::APF_HELM: tooltip = "Has helmet modifiers"; break;
            case ArmourPieceFlagBits::APF_BODY: tooltip = "Has chest modifiers"; break;
            case ArmourPieceFlagBits::APF_ARMS: tooltip = "Has arm modifiers"; break;
            case ArmourPieceFlagBits::APF_COIL: tooltip = "Has coil modifiers"; break;
            case ArmourPieceFlagBits::APF_LEGS: tooltip = "Has leg modifiers"; break;
            default: break;
            }
            if (!tooltip.empty()) {
				CImGui::PushFont(dataManager.getRegularFontOverride(), FONT_SIZE_DEFAULT_MAIN);
                CImGui::SetTooltip(tooltip.c_str());
				CImGui::PopFont();
            }
		}

        CImGui::PopStyleVar();
        CImGui::PopFont();

        static char filterBuffer[128] = "";
        std::string filterStr{ filterBuffer };

        CImGui::SameLine();
        CImGui::PushItemWidth(-1);
        CImGui::InputTextWithHint("##Search", "Search...", filterBuffer, IM_ARRAYSIZE(filterBuffer));
        CImGui::PopItemWidth();

        CImGui::Spacing();

        static bool sortDirAscending;
        static enum class SortCol {
            NONE,
            NAME
        } sortCol;

		ArmourPieceFlags filterFlags = ArmourPieceFlagBits::APF_NONE;
		if (mustHaveSet)  filterFlags |= ArmourPieceFlagBits::APF_SET;
		if (mustHaveHelm) filterFlags |= ArmourPieceFlagBits::APF_HELM;
		if (mustHaveBody) filterFlags |= ArmourPieceFlagBits::APF_BODY;
		if (mustHaveArms) filterFlags |= ArmourPieceFlagBits::APF_ARMS;
		if (mustHaveCoil) filterFlags |= ArmourPieceFlagBits::APF_COIL;
		if (mustHaveLegs) filterFlags |= ArmourPieceFlagBits::APF_LEGS;
        std::vector<const Preset*> presets = dataManager.getPresets(filterStr, filterFlags, mustHaveLegs);

        // Sort
        switch (sortCol)
        {
        case SortCol::NAME:
            std::sort(presets.begin(), presets.end(), [&](const Preset* a, const Preset* b) {
                std::string lowa = toLower(a->name); std::string lowb = toLower(b->name);
                return sortDirAscending ? lowa < lowb : lowa > lowb;
                });
        }

        if (presets.size() == 0) {
            CImGui::Spacing();
            CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
            constexpr char const* noPresetStr = "No Presets Found";
            preAlignCellContentHorizontal(noPresetStr);
            CImGui::Text(noPresetStr);
            CImGui::PopStyleColor();
        }
        else {
            constexpr ImGuiTableFlags presetTableFlags =
                ImGuiTableFlags_BordersInnerH
                | ImGuiTableFlags_BordersInnerV
                | ImGuiTableFlags_PadOuterX
                | ImGuiTableFlags_Sortable
                | ImGuiTableFlags_ScrollY;
            CImGui::BeginTable("#PresetTab_PresetList", 1, presetTableFlags);

            constexpr ImGuiTableColumnFlags stretchSortFlags =
                ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthStretch;

            CImGui::TableSetupColumn("Preset", stretchSortFlags, 0.0f);
            CImGui::TableSetupScrollFreeze(0, 1);
            CImGui::TableHeadersRow();

            // Sorting for preset name
            if (ImGuiTableSortSpecs* sort_specs = CImGui::TableGetSortSpecs()) {
                if (sort_specs->SpecsDirty && sort_specs->SpecsCount > 0) {
                    const ImGuiTableColumnSortSpecs& sort_spec = sort_specs->Specs[0];
                    sortDirAscending = sort_spec.SortDirection == ImGuiSortDirection_Ascending;

                    switch (sort_spec.ColumnIndex)
                    {
                    case 0:  sortCol = SortCol::NAME; break;
                    default: sortCol = SortCol::NONE; break;
                    }

                    sort_specs->SpecsDirty = false;
                }
            }

            CImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(LIST_PADDING.x, 0.0f));

            float contentRegionWidth = CImGui::GetContentRegionAvail().x;
            for (const Preset* preset : presets) {
                if (!bundleFilter.empty() && preset->bundle != bundleFilter) continue;

                CImGui::TableNextRow();

                CImGui::TableNextColumn();

                constexpr float selectableHeight = 60.0f;
                constexpr ImVec4 armourMissingCol{ 1.0f, 1.0f, 1.0f, 0.1f };
                constexpr ImVec4 armourPresentCol{ 1.0f, 1.0f, 1.0f, 1.0f };

                ImVec2 pos = CImGui::GetCursorScreenPos();
                if (CImGui::Selectable(("##Selectable_Preset_" + preset->name).c_str(), false, 0, ImVec2(0.0f, selectableHeight))) {
                    openEditPresetPanel(preset->uuid);
                }

                // Sex Mark
                std::string sexMarkSymbol = preset->female ? WS_FONT_FEMALE : WS_FONT_MALE;
                ImVec4 sexMarkerCol = preset->female ? ImVec4(0.76f, 0.50f, 0.24f, 1.0f) : ImVec4(0.50f, 0.70f, 0.33f, 1.0f);

                CImGui::PushFont(wsSymbolFont, FONT_SIZE_DEFAULT_WILDS_SYMBOLS);

                constexpr float sexMarkerSpacingAfter = 5.0f;
                constexpr float sexMarkerVerticalAlignOffset = 5.0f;
                ImVec2 sexMarkerSize = CImGui::CalcTextSize(sexMarkSymbol.c_str());
                ImVec2 sexMarkerPos;
                sexMarkerPos.x = pos.x + CImGui::GetStyle().ItemSpacing.x;
                sexMarkerPos.y = pos.y + (selectableHeight - sexMarkerSize.y) * 0.5f + sexMarkerVerticalAlignOffset;
                CImGui::GetWindowDrawList()->AddText(sexMarkerPos, CImGui::GetColorU32(sexMarkerCol), sexMarkSymbol.c_str());

                CImGui::PopFont();

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

                // Armour Marks
                constexpr float armourVerticalAlignOffset = 2.5f;
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

            CImGui::PopStyleVar();
            CImGui::EndTable();
        }

    }

    void PresetsTab::openCreatePresetPanel() {
        createPresetPanel.openNew("Create Preset", "CreatePresetPanel", dataManager, wsSymbolFont, wsArmourFont, getBundleViewed());
        createPresetPanel.get()->focus();

        createPresetPanel.get()->onCancel([&]() {
            createPresetPanel.close();
        });

        createPresetPanel.get()->onCreate([&](const Preset& preset) {
            dataManager.addPreset(preset);
            createPresetPanel.close();
        });
    }

    void PresetsTab::openCombinePresetsPanel() {
        combinePresetsPanel.openNew("Combine Presets", "CombinePresetsPanel", dataManager, wsSymbolFont, wsArmourFont, getBundleViewed());
        combinePresetsPanel.get()->focus();
        combinePresetsPanel.get()->onCancel([&]() {
            combinePresetsPanel.close();
        });
        combinePresetsPanel.get()->onCombine([&](const Preset& preset) {
            dataManager.addPreset(preset);
            combinePresetsPanel.close();
        });
	}

    void PresetsTab::openEditPresetPanel(const std::string& presetUUID) {
        const Preset* preset = dataManager.getPresetByUUID(presetUUID);
        std::string windowName = std::format("Edit Preset - {}", preset ? preset->name : "Unknown");
        editPresetPanel.openNew(presetUUID, windowName, "EditPresetPanel", dataManager, wsSymbolFont, wsArmourFont);
        editPresetPanel.get()->focus();

        editPresetPanel.get()->onDelete([&](const std::string& presetUUID) {
            dataManager.deletePreset(presetUUID);
            editPresetPanel.close();
        });

        editPresetPanel.get()->onCancel([&]() {
            editPresetPanel.close();
        });

        editPresetPanel.get()->onUpdate([&](const std::string& presetUUID, Preset preset) {
            dataManager.updatePreset(presetUUID, preset);
            editPresetPanel.close();
        });

        editPresetPanel.get()->onOpenEditor([&](std::string presetUUID) {
            editPresetPanel.close();
            INVOKE_REQUIRED_CALLBACK(openPresetInEditorCb, presetUUID);
        });
    }

    void PresetsTab::openImportFBSPresetsPanel() {
        importFBSPresetsPanel.openNew("Import FBS Presets", "ImportFBSPresetsPanel", dataManager, wsSymbolFont, wsArmourFont);
        importFBSPresetsPanel.get()->focus();
        importFBSPresetsPanel.get()->onCancel([&]() {
            importFBSPresetsPanel.close();
        });
        importFBSPresetsPanel.get()->onImport([&](const std::vector<Preset>& presets) {
            for (const Preset& preset : presets) {
                dataManager.addPreset(preset);
            }
            importFBSPresetsPanel.close();
        });
	}

    void PresetsTab::openWarnDeleteBundlePanel(const std::string& bundleName) {
        auto messages = std::vector<std::string>{
            std::format("Are you sure you want to delete the bundle \"{}\"?", bundleName),
            "All presets in the bundle will be permanently removed."
        };

        warnDeleteBundlePanel.openNew(
            std::format("Warning: Delete Bundle \"{}\"", bundleName),
            "WarnDeleteBundlePanel",
            messages,
            "Delete Bundle",
            "Cancel",
            false
        );
        warnDeleteBundlePanel.get()->focus();
        warnDeleteBundlePanel.get()->onOk([this, bundleName]() {
            dataManager.deletePresetBundle(bundleName);
            bundleViewed = "";
            warnDeleteBundlePanel.close();
        });
	}

    std::string PresetsTab::getBundleViewed() {
        return inBundlesTab ? bundleViewed : "";
    }
}