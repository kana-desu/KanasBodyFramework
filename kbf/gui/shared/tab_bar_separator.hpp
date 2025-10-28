#pragma once

#include <string>
#include <kbf/cimgui/cimgui_funcs.hpp>

namespace kbf {

    inline void drawTabBarSeparator(const std::string& label, const std::string& id) {
        if (CImGui::BeginTabBar(id.c_str())) {
            CImGui::BeginTabItem(label.c_str());
            CImGui::EndTabItem();
            CImGui::EndTabBar();
        }
    }

}