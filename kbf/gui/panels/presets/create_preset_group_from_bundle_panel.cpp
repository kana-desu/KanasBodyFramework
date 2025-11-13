#include <kbf/gui/panels/presets/create_preset_group_from_bundle_panel.hpp>

#include <kbf/util/id/uuid_generator.hpp>
#include <kbf/util/functional/invoke_callback.hpp>
#include <kbf/debug/debug_stack.hpp>

#define CREATE_PRESET_GROUP_PANEL_LOG_TAG "[CreatePresetGroupFromBundlePanel]"

namespace kbf {

    CreatePresetGroupFromBundlePanel::CreatePresetGroupFromBundlePanel(
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

    void CreatePresetGroupFromBundlePanel::initializeBuffers() {
        std::strcpy(presetGroupNameBuffer, presetGroup.name.c_str());
    }

    bool CreatePresetGroupFromBundlePanel::draw() {
        bool open = true;
        processFocus();

        conflictInfoPanel.draw();

        bool needsDisable = false;
		if (conflictInfoPanel.isVisible()) needsDisable = true;
        if (needsDisable) {
            CImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, 1.0f); // Prevent fading
            CImGui::BeginDisabled();
        }

        CImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, ImVec2(0.5f, 0.5f));

        constexpr ImVec2 minWindowSize = ImVec2(450, 425);

        if (isFirstDraw()) { CImGui::SetNextWindowSize(minWindowSize); }
        CImGui::SetNextWindowSizeConstraints(minWindowSize, ImVec2(FLT_MAX, FLT_MAX));
        CImGui::Begin(nameWithID.c_str(), &open, ImGuiWindowFlags_NoCollapse);

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

        static char selectedBundleBuffer[128] = "";
        std::strcpy(selectedBundleBuffer, selectedBundle.c_str());

        CImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, 1.0f); // Prevent fading
        CImGui::BeginDisabled();
        CImGui::InputText(" Bundle ", selectedBundleBuffer, IM_ARRAYSIZE(selectedBundleBuffer));
        CImGui::EndDisabled();
        CImGui::PopStyleVar();

        CImGui::Spacing();

        static char filterBuffer[128] = "";
        std::string filterStr{ filterBuffer };

        drawBundleList(dataManager.getPresetBundlesWithCounts(filterStr, true));

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

        const bool nameEmpty = presetGroup.name.empty();
        const bool bundleEmpty = selectedBundle.empty();
        const bool alreadyExists = dataManager.presetGroupExists(presetGroup.name);
        const bool disableCreateButton = nameEmpty || bundleEmpty || alreadyExists;
        if (disableCreateButton) CImGui::BeginDisabled();
        if (CImGui::Button(kCreateLabel)) {
			size_t conflicts = assignPresetsToGroup(dataManager.getPresetsInBundle(selectedBundle));
            if (conflicts > 0) {
                openConflictInfoPanel(conflicts);
            }
            else {
                INVOKE_REQUIRED_CALLBACK(createCallback, presetGroup);
            }
        }
        if (disableCreateButton) CImGui::EndDisabled();
        if (nameEmpty) CImGui::SetItemTooltip("Please provide a preset group name");
        else if (bundleEmpty) CImGui::SetItemTooltip("Please select a bundle");
        else if (alreadyExists) CImGui::SetItemTooltip("Preset group name already taken");

        CImGui::End();
        CImGui::PopStyleVar();

        if (needsDisable) {
            CImGui::EndDisabled();
            CImGui::PopStyleVar();
		}

        return open;
    }

    void CreatePresetGroupFromBundlePanel::drawBundleList(const std::vector<std::pair<std::string, size_t>>& bundleList) {
        // Fixed-height, scrollable region
        CImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.02f, 0.02f, 0.02f, 1.0f));
        CImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 10));

        const float height = -(2.0f * CImGui::GetFrameHeightWithSpacing() + 10.0f); // Leave space for the search bar
        CImGui::BeginChild("BundleListChild", ImVec2(0, height), true, ImGuiWindowFlags_HorizontalScrollbar);

        float contentRegionWidth = CImGui::GetContentRegionAvail().x;

        if (bundleList.size() == 0) {
            constexpr const char* noneFoundStr = "No Bundles Found";

            CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
            CImGui::SetCursorPosX(CImGui::GetCursorPosX() + (CImGui::GetColumnWidth() - CImGui::CalcTextSize(noneFoundStr).x) * 0.5f);
            CImGui::Text(noneFoundStr);
            CImGui::PopStyleColor();
        }
        else {
            for (const auto& bundle : bundleList)
            {
				std::string bundleName = bundle.first;
				size_t presetCount = bundle.second;

                if (CImGui::Selectable(bundleName.c_str(), selectedBundle == bundleName)) {
                    selectedBundle = bundleName;
                }

                std::string presetCountStr;
                if (presetCount == 0) presetCountStr = "Empty";
                else if (presetCount == 1) presetCountStr = "1 Preset";
                else presetCountStr = std::to_string(presetCount) + " Presets";

                ImVec2 rightTextSize = CImGui::CalcTextSize(presetCountStr.c_str());
                float presetCountTextPosX = CImGui::GetCursorScreenPos().x + contentRegionWidth - rightTextSize.x;
                float presetCountTextPosY = CImGui::GetItemRectMin().y + (CImGui::GetFrameHeight() - rightTextSize.y) * 0.5f;  // Same Y as the selectable item, plus vertical alignment

                CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
                CImGui::GetWindowDrawList()->AddText(ImVec2(presetCountTextPosX, presetCountTextPosY), CImGui::GetColorU32(ImGuiCol_Text), presetCountStr.c_str());
                CImGui::PopStyleColor();
            }
        }

        CImGui::EndChild();
        CImGui::PopStyleVar();
        CImGui::PopStyleColor();
    }

    size_t CreatePresetGroupFromBundlePanel::assignPresetsToGroup(const std::vector<std::string>& presetUUIDs) {
		DEBUG_STACK.push(std::format("{} Creating preset group \"{}\" ({}) from bundle \"{}\"...", CREATE_PRESET_GROUP_PANEL_LOG_TAG, presetGroup.name, presetGroup.uuid, selectedBundle), DebugStack::Color::COL_INFO);

        size_t conflictCount = 0;

        for (const auto& uuid : presetUUIDs) {
            const Preset* preset = dataManager.getPresetByUUID(uuid);
            if (!preset) continue; // Skip if preset not found

            for (ArmourPiece piece = ArmourPiece::AP_MIN; piece <= ArmourPiece::AP_MAX_EXCLUDING_SLINGER; piece = static_cast<ArmourPiece>(static_cast<int>(piece) + 1)) {
                if (preset->hasModifiers(piece) || preset->hasPartOverrides(piece)) {
                    // Base (set) modifiers can be applied as many times as we want.
                    std::unordered_map<ArmourSet, std::string>* presetMap = presetGroup.getPresetMap(piece);
					bool presetAlreadyAssigned = piece != ArmourPiece::AP_SET && presetMap->find(preset->armour) != presetMap->end();
                    if (presetAlreadyAssigned) {
						std::string existingPresetName = dataManager.getPresetByUUID(presetMap->at(preset->armour))->name;
                        DEBUG_STACK.push(std::format("{} Preset \"{}\" was ignored while assigning {} due to a conflicting preset \"{}\" from the same bundle", CREATE_PRESET_GROUP_PANEL_LOG_TAG, preset->name, preset->armour.name, existingPresetName), DebugStack::Color::COL_WARNING);
                        conflictCount++;
                    }
					else {
						presetMap->emplace(preset->armour, uuid);
                    }
                }
            }
           
            if (!preset->hasAnyModifiers() && !preset->hasAnyPartOverrides() && !preset->hasAnyMaterialOverrides()) {
                DEBUG_STACK.push(std::format("{} Preset \"{}\" ({}) was ignored while assigning to preset group as it contains no modifiers/removers.", CREATE_PRESET_GROUP_PANEL_LOG_TAG, preset->name, preset->armour.name), DebugStack::Color::COL_WARNING);
                conflictCount++;
			}
        }

        DEBUG_STACK.push(std::format("{} Created preset group \"{}\" ({}) from bundle \"{}\" ({} Conflicts)", CREATE_PRESET_GROUP_PANEL_LOG_TAG, presetGroup.name, presetGroup.uuid, selectedBundle, conflictCount), DebugStack::Color::COL_SUCCESS);
        return conflictCount;
    }

    void CreatePresetGroupFromBundlePanel::openConflictInfoPanel(size_t conflictCount) {
        conflictInfoPanel.openNew(
            std::format("Warning: Bundle {} Has Conflicts", selectedBundle),
            "ConflictInfoPanel", 
            std::format("{} presets will be ignored in the created preset group due to conflicting presets in the bundle.\nCheck Debug > Log after creation to see which presets were used to resolve the conflict.", conflictCount),
            "Create Anyway",
            "",
            false
        );

        conflictInfoPanel.get()->focus();
        conflictInfoPanel.get()->onOk([this]() {
            conflictInfoPanel.close();
            INVOKE_REQUIRED_CALLBACK(createCallback, presetGroup);
		});
	}

}