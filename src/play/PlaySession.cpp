#include <play/PlaySession.h>

#include "imgui.h"
#include <util/Math.h>
#include <util/Logger.h>
#include <util/FileUtil.h>
#include <manager/UIMgr.h>
#include <manager/ShaderMgr.h>

#include <file/game/zstd/ZStdBackend.h>
#include <file/game/phive/shape/PhiveShape.h>
#include <game/actor_component/ActorComponentShapeParam.h>
#include <tool/scene/static_compound/StaticCompoundImplementationFieldScene.h>

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

        mDynamicsWorld->setDebugDrawer(mDebugDrawer);

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
            application::tool::scene::static_compound::StaticCompoundImplementationFieldScene* StaticCompoundImpl = static_cast<application::tool::scene::static_compound::StaticCompoundImplementationFieldScene*>(mScene.mStaticCompoundImplementation.get());
            
            uint32_t Index = 0;

            for (application::file::game::phive::PhiveStaticCompoundFile& File : StaticCompoundImpl->mStaticCompoundFiles)
            {
                for (auto& ExternalBphshMesh : File.mExternalBphshMeshes)
                {
                    PlaySessionRigidBodyEntity PhysicsEntity;

                    std::vector<float> Vertices = ExternalBphshMesh.mPhiveShape.ToVertices();
                    std::vector<uint32_t> Indices = ExternalBphshMesh.mPhiveShape.ToIndices();

                    btTriangleMesh* mesh = new btTriangleMesh();
                    for (size_t i = 0; i < Indices.size(); i += 3)
                    {
                        mesh->addTriangle(
                            btVector3(Vertices[Indices[i + 0] * 3], Vertices[Indices[i + 0] * 3 + 1], Vertices[Indices[i + 0] * 3 + 2]),
                            btVector3(Vertices[Indices[i + 1] * 3], Vertices[Indices[i + 1] * 3 + 1], Vertices[Indices[i + 1] * 3 + 2]),
                            btVector3(Vertices[Indices[i + 2] * 3], Vertices[Indices[i + 2] * 3 + 1], Vertices[Indices[i + 2] * 3 + 2]),
                            true
                        );
                    }

                    const std::string id = std::to_string(reinterpret_cast<uint64_t>(&ExternalBphshMesh));

                    mCollisionMeshes[id] = mesh;

                    mShapesToDelete.push_back(new btBvhTriangleMeshShape(mCollisionMeshes[id], true));

                    PhysicsEntity.mCollisionShape = mShapesToDelete.back();

                    glm::vec3 Offset = StaticCompoundImpl->GetStaticCompoundMiddlePoint(Index % 4, Index / 4);

                    btTransform MotionState;
                    MotionState.setIdentity();
                    MotionState.setOrigin(btVector3(Offset.x, 0.5f, Offset.z));
                    PhysicsEntity.mMotionState = new btDefaultMotionState(MotionState);

                    btVector3 Inertia(0, 0, 0);

                    btRigidBody::btRigidBodyConstructionInfo Body(0.0f, PhysicsEntity.mMotionState, PhysicsEntity.mCollisionShape, Inertia);
                    PhysicsEntity.mRigidBody = new btRigidBody(Body);

                    mTerrainRigidBodies.push_back(PhysicsEntity);
                    Index++;
                }
            }
        }

        for (application::game::Scene::BancEntityRenderInfo* Entity : mScene.mDrawListRenderInfoIndices)
        {
            application::game::actor_component::ActorComponentShapeParam* ShapeParam = static_cast<application::game::actor_component::ActorComponentShapeParam*>(Entity->mEntity->mActorPack->GetComponent(application::game::actor_component::ActorComponentBase::ComponentType::SHAPE_PARAM));

            if (!ShapeParam)
                continue;

            if (ShapeParam->mPhiveShapeParam.IsEmpty())
                continue;

            if (Entity == mPlayerEntityInfo)
                continue;

            PlaySessionRigidBodyEntity PhysicsEntity;

            std::string Hash = ShapeParam->mPhiveShapeParam.GetHash();

            if (!mCollisionShapes.contains(
                Hash +
                "_" + std::to_string(Entity->mScale.x) + 
                "_" + std::to_string(Entity->mScale.y) + 
                "_" + std::to_string(Entity->mScale.z)
            ))
            {
                std::vector<btCollisionShape*> CollisionShapes;
                for (const std::string& PathName : ShapeParam->mPhiveShapeParam.mPhiveShapes)
                {
                    application::file::game::phive::shape::PhiveShape Shape(application::file::game::ZStdBackend::Decompress(application::util::FileUtil::GetRomFSFilePath("Phive/Shape/Dcc/" + PathName + ".Nin_NX_NVN.bphsh.zs")));
                    std::vector<float> Vertices = Shape.ToVertices();
                    std::vector<uint32_t> Indices = Shape.ToIndices();

                    if (!mCollisionMeshes.contains(PathName))
                    {
                        btTriangleMesh* mesh = new btTriangleMesh();
                        for (size_t i = 0; i < Indices.size(); i += 3)
                        {
                            mesh->addTriangle(
                                btVector3(Vertices[Indices[i + 0] * 3], Vertices[Indices[i + 0] * 3 + 1], Vertices[Indices[i + 0] * 3 + 2]),
                                btVector3(Vertices[Indices[i + 1] * 3], Vertices[Indices[i + 1] * 3 + 1], Vertices[Indices[i + 1] * 3 + 2]),
                                btVector3(Vertices[Indices[i + 2] * 3], Vertices[Indices[i + 2] * 3 + 1], Vertices[Indices[i + 2] * 3 + 2]),
                                true
                            );
                        }

                        mCollisionMeshes[PathName] = mesh;
                    }

                    CollisionShapes.push_back(new btBvhTriangleMeshShape(mCollisionMeshes[PathName], true));
                }

                for (application::game::actor_component::ActorComponentShapeParam::PhiveShapeParam::Box& Box : ShapeParam->mPhiveShapeParam.mBoxes)
                {
                    btVector3 halfExtents(Box.mHalfExtents.x, Box.mHalfExtents.y, Box.mHalfExtents.z);
                    btBoxShape* boxShape = new btBoxShape(halfExtents);
                    btCompoundShape* compoundShape = new btCompoundShape();

                    btTransform localTransform;
                    localTransform.setIdentity();
                    localTransform.setOrigin(btVector3(Box.mOffsetTranslation.x + Box.mCenter.x, Box.mOffsetTranslation.y + Box.mCenter.y, Box.mOffsetTranslation.z + Box.mCenter.z));

                    compoundShape->addChildShape(localTransform, boxShape);

                    CollisionShapes.push_back(compoundShape);

                    mShapesToDelete.push_back(boxShape);
                }

                for (application::game::actor_component::ActorComponentShapeParam::PhiveShapeParam::Sphere& Sphere : ShapeParam->mPhiveShapeParam.mSpheres)
                {
                    btSphereShape* sphereShape = new btSphereShape(Sphere.mRadius);
                    btCompoundShape* compoundShape = new btCompoundShape();

                    btTransform localTransform;
                    localTransform.setIdentity();
                    localTransform.setOrigin(btVector3(Sphere.mCenter.x, Sphere.mCenter.y, Sphere.mCenter.z));

                    compoundShape->addChildShape(localTransform, sphereShape);

                    CollisionShapes.push_back(compoundShape);

                    mShapesToDelete.push_back(sphereShape);
                }

                for (application::game::actor_component::ActorComponentShapeParam::PhiveShapeParam::Polytope& Polytope : ShapeParam->mPhiveShapeParam.mPolytopes)
                {
                    btConvexHullShape* ConvexHull = new btConvexHullShape();

                    for (size_t i = 0; i < Polytope.mVertices.size(); i++)
                    {
                        ConvexHull->addPoint(btVector3(
                            Polytope.mVertices[i].x,
                            Polytope.mVertices[i].y,
                            Polytope.mVertices[i].z
                        ), false);
                    }

                    ConvexHull->recalcLocalAabb();
                    ConvexHull->optimizeConvexHull();

                    btCompoundShape* compoundShape = new btCompoundShape();

                    btTransform localTransform;
                    localTransform.setIdentity();
                    localTransform.setOrigin(btVector3(Polytope.mOffsetTranslation.x, Polytope.mOffsetTranslation.y, Polytope.mOffsetTranslation.z));

                    compoundShape->addChildShape(localTransform, ConvexHull);

                    CollisionShapes.push_back(compoundShape);

                    mShapesToDelete.push_back(ConvexHull);
                }

                if (CollisionShapes.size() == 1)
                {
                    CollisionShapes[0]->setLocalScaling(btVector3(
                        Entity->mScale.x,
                        Entity->mScale.y,
                        Entity->mScale.z
                    ));

                    mCollisionShapes[
                        Hash +
                            "_" + std::to_string(Entity->mScale.x) +
                            "_" + std::to_string(Entity->mScale.y) +
                            "_" + std::to_string(Entity->mScale.z)
                    ] = CollisionShapes[0];
                }
                else
                {
                    btCompoundShape* CompoundShape = new btCompoundShape();

                    for (btCollisionShape* Shape : CollisionShapes)
                    {
                        btTransform Origin;
                        Origin.setIdentity();
                        Origin.setOrigin(btVector3(0, 0, 0));
                        CompoundShape->addChildShape(Origin, Shape);
                    }

                    CompoundShape->setLocalScaling(btVector3(
                        Entity->mScale.x,
                        Entity->mScale.y,
                        Entity->mScale.z
                    ));

                    mCollisionShapes[
                        Hash +
                            "_" + std::to_string(Entity->mScale.x) +
                            "_" + std::to_string(Entity->mScale.y) +
                            "_" + std::to_string(Entity->mScale.z)
                    ] = CompoundShape;

                    mShapesToDelete.insert(mShapesToDelete.end(), CollisionShapes.begin(), CollisionShapes.end());
                }
            }

            PhysicsEntity.mCollisionShape = mCollisionShapes[Hash +
                "_" + std::to_string(Entity->mScale.x) +
                "_" + std::to_string(Entity->mScale.y) +
                "_" + std::to_string(Entity->mScale.z)];

            btQuaternion Quat;
            Quat.setEulerZYX(glm::radians(Entity->mRotate.z), glm::radians(Entity->mRotate.y), glm::radians(Entity->mRotate.x));

            btTransform MotionState;
            MotionState.setIdentity();
            MotionState.setOrigin(btVector3(Entity->mTranslate.x, Entity->mTranslate.y, Entity->mTranslate.z));
            MotionState.setRotation(Quat);
            PhysicsEntity.mMotionState = new btDefaultMotionState(MotionState);

            btVector3 Inertia(0, 0, 0);
            if (Entity->mEntity->mActorPack->mMass > 0.0f && Entity->mEntity->mActorPack->mNeedsPhysicsHash)
            {
                PhysicsEntity.mCollisionShape->calculateLocalInertia(Entity->mEntity->mActorPack->mMass, Inertia);
            }

            btRigidBody::btRigidBodyConstructionInfo Body(Entity->mEntity->mActorPack->mNeedsPhysicsHash ? Entity->mEntity->mActorPack->mMass : 0.0f, PhysicsEntity.mMotionState, PhysicsEntity.mCollisionShape, Inertia);
            PhysicsEntity.mRigidBody = new btRigidBody(Body);

            mRigidBodies[Entity] = PhysicsEntity;
        }

        for (auto& [Entity, Body] : mRigidBodies)
        {
            mDynamicsWorld->addRigidBody(Body.mRigidBody);
        }

        for (auto& Body : mTerrainRigidBodies)
        {
            mDynamicsWorld->addRigidBody(Body.mRigidBody);
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

        mDynamicsWorld->stepSimulation(dt, 10, 1.0f/240.0f);
        glm::mat4 camMatrix = mCamera.GetProjectionMatrix() * mCamera.GetViewMatrix();
        mDebugDrawer->mCameraMatrix = camMatrix; // assign current camera
        mDynamicsWorld->debugDrawWorld();
        mDebugDrawer->Render(mDebugShader);

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

            if (RenderInfo.mEntity->mActorPack->mNeedsPhysicsHash && RenderInfo.mEntity->mActorPack->mMass > 0.0f && mRigidBodies.contains(&RenderInfo))
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
        mDynamicsWorld->removeAction(mPlayerPhysicsEntity.mCharacterController);

        for (auto& [Entity, Body] : mRigidBodies)
        {
            mDynamicsWorld->removeRigidBody(Body.mRigidBody);
        }

        for (auto& Body : mTerrainRigidBodies)
        {
            mDynamicsWorld->removeRigidBody(Body.mRigidBody);
        }

        for (auto& [Entity, Body] : mRigidBodies)
        {
            delete Body.mMotionState;
            delete Body.mRigidBody;
        }

        for (auto& Body : mTerrainRigidBodies)
        {
            delete Body.mMotionState;
            delete Body.mRigidBody;
        }

        for (auto& [Name, Shape] : mCollisionShapes)
        {
            delete Shape;
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
        mCollisionShapes.clear();
        mCollisionMeshes.clear();
        mShapesToDelete.clear();

        delete mPlayerPhysicsEntity.mCharacterController;
        delete mDynamicsWorld;
        delete mSolver;
        delete mBroadphase;
        delete mDispatcher;
        delete mCollisionConfig;

        mScene.DeleteBancEntity(mPlayerEntityInfo);
        mScene.GenerateDrawList();
	}
}