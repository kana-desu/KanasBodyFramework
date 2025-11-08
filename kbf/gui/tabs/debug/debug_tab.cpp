#include <kbf/gui/tabs/debug/debug_tab.hpp>

#include <kbf/npc/get_npc_name_from_armour.hpp>
#include <kbf/gui/shared/styling_consts.hpp>
#include <kbf/gui/shared/alignment.hpp>
#include <kbf/data/ids/font_symbols.hpp>
#include <kbf/profiling/cpu_profiler.hpp>
#include <kbf/debug/debug_stack.hpp>
#include <kbf/util/string/copy_to_clipboard.hpp>
#include <kbf/util/font/default_font_sizes.hpp>
#include <kbf/util/string/ptr_to_hex_string.hpp>
#include <kbf/situation/situation_watcher.hpp>

#include <chrono>
#include <sstream>

// Remove this stupid windows macro
#undef ERROR

namespace kbf {

	void DebugTab::draw() {
        if (CImGui::BeginTabBar("DebugTabs")) {
            if (CImGui::BeginTabItem("Log")) {
                drawDebugTab();
                CImGui::EndTabItem();
            }
            if (CImGui::BeginTabItem("Performance")) {
                drawPerformanceTab();
                CImGui::EndTabItem();
            }
            if (CImGui::BeginTabItem("Situation")) {
                drawSituationTab();
                CImGui::EndTabItem();
            }
            if (CImGui::BeginTabItem("Player List")) {
                drawPlayerList();
                CImGui::EndTabItem();
            }
            if (CImGui::BeginTabItem("NPC List")) {
                drawNpcList();
                CImGui::EndTabItem();
            }
            if (CImGui::BeginTabItem("Bone Cache")) {
                drawBoneCacheTab();
                CImGui::EndTabItem();
            }
            if (CImGui::BeginTabItem("Part Cache")) {
				drawPartCacheTab();
                CImGui::EndTabItem();
            }
            CImGui::EndTabBar();
        }
	}

    void DebugTab::drawPopouts() {};
    void DebugTab::closePopouts() {};

    void DebugTab::drawDebugTab() {
        constexpr float padding = 5.0f;
		const float availableWidth = CImGui::GetContentRegionAvail().x;

        CImGui::Spacing();
        CImGui::Checkbox("Autoscroll", &consoleAutoscroll);
        CImGui::SameLine();
		CImGui::Checkbox("Debug", &showDebug);
		CImGui::SameLine();
        CImGui::Checkbox("Info", &showInfo);
		CImGui::SameLine();
		CImGui::Checkbox("Success", &showSuccess);
		CImGui::SameLine();
		CImGui::Checkbox("Warn", &showWarn);
		CImGui::SameLine();
		CImGui::Checkbox("Error", &showError);

        // Delete Button
        CImGui::SameLine();

        static constexpr const char* kCopyLabel = "Copy to Clipboard";

        float deleteButtonWidth = CImGui::CalcTextSize(kCopyLabel).x + CImGui::GetStyle().FramePadding.x;
        float totalWidth = deleteButtonWidth;

        float deleteButtonPos = availableWidth - deleteButtonWidth;
        CImGui::SetCursorPosX(deleteButtonPos);

        if (CImGui::Button(kCopyLabel)) {
            copyToClipboard(DEBUG_STACK.string());
        }

        CImGui::Spacing();

        CImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.02f, 0.02f, 0.02f, 1.0f));
        if (CImGui::BeginChild("DebugLogScrollWindow")) {
            CImGui::PushFont(monoFont, FONT_SIZE_DEFAULT_MONO);

            CImGui::Dummy(ImVec2(padding, padding)); // Top padding

            static const float timestampWidth = CImGui::CalcTextSize("00:00:00:0000 ").x;
            CImGui::PushTextWrapPos();

            for (const LogData& entry : DEBUG_STACK) {
                if ((entry.colour == DebugStack::Color::DEBUG && !showDebug) ||
                    (entry.colour == DebugStack::Color::INFO && !showInfo) ||
					(entry.colour == DebugStack::Color::SUCCESS && !showSuccess) ||
                    (entry.colour == DebugStack::Color::WARNING && !showWarn) ||
                    (entry.colour == DebugStack::Color::ERROR && !showError)) {
                    continue; // Skip entries based on filter settings
				}

                CImGui::PushTextWrapPos(CImGui::GetColumnWidth() - timestampWidth);

                // Payload
                CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(entry.colour.r, entry.colour.g, entry.colour.b, 1.0f));
                CImGui::TextUnformatted((" > " + entry.data).c_str());
                CImGui::PopStyleColor();

                // Time stamp.
                CImGui::PopTextWrapPos();

                // Right align.
                CImGui::SameLine(CImGui::GetColumnWidth(-1) - timestampWidth);

                // Draw time stamp.
                CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));

                // Convert timestamp to local time
                auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(entry.timestamp.time_since_epoch()).count();

                std::time_t time_t_seconds = millis / 1000;
                std::tm local_tm;
#ifdef _WIN32
                localtime_s(&local_tm, &time_t_seconds);
#else
                localtime_r(&time_t_seconds, &local_tm);
