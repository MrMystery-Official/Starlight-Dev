#include "Scene.h"

#include <unordered_set>
#include <file/game/byml/BymlFile.h>
#include <file/game/zstd/ZStdBackend.h>
#include <util/Logger.h>
#include <util/FileUtil.h>
#include <util/Math.h>
#include <util/General.h>
#include <glm/gtx/euler_angles.hpp>
#include <manager/MergedActorMgr.h>
#include <manager/ActorPackMgr.h>
#include <manager/BfresFileMgr.h>
#include <manager/BfresRendererMgr.h>
#include <manager/CaveMgr.h>
#include <manager/GameDataListMgr.h>
#include <tool/scene/navmesh/NavMeshImplementationSingleScene.h>
#include <tool/scene/navmesh/NavMeshImplementationFieldScene.h>
#include <tool/scene/static_compound/StaticCompoundImplementationSingleScene.h>
#include <tool/scene/static_compound/StaticCompoundImplementationFieldScene.h>

namespace application::game
{
	void Scene::Import(Scene& other)
	{
		mTerrainRenderer = other.mTerrainRenderer;
		mStaticBymlFile = other.mStaticBymlFile;
		mDynamicBymlFile = other.mDynamicBymlFile;

		mSceneType = other.mSceneType;
		mBcettName = other.mBcettName;
		mBancEntities = other.mBancEntities;
		mAiGroups = other.mAiGroups;

		mGameBancParamPath = other.mGameBancParamPath;
		mGameBancParam = other.mGameBancParam;
		mTerrainRenderer = other.mTerrainRenderer;
		mGeneratePhysicsHashes = other.mGeneratePhysicsHashes;
		mDebugMeshes = other.mDebugMeshes;

		if (other.mNavMeshImplementation)
			mNavMeshImplementation.reset();

		if (other.mStaticCompoundImplementation)
		{
			switch (mSceneType)
			{
			case SceneType::SMALL_DUNGEON:
				mStaticCompoundImplementation = std::make_unique<application::tool::scene::static_compound::StaticCompoundImplementationSingleScene>(this, "Phive/StaticCompoundBody/SmallDungeon/" + mBcettName + ".Nin_NX_NVN.bphsc.zs");
				break;
			case SceneType::NORMAL_STAGE:
				mStaticCompoundImplementation = std::make_unique<application::tool::scene::static_compound::StaticCompoundImplementationSingleScene>(this, "Phive/StaticCompoundBody/NormalStage/" + mBcettName + ".Nin_NX_NVN.bphsc.zs");
				break;
			case SceneType::LARGA_DUNGEON:
				if (mBcettName == "LargeDungeonFire")
					mStaticCompoundImplementation = std::make_unique<application::tool::scene::static_compound::StaticCompoundImplementationSingleScene>(this, "Phive/StaticCompoundBody/MinusField/LargeDungeon/" + mBcettName + ".Nin_NX_NVN.bphsc.zs");
				else
					mStaticCompoundImplementation = std::make_unique<application::tool::scene::static_compound::StaticCompoundImplementationSingleScene>(this, "Phive/StaticCompoundBody/MainField/LargeDungeon/" + mBcettName + ".Nin_NX_NVN.bphsc.zs");
				break;
			case SceneType::MAIN_FIELD:
				if (mBcettName.length() == 3)
					mStaticCompoundImplementation = std::make_unique<application::tool::scene::static_compound::StaticCompoundImplementationFieldScene>(this, "Phive/StaticCompoundBody/MainField/MainField_" + mBcettName + "_"); //This is the RomFS prefix to the files
				else
					mStaticCompoundImplementation = std::make_unique<application::tool::scene::static_compound::StaticCompoundImplementationSingleScene>(this, "Phive/StaticCompoundBody/MainField/Castle/" + mBcettName + ".Nin_NX_NVN.bphsc.zs");
				break;
			case SceneType::MINUS_FIELD:
				mStaticCompoundImplementation = std::make_unique<application::tool::scene::static_compound::StaticCompoundImplementationFieldScene>(this, "Phive/StaticCompoundBody/MinusField/MinusField_" + mBcettName + "_"); //This is the RomFS prefix to the files
				break;
			case SceneType::TRIAL_FIELD:
				mStaticCompoundImplementation = std::make_unique<application::tool::scene::static_compound::StaticCompoundImplementationFieldScene>(this, "Phive/StaticCompoundBody/TrialField/TrialField_" + mBcettName + "_"); //This is the RomFS prefix to the files
				break;
			default:
				mStaticCompoundImplementation = std::make_unique<application::tool::scene::StaticCompoundImplementationBase>();
				application::util::Logger::Info("Scene", "No StaticCompound implementation found");
			}
			mStaticCompoundImplementation->Initialize();
		}

		GenerateDrawList();
	}

