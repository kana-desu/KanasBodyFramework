#pragma once 

#include <kbf/gui/panels/i_panel.hpp>
#include <kbf/data/kbf_data_manager.hpp>

#include <kbf/cimgui/cimgui_funcs.hpp>

#include <functional>
#include <kbf/gui/shared/hovered_bone.hpp>

namespace kbf {

	class BonePanel : public iPanel {
	public:
		BonePanel(
			const std::string& label,
			const std::string& strID,
			KBFDataManager& dataManager,
			Preset** preset,
			ArmourPiece piece,
			ImFont* wsSymbolFont);

		bool draw() override;
		void onSelectBone(std::function<void(std::string)> callback) { selectCallback = callback; }
		void onCheckBoneDisabled(std::function<bool(std::string)> callback) { checkDisableBoneCallback = callback; }
		void onAddDefaults(std::function<void(void)> callback) { addDefaultsCallback = callback; }
        void onHover(std::function<void(HoveredBone)> callback) { hoverCallback = callback; }

	private:
		KBFDataManager& dataManager;
		Preset** preset;
		ArmourPiece piece;

		std::vector<std::string> filterBoneList(
			const std::string& filter,
			const std::vector<std::string>& boneList);
		void drawBoneList(const std::vector<std::string>& boneList);

		std::function<void(std::string)> selectCallback;
		std::function<bool(std::string)> checkDisableBoneCallback;
		std::function<void(void)>        addDefaultsCallback;
        std::function<void(HoveredBone)> hoverCallback;

		ImFont* wsSymbolFont;
	};

}