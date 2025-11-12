#pragma once

#include <string>
#include <filesystem>

namespace kbf {

    std::string getRelativeSubfolder(const std::filesystem::path& base, const std::filesystem::path& full) {
        auto rel = std::filesystem::relative(full.parent_path(), base);
        std::string folder = rel.string();
        std::replace(folder.begin(), folder.end(), '\\', '/'); // Normalize separators
        return folder;
    };

}