	void Scene::Load(SceneType Type, const std::string& Identifier, float TerrainLOD)
	{
		switch (Type)
		{
		case SceneType::SMALL_DUNGEON:
			Load(Type, application::util::FileUtil::GetRomFSFilePath("Banc/SmallDungeon/" + Identifier + "_Static.bcett.byml.zs"), application::util::FileUtil::GetRomFSFilePath("Banc/SmallDungeon/" + Identifier + "_Dynamic.bcett.byml.zs"), TerrainLOD);
			break;
		case SceneType::NORMAL_STAGE:
			Load(Type, application::util::FileUtil::GetRomFSFilePath("Banc/NormalStage/" + Identifier + "_Static.bcett.byml.zs"), application::util::FileUtil::GetRomFSFilePath("Banc/NormalStage/" + Identifier + "_Dynamic.bcett.byml.zs"), TerrainLOD);
			break;
		case SceneType::LARGA_DUNGEON:
			if(Identifier == "LargeDungeonFire")
				Load(Type, application::util::FileUtil::GetRomFSFilePath("Banc/MinusField/LargeDungeon/" + Identifier + "_Static.bcett.byml.zs"), application::util::FileUtil::GetRomFSFilePath("Banc/MinusField/LargeDungeon/" + Identifier + "_Dynamic.bcett.byml.zs"), TerrainLOD);
			else
				Load(Type, application::util::FileUtil::GetRomFSFilePath("Banc/MainField/LargeDungeon/" + Identifier + "_Static.bcett.byml.zs"), application::util::FileUtil::GetRomFSFilePath("Banc/MainField/LargeDungeon/" + Identifier + "_Dynamic.bcett.byml.zs"), TerrainLOD);
			break;
		case SceneType::MAIN_FIELD:
			if (Identifier != "Set_HyruleCastle_IslandGround_A")
				Load(Type, application::util::FileUtil::GetRomFSFilePath("Banc/MainField/" + Identifier + "_Static.bcett.byml.zs"), application::util::FileUtil::GetRomFSFilePath("Banc/MainField/" + Identifier + "_Dynamic.bcett.byml.zs"), TerrainLOD);
			else
				Load(Type, application::util::FileUtil::GetRomFSFilePath("Banc/MainField/Castle/" + Identifier + "_Static.bcett.byml.zs"), application::util::FileUtil::GetRomFSFilePath("Banc/MainField/Castle/" + Identifier + "_Dynamic.bcett.byml.zs"), TerrainLOD);
			break;
		case SceneType::MINUS_FIELD:
			Load(Type, application::util::FileUtil::GetRomFSFilePath("Banc/MinusField/" + Identifier + "_Static.bcett.byml.zs"), application::util::FileUtil::GetRomFSFilePath("Banc/MinusField/" + Identifier + "_Dynamic.bcett.byml.zs"), TerrainLOD);
			break;
		case SceneType::TRIAL_FIELD:
			Load(Type, application::util::FileUtil::GetRomFSFilePath("Banc/TrialField/" + Identifier + "_Static.bcett.byml.zs"), application::util::FileUtil::GetRomFSFilePath("Banc/TrialField/" + Identifier + "_Dynamic.bcett.byml.zs"), TerrainLOD);
			break;
		case SceneType::SKY:
			Load(Type, application::util::FileUtil::GetRomFSFilePath("Banc/MainField/Sky/" + Identifier + "_Static.bcett.byml.zs"), application::util::FileUtil::GetRomFSFilePath("Banc/MainField/Sky/" + Identifier + "_Dynamic.bcett.byml.zs"), TerrainLOD);
			break;
		case SceneType::NONE:
		default:
			application::util::Logger::Error("Scene", "Scene Type not supported");
		}
	}

	//This function will load a scene without unloading an old one. That way multiple field sections can be loaded at once. This is only useful for viewing though as saving is not supported
	void Scene::_LoadInplace(SceneType Type, const std::string& StaticBymlPath, const std::string& DynamicBymlPath)
	{
		if (mSceneType == SceneType::NONE)
		{
			application::util::Logger::Error("Scene", "Tried to initialize game scene in place without loaded scene");
			return;
		}

		application::file::game::byml::BymlFile StaticBymlFile(application::file::game::ZStdBackend::Decompress(StaticBymlPath));
		application::file::game::byml::BymlFile DynamicBymlFile(application::file::game::ZStdBackend::Decompress(DynamicBymlPath));

		auto DecodeByml = [this](application::file::game::byml::BymlFile& File, application::game::BancEntity::BancType Type)
			{
				if (!File.HasChild("Actors"))
					return;

				for (application::file::game::byml::BymlFile::Node& ActorNode : File.GetNode("Actors")->GetChildren())
				{
					application::game::BancEntity Entity;
					if (!Entity.FromByml(ActorNode))
					{
						application::util::Logger::Error("Scene", "Error while parsing BancEntity");
						continue;
					}
					Entity.mBancType = Type;

					mBancEntities.push_back(Entity);
				}
			};

		auto GetActorCount = [](application::file::game::byml::BymlFile& File)
			{
				if (!File.HasChild("Actors")) return (unsigned int)0;

				return (unsigned int)File.GetNode("Actors")->GetChildren().size();
			};

		mBancEntities.reserve(mBancEntities.size() + GetActorCount(StaticBymlFile) + GetActorCount(DynamicBymlFile));

		DecodeByml(StaticBymlFile, application::game::BancEntity::BancType::STATIC);
		DecodeByml(DynamicBymlFile, application::game::BancEntity::BancType::DYNAMIC);

		mBancEntities.shrink_to_fit();

		std::string BcettName = std::filesystem::path(StaticBymlPath).stem().string();

		switch (Type)
		{
		case SceneType::MAIN_FIELD:
		{
			break;
		}
		default:
			application::util::Logger::Info("Scene", "No terrain will be loaded for this scene");
		}

		GenerateDrawList();
	}

