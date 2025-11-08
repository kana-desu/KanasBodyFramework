#include <kbf/gui/tabs/editor/editor_tab.hpp>

#include <kbf/gui/shared/alignment.hpp>
#include <kbf/gui/shared/delete_button.hpp>
#include <kbf/gui/shared/bone_slider.hpp>
#include <kbf/gui/shared/styling_consts.hpp>
#include <kbf/gui/shared/sex_marker.hpp>
#include <kbf/data/ids/font_symbols.hpp>
#include <kbf/data/ids/special_armour_ids.hpp>
#include <kbf/data/armour/armour_list.hpp>
#include <kbf/util/functional/invoke_callback.hpp>
#include <kbf/debug/debug_stack.hpp>
#include <kbf/data/bones/default_bones.hpp>
#include <kbf/data/bones/common_bones.hpp>
#include <kbf/data/bones/bone_symmetry_utils.hpp>
#include <kbf/util/font/default_font_sizes.hpp>

#include <kbf/gui/components/toggle/imgui_toggle.h>
#include <kbf/gui/shared/toggle_colours.hpp>

#include <kbf/util/string/to_lower.hpp>

#include <format>
#include <unordered_set>

#define EDITOR_TAB_LOG_TAG "[EditorTab]"

namespace kbf {

    void EditorTab::draw() {
        if (needsEditNone) {
            editNone();
			needsEditNone = false;
        }
        else if (presetToEdit) {
            editNone();
            editPreset(presetToEdit);
            presetToEdit = nullptr;
		}

        if (openObject.notSet()) {
            drawNoEditor();
        }
        else if (openObject.type == EditableObject::ObjectType::PRESET) {
            drawPresetEditor();
        }
        else if (openObject.type == EditableObject::ObjectType::PRESET_GROUP) {
            drawPresetGroupEditor();
        }
    }

    void EditorTab::drawPopouts() {
        presetPanel.draw();
        presetGroupPanel.draw();
        selectBonePanel.draw();
        partRemoverPanel.draw();
        navWarnUnsavedPanel.draw();
        assignPresetPanel.draw();
		createPresetPanel.draw();
    }

    void EditorTab::closePopouts() {
        presetPanel.close();
        presetGroupPanel.close();
        selectBonePanel.close();
		partRemoverPanel.close();
        navWarnUnsavedPanel.close();
        assignPresetPanel.close();
		createPresetPanel.close();
        dataManager.previewPreset(nullptr);
    }

    void EditorTab::editPresetGroup(PresetGroup* presetGroup) { 
        openObject.setPresetGroup(presetGroup);
        presetPanel.close();
        presetGroupPanel.close();
        selectBonePanel.close();
		partRemoverPanel.close();
        assignPresetPanel.close();
		createPresetPanel.close();
        initializePresetGroupBuffers(presetGroup);
    }

    void EditorTab::editPreset(Preset* preset) { 
        openObject.setPreset(preset); 
		setBoneInfoWidget.onAddBoneModifier([&]() { openSelectBonePanel(ArmourPiece::AP_SET); });
		helmBoneInfoWidget.onAddBoneModifier([&]() { openSelectBonePanel(ArmourPiece::AP_HELM); });
        bodyBoneInfoWidget.onAddBoneModifier([&]() { openSelectBonePanel(ArmourPiece::AP_BODY); });
		armsBoneInfoWidget.onAddBoneModifier([&]() { openSelectBonePanel(ArmourPiece::AP_ARMS); });
		coilBoneInfoWidget.onAddBoneModifier([&]() { openSelectBonePanel(ArmourPiece::AP_COIL); });
        legsBoneInfoWidget.onAddBoneModifier([&]() { openSelectBonePanel(ArmourPiece::AP_LEGS); });
        presetPanel.close();
        presetGroupPanel.close();
        selectBonePanel.close();
		partRemoverPanel.close();
        assignPresetPanel.close();
		createPresetPanel.close();
        initializePresetBuffers(preset);
    }

    void EditorTab::initializePresetGroupBuffers(const PresetGroup* presetGroup) {
        strcpy(presetGroupNameBuffer, presetGroup->name.c_str());
    }

    void EditorTab::initializePresetBuffers(const Preset* preset) {
        strcpy(presetNameBuffer, preset->name.c_str());
        strcpy(presetBundleBuffer, preset->bundle.c_str());
    }

    void EditorTab::drawNoEditor() {
        CImGui::PushStyleVar(ImGuiStyleVar_CellPadding, LIST_PADDING);

        const ImVec2 buttonSize = ImVec2(CImGui::GetContentRegionAvail().x, 0.0f);
        if (CImGui::Button("Edit a Preset", buttonSize)) {
            openSelectPresetPanel();
        }

        if (CImGui::Button("Edit a Preset Group", buttonSize)) {
            openSelectPresetGroupPanel();
        }
        CImGui::Spacing();

        CImGui::Spacing();
        CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
        constexpr char const* noPresetStr = "You can also click on entries in the Preset / Preset Group tabs to edit them here.";
        preAlignCellContentHorizontal(noPresetStr);
        CImGui::Text(noPresetStr);
        CImGui::PopStyleColor();

        CImGui::PopStyleVar();
    }

    void EditorTab::openSelectPresetPanel() {
        presetPanel.openNew("Select Preset", "EditPanel_NpcTab", dataManager, wsSymbolFont, wsArmourFont, false);
        presetPanel.get()->focus();

        presetPanel.get()->onSelectPreset([&](std::string uuid) {
            editPreset(dataManager.getPresetByUUID(uuid));
            presetPanel.close();
        });
    }

    void EditorTab::openSelectPresetGroupPanel() {
        presetGroupPanel.openNew("Select Preset Group", "EditDefaultPanel_NpcTab", dataManager, wsSymbolFont, false);
        presetGroupPanel.get()->focus();

        presetGroupPanel.get()->onSelectPresetGroup([&](std::string uuid) {
            editPresetGroup(dataManager.getPresetGroupByUUID(uuid));
            presetGroupPanel.close();
        });
    }

    void EditorTab::openCopyPresetGroupPanel() {
        presetGroupPanel.openNew("Copy Existing Preset Group", "EditPresetGroupPanel_CopyPanel", dataManager, wsSymbolFont, false);
        presetGroupPanel.get()->focus();

        presetGroupPanel.get()->onSelectPresetGroup([&](std::string uuid) {
            const PresetGroup* copyPresetGroup = dataManager.getPresetGroupByUUID(uuid);
            if (copyPresetGroup) {
                openObject.setPresetGroupCurrent(copyPresetGroup);
                openObject.ptrAfter.presetGroup->uuid = openObject.ptrBefore.presetGroup->uuid;
                openObject.ptrAfter.presetGroup->name = openObject.ptrBefore.presetGroup->name;
                initializePresetGroupBuffers(openObject.ptrAfter.presetGroup);
            }
            else {
                DEBUG_STACK.push(std::format("{} Could not find preset group with UUID {} while trying to make a copy.", EDITOR_TAB_LOG_TAG, uuid), DebugStack::Color::ERROR);
            }
            presetGroupPanel.close();
        });
    }

    void EditorTab::openCopyPresetPanel() {
        presetPanel.openNew("Copy Existing Preset", "EditPresetPanel_CopyPanel", dataManager, wsSymbolFont, wsArmourFont, false);
        presetPanel.get()->focus();

        presetPanel.get()->onSelectPreset([&](std::string uuid) {
            const Preset* copyPreset = dataManager.getPresetByUUID(uuid);
            if (copyPreset) {
                openObject.setPresetCurrent(copyPreset);
                openObject.ptrAfter.preset->uuid = openObject.ptrBefore.preset->uuid;
                openObject.ptrAfter.preset->name = openObject.ptrBefore.preset->name;
                initializePresetBuffers(openObject.ptrAfter.preset);
            }
            else {
                DEBUG_STACK.push(std::format("{} Could not find preset with UUID {} while trying to make a copy.", EDITOR_TAB_LOG_TAG, uuid), DebugStack::Color::ERROR);
            }
            presetPanel.close();
            });
    }

