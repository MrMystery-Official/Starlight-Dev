#include <play/PlaySession.h>

#include "imgui.h"
#include <util/Math.h>
#include <util/Logger.h>
#include <util/FileUtil.h>
#include <manager/UIMgr.h>
#include <manager/ShaderMgr.h>
#include <manager/ActorInfoMgr.h>

#include <file/game/zstd/ZStdBackend.h>
#include <file/game/phive/shape/PhiveShape.h>
#include <game/actor_component/ActorComponentShapeParam.h>
#include <tool/scene/static_compound/StaticCompoundImplementationFieldScene.h>
#include <glm/geometric.hpp>
#include <algorithm>

namespace application::play
{
	PlaySession::PlaySession(application::gl::Shader* InstancedShader) : mInstancedShader(InstancedShader)
	{
        mCamera.mWindow = application::manager::UIMgr::gWindow;

		mCamera.SetMode(application::gl::Camera::CameraMode::THIRD_PERSON);
		mCamera.SetTarget(glm::vec3(0, 0, 0)); //Default target at origin
		mCamera.mPosition = glm::vec3(0, 0, 0);
	}

	void PlaySession::InitializeFromScene(application::game::Scene& Scene, const glm::vec3& StartPos)
	{
        mScene.Import(Scene);

        mPlayerEntityInfo = mScene.AddBancEntity("Player");
        mPlayerEntityInfo->mTranslate = StartPos;
        mScene.SyncBancEntity(mPlayerEntityInfo);

        mCamera.SetTarget(StartPos + glm::vec3(0, 2.0f, 0));
        mCamera.mPosition = StartPos + glm::vec3(0, 2.0f, -6.0);
        mCamera.mYaw = 90.0f;


        mCollisionConfig = new btDefaultCollisionConfiguration();
        mDispatcher = new btCollisionDispatcher(mCollisionConfig);
        mBroadphase = new btDbvtBroadphase();

        btGhostPairCallback* ghostCallback = new btGhostPairCallback();
        mBroadphase->getOverlappingPairCache()->setInternalGhostPairCallback(ghostCallback);

        mSolver = new btSequentialImpulseConstraintSolver();

        mDynamicsWorld = new btDiscreteDynamicsWorld(
            mDispatcher, mBroadphase, mSolver, mCollisionConfig
        );
        mDynamicsWorld->setGravity(btVector3(0, -9.81f, 0));

        mDebugShader = application::manager::ShaderMgr::GetShader("NavMesh");
        mDebugDrawer = new GLDebugDrawer();

        mDebugDrawer->setDebugMode(
            btIDebugDraw::DBG_DrawWireframe
        );

        //mDynamicsWorld->setDebugDrawer(mDebugDrawer);

        //Values taken from Player.pack.zs, proud of me :)
        float capsuleRadius = 0.35f;
        float capsuleHeight = 1.8f - 2 * capsuleRadius;
        float mass = 10.0f;

        mPlayerCollisionHeightOffset = capsuleHeight / 2 + capsuleRadius;

        mPlayerPhysicsEntity.mPlayerCapsuleShape = new btCapsuleShapeZ(capsuleRadius, capsuleHeight);
        mPlayerPhysicsEntity.mPlayerGhostObject = new btPairCachingGhostObject();

        btTransform startTransform;
        startTransform.setIdentity();
        startTransform.setOrigin(btVector3(
            StartPos.x,
            StartPos.y + mPlayerCollisionHeightOffset,
            StartPos.z
        ));

        mPlayerPhysicsEntity.mPlayerGhostObject->setWorldTransform(startTransform);
        mPlayerPhysicsEntity.mPlayerGhostObject->setCollisionShape(mPlayerPhysicsEntity.mPlayerCapsuleShape);
        mPlayerPhysicsEntity.mPlayerGhostObject->setCollisionFlags(
            btCollisionObject::CF_CHARACTER_OBJECT
        );

        // Character controller
        mPlayerPhysicsEntity.mCharacterController = new btKinematicCharacterController(
            mPlayerPhysicsEntity.mPlayerGhostObject,
            mPlayerPhysicsEntity.mPlayerCapsuleShape,
            0.35f,
            btVector3(0, 1, 0) // up axis (Y)
        );

        //mPlayerPhysicsEntity.mCharacterController->setGravity(btVector3(0, -9.81f, 0));
        //mPlayerPhysicsEntity.mCharacterController->setMaxSlope(btRadians(45.0f));
        //mPlayerPhysicsEntity.mCharacterController->setFallSpeed(btRadians(45.0f));
        mPlayerPhysicsEntity.mCharacterController->setUseGhostSweepTest(true);

        // Register with world
        mDynamicsWorld->addCollisionObject(
            mPlayerPhysicsEntity.mPlayerGhostObject,
            btBroadphaseProxy::CharacterFilter,
            btBroadphaseProxy::StaticFilter |
            btBroadphaseProxy::DefaultFilter |
            btBroadphaseProxy::KinematicFilter
        );

        mDynamicsWorld->addAction(mPlayerPhysicsEntity.mCharacterController);

        if (mScene.IsTerrainScene() && mScene.mStaticCompoundImplementation != nullptr)
        {
            application::tool::scene::static_compound::StaticCompoundImplementationFieldScene* staticCompoundImpl =
                static_cast<application::tool::scene::static_compound::StaticCompoundImplementationFieldScene*>(mScene.mStaticCompoundImplementation.get());

            mPendingTerrainBodies.clear();
            mPendingTerrainBodies.reserve(128);

            for (uint32_t fileIndex = 0; fileIndex < staticCompoundImpl->mStaticCompoundFiles.size(); ++fileIndex)
            {
                application::file::game::phive::PhiveStaticCompoundFile& file = staticCompoundImpl->mStaticCompoundFiles[fileIndex];
                const glm::vec3 tileOffset = staticCompoundImpl->GetStaticCompoundMiddlePoint(fileIndex % 4, fileIndex / 4);

                for (uint32_t meshIndex = 0; meshIndex < file.mExternalBphshMeshes.size(); ++meshIndex)
                {
                    PendingTerrainBody pendingBody;
                    pendingBody.mFileIndex = fileIndex;
                    pendingBody.mMeshIndex = meshIndex;
                    pendingBody.mOffset = tileOffset;
                    pendingBody.mCenter = tileOffset + glm::vec3(125.0f, 0.0f, 125.0f);
                    mPendingTerrainBodies.push_back(pendingBody);
                }
            }
        }

        mPhysicsCandidates.clear();
        mActorActivationRadius.clear();
        for (application::game::Scene::BancEntityRenderInfo* Entity : mScene.mDrawListRenderInfoIndices)
        {
            application::game::actor_component::ActorComponentShapeParam* ShapeParam = static_cast<application::game::actor_component::ActorComponentShapeParam*>(Entity->mEntity->mActorPack->GetComponent(application::game::actor_component::ActorComponentBase::ComponentType::SHAPE_PARAM));

            if (!ShapeParam)
                continue;

            if (ShapeParam->mPhiveShapeParam.IsEmpty())
                continue;

            if (Entity == mPlayerEntityInfo)
                continue;

            mPhysicsCandidates.push_back(Entity);

            float activationRadius = mPhysicsActivationRadius;
            if (Entity->mEntity != nullptr)
            {
                if (application::manager::ActorInfoMgr::ActorInfoEntry* actorInfo = application::manager::ActorInfoMgr::GetActorInfo(Entity->mEntity->mGyml);
                    actorInfo != nullptr && actorInfo->mLoadRadius.has_value() && actorInfo->mLoadRadius.value() > 0.0f)
                {
                    activationRadius = actorInfo->mLoadRadius.value();
                }
            }
            mActorActivationRadius[Entity] = activationRadius;
        }

        StartActorCollisionLoader();
        for (application::game::Scene::BancEntityRenderInfo* entity : mPhysicsCandidates)
        {
            QueueActorCollisionLoad(entity);
        }
        FlushPendingActorCollisionRequests(StartPos);

        mActivationUpdateTimer = mActivationUpdateInterval;
        UpdatePhysicsActivation(StartPos);
	}

