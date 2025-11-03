#pragma once

#include <kbf/cimgui/cimgui_funcs.hpp>

namespace kbf {

	inline void pushToggleColors(bool enabled) {
		if (enabled) {
			CImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.5f, 0.1f, 1.0f));
			CImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.8f, 0.3f, 1.0f));
		}
		else {
			CImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.5f, 0.1f, 0.1f, 1.0f));
			CImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.8f, 0.3f, 0.3f, 1.0f));
		}
	}

	inline void popToggleColors() {
		CImGui::PopStyleColor(2);
	}

}