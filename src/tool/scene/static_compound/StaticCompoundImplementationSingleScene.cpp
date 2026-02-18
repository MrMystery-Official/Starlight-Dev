#include "StaticCompoundImplementationSingleScene.h"

#include <file/game/zstd/ZStdBackend.h>
#include <util/FileUtil.h>
#include <util/Logger.h>
#include <util/Math.h>
#include <manager/GameDataListMgr.h>

namespace application::tool::scene::static_compound
{
	StaticCompoundImplementationSingleScene::StaticCompoundImplementationSingleScene(application::game::Scene* Scene, const std::string& Path) : application::tool::scene::StaticCompoundImplementationBase(), mScene(Scene), mPath(Path)
	{

	}

	bool StaticCompoundImplementationSingleScene::Initialize()
	{
		mStaticCompoundFile = application::file::game::phive::PhiveStaticCompoundFile(application::file::game::ZStdBackend::Decompress(application::util::FileUtil::GetRomFSFilePath(mPath)));
		
		LoadHashes();

		for (application::game::Scene::BancEntityRenderInfo* RenderInfo : mScene->mDrawListRenderInfoIndices)
		{

			for (auto Iter = mStaticCompoundFile.mWaterBoxArray.begin(); Iter != mStaticCompoundFile.mWaterBoxArray.end(); )
			{
				if (application::util::Math::CompareFloatsWithTolerance(Iter->mPositionX, RenderInfo->mTranslate.x, 0.1f) && 
					application::util::Math::CompareFloatsWithTolerance(Iter->mPositionY, RenderInfo->mTranslate.y, 0.1f) &&
					application::util::Math::CompareFloatsWithTolerance(Iter->mPositionZ, RenderInfo->mTranslate.z, 0.1f) &&
					application::util::Math::CompareFloatsWithTolerance(glm::degrees(Iter->mRotationX), RenderInfo->mRotate.x, 0.1f) && 
					application::util::Math::CompareFloatsWithTolerance(glm::degrees(Iter->mRotationY), RenderInfo->mRotate.y, 0.1f) && 
					application::util::Math::CompareFloatsWithTolerance(glm::degrees(Iter->mRotationZ), RenderInfo->mRotate.z, 0.1f)
					)
				{
					RenderInfo->mEntity->mBakedFluid.mBakeFluid = true;

					RenderInfo->mEntity->mBakedFluid.mFluidShapeType = application::game::BancEntity::FluidShapeType::BOX;
					RenderInfo->mEntity->mBakedFluid.mFluidFlowSpeed = Iter->mFlowSpeed;
					RenderInfo->mEntity->mBakedFluid.mFluidMaterialType = application::game::BancEntity::BakedFluid::ToMaterialType(Iter->mFluidType);
					RenderInfo->mEntity->mBakedFluid.mFluidBoxHeight = Iter->mScaleTimesTenY;
					Iter = mStaticCompoundFile.mWaterBoxArray.erase(Iter);
					goto LinkedFluidSource;
				}
				Iter++;
			}

			for (auto Iter = mStaticCompoundFile.mWaterCylinderArray.begin(); Iter != mStaticCompoundFile.mWaterCylinderArray.end(); )
			{
				if (Iter->mPositionX == RenderInfo->mTranslate.x && Iter->mPositionY == RenderInfo->mTranslate.y && Iter->mPositionZ == RenderInfo->mTranslate.z
					&& application::util::Math::CompareFloatsWithTolerance(glm::degrees(Iter->mRotationX), RenderInfo->mRotate.x, 0.1f) && application::util::Math::CompareFloatsWithTolerance(glm::degrees(Iter->mRotationY), RenderInfo->mRotate.y, 0.1f) && application::util::Math::CompareFloatsWithTolerance(glm::degrees(Iter->mRotationZ), RenderInfo->mRotate.z, 0.1f))
				{
					RenderInfo->mEntity->mBakedFluid.mBakeFluid = true;

					RenderInfo->mEntity->mBakedFluid.mFluidShapeType = application::game::BancEntity::FluidShapeType::CYLINDER;
					RenderInfo->mEntity->mBakedFluid.mFluidFlowSpeed = Iter->mFlowSpeed;
					RenderInfo->mEntity->mBakedFluid.mFluidMaterialType = application::game::BancEntity::BakedFluid::ToMaterialType(Iter->mFluidType);
					RenderInfo->mEntity->mBakedFluid.mFluidBoxHeight = Iter->mScaleTimesTenY;
					Iter = mStaticCompoundFile.mWaterCylinderArray.erase(Iter);
					goto LinkedFluidSource;
				}
				Iter++;
			}

		LinkedFluidSource:;
		}

		return true;
	}