    void PlaySession::SetRigidBodyActive(PlaySessionRigidBodyEntity& Body, bool Active)
    {
        if (Body.mRigidBody == nullptr)
            return;

        if (Active)
        {
            if (!Body.mInWorld)
            {
                mDynamicsWorld->addRigidBody(Body.mRigidBody);
                Body.mInWorld = true;
            }
        }
        else if (Body.mInWorld)
        {
            mDynamicsWorld->removeRigidBody(Body.mRigidBody);
            Body.mInWorld = false;
        }
    }

    void PlaySession::EnsureTerrainRigidBody(PendingTerrainBody& TerrainBody)
    {
        if (TerrainBody.mBody.has_value())
            return;

        if (mScene.mStaticCompoundImplementation == nullptr)
            return;

        application::tool::scene::static_compound::StaticCompoundImplementationFieldScene* staticCompoundImpl =
            static_cast<application::tool::scene::static_compound::StaticCompoundImplementationFieldScene*>(mScene.mStaticCompoundImplementation.get());

        if (TerrainBody.mFileIndex >= staticCompoundImpl->mStaticCompoundFiles.size())
            return;

        application::file::game::phive::PhiveStaticCompoundFile& file = staticCompoundImpl->mStaticCompoundFiles[TerrainBody.mFileIndex];
        if (TerrainBody.mMeshIndex >= file.mExternalBphshMeshes.size())
            return;

        auto& externalBphshMesh = file.mExternalBphshMeshes[TerrainBody.mMeshIndex];
        std::vector<float> vertices = externalBphshMesh.mPhiveShape.ToVertices();
        std::vector<uint32_t> indices = externalBphshMesh.mPhiveShape.ToIndices();

        btTriangleMesh* mesh = new btTriangleMesh();
        for (size_t i = 0; i + 2 < indices.size(); i += 3)
        {
            mesh->addTriangle(
                btVector3(vertices[indices[i + 0] * 3], vertices[indices[i + 0] * 3 + 1], vertices[indices[i + 0] * 3 + 2]),
                btVector3(vertices[indices[i + 1] * 3], vertices[indices[i + 1] * 3 + 1], vertices[indices[i + 1] * 3 + 2]),
                btVector3(vertices[indices[i + 2] * 3], vertices[indices[i + 2] * 3 + 1], vertices[indices[i + 2] * 3 + 2]),
                true
            );
        }

        const std::string id = "terrain_" + std::to_string(TerrainBody.mFileIndex) + "_" + std::to_string(TerrainBody.mMeshIndex);
        mCollisionMeshes[id] = mesh;

        PlaySessionRigidBodyEntity physicsEntity;
        mShapesToDelete.push_back(new btBvhTriangleMeshShape(mesh, true));
        physicsEntity.mCollisionShape = mShapesToDelete.back();

        btTransform motionState;
        motionState.setIdentity();
        motionState.setOrigin(btVector3(TerrainBody.mOffset.x, 0.5f, TerrainBody.mOffset.z));
        physicsEntity.mMotionState = new btDefaultMotionState(motionState);

        btVector3 inertia(0, 0, 0);
        btRigidBody::btRigidBodyConstructionInfo body(0.0f, physicsEntity.mMotionState, physicsEntity.mCollisionShape, inertia);
        physicsEntity.mRigidBody = new btRigidBody(body);
        physicsEntity.mInWorld = false;

        TerrainBody.mBody = physicsEntity;
    }

