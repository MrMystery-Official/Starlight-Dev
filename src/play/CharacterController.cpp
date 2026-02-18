#include <play/CharacterController.h>

#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>

namespace application::play
{
	void CharacterController::Initialize(btRigidBody* Body)
	{
		mBody = Body;
	}

	void CharacterController::Update(const glm::vec3& cameraForward, const glm::vec3& cameraRight, GLFWwindow* window, btDiscreteDynamicsWorld* world, float deltaTime)
	{
        if (!mBody) return;

        glm::vec3 inputDir(0.0f);

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) inputDir += cameraForward;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) inputDir -= cameraForward;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) inputDir -= cameraRight;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) inputDir += cameraRight;

        if (glm::length2(inputDir) > 0.0f)
            inputDir = glm::normalize(inputDir);

        btVector3 vel = mBody->getLinearVelocity();
        glm::vec3 horizontalVel(vel.getX(), 0.0f, vel.getZ());

        glm::vec3 desiredVel = inputDir * mWalkSpeed;
        glm::vec3 newVel = glm::mix(horizontalVel, desiredVel, 1.0f - exp(-mAcceleration * deltaTime));

        vel.setX(newVel.x);
        vel.setZ(newVel.z);

        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && mCanJump)
        {
            vel.setY(mJumpVelocity);
            mCanJump = false;
        }

        mBody->setLinearVelocity(vel);

        btTransform t;
        mBody->getMotionState()->getWorldTransform(t);
        btVector3 from = t.getOrigin();
        btVector3 to = from - btVector3(0, 0.1f + 0.01f, 0); // 10 cm below
        btCollisionWorld::ClosestRayResultCallback ray(from, to);
        world->rayTest(from, to, ray);
        mCanJump = ray.hasHit();
	}
}