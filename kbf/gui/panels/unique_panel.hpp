#pragma once

#include <kbf/gui/panels/i_panel.hpp>

#include <kbf/cimgui/cimgui_funcs.hpp>

#include <memory>

namespace kbf {

	// Container for single-view panel classes.
	// Provides generic management of the panel state (visible/not visible) and 
	//    a generic draw interface.
	template <class PanelT>
	class UniquePanel {
	public:
		UniquePanel() = default;

		PanelT* get() { return pendingNewPanel ? pendingNewPanel.get() : panel.get(); }
		bool isVisible() const { return panel != nullptr; }

		// TODO: I don't know how to make intellisense pick up the packed types here - Oh well!
		template <typename... Args>
		inline void open(Args&&... args) {
			if (panel == nullptr) panel = std::make_unique<PanelT>(std::forward<Args>(args)...);
			else panel->focus();
		}

		// Close and reopen the panel in a thread-safe mannerw
		template <typename... Args>
		inline void reopen(Args&&... args) {
			pendingNewPanel = std::make_unique<PanelT>(std::forward<Args>(args)...);
		}

		template <typename... Args>
		inline void openNew(Args&&... args) {
			if (isVisible()) reopen(std::forward<Args>(args)...);
			else             open(std::forward<Args>(args)...);
		}

		// Close that waits until next draw to complete (else synchronization issues may occur)
		inline void close() { if (panel != nullptr) needsClose = true; }
		inline void forceClose() { panel = nullptr; }

		void draw() {
			if (!isVisible()) return;

			ImVec2 center = CImGui::GetMainViewport()->GetCenter();
			CImGui::SetNextWindowPos(center, ImGuiCond_Once, ImVec2(0.5f, 0.5f));

			bool open = panel->draw();
			panel->setFirstDrawFlag(false);

			if (needsClose || !open) {
				panel = nullptr;
				needsClose = false;
			}

			if (pendingNewPanel) {
				panel = std::move(pendingNewPanel);
			}
		}

	private:
		std::unique_ptr<PanelT> panel;
		std::unique_ptr<PanelT> pendingNewPanel;
		bool needsClose = false;

	};

}