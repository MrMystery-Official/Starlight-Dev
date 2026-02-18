#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <btBulletDynamicsCommon.h>
#include <glm/vec3.hpp>

namespace application::play
{
	class CharacterController
	{
	public:
		void Initialize(btRigidBody* Body);

		void Update(const glm::vec3& cameraForward, const glm::vec3& cameraRight, GLFWwindow* window, btDiscreteDynamicsWorld* world, float deltaTime);
	private:
		btRigidBody* mBody = nullptr;
		float mWalkSpeed = 5.0f;        // horizontal speed
		float mAcceleration = 10.0f;    // smoothing factor
		float mJumpVelocity = 5.0f;
		bool mCanJump = false;
	};
}