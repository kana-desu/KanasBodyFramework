#include <unordered_map>
#include <string>
#include <utility> 
#include <functional>

namespace kbf {

    struct PairHash {
        template <typename T1, typename T2>
        size_t operator()(const std::pair<T1, T2>& p) const noexcept {
            size_t h1 = std::hash<T1>{}(p.first);
            size_t h2 = std::hash<T2>{}(p.second);
            // Combine hashes (boost style)
            return h1 ^ (h2 << 1);
        }
    };

}
