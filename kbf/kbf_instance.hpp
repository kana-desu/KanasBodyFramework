#pragma once

#include <kbf/gui/kbf_window.hpp>
#include <kbf/npc/npc_tracker.hpp>
#include <kbf/player/player_tracker.hpp>
#include <kbf/data/kbf_data_manager.hpp>
#include <kbf/profiling/cpu_profiler.hpp>
#include <kbf/situation/situation_watcher.hpp>

namespace kbf {

	class KBFInstance {
	public:
		KBFInstance() = default;
		~KBFInstance() = default;

		__declspec(noinline)
		void initialize() {
			kbfDataManager.loadData();
			//kbf::SituationWatcher::initialize();

			kbfWindow.initialize();

			CpuProfiler::GlobalTimelineProfiler = CpuProfiler::Builder()
				.setWindowSize(1.0)
				.addBlock("(Pre) OnUpdateMotion")
				.addBlock("(Post) OnLateUpdateBehavior")
				.build();

			CpuProfiler::GlobalMultiScopeProfiler = CpuProfiler::Builder()
				.setWindowSize(1.0)
				.addBlock("NPC Fetch")
				.addBlock("NPC Fetch - Normal Gameplay - Basic Info")
				.addBlock("NPC Fetch - Normal Gameplay - Basic Info - Cache Load")
				.addBlock("NPC Fetch - Normal Gameplay - Visibility")
				.addBlock("NPC Fetch - Normal Gameplay - Persistent Info")
				.addBlock("NPC Fetch - Normal Gameplay - Equipped Armours")
				.addBlock("NPC Fetch - Normal Gameplay - Armour Transforms")
				.addBlock("NPC Fetch - Normal Gameplay - Bones")
				.addBlock("NPC Fetch - Normal Gameplay - Parts")
				.addBlock("NPC Apply")
				.addBlock("Player Fetch")
				.addBlock("Player Fetch - Normal Gameplay - Basic Info")
				.addBlock("Player Fetch - Normal Gameplay - Basic Info - Cache Load")
				.addBlock("Player Fetch - Normal Gameplay - Visibility")
				.addBlock("Player Fetch - Normal Gameplay - Persistent Info")
				.addBlock("Player Fetch - Normal Gameplay - Equipped Armours")
				.addBlock("Player Fetch - Normal Gameplay - Armour Transforms")
				.addBlock("Player Fetch - Normal Gameplay - Bones")
				.addBlock("Player Fetch - Normal Gameplay - Parts")
				.addBlock("Player Fetch - Normal Gameplay - Weapons")
				.addBlock("Player Fetch - Main Menu - Basic Info")
				.addBlock("Player Fetch - Main Menu - Equipped Armours")
				.addBlock("Player Fetch - Main Menu - Armour Transforms")
				.addBlock("Player Fetch - Main Menu - Bones")
				.addBlock("Player Fetch - Main Menu - Parts")
				.addBlock("Player Fetch - Main Menu - Weapon Objects")
				.addBlock("Player Fetch - Save Select - Basic Info")
				.addBlock("Player Fetch - Save Select - Equipped Armours")
				.addBlock("Player Fetch - Save Select - Armour Transforms")
				.addBlock("Player Fetch - Save Select - Bones")
				.addBlock("Player Fetch - Save Select - Parts")
				.addBlock("Player Fetch - Save Select - Weapon Objects")
				.addBlock("Player Fetch - Character Creator - Basic Info")
				.addBlock("Player Fetch - Character Creator - Equipped Armours")
				.addBlock("Player Fetch - Character Creator - Armour Transforms")
				.addBlock("Player Fetch - Character Creator - Bones")
				.addBlock("Player Fetch - Character Creator - Parts")
				.addBlock("Player Fetch - Guild Card - Basic Info")
				.addBlock("Player Fetch - Guild Card - Equipped Armours")
				.addBlock("Player Fetch - Guild Card - Armour Transforms")
				.addBlock("Player Fetch - Guild Card - Bones")
				.addBlock("Player Fetch - Guild Card - Parts")
				.addBlock("Player Apply")
				.build();
		}