    void PlaySession::QueueActorCollisionLoad(application::game::Scene::BancEntityRenderInfo* Entity)
    {
        if (Entity == nullptr)
            return;

        mActorCollisionPendingRequests.insert(Entity);
    }

    void PlaySession::FlushPendingActorCollisionRequests(const glm::vec3& PlayerPosition)
    {
        if (mActorCollisionPendingRequests.empty())
            return;

        std::unique_lock<std::mutex> lock(mActorCollisionMutex, std::try_to_lock);
        if (!lock.owns_lock())
            return;

        bool notifyWorker = false;
        for (auto it = mActorCollisionPendingRequests.begin(); it != mActorCollisionPendingRequests.end();)
        {
            application::game::Scene::BancEntityRenderInfo* entity = *it;
            if (entity == nullptr ||
                entity->mEntity == nullptr ||
                entity->mEntity->mActorPack == nullptr ||
                mActorCollisionQueued.contains(entity) ||
                mActorCollisionReady.contains(entity) ||
                mActorCollisionLoaded.contains(entity))
            {
                it = mActorCollisionPendingRequests.erase(it);
                continue;
            }

            application::game::actor_component::ActorComponentShapeParam* shapeParam =
                static_cast<application::game::actor_component::ActorComponentShapeParam*>(
                    entity->mEntity->mActorPack->GetComponent(application::game::actor_component::ActorComponentBase::ComponentType::SHAPE_PARAM)
                );
            if (shapeParam == nullptr || shapeParam->mPhiveShapeParam.IsEmpty())
            {
                it = mActorCollisionPendingRequests.erase(it);
                continue;
            }

            ActorCollisionJob job;
            job.mEntity = entity;
            job.mShapeParam = shapeParam;
            job.mTranslate = entity->mTranslate;
            job.mRotate = entity->mRotate;
            job.mScale = entity->mScale;
            const glm::vec3 delta = entity->mTranslate - PlayerPosition;
            job.mDistanceSq = glm::dot(delta, delta);
            job.mMass = entity->mEntity->mActorPack->mMass;
            job.mDynamic = (entity->mEntity->mActorPack->mNeedsPhysicsHash && entity->mEntity->mActorPack->mMass > 0.0f);

            mActorCollisionQueue.push(job);
            mActorCollisionQueued.insert(entity);
            notifyWorker = true;
            it = mActorCollisionPendingRequests.erase(it);
        }

        if (notifyWorker)
            mActorCollisionCv.notify_one();
    }

    void PlaySession::ConsumeReadyActorCollisionData()
    {
        std::unique_lock<std::mutex> lock(mActorCollisionMutex, std::try_to_lock);
        if (!lock.owns_lock() || mActorCollisionReady.empty())
            return;

        for (auto it = mActorCollisionReady.begin(); it != mActorCollisionReady.end();)
        {
            mActorCollisionLoaded[it->first] = std::move(it->second);
            it = mActorCollisionReady.erase(it);
        }
    }