	uint64_t StaticCompoundImplementationSingleScene::RequestUniqueHash()
	{
		do mSmallestHash++; while (mSceneHashes.contains(mSmallestHash));

		return mSmallestHash;
	}

	uint32_t StaticCompoundImplementationSingleScene::RequestUniqueSRTHash()
	{
		do mSmallestSRTHash++; while (mSceneSRTHashes.contains(mSmallestSRTHash));

		return mSmallestSRTHash;
	}

	void StaticCompoundImplementationSingleScene::SyncAiGroupHashes(const uint64_t& OldHash, const uint64_t& NewHash)
	{
		for (application::game::Scene::AiGroup& Group : mScene->mAiGroups)
		{
			for (application::game::Scene::AiGroup::Reference& Ref : Group.mReferences)
			{
				if (Ref.mReference == OldHash) Ref.mReference = NewHash;
			}
		}
	}

	std::optional<glm::vec2> GetActorModelScale(application::game::Scene::BancEntityRenderInfo* RenderInfo)
	{
		if (RenderInfo->mEntity == nullptr)
			return std::nullopt;

		if (RenderInfo->mEntity->mBfresRenderer == nullptr)
			return std::nullopt;

		if (RenderInfo->mEntity->mBfresRenderer->mBfresFile == nullptr)
			return std::nullopt;

		if (RenderInfo->mEntity->mBfresRenderer->mBfresFile->Models.mNodes.empty())
			return std::nullopt;

		if (RenderInfo->mEntity->mBfresRenderer->mBfresFile->Models.GetByIndex(0).mValue.Shapes.mNodes.empty())
			return std::nullopt;

		std::vector<glm::vec3> ModelVertices;

		glm::mat4 GLMModel = glm::mat4(1.0f);  // Identity matrix
		GLMModel = glm::scale(GLMModel, glm::vec3(RenderInfo->mScale.x, RenderInfo->mScale.y, RenderInfo->mScale.z));
		for (uint32_t SubModelIndex = 0; SubModelIndex < RenderInfo->mEntity->mBfresRenderer->mBfresFile->Models.GetByIndex(0).mValue.Shapes.mNodes.size(); SubModelIndex++)
		{
			std::vector<glm::vec4> Positions = RenderInfo->mEntity->mBfresRenderer->mBfresFile->Models.GetByIndex(0).mValue.VertexBuffers[RenderInfo->mEntity->mBfresRenderer->mBfresFile->Models.GetByIndex(0).mValue.Shapes.mNodes[RenderInfo->mEntity->mBfresRenderer->mBfresFile->Models.GetByIndex(0).mValue.Shapes.GetKey(SubModelIndex)].mValue.VertexBufferIndex].GetPositions();
			for (glm::vec4& Vertex : Positions)
			{
				Vertex = GLMModel * Vertex;
				ModelVertices.push_back(glm::vec3(Vertex.x, Vertex.y, Vertex.z));
			}
		}

		glm::vec2 Max(std::numeric_limits<float>::lowest());

		for (glm::vec3& Vertex : ModelVertices) {
			if (Vertex.x > Max.x) Max.x = Vertex.x * 2;
			if (Vertex.z > Max.y) Max.y = Vertex.z * 2;
		}

		return Max;
	}

