#pragma once

#include <optional>
#include <string>

namespace kbf {

    enum class HoverSource {
        None = 0,
        BonePanel,
        EditorTable
    };

    struct HoveredBone {
        std::optional<std::string> primary;   // main bone name
        std::optional<std::string> secondary; // optional second bone (symmetry)
        HoverSource source = HoverSource::None;
    };

}