    std::optional<PlaySession::AsyncActorCollisionData> PlaySession::BuildActorCollisionData(const ActorCollisionJob& Job)
    {
        if (Job.mEntity == nullptr || Job.mShapeParam == nullptr)
            return std::nullopt;

        application::game::actor_component::ActorComponentShapeParam* shapeParam = Job.mShapeParam;
        if (shapeParam->mPhiveShapeParam.IsEmpty())
            return std::nullopt;

        AsyncActorCollisionData data;
        std::vector<btCollisionShape*> collisionShapes;
        collisionShapes.reserve(
            shapeParam->mPhiveShapeParam.mPhiveShapes.size() +
            shapeParam->mPhiveShapeParam.mBoxes.size() +
            shapeParam->mPhiveShapeParam.mSpheres.size() +
            shapeParam->mPhiveShapeParam.mPolytopes.size()
        );

        for (const std::string& pathName : shapeParam->mPhiveShapeParam.mPhiveShapes)
        {
            application::file::game::phive::shape::PhiveShape shape(
                application::file::game::ZStdBackend::Decompress(
                    application::util::FileUtil::GetRomFSFilePath("Phive/Shape/Dcc/" + pathName + ".Nin_NX_NVN.bphsh.zs")
                )
            );

            std::vector<float> vertices = shape.ToVertices();
            std::vector<uint32_t> indices = shape.ToIndices();

            btTriangleMesh* mesh = new btTriangleMesh();
            for (size_t i = 0; i + 2 < indices.size(); i += 3)
            {
                mesh->addTriangle(
                    btVector3(vertices[indices[i + 0] * 3], vertices[indices[i + 0] * 3 + 1], vertices[indices[i + 0] * 3 + 2]),
                    btVector3(vertices[indices[i + 1] * 3], vertices[indices[i + 1] * 3 + 1], vertices[indices[i + 1] * 3 + 2]),
                    btVector3(vertices[indices[i + 2] * 3], vertices[indices[i + 2] * 3 + 1], vertices[indices[i + 2] * 3 + 2]),
                    true
                );
            }

            btCollisionShape* meshShape = new btBvhTriangleMeshShape(mesh, true);
            data.mOwnedMeshes.push_back(mesh);
            data.mOwnedShapes.push_back(meshShape);
            collisionShapes.push_back(meshShape);
        }

        for (application::game::actor_component::ActorComponentShapeParam::PhiveShapeParam::Box& box : shapeParam->mPhiveShapeParam.mBoxes)
        {
            btVector3 halfExtents(box.mHalfExtents.x, box.mHalfExtents.y, box.mHalfExtents.z);
            btBoxShape* boxShape = new btBoxShape(halfExtents);
            btCompoundShape* compoundShape = new btCompoundShape();

            btTransform localTransform;
            localTransform.setIdentity();
            localTransform.setOrigin(btVector3(box.mOffsetTranslation.x + box.mCenter.x, box.mOffsetTranslation.y + box.mCenter.y, box.mOffsetTranslation.z + box.mCenter.z));
            compoundShape->addChildShape(localTransform, boxShape);

            data.mOwnedShapes.push_back(boxShape);
            data.mOwnedShapes.push_back(compoundShape);
            collisionShapes.push_back(compoundShape);
        }

        for (application::game::actor_component::ActorComponentShapeParam::PhiveShapeParam::Sphere& sphere : shapeParam->mPhiveShapeParam.mSpheres)
        {
            btSphereShape* sphereShape = new btSphereShape(sphere.mRadius);
            btCompoundShape* compoundShape = new btCompoundShape();

            btTransform localTransform;
            localTransform.setIdentity();
            localTransform.setOrigin(btVector3(sphere.mCenter.x, sphere.mCenter.y, sphere.mCenter.z));
            compoundShape->addChildShape(localTransform, sphereShape);

            data.mOwnedShapes.push_back(sphereShape);
            data.mOwnedShapes.push_back(compoundShape);
            collisionShapes.push_back(compoundShape);
        }

        for (application::game::actor_component::ActorComponentShapeParam::PhiveShapeParam::Polytope& polytope : shapeParam->mPhiveShapeParam.mPolytopes)
        {
            btConvexHullShape* convexHull = new btConvexHullShape();
            for (size_t i = 0; i < polytope.mVertices.size(); i++)
            {
                convexHull->addPoint(btVector3(polytope.mVertices[i].x, polytope.mVertices[i].y, polytope.mVertices[i].z), false);
            }
            convexHull->recalcLocalAabb();
            convexHull->optimizeConvexHull();

            btCompoundShape* compoundShape = new btCompoundShape();
            btTransform localTransform;
            localTransform.setIdentity();
            localTransform.setOrigin(btVector3(polytope.mOffsetTranslation.x, polytope.mOffsetTranslation.y, polytope.mOffsetTranslation.z));
            compoundShape->addChildShape(localTransform, convexHull);

            data.mOwnedShapes.push_back(convexHull);
            data.mOwnedShapes.push_back(compoundShape);
            collisionShapes.push_back(compoundShape);
        }

        if (collisionShapes.empty())
            return std::nullopt;

        if (collisionShapes.size() == 1)
        {
            data.mCollisionShape = collisionShapes[0];
        }
        else
        {
            btCompoundShape* compoundShape = new btCompoundShape();
            for (btCollisionShape* shape : collisionShapes)
            {
                btTransform origin;
                origin.setIdentity();
                origin.setOrigin(btVector3(0, 0, 0));
                compoundShape->addChildShape(origin, shape);
            }
            data.mOwnedShapes.push_back(compoundShape);
            data.mCollisionShape = compoundShape;
        }

        data.mCollisionShape->setLocalScaling(btVector3(Job.mScale.x, Job.mScale.y, Job.mScale.z));

        PlaySessionRigidBodyEntity prebuiltBody;
        prebuiltBody.mCollisionShape = data.mCollisionShape;

        btQuaternion quat;
        quat.setEulerZYX(glm::radians(Job.mRotate.z), glm::radians(Job.mRotate.y), glm::radians(Job.mRotate.x));

        btTransform motionState;
        motionState.setIdentity();
        motionState.setOrigin(btVector3(Job.mTranslate.x, Job.mTranslate.y, Job.mTranslate.z));
        motionState.setRotation(quat);
        prebuiltBody.mMotionState = new btDefaultMotionState(motionState);

        btVector3 inertia(0, 0, 0);
        if (Job.mDynamic)
            prebuiltBody.mCollisionShape->calculateLocalInertia(Job.mMass, inertia);

        btRigidBody::btRigidBodyConstructionInfo body(
            Job.mDynamic ? Job.mMass : 0.0f,
            prebuiltBody.mMotionState,
            prebuiltBody.mCollisionShape,
            inertia
        );
        prebuiltBody.mRigidBody = new btRigidBody(body);
        prebuiltBody.mInWorld = false;
        data.mBody = prebuiltBody;

        return data;
    }

