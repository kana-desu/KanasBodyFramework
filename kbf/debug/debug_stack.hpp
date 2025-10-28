#pragma once

#include <kbf/debug/log_data.hpp>

#include <deque>
#include <string>
#include <chrono>
#include <mutex>

#undef ERROR

namespace kbf {

    class DebugStack {
    public:
        DebugStack(size_t limit) : limit{ limit } {}

        void push(LogData logData) {
            std::lock_guard<std::mutex> lock(mux);
            stack.push_back(logData);

            // Inline this pop to prevent double mutex lock causing a deadlock.
            if (stack.size() > limit && !stack.empty()) {
                stack.pop_front();
            }
        }

        void push(std::string message, glm::vec3 colour = DebugStack::Color::DEBUG) {
            push(LogData{ message, colour, DebugStack::now() });
        }

        void pop() {
            std::lock_guard<std::mutex> lock(mux);
            if (!stack.empty()) {
                stack.pop_front();
            }
        }

        const LogData& peek() const {
            std::lock_guard<std::mutex> lock(mux);
            return stack.back();
        }

        void clear() {
            std::lock_guard<std::mutex> lock(mux);
            stack.clear();
        }

        bool empty() { 
            std::lock_guard<std::mutex> lock(mux);
            return stack.empty(); 
        }

        auto begin() { return stack.begin(); }
        auto end() { return stack.end(); }

        auto begin() const { return stack.begin(); }
        auto end() const { return stack.end(); }

        struct Color {
            static constexpr glm::vec3 ERROR   = glm::vec3(0.839f, 0.365f, 0.365f);  // #D65D5D
            static constexpr glm::vec3 WARNING = glm::vec3(0.902f, 0.635f, 0.235f);  // #E6A23C
            static constexpr glm::vec3 INFO    = glm::vec3(0.753f, 0.753f, 0.753f);  // #C0C0C0
            static constexpr glm::vec3 DEBUG   = glm::vec3(0.365f, 0.678f, 0.886f);  // #5DADE2
            static constexpr glm::vec3 SUCCESS = glm::vec3(0.451f, 0.776f, 0.424f);  // #73C66C		
        };

        static inline std::chrono::system_clock ::time_point now() noexcept { return std::chrono::system_clock::now(); }

        std::string string() const {
            std::string result;
            for (const auto& log : stack) {
                auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(log.timestamp.time_since_epoch()).count();

                std::time_t time_t_seconds = millis / 1000;
                std::tm local_tm;
#ifdef _WIN32
                localtime_s(&local_tm, &time_t_seconds);
#else
                localtime_r(&time_t_seconds, &local_tm);
#endif
                int milliseconds = millis % 1000;

                std::ostringstream oss;
                oss << std::setfill('0')
                    << std::setw(2) << local_tm.tm_hour << ":"
                    << std::setw(2) << local_tm.tm_min << ":"
                    << std::setw(2) << local_tm.tm_sec << ":"
                    << std::setw(4) << milliseconds;

				result += std::format("[{}] {}\n", oss.str(), log.data);
            }

			return result;
        }

    private:
        mutable std::mutex mux;
        size_t limit;
        std::deque<LogData> stack{};
    };

    inline DebugStack DEBUG_STACK{ 10000 };

}