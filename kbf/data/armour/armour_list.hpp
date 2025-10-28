#pragma once

#include <kbf/data/armour/armour_id.hpp>
#include <kbf/data/armour/armour_set.hpp>
#include <kbf/data/armour/armour_piece.hpp>
#include <kbf/data/ids/special_armour_ids.hpp>

#include <vector>
#include <string>
#include <map>

namespace kbf {

    typedef std::map<ArmourSet, ArmourID> ArmourMapping;

    class ArmourList {
    public:
        static std::vector<ArmourSet> getFilteredSets(const std::string& filter);
        static bool isValidArmourSet(const std::string& name, bool female);
        static ArmourSet getArmourSetFromId(const std::string& id, ArmourPiece* pieceFoundOut = nullptr);
        static ArmourID getArmourIdFromSet(const ArmourSet& set);
        static std::string getArmourId(const ArmourSet& set, ArmourPiece piece, bool female);

        static ArmourSet DefaultArmourSet() { return ArmourSet{ ANY_ARMOUR_ID, false }; }

        const static ArmourMapping FALLBACK_MAPPING;
        static ArmourMapping ACTIVE_MAPPING;

    };

}