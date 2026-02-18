#pragma once

#include <game/Scene.h>
#include <tool/scene/NavMeshImplementationBase.h>
#include <file/game/phive/navmesh/PhiveNavMesh.h>
#include <optional>
#include <gl/SimpleMesh.h>
#include <gl/Shader.h>

namespace application::tool::scene::navmesh
{
	class NavMeshImplementationFieldScene : public application::tool::scene::NavMeshImplementationBase
	{
	public:
		struct FieldNavMeshSectionInfo
		{
			int mXIndex = 0;
			int mZIndex = 0;

			std::string GetSectionName() const
			{
				return "X" + std::to_string(mXIndex) + "_Z" + std::to_string(mZIndex);
			}

			glm::vec2 GetSectionOrigin() const
			{
				return glm::vec2((mXIndex - 20) * 250, (mZIndex - 16) * 250);
			}

			FieldNavMeshSectionInfo(float x, float z) : mXIndex(std::floor(x / 250) + 20), mZIndex(std::floor(z / 250) + 16) {}
			FieldNavMeshSectionInfo(int xIndex, int zIndex) : mXIndex(xIndex), mZIndex(zIndex) {}
		};

		virtual bool Initialize() override;
		virtual bool SupportNavMeshGeneration() override;
		virtual bool SupportNavMeshViewing() override;
		virtual void DrawNavMesh(application::gl::Camera& Camera) override;
		virtual void DrawImGuiGeneratorMenu() override;

		void ReloadMeshes();

		NavMeshImplementationFieldScene(application::game::Scene* Scene);
		virtual ~NavMeshImplementationFieldScene() override;
	private:
		glm::vec3 GetNavMeshMiddlePoint(uint8_t X, uint8_t Z);
		void RemoveDuplicateVertices(application::file::game::phive::navmesh::PhiveNavMesh::GeneratorData& GeneratorData);

		uint32_t mGenerationNavMeshIndex = 0;

		application::game::Scene* mScene;
		application::gl::Shader* mShader = nullptr;

		std::vector<std::pair<FieldNavMeshSectionInfo, application::file::game::phive::navmesh::PhiveNavMesh>> mNavMeshes;
		std::vector<application::gl::SimpleMesh> mMeshes;

		application::file::game::phive::navmesh::PhiveNavMesh::GeneratorData mGenerator;
		bool mMergingMode = true;
		bool mIncludeTerrainGeometry = false;
	};
}