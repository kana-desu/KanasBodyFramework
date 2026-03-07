#pragma once

#include <reframework/API.hpp>
#include <kbf/util/re_engine/re_singleton.hpp>
#include <kbf/util/re_engine/reinvoke.hpp>
#include <glm/glm.hpp>
#include <optional>

namespace kbf {

    class CameraManager {
    public:
        // Singleton access
        static CameraManager& instance();

        CameraManager(const CameraManager&) = delete;
        CameraManager& operator=(const CameraManager&) = delete;

        std::optional<glm::vec3> getPosition();
        std::optional<glm::mat4> getViewProjMatrix();
        std::optional<glm::vec2> worldToScreen(const glm::vec3& worldPos, const glm::uvec2& displayResolution);
        std::optional<glm::vec2> worldToScreen(const glm::vec3& worldPos, const glm::mat4& viewProjMatrix, const glm::uvec2& displayResolution);

    private:
        CameraManager() = default;

        RENativeSingleton sceneManager{ "via.SceneManager" };
    };

}
