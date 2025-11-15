#include <kbf/gui/panels/lists/bone_panel.hpp>

#include <kbf/data/ids/font_symbols.hpp>
#include <kbf/util/string/to_lower.hpp>
#include <kbf/util/functional/invoke_callback.hpp>
#include <kbf/data/bones/default_bones.hpp>

#include <set>

namespace kbf {

    BonePanel::BonePanel(
        const std::string& name,
        const std::string& strID,
        KBFDataManager& dataManager,
        Preset** preset,
        ArmourPiece piece,
        ImFont* wsSymbolFont
    ) : iPanel(name, strID), 
        dataManager{ dataManager },
        preset{ preset },
        piece{ piece },
        wsSymbolFont{ wsSymbolFont } {}

    bool BonePanel::draw() {
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

        std::vector<std::string> cacheBones;

        ArmourSetWithCharacterSex armourWithSex{ (**preset).armour, (**preset).female };
        const BoneCache* cache = dataManager.boneCacheManager().getCache(armourWithSex);
        if (cache) {
            HashedBoneList cacheBoneList;
            switch (piece) {
			case ArmourPiece::AP_SET:  cacheBoneList = cache->set; break;
			case ArmourPiece::AP_HELM: cacheBoneList = cache->helm; break;
			case ArmourPiece::AP_BODY: cacheBoneList = cache->body; break;
			case ArmourPiece::AP_ARMS: cacheBoneList = cache->arms; break;
			case ArmourPiece::AP_COIL: cacheBoneList = cache->coil; break;
			case ArmourPiece::AP_LEGS: cacheBoneList = cache->legs; break;
            }
            cacheBones = cacheBoneList.getBones();
        }

        std::set<std::string> selectableBones = getDefaultBones(piece, (**preset).female);
        selectableBones.insert(cacheBones.begin(), cacheBones.end());

        std::vector<std::string> boneList(selectableBones.begin(), selectableBones.end());

        drawBoneList(filterBoneList(filterStr, boneList));

        CImGui::Spacing();
        CImGui::PushItemWidth(-1);
        CImGui::InputTextWithHint("##Search", "Search...", filterBuffer, IM_ARRAYSIZE(filterBuffer));
        CImGui::PopItemWidth();

        CImGui::Spacing();
        if (CImGui::Button("Add Defaults", ImVec2(CImGui::GetContentRegionAvail().x, 0.0f))) {
            INVOKE_REQUIRED_CALLBACK(addDefaultsCallback);
        }
        CImGui::SetItemTooltip("Adds a selection of common bones that appear in most armour sets.");

        CImGui::End();
        CImGui::PopStyleVar();

        return open;
    }

    std::vector<std::string> BonePanel::filterBoneList(
        const std::string& filter,
        const std::vector<std::string>& boneList
    ) {
        if (filter == "") return boneList;

        std::vector<std::string> nameMatches;

        std::string filterLower = toLower(filter);

        for (const std::string& bone : boneList)
        {
            std::string nameLower = toLower(bone);
            if (nameLower.find(filterLower) != std::string::npos)
                nameMatches.push_back(bone);
        }

        return nameMatches;
    }

    void BonePanel::drawBoneList(const std::vector<std::string>& boneList) {

        // Fixed-height, scrollable region
        CImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.02f, 0.02f, 0.02f, 1.0f));
        CImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 10));

        const float height = -(2.0f * CImGui::GetFrameHeightWithSpacing() + 10.0f); // Leave space for the search bar
        CImGui::BeginChild("BoneListChild", ImVec2(0, height), true, ImGuiWindowFlags_HorizontalScrollbar);

        float contentRegionWidth = CImGui::GetContentRegionAvail().x;
        size_t boneDrawCount = 0;


        if (boneList.size() != 0) {
            for (const std::string& bone : boneList)
            {
                bool disableBone = INVOKE_OPTIONAL_CALLBACK_TYPED(bool, false, checkDisableBoneCallback, bone);
                if (disableBone) continue;

                boneDrawCount++;

                if (CImGui::Selectable(bone.c_str())) {
                    INVOKE_REQUIRED_CALLBACK(selectCallback, bone);
                }
            }
        }

        if (boneList.size() == 0 || boneDrawCount == 0) {
            const char* noneFoundStr = boneList.size() == 0 ? "No Bones Found In Cache. (Try Equipping the Armour in-game)" : "All Recognised Bones Already Added";

            CImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
            CImGui::SetCursorPosX(CImGui::GetCursorPosX() + (CImGui::GetColumnWidth() - CImGui::CalcTextSize(noneFoundStr).x) * 0.5f);
            CImGui::Text(noneFoundStr);
            CImGui::PopStyleColor();
        }

        CImGui::EndChild();
        CImGui::PopStyleVar();
        CImGui::PopStyleColor();
    }

}