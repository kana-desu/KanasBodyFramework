#include <kbf/gui/tabs/about/about_tab.hpp>

#include <kbf/gui/tabs/about/ascii_art_splash.hpp>
#include <kbf/gui/shared/tab_bar_separator.hpp>
#include <kbf/util/font/default_font_sizes.hpp>

#include <format>

#include <Windows.h>

#define WRAP_BULLET(bullet, text) CImGui::TextWrapped(bullet); CImGui::SameLine(); CImGui::TextWrapped(text)

namespace kbf {

	void AboutTab::draw() {
        if (CImGui::BeginTabBar("AboutTabs")) {
            if (CImGui::BeginTabItem("Info")) {
                drawInfoTab();
                CImGui::EndTabItem();
            }
            if (CImGui::BeginTabItem("Tutorials")) {
                drawTutorialsTab();
                CImGui::EndTabItem();
            }
            if (CImGui::BeginTabItem("Changelogs")) {
                drawChangelogTab();
                CImGui::EndTabItem();
            }
            CImGui::EndTabBar();
        }
	}

    void AboutTab::drawPopouts() {}
    void AboutTab::closePopouts() {}

    void AboutTab::drawInfoTab() {
        CImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 15));
        CImGui::Spacing();

        drawTabBarSeparator("Support", "Support");
        CImGui::TextWrapped(
            "A lot of time and effort went into making this plugin. It needed to be written from scratch in c++ for performance and functionality reasons, and was subsequently much more difficult than making a lua script.");
        CImGui::TextWrapped(
            "If you like KBF, please consider dropping me some change on Ko-fi! c:"
        );

        if (CImGui::Button("Ko-Fi", ImVec2(CImGui::GetContentRegionAvail().x, 50.0f))) {
#if defined(_WIN32)
            ShellExecute(0, 0, "https://ko-fi.com/kana00", 0, 0, SW_SHOW);
#endif
        }

        CImGui::Spacing();

        drawTabBarSeparator("Version", "Version");
        CImGui::Text(std::format("Kana's Body Framework v{}", KBF_VERSION).c_str());
        CImGui::Text(std::format("This version is tested to support MHWilds TU3, and migration from FBS v1.19").c_str());
        
        CImGui::Spacing();

        drawTabBarSeparator("What is Kana's Body Framework?", "WhatIsKBF");

        CImGui::TextWrapped(
            "Kana's Body Framework is primarily a successor to my mod Female Body Sliders For Everyone (FBS4all)."
        );

        CImGui::TextWrapped(
            "This framework allows you to modify model bone values on a per-character basis, and is not limited to your own player!"
            " Bones of other players, named npcs (e.g. alma, gemma...), and unnamed npcs (hunters that walk around) can also be modified."
        );

        CImGui::TextWrapped(
            "KBF is built on top of some of the core funcitonality of FBS (see \"Migrating from FBS\" tutorial), and can serve as an addition to it, or more "
            "efficient and flexible replacement for it if you don't use all the features of FBS (Extra Layered Armour, etc.)"
        );
        CImGui::Spacing();
        CImGui::Separator();
        CImGui::Spacing();


        CImGui::TextWrapped(
            "This means, if you have a bunch of modded armour models that have different body shapes, etc, you can use them all at once while "
            " keeping the body shapes you like globally, or for specific characters."
        );
        CImGui::TextWrapped(
            "If you have friends that also use KBF, you can share your presets with each other (see \"Sharing Presets\" tutorial) to see eachothers desired body types locally!"
        );

        CImGui::Spacing();

        drawTabBarSeparator("Help", "Help");

        if (CImGui::Button("View Visual Tutorials on GitHub", ImVec2(CImGui::GetContentRegionAvail().x, 50.0f))) {
#if defined(_WIN32)
            ShellExecute(0, 0, "https://github.com/kana-desu/KanasBodyFramework/tree/master/tutorials", 0, 0, SW_SHOW);
#endif
        }
        CImGui::TextWrapped(
            "KBF is a reasonably complex plugin, so I have provided picture-based guides for core usage of it on GitHub via the above button."
        );

        CImGui::TextWrapped(
            "You can also find some text-based tutorials under the \"Tutorials\" tab."
        );

        CImGui::Spacing();

        drawTabBarSeparator("Bug Reports", "Bug Reports");
        if (CImGui::Button("Open an Issue on GitHub", ImVec2(CImGui::GetContentRegionAvail().x, 50.0f))) {
#if defined(_WIN32)
            ShellExecute(0, 0, "https://github.com/kana-desu/KanasBodyFramework/issues/new?template=bug_report.yml", 0, 0, SW_SHOW);
#endif
        }
        CImGui::TextWrapped(
            "If you encounter any persistent or reoccurring bugs, please submit them as an issue on GitHub using the button above."
        );
        CImGui::TextWrapped(
            "When doing so, please include at least steps to reproduce the bug, and any error messages in pop-ups / KBF's console (Debug > Logs)."
        );
        
        CImGui::PopStyleVar();
    }

    void AboutTab::drawTutorialsTab() {
        CImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 15));
        CImGui::Spacing();

        if (CImGui::Button("View Visual Tutorials on GitHub", ImVec2(CImGui::GetContentRegionAvail().x, 50.0f))) {
#if defined(_WIN32)
            ShellExecute(0, 0, "https://github.com/kana-desu/KanasBodyFramework/tree/master/tutorials", 0, 0, SW_SHOW);
#endif
        }

        CImGui::Separator();

        if (CImGui::CollapsingHeader("Getting Started"))           drawTutorials_GettingStarted();
        if (CImGui::CollapsingHeader("Creating Presets"))          drawTutorials_CreatingPresets();
        if (CImGui::CollapsingHeader("Creating Preset Groups"))    drawTutorials_CreatingPresetGroups();
		if (CImGui::CollapsingHeader("Creating Player Overrides")) drawTutorials_CreatingPlayerOverrides();
        if (CImGui::CollapsingHeader("Migrating From FBS"))        drawTutorials_MigratingFromFbs();
        if (CImGui::CollapsingHeader("Sharing Presets"))           drawTutorials_SharingPresets();
        if (CImGui::CollapsingHeader("Manually Updating KBF"))     drawTutorials_ManuallyUpdatingKBF();

        CImGui::PopStyleVar();
    }

    void AboutTab::drawTutorials_GettingStarted() {
        CImGui::TextWrapped(
            "KBF Allows you to modify bones of characters with three types of configs:"
        );

        CImGui::Indent();
		WRAP_BULLET("1.", "Preset Groups - These are collections of presets that can be assigned globally to players and npcs.");
        WRAP_BULLET("2.", "Presets - These are individual settings for a specific armour set, which can be assigned to a preset group, or individual npc outfits (e.g. alma, gemma...).");
        WRAP_BULLET("3.", "Player Overrides - These are preset groups applied specifically to a single player, overriding any default preset groups set.");
        CImGui::Unindent();

        CImGui::TextWrapped(
            "To get started with KBF, it is recommended to make at least one preset group, and the presets you wish to use within it."
        );
        CImGui::TextWrapped(
            "This is very simple if you've used FBS previously. Check the \"Migrating from FBS\" tutorial. This will show you how to create a Preset Group & Presets from your existing FBS presets."
        );
        CImGui::TextWrapped(
            "Otherwise, you can follow the tutorials \"Creating Presets\", \"Creating Preset Groups\" & \"Creating Player Overrides\" to set things up properly."
        );

        CImGui::Spacing();
        CImGui::Separator();
        CImGui::Spacing();

        CImGui::TextWrapped(
            "Once created, you can apply preset groups globally to players or npcs by clicking Players > Male / Female, or NPCs > Male / Female"
        );

        CImGui::TextWrapped(
            "Individual presets can be applied to specific NPC outfits by clicking them in the NPCs tab."
        );
    }

    void AboutTab::drawTutorials_CreatingPresets() {
        CImGui::TextWrapped(
            "Presets are collections of bone modifiers for a specific armour set, which can be assigned to a preset group, or individual npc outfits (e.g. alma, gemma...). They also allow you to hide your slinger / weapon"
		);

        CImGui::Spacing();
        CImGui::TextWrapped(
            "To Create a Preset:"
        );
        
        CImGui::TextWrapped("Set-up basic info");
        CImGui::Separator();
        CImGui::Indent();
        WRAP_BULLET("-", "Click the \"Create Preset\" button at the top of the Presets tab");
        WRAP_BULLET("-", "If you already have a similar preset to the one you wish to create, you can opt to copy the existing preset here.");
		WRAP_BULLET("-", "Enter a name for your preset in the \"Name\" field.");
		WRAP_BULLET("-", "Enter a bundle name for your preset in the \"Bundle\" field. This is used to group similar presets together, e.g. \"Kana's Presets\", \"Alma Presets\", etc.");
		WRAP_BULLET("-", "Select the suggested sex for this preset in the \"Sex\" combo box. This is used only as a visual aid when selecting presets for characters of a certain sex.");
		WRAP_BULLET("-", "Select the armour set this preset is intended for in the armour list. If you want the preset to be generic, you can select \"Default\".");
		WRAP_BULLET("-", "Click the \"Create\" button to create the preset.");
        CImGui::Unindent();
        CImGui::Spacing();
        CImGui::TextWrapped("Modify Bones");
        CImGui::Separator();
        CImGui::Indent();
        WRAP_BULLET("-", "Open the preset you just created in the Editor via Editor > Edit a Preset, or by clicking on it in the preset tab > Edit.");
		WRAP_BULLET("-", "Once open in the Editor, click \"Body Modifiers\" / \"Leg Modifiers\" to begin adjusting bones for the body / legs");
		WRAP_BULLET("-", "Click the \"Add Bone Modifier\" button to add a new bone modifier.");
		WRAP_BULLET("-", "In the pop-up list select a bone, or add a list of the commonly appearing bones via \"Add Defaults\" at the bottom.");
		WRAP_BULLET("-", "Adjust the sliders for bones that appear in the bone list.");
        CImGui::Unindent();
        CImGui::Spacing();
		CImGui::TextWrapped("Remove Parts (Optional)");
        CImGui::Separator();
        CImGui::Indent();
        WRAP_BULLET("-", "Click the \"Remove Parts\" tab in the Editor.");
		WRAP_BULLET("-", "You can optionally hide the slinger and weapon by checking the \"Hide Slinger\" and \"Hide Weapon\" checkboxes.");
		WRAP_BULLET("-", "You can remove parts of the armour set by clicking the \"Remove Parts\" button and selecting the parts you want to remove from the list.");
		WRAP_BULLET("-", "Any parts that appear in the part list will be hidden for this specific preset.");
        CImGui::Unindent();

        CImGui::Spacing();
        CImGui::Separator();
        CImGui::Spacing();
        CImGui::TextWrapped(
            "Once you are done modifying the preset, you can save it by clicking the \"Save\" button at the bottom of the Editor."
        );
        CImGui::TextWrapped(
            "You can also preview the effect of the preset on your own player by equipping the piece and toggling \"Preview\" next to the revert and save buttons."
        );
        // Pastel orange text
		CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.6f, 1.0f));
        CImGui::TextWrapped(
            "Important Note: Bone & Part selection operates on a cache-based system. They won't necessarily show up in menus until they are loaded for the first time ever in-game with KBF installed."
            " It is recommended to hop into a big public lobby or equip the desired armour set before modifying."
		);
        CImGui::PopStyleColor();
    }

    void AboutTab::drawTutorials_CreatingPresetGroups() {
        CImGui::TextWrapped(
            "Preset Groups are collections of presets that can be assigned to players and unnamed hunters (npcs)."
		);
        CImGui::TextWrapped(
            "They contain presets that are applied based on the armour set a player / npc is wearing."
		);

        CImGui::Spacing();
        CImGui::TextWrapped(
            "To Create a Preset Group:"
        );

        CImGui::TextWrapped("Set it up manually");
        CImGui::Separator();
        CImGui::Indent();
        WRAP_BULLET("-", "Click the \"Create Preset Group\" button at the top of the Preset Groups tab");
		WRAP_BULLET("-", "If you already have a similar preset group to the one you intend to make, you can opt to copy it here.");
		WRAP_BULLET("-", "Enter a name for your preset group in the \"Name\" field.");
        WRAP_BULLET("-", "Enter a suggested character sex for the preset group to be used with in the \"Sex\" combo box. This is used only as a visual aid when using the preset group for characters.");
		WRAP_BULLET("-", "Click the \"Create\" button to create the preset group.");
		CImGui::Unindent();
		CImGui::Spacing();
		CImGui::TextWrapped("Assign Presets");
		CImGui::Separator();
		CImGui::Indent();
		WRAP_BULLET("-", "Open the preset group you just created in the Editor via Editor > Edit a Preset Group, or by clicking on it in the preset group tab > Edit.");
		WRAP_BULLET("-", "Click the \"Assigned Presets\" tab in the Editor.");
		WRAP_BULLET("-", "Assign presets to specific armour pieces by clicking the grid cells in the Body / Legs columns.");
		WRAP_BULLET("-", "Presets will apply to characters using this preset group based on the specified armour set that is equipped.");
		WRAP_BULLET("-", "If no preset is assigned for a particular armour piece, the preset assigned to \"Default\" will be used.");
		CImGui::Unindent();
		CImGui::Spacing();
		CImGui::TextWrapped("Alternatively: Create from a Preset Bundle");
		CImGui::Separator();
		CImGui::Indent();
		WRAP_BULLET("-", "Click the \"Create From Preset Bundle\" button at the top of the Preset Groups tab");
		WRAP_BULLET("-", "Enter a name for your preset group in the \"Name\" field.");
		WRAP_BULLET("-", "Enter a suggested sex for the preset group to be used with in the \"Sex\" combo box. This is used only as a visual aid when using the preset group for characters.");
		WRAP_BULLET("-", "Select the bundle to use. Presets will be automatically assigned to armour sets based on their suggested armour sets.");
		WRAP_BULLET("-", "Click the \"Create\" button to create the preset group.");
		WRAP_BULLET("-", "If you got a pop up saying there were conflicts, you should check which presets conflicted (used the same armour piece) in Debug > Log, and adjust assigned presets accordingly in the editor.");
		CImGui::Unindent();

        CImGui::Spacing();
        CImGui::Separator();
        CImGui::Spacing();

        CImGui::TextWrapped(
            "Once you are done modifying the preset group, you can save it by clicking the \"Save\" button at the bottom of the Editor."
		);
    }

    void AboutTab::drawTutorials_CreatingPlayerOverrides() {
        CImGui::TextWrapped(
            "Player Overrides are configs that allow you to apply a specific preset group to a specific player, independently of any default preset groups set."
        );

        CImGui::Spacing();
        CImGui::TextWrapped("Creating a Player Override is Simple:");
        CImGui::Separator();
        CImGui::Indent();
        WRAP_BULLET("-", "Click the \"Add Override\" button at the bottom of the Players tab");
        WRAP_BULLET("-", "Select the player you want to create an override for in the appearing player list.");
        WRAP_BULLET("-", "Select the newly created override in the override list, and select the preset group you'd like to apply.");
        CImGui::Unindent();
    }

    void AboutTab::drawTutorials_MigratingFromFbs() {
        CImGui::TextWrapped(
            "The quickest way to migrate from FBS to KBF is as follows:"
        );
        CImGui::Indent();
        WRAP_BULLET("1.", "Import your FBS Player presets via Presets > Import FBS Presets as Bundle.");
        WRAP_BULLET("2.", "Create a Preset Group from the created bundle via Preset Groups > Create From Preset Bundle.");
        WRAP_BULLET("3.", "Ensure that the correct presets are being used for the correct armour pieces via Editor > Edit a Preset Group > Assigned Presets.");
        WRAP_BULLET("4.", "Assign the created preset group and presets to players and npcs in the \"Players\" / \"NPCs tabs\".");
        CImGui::Unindent();

        CImGui::TextWrapped(
            "Step 3 is particularly important if you received a notification at step 2 saying that there were some conflicts with the presets you imported."
        );

        CImGui::TextWrapped(
            "If you have many (unused) presets in your FBS folder, you can restrict the import to only active presets by checking the \"Import Autoswitch Presets Only\" checkbox in the import panel during Step 1."
        );

        CImGui::Spacing();
        CImGui::Separator();
        CImGui::Spacing();

        CImGui::TextWrapped(
            "There are a few preset settings which are NOT able to be migrated automatically from FBS to KBF, and you will have to adjust these manually after importing your FBS presets."
        );

        CImGui::TextWrapped(
            "These settings are:"
        );
        CImGui::Indent();
        WRAP_BULLET("1.", "Manual bone settings - these are ambiguous in FBS, but not in KBF.");
        WRAP_BULLET("2.", "Slinger visibility - these are global in FBS, but per-preset in KBF.");
        WRAP_BULLET("3.", "Part Enables - these are global in FBS, but per-preset in KBF.");
        WRAP_BULLET("4.", "Face Presets - Unsupported in KBF. Might make it into a future release.");
        CImGui::Unindent();

        CImGui::TextWrapped(
            "Manual bones will be imported and show up in the editor for reference only, but will have no effect. You should replace these with modifiers on the specific bones they are supposed to affect."
        );

        CImGui::Spacing();
        CImGui::Separator();
        CImGui::Spacing();

        CImGui::TextWrapped(
            "Additionally, there are some preset settings in KBF which do not exist in FBS, which you may also want to adjust for any imported FBS presets:"
        );

        CImGui::Indent();
        WRAP_BULLET("1.", "Preset female / male preference - This is for sorting your presets.");
        WRAP_BULLET("2.", "Preset bundle - Imported presets are sorted into a single (specified) bundle, sorting these further can allow you to make preset groups easier if you have lots of presets for different models, etc.");
        CImGui::Unindent();

        CImGui::Spacing();
        CImGui::Separator();
        CImGui::Spacing();

        CImGui::TextWrapped(
            "Alma & Gemma presets must be migrated over manually, but it is recommended to do so anyway as KBF allows you to apply individual presets for each of their outfits."
        );
    }

    void AboutTab::drawTutorials_SharingPresets() {
        CImGui::TextWrapped(
            "Tools for sharing are found in the \"Share\" tab. Sharing presets can allow you to use others' premade settings."
        );

        CImGui::TextWrapped(
            "If you share between friends, you can use imported presets to create a player override so that your friend appears on your screen as they appear on theirs."
        );

        CImGui::Spacing();
        CImGui::Separator();
        CImGui::Spacing();

        CImGui::TextWrapped(
            "There are two ways to Import / Export presets into KBF - as a .KBF File or as a .zip (mod) archive."
        );

        CImGui::TextWrapped(
            "These are functionally identical, but are more convenient depending on if you're looking for single-time import or contininual updates (e.g. as a mod)."
        );

        CImGui::Spacing();
        CImGui::Spacing();
        CImGui::SeparatorText(".KBF File");

        CImGui::TextWrapped(
            "This option is best used if you want to share presets a single time, e.g. to create a back-up of your own presets."
        );

        CImGui::Spacing();
        CImGui::TextWrapped("Export");
        CImGui::Separator();
        CImGui::Indent();
		WRAP_BULLET("-", "Click on \"Export .KBF File\" in the Share tab.");
        WRAP_BULLET("-", "Select the presets you want to export, and click \"Export\".");
        WRAP_BULLET("-", "Select a location to save the .KBF file to, and click \"Save\".");
        WRAP_BULLET("-", "You can now share this file with others, or keep it as a back-up.");
		CImGui::Unindent();
        CImGui::Spacing();
        CImGui::TextWrapped("Import");
        CImGui::Separator();
		CImGui::Indent();
        WRAP_BULLET("-", "Click on \"Import .KBF File\" in the Share tab.");
        WRAP_BULLET("-", "Select the .KBF file you want to import in windows explorer, and click \"Open\".");
        WRAP_BULLET("-", "The presets will be imported into KBF and will be available for use.");
		CImGui::Unindent();

        CImGui::Spacing();
        CImGui::Spacing();
        CImGui::SeparatorText(".zip Archive");
        CImGui::TextWrapped(
            "This option is best used if you want to share presets as a mod, e.g. to create a mod that can be updated with new presets."
        );
        CImGui::Spacing();
        CImGui::TextWrapped("Export");
        CImGui::Separator();
		CImGui::Indent();
        WRAP_BULLET("-", "Click on \"Export Mod Archive\" in the Share tab.");
        WRAP_BULLET("-", "Select the presets you want to export, and click \"Export\".");
        WRAP_BULLET("-", "Select a location to save the .zip archive to, and click \"Save\".");
        WRAP_BULLET("-", "You can now share this archive with others, or keep it as a back-up.");
		CImGui::Unindent();
        CImGui::Spacing();
        CImGui::TextWrapped("Import");
        CImGui::Separator();
		CImGui::Indent();
        WRAP_BULLET("-", "Extract the exported .zip archive in the game's base directory, or drop it into Vortex.");
        CImGui::Unindent();

		CImGui::Spacing();
    }

    void AboutTab::drawTutorials_ManuallyUpdatingKBF() {
        CImGui::TextWrapped(
            "KBF comes with a default look-up list for all armours in the game, but these may not always be up to date with the latest content if you're playing before I release an official update."
		);

        CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.6f, 1.0f));
        CImGui::TextWrapped(
            "Note: I advise only updating *player* armours in this way. Armours for NPCs (especially Alma / Erik) require me to add additional functionality to the mod for full support."
        );
        CImGui::PopStyleColor();

        CImGui::TextWrapped(
            "If you want to update KBF with the latest armours before I do it, you can do so by manually adding them to the armour list in the kbf data folder:"
		);

        startCodeListing("##ArmourListPathEntry");
        CImGui::Indent();
        CImGui::TextWrapped(dataManager.armourListPath.string().c_str());
        CImGui::Unindent();
        endCodeListing();

        CImGui::TextWrapped(
            "Any changes made will not be reflected until you reload the game, or the plugin data via Settings > Reload Data."
        );

        CImGui::TextWrapped(
            "On restart, it is recommended to check Debug > Log for any errors to ensure your modified list was properly loaded (if not - an internal fallback is used)."
        );

        CImGui::Spacing();
        CImGui::Separator();
        CImGui::Spacing();

        CImGui::TextWrapped(
            "You must use the same format as is provided to add custom entries - namely, the following object structure:"
        );

        startCodeListing("##ArmourListEntryFormat");
        CImGui::Indent();
        CImGui::TextWrapped("{...");
        CImGui::Indent();
        CImGui::TextWrapped("\"[ARMOUR NAME]\": {");
		CImGui::Indent();
        CImGui::TextWrapped("\"female\": {\"body\": \"[ARMOUR ID]\", \"legs\": \"[ARMOUR ID]\"},");
        CImGui::TextWrapped("\"male\": {\"body\": \"[ARMOUR ID]\", \"legs\": \"[ARMOUR ID]\"}");
        CImGui::Unindent();
        CImGui::TextWrapped("}");
        CImGui::Unindent();
        CImGui::TextWrapped("}");
        CImGui::Unindent();
        endCodeListing();

        CImGui::Spacing();

        CImGui::TextWrapped(
            "To maintain compatibility with future updates I make to this list, type the armour name EXACTLY as it appears in-game in English, followed by what armour variants exist (i.e. 0 = alpha only, 0/1 = alpha & beta, 2 = gamma). Note that DLC armours (layered armours only) do not need any variants in their name."
        );

        CImGui::TextWrapped(
            "Armours for NPCs are also included in this list. Many of these have no official name, but I adhere to naming them as a combination of the NPC name (e.g. Alma/Gemma/NPC) and the outfit source (e.g. Flamefete/Oilwell Basin/Sild)"
        );

        CImGui::TextWrapped(
			"Female entries within an armour set are optional, but the male entry must always be present for your modified list to load."
        );
        CImGui::TextWrapped(
            "Similarly, body and legs entries are also optional, but again, at least one of them must be present per female / male entry for your modified list to load."
        );

        CImGui::Spacing();

        CImGui::TextWrapped(
            "The following examples show some correct and incorrect usages of this data format."
        );

		// Green text for correct entries, red for incorrect

        // Correct examples
        CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
        CImGui::SeparatorText("Complete Entry (valid)");
        CImGui::PopStyleColor();

        startCodeListing("##ArmourListEntryComplete");
        CImGui::Indent();
        CImGui::TextWrapped("{...");
        CImGui::Indent();
        CImGui::TextWrapped("\"Hope 0\": {");
        CImGui::Indent();
        CImGui::TextWrapped("\"female\": {\"body\": \"ch03_001_0012\", \"legs\": \"ch03_001_0014\"},");
        CImGui::TextWrapped("\"male\": {\"body\": \"ch03_001_0002\", \"legs\": \"ch03_001_0004\"}");
        CImGui::Unindent();
        CImGui::TextWrapped("}");
        CImGui::Unindent();
        CImGui::TextWrapped("}");
        CImGui::Unindent();
        endCodeListing();

        CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
		CImGui::SeparatorText("Optional Female Entry (valid)");
        CImGui::PopStyleColor();

        startCodeListing("##ArmourListEntryOptionalFemale");
        CImGui::Indent();
        CImGui::TextWrapped("{...");
        CImGui::Indent();
        CImGui::TextWrapped("\"Guild Cross 0\": {");
        CImGui::Indent();
        CImGui::TextWrapped("\"male\": {\"body\": \"ch03_073_0002\", \"legs\": \"ch03_073_0004\"}");
        CImGui::Unindent();
        CImGui::TextWrapped("}");
        CImGui::Unindent();
        CImGui::TextWrapped("}");
        CImGui::Unindent();
        endCodeListing();

        CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
        CImGui::SeparatorText("Optional Body Entry (valid)");
        CImGui::PopStyleColor();

        startCodeListing("##ArmourListEntryOptionalBody");
        CImGui::Indent();
        CImGui::TextWrapped("{...");
        CImGui::Indent();
        CImGui::TextWrapped("\"Gajau 0\": {");
        CImGui::Indent();
        CImGui::TextWrapped("\"female\": { \"legs\": \"ch03_052_0014\"},");
        CImGui::TextWrapped("\"male\": {\"legs\": \"ch03_052_0004\"}");
        CImGui::Unindent();
        CImGui::TextWrapped("}");
        CImGui::Unindent();
        CImGui::TextWrapped("}");
        CImGui::Unindent();
        endCodeListing();

        CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
        CImGui::SeparatorText("Optional Female & Legs Entry (valid)");
        CImGui::PopStyleColor();

        startCodeListing("##ArmourListEntryOptionalFemaleAndLegs");
        CImGui::Indent();
        CImGui::TextWrapped("{...");
        CImGui::Indent();
        CImGui::TextWrapped("\"Pinion Necklace 0\": {");
        CImGui::Indent();
        CImGui::TextWrapped("\"male\": {\"body\": \"ch03_089_0002\"}");
        CImGui::Unindent();
        CImGui::TextWrapped("}");
        CImGui::Unindent();
        CImGui::TextWrapped("}");
        CImGui::Unindent();
        endCodeListing();

        CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
        CImGui::SeparatorText("Empty Female/Male Entry (invalid)");
        CImGui::PopStyleColor();

        // Incorrect example
        startCodeListing("##ArmourListEntryEmpty");
        CImGui::Indent();
        CImGui::TextWrapped("{...");
        CImGui::Indent();
        CImGui::TextWrapped("\"Afi 0\": {");
        CImGui::TextWrapped("}");
        CImGui::Unindent();
        CImGui::TextWrapped("}");
        CImGui::Unindent();
        endCodeListing();

        CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
        CImGui::SeparatorText("Empty Body/Legs Entry (invalid)");
        CImGui::PopStyleColor();

        // Incorrect example
        startCodeListing("##ArmourListEntryEmptyLegs/Body");
        CImGui::Indent();
        CImGui::TextWrapped("{...");
        CImGui::Indent();
        CImGui::TextWrapped("\"Seregios 0/1\": {");
        CImGui::Indent();
        CImGui::TextWrapped("\"female\": {},");
        CImGui::TextWrapped("\"male\": {\"body\": \"ch03_038_0002\", \"legs\": \"ch03_038_0004\"}");
        CImGui::Unindent();
        CImGui::TextWrapped("}");
        CImGui::Unindent();
        CImGui::TextWrapped("}");
        CImGui::Unindent();
        endCodeListing();
    }

    void AboutTab::drawChangelogTab() {

        CImGui::Spacing();
        if (CImGui::CollapsingHeader("v1.0.5b", ImGuiTreeNodeFlags_DefaultOpen)) {
            CImGui::Spacing();
            CImGui::SeparatorText("Additions");
            WRAP_BULLET("-", "Added the option to disable weapon hiding entirely (so you can leave this up to other mods to decide).");
            WRAP_BULLET("-", "Added more options to control when weapons are not hidden (always show in tent, while sharpening, while riding seikret).");
            WRAP_BULLET("-", "Added the option to hide kinsects when the weapon is hidden.");
            CImGui::SeparatorText("Changes");
            WRAP_BULLET("-", "When importing an FBS preset, Custom Bone modifiers will be automatically moved under the correct piece tab (where the bone exists), instead of being non-functional in the base-armature.");
            WRAP_BULLET("-", "When editing a preset in the Editor tab, tabs will no longer be fully disabled if the corresponding armour piece for the preset does not exist.");
            CImGui::SeparatorText("Fixes");
            WRAP_BULLET("-", "Fixed a bug causing players and npcs to become untracked after eating a meal / starting a cutscene.");
            WRAP_BULLET("-", "Fixed a bug causing players and npcs to become untracked during cutscenes.");
            WRAP_BULLET("-", "Fixed a bug causing players and npcs to become untracked when opening / closing guild cards.");
            CImGui::Spacing();
        }
        if (CImGui::CollapsingHeader("v1.0.4b")) {
            CImGui::Spacing();
            CImGui::SeparatorText("Additions");
            WRAP_BULLET("-", "Persistent files (e.g. presets, etc) will now be upgraded to the latest format automatically when there is an update.");
            WRAP_BULLET("-", "Added the option to always show a part, as well as hide it.");
            WRAP_BULLET("-", "Added text indicating where bones are symmetrical modifiers (L & R) when editing a preset.");
            CImGui::SeparatorText("Changes");
            WRAP_BULLET("-", "Downgraded some KBF console errors to warnings.");
            CImGui::SeparatorText("Fixes");
            WRAP_BULLET("-", "Fixed presets always causing all parts to show up, even if they are supposed to be disabled.");
            WRAP_BULLET("-", "Fixed some UI conflicts on the editor tab for bones which have identical names after an L_ or R_ prefix is removed.");
            WRAP_BULLET("-", "Fixed presets not being applied to Alma's Scrivener's Coat outfit.");
            CImGui::Spacing();
        }
        if (CImGui::CollapsingHeader("v1.0.3b")) {
            CImGui::Spacing();
            CImGui::SeparatorText("Additions");
            WRAP_BULLET("-", "Added links to visual tutorials on GitHub.");
            CImGui::SeparatorText("Changes");
            WRAP_BULLET("-", "Adjusted non-compact bone sliders to have 3 decimal places of numerical accuracy.");
            CImGui::SeparatorText("Fixes");
            WRAP_BULLET("-", "Fixed a bug causing default presets in a preset group to not be used.");
            WRAP_BULLET("-", "Fixed a bug where player override filenames written in UTF-8 would cause other plugins to crash that only support ANSI. If you made player overrides for players with non-ANSI names before this, it is recommended you remove these files are recreate them.");
            WRAP_BULLET("-", "Fixed incorrect calculation of preset group size.");
            CImGui::Spacing();
        }
        if (CImGui::CollapsingHeader("v1.0.2b")) {
            CImGui::Spacing();
            WRAP_BULLET("-", "Updated to support FBS compatibility for dreamspell event.");
            CImGui::Spacing();
        }
        if (CImGui::CollapsingHeader("v1.0.1b")) {
            CImGui::Spacing();
            WRAP_BULLET("-", "Build System Improvements");
            WRAP_BULLET("-", "Small updates to about page to point to GitHub for issues, etc.");
            CImGui::Spacing();
        }
        if (CImGui::CollapsingHeader("v1.0.0b")) {
            CImGui::Spacing();
            WRAP_BULLET("-", "Initial Release! :)");
            CImGui::Spacing();
        }
    }

    void AboutTab::startCodeListing(const std::string& strID) {
        CImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.02f, 0.02f, 0.02f, 1.0f));
        CImGui::BeginChild(strID.c_str(), ImVec2(0, 0), ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_HorizontalScrollbar);
        CImGui::PushFont(monoFont, FONT_SIZE_DEFAULT_MONO);
        CImGui::Spacing();
        CImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 4));
    }

    void AboutTab::endCodeListing() {
        CImGui::PopStyleVar();
        CImGui::Spacing();
        CImGui::Spacing();
        CImGui::PopFont();
        CImGui::EndChild();
        CImGui::PopStyleColor();
	}

}