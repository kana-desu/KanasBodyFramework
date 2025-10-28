#pragma once 

#include <kbf/gui/tabs/i_tab.hpp>
#include <kbf/gui/tabs/editor/editable_object.hpp>
#include <kbf/gui/tabs/editor/bone_modifier_info_widget.hpp>
#include <kbf/data/kbf_data_manager.hpp>

#include <kbf/gui/panels/unique_panel.hpp>
#include <kbf/gui/panels/lists/preset_panel.hpp>
#include <kbf/gui/panels/lists/preset_group_panel.hpp>
#include <kbf/gui/panels/lists/bone_panel.hpp>
#include <kbf/gui/panels/lists/part_remover_panel.hpp>
#include <kbf/gui/panels/presets/create_preset_panel.hpp>
#include <kbf/gui/panels/info/info_popup_panel.hpp>
#include <kbf/data/bones/sortable_bone_modifier.hpp>

#include <kbf/cimgui/cimgui_funcs.hpp>

namespace kbf {

	class EditorTab : public iTab {
	public:
		EditorTab(
			KBFDataManager& dataManager,
			ImFont* wsSymbolFont = nullptr,
			ImFont* wsArmourFont = nullptr
		) : dataManager{ dataManager }, wsSymbolFont{ wsSymbolFont }, wsArmourFont{ wsArmourFont } {
		}

		void setSymbolFont(ImFont* font) { wsSymbolFont = font; }
		void setArmourFont(ImFont* font) { wsArmourFont = font; }

		void draw() override;
		void drawPopouts() override;
		void closePopouts() override;

		void editNone() { openObject.setNone(); }
		void editNoneDeferred() { needsEditNone = true; }
		void editPresetGroup(PresetGroup* preset);
		void editPreset(Preset* preset);
		void editPresetDeferred(Preset* preset) { presetToEdit = preset; }

	private:
		EditableObject openObject;
		KBFDataManager& dataManager;
		ImFont* wsSymbolFont;
		ImFont* wsArmourFont;

		void drawNoEditor();
		void openSelectPresetPanel();
		void openSelectPresetGroupPanel();
		void openCopyPresetPanel();
		void openCopyPresetGroupPanel();
		void openSelectBonePanel(ArmourPiece piece);
		void openPartRemoverPanel();
		void openAssignPresetPanel(ArmourSet armourSet, ArmourPiece piece);
		UniquePanel<PresetPanel>       presetPanel;
		UniquePanel<PresetGroupPanel>  presetGroupPanel;
		UniquePanel<BonePanel>         selectBonePanel;
		UniquePanel<PartRemoverPanel>  partRemoverPanel;
		UniquePanel<PresetPanel>       assignPresetPanel;
		UniquePanel<CreatePresetPanel> createPresetPanel;

		// Preset Group Editor
		void drawPresetGroupEditor();
		void drawPresetGroupEditor_Properties(PresetGroup** presetGroup);
		void drawPresetGroupEditor_AssignedPresets(PresetGroup** presetGroup);
		void drawPresetGroupEditor_AssignedPresetsTableCell(Preset* preset, ArmourSet armour, ArmourPiece piece, int column, float rowHeight);
		bool canSavePresetGroup(std::string& errMsg) const;
		void assignPreset(ArmourSet armourSet, ArmourPiece piece, std::string uuid);

		void initializePresetGroupBuffers(const PresetGroup* presetGroup);
		char presetGroupNameBuffer[128] = "";

		// Preset Editor
		void drawPresetEditor();
		void drawPresetEditor_Properties(Preset** preset);
		void drawPresetEditor_BoneModifiers(Preset** preset, ArmourPiece piece);
		static std::unordered_map<std::string, std::vector<SortableBoneModifier>> getProcessedModifiers(
			BoneModifierMap& modifiers,
			bool categorizeBones, 
			bool useSymmetry);
		void drawCompactBoneModifierTable(
			std::string tableName, 
			ArmourSetWithCharacterSex armourWithSex,
			ArmourPiece piece,
			std::vector<SortableBoneModifier>& sortableModifiers,
			BoneModifierMap& modifiers,
			float modLimit,
			bool enableWarnings = true);
		void drawBoneModifierTable(
			std::string tableName, 
			ArmourSetWithCharacterSex armourWithSex,
			ArmourPiece piece,
			std::vector<SortableBoneModifier>& sortableModifiers,
			BoneModifierMap& modifiers,
			float modLimit,
			bool enableWarnings = true);
		void drawCompactBoneModifierGroup(const std::string& strID, glm::vec3& group, float limit, ImVec2 size, std::string fmtPrefix = "");
		void drawBoneModifierGroup(const std::string& strID, glm::vec3& group, float limit, float width, float speed);
		void drawPresetEditor_PartVisibilities(Preset** preset);
		void drawPresetEditor_PartVisibilitiesTable(std::string tableName, std::set<MeshPart>& parts);

		bool canSavePreset(std::string& errMsg) const;

		void drawArmourList(Preset& preset, const std::string& filter);
		void drawArmourSetName(const ArmourSet& armourSet, const float offsetBefore, const float offsetAfter);
		BoneModifierInfoWidget setBoneInfoWidget{};
		BoneModifierInfoWidget helmBoneInfoWidget{};
		BoneModifierInfoWidget bodyBoneInfoWidget{};
		BoneModifierInfoWidget armsBoneInfoWidget{};
		BoneModifierInfoWidget coilBoneInfoWidget{};
		BoneModifierInfoWidget legsBoneInfoWidget{};

		void initializePresetBuffers(const Preset* preset);
		char presetNameBuffer[128] = "";
		char presetBundleBuffer[128] = "";

		bool drawStickyNavigationWidget(
			const std::string& text,
			bool* previewOut,
			std::function<bool()> canPreviewCb,
			std::function<bool()> canRevertCb,
			std::function<void()> revertCb,
			std::function<bool(std::string&)> canSaveCb,
			std::function<void()> saveCb);
		UniquePanel<InfoPopupPanel> navWarnUnsavedPanel;

		// Moving here to make sure lifecycle works with unsavedPanels
		std::function<void(void)> savePresetGroupCb = [&]() { dataManager.updatePresetGroup(openObject.ptrBefore.presetGroup->uuid, *openObject.ptrAfter.presetGroup); };
		std::function<void(void)> savePresetCb = [&]() { dataManager.updatePreset(openObject.ptrBefore.preset->uuid, *openObject.ptrAfter.preset); };

		bool needsEditNone = false;
		Preset* presetToEdit = nullptr;
	};

}