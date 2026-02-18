#pragma once

#include <game/Scene.h>
#include <gl/Camera.h>
#include <gl/Shader.h>
#include <glm/vec3.hpp>
#include <util/Frustum.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <play/CharacterController.h>
#include <play/GLDebugDrawer.h>
#include <optional>

#include <btBulletDynamicsCommon.h>

#include <BulletCollision/CollisionDispatch/btGhostObject.h>

#include <BulletDynamics/Character/btKinematicCharacterController.h>

namespace application::play
{
	class PlaySession
	{
	public:
		PlaySession(application::gl::Shader* InstancedShader);

		void InitializeFromScene(application::game::Scene& Scene, const glm::vec3& StartPos);
		void Update(float dt);
		void Render();
		void Shutdown();

		application::gl::Camera mCamera;

	private:
		struct PlaySessionRigidBodyEntity
		{
			btCollisionShape* mCollisionShape = nullptr;
			btDefaultMotionState* mMotionState = nullptr;
			btRigidBody* mRigidBody = nullptr;
		};

		struct PlaySessionKinematicControllerEntity
		{
			btPairCachingGhostObject* mPlayerGhostObject = nullptr;
			btCapsuleShape* mPlayerCapsuleShape = nullptr;
			btKinematicCharacterController* mCharacterController = nullptr;
		};

		void DrawInstancedBancEntityRenderInfo(std::vector<application::game::Scene::BancEntityRenderInfo>& Entities);

		const glm::vec3 mLightColor = glm::vec3(1.0f, 1.0f, 1.0f);

		application::game::Scene mScene;
		application::gl::Shader* mInstancedShader = nullptr;
		application::gl::Shader* mDebugShader = nullptr;
		application::util::Frustum mFrustum;

		application::game::Scene::BancEntityRenderInfo* mPlayerEntityInfo = nullptr;
		PlaySessionKinematicControllerEntity mPlayerPhysicsEntity;
		float mPlayerCollisionHeightOffset = 0.0f;
		float mPlayerCurrentYaw = 0.0f;

		std::unordered_map<std::string, btTriangleMesh*> mCollisionMeshes; //PhiveShapeName -> btTriangleMesh*
		std::unordered_map<std::string, btCollisionShape*> mCollisionShapes; //PhiveShapeName -> btCollisionShape* (to share the same collision ptr between rigid bodies)
		std::unordered_map<application::game::Scene::BancEntityRenderInfo*, PlaySessionRigidBodyEntity> mRigidBodies;
		std::vector<PlaySessionRigidBodyEntity> mTerrainRigidBodies;
		std::vector<btCollisionShape*> mShapesToDelete; //Pointers to the btCollisionShapes bound to a btCompoundShape

		btDefaultCollisionConfiguration* mCollisionConfig = nullptr;
		btCollisionDispatcher* mDispatcher = nullptr;
		btBroadphaseInterface* mBroadphase = nullptr;
		btSequentialImpulseConstraintSolver* mSolver = nullptr;
		btDiscreteDynamicsWorld* mDynamicsWorld = nullptr;
		GLDebugDrawer* mDebugDrawer = nullptr;
	};
}