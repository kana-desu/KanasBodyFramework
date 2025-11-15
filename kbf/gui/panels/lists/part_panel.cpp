#include <kbf/gui/panels/lists/part_panel.hpp>

#include <kbf/util/string/to_lower.hpp>
#include <kbf/util/functional/invoke_callback.hpp>

namespace kbf {

    PartPanel::PartPanel(
        const std::string& name,
        const std::string& strID,
        KBFDataManager& dataManager,
        ArmourSetWithCharacterSex armour,
        ImFont* wsSymbolFont
    ) : iPanel(name, strID),
        dataManager{ dataManager },
        armour{ armour },
        wsSymbolFont{ wsSymbolFont } 
    {
        ArmourID armourID = ArmourList::getArmourIdFromSet(armour.set);

        disabledHeaders = ArmourPieceFlagBits::APF_NONE;
        if (!armourID.hasPiece(ArmourPiece::AP_HELM)) disabledHeaders |= ArmourPieceFlagBits::APF_HELM;
        if (!armourID.hasPiece(ArmourPiece::AP_BODY)) disabledHeaders |= ArmourPieceFlagBits::APF_BODY;
        if (!armourID.hasPiece(ArmourPiece::AP_ARMS)) disabledHeaders |= ArmourPieceFlagBits::APF_ARMS;
        if (!armourID.hasPiece(ArmourPiece::AP_COIL)) disabledHeaders |= ArmourPieceFlagBits::APF_COIL;
        if (!armourID.hasPiece(ArmourPiece::AP_LEGS)) disabledHeaders |= ArmourPieceFlagBits::APF_LEGS;
    }

    bool PartPanel::draw() {
        bool open = true;
        processFocus();
        CImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, ImVec2(0.5f, 0.5f));

        constexpr ImVec2 minWindowSize = ImVec2(550, 350);

        if (isFirstDraw()) { CImGui::SetNextWindowSize(minWindowSize); }
        CImGui::SetNextWindowSizeConstraints(minWindowSize, ImVec2(FLT_MAX, FLT_MAX));
        CImGui::Begin(nameWithID.c_str(), &open, ImGuiWindowFlags_NoCollapse);

        float width = CImGui::GetWindowSize().x;

        static char filterBuffer[128] = "";
        std::string filterStr{ filterBuffer };

        std::vector<MeshPart> cacheParts = {};
        const PartCache* cache = dataManager.partCacheManager().getCache(armour);

        ArmourPiece piece = ArmourPiece::AP_SET;

        if (CImGui::BeginTabBar("PartRemovePanelTabs")) {
            struct ArmourTab {
                const char* label;
                ArmourPiece piece;
                ArmourPieceFlagBits flag;
                const char* tooltip;
            };

            const std::string typeLabel = getTypeLabel();

            static const ArmourTab tabs[] = {
                {"Head",  ArmourPiece::AP_HELM, ArmourPieceFlagBits::APF_HELM, std::format("Armour set has no Helmet {}.", typeLabel).c_str()},
                {"Body",  ArmourPiece::AP_BODY, ArmourPieceFlagBits::APF_BODY, std::format("Armour set has no Body {}."  , typeLabel).c_str()},
                {"Arms",  ArmourPiece::AP_ARMS, ArmourPieceFlagBits::APF_ARMS, std::format("Armour set has no Arm {}."   , typeLabel).c_str()},
                {"Waist", ArmourPiece::AP_COIL, ArmourPieceFlagBits::APF_COIL, std::format("Armour set has no Coil {}."  , typeLabel).c_str()},
                {"Legs",  ArmourPiece::AP_LEGS, ArmourPieceFlagBits::APF_LEGS, std::format("Armour set has no Leg {}."   , typeLabel).c_str()},
            };

            const ArmourTab* tabToSelect = nullptr;
            if (isFirstDraw()) {
                for (const auto& tab : tabs) {
                    if (!(disabledHeaders & tab.flag)) {
                        tabToSelect = &tab;
                        piece = tab.piece;
                        break;
                    }
                }
            }

            for (const auto& tab : tabs) {
                bool disabled = disabledHeaders & tab.flag;
                if (disabled) CImGui::BeginDisabled();

                ImGuiTabItemFlags flags = ImGuiTabItemFlags_None;
                if (&tab == tabToSelect)
                    flags |= ImGuiTabItemFlags_SetSelected;

                if (CImGui::BeginTabItem(tab.label, nullptr, flags)) {
                    piece = tab.piece;
                    CImGui::EndTabItem();
                }

                if (disabled) {
                    CImGui::EndDisabled();
                    CImGui::SetItemTooltip(tab.tooltip);
                }
            }

            CImGui::EndTabBar();
        }

