#pragma once

#include <game/Scene.h>
#include <tool/scene/NavMeshImplementationBase.h>
#include <file/game/phive/navmesh/PhiveNavMesh.h>
#include <optional>
#include <gl/SimpleMesh.h>
#include <gl/Shader.h>

namespace application::tool::scene::navmesh
{
	class NavMeshImplementationSingleScene : public application::tool::scene::NavMeshImplementationBase
	{
	public:
		virtual bool Initialize() override;
		virtual bool SupportNavMeshGeneration() override;
		virtual bool SupportNavMeshViewing() override;
		virtual void DrawNavMesh(application::gl::Camera& Camera) override;
		virtual void DrawImGuiGeneratorMenu() override;

		NavMeshImplementationSingleScene(application::game::Scene* Scene);
		virtual ~NavMeshImplementationSingleScene() override;
	private:
		application::game::Scene* mScene;
		application::gl::Shader* mShader = nullptr;

		std::optional<application::file::game::phive::navmesh::PhiveNavMesh> mNavMesh = std::nullopt;
		std::optional<application::gl::SimpleMesh> mMesh = std::nullopt;
		std::optional<application::gl::SimpleMesh> mClusterGraphMesh = std::nullopt;

		application::file::game::phive::navmesh::PhiveNavMesh::GeneratorData mGeneratorData;
	};
}