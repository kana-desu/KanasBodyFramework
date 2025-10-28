#pragma once

#include <kbf/cimgui/cimgui_funcs.hpp>

namespace  {

	inline bool isTableCellHovered(float forceHeight = -1.0f) {
        ImVec2 mousePos = CImGui::GetMousePos();
        ImGuiTable* table = CImGui::GetCurrentTable();
		if (!table) return false;

		ImRect cellRect;
		CImGui::TableGetCellBgRectByArg(&cellRect, table, table->CurrentColumn);

		if (forceHeight > 0.0f) {
			cellRect.Max.y = cellRect.Min.y + forceHeight;
		}

        return cellRect.Min.x <= mousePos.x && mousePos.x <= cellRect.Max.x &&
			   cellRect.Min.y <= mousePos.y && mousePos.y <= cellRect.Max.y;
	}

}