	bool StaticCompoundImplementationSingleScene::Save()
	{
		if (mGeneratePhysicsHashes)
		{
			for (application::game::BancEntity& Entity : mScene->mBancEntities)
			{
				if (Entity.mActorPack->mNeedsPhysicsHash && Entity.mCheckForHashGeneration)
				{
					bool Found = false;
					for (application::file::game::phive::PhiveStaticCompoundFile::PhiveStaticCompoundActorLink& Link : mStaticCompoundFile.mActorLinks)
					{
						if (Link.mActorHash == Entity.mHash)
						{
							Found = true;
							break;
						}
					}
					if (Found)
						continue;

					uint32_t OptimalIndex = 0xFFFFFFFF;
					for (size_t i = 0; i < mStaticCompoundFile.mActorHashMap.size(); i++)
					{
						if (mStaticCompoundFile.mActorHashMap[i] == 0)
						{
							OptimalIndex = i;
							break;
						}
					}
					if (OptimalIndex == 0xFFFFFFFF)
					{
						OptimalIndex = mStaticCompoundFile.mActorLinks.size() * 2;
					}

					std::cout << "Ideal hash: " << OptimalIndex << std::endl;

					uint64_t OldHash = Entity.mHash;

					while (true)
					{
						uint64_t Modulus = (mStaticCompoundFile.mActorLinks.size() + 1) * 2;
						uint64_t Remainder = Entity.mHash % Modulus;
						uint64_t Difference = (OptimalIndex >= Remainder) ? (OptimalIndex - Remainder) : (Modulus - Remainder + OptimalIndex);
						Entity.mHash += Difference;

						bool Duplicate = false;
						for (application::game::BancEntity& SceneEntity : mScene->mBancEntities)
						{
							if (&SceneEntity == &Entity)
								continue;

							if (SceneEntity.mHash == Entity.mHash)
							{
								Duplicate = true;
								break;
							}

							if (SceneEntity.mMergedActorChildren == nullptr)
								continue;

							for (application::game::BancEntity& ChildEntity : *SceneEntity.mMergedActorChildren)
							{
								if (ChildEntity.mHash == Entity.mHash)
								{
									Duplicate = true;
									break;
								}
							}
						}

						if (!Duplicate)
						{
							for (auto& [Hash, Node] : application::manager::GameDataListMgr::gBoolU64HashNodes)
							{
								if (Hash == Entity.mHash)
								{
									Duplicate = true;
									break;
								}
							}
						}

						if (Duplicate)
						{
							Entity.mHash++;
							continue;
						}

						break;
					}

					if (mStaticCompoundFile.mActorHashMap.size() > OptimalIndex)
					{
						mStaticCompoundFile.mActorHashMap[OptimalIndex] = Entity.mHash;
					}
					else
					{
						mStaticCompoundFile.mActorHashMap.resize(OptimalIndex + 2);
						mStaticCompoundFile.mActorHashMap[OptimalIndex] = Entity.mHash;
					}

					application::file::game::phive::PhiveStaticCompoundFile::PhiveStaticCompoundActorLink Link;
					Link.mActorHash = Entity.mHash;
					Link.mBaseIndex = 0xFFFFFFFF;
					Link.mEndIndex = 0xFFFFFFFF;
					Link.mCompoundRigidBodyIndex = 0;
					Link.mIsValid = 0;
					Link.mReserve0 = Entity.mSRTHash;
					Link.mReserve1 = 0;
					Link.mReserve2 = 0;
					Link.mReserve3 = 0;

					mStaticCompoundFile.mActorLinks.push_back(Link);

					Entity.mPhive.mPlacement["ID"] = Entity.mHash;

					SyncAiGroupHashes(OldHash, Entity.mHash);

					for (application::game::BancEntity& SceneEntity : mScene->mBancEntities)
					{
						SyncBancEntityHashes(SceneEntity, OldHash, Entity.mHash);
					}

					Entity.mCheckForHashGeneration = false;
				}
			}
		}

		for (application::game::Scene::BancEntityRenderInfo* RenderInfo : mScene->mDrawListRenderInfoIndices)
		{
			if (!RenderInfo->mEntity->mBakedFluid.mBakeFluid)
				continue;

			switch (RenderInfo->mEntity->mBakedFluid.mFluidShapeType)
			{
			case application::game::BancEntity::FluidShapeType::BOX:
			{
				std::optional<glm::vec2> Scale = GetActorModelScale(RenderInfo);
				if (!Scale.has_value())
				{
					application::util::Logger::Error("StaticCompoundImplementationSingleScene", "Could not bake water box, scale was empty");
					break;
				}

				application::file::game::phive::PhiveStaticCompoundFile::PhiveStaticCompoundWaterBox WaterBox;
				WaterBox.mCompoundId = 0;
				WaterBox.mReserve0 = 0;
				WaterBox.mFlowSpeed = RenderInfo->mEntity->mBakedFluid.mFluidFlowSpeed;
				WaterBox.mReserve2 = 1.0f;
				WaterBox.mScaleTimesTenX = Scale.value().x;
				WaterBox.mScaleTimesTenY = RenderInfo->mEntity->mBakedFluid.mFluidBoxHeight;
				WaterBox.mScaleTimesTenZ = Scale.value().y;
				WaterBox.mRotationX = glm::radians(RenderInfo->mRotate.x);
				WaterBox.mRotationY = glm::radians(RenderInfo->mRotate.y);
				WaterBox.mRotationZ = glm::radians(RenderInfo->mRotate.z);
				WaterBox.mPositionX = RenderInfo->mTranslate.x;
				WaterBox.mPositionY = RenderInfo->mTranslate.y;
				WaterBox.mPositionZ = RenderInfo->mTranslate.z;
				WaterBox.mFluidType = application::game::BancEntity::BakedFluid::ToInternalIdentifier(RenderInfo->mEntity->mBakedFluid.mFluidMaterialType);
				WaterBox.mReserve7 = 0;
				WaterBox.mReserve8 = 0;
				WaterBox.mReserve9 = 0;
				mStaticCompoundFile.mWaterBoxArray.push_back(WaterBox);
				break;
			}
			}
		}

		std::filesystem::create_directories(std::filesystem::path(application::util::FileUtil::GetSaveFilePath(mPath)).parent_path());
		application::util::FileUtil::WriteFile(application::util::FileUtil::GetSaveFilePath(mPath), application::file::game::ZStdBackend::Compress(mStaticCompoundFile.ToBinary(), application::file::game::ZStdBackend::Dictionary::Base));
		LoadHashes();

		return true;
	}

