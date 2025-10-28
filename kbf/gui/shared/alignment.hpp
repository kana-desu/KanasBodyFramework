#pragma once

#include <kbf/cimgui/cimgui_funcs.hpp>

#include <string>

namespace kbf {

	inline void preAlignCellContentHorizontal(const std::string& content) {
		CImGui::SetCursorPosX(CImGui::GetCursorPosX() + (CImGui::GetColumnWidth() - CImGui::CalcTextSize(content.c_str()).x) * 0.5f);
	}

	inline void padX(const float padding) { CImGui::SetCursorPosX(CImGui::GetCursorPosX() + padding); }
	inline void padY(const float padding) { CImGui::SetCursorPosY(CImGui::GetCursorPosY() + padding); }
	inline void stretchNextItemX() { CImGui::SetNextItemWidth(CImGui::GetContentRegionAvail().x); }


}