    void PlaySession::ActorCollisionLoaderMain()
    {
        while (true)
        {
            ActorCollisionJob job;
            {
                std::unique_lock<std::mutex> lock(mActorCollisionMutex);
                mActorCollisionCv.wait(lock, [this]() {
                    return mActorCollisionStop.load() || !mActorCollisionQueue.empty();
                });

                if (mActorCollisionStop.load() && mActorCollisionQueue.empty())
                    return;

                job = mActorCollisionQueue.top();
                mActorCollisionQueue.pop();
                mActorCollisionQueued.erase(job.mEntity);
            }

            std::optional<AsyncActorCollisionData> builtData = BuildActorCollisionData(job);
            if (!builtData.has_value())
                continue;

            std::lock_guard<std::mutex> lock(mActorCollisionMutex);
            mActorCollisionReady[job.mEntity] = std::move(*builtData);
        }
    }

    void PlaySession::StartActorCollisionLoader()
    {
        mActorCollisionStop.store(false);
        if (mActorCollisionLoaderThread.joinable())
            return;
        mActorCollisionLoaderThread = std::thread(&PlaySession::ActorCollisionLoaderMain, this);
    }

    void PlaySession::StopActorCollisionLoader()
    {
        mActorCollisionStop.store(true);
        mActorCollisionCv.notify_all();
        if (mActorCollisionLoaderThread.joinable())
            mActorCollisionLoaderThread.join();
    }

    bool PlaySession::EnsureDynamicRigidBody(application::game::Scene::BancEntityRenderInfo* Entity)
    {
        if (Entity == nullptr)
            return false;
        if (mRigidBodies.contains(Entity))
            return false;

        if (!mActorCollisionLoaded.contains(Entity))
        {
            QueueActorCollisionLoad(Entity);
            return false;
        }

        AsyncActorCollisionData& loadedCollision = mActorCollisionLoaded[Entity];
        if (loadedCollision.mCollisionShape == nullptr ||
            loadedCollision.mBody.mCollisionShape == nullptr ||
            loadedCollision.mBody.mMotionState == nullptr ||
            loadedCollision.mBody.mRigidBody == nullptr)
            return false;

        // Keep ownership in loadedCollision; move only runtime rigid body pointers to active map.
        mRigidBodies[Entity] = loadedCollision.mBody;
        loadedCollision.mBody = {};

        btQuaternion quat;
        quat.setEulerZYX(glm::radians(Entity->mRotate.z), glm::radians(Entity->mRotate.y), glm::radians(Entity->mRotate.x));

        btTransform motionState;
        motionState.setIdentity();
        motionState.setOrigin(btVector3(Entity->mTranslate.x, Entity->mTranslate.y, Entity->mTranslate.z));
        motionState.setRotation(quat);
        mRigidBodies[Entity].mRigidBody->setWorldTransform(motionState);
        mRigidBodies[Entity].mMotionState->setWorldTransform(motionState);
        mRigidBodies[Entity].mRigidBody->setInterpolationWorldTransform(motionState);
        mRigidBodies[Entity].mInWorld = false;
        return true;
    }