	void Scene::Load(Scene::SceneType Type, const std::string& StaticBymlPath, const std::string& DynamicBymlPath, float TerrainLOD)
	{
		if (mSceneType != SceneType::NONE)
		{
			//_LoadInplace(mSceneType, StaticBymlPath, DynamicBymlPath);
			application::util::Logger::Error("Scene", "Tried to initialize game scene multiple times, aborting");
			return;
		}

		/*
		OpenMesh::TriMesh_ArrayKernelT<> MyMesh;
		if (OpenMesh::IO::read_mesh(MyMesh, "C:/Users/XXX/Downloads/cave.obj"))
		{
			application::util::Logger::Info("Scene", "Loaded cave mesh");
			std::vector<float> Vertices;
			std::vector<uint32_t> Indices;

			// Iterate through all vertices
			for (const auto& vh : MyMesh.vertices()) {
				auto point = MyMesh.point(vh);
				Vertices.push_back(point[0]); // x
				Vertices.push_back(point[1]); // y
				Vertices.push_back(point[2]); // z
			}

			// Iterate through all faces (triangles)
			for (const auto& fh : MyMesh.faces()) {
				std::vector<uint32_t> face_indices;
				for (const auto& vh : MyMesh.fv_range(fh)) {
					face_indices.push_back(vh.idx());
				}
				if (face_indices.size() == 3) { // Ensure it's a triangle
					Indices.push_back(face_indices[0]);
					Indices.push_back(face_indices[1]);
					Indices.push_back(face_indices[2]);
				}
			}

			mCaveRenderer = application::gl::CaveRenderer();
			mCaveRenderer.value().Initialize(Vertices, Indices);
		}
		else
		{
			application::util::Logger::Error("Scene", "Could not load cave mesh");
		}
		*/

		mSceneType = Type;
		mBcettName = std::filesystem::path(StaticBymlPath).stem().string();
		application::util::General::ReplaceString(mBcettName, "_Static.bcett.byml", "");

		mStaticBymlFile.Initialize(application::file::game::ZStdBackend::Decompress(StaticBymlPath));
		mDynamicBymlFile.Initialize(application::file::game::ZStdBackend::Decompress(DynamicBymlPath));

		auto DecodeByml = [this](application::file::game::byml::BymlFile& File, application::game::BancEntity::BancType Type)
			{
				if (!File.HasChild("Actors")) 
					return;

				for (application::file::game::byml::BymlFile::Node& ActorNode : File.GetNode("Actors")->GetChildren())
				{
					application::game::BancEntity Entity;
					if (!Entity.FromByml(ActorNode))
					{
						application::util::Logger::Error("Scene", "Error while parsing BancEntity");
						continue;
					}
					Entity.mBancType = Type;

					mBancEntities.push_back(Entity);
				}
			};

		auto GetActorCount = [](application::file::game::byml::BymlFile& File)
			{
				if (!File.HasChild("Actors")) return (unsigned int)0;

				return (unsigned int)File.GetNode("Actors")->GetChildren().size();
			};

		mBancEntities.reserve(GetActorCount(mStaticBymlFile) + GetActorCount(mDynamicBymlFile));

		DecodeByml(mStaticBymlFile, application::game::BancEntity::BancType::STATIC);
		DecodeByml(mDynamicBymlFile, application::game::BancEntity::BancType::DYNAMIC);

		mBancEntities.shrink_to_fit();

		GenerateDrawList();

		application::manager::GameDataListMgr::LoadActorGameData(*this);

		switch (mSceneType)
		{
		case SceneType::SMALL_DUNGEON:
		case SceneType::NORMAL_STAGE:
			mGameBancParamPath = "Banc/GameBancParam/" + mBcettName + ".game__banc__GameBancParam.bgyml";
			break;
		case SceneType::LARGA_DUNGEON:
			mGameBancParamPath = "Banc/GameBancParam/" + (mBcettName == "LargeDungeonWater_AllinArea" ? "LargeDungeonWater" : mBcettName) + ".game__banc__GameBancParam.bgyml";
			break;
		case SceneType::MAIN_FIELD:
		case SceneType::MINUS_FIELD:
		case SceneType::SKY:
			mGameBancParamPath = "Banc/GameBancParam/MainField.game__banc__GameBancParam.bgyml";
			break;
		case SceneType::TRIAL_FIELD:
			mGameBancParamPath = "Banc/GameBancParam/TrialField.game__banc__GameBancParam.bgyml";
			break;
		default:
			application::util::Logger::Error("Scene", "No GameBancParam path set");
		}

		if (!mGameBancParamPath.empty())
		{
			mGameBancParam = application::file::game::byml::BymlFile(application::util::FileUtil::GetRomFSFilePath(mGameBancParamPath));
		}

		switch (mSceneType)
		{
		case SceneType::SMALL_DUNGEON:
			mStaticCompoundImplementation = std::make_unique<application::tool::scene::static_compound::StaticCompoundImplementationSingleScene>(this, "Phive/StaticCompoundBody/SmallDungeon/" + mBcettName + ".Nin_NX_NVN.bphsc.zs");
			break;
		case SceneType::NORMAL_STAGE:
			mStaticCompoundImplementation = std::make_unique<application::tool::scene::static_compound::StaticCompoundImplementationSingleScene>(this, "Phive/StaticCompoundBody/NormalStage/" + mBcettName + ".Nin_NX_NVN.bphsc.zs");
			break;
		case SceneType::LARGA_DUNGEON:
			if(mBcettName == "LargeDungeonFire")
				mStaticCompoundImplementation = std::make_unique<application::tool::scene::static_compound::StaticCompoundImplementationSingleScene>(this, "Phive/StaticCompoundBody/MinusField/LargeDungeon/" + mBcettName + ".Nin_NX_NVN.bphsc.zs");
			else
				mStaticCompoundImplementation = std::make_unique<application::tool::scene::static_compound::StaticCompoundImplementationSingleScene>(this, "Phive/StaticCompoundBody/MainField/LargeDungeon/" + mBcettName + ".Nin_NX_NVN.bphsc.zs");
			break;
		case SceneType::MAIN_FIELD:
			if (mBcettName.length() == 3)
				mStaticCompoundImplementation = std::make_unique<application::tool::scene::static_compound::StaticCompoundImplementationFieldScene>(this, "Phive/StaticCompoundBody/MainField/MainField_" + mBcettName + "_"); //This is the RomFS prefix to the files
			else
				mStaticCompoundImplementation = std::make_unique<application::tool::scene::static_compound::StaticCompoundImplementationSingleScene>(this, "Phive/StaticCompoundBody/MainField/Castle/" + mBcettName + ".Nin_NX_NVN.bphsc.zs");
			break;
		case SceneType::MINUS_FIELD:
			mStaticCompoundImplementation = std::make_unique<application::tool::scene::static_compound::StaticCompoundImplementationFieldScene>(this, "Phive/StaticCompoundBody/MinusField/MinusField_" + mBcettName + "_"); //This is the RomFS prefix to the files
			break;
		case SceneType::TRIAL_FIELD:
			mStaticCompoundImplementation = std::make_unique<application::tool::scene::static_compound::StaticCompoundImplementationFieldScene>(this, "Phive/StaticCompoundBody/TrialField/TrialField_" + mBcettName + "_"); //This is the RomFS prefix to the files
			break;
		default:
			mStaticCompoundImplementation = std::make_unique<application::tool::scene::StaticCompoundImplementationBase>();
			application::util::Logger::Info("Scene", "No StaticCompound implementation found");
		}
		mStaticCompoundImplementation->Initialize();

		switch (mSceneType)
		{
		case SceneType::MAIN_FIELD:
			if (mBcettName.length() == 3)
				mNavMeshImplementation = std::make_unique<application::tool::scene::navmesh::NavMeshImplementationFieldScene>(this);
			else
				mNavMeshImplementation = std::make_unique<application::tool::scene::NavMeshImplementationBase>(); //Placeholder
			break;
		case SceneType::TRIAL_FIELD:
			mNavMeshImplementation = std::make_unique<application::tool::scene::navmesh::NavMeshImplementationFieldScene>(this);
			break;
		case SceneType::MINUS_FIELD:
			mNavMeshImplementation = std::make_unique<application::tool::scene::navmesh::NavMeshImplementationFieldScene>(this);
			break;
		case SceneType::SMALL_DUNGEON:
			mNavMeshImplementation = std::make_unique<application::tool::scene::navmesh::NavMeshImplementationSingleScene>(this);
			break;
		default:
			mNavMeshImplementation = std::make_unique<application::tool::scene::NavMeshImplementationBase>();
			application::util::Logger::Info("Scene", "No NavMesh implementation found");
		}
		mNavMeshImplementation->Initialize();

		switch (mSceneType)
		{
		case SceneType::MAIN_FIELD:
			if (mBcettName.length() == 3)
				mTerrainRenderer = application::manager::TerrainMgr::GetTerrainRenderer("MainField", mBcettName);
			break;
		case SceneType::MINUS_FIELD:
			mTerrainRenderer = application::manager::TerrainMgr::GetTerrainRenderer("MinusField", mBcettName);
			break;
		case SceneType::TRIAL_FIELD:
			mTerrainRenderer = application::manager::TerrainMgr::GetTerrainRenderer("TrialField", mBcettName);
			break;
		default:
			application::util::Logger::Info("Scene", "No terrain will be loaded for this scene");
		}

		if (mStaticBymlFile.HasChild("AiGroups"))
		{
			mAiGroups.reserve(mStaticBymlFile.GetNode("AiGroups")->GetChildren().size());

			for (application::file::game::byml::BymlFile::Node& AiGroupNode : mStaticBymlFile.GetNode("AiGroups")->GetChildren())
			{
				application::game::Scene::AiGroup AiGroup;

				AiGroup.mHash = AiGroupNode.GetChild("Hash")->GetValue<uint64_t>();
				if (AiGroupNode.HasChild("Logic"))
				{
					AiGroup.mType = application::game::Scene::AiGroup::Type::LOGIC;
					AiGroup.mPath = AiGroupNode.GetChild("Logic")->GetValue<std::string>();
				}
				else if (AiGroupNode.HasChild("Meta"))
				{
					AiGroup.mType = application::game::Scene::AiGroup::Type::META;
					AiGroup.mPath = AiGroupNode.GetChild("Meta")->GetValue<std::string>();
				}
				else
				{
					application::util::Logger::Warning("Scene", "Error while decoding AiGroup %lu", AiGroup.mHash);
					continue;
				}

				if (AiGroupNode.HasChild("References"))
				{
					AiGroup.mReferences.reserve(AiGroupNode.GetChild("References")->GetChildren().size());

					for (application::file::game::byml::BymlFile::Node& RefNode : AiGroupNode.GetChild("References")->GetChildren())
					{
						application::game::Scene::AiGroup::Reference Ref;

						if (RefNode.HasChild("Id")) Ref.mId = RefNode.GetChild("Id")->GetValue<std::string>();
						if (RefNode.HasChild("Path")) Ref.mPath = RefNode.GetChild("Path")->GetValue<std::string>();
						if (RefNode.HasChild("Reference")) Ref.mReference = RefNode.GetChild("Reference")->GetValue<uint64_t>();

						if (RefNode.HasChild("InstanceName")) Ref.mInstanceName = RefNode.GetChild("InstanceName")->GetValue<std::string>();
						if (RefNode.HasChild("Logic")) Ref.mLogic = RefNode.GetChild("Logic")->GetValue<std::string>();
						if (RefNode.HasChild("Meta")) Ref.mMeta = RefNode.GetChild("Meta")->GetValue<std::string>();

						AiGroup.mReferences.push_back(Ref);
					}
				}

				if(AiGroupNode.HasChild("Blackboards"))
				{
					AiGroup.mBlackboards.reserve(AiGroupNode.GetChild("Blackboards")->GetChildren().size());
					for (application::file::game::byml::BymlFile::Node& BBNode : AiGroupNode.GetChild("Blackboards")->GetChildren())
					{
						AiGroup.mBlackboards.push_back(BBNode.GetValue<std::string>());
					}
				}

				if (AiGroupNode.HasChild("InstanceName"))
				{
					AiGroup.mInstanceName = AiGroupNode.GetChild("InstanceName")->GetValue<std::string>();
				}

				mAiGroups.push_back(AiGroup);
			}
		}

		application::util::Logger::Info("Scene", "Scene loaded, unique BancEntity count: %u, AiGroup count: %u", mBancEntities.size(), mAiGroups.size());
	}