#endif
                int hours = local_tm.tm_hour;
                int minutes = local_tm.tm_min;
                int seconds = local_tm.tm_sec;
                int milliseconds = millis % 1000;
                std::string timeStr = std::format("{:02}:{:02}:{:02}:{:04}", hours, minutes, seconds, milliseconds);
                CImGui::Text(timeStr.c_str());
                CImGui::PopStyleColor();
            }

            CImGui::PopTextWrapPos();
            if (consoleAutoscroll) CImGui::SetScrollHereY(1.0f);
            CImGui::Dummy(ImVec2(padding, padding)); // Bot padding

            CImGui::PopFont();
            CImGui::EndChild();
        }

        CImGui::PopStyleColor(1);
    }

    void DebugTab::drawPerformanceTab() {
#if KBF_DEBUG_BUILD
        constexpr ImGuiTableFlags tableFlags = ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_PadOuterX;

        CImGui::Spacing();

        static bool hideNoOps = true;
        CImGui::Checkbox("Hide No-ops", &hideNoOps);

        CImGui::Spacing();
        CImGui::Separator();
        CImGui::Spacing();

        CImGui::BeginChild("ProfilerLists");

        // Global Timeline Profiler
        CImGui::Spacing();
        CImGui::SeparatorText("Timeline Profiler");
        CImGui::Spacing();

        CImGui::BeginTable("##TimelineProfilerTable", 1, tableFlags);
        CImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(LIST_PADDING.x, 0.0f));

        double total_ms = 0.0;
        for (auto& [blockName, t] : CpuProfiler::GlobalTimelineProfiler->getNamedBlocks()) {
            total_ms += t.totalMs;
            if (hideNoOps && t.totalMs == 0.0f) continue;

            drawPerformanceTab_TimingRow(blockName, t.totalMs, &t.maxTotalMs);
        }

        drawPerformanceTab_TimingRow("Total", total_ms);

        CImGui::PopStyleVar();
        CImGui::EndTable();

        CImGui::Spacing();
        CImGui::SeparatorText("Multi-scope Profiler");
        CImGui::Spacing();

        // Global Multi-Scope Profiler
        CImGui::BeginTable("##MultiScopeProfilerTable", 1, tableFlags);
        CImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(LIST_PADDING.x, 0.0f));

        total_ms = 0.0;
        for (auto& [blockName, t] : CpuProfiler::GlobalMultiScopeProfiler->getNamedBlocks()) {
            total_ms += t.totalMs;
            if (hideNoOps && t.totalMs == 0.0f) continue;

            drawPerformanceTab_TimingRow(blockName, t.totalMs, &t.maxTotalMs);
        }

        CImGui::PopStyleVar();
        CImGui::EndTable();

        CImGui::EndChild();

#else 

        CImGui::Spacing();

        CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.902f, 0.635f, 0.235f, 1.0f));
        constexpr char const* profilingDisabledStr = "Profiling is disabled for release builds of KBF.";
        preAlignCellContentHorizontal(profilingDisabledStr);
        CImGui::Text(profilingDisabledStr);
        CImGui::PopStyleColor();