	void StaticCompoundImplementationSingleScene::LoadHashes()
	{
		mSmallestHash = std::numeric_limits<uint64_t>::max();
		mSmallestSRTHash = std::numeric_limits<uint32_t>::max();

		//Calculating smallest hash
		for (application::game::BancEntity& Entity : mScene->mBancEntities)
		{
			mSceneHashes.insert(Entity.mHash);
			mSceneSRTHashes.insert(Entity.mSRTHash);

			if (Entity.mMergedActorChildren == nullptr)
				continue;

			for (application::game::BancEntity& ChildEntity : *Entity.mMergedActorChildren)
			{
				mSceneHashes.insert(ChildEntity.mHash);
				mSceneSRTHashes.insert(ChildEntity.mSRTHash);
			}
		}

		if (mScene->mBancEntities.empty())
		{
			for (auto& ActorLink : mStaticCompoundFile.mActorLinks)
			{
				mSceneHashes.insert(ActorLink.mActorHash);
				mSceneSRTHashes.insert(ActorLink.mReserve0);
			}
		}

		for (const uint64_t& Hash : mSceneHashes)
			mSmallestHash = std::min(mSmallestHash, Hash);

		for (const uint32_t& SRTHash : mSceneSRTHashes)
			mSmallestSRTHash = std::min(mSmallestSRTHash, SRTHash);
	}