	bool Scene::IsLoaded()
	{
		return mSceneType != SceneType::NONE;
	}

	bool Scene::IsTerrainScene()
	{
		return mBcettName.find("-") != std::string::npos;
	}

	void Scene::UpdateChildsRenderInfo(application::game::BancEntity* Parent, BancEntityRenderInfo& ChildRenderInfo)
	{
		ChildRenderInfo.mScale = Parent->mScale * ChildRenderInfo.mEntity->mScale;


		//Childs translation depending on parents rotation (orbit)
		glm::vec3 ParentRotationRadians = glm::radians(Parent->mRotate);
		glm::mat4 Transform = glm::mat4(1.0f);

		Transform = glm::translate(Transform, Parent->mTranslate);
		Transform = glm::rotate(Transform, ParentRotationRadians.z, glm::vec3(0.0f, 0.0f, 1.0f));
		Transform = glm::rotate(Transform, ParentRotationRadians.y, glm::vec3(0.0f, 1.0f, 0.0f));
		Transform = glm::rotate(Transform, ParentRotationRadians.x, glm::vec3(1.0f, 0.0f, 0.0f));

		glm::vec4 TransformedOffset = Transform * glm::vec4(ChildRenderInfo.mEntity->mTranslate, 1.0f);
		ChildRenderInfo.mTranslate = glm::vec3(TransformedOffset);



		//Childs rotation
		glm::vec3 ChildRotationRadians = glm::radians(ChildRenderInfo.mEntity->mRotate);
		glm::quat ParentQuat = glm::quat(glm::vec3(ParentRotationRadians.x, ParentRotationRadians.y, ParentRotationRadians.z));
		glm::quat ChildQuat = glm::quat(glm::vec3(ChildRotationRadians.x, ChildRotationRadians.y, ChildRotationRadians.z));
		glm::quat CombinedQuat = ParentQuat * ChildQuat;
		glm::vec3 CombinedRotRad = glm::eulerAngles(CombinedQuat);
		ChildRenderInfo.mRotate = glm::degrees(CombinedRotRad);
	}

