#include "kbf/situation/camera_manager.hpp"
#include <kbf/debug/debug_stack.hpp>
#include <kbf/util/re_engine/reinvoke.hpp>

#define CAMERA_MANAGER_LOG_TAG "[CameraManager]"

namespace kbf {

    CameraManager& CameraManager::instance() {
        static CameraManager inst;
        return inst;
    }

    std::optional<glm::vec3> CameraManager::getPosition() {
        static auto sceneManagerTD = REApi::get()->tdb()->find_type("via.SceneManager");
        static auto getMainViewTD = sceneManagerTD->find_method("get_MainView");

        REApi::ManagedObject* mainView = getMainViewTD->call<REApi::ManagedObject*>(REApi::get()->get_vm_context(), sceneManager);
        if (!mainView) return std::nullopt;

        REApi::ManagedObject* camera = REInvokePtr<REApi::ManagedObject>(mainView, "get_PrimaryCamera", {});
        if (!camera) return std::nullopt;

        glm::mat4 worldMatrix = REInvoke<glm::mat4>(camera, "get_WorldMatrix", {}, InvokeReturnType::BYTES);

        // Extract position from the 4th column of the world matrix
        return glm::vec3(worldMatrix[3]);
    }

    std::optional<glm::mat4> CameraManager::getViewProjMatrix() {
        static auto sceneManagerTD = REApi::get()->tdb()->find_type("via.SceneManager");
        static auto getMainViewTD = sceneManagerTD->find_method("get_MainView");

        REApi::ManagedObject* mainView = getMainViewTD->call<REApi::ManagedObject*>(REApi::get()->get_vm_context(), sceneManager);
        if (!mainView) return std::nullopt;

        REApi::ManagedObject* camera = REInvokePtr<REApi::ManagedObject>(mainView, "get_PrimaryCamera", {});
        if (!camera) return std::nullopt;

        glm::mat4 viewProjMat = REInvoke<glm::mat4>(camera, "get_ViewProjMatrix", {}, InvokeReturnType::BYTES);

        return viewProjMat;
    }

    std::optional<glm::vec2> CameraManager::worldToScreen(const glm::vec3& worldPos, const glm::uvec2& displayResolution) {
        auto mOpt = getViewProjMatrix();
        return mOpt.has_value() 
            ? worldToScreen(worldPos, *mOpt, displayResolution) 
            : std::nullopt;
    }

    std::optional<glm::vec2> CameraManager::worldToScreen(const glm::vec3& worldPos, const glm::mat4& viewProjMatrix, const glm::uvec2& displayResolution) {
        glm::vec4 clip = viewProjMatrix * glm::vec4(worldPos, 1.0f);
        if (clip.w == 0.0f) return std::nullopt;

        float ndcX = clip.x / clip.w;
        float ndcY = clip.y / clip.w;
        float ndcZ = clip.z / clip.w;

        if (ndcZ < -1.0f || ndcZ > 1.0f) return std::nullopt;

        glm::vec2 screen;
        screen.x = (ndcX * 0.5f + 0.5f) * static_cast<float>(displayResolution.x);
        screen.y = (1.0f - (ndcY * 0.5f + 0.5f)) * static_cast<float>(displayResolution.y);
        return screen;
    }

}
