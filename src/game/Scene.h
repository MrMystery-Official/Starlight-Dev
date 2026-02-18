#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <game/BancEntity.h>
#include <file/game/byml/BymlFile.h>
#include <file/game/phive/PhiveStaticCompoundFile.h>
#include <tool/scene/NavMeshImplementationBase.h>
#include <tool/scene/StaticCompoundImplementationBase.h>
#include <gl/CaveRenderer.h>
#include <gl/SimpleMesh.h>
#include <map>
#include <manager/TerrainMgr.h>
#include <memory>

namespace application::game
{
	class Scene
	{
	public:
		enum class SceneType : uint8_t
		{
			NONE = 0,
			SMALL_DUNGEON = 1,
			LARGA_DUNGEON = 2,
			MAIN_FIELD = 3,
			MINUS_FIELD = 4,
			SKY = 5,
			NORMAL_STAGE = 6,
			TRIAL_FIELD = 7
		};

		struct BancEntityRenderInfo
		{
			application::game::BancEntity* mEntity = nullptr;
			application::game::BancEntity* mMergedActorParent = nullptr;
			uint32_t mEntityIndex = 0;
			float mSphereScreenSize = 0.0f;
			glm::vec3 mTranslate = glm::vec3(0.0f, 0.0f, 0.0f);
			glm::vec3 mRotate = glm::vec3(0.0f, 0.0f, 0.0f);
			glm::vec3 mScale = glm::vec3(1.0f, 1.0f, 1.0f);
		};

		struct AiGroup
		{
			enum class Type : uint32_t
			{
				LOGIC = 0,
				META = 1
			};

			struct Reference
			{
				std::string mId = "";
				std::string mPath = "";
				uint64_t mReference = 0;

				std::optional<std::string> mInstanceName = std::nullopt;
				std::optional<std::string> mLogic = std::nullopt;
				std::optional<std::string> mMeta = std::nullopt;
			};

			std::vector<std::string> mBlackboards;
			std::optional<std::string> mInstanceName = std::nullopt;
			uint64_t mHash = 0;
			Type mType = Type::LOGIC;
			std::string mPath = "";
			std::vector<Reference> mReferences;
		};

		Scene() = default;

		void Load(SceneType Type, const std::string& Identifier, float TerrainLOD = 32.0f);
		void Load(SceneType Type, const std::string& StaticBymlPath, const std::string& DynamicBymlPath, float TerrainLOD);
		void _LoadInplace(SceneType Type, const std::string& StaticBymlPath, const std::string& DynamicBymlPath);

		void Save(const std::string& Directory);

		void SyncBancEntity(BancEntityRenderInfo* RenderInfo);
		void SyncBancEntityModel(BancEntityRenderInfo* RenderInfo);
		BancEntityRenderInfo* AddBancEntity(const std::string& Gyml);
		BancEntityRenderInfo* AddBancEntity(const application::game::BancEntity& Entity);
		std::optional<application::game::BancEntity> CreateBancEntity(const std::string& Gyml);
		std::optional<application::game::BancEntity> CreateBancEntity(const application::game::BancEntity& Template);
		void DeleteBancEntity(application::game::Scene::BancEntityRenderInfo* RenderInfo);

		std::pair<std::vector<glm::vec3>, std::vector<uint32_t>> GetSceneModel(std::optional<std::function<bool(BancEntityRenderInfo*)>> Condition = std::nullopt);
		std::string GetSceneTypeName(SceneType Type);

		bool IsLoaded();
		bool IsTerrainScene();
		void GenerateDrawList();

		void Import(Scene& other);

		SceneType mSceneType = SceneType::NONE;
		std::string mBcettName;
		std::vector<application::game::BancEntity> mBancEntities;
		std::vector<AiGroup> mAiGroups;

		std::map<application::gl::BfresRenderer*, std::vector<BancEntityRenderInfo>> mDrawList;
		std::vector<application::game::BancEntity*> mDrawListIndices;
		std::vector<BancEntityRenderInfo*> mDrawListRenderInfoIndices;

		std::unique_ptr<application::tool::scene::NavMeshImplementationBase> mNavMeshImplementation;
		std::unique_ptr<application::tool::scene::StaticCompoundImplementationBase> mStaticCompoundImplementation;

		std::string mGameBancParamPath = "";
		std::optional<application::file::game::byml::BymlFile> mGameBancParam = std::nullopt;

		application::gl::TerrainRenderer* mTerrainRenderer = nullptr;

		std::vector<application::gl::SimpleMesh> mDebugMeshes;

		bool mGeneratePhysicsHashes = true;
	private:
		void UpdateChildsRenderInfo(application::game::BancEntity* Parent, BancEntityRenderInfo& ChildRenderInfo);
		void UpdateChildsBancEntity(BancEntityRenderInfo* RenderInfo);

		application::file::game::byml::BymlFile mStaticBymlFile;
		application::file::game::byml::BymlFile mDynamicBymlFile;
	};
}