	void Scene::UpdateChildsBancEntity(BancEntityRenderInfo* RenderInfo)
	{
		RenderInfo->mEntity->mScale = RenderInfo->mScale / RenderInfo->mMergedActorParent->mScale;



		glm::vec3 ParentRotationRadians = glm::radians(RenderInfo->mMergedActorParent->mRotate);
		glm::quat ParentQuat = glm::quat(glm::vec3(ParentRotationRadians.x, ParentRotationRadians.y, ParentRotationRadians.z));
		glm::vec3 FinalRotationRadians = glm::radians(RenderInfo->mRotate);
		glm::quat FinalQuat = glm::quat(glm::vec3(FinalRotationRadians.x, FinalRotationRadians.y, FinalRotationRadians.z));
		glm::quat InverseParentQuat = glm::conjugate(ParentQuat);
		glm::quat ChildRawQuat = InverseParentQuat * FinalQuat;
		glm::vec3 ChildRawRotationRad = glm::eulerAngles(ChildRawQuat);
		RenderInfo->mEntity->mRotate = glm::degrees(ChildRawRotationRad);



		glm::mat4 ParentTransform = glm::mat4(1.0f);
		ParentTransform = glm::translate(ParentTransform, RenderInfo->mMergedActorParent->mTranslate);
		ParentTransform = glm::rotate(ParentTransform, ParentRotationRadians.z, glm::vec3(0.0f, 0.0f, 1.0f));
		ParentTransform = glm::rotate(ParentTransform, ParentRotationRadians.y, glm::vec3(0.0f, 1.0f, 0.0f));
		ParentTransform = glm::rotate(ParentTransform, ParentRotationRadians.x, glm::vec3(1.0f, 0.0f, 0.0f));

		glm::mat4 InverseParentTransform = glm::inverse(ParentTransform);

		glm::vec4 LocalPos = InverseParentTransform * glm::vec4(RenderInfo->mTranslate, 1.0f);
		RenderInfo->mEntity->mTranslate = glm::vec3(LocalPos);
	}

	void Scene::SyncBancEntityModel(BancEntityRenderInfo* RenderInfo)
	{
		application::game::ActorPack* ActorPack = application::manager::ActorPackMgr::GetActorPack(RenderInfo->mEntity->mGyml);
		if (ActorPack == nullptr)
		{
			application::file::game::bfres::BfresFile* File = &application::manager::BfresFileMgr::gBfresFiles["Default"];
			RenderInfo->mEntity->mBfresRenderer = application::manager::BfresRendererMgr::GetRenderer(File);
			return;
		}

		RenderInfo->mEntity->GenerateBancEntityInfo();
		GenerateDrawList();
	}

	void Scene::SyncBancEntity(BancEntityRenderInfo* RenderInfo)
	{
		bool RegenerateDrawList = false;
		if (RenderInfo->mEntity->mBancType == application::game::BancEntity::BancType::MERGED)
		{
			UpdateChildsBancEntity(RenderInfo);
			RenderInfo->mMergedActorParent->mMergedActorChildrenModified = true;

			//Updating other instances render info
			for (application::game::BancEntity& ChildEntity : *RenderInfo->mMergedActorParent->mMergedActorChildren)
			{
				for (BancEntityRenderInfo& ChildRenderInfo : mDrawList[ChildEntity.mBfresRenderer])
				{
					if (ChildRenderInfo.mEntity == &ChildEntity && ChildRenderInfo.mMergedActorParent != RenderInfo->mMergedActorParent)
					{
						UpdateChildsRenderInfo(ChildRenderInfo.mMergedActorParent, ChildRenderInfo);
					}
				}
			}
		}
		else
		{
			RenderInfo->mEntity->mTranslate = RenderInfo->mTranslate;
			RenderInfo->mEntity->mRotate = RenderInfo->mRotate;
			RenderInfo->mEntity->mScale = RenderInfo->mScale;

			//BancPath was deleted on a merged actor parent
			if ((!RenderInfo->mEntity->mDynamic.contains("BancPath") || !application::manager::MergedActorMgr::DoesMergedActorExist(std::get<std::string>(RenderInfo->mEntity->mDynamic["BancPath"]))) && RenderInfo->mEntity->mMergedActorChildren != nullptr)
			{
				RenderInfo->mEntity->mMergedActorChildren = nullptr;
				RegenerateDrawList = true;
			}
			//BancPath exists and BancPath is valid
			else if (RenderInfo->mEntity->mDynamic.contains("BancPath") && application::manager::MergedActorMgr::DoesMergedActorExist(std::get<std::string>(RenderInfo->mEntity->mDynamic["BancPath"])))
			{
				std::vector<application::game::BancEntity>* Children = application::manager::MergedActorMgr::GetMergedActor(std::get<std::string>(RenderInfo->mEntity->mDynamic["BancPath"]));
				if (RenderInfo->mEntity->mMergedActorChildren != Children)
				{
					RenderInfo->mEntity->mMergedActorChildren = Children;
					RegenerateDrawList = true;
				}
			}

			if (RenderInfo->mEntity->mMergedActorChildren != nullptr && !RegenerateDrawList) //BancEntity is a merged actor parent, probably a MergedActorUncond or smth
			{
				for (application::game::BancEntity& ChildEntity : *RenderInfo->mEntity->mMergedActorChildren)
				{
					for (BancEntityRenderInfo& ChildRenderInfo : mDrawList[ChildEntity.mBfresRenderer])
					{
						if (ChildRenderInfo.mEntity == &ChildEntity && ChildRenderInfo.mMergedActorParent == RenderInfo->mEntity)
						{
							UpdateChildsRenderInfo(RenderInfo->mEntity, ChildRenderInfo);
						}
					}
				}
			}
		}

		if (RegenerateDrawList)
			GenerateDrawList();
	}