		__declspec(noinline)
		void draw() {
			ImGuiStyle reframeworkStyle;
			ImGuiStyle* activeStyle = CImGui::GetStylePtr();
			memcpy(&reframeworkStyle, activeStyle, sizeof(ImGuiStyle));
			// Fix default style
			CImGui::StyleColorsDarkByArg(activeStyle);

			CImGui::Spacing();
			CImGui::Separator();

			if (CImGui::Button("Kana's Body Framework", ImVec2(CImGui::GetContentRegionAvail().x, 0))) {
				drawWindow = !drawWindow;
			}

			CImGui::Separator();
			CImGui::Spacing();

			if (drawWindow) {
				// Some stuff seems to be overridden by reframework
				CImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 9.0f);
				CImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
				CImGui::PushStyleVar(ImGuiStyleVar_TabRounding, 5.0f);

				kbfWindow.draw(&drawWindow);

				CImGui::PopStyleVar(3);
			}

			memcpy(activeStyle, &reframeworkStyle, sizeof(ImGuiStyle));
		}

		__declspec(noinline)
		void drawDisabled() {
			drawWindow = false;

			ImGuiStyle reframeworkStyle;
			ImGuiStyle* activeStyle = CImGui::GetStylePtr();
			memcpy(&reframeworkStyle, activeStyle, sizeof(ImGuiStyle));
			// Fix default style
			CImGui::StyleColorsDarkByArg(activeStyle);

			CImGui::Spacing();
			CImGui::Separator();

			CImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.70f, 0.15f, 0.15f, 1.0f));
			CImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.80f, 0.20f, 0.20f, 1.0f));
			CImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.60f, 0.12f, 0.12f, 1.0f));

			CImGui::BeginDisabled();
			CImGui::Button("Kana's Body Framework (CRASHED)", ImVec2(CImGui::GetContentRegionAvail().x, 0));
			CImGui::SetItemTooltip(
				"Kana's Body Framework has detected a crash, and has been disabled to prevent further issues.\n"
				"Please check REFramework's log and submit a bug report with it attached.");
			CImGui::EndDisabled();

			CImGui::PopStyleColor(3);

			CImGui::Separator();
			CImGui::Spacing();

			memcpy(activeStyle, &reframeworkStyle, sizeof(ImGuiStyle));
		}

		__declspec(noinline)
		void onPreUpdateMotion() {
			if (!kbfDataManager.settings().enabled) return;

			CpuProfiler::GlobalTimelineProfiler.get()->resetAccumulatedAll();
			CpuProfiler::GlobalMultiScopeProfiler.get()->resetAccumulatedAll();

			BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalTimelineProfiler.get(), "(Pre) OnUpdateMotion");

			if (kbfDataManager.settings().enabled) {
				BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler.get(), "Player Fetch");
				playerTracker.updatePlayers();
				END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler.get(), "Player Fetch");
			}

			if (kbfDataManager.settings().enableNpcs) {
				BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler.get(), "NPC Fetch");
				npcTracker.updateNpcs();
				END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler.get(), "NPC Fetch");
			}

			END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalTimelineProfiler.get(), "(Pre) OnUpdateMotion");
		}

		__declspec(noinline)
		void onPostLateUpdateBehavior() {
			if (!kbfDataManager.settings().enabled) return;

			BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalTimelineProfiler.get(), "(Post) OnLateUpdateBehavior");

			if (kbfDataManager.settings().enablePlayers) {
				BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler.get(), "Player Apply");
				playerTracker.applyPresets();
				END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler.get(), "Player Apply");
			}
			
			if (kbfDataManager.settings().enableNpcs) {
				BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler.get(), "NPC Apply");
				npcTracker.applyPresets();
				END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalMultiScopeProfiler.get(), "NPC Apply");
			}

			END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalTimelineProfiler.get(), "(Post) OnLateUpdateBehavior");
		}
			
	private:
		KBFDataManager kbfDataManager{ KBF_ASSET_PATH("KBF"), KBF_ASSET_PATH("FBSPresets") };
		PlayerTracker playerTracker{ kbfDataManager };
		NpcTracker    npcTracker{ kbfDataManager };

		KBFWindow kbfWindow{ kbfDataManager, playerTracker, npcTracker };

		// Auto show the window in debug builds - workflow really annoying otherwise.
#ifdef _DEBUG 
		bool drawWindow = true;
#else 
		bool drawWindow = false;
#endif
	};


}