    void PlaySession::UpdatePhysicsActivation(const glm::vec3& PlayerPosition)
    {
        uint32_t createdActorBodies = 0;
        uint32_t actorStateChanges = 0;

        struct ActorActivationCandidate
        {
            application::game::Scene::BancEntityRenderInfo* mEntity = nullptr;
            float mDistanceSq = 0.0f;
        };

        std::vector<ActorActivationCandidate> addCandidates;
        std::vector<ActorActivationCandidate> removeCandidates;
        addCandidates.reserve(mPhysicsCandidates.size());
        removeCandidates.reserve(mPhysicsCandidates.size());

        FlushPendingActorCollisionRequests(PlayerPosition);
        ConsumeReadyActorCollisionData();

        for (application::game::Scene::BancEntityRenderInfo* entity : mPhysicsCandidates)
        {
            if (entity == nullptr)
                continue;
            if (entity == mPlayerEntityInfo)
                continue;

            float activationRadius = mPhysicsActivationRadius;
            if (auto it = mActorActivationRadius.find(entity); it != mActorActivationRadius.end() && it->second > 0.0f)
                activationRadius = it->second;

            const float addRadiusSq = activationRadius * activationRadius;
            const float removeRadius = activationRadius + mActivationHysteresis;
            const float removeRadiusSq = removeRadius * removeRadius;

            const glm::vec3 delta = entity->mTranslate - PlayerPosition;
            const float distanceSq = glm::dot(delta, delta);

            if (distanceSq <= addRadiusSq)
            {
                addCandidates.push_back({ entity, distanceSq });
            }
            else if (mRigidBodies.contains(entity) && distanceSq >= removeRadiusSq)
            {
                removeCandidates.push_back({ entity, distanceSq });
            }
        }

        std::sort(addCandidates.begin(), addCandidates.end(),
            [](const ActorActivationCandidate& lhs, const ActorActivationCandidate& rhs) {
                return lhs.mDistanceSq < rhs.mDistanceSq;
            });
        std::sort(removeCandidates.begin(), removeCandidates.end(),
            [](const ActorActivationCandidate& lhs, const ActorActivationCandidate& rhs) {
                return lhs.mDistanceSq > rhs.mDistanceSq;
            });

        for (const ActorActivationCandidate& candidate : addCandidates)
        {
            application::game::Scene::BancEntityRenderInfo* entity = candidate.mEntity;
            if (entity == nullptr)
                continue;

            if (!mRigidBodies.contains(entity))
            {
                if (createdActorBodies < mMaxActorBodiesCreatedPerActivation)
                {
                    if (EnsureDynamicRigidBody(entity))
                        createdActorBodies++;
                }
                else
                {
                    QueueActorCollisionLoad(entity);
                }
            }

            if (mRigidBodies.contains(entity))
            {
                PlaySessionRigidBodyEntity& body = mRigidBodies[entity];
                if (!body.mInWorld && actorStateChanges < mMaxActorBodiesStateChangesPerActivation)
                {
                    SetRigidBodyActive(body, true);
                    actorStateChanges++;
                }
            }
        }

        for (const ActorActivationCandidate& candidate : removeCandidates)
        {
            if (actorStateChanges >= mMaxActorBodiesStateChangesPerActivation)
                break;

            application::game::Scene::BancEntityRenderInfo* entity = candidate.mEntity;
            if (!mRigidBodies.contains(entity))
                continue;

            PlaySessionRigidBodyEntity& body = mRigidBodies[entity];
            if (body.mInWorld)
            {
                SetRigidBodyActive(body, false);
                actorStateChanges++;
            }
        }

        FlushPendingActorCollisionRequests(PlayerPosition);
        ConsumeReadyActorCollisionData();

        const float terrainAddRadiusSq = mTerrainActivationRadius * mTerrainActivationRadius;
        const float terrainRemoveRadiusSq = (mTerrainActivationRadius + mActivationHysteresis) * (mTerrainActivationRadius + mActivationHysteresis);
        for (PendingTerrainBody& terrainBody : mPendingTerrainBodies)
        {
            const glm::vec3 delta = terrainBody.mCenter - PlayerPosition;
            const float distanceSq = glm::dot(delta, delta);

            if (distanceSq <= terrainAddRadiusSq)
            {
                EnsureTerrainRigidBody(terrainBody);
                if (terrainBody.mBody.has_value())
                    SetRigidBodyActive(*terrainBody.mBody, true);
            }
            else if (terrainBody.mBody.has_value() && terrainBody.mBody->mInWorld && distanceSq >= terrainRemoveRadiusSq)
            {
                SetRigidBodyActive(*terrainBody.mBody, false);
            }
        }
    }

	void PlaySession::Update(float dt)
	{
        glm::vec3 camForward = glm::normalize(glm::vec3(
            mCamera.mTarget.x - mCamera.mPosition.x,
            0.0f,
            mCamera.mTarget.z - mCamera.mPosition.z
        ));
        glm::vec3 camRight = glm::normalize(glm::cross(camForward, glm::vec3(0, 1, 0)));

        glm::vec3 moveDir(0.0f);

        if (glfwGetKey(mCamera.mWindow, GLFW_KEY_W) == GLFW_PRESS)
            moveDir += camForward;
        if (glfwGetKey(mCamera.mWindow, GLFW_KEY_S) == GLFW_PRESS)
            moveDir -= camForward;
        if (glfwGetKey(mCamera.mWindow, GLFW_KEY_D) == GLFW_PRESS)
            moveDir += camRight;
        if (glfwGetKey(mCamera.mWindow, GLFW_KEY_A) == GLFW_PRESS)
            moveDir -= camRight;

        bool isMoving = false;
        if (glm::length(moveDir) > 0.0f)
        {
            moveDir = glm::normalize(moveDir);
            isMoving = true;
        }

        float speed = 0.075;
        btVector3 walkDirection(
            moveDir.x * speed,
            0.0f,
            moveDir.z * speed
        );

        mPlayerPhysicsEntity.mCharacterController->setWalkDirection(walkDirection);

        if (isMoving)
        {
            float targetYaw = glm::degrees(atan2f(moveDir.x, moveDir.z));

            auto normalizeAngle = [](float angle) -> float {
                while (angle > 180.0f) angle -= 360.0f;
                while (angle < -180.0f) angle += 360.0f;
                return angle;
                };

            targetYaw = normalizeAngle(targetYaw);
            float currentYaw = normalizeAngle(mPlayerCurrentYaw);

            float angleDiff = normalizeAngle(targetYaw - currentYaw);

            float rotationSpeed = 720.0f;

            float maxRotationThisFrame = rotationSpeed * dt;

            if (fabs(angleDiff) < maxRotationThisFrame)
            {
                mPlayerCurrentYaw = targetYaw;
            }
            else
            {
                mPlayerCurrentYaw += (angleDiff > 0 ? 1.0f : -1.0f) * maxRotationThisFrame;
            }

            mPlayerEntityInfo->mRotate.y = mPlayerCurrentYaw;
        }

        if (glfwGetKey(mCamera.mWindow, GLFW_KEY_SPACE) == GLFW_PRESS)
        {
            if (mPlayerPhysicsEntity.mCharacterController->onGround())
            {
                mPlayerPhysicsEntity.mCharacterController->jump(btVector3(0, 10.0f, 0));
            }
        }

        mActivationUpdateTimer += dt;
        if (mActivationUpdateTimer >= mActivationUpdateInterval)
        {
            const btTransform playerTransform = mPlayerPhysicsEntity.mPlayerGhostObject->getWorldTransform();
            const btVector3 playerPos = playerTransform.getOrigin();
            UpdatePhysicsActivation(glm::vec3(playerPos.x(), playerPos.y(), playerPos.z()));
            mActivationUpdateTimer = 0.0f;
        }

        mDynamicsWorld->stepSimulation(dt, 10, 1.0f/240.0f);
        glm::mat4 camMatrix = mCamera.GetProjectionMatrix() * mCamera.GetViewMatrix();
        mDebugDrawer->mCameraMatrix = camMatrix; // assign current camera
        //mDynamicsWorld->debugDrawWorld();
        //mDebugDrawer->Render(mDebugShader);

        btTransform trans = mPlayerPhysicsEntity.mPlayerGhostObject->getWorldTransform();

        btVector3 pos = trans.getOrigin();

        mPlayerEntityInfo->mTranslate.x = pos.x();
        mPlayerEntityInfo->mTranslate.y = pos.y() - mPlayerCollisionHeightOffset;
        mPlayerEntityInfo->mTranslate.z = pos.z();
        mScene.SyncBancEntity(mPlayerEntityInfo);

		ImVec2 WindowPos = ImGui::GetWindowPos();
        mCamera.SetTarget(mPlayerEntityInfo->mTranslate + glm::vec3(0.0f, 2.0f, 0.0f));
		mCamera.Inputs(ImGui::GetIO().Framerate, glm::vec2(WindowPos.x, WindowPos.y));
		mCamera.UpdateMatrix(45.0f, 0.1f, 30000.0f);
	}

