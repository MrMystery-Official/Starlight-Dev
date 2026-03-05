#pragma once

#include <game/Scene.h>
#include <gl/Camera.h>
#include <gl/Shader.h>
#include <glm/vec3.hpp>
#include <util/Frustum.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <deque>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <play/CharacterController.h>
#include <play/GLDebugDrawer.h>
#include <optional>

#include <btBulletDynamicsCommon.h>

#include <BulletCollision/CollisionDispatch/btGhostObject.h>

#include <BulletDynamics/Character/btKinematicCharacterController.h>

namespace application::game::actor_component
{
	class ActorComponentShapeParam;
}

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
				bool mInWorld = false;
			};

			struct PendingTerrainBody
			{
				uint32_t mFileIndex = 0;
				uint32_t mMeshIndex = 0;
				glm::vec3 mCenter = glm::vec3(0.0f);
				glm::vec3 mOffset = glm::vec3(0.0f);
				std::optional<PlaySessionRigidBodyEntity> mBody;
			};

			struct AsyncActorCollisionData
			{
				btCollisionShape* mCollisionShape = nullptr;
				std::vector<btCollisionShape*> mOwnedShapes;
				std::vector<btTriangleMesh*> mOwnedMeshes;
				PlaySessionRigidBodyEntity mBody;
			};

			struct ActorCollisionJob
			{
				application::game::Scene::BancEntityRenderInfo* mEntity = nullptr;
				application::game::actor_component::ActorComponentShapeParam* mShapeParam = nullptr;
				glm::vec3 mTranslate = glm::vec3(0.0f);
				glm::vec3 mRotate = glm::vec3(0.0f);
				glm::vec3 mScale = glm::vec3(1.0f);
				float mDistanceSq = 0.0f;
				float mMass = 0.0f;
				bool mDynamic = false;
			};

			struct ActorCollisionJobPriority
			{
				bool operator()(const ActorCollisionJob& lhs, const ActorCollisionJob& rhs) const
				{
					return lhs.mDistanceSq > rhs.mDistanceSq;
				}
			};

			struct PlaySessionKinematicControllerEntity
			{
			btPairCachingGhostObject* mPlayerGhostObject = nullptr;
			btCapsuleShape* mPlayerCapsuleShape = nullptr;
			btKinematicCharacterController* mCharacterController = nullptr;
		};

			void DrawInstancedBancEntityRenderInfo(std::vector<application::game::Scene::BancEntityRenderInfo>& Entities);
			void UpdatePhysicsActivation(const glm::vec3& PlayerPosition);
			bool EnsureDynamicRigidBody(application::game::Scene::BancEntityRenderInfo* Entity);
			void EnsureTerrainRigidBody(PendingTerrainBody& TerrainBody);
			void SetRigidBodyActive(PlaySessionRigidBodyEntity& Body, bool Active);
			void QueueActorCollisionLoad(application::game::Scene::BancEntityRenderInfo* Entity);
			void FlushPendingActorCollisionRequests(const glm::vec3& PlayerPosition);
			void ConsumeReadyActorCollisionData();
			void ActorCollisionLoaderMain();
			std::optional<AsyncActorCollisionData> BuildActorCollisionData(const ActorCollisionJob& Job);
			void StartActorCollisionLoader();
			void StopActorCollisionLoader();

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
			std::vector<application::game::Scene::BancEntityRenderInfo*> mPhysicsCandidates;
			std::unordered_map<application::game::Scene::BancEntityRenderInfo*, float> mActorActivationRadius;
			std::vector<PendingTerrainBody> mPendingTerrainBodies;
			std::vector<btCollisionShape*> mShapesToDelete; //Pointers to the btCollisionShapes bound to a btCompoundShape
			std::unordered_map<application::game::Scene::BancEntityRenderInfo*, AsyncActorCollisionData> mActorCollisionReady;
			std::unordered_map<application::game::Scene::BancEntityRenderInfo*, AsyncActorCollisionData> mActorCollisionLoaded;
			std::unordered_set<application::game::Scene::BancEntityRenderInfo*> mActorCollisionQueued;
			std::unordered_set<application::game::Scene::BancEntityRenderInfo*> mActorCollisionPendingRequests;
			std::priority_queue<ActorCollisionJob, std::vector<ActorCollisionJob>, ActorCollisionJobPriority> mActorCollisionQueue;
			std::thread mActorCollisionLoaderThread;
			std::mutex mActorCollisionMutex;
			std::condition_variable mActorCollisionCv;
			std::atomic<bool> mActorCollisionStop{ false };

			float mPhysicsActivationRadius = 10.0f;
			float mTerrainActivationRadius = 220.0f;
			float mActivationHysteresis = 3.0f;
			float mActivationUpdateInterval = 0.05f;
			float mActivationUpdateTimer = 0.0f;
			uint32_t mMaxActorBodiesCreatedPerActivation = 12;
			uint32_t mMaxActorBodiesStateChangesPerActivation = 24;

			btDefaultCollisionConfiguration* mCollisionConfig = nullptr;
		btCollisionDispatcher* mDispatcher = nullptr;
		btBroadphaseInterface* mBroadphase = nullptr;
		btSequentialImpulseConstraintSolver* mSolver = nullptr;
		btDiscreteDynamicsWorld* mDynamicsWorld = nullptr;
		GLDebugDrawer* mDebugDrawer = nullptr;
	};
}
