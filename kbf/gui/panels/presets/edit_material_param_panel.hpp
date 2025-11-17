#pragma once 

#include <kbf/gui/panels/i_panel.hpp>
#include <kbf/data/preset/override_material.hpp>
#include <kbf/data/kbf_data_manager.hpp>
#include <kbf/gui/panels/unique_panel.hpp>

#include <kbf/cimgui/cimgui_funcs.hpp>

#include <functional>

namespace kbf {

	class EditMaterialParamPanel : public iPanel {
	public:
		EditMaterialParamPanel(
			const std::string& name,
			const std::string& strID,
			const OverrideMaterial& material,
			ArmourPiece piece,
			ArmourSetWithCharacterSex armour,
			KBFDataManager& dataManager,
			ImFont* wsSymbolFont);

		bool draw() override;
		void onUpdate(std::function<void(OverrideMaterial)> callback) { updateCallback = callback; }
		void onClose(std::function<void()> callback) { closeCallback = callback; }

	private:
		bool drawMaterialParamEditorRow(const MeshMaterialParam& param);
		bool drawMissingMaterialParamRow(const std::string& paramName);

		KBFDataManager& dataManager;
		ArmourSetWithCharacterSex armour;
		ArmourPiece piece;
		OverrideMaterial materialBefore;
		OverrideMaterial materialAfter;

		bool needsWidthStretch = false;
		float widthStretch = 0.0f;

		std::function<void(OverrideMaterial)> updateCallback;
		std::function<void()>                 closeCallback;

		ImFont* wsSymbolFont;
	};

}