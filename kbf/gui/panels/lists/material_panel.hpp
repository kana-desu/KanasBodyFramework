#pragma once 

#include <kbf/gui/panels/i_panel.hpp>
#include <kbf/data/kbf_data_manager.hpp>

#include <kbf/cimgui/cimgui_funcs.hpp>

#include <functional>

namespace kbf {

	class MaterialPanel : public iPanel {
	public:
		MaterialPanel(
			const std::string& label,
			const std::string& strID,
			KBFDataManager& dataManager,
			ArmourSetWithCharacterSex armour,
			ImFont* wsSymbolFont);

		bool draw() override;
		void onSelectMaterial (std::function<void(MeshMaterial, ArmourPiece)> callback) { selectCallback = callback; }
		void onCheckMaterialDisabled(std::function<bool(MeshMaterial, ArmourPiece)> callback) { checkDisableMatCallback = callback; }

	private:
		KBFDataManager& dataManager;
		ArmourSetWithCharacterSex armour;
		ArmourPieceFlags disabledHeaders = ArmourPieceFlagBits::APF_NONE;

		std::vector<MeshMaterial> filterMaterialList(const std::string& filter, const std::vector<MeshMaterial>& matList) const;
		void drawMaterialList(const std::vector<MeshMaterial>& matList, ArmourPiece piece);

		inline std::string getTypeLabel() const { return "Materials"; }

		std::function<void(MeshMaterial, ArmourPiece)> selectCallback;
		std::function<bool(MeshMaterial, ArmourPiece)> checkDisableMatCallback;

		ImFont* wsSymbolFont;
	};

}