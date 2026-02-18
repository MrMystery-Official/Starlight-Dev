#pragma once

#include <rendering/UIWindowBase.h>
#include <file/game/bfres/BfresFile.h>
#include <file/game/phive/shape/PhiveShape.h>
#include <gl/BfresRenderer.h>
#include <gl/Framebuffer.h>
#include <gl/Shader.h>
#include <gl/Camera.h>
#include <gl/SimpleMesh.h>
#include <game/ActorPack.h>
#include <variant>
#include <optional>
#include "imgui.h"

namespace application::rendering::collision
{
	class UICollisionGenerator : public UIWindowBase
	{
	public:
		static application::gl::Shader* gInstancedShader;
		static application::gl::Shader* gNavMeshShader;
		static application::gl::Shader* gSelectedShader;

		UICollisionGenerator() = default;

		void DrawShapesWindow();
		void DrawMaterialsWindow();
		void DrawModelViewWindow();
		void DrawDetailsWindow();
		void DrawOperationsWindow();

		void InitializeImpl() override;
		void DrawImpl() override;
		void DeleteImpl() override;
		WindowType GetWindowType() override;

		std::vector<application::gl::SimpleMesh> mDebugShapes;

	private:
		enum class GenerationTarget : uint8_t
		{
			NONE = 0,
			BFRES_FILE = 1,
			ACTOR_PACK = 2 //When an actor is loaded instead of a bfres, the tool does not only generate the collision but also modifies the actor pack
		};

		struct ShapeEntry
		{
			application::file::game::bfres::BfresFile::Shape* mShape = nullptr;

			bool mEnabled = true;
			int32_t mMaterialIndex = -1;
		};

		struct MaterialEntry
		{
			std::string mName = "";
			application::file::game::phive::util::PhiveMaterialData::PhiveMaterial mPhiveMaterial;
		};

		enum class DetailsEditorContentType : uint8_t
		{
			NONE = 0,
			SHAPE = 1,
			MATERIAL = 2
		};

		struct DetailsEditorContentShape
		{
			ShapeEntry* mShape = nullptr;
		};

		struct DetailsEditorContentMaterial
		{
			MaterialEntry* mMaterial = nullptr;
		};

		struct DetailsEditorContent
		{
			DetailsEditorContentType mType = DetailsEditorContentType::NONE;
			std::variant<int, DetailsEditorContentShape, DetailsEditorContentMaterial> mContent = 0;
		};

		struct CollisionStats
		{
			int32_t mModelVertexCount = -1;
			int32_t mModelFaceCount = -1;

			int32_t mCollisionVertexCount = -1;
			int32_t mCollisionPrimitiveCount = -1;
			int32_t mCollisionBVHCount = -1;
			int32_t mCollisionGeometrySectionCount = -1;
		};

		std::string GetWindowTitle();
		void LoadBfres(const std::string& Path);
		void LoadActorPack(const std::string& Gyml);
		void GenerateBphsh();
		void GenerateActorPack();
		void GenerateCollisionStats();

		GenerationTarget mGenerationTarget = GenerationTarget::NONE;

		application::file::game::bfres::BfresFile* mBfresFile = nullptr;
		application::gl::BfresRenderer* mBfresRenderer = nullptr;
		application::game::ActorPack* mActorPack = nullptr;

		const glm::vec4 mClearColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
		ImVec2 mSceneWindowSize;
		application::gl::Framebuffer* mFramebuffer = nullptr;

		application::gl::Camera mCamera = application::gl::Camera(0, 0, glm::vec3(0.0f, 0.0f, 0.0f), nullptr);

		std::vector<ShapeEntry> mShapes;
		std::vector<MaterialEntry> mMaterials;

		DetailsEditorContent mDetailsEditorContent;

		std::optional<application::gl::SimpleMesh> mCollisionMesh;

		application::file::game::phive::shape::PhiveShape::GeneratorData mGenerator;
		application::file::game::phive::shape::PhiveShape mPhiveShape;
		std::string mPhiveShapeName = "";

		CollisionStats mCollisionStats;

		bool mRenderModel = true;
		bool mRenderCollisionPreview = true;
		bool mRenderModelWireframe = false;
		bool mRenderCollisionPreviewWireframe = false;
	};
}