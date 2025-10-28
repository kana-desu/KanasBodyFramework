#pragma once

#include <kbf/data/formats/format_metadata.hpp>

#include <string>
#include <unordered_map>

namespace kbf {

    struct PlayerData {
        std::string name;
        std::string hunterId;
        bool female;
        FormatMetadata metadata;

        bool operator==(const PlayerData& other) const {
            return name == other.name &&
                   hunterId == other.hunterId &&
                   female == other.female;
        }

        std::string string() const {
            return name + " (" + (female ? "Female" : "Male") + " - " + hunterId + ")";
        }
    };

}

namespace std {
    template <>
    struct hash<kbf::PlayerData> {
        std::size_t operator()(const kbf::PlayerData& p) const {
            std::size_t h1 = std::hash<std::string>{}(p.name);
            std::size_t h2 = std::hash<std::string>{}(p.hunterId);
            std::size_t h3 = std::hash<bool>{}(p.female);

            // Combine the hashes (standard hash combine technique)
            std::size_t seed = h1;
            seed ^= h2 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= h3 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            return seed;
        }
    };
}