    void EditorTab::openSelectBonePanel(ArmourPiece piece) {
        selectBonePanel.openNew("Add Bone Modifier", "EditPreset_BoneModifierPanel", dataManager, &openObject.ptrAfter.preset, piece, wsSymbolFont);
        selectBonePanel.get()->focus();

        selectBonePanel.get()->onSelectBone([&, piece](std::string name) {
            switch (piece) {
			case ArmourPiece::AP_SET:  openObject.ptrAfter.preset->set.modifiers.emplace(name, BoneModifier{}); break;
			case ArmourPiece::AP_HELM: openObject.ptrAfter.preset->helm.modifiers.emplace(name, BoneModifier{}); break;
			case ArmourPiece::AP_BODY: openObject.ptrAfter.preset->body.modifiers.emplace(name, BoneModifier{}); break;
			case ArmourPiece::AP_ARMS: openObject.ptrAfter.preset->arms.modifiers.emplace(name, BoneModifier{}); break;
			case ArmourPiece::AP_COIL: openObject.ptrAfter.preset->coil.modifiers.emplace(name, BoneModifier{}); break;
			case ArmourPiece::AP_LEGS: openObject.ptrAfter.preset->legs.modifiers.emplace(name, BoneModifier{}); break;
            }
            selectBonePanel.close();
        });

        selectBonePanel.get()->onCheckBoneDisabled([&, piece](std::string name) {
            switch (piece) {
			case ArmourPiece::AP_SET:  return openObject.ptrAfter.preset->set.modifiers.find(name) != openObject.ptrAfter.preset->set.modifiers.end();
			case ArmourPiece::AP_HELM: return openObject.ptrAfter.preset->helm.modifiers.find(name) != openObject.ptrAfter.preset->helm.modifiers.end();
			case ArmourPiece::AP_BODY: return openObject.ptrAfter.preset->body.modifiers.find(name) != openObject.ptrAfter.preset->body.modifiers.end();
			case ArmourPiece::AP_ARMS: return openObject.ptrAfter.preset->arms.modifiers.find(name) != openObject.ptrAfter.preset->arms.modifiers.end();
			case ArmourPiece::AP_COIL: return openObject.ptrAfter.preset->coil.modifiers.find(name) != openObject.ptrAfter.preset->coil.modifiers.end();
			case ArmourPiece::AP_LEGS: return openObject.ptrAfter.preset->legs.modifiers.find(name) != openObject.ptrAfter.preset->legs.modifiers.end();
            }

            return true;
        });

        selectBonePanel.get()->onAddDefaults([&, piece]() {
			// TODO: Need to actually catalog some default bones here
            std::set<std::string> defaultBones = getDefaultBones(piece, openObject.ptrAfter.preset->female);

            for (const std::string& bone : defaultBones) {
                switch (piece) {
				case ArmourPiece::AP_SET: openObject.ptrAfter.preset->set.modifiers.emplace(bone, BoneModifier{}); break;
				case ArmourPiece::AP_HELM: openObject.ptrAfter.preset->helm.modifiers.emplace(bone, BoneModifier{}); break;
				case ArmourPiece::AP_BODY: openObject.ptrAfter.preset->body.modifiers.emplace(bone, BoneModifier{}); break;
				case ArmourPiece::AP_ARMS: openObject.ptrAfter.preset->arms.modifiers.emplace(bone, BoneModifier{}); break;
				case ArmourPiece::AP_COIL: openObject.ptrAfter.preset->coil.modifiers.emplace(bone, BoneModifier{}); break;
				case ArmourPiece::AP_LEGS: openObject.ptrAfter.preset->legs.modifiers.emplace(bone, BoneModifier{}); break;
                }
            }
            selectBonePanel.close();
        });
    }

    void EditorTab::openPartOverridePanel() {
        ArmourSetWithCharacterSex armourSetWithSex{
            .set = openObject.ptrAfter.preset->armour,
            .characterFemale = openObject.ptrAfter.preset->female
        };

        partRemoverPanel.openNew("Show/Hide Armour Part", "EditPreset_PartRemoverPanel", dataManager, armourSetWithSex, wsSymbolFont);
        partRemoverPanel.get()->focus();

        partRemoverPanel.get()->onSelectPart([&](MeshPart part, ArmourPiece piece) {
            switch (piece) {
			case ArmourPiece::AP_SET: openObject.ptrAfter.preset->set.partOverrides.insert(part); break;
			case ArmourPiece::AP_HELM: openObject.ptrAfter.preset->helm.partOverrides.insert(part); break;
			case ArmourPiece::AP_BODY: openObject.ptrAfter.preset->body.partOverrides.insert(part); break;
			case ArmourPiece::AP_ARMS: openObject.ptrAfter.preset->arms.partOverrides.insert(part); break;
			case ArmourPiece::AP_COIL: openObject.ptrAfter.preset->coil.partOverrides.insert(part); break;
			case ArmourPiece::AP_LEGS: openObject.ptrAfter.preset->legs.partOverrides.insert(part); break;
            }
            partRemoverPanel.close();
        });

        partRemoverPanel.get()->onCheckPartDisabled([&](MeshPart part, ArmourPiece piece) {
            switch (piece) {
            case ArmourPiece::AP_SET:  return openObject.ptrAfter.preset->set.partOverrides.find(part) != openObject.ptrAfter.preset->set.partOverrides.end();
            case ArmourPiece::AP_HELM: return openObject.ptrAfter.preset->helm.partOverrides.find(part) != openObject.ptrAfter.preset->helm.partOverrides.end();
            case ArmourPiece::AP_BODY: return openObject.ptrAfter.preset->body.partOverrides.find(part) != openObject.ptrAfter.preset->body.partOverrides.end();
            case ArmourPiece::AP_ARMS: return openObject.ptrAfter.preset->arms.partOverrides.find(part) != openObject.ptrAfter.preset->arms.partOverrides.end();
            case ArmourPiece::AP_COIL: return openObject.ptrAfter.preset->coil.partOverrides.find(part) != openObject.ptrAfter.preset->coil.partOverrides.end();
            case ArmourPiece::AP_LEGS: return openObject.ptrAfter.preset->legs.partOverrides.find(part) != openObject.ptrAfter.preset->legs.partOverrides.end();
            }

            return true;
        });
    }

    void EditorTab::openAssignPresetPanel(ArmourSet armourSet, ArmourPiece piece) {
        presetPanel.openNew(std::format("Assign Preset - {} ({}): ",
            armourSet.name,
            armourSet.female ? "F" : "M",
            armourPieceToString(piece)
        ), "AssignPresetPanel", dataManager, wsSymbolFont, wsArmourFont, true);
        presetPanel.get()->focus();

        presetPanel.get()->onSelectPreset([&, armourSet, piece](std::string uuid) {
			assignPreset(armourSet, piece, uuid);
            presetPanel.close();
        });
    }

    void EditorTab::drawPresetGroupEditor() {
        assert(openObject.type == EditableObject::ObjectType::PRESET_GROUP && openObject.ptrAfter.presetGroup != nullptr);
        const PresetGroup& presetGroupBefore = *openObject.ptrBefore.presetGroup;

        bool drawTabContent = drawStickyNavigationWidget(
            std::format("Editing Preset Group \"{}\"", presetGroupBefore.name),
            nullptr,
            nullptr,
            // Callback funcs
            [&]() { return *openObject.ptrBefore.presetGroup != *openObject.ptrAfter.presetGroup; },
            [&]() { openObject.revertPresetGroup(); initializePresetGroupBuffers(openObject.ptrAfter.presetGroup); },
            [&](std::string& errMsg) { return canSavePresetGroup(errMsg); },
            savePresetGroupCb);

        if (!drawTabContent) return;

        if (CImGui::Button("Copy Existing Preset Group", ImVec2(CImGui::GetContentRegionAvail().x, 0))) {
            openCopyPresetGroupPanel();
        }

        CImGui::Spacing();
        CImGui::Separator();
        CImGui::Spacing();
        CImGui::Spacing();
        CImGui::Spacing();

        if (CImGui::BeginTabBar("PresetEditorTabs")) {
            if (CImGui::BeginTabItem("Properties")) {
                CImGui::Spacing();
                drawPresetGroupEditor_Properties(&openObject.ptrAfter.presetGroup);
                CImGui::EndTabItem();
            }
            if (CImGui::BeginTabItem("Assigned Presets")) {
                CImGui::Spacing();
                drawPresetGroupEditor_AssignedPresets(&openObject.ptrAfter.presetGroup);
                CImGui::EndTabItem();
            }
            CImGui::EndTabBar();
        }
    }

    void EditorTab::drawPresetGroupEditor_Properties(PresetGroup** presetGroup) {
        CImGui::BeginChild("PresetGroupProperties");
        CImGui::InputText(" Name ", presetGroupNameBuffer, IM_ARRAYSIZE(presetGroupNameBuffer));
        (**presetGroup).name = std::string{ presetGroupNameBuffer };

        CImGui::Spacing();
        std::string sexComboValue = (**presetGroup).female ? "Female" : "Male";
        if (CImGui::BeginCombo(" Sex ", sexComboValue.c_str())) {
            if (CImGui::Selectable("Male")) {
                (**presetGroup).female = false;
            }
            if (CImGui::Selectable("Female")) {
                (**presetGroup).female = true;
            };
            CImGui::EndCombo();
        }
        CImGui::SetItemTooltip("Suggested character sex to use with (not a hard restriction)");
        CImGui::EndChild();
    }