        if (cache) {
			const HashedPartList* cachePartList = nullptr;
            switch (piece) {
			case ArmourPiece::AP_SET:  cachePartList = &cache->set;  break;
			case ArmourPiece::AP_HELM: cachePartList = &cache->helm; break;
			case ArmourPiece::AP_BODY: cachePartList = &cache->body; break;
			case ArmourPiece::AP_ARMS: cachePartList = &cache->arms; break;
			case ArmourPiece::AP_COIL: cachePartList = &cache->coil; break;
			case ArmourPiece::AP_LEGS: cachePartList = &cache->legs; break;
            }
            
            if (cachePartList) cacheParts = cachePartList->getParts();
        }

        drawPartList(filterPartList(filterStr, cacheParts), piece);

        CImGui::PushItemWidth(-1);
        CImGui::InputTextWithHint("##Search", "Search...", filterBuffer, IM_ARRAYSIZE(filterBuffer));
        CImGui::PopItemWidth();

        CImGui::End();
        CImGui::PopStyleVar();

        return open;
    }

    std::vector<MeshPart> PartPanel::filterPartList(
        const std::string& filter,
        const std::vector<MeshPart>& partList
    ) const {
        std::vector<MeshPart> nameMatches;

        std::string filterLower = toLower(filter);

        for (const MeshPart& part : partList)
        {
            std::string nameLower = toLower(part.name);
            if (nameLower.find(filterLower) != std::string::npos)
                nameMatches.push_back(part);
        }

        return nameMatches;
    }

    void PartPanel::drawPartList(const std::vector<MeshPart>& partList, ArmourPiece piece) {
        // Fixed-height, scrollable region
        CImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.02f, 0.02f, 0.02f, 1.0f));
        CImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 10));

        const float height = -(CImGui::GetFrameHeightWithSpacing() + 10.0f); // Leave space for the search bar
        CImGui::BeginChild("PartListChild", ImVec2(0, height), true, ImGuiWindowFlags_HorizontalScrollbar);

        float contentRegionWidth = CImGui::GetContentRegionAvail().x;
        size_t partDrawCount = 0;

        if (partList.size() != 0) {
            for (const MeshPart& part : partList)
            {
                bool disablePart = INVOKE_OPTIONAL_CALLBACK_TYPED(bool, false, checkDisablePartCallback, part, piece);
                if (disablePart) continue;

                partDrawCount++;

                if (CImGui::Selectable(part.name.c_str())) {
                    INVOKE_REQUIRED_CALLBACK(selectCallback, part, piece);
                }
            }
        }

        if (partList.size() == 0 || partDrawCount == 0) {
            std::string pieceName = "";
            switch (piece) {
            case ArmourPiece::AP_SET:  pieceName = "Base";   break;
            case ArmourPiece::AP_HELM: pieceName = "Helmet"; break;
            case ArmourPiece::AP_BODY: pieceName = "Body";   break;
            case ArmourPiece::AP_ARMS: pieceName = "Arm";    break;
            case ArmourPiece::AP_COIL: pieceName = "Coil";   break;
            case ArmourPiece::AP_LEGS: pieceName = "Leg";    break;
            }

            const std::string typeLabel = getTypeLabel();

            std::string noneFoundStr = partList.size() == 0 
                ? std::format("No {} {} Found In Cache. (Try Equipping the Armour in-game)", pieceName, typeLabel)
                : std::format("All Recognised {} Already Added", typeLabel);

            CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
            CImGui::SetCursorPosX(CImGui::GetCursorPosX() + (CImGui::GetColumnWidth() - CImGui::CalcTextSize(noneFoundStr.c_str()).x) * 0.5f);
            CImGui::Text(noneFoundStr.c_str());
            CImGui::PopStyleColor();
        }

        CImGui::EndChild();
        CImGui::PopStyleVar();
        CImGui::PopStyleColor();
    }

}