    bool IsBancEntityRenderInfoCulled(const application::game::Scene::BancEntityRenderInfo& RenderInfo)
    {
        if (RenderInfo.mEntity->mBfresRenderer->mBfresFile->mDefaultModel) return true;
        if (!RenderInfo.mEntity->mBfresRenderer->mBfresFile->mDefaultModel && RenderInfo.mEntity->mGyml.ends_with("_Far")) return true;

        return false;
    }

    void PlaySession::DrawInstancedBancEntityRenderInfo(std::vector<application::game::Scene::BancEntityRenderInfo>& Entities)
    {
        if (Entities.empty()) return;

        if (IsBancEntityRenderInfoCulled(Entities[0])) return;

        std::vector<glm::mat4> InstanceMatrices(Entities.size());

        int Shrinking = 0;
        for (size_t i = 0; i < Entities.size(); i++)
        {
            application::game::Scene::BancEntityRenderInfo& RenderInfo = Entities[i];

            RenderInfo.mSphereScreenSize = application::util::Math::CalculateScreenRadius(RenderInfo.mTranslate, RenderInfo.mEntity->mBfresRenderer->mSphereBoundingBoxRadius, mCamera.mPosition, 45.0f, mCamera.mHeight);
            RenderInfo.mSphereScreenSize *= std::max(RenderInfo.mScale.x, std::max(RenderInfo.mScale.y, RenderInfo.mScale.z));

            if ((!RenderInfo.mEntity->mBfresRenderer->mIsSystemModelTransparent && RenderInfo.mSphereScreenSize < 5.0f) || !mFrustum.SphereInFrustum(RenderInfo.mTranslate.x, RenderInfo.mTranslate.y, RenderInfo.mTranslate.z, RenderInfo.mEntity->mBfresRenderer->mBfresFile->Models.GetByIndex(0).mValue.BoundingBoxSphereRadius * std::fmax(std::fmax(RenderInfo.mScale.x, RenderInfo.mScale.y), RenderInfo.mScale.z)))
            {
                Shrinking++;
                continue;
            }

            if (RenderInfo.mEntity->mActorPack->mNeedsPhysicsHash &&
                RenderInfo.mEntity->mActorPack->mMass > 0.0f &&
                mRigidBodies.contains(&RenderInfo) &&
                mRigidBodies[&RenderInfo].mInWorld)
            {
                PlaySessionRigidBodyEntity& Entity = mRigidBodies[&RenderInfo];
                
                btTransform trans;
                Entity.mRigidBody->getMotionState()->getWorldTransform(trans);

                btVector3 pos = trans.getOrigin();

                btQuaternion rot = trans.getRotation();

                btScalar yaw, pitch, roll;
                rot.getEulerZYX(roll, pitch, yaw);

                RenderInfo.mRotate.x = glm::degrees(yaw);
                RenderInfo.mRotate.y = glm::degrees(pitch);
                RenderInfo.mRotate.z = glm::degrees(roll);

                RenderInfo.mTranslate.x = pos.x();
                RenderInfo.mTranslate.y = pos.y();
                RenderInfo.mTranslate.z = pos.z();
                mScene.SyncBancEntity(&RenderInfo);
            }

            InstanceMatrices[i - Shrinking] = glm::mat4(1.0f); // Identity matrix

            InstanceMatrices[i - Shrinking] = glm::translate(InstanceMatrices[i - Shrinking], RenderInfo.mTranslate);

            InstanceMatrices[i - Shrinking] = glm::rotate(InstanceMatrices[i - Shrinking], glm::radians(RenderInfo.mRotate.z), glm::vec3(0.0, 0.0f, 1.0));
            InstanceMatrices[i - Shrinking] = glm::rotate(InstanceMatrices[i - Shrinking], glm::radians(RenderInfo.mRotate.y), glm::vec3(0.0f, 1.0, 0.0));
            InstanceMatrices[i - Shrinking] = glm::rotate(InstanceMatrices[i - Shrinking], glm::radians(RenderInfo.mRotate.x), glm::vec3(1.0, 0.0f, 0.0));

            InstanceMatrices[i - Shrinking] = glm::scale(InstanceMatrices[i - Shrinking], RenderInfo.mScale);
        }

        if (Shrinking > 0)
            InstanceMatrices.resize(InstanceMatrices.size() - Shrinking);

        Entities[0].mEntity->mBfresRenderer->Draw(InstanceMatrices);
    }

