#pragma once

#include <string>

namespace kbf {

	class iTab {
	public:
		virtual void draw() = 0;
		virtual void drawPopouts() = 0;
		virtual void closePopouts() = 0;
	};


}