	std::optional<application::game::BancEntity> Scene::CreateBancEntity(const std::string& Gyml)
	{
		application::game::BancEntity Entity;
		if (!Entity.FromGyml(Gyml))
		{
			application::util::Logger::Error("Scene", "Could not create BancEntity, probably wrong Gyml");
			return std::nullopt;
		}

		Entity.mBancType = application::game::BancEntity::BancType::STATIC;
		Entity.mHash = mStaticCompoundImplementation->RequestUniqueHash();
		Entity.mSRTHash = mStaticCompoundImplementation->RequestUniqueSRTHash();
		Entity.mCheckForHashGeneration = true;

		return Entity;
	}

	std::optional<application::game::BancEntity> Scene::CreateBancEntity(const application::game::BancEntity& Template)
	{
		application::game::BancEntity Entity = Template;

		Entity.mHash = mStaticCompoundImplementation->RequestUniqueHash();
		Entity.mSRTHash = mStaticCompoundImplementation->RequestUniqueSRTHash();
		Entity.mCheckForHashGeneration = true;

		mStaticCompoundImplementation->SyncBancEntityHashes(Entity, Template.mHash, Entity.mHash);

		return Entity;
	}

	void Scene::DeleteBancEntity(application::game::Scene::BancEntityRenderInfo* RenderInfo)
	{
		std::vector<application::game::BancEntity>* Entities = RenderInfo->mEntity->mBancType == application::game::BancEntity::BancType::MERGED ? RenderInfo->mMergedActorParent->mMergedActorChildren : &mBancEntities;
		
		if (Entities == nullptr)
		{
			application::util::Logger::Error("Scene", "Could not find BancEntity holder");
			return;
		}

		mStaticCompoundImplementation->OnBancEntityDelete(RenderInfo);
		if(RenderInfo->mMergedActorParent) RenderInfo->mMergedActorParent->mMergedActorChildrenModified = true;

		for (application::game::Scene::BancEntityRenderInfo* EntityInfo : mDrawListRenderInfoIndices)
		{
			if (EntityInfo == RenderInfo)
				continue;

			EntityInfo->mEntity->mLinks.erase(
				std::remove_if(EntityInfo->mEntity->mLinks.begin(), EntityInfo->mEntity->mLinks.end(),
					[RenderInfo](const application::game::BancEntity::Link& Link) { return Link.mDest == RenderInfo->mEntity->mHash || Link.mSrc == RenderInfo->mEntity->mHash; }),
				EntityInfo->mEntity->mLinks.end());
		}

		Entities->erase(
			std::remove_if(Entities->begin(), Entities->end(),
				[RenderInfo](const application::game::BancEntity& Entity) { return &Entity == RenderInfo->mEntity; }),
			Entities->end());

		GenerateDrawList();
	}

	Scene::BancEntityRenderInfo* Scene::AddBancEntity(const application::game::BancEntity& Entity)
	{
		mBancEntities.push_back(Entity);

		GenerateDrawList();

		application::game::BancEntity* EntityPtr = &mBancEntities.back();

		for (BancEntityRenderInfo& Info : mDrawList[Entity.mBfresRenderer])
		{
			if (Info.mEntity == EntityPtr)
			{
				return &Info;
			}
		}
		return nullptr;
	}

	Scene::BancEntityRenderInfo* Scene::AddBancEntity(const std::string& Gyml)
	{
		std::optional<application::game::BancEntity> Entity = CreateBancEntity(Gyml);

		if (!Entity.has_value())
			return nullptr;

		return AddBancEntity(Entity.value());
	}

	std::pair<std::vector<glm::vec3>, std::vector<uint32_t>> Scene::GetSceneModel(std::optional<std::function<bool(BancEntityRenderInfo*)>> Condition)
	{
		std::vector<glm::vec3> Vertices;
		std::vector<uint32_t> Indices;

		for (application::game::Scene::BancEntityRenderInfo* RenderInfo : mDrawListRenderInfoIndices)
		{
			if (RenderInfo->mEntity->mBfresRenderer == nullptr || RenderInfo->mEntity->mBfresRenderer->mBfresFile->mDefaultModel)
				continue;

			if (Condition.has_value())
			{
				if (!Condition.value()(RenderInfo))
				{
					continue;
				}
			}

			glm::mat4 GLMModel = glm::mat4(1.0f); // Identity matrix

			GLMModel = glm::translate(GLMModel, RenderInfo->mTranslate);

			GLMModel = glm::rotate(GLMModel, glm::radians(RenderInfo->mRotate.z), glm::vec3(0.0, 0.0f, 1.0));
			GLMModel = glm::rotate(GLMModel, glm::radians(RenderInfo->mRotate.y), glm::vec3(0.0f, 1.0, 0.0));
			GLMModel = glm::rotate(GLMModel, glm::radians(RenderInfo->mRotate.x), glm::vec3(1.0, 0.0f, 0.0));

			GLMModel = glm::scale(GLMModel, RenderInfo->mScale);

			for (auto& [Key, Val] : RenderInfo->mEntity->mBfresRenderer->mBfresFile->Models.GetByIndex(0).mValue.Shapes.mNodes)
			{
				uint32_t IndexOffset = Vertices.size();

				for (const glm::vec4& Pos : Val.mValue.Vertices)
				{
					glm::vec4 Vertex(Pos.x, Pos.y, Pos.z, 1.0f);
					Vertex = GLMModel * Vertex;
					Vertices.emplace_back(Vertex.x, Vertex.y, Vertex.z);
				}
				for (uint32_t Index : Val.mValue.Meshes[0].GetIndices())
				{
					Indices.push_back(Index + IndexOffset);
				}
			}
		}

		return std::make_pair(Vertices, Indices);
	}

