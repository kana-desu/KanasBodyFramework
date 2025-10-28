#pragma once 

#include <kbf/gui/panels/i_panel.hpp>

#include <kbf/cimgui/cimgui_funcs.hpp>

#include <functional>

namespace kbf {

	class InfoPopupPanel : public iPanel {
	public:
		InfoPopupPanel(
			const std::string& label,
			const std::string& strID,
			const std::vector<std::string>& messages,
			const std::string& okLabel = "OK",
			const std::string& cancelLabel = "",
			const bool allowClose = true);
		InfoPopupPanel(
			const std::string& label,
			const std::string& strID, 
			const std::string& message,
			const std::string& okLabel = "OK",
			const std::string& cancelLabel = "",
			const bool allowClose = true);

		bool draw() override;
		void onOk(std::function<void()> callback) { okCallback = callback; }
		void onCancel(std::function<void()> callback) { cancelCallback = callback; }

	private:
		constexpr static float wrapWidth = 900.0f; // Width for text wrapping
		std::vector<std::string> messages;
		std::string okLabel;
		std::string cancelLabel;
		const bool allowClose;

		std::function<void()> okCallback;
		std::function<void()> cancelCallback;
	};

}