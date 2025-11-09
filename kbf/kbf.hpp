#pragma once

#include <kbf/player/player_tracker.hpp>

#include <kbf/kbf_instance.hpp>

namespace kbf {

	class KBF {
	public: 
		static KBF& get();
		static KBFInstance& getInstance() { return get().instance; }

		static void onPreUpdateMotion();
		static void onPostUpdateMotion();
		static void onPostLateUpdateBehavior();

		void drawUI();
		static void drawInstanceUI() { get().drawUI(); }

	private:
		KBF();

		static void condStackTrace(const char* line);
		static void logStackTrace();
		static void logKbfDebugLog();

		KBFInstance instance;
		static bool pluginDisabled;
	};

}