    void EditorTab::drawPresetGroupEditor_AssignedPresets(PresetGroup** presetGroup) {
        static char filterBuffer[128] = "";
        std::string filterStr{ filterBuffer };

        CImGui::PushItemWidth(-1);
        CImGui::InputTextWithHint("##ArmourSearch", "Armour Name...", filterBuffer, IM_ARRAYSIZE(filterBuffer));
        CImGui::PopItemWidth();
        CImGui::Spacing();

        CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));

        std::string hintTextBegin = std::format("This preset group is marked as {} - using ", (**presetGroup).female ? "Female" : "Male");
        std::string hintTextEnd   = " presets is recommended.";
        const float hintWidth = CImGui::CalcTextSize(hintTextBegin.c_str()).x + CImGui::CalcTextSize(hintTextEnd.c_str()).x + CImGui::GetFontSize();

        CImGui::SetCursorPosX(CImGui::GetCursorPosX() + (CImGui::GetContentRegionAvail().x - hintWidth) * 0.5f);
        CImGui::Text(hintTextBegin.c_str());
        CImGui::SameLine();
        drawSexMarker(wsSymbolFont, !(**presetGroup).female, false, false);
        CImGui::SameLine();
		CImGui::Text(hintTextEnd.c_str());

        std::string hint2Text = "Left click to assign a preset, and right click to open a preset in the editor.";
		const float hint2Width = CImGui::CalcTextSize(hint2Text.c_str()).x;
		CImGui::SetCursorPosX(CImGui::GetCursorPosX() + (CImGui::GetContentRegionAvail().x - hint2Width) * 0.5f);
		CImGui::Text(hint2Text.c_str());

        CImGui::PopStyleColor();

        CImGui::Spacing();
        CImGui::Separator();
        CImGui::Spacing();
        CImGui::Spacing();

        const std::vector<ArmourSet> armourSets = ArmourList::getFilteredSets(filterStr);
        if (armourSets.size() == 0) {
            constexpr char const* noArmourStr = "Armour Set Search Found Zero Results.";
            preAlignCellContentHorizontal(noArmourStr);
            CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
            CImGui::Text(noArmourStr);
            CImGui::PopStyleColor();
        }
        else {
            constexpr ImGuiTableFlags assignedPresetGridFlags =
                ImGuiTableFlags_BordersInnerH
                | ImGuiTableFlags_PadOuterX
                | ImGuiTableFlags_Sortable
                | ImGuiTableFlags_ScrollY;

            constexpr ImGuiTableColumnFlags stretchNoSortFlags =
                ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthStretch;
            constexpr ImGuiTableColumnFlags fixedNoSortFlags =
                ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthFixed;

            CImGui::PushStyleVar(ImGuiStyleVar_CellPadding, LIST_PADDING);

            CImGui::BeginTable("##AssignedPresetGridTable", 7, assignedPresetGridFlags);

            CImGui::TableSetupColumn("",       fixedNoSortFlags, 0.0f);
            CImGui::TableSetupColumn("Armour", fixedNoSortFlags, 0.0f);
			CImGui::TableSetupColumn("Head",   stretchNoSortFlags, 0.0f);
            CImGui::TableSetupColumn("Body",   stretchNoSortFlags, 0.0f);
			CImGui::TableSetupColumn("Arms",   stretchNoSortFlags, 0.0f);
			CImGui::TableSetupColumn("Waist",  stretchNoSortFlags, 0.0f);
            CImGui::TableSetupColumn("Legs",   stretchNoSortFlags, 0.0f);
            CImGui::TableSetupScrollFreeze(0, 1);
            CImGui::TableHeadersRow();

            CImGui::PopStyleVar();
            CImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(LIST_PADDING.x, 0.0f));

            constexpr float rowHeight = 40.0f;

            for (const ArmourSet& armour : armourSets) {
                CImGui::TableNextRow();

                if (armour.name != ANY_ARMOUR_ID) {
                    // Sex Marker
                    CImGui::TableSetColumnIndex(0);
                    ImVec2 cursorPos = CImGui::GetCursorPos();
                    constexpr ImVec2 sexMarkerOffset = ImVec2(5.0f, 12.5f);
                    CImGui::SetCursorPos(ImVec2(cursorPos.x + sexMarkerOffset.x, cursorPos.y + sexMarkerOffset.y));
                    drawSexMarker(wsSymbolFont, !armour.female, false, true);
                }

                // Armour Name
                CImGui::TableSetColumnIndex(1);
                CImGui::PushFont(wsArmourFont, FONT_SIZE_DEFAULT_WILDS_ARMOUR);
                CImGui::SetCursorPosY(CImGui::GetCursorPosY() + (rowHeight - CImGui::GetTextLineHeight()) * 0.5f);

                std::string displayName = armour.name == ANY_ARMOUR_ID ? "Default" : armour.name;
                if (armour.name == ANY_ARMOUR_ID) CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.365f, 0.678f, 0.886f, 0.8f));
                CImGui::Text(displayName.c_str());
				if (armour.name == ANY_ARMOUR_ID) CImGui::PopStyleColor();
                CImGui::PopFont();

				Preset* helmPreset = nullptr;
				if ((**presetGroup).armourHasPresetUUID(armour, ArmourPiece::AP_HELM))
					helmPreset = dataManager.getPresetByUUID((**presetGroup).helmPresets.at(armour));

                Preset* bodyPreset = nullptr;
                if ((**presetGroup).armourHasPresetUUID(armour, ArmourPiece::AP_BODY)) 
                    bodyPreset = dataManager.getPresetByUUID((**presetGroup).bodyPresets.at(armour));

				Preset* armsPreset = nullptr;
				if ((**presetGroup).armourHasPresetUUID(armour, ArmourPiece::AP_ARMS))
					armsPreset = dataManager.getPresetByUUID((**presetGroup).armsPresets.at(armour));

				Preset* coilPreset = nullptr;
				if ((**presetGroup).armourHasPresetUUID(armour, ArmourPiece::AP_COIL))
					coilPreset = dataManager.getPresetByUUID((**presetGroup).coilPresets.at(armour));

                Preset* legsPreset = nullptr;
                if ((**presetGroup).armourHasPresetUUID(armour, ArmourPiece::AP_LEGS))
                    legsPreset = dataManager.getPresetByUUID((**presetGroup).legsPresets.at(armour));

				CImGui::TableSetColumnIndex(2);
				drawPresetGroupEditor_AssignedPresetsTableCell(helmPreset, armour, ArmourPiece::AP_HELM, 2, rowHeight);
				CImGui::TableSetColumnIndex(3);
				drawPresetGroupEditor_AssignedPresetsTableCell(bodyPreset, armour, ArmourPiece::AP_BODY, 3, rowHeight);
				CImGui::TableSetColumnIndex(4);
				drawPresetGroupEditor_AssignedPresetsTableCell(armsPreset, armour, ArmourPiece::AP_ARMS, 4, rowHeight);
				CImGui::TableSetColumnIndex(5);
				drawPresetGroupEditor_AssignedPresetsTableCell(coilPreset, armour, ArmourPiece::AP_COIL, 5, rowHeight);
                CImGui::TableSetColumnIndex(6);
                drawPresetGroupEditor_AssignedPresetsTableCell(legsPreset, armour, ArmourPiece::AP_LEGS, 6, rowHeight);
            }

            CImGui::EndTable();

            CImGui::PopStyleVar();
        }

    }

    void EditorTab::drawPresetGroupEditor_AssignedPresetsTableCell(Preset* preset, ArmourSet armour, ArmourPiece piece, int column, float rowHeight) {
        constexpr ImVec4 hasPresetColor{ 0.3f, 0.6f, 0.3f, 0.35f };
        constexpr ImVec4 disabledColor{ 0.8f, 0.3f, 0.3f, 0.15f };
        constexpr float armourSexMarkerOffsetBefore = 5.0f;
        constexpr float armourSexMarkerOffset = 17.5f;

        ArmourID armourID = ArmourList::getArmourIdFromSet(armour);
        bool isNotAssignable = piece != ArmourPiece::AP_SET && armourID.getPiece(piece).empty();

        if (preset) {
            std::string bodyArmourSexMarkSymbol = preset->female ? WS_FONT_FEMALE : WS_FONT_MALE;
            ImVec4 bodyArmourSexMarkerCol = preset->female ? ImVec4(0.76f, 0.50f, 0.24f, 1.0f) : ImVec4(0.50f, 0.70f, 0.33f, 1.0f);

            ImVec2 bodyArmourSexMarkerSize = CImGui::CalcTextSize(bodyArmourSexMarkSymbol.c_str());
            ImVec2 bodyArmourSexMarkerPos;
            bodyArmourSexMarkerPos.x = CImGui::GetCursorScreenPos().x + armourSexMarkerOffsetBefore;
            bodyArmourSexMarkerPos.y = CImGui::GetCursorScreenPos().y + (rowHeight - bodyArmourSexMarkerSize.y) * 0.5f;
            CImGui::PushFont(wsSymbolFont, FONT_SIZE_DEFAULT_WILDS_SYMBOLS);
            CImGui::GetWindowDrawList()->AddText(bodyArmourSexMarkerPos, CImGui::GetColorU32(bodyArmourSexMarkerCol), bodyArmourSexMarkSymbol.c_str());
            CImGui::PopFont();

            CImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, CImGui::GetColorU32(hasPresetColor), column);
            ImVec2 bodyTextPos;
            bodyTextPos.x = bodyArmourSexMarkerPos.x + armourSexMarkerOffset;
            bodyTextPos.y = CImGui::GetCursorScreenPos().y + (rowHeight - CImGui::GetTextLineHeight()) * 0.5f;
            CImGui::GetWindowDrawList()->AddText(bodyTextPos, CImGui::GetColorU32(ImGuiCol_Text), preset->name.c_str());
        }

        if (isNotAssignable) {
            // Disable selectable and colour cell bg red
			CImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, CImGui::GetColorU32(disabledColor), column);
            CImGui::BeginDisabled();
        }

        std::string bodySelectableId = std::format("##{}_{}_{}", armour.name, armour.female, std::to_string(static_cast<int>(piece)));
        if (CImGui::Selectable(bodySelectableId.c_str(), false, ImGuiSelectableFlags_None, ImVec2(0.0f, rowHeight))) {
            openAssignPresetPanel(armour, piece);
        }
        bool showTooltip = preset && CImGui::IsItemHovered();
        if (isNotAssignable) CImGui::EndDisabled();

        std::string popupID = std::format("AssignedPresetContextMenu_{}_{}", column, CImGui::TableGetRowIndex());
        if (CImGui::BeginPopupContextItem(popupID.c_str(), ImGuiPopupFlags_MouseButtonRight)) {

            // Create Preset
            const std::string createItemStr = "Create New Preset";
            std::string createErr = "";
			bool disableCreateContextMenu = isNotAssignable;
			if (disableCreateContextMenu) CImGui::BeginDisabled();
            if (CImGui::MenuItem(createItemStr.c_str())) {
                createPresetPanel.openNew("Create Preset", "CreatePresetPanel", dataManager, wsSymbolFont, wsArmourFont);
                createPresetPanel.get()->focus();
                createPresetPanel.get()->onCancel([this]() { createPresetPanel.close(); });
                createPresetPanel.get()->onCreate([this, armour, piece](const Preset& preset) {
                    dataManager.addPreset(preset);
					assignPreset(armour, piece, preset.uuid);
                    createPresetPanel.close();
                });
            }
			if (disableCreateContextMenu) CImGui::EndDisabled();


            // Edit Preset
			const std::string presetItemStr = preset ? std::format("Edit Preset \"{}\"", preset->name) : "Edit Preset";
            std::string editErr = "";
            bool disableEditContextMenu = isNotAssignable || preset == nullptr || !canSavePresetGroup(editErr);
			if (disableEditContextMenu) CImGui::BeginDisabled();
            if (CImGui::MenuItem(presetItemStr.c_str())) {
				const auto navigateToPresetEdit = [this, preset]() { 
                    closePopouts();
                    editPresetDeferred(preset);
                };

                bool hasChanges = *openObject.ptrBefore.presetGroup != *openObject.ptrAfter.presetGroup;
                if (hasChanges) {
                    navWarnUnsavedPanel.openNew("Warning: Unsaved Changes", "SaveConfirmInfoPanel", "You have unsaved changes - save now?", "Save", "Revert");
                    navWarnUnsavedPanel.get()->focus();
                    navWarnUnsavedPanel.get()->onCancel(navigateToPresetEdit);
                    navWarnUnsavedPanel.get()->onOk([this, navigateToPresetEdit]() { INVOKE_REQUIRED_CALLBACK(savePresetGroupCb); navigateToPresetEdit(); });
                }
                else {
                    navigateToPresetEdit();
                }
            }
            if (disableEditContextMenu) CImGui::EndDisabled();

            CImGui::EndPopup();
        } else if (showTooltip) {
            CImGui::SetTooltip(preset->name.c_str());
		}
    }

    bool EditorTab::canSavePresetGroup(std::string& errMsg) const {
        assert(openObject.type == EditableObject::ObjectType::PRESET_GROUP && openObject.ptrAfter.presetGroup != nullptr);

        PresetGroup& groupBefore = *openObject.ptrBefore.presetGroup;
        PresetGroup& group = *openObject.ptrAfter.presetGroup;

        if (group.name.empty()) {
            errMsg = "Please provide a group name";
            return false;
        }
        else if (group.name != groupBefore.name && dataManager.presetGroupExists(group.name)) {
            errMsg = "Group name already taken";
            return false;
        }
        return true;
    }

    void EditorTab::assignPreset(ArmourSet armourSet, ArmourPiece piece, std::string uuid) {
        const Preset* presetToAssign = uuid.empty() ? nullptr : dataManager.getPresetByUUID(uuid);
        std::unordered_map<ArmourSet, std::string>* targetMap = nullptr;

        switch (piece) {
        case ArmourPiece::AP_SET:  targetMap = &openObject.ptrAfter.presetGroup->setPresets;  break;
        case ArmourPiece::AP_HELM: targetMap = &openObject.ptrAfter.presetGroup->helmPresets; break;
        case ArmourPiece::AP_BODY: targetMap = &openObject.ptrAfter.presetGroup->bodyPresets; break;
        case ArmourPiece::AP_ARMS: targetMap = &openObject.ptrAfter.presetGroup->armsPresets; break;
        case ArmourPiece::AP_COIL: targetMap = &openObject.ptrAfter.presetGroup->coilPresets; break;
        case ArmourPiece::AP_LEGS: targetMap = &openObject.ptrAfter.presetGroup->legsPresets; break;
        }

        if (targetMap) {
            if (presetToAssign) (*targetMap)[armourSet] = uuid;
            else                targetMap->erase(armourSet);
        }
	}

    void EditorTab::drawPresetEditor() {
        assert(openObject.type == EditableObject::ObjectType::PRESET && openObject.ptrAfter.preset != nullptr);
        const Preset& presetBefore = *openObject.ptrBefore.preset;

		const Preset* currentPreviewPreset = dataManager.getPreviewedPreset();
		bool needsPreviewBefore = currentPreviewPreset != nullptr && currentPreviewPreset->uuid == openObject.ptrAfter.preset->uuid;
        bool needsPreviewCurrent = needsPreviewBefore;
        bool drawTabContent = drawStickyNavigationWidget(
            std::format("Editing Preset \"{}\"", presetBefore.name),
            &needsPreviewCurrent,
            // Callback funcs
            nullptr,  // TODO: Func that checks if required armour equipped
            [this]() { return *openObject.ptrBefore.preset != *openObject.ptrAfter.preset; },
            [this]() { openObject.revertPreset(); initializePresetBuffers(openObject.ptrAfter.preset); },
            [this](std::string& errMsg) { return canSavePreset(errMsg); },
            savePresetCb);

        if (needsPreviewBefore != needsPreviewCurrent) {
            dataManager.previewPreset(needsPreviewCurrent ? openObject.ptrAfter.preset : nullptr);
        }

        if (!drawTabContent) return;

        if (CImGui::Button("Copy Existing Preset", ImVec2(CImGui::GetContentRegionAvail().x, 0))) {
            openCopyPresetPanel();
        }
        
        CImGui::Spacing();
        CImGui::Separator();
        CImGui::Spacing();
        CImGui::Spacing();
        CImGui::Spacing();

        if (CImGui::BeginTabBar("PresetEditorTabs"))
        {
            Preset** p = &openObject.ptrAfter.preset;
            ArmourID id = ArmourList::getArmourIdFromSet((**p).armour);

            if (CImGui::BeginTabItem("Properties")) {
                CImGui::Spacing();
                drawPresetEditor_Properties(p);
                CImGui::EndTabItem();
            }
            if (CImGui::IsItemClicked()) selectBonePanel.close();

            if (CImGui::BeginTabItem("Base Armature")) {
                CImGui::Spacing();
                drawPresetEditor_BoneModifiers(p, ArmourPiece::AP_SET);
                CImGui::EndTabItem();
            }
            if (CImGui::IsItemClicked()) selectBonePanel.close();

            drawPresetEditor_ArmourTab("Head",  ArmourPiece::AP_HELM, "helmet",     p, id);
            drawPresetEditor_ArmourTab("Body",  ArmourPiece::AP_BODY, "chestpiece", p, id);
            drawPresetEditor_ArmourTab("Arms",  ArmourPiece::AP_ARMS, "vambraces",  p, id);
            drawPresetEditor_ArmourTab("Waist", ArmourPiece::AP_COIL, "coil",       p, id);
            drawPresetEditor_ArmourTab("Legs",  ArmourPiece::AP_LEGS, "greaves",    p, id);

            if (CImGui::BeginTabItem("Parts")) {
                CImGui::Spacing();
                drawPresetEditor_PartVisibilities(p);
                CImGui::EndTabItem();
            }
            if (CImGui::IsItemClicked()) selectBonePanel.close();

            CImGui::EndTabBar();
        }
    }

    bool EditorTab::drawPresetEditor_ArmourTab(
        const char* label,
        ArmourPiece piece,
        const char* englishName,
        Preset** preset,
        ArmourID id
    ) {
        constexpr auto beginFauxDisableTabItem = []() {
            ImVec4 off = *CImGui::GetStyleColorVec4(ImGuiCol_TextDisabled);
            CImGui::PushStyleColor(ImGuiCol_Text, off);
            CImGui::PushStyleVar(ImGuiStyleVar_Alpha, CImGui::GetStyle().Alpha * 0.7f);
         };

        constexpr auto endFauxDisableTabItem = []() {
            CImGui::PopStyleVar();
            CImGui::PopStyleColor();
        };

        const bool disabled = !id.hasPiece(piece);
        if (disabled) beginFauxDisableTabItem();

        bool open = false;
        if (CImGui::BeginTabItem(label)) {
            if (disabled) endFauxDisableTabItem();
            CImGui::Spacing();
            drawPresetEditor_BoneModifiers(preset, piece);
            CImGui::EndTabItem();
            open = true;
        }
        else if (disabled) {
            endFauxDisableTabItem();
            CImGui::SetItemTooltip(
                std::format(
                    "Note: This preset's armour set: \"{}\" ({}) has no {}",
                    (**preset).armour.name,
                    (**preset).armour.female ? "F" : "M",
                    englishName
                ).c_str()
            );
        }

        if (CImGui::IsItemClicked())
            selectBonePanel.close();

        return open;
    }

    void EditorTab::drawPresetEditor_Properties(Preset** preset) {
        CImGui::BeginChild("PresetProperties");
        CImGui::InputText(" Name ", presetNameBuffer, IM_ARRAYSIZE(presetNameBuffer));
        (**preset).name = std::string{ presetNameBuffer };

        CImGui::Spacing();
        CImGui::InputText(" Bundle ", presetBundleBuffer, IM_ARRAYSIZE(presetBundleBuffer));
        (**preset).bundle = std::string{ presetBundleBuffer };
        CImGui::SetItemTooltip("Enables sorting similar presets under one title");

        CImGui::Spacing();
        std::string sexComboValue = (**preset).female ? "Female" : "Male";
        if (CImGui::BeginCombo(" Sex ", sexComboValue.c_str())) {
            if (CImGui::Selectable("Male")) {
                (**preset).female = false;
            }
            if (CImGui::Selectable("Female")) {
                (**preset).female = true;
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

        drawArmourSetName((**preset).armour, 10.0f, 17.5f);

        CImGui::Spacing();

        static char filterBuffer[128] = "";
        std::string filterStr{ filterBuffer };

        drawArmourList((**preset), filterStr);

        CImGui::PushItemWidth(-1);
        CImGui::InputTextWithHint("##Search", "Search...", filterBuffer, IM_ARRAYSIZE(filterBuffer));
        CImGui::PopItemWidth();

        CImGui::EndChild();
    }

    void EditorTab::drawPresetEditor_BoneModifiers(Preset** preset, ArmourPiece piece) {
        static bool compactMode = true;
        static bool categorizeBones = true;

        // Get pointers to the current info (for synchronization reasons)
		bool* useSymmetry = nullptr;
        switch (piece) {
		case ArmourPiece::AP_SET:  useSymmetry = &(**preset).set.useSymmetry; break;
		case ArmourPiece::AP_HELM: useSymmetry = &(**preset).helm.useSymmetry; break;
		case ArmourPiece::AP_BODY: useSymmetry = &(**preset).body.useSymmetry; break;
		case ArmourPiece::AP_ARMS: useSymmetry = &(**preset).arms.useSymmetry; break;
		case ArmourPiece::AP_COIL: useSymmetry = &(**preset).coil.useSymmetry; break;
		case ArmourPiece::AP_LEGS: useSymmetry = &(**preset).legs.useSymmetry; break;
        }
		float* modLimit = nullptr;
        switch (piece) {
        case ArmourPiece::AP_SET:  modLimit = &(**preset).set.modLimit; break;
        case ArmourPiece::AP_HELM: modLimit = &(**preset).helm.modLimit; break;
        case ArmourPiece::AP_BODY: modLimit = &(**preset).body.modLimit; break;
        case ArmourPiece::AP_ARMS: modLimit = &(**preset).arms.modLimit; break;
        case ArmourPiece::AP_COIL: modLimit = &(**preset).coil.modLimit; break;
        case ArmourPiece::AP_LEGS: modLimit = &(**preset).legs.modLimit; break;
        }
        BoneModifierMap* boneModifiers = nullptr;
        switch (piece) {
        case ArmourPiece::AP_SET:  boneModifiers = &(**preset).set.modifiers; break;
        case ArmourPiece::AP_HELM: boneModifiers = &(**preset).helm.modifiers; break;
        case ArmourPiece::AP_BODY: boneModifiers = &(**preset).body.modifiers; break;
        case ArmourPiece::AP_ARMS: boneModifiers = &(**preset).arms.modifiers; break;
        case ArmourPiece::AP_COIL: boneModifiers = &(**preset).coil.modifiers; break;
        case ArmourPiece::AP_LEGS: boneModifiers = &(**preset).legs.modifiers; break;
        }

        if (boneModifiers == nullptr || useSymmetry == nullptr || modLimit == nullptr) 
            return;

        CImGui::BeginChild("StickyBoneControlsWidget", ImVec2(0, 150.0f), 0, ImGuiWindowFlags_NoScrollbar);
        switch (piece) {
		case ArmourPiece::AP_SET:  setBoneInfoWidget.draw(&compactMode, &categorizeBones, useSymmetry, modLimit); break;
		case ArmourPiece::AP_HELM: helmBoneInfoWidget.draw(&compactMode, &categorizeBones, useSymmetry, modLimit); break;
		case ArmourPiece::AP_BODY: bodyBoneInfoWidget.draw(&compactMode, &categorizeBones, useSymmetry, modLimit); break;
		case ArmourPiece::AP_ARMS: armsBoneInfoWidget.draw(&compactMode, &categorizeBones, useSymmetry, modLimit); break;
		case ArmourPiece::AP_COIL: coilBoneInfoWidget.draw(&compactMode, &categorizeBones, useSymmetry, modLimit); break;
		case ArmourPiece::AP_LEGS: legsBoneInfoWidget.draw(&compactMode, &categorizeBones, useSymmetry, modLimit); break;
        }
        CImGui::EndChild();

        CImGui::BeginChild("BoneModifiersListBody");

        if (boneModifiers->size() == 0) {
            CImGui::Spacing();
            CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
            constexpr char const* noBoneStr = "No modified bones found - add some above!";
            preAlignCellContentHorizontal(noBoneStr);
            CImGui::Text(noBoneStr);
            CImGui::PopStyleColor();
        }
        else {
            auto categorizedModifiers = getProcessedModifiers(*boneModifiers, categorizeBones, *useSymmetry);

            for (auto& [categoryName, sortableModifiers] : categorizedModifiers) {
                bool display = true;
                if (categorizeBones) display = CImGui::CollapsingHeader(categoryName.c_str(), ImGuiTreeNodeFlags_SpanFullWidth);
                if (display) {
                    ArmourSetWithCharacterSex armourWithSex{ (**preset).armour, (**preset).female };

					bool displayBoneWarnings = piece != ArmourPiece::AP_SET; // Only display warnings for specific pieces, not base armature
                    if (compactMode) {
                        drawCompactBoneModifierTable(categoryName, armourWithSex, piece, sortableModifiers, *boneModifiers, *modLimit, displayBoneWarnings);
                    }
                    else {
                        drawBoneModifierTable(categoryName, armourWithSex, piece, sortableModifiers, *boneModifiers, *modLimit, displayBoneWarnings);
                    }
                }
            }
        }

        CImGui::EndChild();
    }

    std::unordered_map<std::string, std::vector<SortableBoneModifier>> EditorTab::getProcessedModifiers(
        BoneModifierMap& modifiers,
        bool categorizeBones,
        bool useSymmetry
    ) {
        // Categorize bones & Get symmetry Proxies
        std::unordered_set<std::string> processedBones;
        std::unordered_map<std::string, std::vector<SortableBoneModifier>> categorizedModifiers;
        if (categorizeBones) {
            for (auto& [boneName, modifier] : modifiers) {
                SortableBoneModifier sortableModifier{ boneName, false, &modifier, nullptr, boneName, "" };

                bool alreadyProcessed = processedBones.find(boneName) != processedBones.end();
                if (alreadyProcessed) continue;

                if (useSymmetry) {
                    std::string complement;
                    sortableModifier = getSymmetryProxyModifier(boneName, modifiers, &complement);
                    processedBones.insert(complement);
                }

                processedBones.insert(boneName);
                categorizedModifiers[getCommonBoneCategory(boneName)].emplace_back(sortableModifier);
            }
        }
        else {
            for (auto& [boneName, modifier] : modifiers) {
                SortableBoneModifier sortableModifier{ boneName, false, &modifier, nullptr, boneName, "" };

                bool alreadyProcessed = processedBones.find(boneName) != processedBones.end();
                if (alreadyProcessed) continue;

                if (useSymmetry) {
                    std::string complement;
                    sortableModifier = getSymmetryProxyModifier(boneName, modifiers, &complement);
                    processedBones.insert(complement);
                }

                processedBones.insert(boneName);
                categorizedModifiers[getCommonBoneCategory("Bones")].emplace_back(sortableModifier);
            }
        }

        return categorizedModifiers;
    }

    void EditorTab::drawCompactBoneModifierTable(
        std::string tableName,
        ArmourSetWithCharacterSex armourWithSex,
        ArmourPiece piece,
        std::vector<SortableBoneModifier>& sortableModifiers,
        BoneModifierMap& modifiers,
        float modLimit,
        bool enableWarnings
    ) {
        constexpr float deleteButtonScale = 1.2f;
        constexpr float linkButtonScale = 1.0f;
        constexpr float sliderHeight = 66.0f;
        constexpr float sliderWidth = 22.0f;
        constexpr float tableVpad = 2.5f;

        constexpr ImGuiTableFlags boneModTableFlags =
            ImGuiTableFlags_PadOuterX
            | ImGuiTableFlags_Sortable;
        CImGui::BeginTable(("##BoneModifierList_" + tableName).c_str(), 5, boneModTableFlags);

        constexpr ImGuiTableColumnFlags stretchSortFlags =
            ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthStretch;
        constexpr ImGuiTableColumnFlags fixedNoSortFlags =
            ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthFixed;

        CImGui::TableSetupColumn("", fixedNoSortFlags, 0.0f);
        CImGui::TableSetupColumn("Bone", stretchSortFlags, 0.0f);
        CImGui::TableSetupColumn("Scale", fixedNoSortFlags, sliderWidth * 3.0f - 5.0f);
        CImGui::TableSetupColumn("Position", fixedNoSortFlags, sliderWidth * 3.0f - 5.0f);
        CImGui::TableSetupColumn("Rotation", fixedNoSortFlags, sliderWidth * 3.0f - 5.0f);
        CImGui::TableSetupScrollFreeze(0, 1);
        CImGui::TableHeadersRow();

        CImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(LIST_PADDING.x, tableVpad));

        float contentRegionWidth = CImGui::GetContentRegionAvail().x;

        // Sort - this is kinda horrible.
        static bool sortDirAscending = true;
        static bool sort = false;

        if (sort) {
            std::sort(sortableModifiers.begin(), sortableModifiers.end(), [&](const SortableBoneModifier& a, const SortableBoneModifier& b) {
                std::string lowa = toLower(a.name); std::string lowb = toLower(b.name);
                return sortDirAscending ? lowa < lowb : lowa > lowb;
                });
        }

        if (ImGuiTableSortSpecs* sort_specs = CImGui::TableGetSortSpecs()) {
            if (sort_specs->SpecsDirty && sort_specs->SpecsCount > 0) {
                const ImGuiTableColumnSortSpecs& sort_spec = sort_specs->Specs[0];
                sortDirAscending = sort_spec.SortDirection == ImGuiSortDirection_Ascending;

                switch (sort_spec.ColumnIndex)
                {
                case 1: sort = true;
                }

                sort_specs->SpecsDirty = false;
            }
        }

        std::vector<const SortableBoneModifier*> bonesToDelete{};
		size_t i = 0;
        for (const SortableBoneModifier& bone : sortableModifiers) {
            // Display warning if any of the bones aren't in the bone cache
            bool hasWarning = enableWarnings && !dataManager.boneCache().boneExists(armourWithSex, piece, bone.boneName);
            std::string boneKey = bone.name + (bone.isSymmetryProxy ? "LRProxy" : "Unique");

            ImU32 rowCol = i % 2 == 0 ? CImGui::GetColorU32(ImGuiCol_TableRowBg) : CImGui::GetColorU32(ImGuiCol_TableRowBgAlt);
            if (hasWarning) rowCol = CImGui::GetColorU32(ImVec4(1.0f, 0.0f, 0.0f, 0.5f));

            CImGui::TableNextRow();
            CImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, rowCol);
            CImGui::TableNextColumn();
            CImGui::PopStyleVar();

            CImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
            CImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.1f, 0.1f, 1.0f));
            CImGui::SetCursorPosY(CImGui::GetCursorPosY() + (sliderHeight + tableVpad - CImGui::GetFontSize() * deleteButtonScale) * 0.5f);
            if (ImDeleteButton(("##del_" + boneKey).c_str(), deleteButtonScale)) {
                bonesToDelete.push_back(&bone);
            }
            CImGui::PopStyleColor(2);

            CImGui::TableNextColumn();
            const std::string boneNameStr = bone.isSymmetryProxy ? (bone.name + " (L & R)") : bone.name;
            CImGui::SetCursorPosY(CImGui::GetCursorPosY() + (sliderHeight + tableVpad - CImGui::CalcTextSize(boneNameStr.c_str()).y) * 0.5f);
            CImGui::Text(boneNameStr.c_str());

            CImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 4));

            const ImVec2 size{ sliderWidth, sliderHeight };
            CImGui::TableNextColumn();
            drawCompactBoneModifierGroup(boneKey + "_scale_", bone.modifier->scale, modLimit, size, "Scale ");
            CImGui::TableNextColumn();
            drawCompactBoneModifierGroup(boneKey + "_position_", bone.modifier->position, modLimit, size, "Pos ");
            CImGui::TableNextColumn();
            glm::vec3 rotation = bone.modifier->getRotation();
            drawCompactBoneModifierGroup(boneKey + "_rotation_", rotation, modLimit, size, "Rot ");
            bone.modifier->setRotation(rotation);

            if (bone.isSymmetryProxy) *bone.reflectedModifier = bone.modifier->reflect();

            i++;
        }

        for (const SortableBoneModifier* bone : bonesToDelete) {
            if (bone->isSymmetryProxy) {
                modifiers.erase(bone->boneName);
                modifiers.erase(bone->reflectedBoneName);
            }
            else {
                modifiers.erase(bone->boneName);
            }
        }

        CImGui::PopStyleVar();
        CImGui::EndTable();
    }

    void EditorTab::drawBoneModifierTable(
        std::string tableName,
        ArmourSetWithCharacterSex armourWithSex,
        ArmourPiece piece,
        std::vector<SortableBoneModifier>& sortableModifiers,
        BoneModifierMap& modifiers,
        float modLimit,
        bool enableWarnings
    ) {
        constexpr float deleteButtonScale = 1.2f;
        constexpr float linkButtonScale = 1.0f;
        constexpr float sliderWidth = 80.0f;
        constexpr float sliderSpeed = 0.001f;
        constexpr float tableVpad = 2.5f;

        constexpr ImGuiTableFlags boneModTableFlags =
            ImGuiTableFlags_PadOuterX
            | ImGuiTableFlags_Sortable;
        CImGui::BeginTable("##BoneModifierList", 4, boneModTableFlags);

        constexpr ImGuiTableColumnFlags stretchSortFlags =
            ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthStretch;
        constexpr ImGuiTableColumnFlags fixedNoSortFlags =
            ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthFixed;

        CImGui::TableSetupColumn("", fixedNoSortFlags, 0.0f);
        CImGui::TableSetupColumn("Bone", stretchSortFlags, 0.0f);
        CImGui::TableSetupColumn("", fixedNoSortFlags, CImGui::CalcTextSize("Position").x + 4.0f);
        CImGui::TableSetupColumn("", fixedNoSortFlags, sliderWidth * 3.0f + 2.0f);
        CImGui::TableHeadersRow();

        CImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(LIST_PADDING.x, tableVpad));

        float contentRegionWidth = CImGui::GetContentRegionAvail().x;

        // Sort - this is kinda horrible.
        static bool sortDirAscending = true;
        static bool sort = false;

        if (sort) {
            std::sort(sortableModifiers.begin(), sortableModifiers.end(), [&](const SortableBoneModifier& a, const SortableBoneModifier& b) {
                std::string lowa = toLower(a.name); std::string lowb = toLower(b.name);
                return sortDirAscending ? lowa < lowb : lowa > lowb;
                });
        }

        if (ImGuiTableSortSpecs* sort_specs = CImGui::TableGetSortSpecs()) {
            if (sort_specs->SpecsDirty && sort_specs->SpecsCount > 0) {
                const ImGuiTableColumnSortSpecs& sort_spec = sort_specs->Specs[0];
                sortDirAscending = sort_spec.SortDirection == ImGuiSortDirection_Ascending;

                switch (sort_spec.ColumnIndex)
                {
                case 1: sort = true;
                }

                sort_specs->SpecsDirty = false;
            }
        }

        std::vector<const SortableBoneModifier*> bonesToDelete{};
        size_t i = 0;
        for (const SortableBoneModifier& bone : sortableModifiers) {
            // Display warning if any of the bones aren't in the bone cache
            bool hasWarning = enableWarnings && !dataManager.boneCache().boneExists(armourWithSex, piece, bone.boneName);
            std::string boneKey = bone.name + (bone.isSymmetryProxy ? "LRProxy" : "Unique");

            ImU32 rowCol = i % 2 == 0 ? CImGui::GetColorU32(ImGuiCol_TableRowBg) : CImGui::GetColorU32(ImGuiCol_TableRowBgAlt);
            if (hasWarning) rowCol = CImGui::GetColorU32(ImVec4(1.0f, 0.0f, 0.0f, 0.5f));

            // Top Row
            CImGui::TableNextRow();
            CImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, rowCol);

            CImGui::TableSetColumnIndex(2);
            CImGui::SetCursorPosY(CImGui::GetCursorPosY() + (CImGui::GetFrameHeight() - CImGui::GetTextLineHeight()) * 0.5f);
            CImGui::Text("Scale");
            CImGui::TableSetColumnIndex(3);
            CImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));
            drawBoneModifierGroup(boneKey + "_scale_", bone.modifier->scale, modLimit, sliderWidth, sliderSpeed);
            CImGui::PopStyleVar();

            // Middle Row
            CImGui::TableNextRow();
            CImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, rowCol);

            CImGui::TableSetColumnIndex(0);
            CImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
            CImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.1f, 0.1f, 1.0f));
            CImGui::SetCursorPosY(CImGui::GetCursorPosY() + (CImGui::GetFrameHeight() - CImGui::GetFontSize() * deleteButtonScale) * 0.5f);
            if (ImDeleteButton(("##del_" + boneKey).c_str(), deleteButtonScale)) {
                bonesToDelete.push_back(&bone);
            }
            CImGui::PopStyleColor(2);

            CImGui::TableSetColumnIndex(1);
            const std::string boneNameStr = bone.isSymmetryProxy ? (bone.name + " (L & R)") : bone.name;
            CImGui::SetCursorPosY(CImGui::GetCursorPosY() + (CImGui::GetFrameHeight() - CImGui::GetTextLineHeight()) * 0.5f);
            CImGui::Text(boneNameStr.c_str());

            CImGui::TableSetColumnIndex(2);
            CImGui::SetCursorPosY(CImGui::GetCursorPosY() + (CImGui::GetFrameHeight() - CImGui::GetTextLineHeight()) * 0.5f);
            CImGui::Text("Position");
            CImGui::TableSetColumnIndex(3);
            CImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));
            drawBoneModifierGroup(boneKey + "_position_", bone.modifier->position, modLimit, sliderWidth, sliderSpeed);
            CImGui::PopStyleVar();

            // Bottom Row
            CImGui::TableNextRow();
            CImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, rowCol);

            CImGui::TableSetColumnIndex(2);
            CImGui::SetCursorPosY(CImGui::GetCursorPosY() + (CImGui::GetFrameHeight() - CImGui::GetTextLineHeight()) * 0.5f);
            CImGui::Text("Rotation");
            CImGui::TableSetColumnIndex(3);
            CImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));
            glm::vec3 rotation = bone.modifier->getRotation();
            drawBoneModifierGroup(boneKey + "_rotation_", rotation, modLimit, sliderWidth, sliderSpeed);
            bone.modifier->setRotation(rotation);
            CImGui::PopStyleVar();

            if (bone.isSymmetryProxy) *bone.reflectedModifier = bone.modifier->reflect();
            i++;
        }

        for (const SortableBoneModifier* bone : bonesToDelete) {
            if (bone->isSymmetryProxy) {
                modifiers.erase(bone->boneName);
                modifiers.erase(bone->reflectedBoneName);
            }
            else {
                modifiers.erase(bone->boneName);
            }
        }

        CImGui::PopStyleVar();
        CImGui::EndTable();
    }

    void EditorTab::drawCompactBoneModifierGroup(const std::string& strID, glm::vec3& group, float limit, ImVec2 size, std::string fmtPrefix) {
        bool changedX = ImBoneSlider(("##" + strID + "x").c_str(), size, &group.x, limit, "", (fmtPrefix + "x: %.3f").c_str());
        CImGui::SameLine();
        bool changedY = ImBoneSlider(("##" + strID + "y").c_str(), size, &group.y, limit, "", (fmtPrefix + "y: %.3f").c_str());
        CImGui::SameLine();
        bool changedZ = ImBoneSlider(("##" + strID + "z").c_str(), size, &group.z, limit, "", (fmtPrefix + "z: %.3f").c_str());

        if (changedX && CImGui::IsKeyDown(ImGuiMod_Shift)) { group.y = group.x; group.z = group.x; }
        if (changedY && CImGui::IsKeyDown(ImGuiMod_Shift)) { group.x = group.y; group.z = group.y; }
        if (changedZ && CImGui::IsKeyDown(ImGuiMod_Shift)) { group.x = group.z; group.y = group.z; }
    }

    void EditorTab::drawBoneModifierGroup(const std::string& strID, glm::vec3& group, float limit, float width, float speed) {
        bool changedX = ImBoneSliderH(("##" + strID + "x").c_str(), width, &group.x, speed, limit, "x: %.3f");
        CImGui::SameLine();
        bool changedY = ImBoneSliderH(("##" + strID + "y").c_str(), width, &group.y, speed, limit, "y: %.3f");
        CImGui::SameLine();
        bool changedZ = ImBoneSliderH(("##" + strID + "z").c_str(), width, &group.z, speed, limit, "z: %.3f");

        if (changedX && CImGui::IsKeyDown(ImGuiMod_Shift)) { group.y = group.x; group.z = group.x; }
        if (changedY && CImGui::IsKeyDown(ImGuiMod_Shift)) { group.x = group.y; group.z = group.y; }
        if (changedZ && CImGui::IsKeyDown(ImGuiMod_Shift)) { group.x = group.z; group.y = group.z; }
    }

    void EditorTab::drawPresetEditor_PartVisibilities(Preset** preset) {
		CImGui::Toggle(" Hide Slinger ", &(**preset).hideSlinger, ImGuiToggleFlags_Animated);
        CImGui::SameLine();
        CImGui::Toggle(" Hide Weapon ", &(**preset).hideWeapon, ImGuiToggleFlags_Animated);
        CImGui::SameLine();

        bool disableHidePart = (**preset).armour == ArmourList::DefaultArmourSet();
        if (disableHidePart) CImGui::BeginDisabled();
        if (CImGui::Button("Show/Hide Part", ImVec2(CImGui::GetContentRegionAvail().x, 0))) {
            openPartOverridePanel();
        }
		if (disableHidePart) CImGui::EndDisabled();
        if (disableHidePart) CImGui::SetItemTooltip("Cannot show/hide parts when the preset's armour set is set to \"Any\"");

        constexpr const char* hintText = "Note: Shown/Hidden parts won't reset to default visibility until reloaded/re-equipped.";
        const float hintWidth = CImGui::CalcTextSize(hintText).x;
        CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
        CImGui::SetCursorPosX(CImGui::GetCursorPosX() + (CImGui::GetContentRegionAvail().x - hintWidth) * 0.5f);
        CImGui::SetCursorPosY(CImGui::GetCursorPosY() - 10.0f);
        CImGui::Text(hintText);
        CImGui::PopStyleColor();

        CImGui::Spacing();
        CImGui::Separator();

		bool hasHelmParts = (**preset).helm.hasPartOverrides();
		bool hasBodyParts = (**preset).body.hasPartOverrides();
		bool hasArmsParts = (**preset).arms.hasPartOverrides();
		bool hasCoilParts = (**preset).coil.hasPartOverrides();
		bool hasLegsParts = (**preset).legs.hasPartOverrides();
		bool hasAnyParts = hasHelmParts || hasBodyParts || hasArmsParts || hasCoilParts || hasLegsParts;
        if (!hasAnyParts) {
            CImGui::Spacing();
            CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
            constexpr char const* noBoneStr = "No Parts Currently Being Shown/Hidden.";
            preAlignCellContentHorizontal(noBoneStr);
            CImGui::Text(noBoneStr);
            CImGui::PopStyleColor();
        }
        else {
            CImGui::Spacing();
            CImGui::BeginChild("PartVisibilitiesTable");
            if (hasHelmParts) {
                if (CImGui::CollapsingHeader("Helm Parts", ImGuiTreeNodeFlags_SpanFullWidth)) {
                    drawPresetEditor_PartVisibilitiesTable("Helm Parts", (**preset).helm.partOverrides);
                }
            }
            if (hasBodyParts) {
                if (CImGui::CollapsingHeader("Body Parts", ImGuiTreeNodeFlags_SpanFullWidth)) {
                    drawPresetEditor_PartVisibilitiesTable("Body Parts", (**preset).body.partOverrides);
				}
            }
            if (hasArmsParts) {
                if (CImGui::CollapsingHeader("Arms Parts", ImGuiTreeNodeFlags_SpanFullWidth)) {
                    drawPresetEditor_PartVisibilitiesTable("Arms Parts", (**preset).arms.partOverrides);
                }
			}
            if (hasCoilParts) {
                if (CImGui::CollapsingHeader("Coil Parts", ImGuiTreeNodeFlags_SpanFullWidth)) {
                    drawPresetEditor_PartVisibilitiesTable("Coil Parts", (**preset).coil.partOverrides);
                }
            }
            if (hasLegsParts) {
                if (CImGui::CollapsingHeader("Legs Parts", ImGuiTreeNodeFlags_SpanFullWidth)) {
                    drawPresetEditor_PartVisibilitiesTable("Legs Parts", (**preset).legs.partOverrides);
                }
			}
            CImGui::EndChild();
        }

    }

    void EditorTab::drawPresetEditor_PartVisibilitiesTable(std::string tableName, std::set<OverrideMeshPart>& parts) {
        std::vector<OverrideMeshPart> overrideParts(parts.begin(), parts.end());

        constexpr float deleteButtonScale = 1.2f;
        constexpr float linkButtonScale = 1.0f;
        constexpr float tableVpad = 5.0f;
		constexpr float selectableHeight = 70.0f;
        constexpr float alignAdjust = 10.0f;

        constexpr ImGuiTableFlags boneModTableFlags =
            ImGuiTableFlags_RowBg
            | ImGuiTableFlags_PadOuterX
            | ImGuiTableFlags_Sortable;
        CImGui::BeginTable("##PartRemoverList", 3, boneModTableFlags);

        constexpr ImGuiTableColumnFlags stretchSortFlags =
            ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthStretch;
        constexpr ImGuiTableColumnFlags fixedNoSortFlags =
            ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthFixed;

        CImGui::TableSetupColumn("", fixedNoSortFlags, 0.0f);
        CImGui::TableSetupColumn("Part", stretchSortFlags, 0.0f);
        CImGui::TableSetupColumn("Hide/Show", fixedNoSortFlags, 0.0f);
        CImGui::TableSetupScrollFreeze(0, 1);
        CImGui::TableHeadersRow();

        CImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(LIST_PADDING.x, tableVpad));

        float contentRegionWidth = CImGui::GetContentRegionAvail().x;

        // Sort - this is kinda horrible.
        static bool sortDirAscending = true;
        static bool sort = false;

        if (sort) {
            std::sort(overrideParts.begin(), overrideParts.end(), [&](const OverrideMeshPart& a, const OverrideMeshPart& b) {
                std::string lowa = toLower(a.part.name); std::string lowb = toLower(b.part.name);
                return sortDirAscending ? lowa < lowb : lowa > lowb;
            });
        }

        if (ImGuiTableSortSpecs* sort_specs = CImGui::TableGetSortSpecs()) {
            if (sort_specs->SpecsDirty && sort_specs->SpecsCount > 0) {
                const ImGuiTableColumnSortSpecs& sort_spec = sort_specs->Specs[0];
                sortDirAscending = sort_spec.SortDirection == ImGuiSortDirection_Ascending;

                switch (sort_spec.ColumnIndex)
                {
                case 1: sort = true;
                }

                sort_specs->SpecsDirty = false;
            }
        }

        std::vector<OverrideMeshPart> partRemoversToDelete{};
        for (OverrideMeshPart& partOverride : overrideParts) {
            CImGui::TableNextRow(0, selectableHeight);
            CImGui::TableNextColumn();

            CImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
            CImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.1f, 0.1f, 1.0f));
            CImGui::SetCursorPosY(CImGui::GetCursorPosY() + (selectableHeight - alignAdjust + tableVpad - CImGui::GetFontSize() * deleteButtonScale) * 0.5f);
            if (ImDeleteButton(("##del_" + partOverride.part.name).c_str(), deleteButtonScale)) {
                partRemoversToDelete.push_back(partOverride);
            }
            CImGui::PopStyleColor(2);

            CImGui::TableNextColumn();
            CImGui::SetCursorPosY(CImGui::GetCursorPosY() + (selectableHeight - alignAdjust + tableVpad - CImGui::CalcTextSize(partOverride.part.name.c_str()).y) * 0.5f);
            CImGui::Text(partOverride.part.name.c_str());

            CImGui::TableNextColumn();
			CImGui::SetCursorPosY(CImGui::GetCursorPosY() + (selectableHeight - alignAdjust + tableVpad - CImGui::GetFrameHeight()) * 0.5f);
			CImGui::SetCursorPosX(CImGui::GetCursorPosX() + (CImGui::GetColumnWidth() - 50.0f) * 0.5f);
            pushToggleColors(partOverride.shown);
            CImGui::Toggle(("##toggle_" + partOverride.part.name).c_str(), &partOverride.shown, ImGuiToggleFlags_Animated);
            popToggleColors();

			// Update value of 'shown' in the set. Kind of ugly. Too bad!
            auto it = parts.find(partOverride);
            if (it != parts.end() && it->shown != partOverride.shown) {
                auto tmp = *it;    // make a copy
                tmp.shown = partOverride.shown;
                parts.erase(it);
                parts.insert(std::move(tmp));
            }

			const char* tooltipText = partOverride.shown ? "This part will always be shown." : "This part will always be hidden.";
            CImGui::SetItemTooltip(tooltipText);
        }

        for (const OverrideMeshPart& part : partRemoversToDelete) {
            parts.erase(part);
        }

        CImGui::PopStyleVar();
        CImGui::EndTable();
    }

    bool EditorTab::canSavePreset(std::string& errMsg) const {
        assert(openObject.type == EditableObject::ObjectType::PRESET && openObject.ptrAfter.preset != nullptr);

        Preset& presetBefore = *openObject.ptrBefore.preset;
        Preset& preset = *openObject.ptrAfter.preset;

        if (preset.name.empty()) {
            errMsg = "Please provide a preset name";
            return false;
        }
        else if (preset.bundle.empty()) {
            errMsg = "Please provide a bundle name";
            return false;
        }
        else if (preset.name != presetBefore.name && dataManager.presetExists(preset.name)) {
            errMsg = "Preset name already taken";
            return false;
        }

        return true;
    }

    void EditorTab::drawArmourList(Preset& preset, const std::string& filter) {
        std::vector<ArmourSet> armours = ArmourList::getFilteredSets(filter);

        // Fixed-height, scrollable region
        CImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.02f, 0.02f, 0.02f, 1.0f));
        CImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 10));

        const float height = -(CImGui::GetFrameHeightWithSpacing() + 10.0f); // Leave space for the search bar
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
                    // TODO: Check if this behaviour is fine (will need some way of checking if part is present in cache)
     //               if (preset.armour != armourSet) {
     //                   preset.removedPartsBody.clear();
     //                   preset.removedPartsLegs.clear();
					//}
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

    void EditorTab::drawArmourSetName(const ArmourSet& armourSet, const float offsetBefore, const float offsetAfter) {
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

    bool EditorTab::drawStickyNavigationWidget(
        const std::string& text,
        bool* previewOut,
        std::function<bool()> canPreviewCb,
        std::function<bool()> canRevertCb,
        std::function<void()> revertCb,
        std::function<bool(std::string&)> canSaveCb,
        std::function<void()> saveCb
    ) {
        CImGui::BeginChild("StickyNavWidget", ImVec2(0, 40.0f), false, ImGuiWindowFlags_NoScrollbar);

        float availableWidth = CImGui::GetContentRegionAvail().x;
        float spacing = CImGui::GetStyle().ItemSpacing.x;

        const auto navigateBack = [&]() {
            closePopouts();
            editNoneDeferred();
        };

        // Check update state of object
        bool canRevert = INVOKE_REQUIRED_CALLBACK(canRevertCb);

        std::string errMsg = "";
        bool canSave = INVOKE_REQUIRED_CALLBACK(canSaveCb, errMsg);
        canSave &= canRevert;

        if (CImGui::ArrowButton("##navBackButton", ImGuiDir_Left)) {
            if (canSave) {
                navWarnUnsavedPanel.openNew("Warning: Unsaved Changes", "SaveConfirmInfoPanel", "You have unsaved changes - save now?", "Save", "Revert");
                navWarnUnsavedPanel.get()->focus();
                navWarnUnsavedPanel.get()->onCancel(navigateBack);
                navWarnUnsavedPanel.get()->onOk([saveCb, navigateBack]() { saveCb(); navigateBack(); });
            }
            else {
                navigateBack();
                CImGui::EndChild();
                return false;
            }
        }
        CImGui::SameLine();

        CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.75f));
        CImGui::Text(text.c_str());
        CImGui::PopStyleColor();

        // Cancel Button
        CImGui::SameLine();

        static constexpr const char* kRevertLabel  = "Revert";
        static constexpr const char* kSaveLabel    = "Save";
        static constexpr const char* kPreviewLabel = "Preview";

        float revertButtonWidth = CImGui::CalcTextSize(kRevertLabel).x + CImGui::GetStyle().FramePadding.x * 2;
        float saveButtonWidth = CImGui::CalcTextSize(kSaveLabel).x + CImGui::GetStyle().FramePadding.x * 2;
        float previewButtonWidth = CImGui::CalcTextSize(kSaveLabel).x + CImGui::GetStyle().ItemSpacing.x + 75.0f; // manual guess of how big the button is

        float useablePreviewWidth = previewOut ? previewButtonWidth : 0.0f;
        float useableSpacing = previewOut ? spacing * 2.0f : spacing;
        float totalWidth = revertButtonWidth + saveButtonWidth + useablePreviewWidth + useableSpacing;

        float cancelButtonPos = availableWidth - totalWidth;
        CImGui::SetCursorPosX(cancelButtonPos);
        CImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
        CImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.20f, 0.20f, 1.0f));
        CImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.10f, 0.10f, 0.10f, 1.0f));
        
        if (previewOut != nullptr) {
            if (*previewOut) {
                CImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.5f, 0.1f, 1.0f));
                CImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.8f, 0.3f, 1.0f));
            }
            else {
                CImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.5f, 0.1f, 0.1f, 1.0f));
                CImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.8f, 0.3f, 0.3f, 1.0f));
            }

            bool previewDisabled = INVOKE_OPTIONAL_CALLBACK_TYPED(bool, false, canPreviewCb);
            CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.75f));
            CImGui::Text("Preview");
            CImGui::PopStyleColor();
            CImGui::SameLine();
            if (previewDisabled) {
                CImGui::BeginDisabled();
                *previewOut = false;
            }
            CImGui::Toggle("##PreviewToggle", previewOut);
            CImGui::SameLine();
            if (previewDisabled) {
                CImGui::EndDisabled();
                CImGui::SetItemTooltip("Equip the matching armour set on your player to preview.");
            }
            else {
                CImGui::SetItemTooltip("Preview this preset on applicable characters.");
            }

            CImGui::PopStyleColor(2);
        }

        if (!canRevert) CImGui::BeginDisabled();
        if (CImGui::Button(kRevertLabel)) {
            INVOKE_REQUIRED_CALLBACK(revertCb);
        }
        if (!canRevert) CImGui::EndDisabled();

        CImGui::PopStyleColor(3);

        CImGui::SameLine();

        if (!canSave) CImGui::BeginDisabled();
        if (CImGui::Button(kSaveLabel)) {
            INVOKE_REQUIRED_CALLBACK(saveCb);
        }
        if (!canSave) CImGui::EndDisabled();
        if (!canRevert) CImGui::SetItemTooltip("No changes made.");
        else if (!canSave) CImGui::SetItemTooltip(errMsg.c_str());

        CImGui::Spacing();
        CImGui::Separator();

        CImGui::EndChild();

        return true;
    }

}