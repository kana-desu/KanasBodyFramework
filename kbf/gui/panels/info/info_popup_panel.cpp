#include <kbf/gui/panels/info/info_popup_panel.hpp>

#include <kbf/util/functional/invoke_callback.hpp>

namespace kbf {

	InfoPopupPanel::InfoPopupPanel(
		const std::string& label,
		const std::string& strID,
		const std::vector<std::string>& messages,
		const std::string& okLabel,
		const std::string& cancelLabel,
		const bool allowClose
	) : iPanel(label, strID), messages{ messages }, okLabel{ okLabel }, cancelLabel{ cancelLabel }, allowClose{ allowClose } {}

	InfoPopupPanel::InfoPopupPanel(
		const std::string& label,
		const std::string& strID,
		const std::string& message,
		const std::string& okLabel,
		const std::string& cancelLabel,
		const bool allowClose
	) : iPanel(label, strID), messages{ std::vector{ message } }, okLabel{ okLabel }, cancelLabel{ cancelLabel }, allowClose{ allowClose } {
	}

	bool InfoPopupPanel::draw() {
		bool open = true;
		processFocus();
		CImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, ImVec2(0.5f, 0.5f));
		CImGui::Begin(nameWithID.c_str(), allowClose ? &open : nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);

		for (const std::string& message : messages) {
			ImVec2 text_size = CImGui::CalcTextSize(message.c_str(), nullptr, true, wrapWidth);
			CImGui::SetCursorPosX(CImGui::GetCursorPosX() + (CImGui::GetColumnWidth() - CImGui::CalcTextSize(message.c_str()).x) * 0.5f);
			CImGui::PushTextWrapPos(CImGui::GetCursorPosX() + wrapWidth);
			CImGui::TextWrapped("%s", message.c_str());
			CImGui::PopTextWrapPos();
		}

		CImGui::Spacing();
		CImGui::Spacing();

		// OK button only
		if (cancelLabel.empty()) {
			float windowWidth = CImGui::GetContentRegionAvail().x;
			float buttonWidth = CImGui::CalcTextSize(okLabel.c_str()).x; // + CImGui::GetStyle().FramePadding.x * 2;
			CImGui::SetCursorPosX((windowWidth - buttonWidth) * 0.5f);
			if (CImGui::Button(okLabel.c_str())) {
				INVOKE_OPTIONAL_CALLBACK(okCallback);
				open = false;
			}
		}
		else {
			constexpr float framePadding = 8.0f;
			float windowWidth = CImGui::GetContentRegionAvail().x;
			float okWidth = CImGui::CalcTextSize(okLabel.c_str()).x + 2.0f * framePadding;
			float cancelWidth = CImGui::CalcTextSize(cancelLabel.c_str()).x + 2.0f * framePadding;
			float totalWidth = okWidth + cancelWidth + CImGui::GetStyle().ItemSpacing.x;

			CImGui::SetCursorPosX((windowWidth - totalWidth) * 0.5f);
			CImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
			CImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.20f, 0.20f, 1.0f));
			CImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.10f, 0.10f, 0.10f, 1.0f));
			if (CImGui::Button(cancelLabel.c_str())) {
				INVOKE_OPTIONAL_CALLBACK(cancelCallback);
				open = false;
			}
			CImGui::SameLine();
			CImGui::PopStyleColor(3);
			if (CImGui::Button(okLabel.c_str())) {
				INVOKE_OPTIONAL_CALLBACK(okCallback);
				open = false;
			}
		}

		CImGui::End();
		CImGui::PopStyleVar();
		return open;
	}

}