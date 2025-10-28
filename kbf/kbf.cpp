#include <kbf/kbf.hpp>

#include <kbf/situation/situation_watcher.hpp>
#include <kbf/debug/debug_stack.hpp>
#include <kbf/cimgui/cimgui_funcs.hpp>

namespace kbf {

    KBF::KBF() {
        instance.initialize();
    }

    KBF& KBF::get() {
        static KBF kbf;
        return kbf;
    }

    void KBF::onPreUpdateMotion() {
        get().instance.onPreUpdateMotion();
    }

    void KBF::onPostUpdateMotion() { }

	void KBF::onPostLateUpdateBehavior() {
		get().instance.onPostLateUpdateBehavior();
    }

    void KBF::drawUI() {
        if (reframework::API::get()->reframework()->is_drawing_ui()) {
            instance.draw();
        }
    }

}