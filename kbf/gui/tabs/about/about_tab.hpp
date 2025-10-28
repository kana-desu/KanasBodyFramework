#pragma once 

#include <kbf/data/kbf_data_manager.hpp>
#include <kbf/gui/tabs/i_tab.hpp>

#include <kbf/cimgui/cimgui_funcs.hpp>

namespace kbf {

	class AboutTab : public iTab {
	public:
		AboutTab(
			KBFDataManager& dataManager,
			ImFont* monoFont = nullptr,
			ImFont* monoFontTiny = nullptr
		) : dataManager{ dataManager }, monoFont{ monoFont }, monoFontTiny{ monoFontTiny } {}

		void setMonoFont(ImFont* font) { monoFont = font; }
		void setMonoFontTiny(ImFont* font) { monoFontTiny = font; }

		void draw() override;
		void drawPopouts() override;
		void closePopouts() override;

	private:
		KBFDataManager& dataManager;

		void drawInfoTab();

		void drawTutorialsTab();
		void drawTutorials_GettingStarted();
		void drawTutorials_CreatingPresets();
		void drawTutorials_CreatingPresetGroups();
		void drawTutorials_CreatingPlayerOverrides();
		void drawTutorials_MigratingFromFbs();
		void drawTutorials_SharingPresets();
		void drawTutorials_ManuallyUpdatingKBF();

		void drawChangelogTab();

		void startCodeListing(const std::string& strID);
		void endCodeListing();

		ImFont* monoFont;
		ImFont* monoFontTiny;

	};

}