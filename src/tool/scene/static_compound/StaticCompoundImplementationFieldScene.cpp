#include "StaticCompoundImplementationFieldScene.h"

#include <file/game/zstd/ZStdBackend.h>
#include <util/FileUtil.h>
#include <manager/ShaderMgr.h>
#include <manager/GameDataListMgr.h>

namespace application::tool::scene::static_compound
{
	StaticCompoundImplementationFieldScene::StaticCompoundImplementationFieldScene(application::game::Scene* Scene, const std::string& Path) : application::tool::scene::StaticCompoundImplementationBase(), mScene(Scene), mPath(Path)
	{

	}

	bool StaticCompoundImplementationFieldScene::Initialize()
	{
		//mStaticCompoundFile = application::file::game::phive::PhiveStaticCompoundFile(application::file::game::ZStdBackend::Decompress(application::util::FileUtil::GetRomFSFilePath(mPath)));

		mStaticCompoundFiles.emplace_back(application::file::game::ZStdBackend::Decompress(application::util::FileUtil::GetRomFSFilePath(mPath + "X0_Z0.Nin_NX_NVN.bphsc.zs")));
		mStaticCompoundFiles.emplace_back(application::file::game::ZStdBackend::Decompress(application::util::FileUtil::GetRomFSFilePath(mPath + "X1_Z0.Nin_NX_NVN.bphsc.zs")));
		mStaticCompoundFiles.emplace_back(application::file::game::ZStdBackend::Decompress(application::util::FileUtil::GetRomFSFilePath(mPath + "X2_Z0.Nin_NX_NVN.bphsc.zs")));
		mStaticCompoundFiles.emplace_back(application::file::game::ZStdBackend::Decompress(application::util::FileUtil::GetRomFSFilePath(mPath + "X3_Z0.Nin_NX_NVN.bphsc.zs")));

		mStaticCompoundFiles.emplace_back(application::file::game::ZStdBackend::Decompress(application::util::FileUtil::GetRomFSFilePath(mPath + "X0_Z1.Nin_NX_NVN.bphsc.zs")));
		mStaticCompoundFiles.emplace_back(application::file::game::ZStdBackend::Decompress(application::util::FileUtil::GetRomFSFilePath(mPath + "X1_Z1.Nin_NX_NVN.bphsc.zs")));
		mStaticCompoundFiles.emplace_back(application::file::game::ZStdBackend::Decompress(application::util::FileUtil::GetRomFSFilePath(mPath + "X2_Z1.Nin_NX_NVN.bphsc.zs")));
		mStaticCompoundFiles.emplace_back(application::file::game::ZStdBackend::Decompress(application::util::FileUtil::GetRomFSFilePath(mPath + "X3_Z1.Nin_NX_NVN.bphsc.zs")));

		mStaticCompoundFiles.emplace_back(application::file::game::ZStdBackend::Decompress(application::util::FileUtil::GetRomFSFilePath(mPath + "X0_Z2.Nin_NX_NVN.bphsc.zs")));
		mStaticCompoundFiles.emplace_back(application::file::game::ZStdBackend::Decompress(application::util::FileUtil::GetRomFSFilePath(mPath + "X1_Z2.Nin_NX_NVN.bphsc.zs")));
		mStaticCompoundFiles.emplace_back(application::file::game::ZStdBackend::Decompress(application::util::FileUtil::GetRomFSFilePath(mPath + "X2_Z2.Nin_NX_NVN.bphsc.zs")));
		mStaticCompoundFiles.emplace_back(application::file::game::ZStdBackend::Decompress(application::util::FileUtil::GetRomFSFilePath(mPath + "X3_Z2.Nin_NX_NVN.bphsc.zs")));

		mStaticCompoundFiles.emplace_back(application::file::game::ZStdBackend::Decompress(application::util::FileUtil::GetRomFSFilePath(mPath + "X0_Z3.Nin_NX_NVN.bphsc.zs")));
		mStaticCompoundFiles.emplace_back(application::file::game::ZStdBackend::Decompress(application::util::FileUtil::GetRomFSFilePath(mPath + "X1_Z3.Nin_NX_NVN.bphsc.zs")));
		mStaticCompoundFiles.emplace_back(application::file::game::ZStdBackend::Decompress(application::util::FileUtil::GetRomFSFilePath(mPath + "X2_Z3.Nin_NX_NVN.bphsc.zs")));
		mStaticCompoundFiles.emplace_back(application::file::game::ZStdBackend::Decompress(application::util::FileUtil::GetRomFSFilePath(mPath + "X3_Z3.Nin_NX_NVN.bphsc.zs")));

		LoadHashes();

		mModified.reserve(mStaticCompoundFiles.size());
		for (application::file::game::phive::PhiveStaticCompoundFile& File : mStaticCompoundFiles)
		{
			mModified.push_back(false);
		}

		ReloadMeshes();

		mShader = application::manager::ShaderMgr::GetShader("NavMesh");

		//mStaticCompoundFiles[13].mExternalBphshMeshes[0].mReserializeCollision = true;
		//application::util::FileUtil::WriteFile(application::util::FileUtil::GetWorkingDirFilePath("Reserialized.bphsc"), mStaticCompoundFiles[13].ToBinary());
		//application::util::FileUtil::WriteFile(application::util::FileUtil::GetWorkingDirFilePath("Original.bphsc"), application::file::game::ZStdBackend::Decompress(application::util::FileUtil::GetRomFSFilePath(mPath + "X1_Z3.Nin_NX_NVN.bphsc.zs")));


		return true;
	}