	void StaticCompoundImplementationSingleScene::RemoveBakedCollision()
	{
		mStaticCompoundFile.mCompoundTagFile.clear();
		mStaticCompoundFile.mImage.mRigidBodyEntryEntityCount = 0;
		mStaticCompoundFile.mImage.mRigidBodyEntrySensorCount = 0;

		application::util::Logger::Info("StaticCompoundImplementationSingleScene", "Successfully removed baked collision tag files and updated local image info");
	}

	void StaticCompoundImplementationSingleScene::OnBancEntityDelete(void* RenderInfo)
	{
		application::game::Scene::BancEntityRenderInfo* EntityRenderInfo = reinterpret_cast<application::game::Scene::BancEntityRenderInfo*>(RenderInfo);

		for(application::file::game::phive::PhiveStaticCompoundFile::PhiveStaticCompoundActorLink& Link : mStaticCompoundFile.mActorLinks)
		{
			if(Link.mActorHash == EntityRenderInfo->mEntity->mHash)
			{
				if(Link.mBaseIndex == 0xFFFFFFFF || Link.mEndIndex == 0xFFFFFFFF)
				{
					break;
				}

				for(int baseIndex = Link.mBaseIndex; baseIndex <= Link.mEndIndex; baseIndex++)
				{
					/* I believe stack index is not really the compound file index
					uint8_t StackIndex = mStaticCompoundFiles[i].mFixedOffset0Array[baseIndex].mStackIndex;
					*/

					const uint8_t StackIndex = 1;

					if(StackIndex >= mStaticCompoundFile.mCompoundTagFile.size())
					{
						application::util::Logger::Warning("StaticCompoundImplementationSingleScene", "Failed to remove hknpShapeInstance, invalid stack index %u (max %zu)", StackIndex, mStaticCompoundFile.mCompoundTagFile.size());
						continue;
					}

					uint8_t InstanceIndex = mStaticCompoundFile.mFixedOffset0Array[baseIndex].mInstanceIndex;

					if(InstanceIndex >= mStaticCompoundFile.mCompoundTagFile[StackIndex].mTagFile.mRootClass.mInstances.mParent.mElements.mParent.mElements.size())
					{
						application::util::Logger::Warning("StaticCompoundImplementationSingleScene", "Failed to remove hknpShapeInstance, invalid instance index %u (max %zu)", InstanceIndex, mStaticCompoundFile.mCompoundTagFile[StackIndex].mTagFile.mRootClass.mInstances.mParent.mElements.mParent.mElements.size());
						continue;
					}

					auto& MeshInstance = mStaticCompoundFile.mCompoundTagFile[StackIndex].mTagFile.mRootClass.mInstances.mParent.mElements.mParent.mElements[InstanceIndex];

					MeshInstance.mParent.mScale.mX = 0.0f;
					MeshInstance.mParent.mScale.mY = 0.0f;
					MeshInstance.mParent.mScale.mZ = 0.0f;

					MeshInstance.mParent.mTranslation.mX = 0.0f;
					MeshInstance.mParent.mTranslation.mY = 0.0f;
					MeshInstance.mParent.mTranslation.mZ = 0.0f;

					MeshInstance.mParent.mRotation.mParent.mVec.mData[0] = 0.0f;
					MeshInstance.mParent.mRotation.mParent.mVec.mData[1] = 0.0f;
					MeshInstance.mParent.mRotation.mParent.mVec.mData[2] = 0.0f;
					MeshInstance.mParent.mRotation.mParent.mVec.mData[3] = 0.0f;

					MeshInstance.mParent.mFlags.mParent = ((uint8_t)1 << 3); //HAS_SCALE

					application::util::Logger::Info("StaticCompoundImplementationSingleScene", "Removed hknpShapeInstance");
				}


				break;
			}
		}
	}