	void PlaySession::Render()
	{
        mInstancedShader->Bind();
        mCamera.Matrix(mInstancedShader, "camMatrix");
        glUniform3fv(glGetUniformLocation(mInstancedShader->mID, "lightColor"), 1, &mLightColor[0]);
        glUniform3fv(glGetUniformLocation(mInstancedShader->mID, "lightPos"), 1, &mCamera.mPosition[0]);

        mFrustum.CalculateFrustum(mCamera);

        for (auto& [Model, Entities] : mScene.mDrawList)
        {
            DrawInstancedBancEntityRenderInfo(Entities);
        }

        if (mScene.mTerrainRenderer != nullptr)
        {
            mScene.mTerrainRenderer->Draw(mCamera);
        }
	}

	void PlaySession::Shutdown()
	{
        StopActorCollisionLoader();

        mDynamicsWorld->removeAction(mPlayerPhysicsEntity.mCharacterController);
        mDynamicsWorld->removeCollisionObject(mPlayerPhysicsEntity.mPlayerGhostObject);

        for (auto& [Entity, Body] : mRigidBodies)
        {
            if (Body.mInWorld)
                mDynamicsWorld->removeRigidBody(Body.mRigidBody);
        }

        for (auto& PendingBody : mPendingTerrainBodies)
        {
            if (PendingBody.mBody.has_value() && PendingBody.mBody->mInWorld)
                mDynamicsWorld->removeRigidBody(PendingBody.mBody->mRigidBody);
        }

        for (auto& [Entity, Body] : mRigidBodies)
        {
            delete Body.mMotionState;
            delete Body.mRigidBody;
        }

        for (auto& PendingBody : mPendingTerrainBodies)
        {
            if (!PendingBody.mBody.has_value())
                continue;

            delete PendingBody.mBody->mMotionState;
            delete PendingBody.mBody->mRigidBody;
        }

        for (auto& Shape : mShapesToDelete)
        {
            delete Shape;
        }

        for (auto& [Name, Mesh] : mCollisionMeshes)
        {
            delete Mesh;
        }

        mRigidBodies.clear();
        mPhysicsCandidates.clear();
        mActorActivationRadius.clear();
        mPendingTerrainBodies.clear();
        mCollisionShapes.clear();
        mCollisionMeshes.clear();
        mShapesToDelete.clear();
        mActorCollisionQueued.clear();
        mActorCollisionPendingRequests.clear();
        while (!mActorCollisionQueue.empty())
            mActorCollisionQueue.pop();

        for (auto& [Entity, CollisionData] : mActorCollisionLoaded)
        {
            if (CollisionData.mBody.mMotionState != nullptr)
                delete CollisionData.mBody.mMotionState;
            if (CollisionData.mBody.mRigidBody != nullptr)
                delete CollisionData.mBody.mRigidBody;

            for (btCollisionShape* shape : CollisionData.mOwnedShapes)
                delete shape;
            for (btTriangleMesh* mesh : CollisionData.mOwnedMeshes)
                delete mesh;
        }
        for (auto& [Entity, CollisionData] : mActorCollisionReady)
        {
            if (CollisionData.mBody.mMotionState != nullptr)
                delete CollisionData.mBody.mMotionState;
            if (CollisionData.mBody.mRigidBody != nullptr)
                delete CollisionData.mBody.mRigidBody;

            for (btCollisionShape* shape : CollisionData.mOwnedShapes)
                delete shape;
            for (btTriangleMesh* mesh : CollisionData.mOwnedMeshes)
                delete mesh;
        }
        mActorCollisionLoaded.clear();
        mActorCollisionReady.clear();

        delete mPlayerPhysicsEntity.mCharacterController;
        delete mPlayerPhysicsEntity.mPlayerGhostObject;
        delete mPlayerPhysicsEntity.mPlayerCapsuleShape;
        delete mDebugDrawer;
        delete mDynamicsWorld;
        delete mSolver;
        delete mBroadphase;
        delete mDispatcher;
        delete mCollisionConfig;

        mScene.DeleteBancEntity(mPlayerEntityInfo);
        mScene.GenerateDrawList();
	}
}
