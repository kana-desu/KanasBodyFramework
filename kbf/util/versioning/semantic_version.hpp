#pragma once

#include <cstdint>
#include <sstream>
#include <vector>

namespace kbf {

    class SemanticVersion {
    public:
        uint32_t major;
        uint32_t minor;
        uint32_t patch;

        SemanticVersion(uint32_t maj = 0, uint32_t min = 0, uint32_t pat = 0)
            : major(maj), minor(min), patch(pat) {
        }

        bool isZero() const { return major == 0 && minor == 0 && patch == 0; }
        static SemanticVersion currentVersion() { return SemanticVersion::fromString(KBF_VERSION); }

        // Parse from string like "1.2.3" or "1.0"
        static SemanticVersion fromString(const std::string& str) {
            std::istringstream iss(str);
            std::string token;
            std::vector<uint32_t> parts;

            while (std::getline(iss, token, '.')) {
                parts.push_back(static_cast<uint32_t>(std::stoul(token)));
            }

            while (parts.size() < 3) parts.push_back(0); // pad missing parts
            return SemanticVersion(parts[0], parts[1], parts[2]);
        }


        std::string toString() const {
            return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
		}

        // Convert to a sortable integer
        uint32_t toInt() const {
            return major * 1000000 + minor * 1000 + patch;
        }

        bool operator<(const SemanticVersion& other) const {
            return toInt() < other.toInt();
        }
        bool operator==(const SemanticVersion& other) const {
            return toInt() == other.toInt();
        }
    };

}