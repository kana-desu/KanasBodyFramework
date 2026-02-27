#pragma once

#include <kbf/cimgui/cimgui_funcs.hpp>
#include <kbf/kbf.hpp>

#include <reframework/API.hpp>
#define RE_EXPORT __declspec(dllexport)

extern lua_State* g_lua;

namespace kbf {
	inline void preInitialize() { CImGui::initializeFuncs(); }
	inline void forceInitializeReframework(const REFrameworkPluginInitializeParam* param) { reframework::API::initialize(param); }
	inline void fetch() { kbf::KBF::fetch(); }
	inline void apply() { kbf::KBF::apply(); }
	inline void drawUI() { kbf::KBF::drawInstanceUI(); }
}