#endif
    }

    void DebugTab::drawPerformanceTab_TimingRow(std::string blockName, double t, const double* max_t) {
        const ImVec4 timeCol    = getTimingColour(t);
		const ImVec4 maxTimeCol = getTimingColour(max_t ? *max_t : 0.0);
        constexpr float selectableHeight = 40.0f;
        const ImVec2 timingSize = CImGui::CalcTextSize("000.00000 ms");
        const ImVec2 maxTimingSize = CImGui::CalcTextSize("|    Max: 000.00000 ms");

        CImGui::TableNextRow();
        CImGui::TableNextColumn();

        ImVec2 pos = CImGui::GetCursorScreenPos();
        CImGui::Selectable(("##Selectable_" + blockName).c_str(), false, 0, ImVec2(0.0f, selectableHeight));

        ImVec2 blockNameSize = CImGui::CalcTextSize(blockName.c_str());
        ImVec2 blockNamePos;
        blockNamePos.x = pos.x;
        blockNamePos.y = pos.y + (selectableHeight - blockNameSize.y) * 0.5f;
        CImGui::GetWindowDrawList()->AddText(blockNamePos, CImGui::GetColorU32(ImGuiCol_Text), blockName.c_str());

        // Timings
        float endPos = CImGui::GetCursorScreenPos().x + CImGui::GetContentRegionAvail().x;

        constexpr float spacingBetween = 1.0f;

        ImVec2 timingPos;
        timingPos.x = endPos - timingSize.x;
        timingPos.y = pos.y + (selectableHeight - timingSize.y) * 0.5f;
		if (max_t) timingPos.x -= maxTimingSize.x + spacingBetween;

        // Stringify timings to 5 decimal places
        std::ostringstream t_oss;
        t_oss << std::fixed << std::setprecision(5) << t << " ms";
        std::string tStr = t_oss.str();

        CImGui::PushStyleColor(ImGuiCol_Text, timeCol);
        CImGui::GetWindowDrawList()->AddText(timingPos, CImGui::GetColorU32(ImGuiCol_Text), tStr.c_str());
        CImGui::PopStyleColor();

        ImVec2 maxTimingPos;
        if (max_t) {
            maxTimingPos.x = endPos - maxTimingSize.x - spacingBetween;
            maxTimingPos.y = pos.y + (selectableHeight - maxTimingSize.y) * 0.5f;

            std::ostringstream max_t_oss;
            max_t_oss << std::fixed << std::setprecision(5) << *max_t;
            std::string maxTStr = std::format("|    Max: {} ms", max_t_oss.str());
            CImGui::PushStyleColor(ImGuiCol_Text, maxTimeCol);
            CImGui::GetWindowDrawList()->AddText(maxTimingPos, CImGui::GetColorU32(ImGuiCol_Text), maxTStr.c_str());
            CImGui::PopStyleColor();
		}

    }

    void DebugTab::drawSituationTab() {
		CImGui::BeginChild("SituationList");
        
        constexpr ImGuiTableFlags tableFlags = ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_PadOuterX;

        CImGui::Spacing();
        CImGui::SeparatorText("Internal Situations");
        CImGui::Spacing();

        CImGui::BeginTable("##SituationTable", 1, tableFlags);
        CImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(LIST_PADDING.x, 0.0f));

        for (const auto& entry : SITUATION_NAMES) {
            bool active = SituationWatcher::inSituation(static_cast<KnownSituation>(entry.first));
            drawSituationTab_Row(entry.second, active, true);
        }
        
        drawSituationTab_Row("Multiplayer Safe", SituationWatcher::isMultiplayerSafe(), false);

        CImGui::PopStyleVar();
        CImGui::EndTable();

        CImGui::Spacing();
        CImGui::SeparatorText("Custom Situations");
        CImGui::Spacing();

        CImGui::BeginTable("##CustomSituationTable", 1, tableFlags);
        CImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(LIST_PADDING.x, 0.0f));

        for (const auto& entry : CUSTOM_SITUATION_NAMES) {
            bool active = SituationWatcher::inCustomSituation(static_cast<CustomSituation>(entry.first));
            drawSituationTab_Row(entry.second, active, true);
        }

        CImGui::PopStyleVar();
        CImGui::EndTable();

        CImGui::EndChild();
    }

    void DebugTab::drawSituationTab_Row(std::string name, bool active, bool colorBg) {
        ImVec4 bgCol = active ? ImVec4(0.2f, 0.6f, 0.2f, 0.5f) : ImVec4(0.6f, 0.2f, 0.2f, 0.5f);
        std::string situationName = name;
        std::string situationStatus = active ? "Active" : "Inactive";

        constexpr ImVec4 timeCol = ImVec4(1.0f, 1.0f, 1.0f, 0.5f);
        constexpr float selectableHeight = 40.0f;
        const ImVec2 timingSize = CImGui::CalcTextSize("0.00000 ms");

        CImGui::TableNextRow();
        CImGui::TableNextColumn();

        if (colorBg)  CImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, CImGui::GetColorU32(bgCol));
        if (!colorBg) CImGui::PushStyleColor(ImGuiCol_Text, bgCol);

        ImVec2 pos = CImGui::GetCursorScreenPos();
        CImGui::Selectable(("##Selectable_" + situationName).c_str(), false, 0, ImVec2(0.0f, selectableHeight));

        ImVec2 blockNameSize = CImGui::CalcTextSize(situationName.c_str());
        ImVec2 blockNamePos;
        blockNamePos.x = pos.x;
        blockNamePos.y = pos.y + (selectableHeight - blockNameSize.y) * 0.5f;
        CImGui::GetWindowDrawList()->AddText(blockNamePos, CImGui::GetColorU32(ImGuiCol_Text), situationName.c_str());
        if (!colorBg) CImGui::PopStyleColor();

        // Preset Group Name
        float endPos = CImGui::GetCursorScreenPos().x + CImGui::GetContentRegionAvail().x;

        ImVec2 presetGroupNamePos;
        presetGroupNamePos.x = endPos - timingSize.x;
        presetGroupNamePos.y = pos.y + (selectableHeight - timingSize.y) * 0.5f;

        // Stringify timings to 5 decimal places
        CImGui::PushStyleColor(ImGuiCol_Text, timeCol);
        CImGui::GetWindowDrawList()->AddText(presetGroupNamePos, CImGui::GetColorU32(ImGuiCol_Text), situationStatus.c_str());
        CImGui::PopStyleColor();
    }

    void DebugTab::drawPlayerList() {
        CImGui::BeginChild("PlayerList");
        
        constexpr ImGuiTableFlags tableFlags = ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_PadOuterX;

        const std::vector<PlayerData>& playerList = playerTracker.getPlayerList();
        if (playerList.size() == 0) {
            CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
            constexpr char const* noPresetStr = "\nNo Players Found.";
            CImGui::Spacing();
            preAlignCellContentHorizontal(noPresetStr);
            CImGui::Text(noPresetStr);
            CImGui::PopStyleColor();
            CImGui::EndChild();
            return;
        }

        CImGui::Spacing();
        CImGui::BeginTable("##PlayerListTable", 1, tableFlags);
        CImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(LIST_PADDING.x, 0.0f));

        for (const PlayerData& player : playerList) {
			const PlayerInfo& info = playerTracker.getPlayerInfo(player);
			const PersistentPlayerInfo* pInfo = playerTracker.getPersistentPlayerInfo(player);
            drawPlayerListRow(info, pInfo);
        }

        CImGui::PopStyleVar();
        CImGui::EndTable();

        CImGui::EndChild();
	}

    void DebugTab::drawPlayerListRow(PlayerInfo info, const PersistentPlayerInfo* pInfo) {
        CImGui::TableNextRow();
        CImGui::TableNextColumn();

        constexpr float selectableHeight = 60.0f;
        ImVec2 pos = CImGui::GetCursorScreenPos();
        CImGui::Selectable(("##Selectable_" + info.playerData.string()).c_str(), false, 0, ImVec2(0.0f, selectableHeight));

		constexpr const char* missingStr = "PERSISTENT INFO MISSING";
        std::string helmArmourStr    = missingStr;
		std::string bodyArmourStr    = missingStr;
		std::string armsArmourStr    = missingStr;
		std::string coilArmourStr    = missingStr;
		std::string legsArmourStr    = missingStr;
        std::string slingerArmourStr = missingStr;
        std::string wpParentStr           = missingStr;
		std::string wpSubParentStr        = missingStr;
		std::string wpReserveParentStr    = missingStr;
		std::string wpSubReserveParentStr = missingStr;
        std::string wpKinsectStr          = missingStr;
        std::string wpReserveKinsectStr   = missingStr;

        constexpr auto getArmourStr = [](const std::optional<ArmourSet>& armour) {
			return armour.has_value() ? std::format("{} ({})", armour.value().name, armour.value().female ? "F" : "M") : std::string{ "UNKNOWN" };
		};

        if (pInfo != nullptr) {
			helmArmourStr    = getArmourStr(pInfo->armourInfo.helm);
			bodyArmourStr    = getArmourStr(pInfo->armourInfo.body);
			armsArmourStr    = getArmourStr(pInfo->armourInfo.arms);
			coilArmourStr    = getArmourStr(pInfo->armourInfo.coil);
			legsArmourStr    = getArmourStr(pInfo->armourInfo.legs);
            slingerArmourStr = getArmourStr(pInfo->armourInfo.slinger);

			wpParentStr           = ptrToHexString(pInfo->Wp_Parent_GameObject);
			wpSubParentStr        = ptrToHexString(pInfo->WpSub_Parent_GameObject);
			wpReserveParentStr    = ptrToHexString(pInfo->Wp_ReserveParent_GameObject);
			wpSubReserveParentStr = ptrToHexString(pInfo->WpSub_ReserveParent_GameObject);
            wpKinsectStr          = ptrToHexString(pInfo->Wp_Insect);
            wpReserveKinsectStr   = ptrToHexString(pInfo->Wp_ReserveInsect);
        }
        
        CImGui::SetItemTooltip(std::format(
            "Transform: {}"
            "\n\nPlayerManageInfo: {}\nHunterCharacter: {}\ncHunterCreateInfo: {}\nEventModelSetupper: {}\nVolumeOccludee: {}"
            "\n\nHelm: {}\nBody: {}\nArms: {}\nCoil: {}\nLegs: {}\nSlinger: {}"
            "\n\nWp_Parent: {}\nWpSub_Parent: {}\nWp_ReserveParent: {}\nWpSub_ReserveParent: {}\nWp_Kinsect: {}\nWp_ResrveKinsect: {}"
            "\n\nVisible: {}\nWeapon Drawn: {}\nIn Combat: {}\nIn Tent: {}\nSharpening: {}\nRiding Seikret: {}"
            "\n\nCamera Distance Sq: {}", 
            ptrToHexString(info.pointers.Transform),
            ptrToHexString(info.optionalPointers.cPlayerManageInfo),
            ptrToHexString(info.optionalPointers.HunterCharacter),
			ptrToHexString(info.optionalPointers.cHunterCreateInfo),
            ptrToHexString(info.optionalPointers.EventModelSetupper),
            ptrToHexString(info.optionalPointers.VolumeOccludee),
            helmArmourStr,
            bodyArmourStr,
			armsArmourStr,
			coilArmourStr,
            legsArmourStr,
            slingerArmourStr,
            wpParentStr,
            wpSubParentStr,
            wpReserveParentStr,
            wpSubReserveParentStr,
			wpKinsectStr,
			wpReserveKinsectStr,
			info.visible ? "YES" : "NO",
            info.weaponDrawn ? "YES" : "NO",
			info.inCombat ? "YES" : "NO",
			info.inTent ? "YES" : "NO",
			info.isSharpening ? "YES" : "NO",
			info.isRidingSeikret ? "YES" : "NO",
            info.distanceFromCameraSq
        ).c_str());
            
        // Sex Mark
        std::string sexMarkSymbol = info.playerData.female ? WS_FONT_FEMALE : WS_FONT_MALE;
        ImVec4 sexMarkerCol = info.playerData.female ? ImVec4(0.76f, 0.50f, 0.24f, 1.0f) : ImVec4(0.50f, 0.70f, 0.33f, 1.0f);

        CImGui::PushFont(wsSymbolFont, FONT_SIZE_DEFAULT_WILDS_SYMBOLS);

        constexpr float sexMarkerSpacingAfter = 5.0f;
        constexpr float sexMarkerVerticalAlignOffset = 5.0f;
        ImVec2 sexMarkerSize = CImGui::CalcTextSize(sexMarkSymbol.c_str());
        ImVec2 sexMarkerPos;
        sexMarkerPos.x = pos.x + CImGui::GetStyle().ItemSpacing.x;
        sexMarkerPos.y = pos.y + (selectableHeight - sexMarkerSize.y) * 0.5f + sexMarkerVerticalAlignOffset;
        CImGui::GetWindowDrawList()->AddText(sexMarkerPos, CImGui::GetColorU32(sexMarkerCol), sexMarkSymbol.c_str());

        CImGui::PopFont();

        // Player name
        constexpr float playerNameSpacingAfter = 5.0f;
        ImVec2 playerNameSize = CImGui::CalcTextSize(info.playerData.name.c_str());
        ImVec2 playerNamePos;
        playerNamePos.x = sexMarkerPos.x + sexMarkerSize.x + sexMarkerSpacingAfter;
        playerNamePos.y = pos.y + (selectableHeight - playerNameSize.y) * 0.5f;

		bool isVisible = info.visible;
		if (!isVisible) CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
        CImGui::GetWindowDrawList()->AddText(playerNamePos, CImGui::GetColorU32(ImGuiCol_Text), info.playerData.name.c_str());
		if (!isVisible) CImGui::PopStyleColor();

        float endPos = CImGui::GetCursorScreenPos().x + CImGui::GetContentRegionAvail().x;
        bool nonNull = !info.pointers.hasNull();
		bool hasPersistentInfo = pInfo != nullptr;
        bool foundArmour = pInfo != nullptr && pInfo->armourInfo.isFullyPopulated();

        // Validity String
        std::string validityStr = "OK";
        if (!nonNull)                validityStr = "HAS NULL";
		else if (!hasPersistentInfo) validityStr = "NOT LOADED";
        else if (!foundArmour)       validityStr = "UKNOWN ARMOUR";
        ImVec2 validityStrSize = CImGui::CalcTextSize(validityStr.c_str());
        ImVec2 validityStrPos;
        validityStrPos.x = endPos - (validityStrSize.x + CImGui::GetStyle().ItemSpacing.x);
        validityStrPos.y = pos.y + (selectableHeight - validityStrSize.y) * 0.5f;

        ImVec4 validityStrCol = (nonNull && foundArmour) ? ImVec4(1.0f, 1.0f, 1.0f, 0.5f) : ImVec4(0.365f, 0.678f, 0.886f, 0.8f);
        CImGui::PushStyleColor(ImGuiCol_Text, validityStrCol);
        CImGui::GetWindowDrawList()->AddText(validityStrPos, CImGui::GetColorU32(ImGuiCol_Text), validityStr.c_str());
        CImGui::PopStyleColor();

        // Hunter ID
        std::string hunterIdStr = "(" + info.playerData.hunterId + ")";
        ImVec2 hunterIdStrSize = CImGui::CalcTextSize(hunterIdStr.c_str());
        ImVec2 hunterIdStrPos;
        hunterIdStrPos.x = playerNamePos.x + playerNameSize.x + playerNameSpacingAfter;
        hunterIdStrPos.y = pos.y + (selectableHeight - hunterIdStrSize.y) * 0.5f;
        CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
        CImGui::GetWindowDrawList()->AddText(hunterIdStrPos, CImGui::GetColorU32(ImGuiCol_Text), hunterIdStr.c_str());
        CImGui::PopStyleColor();
    }

    void DebugTab::drawNpcList() {
        constexpr ImGuiTableFlags tableFlags = ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_PadOuterX;

        std::vector<size_t> npcList = npcTracker.getNpcList();
		// Sort NPCs by index
		std::sort(npcList.begin(), npcList.end());

        CImGui::BeginChild("NpcList");

        if (npcList.size() == 0) {
            CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
            constexpr char const* noPresetStr = "\nNo NPCs Found.";
            CImGui::Spacing();
            preAlignCellContentHorizontal(noPresetStr);
            CImGui::Text(noPresetStr);
            CImGui::PopStyleColor();
            CImGui::EndChild();
            return;
        }

        CImGui::Spacing();
        CImGui::BeginTable("##PlayerListTable", 1, tableFlags);
        CImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(LIST_PADDING.x, 0.0f));

        for (const size_t& npc : npcList) {
            drawNpcListRow(npcTracker.getNpcInfo(npc), npcTracker.getPersistentNpcInfo(npc));
        }

        CImGui::PopStyleVar();
        CImGui::EndTable();

        CImGui::EndChild();
    }

    void DebugTab::drawNpcListRow(NpcInfo info, const PersistentNpcInfo* pInfo) {
        CImGui::TableNextRow();
        CImGui::TableNextColumn();

        constexpr float selectableHeight = 60.0f;
        ImVec2 pos = CImGui::GetCursorScreenPos();

        constexpr const char* missingStr = "PERSISTENT INFO MISSING";
        std::string helmArmourStr = missingStr;
        std::string bodyArmourStr = missingStr;
        std::string armsArmourStr = missingStr;
        std::string coilArmourStr = missingStr;
        std::string legsArmourStr = missingStr;
        std::string slingerArmourStr = missingStr;

        constexpr auto getArmourStr = [](const std::optional<ArmourSet>& armour) {
            return armour.has_value() ? std::format("{} ({})", armour.value().name, armour.value().female ? "F" : "M") : std::string{ "UNKNOWN" };
            };

        if (pInfo != nullptr) {
            helmArmourStr    = getArmourStr(pInfo->armourInfo.helm);
            bodyArmourStr    = getArmourStr(pInfo->armourInfo.body);
            armsArmourStr    = getArmourStr(pInfo->armourInfo.arms);
            coilArmourStr    = getArmourStr(pInfo->armourInfo.coil);
            legsArmourStr    = getArmourStr(pInfo->armourInfo.legs);
			slingerArmourStr = getArmourStr(pInfo->armourInfo.slinger);
        }

        CImGui::Selectable(("##Selectable_" + std::to_string(info.index)).c_str(), false, 0, ImVec2(0.0f, selectableHeight));
        CImGui::SetItemTooltip(std::format(
            "Transform: {}"
            "\ncNpcManageInfo: {}\nNpcAccessor: {}\nHunterCharacter: {}\nNpcParamHolder: {}\nNpcVisualSetting: {}\nVolumeOccludee: {}\nMeshBoundary: {}"
            "\n\nHelm: {}\nBody: {}\nArms: {}\nCoil: {}\nLegs: {}\nSlinger: {}"
            "\n\nPrefab: {}"
            "\n\nCamera Distance Sq: {}",
            ptrToHexString(info.pointers.Transform),
            ptrToHexString(info.optionalPointers.cNpcManageInfo),
            ptrToHexString(info.optionalPointers.NpcAccessor),
            ptrToHexString(info.optionalPointers.HunterCharacter),
            ptrToHexString(info.optionalPointers.NpcParamHolder),
            ptrToHexString(info.optionalPointers.NpcVisualSetting),
            ptrToHexString(info.optionalPointers.VolumeOccludee),
            ptrToHexString(info.optionalPointers.MeshBoundary),
            helmArmourStr,
            bodyArmourStr,
            armsArmourStr,
            coilArmourStr,
            legsArmourStr,
            slingerArmourStr,
            info.prefabPath.empty() ? "EMPTY" : info.prefabPath,
            info.distanceFromCameraSq
        ).c_str());

        // Sex Mark
        std::string sexMarkSymbol = info.female ? WS_FONT_FEMALE : WS_FONT_MALE;
        ImVec4 sexMarkerCol = info.female ? ImVec4(0.76f, 0.50f, 0.24f, 1.0f) : ImVec4(0.50f, 0.70f, 0.33f, 1.0f);

        CImGui::PushFont(wsSymbolFont, FONT_SIZE_DEFAULT_WILDS_SYMBOLS);

        constexpr float sexMarkerSpacingAfter = 5.0f;
        constexpr float sexMarkerVerticalAlignOffset = 5.0f;
        ImVec2 sexMarkerSize = CImGui::CalcTextSize(sexMarkSymbol.c_str());
        ImVec2 sexMarkerPos;
        sexMarkerPos.x = pos.x + CImGui::GetStyle().ItemSpacing.x;
        sexMarkerPos.y = pos.y + (selectableHeight - sexMarkerSize.y) * 0.5f + sexMarkerVerticalAlignOffset;
        CImGui::GetWindowDrawList()->AddText(sexMarkerPos, CImGui::GetColorU32(sexMarkerCol), sexMarkSymbol.c_str());

        CImGui::PopFont();

        // Npc name
        std::string npcName = std::format("[{}] NPC (Not loaded)", info.index);
        if (pInfo != nullptr) {
			npcName = std::format("[{}] {}", info.index, getNpcNameFromArmourSet(pInfo->armourInfo.body.has_value()
                ? pInfo->armourInfo.body.value()
                : ArmourList::DefaultArmourSet()));
        }

        constexpr float npcNameSpacingAfter = 5.0f;
        ImVec2 npcNameSize = CImGui::CalcTextSize(npcName.c_str());
        ImVec2 npcNamePos;
        npcNamePos.x = sexMarkerPos.x + sexMarkerSize.x + sexMarkerSpacingAfter;
        npcNamePos.y = pos.y + (selectableHeight - npcNameSize.y) * 0.5f;

        bool isVisible = info.visible;
        if (!isVisible) CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
        CImGui::GetWindowDrawList()->AddText(npcNamePos, CImGui::GetColorU32(ImGuiCol_Text), npcName.c_str());
		if (!isVisible) CImGui::PopStyleColor();

        float endPos = CImGui::GetCursorScreenPos().x + CImGui::GetContentRegionAvail().x;
        bool nonNull = !info.pointers.hasNull();
        bool hasPersistentInfo = pInfo != nullptr;
        bool foundArmour = pInfo != nullptr && pInfo->armourInfo.body.has_value();

        // Validity String
        std::string validityStr = "OK";
        if (!nonNull)                validityStr = "HAS NULL";
        else if (!hasPersistentInfo) validityStr = "NOT LOADED";
        else if (!foundArmour)       validityStr = "UKNOWN ARMOUR";
        ImVec2 validityStrSize = CImGui::CalcTextSize(validityStr.c_str());
        ImVec2 validityStrPos;
        validityStrPos.x = endPos - (validityStrSize.x + CImGui::GetStyle().ItemSpacing.x);
        validityStrPos.y = pos.y + (selectableHeight - validityStrSize.y) * 0.5f;

        ImVec4 validityStrCol = (nonNull && foundArmour) ? ImVec4(1.0f, 1.0f, 1.0f, 0.5f) : ImVec4(0.365f, 0.678f, 0.886f, 0.8f);
        CImGui::PushStyleColor(ImGuiCol_Text, validityStrCol);
        CImGui::GetWindowDrawList()->AddText(validityStrPos, CImGui::GetColorU32(ImGuiCol_Text), validityStr.c_str());
        CImGui::PopStyleColor();
    }

    void DebugTab::drawBoneCacheTab() {
        std::vector<ArmourSetWithCharacterSex> existingCaches = dataManager.boneCache().getCachedArmourSets();

        // Sort by armour set name
        std::sort(existingCaches.begin(), existingCaches.end(), [](const ArmourSetWithCharacterSex& a, const ArmourSetWithCharacterSex& b) {
            if (a.set.name < b.set.name) return true;
            if (a.set.name > b.set.name) return false;

            // If names are equal, sort based on sex
            if (a.characterFemale && !b.characterFemale) return true;
            if (!a.characterFemale && b.characterFemale) return false;

            // then by armour sex...
            if (a.set.female && !b.set.female) return true;
			if (!a.set.female && b.set.female) return false;

            return true;
        });

        static std::optional<ArmourSetWithCharacterSex> selectedSet = std::nullopt;

        CImGui::Spacing();
        CImGui::PushItemWidth(-1);
        CImGui::PushFont(wsArmourFont, FONT_SIZE_DEFAULT_WILDS_ARMOUR);

        std::string previewValue = selectedSet.has_value() 
            ? std::format("[{}-{}] {}", selectedSet.value().characterFemale ? "F" : "M", selectedSet.value().set.female ? "F" : "M", selectedSet.value().set.name)
            : "None Selected";
        
        if (CImGui::BeginCombo("##BoneCacheCombo", previewValue.c_str())) {
            if (CImGui::Selectable("None Selected", !selectedSet.has_value())) {
                selectedSet = std::nullopt;
		    }

            for (const ArmourSetWithCharacterSex& armour : existingCaches) {
                std::string displayName = std::format("[{}-{}] {}",
                    armour.characterFemale ? "F" : "M",
                    armour.set.female ? "F" : "M",
                    armour.set.name);

			    bool selected = selectedSet.has_value() && selectedSet.value() == armour;

                if (CImGui::Selectable(displayName.c_str(), selected)) {
                    selectedSet = armour;
                }

			    if (selected) CImGui::SetItemDefaultFocus();
            }

            CImGui::EndCombo();
        }
        CImGui::PopFont();
        CImGui::PopItemWidth();

        CImGui::Spacing();
        CImGui::Separator();
        CImGui::Spacing();

        CImGui::BeginChild("BoneCacheList");

        if (!selectedSet.has_value()) {
            CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
            constexpr char const* noCacheStr = "\nNo Bone Cache Selected.";
            CImGui::Spacing();
            preAlignCellContentHorizontal(noCacheStr);
            CImGui::Text(noCacheStr);
            CImGui::PopStyleColor();
            CImGui::EndChild();
            return;
        }
        else {
			const BoneCache* cache = dataManager.boneCache().getCachedBones(selectedSet.value());

            std::vector<std::string> setBones  = cache->set.getBones();
			std::vector<std::string> helmBones = cache->helm.getBones();
            std::vector<std::string> bodyBones = cache->body.getBones();
			std::vector<std::string> armsBones = cache->arms.getBones();
			std::vector<std::string> coilBones = cache->coil.getBones();
			std::vector<std::string> legsBones = cache->legs.getBones();
            size_t setHash  = cache->set.getHash();
			size_t helmHash = cache->helm.getHash();
            size_t bodyHash = cache->body.getHash();
			size_t armsHash = cache->arms.getHash();
			size_t coilHash = cache->coil.getHash();
            size_t legsHash = cache->legs.getHash();

            std::string setListLabel  = std::format("Base Bones ({}) [Hash={}]", setBones.size(),  setHash);
			std::string helmListLabel = std::format("Helm Bones ({}) [Hash={}]", helmBones.size(), helmHash);
			std::string bodyListLabel = std::format("Body Bones ({}) [Hash={}]", bodyBones.size(), bodyHash);
			std::string armsListLabel = std::format("Arms Bones ({}) [Hash={}]", armsBones.size(), armsHash);
			std::string coilListLabel = std::format("Coil Bones ({}) [Hash={}]", coilBones.size(), coilHash);
			std::string legsListLabel = std::format("Legs Bones ({}) [Hash={}]", legsBones.size(), legsHash);

            if (setBones.size() > 0)  drawBoneCacheTab_BoneList(setListLabel.c_str(),  setBones);
			if (helmBones.size() > 0) drawBoneCacheTab_BoneList(helmListLabel.c_str(), helmBones);
			if (bodyBones.size() > 0) drawBoneCacheTab_BoneList(bodyListLabel.c_str(), bodyBones);
			if (armsBones.size() > 0) drawBoneCacheTab_BoneList(armsListLabel.c_str(), armsBones);
			if (coilBones.size() > 0) drawBoneCacheTab_BoneList(coilListLabel.c_str(), coilBones);
			if (legsBones.size() > 0) drawBoneCacheTab_BoneList(legsListLabel.c_str(), legsBones);
        }

        CImGui::EndChild();
    }

    void DebugTab::drawBoneCacheTab_BoneList(const std::string& label, const std::vector<std::string>& bones) {
        constexpr ImGuiTableFlags tableFlags = ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_PadOuterX;
        CImGui::Spacing();

        if (CImGui::CollapsingHeader(label.c_str())) {
            CImGui::BeginTable(("##" + label + "Table").c_str(), 1, tableFlags);
            CImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(LIST_PADDING.x, 0.0f));

            for (const std::string& bone : bones) {
                CImGui::TableNextRow();
                CImGui::TableNextColumn();
                constexpr float selectableHeight = 30.0f;
                ImVec2 pos = CImGui::GetCursorScreenPos();
                CImGui::Selectable(("##Selectable_" + bone).c_str(), false, 0, ImVec2(0.0f, selectableHeight));
                CImGui::SetItemTooltip(bone.c_str());
                ImVec2 boneNameSize = CImGui::CalcTextSize(bone.c_str());
                ImVec2 boneNamePos;
                boneNamePos.x = pos.x + CImGui::GetStyle().ItemSpacing.x;
                boneNamePos.y = pos.y + (selectableHeight - boneNameSize.y) * 0.5f;
                CImGui::GetWindowDrawList()->AddText(boneNamePos, CImGui::GetColorU32(ImGuiCol_Text), bone.c_str());
            }

            CImGui::PopStyleVar();
            CImGui::EndTable();
        }
	}

    void DebugTab::drawPartCacheTab() {
        std::vector<ArmourSetWithCharacterSex> existingCaches = dataManager.partCache().getCachedArmourSets();

        // Sort by armour set name
        std::sort(existingCaches.begin(), existingCaches.end(), [](const ArmourSetWithCharacterSex& a, const ArmourSetWithCharacterSex& b) {
            if (a.set.name < b.set.name) return true;
            if (a.set.name > b.set.name) return false;

            // If names are equal, sort based on sex
            if (a.characterFemale && !b.characterFemale) return true;
            if (!a.characterFemale && b.characterFemale) return false;

            // then by armour sex...
            if (a.set.female && !b.set.female) return true;
            if (!a.set.female && b.set.female) return false;

            return true;
            });

        static std::optional<ArmourSetWithCharacterSex> selectedSet = std::nullopt;

        CImGui::Spacing();
        CImGui::PushItemWidth(-1);
        CImGui::PushFont(wsArmourFont, FONT_SIZE_DEFAULT_WILDS_ARMOUR);

        std::string previewValue = selectedSet.has_value()
            ? std::format("[{}-{}] {}", selectedSet.value().characterFemale ? "F" : "M", selectedSet.value().set.female ? "F" : "M", selectedSet.value().set.name)
            : "None Selected";

        if (CImGui::BeginCombo("##BoneCacheCombo", previewValue.c_str())) {
            if (CImGui::Selectable("None Selected", !selectedSet.has_value())) {
                selectedSet = std::nullopt;
            }

            for (const ArmourSetWithCharacterSex& armour : existingCaches) {
                std::string displayName = std::format("[{}-{}] {}",
                    armour.characterFemale ? "F" : "M",
                    armour.set.female ? "F" : "M",
                    armour.set.name);

                bool selected = selectedSet.has_value() && selectedSet.value() == armour;

                if (CImGui::Selectable(displayName.c_str(), selected)) {
                    selectedSet = armour;
                }

                if (selected) CImGui::SetItemDefaultFocus();
            }

            CImGui::EndCombo();
        }
        CImGui::PopFont();
        CImGui::PopItemWidth();

        CImGui::Spacing();
        CImGui::Separator();
        CImGui::Spacing();

        CImGui::BeginChild("PartCacheList");

        if (!selectedSet.has_value()) {
            CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
            constexpr char const* noCacheStr = "\nNo Part Cache Selected.";
            CImGui::Spacing();
            preAlignCellContentHorizontal(noCacheStr);
            CImGui::Text(noCacheStr);
            CImGui::PopStyleColor();
            CImGui::EndChild();
            return;
        }
        else {
            const PartCache* cache = dataManager.partCache().getCachedParts(selectedSet.value());

			std::vector<MeshPart> setParts  = cache->set.getParts();
			std::vector<MeshPart> helmParts = cache->helm.getParts();
            std::vector<MeshPart> bodyParts = cache->body.getParts();
			std::vector<MeshPart> armsParts = cache->arms.getParts();
			std::vector<MeshPart> coilParts = cache->coil.getParts();
            std::vector<MeshPart> legsParts = cache->legs.getParts();
			size_t setHash  = cache->set.getHash();
			size_t helmHash = cache->helm.getHash();
            size_t bodyHash = cache->body.getHash();
			size_t armsHash = cache->arms.getHash();
			size_t coilHash = cache->coil.getHash();
            size_t legsHash = cache->legs.getHash();

            std::string setListLabel  = std::format("Base Parts ({}) [Hash={}]", setParts.size(),  setHash);
			std::string helmListLabel = std::format("Helm Parts ({}) [Hash={}]", helmParts.size(), helmHash);
            std::string bodyListLabel = std::format("Body Parts ({}) [Hash={}]", bodyParts.size(), bodyHash);
			std::string armsListLabel = std::format("Arms Parts ({}) [Hash={}]", armsParts.size(), armsHash);
			std::string coilListLabel = std::format("Coil Parts ({}) [Hash={}]", coilParts.size(), coilHash);
            std::string legsListLabel = std::format("Legs Parts ({}) [Hash={}]", legsParts.size(), legsHash);

			if (setParts.size() > 0)  drawPartCacheTab_PartList(setListLabel.c_str(),  setParts);
			if (helmParts.size() > 0) drawPartCacheTab_PartList(helmListLabel.c_str(), helmParts);
            if (bodyParts.size() > 0) drawPartCacheTab_PartList(bodyListLabel.c_str(), bodyParts);
			if (armsParts.size() > 0) drawPartCacheTab_PartList(armsListLabel.c_str(), armsParts);
			if (coilParts.size() > 0) drawPartCacheTab_PartList(coilListLabel.c_str(), coilParts);
            if (legsParts.size() > 0) drawPartCacheTab_PartList(legsListLabel.c_str(), legsParts);
        }

        CImGui::EndChild();
    }

    void DebugTab::drawPartCacheTab_PartList(const std::string& label, const std::vector<MeshPart>& parts) {
        constexpr ImGuiTableFlags tableFlags = ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_PadOuterX;
        CImGui::Spacing();

        if (CImGui::CollapsingHeader(label.c_str())) {
            CImGui::BeginTable(("##" + label + "Table").c_str(), 1, tableFlags);
            CImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(LIST_PADDING.x, 0.0f));

            for (const MeshPart& part : parts) {
                CImGui::TableNextRow();
                CImGui::TableNextColumn();
                constexpr float selectableHeight = 30.0f;
                ImVec2 pos = CImGui::GetCursorScreenPos();
                CImGui::Selectable(("##Selectable_" + part.name).c_str(), false, 0, ImVec2(0.0f, selectableHeight));
                CImGui::SetItemTooltip(part.name.c_str());
                ImVec2 boneNameSize = CImGui::CalcTextSize(part.name.c_str());
                ImVec2 boneNamePos;
                boneNamePos.x = pos.x + CImGui::GetStyle().ItemSpacing.x;
                boneNamePos.y = pos.y + (selectableHeight - boneNameSize.y) * 0.5f;
                CImGui::GetWindowDrawList()->AddText(boneNamePos, CImGui::GetColorU32(ImGuiCol_Text), part.name.c_str());
            }

            CImGui::PopStyleVar();
            CImGui::EndTable();
        }
	}

    ImVec4 DebugTab::getTimingColour(double ms) {
		constexpr double greenThreshold = 0.5;
        constexpr double orangeThreshold = 1.0;

		if (ms < greenThreshold)  return ImVec4(0.2f, 0.8f, 0.2f, 1.0f); // Green
		if (ms < orangeThreshold) return ImVec4(0.9f, 0.6f, 0.2f, 1.0f); // Orange
		if (ms > orangeThreshold) return ImVec4(0.9f, 0.2f, 0.2f, 1.0f); // Red

		return ImVec4(1.0f, 1.0f, 1.0f, 0.5f); // wtf
    }

}