	void Scene::GenerateDrawList()
	{
		mDrawList.clear();
		mDrawListIndices.clear();
		mDrawListRenderInfoIndices.clear();

		uint32_t EntityIndex = 0;

		for (application::game::BancEntity& Entity : mBancEntities)
		{
			if (Entity.mBfresRenderer == nullptr || Entity.mBfresRenderer->mIsSystemModelTransparent)
				continue;

			if (Entity.mDynamic.contains("BancPath"))
			{
				Entity.mMergedActorChildren = application::manager::MergedActorMgr::GetMergedActor(std::get<std::string>(Entity.mDynamic["BancPath"]));
			}

			BancEntityRenderInfo RenderInfo;
			RenderInfo.mEntity = &Entity;
			RenderInfo.mEntityIndex = EntityIndex++;
			RenderInfo.mTranslate = Entity.mTranslate;
			RenderInfo.mRotate = Entity.mRotate;
			RenderInfo.mScale = Entity.mScale;

			mDrawList[Entity.mBfresRenderer].push_back(RenderInfo);
			mDrawListIndices.push_back(&Entity);

			if (Entity.mMergedActorChildren == nullptr)
				continue;

			//Loading merged actor children
			for (application::game::BancEntity& ChildEntity : *Entity.mMergedActorChildren)
			{
				BancEntityRenderInfo ChildRenderInfo;
				ChildRenderInfo.mEntity = &ChildEntity;
				ChildRenderInfo.mEntityIndex = EntityIndex++;
				ChildRenderInfo.mTranslate = Entity.mTranslate + ChildEntity.mTranslate;
				ChildRenderInfo.mRotate = ChildEntity.mRotate;
				ChildRenderInfo.mScale = Entity.mScale * ChildEntity.mScale;
				ChildRenderInfo.mMergedActorParent = &Entity;

				UpdateChildsRenderInfo(&Entity, ChildRenderInfo);

				mDrawList[ChildEntity.mBfresRenderer].push_back(ChildRenderInfo);
				mDrawListIndices.push_back(&ChildEntity);
			}
		}

		for (application::game::BancEntity& Entity : mBancEntities)
		{
			if (Entity.mBfresRenderer == nullptr || !Entity.mBfresRenderer->mIsSystemModelTransparent)
				continue;

			BancEntityRenderInfo RenderInfo;
			RenderInfo.mEntity = &Entity;
			RenderInfo.mEntityIndex = EntityIndex++;
			RenderInfo.mTranslate = Entity.mTranslate;
			RenderInfo.mRotate = Entity.mRotate;
			RenderInfo.mScale = Entity.mScale;

			mDrawList[Entity.mBfresRenderer].push_back(RenderInfo);
			mDrawListIndices.push_back(&Entity);
		}

		mDrawListRenderInfoIndices.resize(EntityIndex);
		for (auto& [Renderer, Entities] : mDrawList)
		{
			for (BancEntityRenderInfo& RenderInfo : Entities)
			{
				mDrawListRenderInfoIndices[RenderInfo.mEntityIndex] = &RenderInfo;
			}
		}
	}

	std::string Scene::GetSceneTypeName(Scene::SceneType Type)
	{
		switch (Type)
		{
		case SceneType::SMALL_DUNGEON:
			return "SmallDungeon";
		case SceneType::LARGA_DUNGEON:
			if(mBcettName == "LargeDungeonFire")
				return "MinusField/LargeDungeon";
			else
				return "MainField/LargeDungeon";
		case SceneType::MAIN_FIELD:
			if (mBcettName.length() == 3)
				return "MainField";
			else
				return "MainField/Castle";
		case SceneType::MINUS_FIELD:
			return "MinusField";
		case SceneType::TRIAL_FIELD:
			return "TrialField";
		case SceneType::NORMAL_STAGE:
			return "NormalStage";
		case SceneType::SKY:
			return "MainField/Sky";
		default:
			application::util::Logger::Error("Scene", "Tried to get name for invalid scene type");
			return "NONE";
		}
	}