	void StaticCompoundImplementationSingleScene::CleanupCollision(void* Scene)
	{
		application::game::Scene* ScenePtr = reinterpret_cast<application::game::Scene*>(Scene);

		uint32_t TotalCount = 0;
		for (size_t i = 0; i < mStaticCompoundFile.mCompoundTagFile.size(); i++)
		{
			TotalCount += mStaticCompoundFile.mCompoundTagFile[i].mTagFile.mRootClass.mInstances.mParent.mElements.mParent.mElements.size();
		}

		uint32_t RemovedCount = 0;
		
		for (application::file::game::phive::PhiveStaticCompoundFile::PhiveStaticCompoundActorLink& Link : mStaticCompoundFile.mActorLinks)
		{
			if (Link.mBaseIndex == 0xFFFFFFFF || Link.mEndIndex == 0xFFFFFFFF)
			{
				continue;
			}

			bool Found = false;

			for (application::game::Scene::BancEntityRenderInfo* RenderInfo : ScenePtr->mDrawListRenderInfoIndices)
			{
				if (RenderInfo->mEntity->mHash == Link.mActorHash)
				{
					Found = true;
					break;
				}
			}

			if (Found)
				continue;


			for (int baseIndex = Link.mBaseIndex; baseIndex <= Link.mEndIndex; baseIndex++)
			{
				const uint8_t StackIndex = 1;

				if (StackIndex >= mStaticCompoundFile.mCompoundTagFile.size())
				{
					application::util::Logger::Warning("StaticCompoundImplementationSingleScene", "Failed to remove hknpShapeInstance, invalid stack index %u (max %zu)", StackIndex, mStaticCompoundFile.mCompoundTagFile.size());
					continue;
				}

				uint8_t InstanceIndex = mStaticCompoundFile.mFixedOffset0Array[baseIndex].mInstanceIndex;

				if (InstanceIndex >= mStaticCompoundFile.mCompoundTagFile[StackIndex].mTagFile.mRootClass.mInstances.mParent.mElements.mParent.mElements.size())
				{
					application::util::Logger::Warning("StaticCompoundImplementationSingleScene", "Failed to remove hknpShapeInstance, invalid instance index %u (max %zu)", InstanceIndex, mStaticCompoundFile.mCompoundTagFile[StackIndex].mTagFile.mRootClass.mInstances.mParent.mElements.mParent.mElements.size());
					continue;
				}

				auto& MeshInstance = mStaticCompoundFile.mCompoundTagFile[StackIndex].mTagFile.mRootClass.mInstances.mParent.mElements.mParent.mElements[InstanceIndex];

				MeshInstance.mParent.mScale.mX = 0.0f;
				MeshInstance.mParent.mScale.mY = 0.0f;
				MeshInstance.mParent.mScale.mZ = 0.0f;

				MeshInstance.mParent.mTranslation.mX = 0.0f;
				MeshInstance.mParent.mTranslation.mY = 0.0f;
				MeshInstance.mParent.mTranslation.mZ = 0.0f;

				MeshInstance.mParent.mRotation.mParent.mVec.mData[0] = 0.0f;
				MeshInstance.mParent.mRotation.mParent.mVec.mData[1] = 0.0f;
				MeshInstance.mParent.mRotation.mParent.mVec.mData[2] = 0.0f;
				MeshInstance.mParent.mRotation.mParent.mVec.mData[3] = 0.0f;

				MeshInstance.mParent.mFlags.mParent = ((uint8_t)1 << 3); //HAS_SCALE

				RemovedCount++;
			}
		}

		application::util::Logger::Info("StaticCompoundImplementationSingleScene", "Removed %u hknpShapeInstances (%u in total)", RemovedCount, TotalCount);
	}