	void StaticCompoundImplementationFieldScene::ReloadMeshes()
	{
		for (size_t i = 0; i < mMeshes.size(); i++)
		{
			mMeshes[i].Delete();
		}
		mMeshes.clear();

		for (application::file::game::phive::PhiveStaticCompoundFile& File : mStaticCompoundFiles)
		{
			for (auto& ExternalBphshMesh : File.mExternalBphshMeshes)
			{
				std::vector<float> Vertices = ExternalBphshMesh.mPhiveShape.ToVertices();
				std::vector<uint32_t> Indices = ExternalBphshMesh.mPhiveShape.ToIndices();

				mMeshes.emplace_back(Vertices, Indices);
			}
		}
	}

	void StaticCompoundImplementationFieldScene::DrawStaticCompound(application::gl::Camera& Camera)
	{
		mShader->Bind();
		Camera.Matrix(mShader, "camMatrix");

		for (size_t i = 0; i < mMeshes.size(); i++)
		{
			glm::mat4 GLMModel = glm::mat4(1.0f);

			glm::vec3 Offset = GetStaticCompoundMiddlePoint(i % 4, i / 4);

			if (mScene->GetSceneTypeName(mScene->mSceneType) == "MinusField")
			{
				Offset.y -= 200.0f;
			}

			GLMModel = glm::translate(GLMModel, Offset);

			glUniformMatrix4fv(glGetUniformLocation(mShader->mID, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(GLMModel));

			mMeshes[i].Draw();
		}
	}

	uint64_t StaticCompoundImplementationFieldScene::RequestUniqueHash()
	{
		do mSmallestHash++; while (mSceneHashes.contains(mSmallestHash));

		return mSmallestHash;
	}

	uint32_t StaticCompoundImplementationFieldScene::RequestUniqueSRTHash()
	{
		do mSmallestSRTHash++; while (mSceneSRTHashes.contains(mSmallestSRTHash));

		return mSmallestSRTHash;
	}

	void StaticCompoundImplementationFieldScene::SyncAiGroupHashes(const uint64_t& OldHash, const uint64_t& NewHash)
	{
		for (application::game::Scene::AiGroup& Group : mScene->mAiGroups)
		{
			for (application::game::Scene::AiGroup::Reference& Ref : Group.mReferences)
			{
				if (Ref.mReference == OldHash) Ref.mReference = NewHash;
			}
		}
	}

	glm::vec3 StaticCompoundImplementationFieldScene::GetStaticCompoundMiddlePoint(uint8_t X, uint8_t Z)
	{
		glm::vec2 Result;
		Result.x = mScene->mBcettName[0] - 65;
		Result.y = ((char)mScene->mBcettName[2]) - '0' - 1;

		glm::vec2 Origin = glm::vec2((Result.x - 5) * 1000, (Result.y - 3) * 1000);
		Origin.y -= 1000.0f;

		Origin.x += X * 250.0f;
		Origin.y += Z * 250.0f;

		return glm::vec3(Origin.x, 0, Origin.y);
	}

	application::file::game::phive::PhiveStaticCompoundFile* StaticCompoundImplementationFieldScene::GetStaticCompoundAtIndex(const uint8_t& X, const uint8_t& Z)
	{
		return &mStaticCompoundFiles[X + Z * 4];
	}

	application::file::game::phive::PhiveStaticCompoundFile* StaticCompoundImplementationFieldScene::GetStaticCompoundForPos(glm::vec3 Pos)
	{
		if (mStaticCompoundFiles.empty())
			return nullptr;

		int32_t MapUnitDistanceX = 0;
		int32_t MapUnitDistanceZ = 0;

		char MapUnitIdentifierX = mScene->mBcettName[0];
		uint8_t MapUnitIdentifierZ = mScene->mBcettName[2] - 48; //48 = '0'

		MapUnitDistanceZ = (5 - MapUnitIdentifierZ) * 1000;
		MapUnitDistanceX = (MapUnitIdentifierX - 'F') * -1000;

		int8_t MapUnitDistanceZMultiplier = MapUnitIdentifierZ > 5 ? -1 : 1;

		Pos += glm::vec3(MapUnitDistanceX, 0, MapUnitDistanceZ * MapUnitDistanceZMultiplier);

		uint8_t StaticCompoundIndexX = 0;
		uint8_t StaticCompoundIndexZ = 0;

		if (Pos.x < 0)
		{
			StaticCompoundIndexX = 0;
		}
		else if (Pos.x > 250 && Pos.x <= 500)
		{
			StaticCompoundIndexX = 1;
		}
		else if (Pos.x > 500 && Pos.x <= 750)
		{
			StaticCompoundIndexX = 2;
		}
		else if (Pos.x > 750)
		{
			StaticCompoundIndexX = 3;
		}

		if (Pos.z < 0)
		{
			StaticCompoundIndexX = 0;
		}
		else if (Pos.z > 250 && Pos.z <= 500)
		{
			StaticCompoundIndexZ = 1;
		}
		else if (Pos.z > 500 && Pos.z <= 750)
		{
			StaticCompoundIndexZ = 2;
		}
		else if (Pos.z > 750)
		{
			StaticCompoundIndexZ = 3;
		}

		uint8_t FileIndex = StaticCompoundIndexX + 4 * StaticCompoundIndexZ;

		if (FileIndex >= mStaticCompoundFiles.size())
			return nullptr;

		return &mStaticCompoundFiles[StaticCompoundIndexX + 4 * StaticCompoundIndexZ];
	}

	bool StaticCompoundImplementationFieldScene::Save()
	{
		if (!mGeneratePhysicsHashes)
			return true;

		for (application::game::BancEntity& Entity : mScene->mBancEntities)
		{
			if (Entity.mActorPack->mNeedsPhysicsHash && Entity.mCheckForHashGeneration)
			{
				application::file::game::phive::PhiveStaticCompoundFile* StaticCompound = GetStaticCompoundForPos(Entity.mTranslate);

				if (StaticCompound == nullptr)
				{
					application::util::Logger::Error("StaticCompoundImplementationFieldScene", "application::file::game::phive::PhiveStaticCompoundFile* was a nullptr");
					continue;
				}

				bool Found = false;
				for (application::file::game::phive::PhiveStaticCompoundFile::PhiveStaticCompoundActorLink& Link : StaticCompound->mActorLinks)
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
				for (size_t i = 0; i < StaticCompound->mActorHashMap.size(); i++)
				{
					if (StaticCompound->mActorHashMap[i] == 0)
					{
						OptimalIndex = i;
						break;
					}
				}
				if (OptimalIndex == 0xFFFFFFFF)
				{
					OptimalIndex = StaticCompound->mActorLinks.size() * 2;
				}

				uint64_t OldHash = Entity.mHash;

				while (true)
				{
					uint64_t Modulus = (StaticCompound->mActorLinks.size() + 1) * 2;
					uint64_t Remainder = Entity.mHash % Modulus;
					uint64_t Difference = (OptimalIndex >= Remainder) ? (OptimalIndex - Remainder) : (Modulus - Remainder + OptimalIndex);
					Entity.mHash += Difference;

					bool Duplicate = false;
					for (const application::game::BancEntity& SceneEntity : mScene->mBancEntities)
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

						for (const application::game::BancEntity& ChildEntity : *SceneEntity.mMergedActorChildren)
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

				if (StaticCompound->mActorHashMap.size() > OptimalIndex)
				{
					StaticCompound->mActorHashMap[OptimalIndex] = Entity.mHash;
				}
				else
				{
					StaticCompound->mActorHashMap.resize(OptimalIndex + 2);
					StaticCompound->mActorHashMap[OptimalIndex] = Entity.mHash;
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

				StaticCompound->mActorLinks.push_back(Link);

				Entity.mPhive.mPlacement["ID"] = Entity.mHash;

				SyncAiGroupHashes(OldHash, Entity.mHash);

				for (application::game::BancEntity& SceneEntity : mScene->mBancEntities)
				{
					SyncBancEntityHashes(SceneEntity, OldHash, Entity.mHash);
				}

				Entity.mCheckForHashGeneration = false;

				mModified[std::distance(&mStaticCompoundFiles[0], StaticCompound)] = true;
			}
		}

		for (const bool& Modified : mModified)
		{
			if (Modified)
			{
				std::filesystem::create_directories(std::filesystem::path(application::util::FileUtil::GetSaveFilePath(mPath)).parent_path());
				break;
			}
		}

		for (size_t i = 0; i < mStaticCompoundFiles.size(); i++)
		{
			if (!mModified[i])
				continue;

			application::util::FileUtil::WriteFile(application::util::FileUtil::GetSaveFilePath(mPath + "X" + std::to_string((int)(i % 4)) + "_Z" + std::to_string((int)std::floor(i / 4.0f)) + ".Nin_NX_NVN.bphsc.zs"), application::file::game::ZStdBackend::Compress(mStaticCompoundFiles[i].ToBinary(), application::file::game::ZStdBackend::Dictionary::Base));
			mModified[i] = false;
		}
		LoadHashes();

		return true;
	}

	void StaticCompoundImplementationFieldScene::LoadHashes()
	{
		mSmallestHash = std::numeric_limits<uint64_t>::max();
		mSmallestSRTHash = std::numeric_limits<uint32_t>::max();

		//Calculating smallest hash
		for (const application::game::BancEntity& Entity : mScene->mBancEntities)
		{
			mSceneHashes.insert(Entity.mHash);
			mSceneSRTHashes.insert(Entity.mSRTHash);

			if (Entity.mMergedActorChildren == nullptr)
				continue;

			for (const application::game::BancEntity& ChildEntity : *Entity.mMergedActorChildren)
			{
				mSceneHashes.insert(ChildEntity.mHash);
				mSceneSRTHashes.insert(ChildEntity.mSRTHash);
			}
		}

		if (mScene->mBancEntities.empty())
		{
			for (const application::file::game::phive::PhiveStaticCompoundFile& Compound : mStaticCompoundFiles)
			{
				for (auto& ActorLink : Compound.mActorLinks)
				{
					mSceneHashes.insert(ActorLink.mActorHash);
					mSceneSRTHashes.insert(ActorLink.mReserve0);
				}
			}
		}

		for (const uint64_t& Hash : mSceneHashes)
			mSmallestHash = std::min(mSmallestHash, Hash);

		for (const uint32_t& SRTHash : mSceneSRTHashes)
			mSmallestSRTHash = std::min(mSmallestSRTHash, SRTHash);
	}

	void StaticCompoundImplementationFieldScene::OnBancEntityDelete(void* RenderInfo)
	{
		application::game::Scene::BancEntityRenderInfo* EntityRenderInfo = reinterpret_cast<application::game::Scene::BancEntityRenderInfo*>(RenderInfo);

		/*
		for (size_t i = 0; i < mStaticCompoundFiles.size(); i++)
		{
			for(application::file::game::phive::PhiveStaticCompoundFile::PhiveStaticCompoundHkTagFile& TagFile : mStaticCompoundFiles[i].mCompoundTagFile)
			{
				if(TagFile.mTagFile.mRootClass.mInstances.mParent.mElements.mParent.mElements.empty())
					continue;


				for(size_t j = 0; j < TagFile.mTagFile.mRootClass.mInstances.mParent.mElements.mParent.mElements.size(); j++)
				{
					auto& Instance = TagFile.mTagFile.mRootClass.mInstances.mParent.mElements.mParent.mElements[j];

					if(Instance.mParent.mTranslation.mX == EntityRenderInfo->mTranslate.x &&
					   Instance.mParent.mTranslation.mY == EntityRenderInfo->mTranslate.y &&
					   Instance.mParent.mTranslation.mZ == EntityRenderInfo->mTranslate.z)
					{
						application::util::Logger::Info("StaticCompoundImplementationFieldScene", "Found by position match at %zu", j);
					}
				}
			}
		}
		*/

		for (size_t i = 0; i < mStaticCompoundFiles.size(); i++)
		{
			for (application::file::game::phive::PhiveStaticCompoundFile::PhiveStaticCompoundActorLink& Link : mStaticCompoundFiles[i].mActorLinks)
			{
				if (Link.mActorHash == EntityRenderInfo->mEntity->mHash)
				{
					if (Link.mBaseIndex == 0xFFFFFFFF || Link.mEndIndex == 0xFFFFFFFF)
					{
						break;
					}

					for (int baseIndex = Link.mBaseIndex; baseIndex <= Link.mEndIndex; baseIndex++)
					{
						/* I believe stack index is not really the compound file index
						uint8_t StackIndex = mStaticCompoundFiles[i].mFixedOffset0Array[baseIndex].mStackIndex;
						*/

						const uint8_t StackIndex = 1;

						if (StackIndex >= mStaticCompoundFiles[i].mCompoundTagFile.size())
						{
							application::util::Logger::Warning("StaticCompoundImplementationFieldScene", "Failed to remove hknpShapeInstance, invalid stack index %u (max %zu) in %u", StackIndex, mStaticCompoundFiles[i].mCompoundTagFile.size(), i);
							continue;
						}

						uint8_t InstanceIndex = mStaticCompoundFiles[i].mFixedOffset0Array[baseIndex].mInstanceIndex;

						if (InstanceIndex >= mStaticCompoundFiles[i].mCompoundTagFile[StackIndex].mTagFile.mRootClass.mInstances.mParent.mElements.mParent.mElements.size())
						{
							application::util::Logger::Warning("StaticCompoundImplementationFieldScene", "Failed to remove hknpShapeInstance, invalid instance index %u (max %zu) in %u", InstanceIndex, mStaticCompoundFiles[i].mCompoundTagFile[StackIndex].mTagFile.mRootClass.mInstances.mParent.mElements.mParent.mElements.size(), i);
							continue;
						}

						auto& MeshInstance = mStaticCompoundFiles[i].mCompoundTagFile[StackIndex].mTagFile.mRootClass.mInstances.mParent.mElements.mParent.mElements[InstanceIndex];

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

						mModified[i] = true;

						application::util::Logger::Info("StaticCompoundImplementationFieldScene", "Removed hknpShapeInstance");
					}

					break;
				}
			}
		}
	}

	void StaticCompoundImplementationFieldScene::CleanupCollision(void* Scene)
	{
		application::game::Scene* ScenePtr = reinterpret_cast<application::game::Scene*>(Scene);

		uint32_t TotalCount = 0;
		for (size_t i = 0; i < mStaticCompoundFiles.size(); i++)
		{
			for (size_t j = 0; j < mStaticCompoundFiles[i].mCompoundTagFile.size(); j++)
			{
				TotalCount += mStaticCompoundFiles[i].mCompoundTagFile[j].mTagFile.mRootClass.mInstances.mParent.mElements.mParent.mElements.size();
			}
		}

		uint32_t RemovedCount = 0;

		for (size_t i = 0; i < mStaticCompoundFiles.size(); i++)
		{
			for (application::file::game::phive::PhiveStaticCompoundFile::PhiveStaticCompoundActorLink& Link : mStaticCompoundFiles[i].mActorLinks)
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

					if (StackIndex >= mStaticCompoundFiles[i].mCompoundTagFile.size())
					{
						application::util::Logger::Warning("StaticCompoundImplementationSingleScene", "Failed to remove hknpShapeInstance, invalid stack index %u (max %zu)", StackIndex, mStaticCompoundFiles[i].mCompoundTagFile.size());
						continue;
					}

					uint8_t InstanceIndex = mStaticCompoundFiles[i].mFixedOffset0Array[baseIndex].mInstanceIndex;

					if (InstanceIndex >= mStaticCompoundFiles[i].mCompoundTagFile[StackIndex].mTagFile.mRootClass.mInstances.mParent.mElements.mParent.mElements.size())
					{
						application::util::Logger::Warning("StaticCompoundImplementationSingleScene", "Failed to remove hknpShapeInstance, invalid instance index %u (max %zu)", InstanceIndex, mStaticCompoundFiles[i].mCompoundTagFile[StackIndex].mTagFile.mRootClass.mInstances.mParent.mElements.mParent.mElements.size());
						continue;
					}

					auto& MeshInstance = mStaticCompoundFiles[i].mCompoundTagFile[StackIndex].mTagFile.mRootClass.mInstances.mParent.mElements.mParent.mElements[InstanceIndex];

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
					mModified[i] = true;
				}
			}
		}

		application::util::Logger::Info("StaticCompoundImplementationFieldScene", "Removed %u hknpShapeInstances (%u in total)", RemovedCount, TotalCount);
	}
}