	void Scene::Save(const std::string& Directory)
	{
		if (mSceneType == SceneType::NONE)
		{
			application::util::Logger::Error("Scene", "Tried to save game scene without initializing it");
			return;
		}

		auto SavingStartTime = std::chrono::system_clock::now();

		//Saving terrain
		if(mTerrainRenderer) mTerrainRenderer->Save(static_cast<application::tool::scene::static_compound::StaticCompoundImplementationFieldScene*>(mStaticCompoundImplementation.get()));

		//GenerateHashes();
		mStaticCompoundImplementation->Save();

		bool Abort = false;

		application::file::game::byml::BymlFile::Node StaticActorsNode(application::file::game::byml::BymlFile::Type::Array, "Actors");
		application::file::game::byml::BymlFile::Node DynamicActorsNode(application::file::game::byml::BymlFile::Type::Array, "Actors");

		application::file::game::byml::BymlFile::Node AiGroupsNode(application::file::game::byml::BymlFile::Type::Array, "AiGroups");

		std::vector<std::string> UniqueBancPaths;

		for (application::game::BancEntity& Entity : mBancEntities)
		{
			application::file::game::byml::BymlFile::Node EntityNode = Entity.ToByml();

			if (Entity.mMergedActorChildren != nullptr && Entity.mMergedActorChildrenModified) //BancEntity is a MergedActor parent
			{
				if (Entity.mDynamic.contains("BancPath"))
				{
					std::string Path = std::get<std::string>(Entity.mDynamic["BancPath"]);

					if (std::find(UniqueBancPaths.begin(), UniqueBancPaths.end(), Path) == UniqueBancPaths.end())
						UniqueBancPaths.push_back(Path);
				}
			}

			if (Entity.mBancType == application::game::BancEntity::BancType::STATIC)
			{
				StaticActorsNode.AddChild(EntityNode);
			}
			else if (Entity.mBancType == application::game::BancEntity::BancType::DYNAMIC)
			{
				DynamicActorsNode.AddChild(EntityNode);
			}
			else
			{
				application::util::Logger::Error("Scene", "BancEntity with hash %lu at %.3f %.3f %.3f has an invalid banc type", Entity.mHash, Entity.mTranslate.x, Entity.mTranslate.y, Entity.mTranslate.z);
				Abort = true;
				break;
			}
		}

		for (application::game::Scene::AiGroup& AiGroup : mAiGroups)
		{
			application::file::game::byml::BymlFile::Node AiGroupRoot(application::file::game::byml::BymlFile::Type::Dictionary);

			//Blackboards
			{
				if(!AiGroup.mBlackboards.empty())
				{
					application::file::game::byml::BymlFile::Node BlackboardsRoot(application::file::game::byml::BymlFile::Type::Array, "Blackboards");
					for (const std::string& BB : AiGroup.mBlackboards)
					{
						application::file::game::byml::BymlFile::Node Node(application::file::game::byml::BymlFile::Type::StringIndex);
						Node.SetValue<std::string>(BB);
						BlackboardsRoot.AddChild(Node);
					}
					AiGroupRoot.AddChild(BlackboardsRoot);
				}
			}

			//Hash
			{
				application::file::game::byml::BymlFile::Node Node(application::file::game::byml::BymlFile::Type::UInt64, "Hash");
				Node.SetValue<uint64_t>(AiGroup.mHash);
				AiGroupRoot.AddChild(Node);
			}

			//InstanceName
			{
				if (AiGroup.mInstanceName.has_value())
				{
					application::file::game::byml::BymlFile::Node Node(application::file::game::byml::BymlFile::Type::StringIndex, "InstanceName");
					Node.SetValue<std::string>(AiGroup.mInstanceName.value());
					AiGroupRoot.AddChild(Node);
				}
			}

			//Logic/Meta Path
			{
				application::file::game::byml::BymlFile::Node Node(application::file::game::byml::BymlFile::Type::StringIndex, AiGroup.mType == application::game::Scene::AiGroup::Type::LOGIC ? "Logic" : "Meta");
				Node.SetValue<std::string>(AiGroup.mPath);
				AiGroupRoot.AddChild(Node);
			}

			//References
			{
				application::file::game::byml::BymlFile::Node ReferencesRoot(application::file::game::byml::BymlFile::Type::Array, "References");

				for (application::game::Scene::AiGroup::Reference& Ref : AiGroup.mReferences)
				{
					application::file::game::byml::BymlFile::Node ReferenceNode(application::file::game::byml::BymlFile::Type::Dictionary);
					//Id
					{
						application::file::game::byml::BymlFile::Node Node(application::file::game::byml::BymlFile::Type::StringIndex, "Id");
						Node.SetValue<std::string>(Ref.mId);
						ReferenceNode.AddChild(Node);
					}

					//Path
					if(!Ref.mPath.empty())
					{
						application::file::game::byml::BymlFile::Node Node(application::file::game::byml::BymlFile::Type::StringIndex, "Path");
						Node.SetValue<std::string>(Ref.mPath);
						ReferenceNode.AddChild(Node);
					}

					//Reference (BancEntity->mHash)
					if(Ref.mReference > 0) 
					{
						application::file::game::byml::BymlFile::Node Node(application::file::game::byml::BymlFile::Type::UInt64, "Reference");
						Node.SetValue<uint64_t>(Ref.mReference);
						ReferenceNode.AddChild(Node);
					}

					//InstanceName
					if (Ref.mInstanceName.has_value())
					{
						application::file::game::byml::BymlFile::Node Node(application::file::game::byml::BymlFile::Type::StringIndex, "InstanceName");
						Node.SetValue<std::string>(Ref.mInstanceName.value());
						ReferenceNode.AddChild(Node);
					}

					//Logic
					if (Ref.mLogic.has_value())
					{
						application::file::game::byml::BymlFile::Node Node(application::file::game::byml::BymlFile::Type::StringIndex, "Logic");
						Node.SetValue<std::string>(Ref.mLogic.value());
						ReferenceNode.AddChild(Node);
					}

					if (Ref.mMeta.has_value())
					{
						application::file::game::byml::BymlFile::Node Node(application::file::game::byml::BymlFile::Type::StringIndex, "Meta");
						Node.SetValue<std::string>(Ref.mMeta.value());
						ReferenceNode.AddChild(Node);
					}

					ReferencesRoot.AddChild(ReferenceNode);
				}

				AiGroupRoot.AddChild(ReferencesRoot);
			}

			AiGroupsNode.AddChild(AiGroupRoot);
		}

		if (Abort)
		{
			application::util::Logger::Error("Scene", "Aborting due to previous error(s)");
			return;
		}

		if (mStaticBymlFile.HasChild("Actors"))
		{
			mStaticBymlFile.GetNode("Actors")->mChildren = StaticActorsNode.mChildren;
		}
		else if (!StaticActorsNode.mChildren.empty())
		{
			mStaticBymlFile.GetNodes().push_back(StaticActorsNode);
		}

		if (mDynamicBymlFile.HasChild("Actors"))
		{
			mDynamicBymlFile.GetNode("Actors")->mChildren = DynamicActorsNode.mChildren;
		}
		else if (!DynamicActorsNode.mChildren.empty())
		{
			mDynamicBymlFile.GetNodes().push_back(DynamicActorsNode);
		}

		if (mStaticBymlFile.HasChild("AiGroups"))
		{
			mStaticBymlFile.GetNode("AiGroups")->mChildren = AiGroupsNode.mChildren;
		}
		else if (!AiGroupsNode.mChildren.empty())
		{
			mStaticBymlFile.GetNodes().push_back(AiGroupsNode);
		}

		const std::string BancBasePath = Directory + "/Banc/" + GetSceneTypeName(mSceneType);
		std::filesystem::create_directories(BancBasePath);
		application::util::FileUtil::WriteFile(BancBasePath + "/" + mBcettName + "_Static.bcett.byml.zs", application::file::game::ZStdBackend::Compress(mStaticBymlFile.ToBinary(application::file::game::byml::BymlFile::TableGeneration::AUTO), application::file::game::ZStdBackend::Dictionary::BcettByaml));
		application::util::FileUtil::WriteFile(BancBasePath + "/" + mBcettName + "_Dynamic.bcett.byml.zs", application::file::game::ZStdBackend::Compress(mDynamicBymlFile.ToBinary(application::file::game::byml::BymlFile::TableGeneration::AUTO), application::file::game::ZStdBackend::Dictionary::BcettByaml));
		
		for (const std::string& BancPath : UniqueBancPaths)
		{
			application::manager::MergedActorMgr::WriteMergedActor(BancPath);
		}

		application::manager::GameDataListMgr::RebuildAndSave(*this);

		auto SavingEndTime = std::chrono::system_clock::now();
		std::chrono::duration<float> ElapsedSeconds = SavingEndTime - SavingStartTime;
		std::time_t EndTime = std::chrono::system_clock::to_time_t(SavingEndTime);

		char Buffer[100];
		std::tm* TmInfo = std::localtime(&EndTime);
		std::strftime(Buffer, sizeof(Buffer), "%Y-%m-%d %H:%M:%S", TmInfo);

		application::util::Logger::Info("Scene", "%s saved at [%s], took %f seconds", mBcettName.c_str(), Buffer, ElapsedSeconds.count());
	}
}