	void StaticCompoundImplementationSingleScene::CleanupCollisionVolume(void* Scene)
	{
		application::game::Scene* ScenePtr = reinterpret_cast<application::game::Scene*>(Scene);

		uint32_t RemovedCount = 0;

		for (size_t i = 0; i < mStaticCompoundFile.mCompoundTagFile.size(); i++)
		{
			for (auto& MeshInstance : mStaticCompoundFile.mCompoundTagFile[i].mTagFile.mRootClass.mInstances.mParent.mElements.mParent.mElements)
			{
				glm::vec3 Translation(MeshInstance.mParent.mTranslation.mX, MeshInstance.mParent.mTranslation.mY, MeshInstance.mParent.mTranslation.mZ);
				glm::vec3 Scale(MeshInstance.mParent.mScale.mX, MeshInstance.mParent.mScale.mY, MeshInstance.mParent.mScale.mZ);

				if (Scale == glm::vec3(0.0f))
					continue;

				bool Found = false;

				for (application::game::Scene::BancEntityRenderInfo* RenderInfo : ScenePtr->mDrawListRenderInfoIndices)
				{
					if (RenderInfo->mEntity->mGyml != "Area" || !RenderInfo->mEntity->mDynamic.contains("Starlight_GhostCollisionVolume") || !std::get<bool>(RenderInfo->mEntity->mDynamic["Starlight_GhostCollisionVolume"]))
						continue;

					glm::vec4 MinPoint(-1, 0, -1, 1);
					glm::vec4 MaxPoint(1, 2, 1, 1);

					glm::mat4 GLMModel = glm::mat4(1.0f); // Identity matrix

					GLMModel = glm::translate(GLMModel, RenderInfo->mTranslate);

					GLMModel = glm::rotate(GLMModel, glm::radians(RenderInfo->mRotate.z), glm::vec3(0.0, 0.0f, 1.0));
					GLMModel = glm::rotate(GLMModel, glm::radians(RenderInfo->mRotate.y), glm::vec3(0.0f, 1.0, 0.0));
					GLMModel = glm::rotate(GLMModel, glm::radians(RenderInfo->mRotate.x), glm::vec3(1.0, 0.0f, 0.0));

					GLMModel = glm::scale(GLMModel, RenderInfo->mScale);

					MinPoint = GLMModel * MinPoint;
					MaxPoint = GLMModel * MaxPoint;

					if (Translation.x >= MinPoint.x && Translation.y >= MinPoint.y && Translation.z >= MinPoint.z &&
						Translation.x <= MaxPoint.x && Translation.y <= MaxPoint.y && Translation.z <= MaxPoint.z)
					{
						Found = true;
						break;
					}
				}

				if (!Found) continue;

				MeshInstance.mParent.mScale.mX = 0.0f;
				MeshInstance.mParent.mScale.mY = 0.0f;
				MeshInstance.mParent.mScale.mZ = 0.0f;

				MeshInstance.mParent.mTranslation.mX = 0.0f;
				MeshInstance.mParent.mTranslation.mY = 0.0f;
				MeshInstance.mParent.mTranslation.mZ = 0.0f;

				MeshInstance.mParent.mRotation.mParent.mVec.mData[0] = 0.0f;
				MeshInstance.mParent.mRotation.mParent.mVec.mData[1] = 0.0f;
				MeshInstance.mParent.mRotation.mParent.mVec.mData[2] = 0.0f;
				MeshInstance.mParent.mRotation.mParent.mVec.mData[3] = 0.0f;

				MeshInstance.mParent.mFlags.mParent = ((uint8_t)1 << 3); //HAS_SCALE				
				RemovedCount++;
			}
		}

		application::util::Logger::Info("StaticCompoundImplementationSingleScene", "Removed %u hknpShapeInstances (GhostCollisionVolumeMode)", RemovedCount);
	}
}