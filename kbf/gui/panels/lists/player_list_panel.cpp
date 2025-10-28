#include <kbf/gui/panels/lists/player_list_panel.hpp>

#include <kbf/data/ids/font_symbols.hpp>
#include <kbf/util/string/to_lower.hpp>
#include <kbf/util/functional/invoke_callback.hpp>
#include <kbf/util/font/default_font_sizes.hpp>

#include <kbf/cimgui/cimgui_funcs.hpp>

#include <algorithm>

namespace kbf {

    PlayerListPanel::PlayerListPanel(
        const std::string& name,
        const std::string& strID,
        PlayerTracker& playerTracker,
        ImFont* wsSymbolFont
    ) : iPanel(name, strID), playerTracker{ playerTracker }, wsSymbolFont{ wsSymbolFont } {}

    bool PlayerListPanel::draw() {
        bool open = true;
        processFocus();
        CImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, ImVec2(0.5f, 0.5f));

        constexpr ImVec2 minWindowSize = ImVec2(550, 300);

        if (isFirstDraw()) { CImGui::SetNextWindowSize(minWindowSize); }
		CImGui::SetNextWindowSizeConstraints(minWindowSize, ImVec2(FLT_MAX, FLT_MAX));
        CImGui::Begin(nameWithID.c_str(), &open, ImGuiWindowFlags_NoCollapse);

        float width = CImGui::GetWindowSize().x;

        std::vector<PlayerData> playerList = playerTracker.getPlayerList();

        static char filterBuffer[128] = "";
        std::string filterStr{ filterBuffer };

        drawPlayerList(filterPlayerList(filterStr, playerList));

        CImGui::Spacing();
        CImGui::PushItemWidth(-1);
        CImGui::InputTextWithHint("##Search", "Search...", filterBuffer, IM_ARRAYSIZE(filterBuffer));
        CImGui::PopItemWidth();

        CImGui::End();
        CImGui::PopStyleVar();

        return open;
    }

    std::vector<PlayerData> PlayerListPanel::filterPlayerList(
        const std::string& filter,
        const std::vector<PlayerData>& playerList
    ) {
        std::vector<PlayerData> nameMatches;
        std::vector<PlayerData> idMatches;

        std::string filterLower = toLower(filter);

        for (const auto& player : playerList)
        {
            std::string nameLower = toLower(player.name);
            std::string idLower   = toLower(player.hunterId);

            if (!filter.empty())
            {
                if (nameLower.find(filterLower) != std::string::npos)
                    nameMatches.push_back(player);
                else if (idLower.find(filterLower) != std::string::npos)
                    idMatches.push_back(player);
            }
            else
            {
                nameMatches.push_back(player);  // Show all by default
            }
        }

        std::vector<PlayerData> filteredList;
        filteredList.insert(filteredList.end(), nameMatches.begin(), nameMatches.end());
        filteredList.insert(filteredList.end(), idMatches.begin(), idMatches.end());
        return filteredList;
    }

    void PlayerListPanel::drawPlayerList(const std::vector<PlayerData>& playerList) {
        // Fixed-height, scrollable region
        CImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.02f, 0.02f, 0.02f, 1.0f));
        CImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 10));
        
		const float height = -(CImGui::GetFrameHeightWithSpacing() + 10.0f); // Leave space for the search bar
        CImGui::BeginChild("PlayerListChild", ImVec2(0, height), true, ImGuiWindowFlags_HorizontalScrollbar);

        float contentRegionWidth = CImGui::GetContentRegionAvail().x;

        if (playerList.size() == 0) {
            const char* noneFoundStr = "No Players Found";

            CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
            CImGui::SetCursorPosX(CImGui::GetCursorPosX() + (CImGui::GetColumnWidth() - CImGui::CalcTextSize(noneFoundStr).x) * 0.5f);
            CImGui::Text(noneFoundStr);
            CImGui::PopStyleColor();
        }
        else {
            for (const auto& player : playerList)
            {
                bool disablePlayer = INVOKE_OPTIONAL_CALLBACK_TYPED(bool, false, checkDisablePlayerCallback, player);
                std::string disableTooltip = INVOKE_OPTIONAL_CALLBACK_TYPED(std::string, "", requestDisabledPlayerTooltipCallback);

                if (disablePlayer) {
                    const ImVec4 test = ImVec4(255, 0, 0, 100);
                    const ImVec4 disabled_bg = ImVec4(100, 100, 100, 100); // Greyed-out background
                    const ImVec4 disabled_text = ImVec4(1.0f, 1.0f, 1.0f, 0.5f);
                    CImGui::PushStyleColor(ImGuiCol_Header, test);           // Unselected background
                    CImGui::PushStyleColor(ImGuiCol_HeaderHovered, test);    // Hovered background
                    CImGui::PushStyleColor(ImGuiCol_HeaderActive, test);     // Active background
                    CImGui::PushStyleColor(ImGuiCol_Text, disabled_text);    // Greyed-out text
                }
                if (CImGui::Selectable(player.name.c_str())) {
                    if (!disablePlayer) INVOKE_REQUIRED_CALLBACK(selectCallback, player);
                }
                if (disablePlayer) {
                    CImGui::PopStyleColor(4);
                    if (!disableTooltip.empty()) CImGui::SetItemTooltip(disableTooltip.c_str());
                }

                constexpr float sexMarkerOffset = 10.0f;

                // Hunter ID
                const char* rightText = player.hunterId.c_str();
                ImVec2 rightTextSize = CImGui::CalcTextSize(rightText);
                float hunterIdCursorPosX = CImGui::GetCursorScreenPos().x + contentRegionWidth - (rightTextSize.x + sexMarkerOffset) - CImGui::GetStyle().ItemSpacing.x;
                float hunterIdCursorPosY = CImGui::GetItemRectMin().y + 4.0f;  // Same Y as the selectable item, plus vertical alignment

                CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
                CImGui::GetWindowDrawList()->AddText(ImVec2(hunterIdCursorPosX, hunterIdCursorPosY), CImGui::GetColorU32(ImGuiCol_Text), rightText);
                CImGui::PopStyleColor();

                // Sex Mark
                std::string symbol  = player.female ? WS_FONT_FEMALE : WS_FONT_MALE; // These are inverted in the font because i'm a dumbass
                std::string tooltip = player.female ? "Female" : "Male";
                ImVec4 colour = player.female ? ImVec4(0.76f, 0.50f, 0.24f, 1.0f) : ImVec4(0.50f, 0.70f, 0.33f, 1.0f);

                CImGui::PushFont(wsSymbolFont, FONT_SIZE_DEFAULT_WILDS_SYMBOLS);
                CImGui::PushStyleColor(ImGuiCol_Text, colour);

                float sexMarkerCursorPosX = CImGui::GetCursorScreenPos().x + contentRegionWidth - sexMarkerOffset;
                float sexMarkerCursorPosY = CImGui::GetItemRectMin().y + 4.0f;  // Same Y as the selectable item, plus vertical alignment
                CImGui::GetWindowDrawList()->AddText(
                    ImVec2(sexMarkerCursorPosX, sexMarkerCursorPosY),
                    CImGui::GetColorU32(ImGuiCol_Text),
                    symbol.c_str());

                CImGui::PopStyleColor();
                CImGui::PopFont();
            }
        }

        CImGui::EndChild();
        CImGui::PopStyleVar();
        CImGui::PopStyleColor();
    }

}