#pragma once

#include <string>

#include <kbf/cimgui/cimgui_funcs.hpp>

namespace kbf {

	class iPanel {
	public:
		virtual bool draw() = 0;
		void focus() { needsFocus = true; }
		bool isFirstDraw() const { return firstDraw; }
		void setFirstDrawFlag(bool value) { firstDraw = value; }

	protected:
		iPanel(const std::string& name, const std::string& strID) 
			: name{ name }, strID{ strID } {}

		void processFocus() { if (needsFocus) { CImGui::SetNextWindowFocus(); needsFocus = false; } }

		const std::string name;
		const std::string strID;
		const std::string nameWithID = name + "###" + strID;

		bool firstDraw = true;
		bool needsFocus = false;
	};


}