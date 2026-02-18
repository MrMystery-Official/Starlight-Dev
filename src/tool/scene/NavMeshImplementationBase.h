#pragma once

#include <gl/Shader.h>
#include <gl/Camera.h>

namespace application::tool::scene
{
	class NavMeshImplementationBase
	{
	public:
		virtual bool Initialize();
		virtual bool SupportNavMeshGeneration();
		virtual bool SupportNavMeshViewing();
		virtual void DrawNavMesh(application::gl::Camera& Camera);
		virtual void DrawImGuiGeneratorMenu();

		virtual ~NavMeshImplementationBase() = default;
	};
}