#pragma once 

#include <kbf/data/kbf_data_manager.hpp>
#include <kbf/gui/tabs/i_tab.hpp>
#include <kbf/gui/panels/unique_panel.hpp>
#include <kbf/gui/panels/share/export_panel.hpp>

#include <queue>
#include <mutex>
#include <functional>
#include <atomic>

namespace kbf {

	class ShareTab : public iTab {
	public:
		ShareTab(KBFDataManager& dataManager) : iTab(), dataManager{ dataManager } {}

		void draw() override;
		void drawPopouts() override;
		void closePopouts() override;

		void setSymbolFont(ImFont* font) { wsSymbolFont = font; }
		void setArmourFont(ImFont* font) { wsArmourFont = font; }

	private:
		KBFDataManager& dataManager;

		void drawModArchiveList();

		std::string getImportFileDialog();

		// Thread-safe task queue
		std::queue<std::function<void()>> callbackQueue;
		std::mutex callbackMutex;

		std::atomic<bool> importDialogOpen = false;

		// Helper to post callback to run on main thread
		void postToMainThread(std::function<void()> func);
		void processCallbacks();

		UniquePanel<ExportPanel>    exportPanel;
		UniquePanel<InfoPopupPanel> importInfoPanel;
		UniquePanel<InfoPopupPanel> exportInfoPanel;
		void openExportFilePanel();
		void openExportModArchivePanel();
		void openImportInfoPanel(bool success, size_t nConflicts);
		void openExportInfoPanel(bool success);

		ImFont* wsSymbolFont = nullptr;
		ImFont* wsArmourFont = nullptr;
	};

}