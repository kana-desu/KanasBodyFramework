#pragma once 

#include <kbf/gui/panels/i_panel.hpp>
#include <kbf/data/kbf_data_manager.hpp>

#include <kbf/cimgui/cimgui_funcs.hpp>

#include <functional>

namespace kbf {

	class PartPanel : public iPanel {
	public:
		PartPanel(
			const std::string& label,
			const std::string& strID,
			KBFDataManager& dataManager,
			ArmourSetWithCharacterSex armour,
			ImFont* wsSymbolFont);

		bool draw() override;
		void onSelectPart(std::function<void(MeshPart, ArmourPiece)> callback) { selectCallback = callback; }
		void onCheckPartDisabled(std::function<bool(MeshPart, ArmourPiece)> callback) { checkDisablePartCallback = callback; }

	private:
		KBFDataManager& dataManager;
		ArmourSetWithCharacterSex armour;
		ArmourPieceFlags disabledHeaders = ArmourPieceFlagBits::APF_NONE;

		std::vector<MeshPart> filterPartList(const std::string& filter, const std::vector<MeshPart>& partList) const;
		void drawPartList(const std::vector<MeshPart>& partList, ArmourPiece piece);

		std::string getTypeLabel() const { return "Parts"; }

		std::function<void(MeshPart, ArmourPiece)> selectCallback;
		std::function<bool(MeshPart, ArmourPiece)> checkDisablePartCallback;

		ImFont* wsSymbolFont;
	};

}