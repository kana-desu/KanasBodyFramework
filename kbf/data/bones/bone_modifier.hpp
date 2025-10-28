#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace kbf {

	class BoneModifier {
	public:
		BoneModifier() { calculateQuaternionRotation(); }
		BoneModifier(glm::vec3 scale, glm::vec3 position, glm::vec3 rotation)
			: scale{ scale }, position{ position }, rotation{ rotation } { calculateQuaternionRotation(); };

		glm::vec3 scale{ 0.0f, 0.0f, 0.0f };
		glm::vec3 position{ 0.0f, 0.0f, 0.0f };

		bool operator==(const BoneModifier& other) const {
			return (
				scale == other.scale &&
				position == other.position &&
				rotation == other.rotation
			);
		}

		inline bool hasScale() const {
			return glm::any(glm::notEqual(scale, glm::vec3(0.0f)));
		}

		inline bool hasPosition() const {
			return glm::any(glm::notEqual(position, glm::vec3(0.0f)));
		}

		inline bool hasRotation() const {
			return glm::any(glm::notEqual(rotation, glm::vec3(0.0f)));
		}

		glm::vec3 getReflectedScale() const {
			return glm::vec3(scale.x, scale.y, scale.z);
		}

		glm::vec3 getReflectedPosition() const {
			return glm::vec3(-position.x, position.y, position.z);
		}

		glm::vec3 getReflectedRotation() const {
			return glm::vec3(rotation.x, -rotation.y, -rotation.z);
		}

		BoneModifier reflect() const {
			return BoneModifier{
				getReflectedScale(),
				getReflectedPosition(),
				getReflectedRotation()
			};
		}

		const glm::vec3& getRotation() const { return rotation; }
		void setRotation(const glm::vec3& rot) { rotation = rot; calculateQuaternionRotation(); }

		const glm::fquat& getQuaternionRotation() const { return quatRotation; }

	private:
		glm::vec3 rotation{ 0.0f, 0.0f, 0.0f };
		glm::fquat quatRotation{ 1.0f, 0.0f, 0.0f, 0.0f };

		void calculateQuaternionRotation() { 
			if (rotation.x == 0.0f && rotation.y == 0.0f && rotation.z == 0.0f) {
				quatRotation = glm::quat{ 1.0f, 0.0f, 0.0f, 0.0f };
			}
			else {
				quatRotation = glm::quat{ rotation }; 
			}
		}
	};

}