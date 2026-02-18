#include "NavMeshImplementationBase.h"

namespace application::tool::scene
{
	bool NavMeshImplementationBase::Initialize()
	{
		return true;
	}

	bool NavMeshImplementationBase::SupportNavMeshGeneration()
	{
		return false;
	}

	bool NavMeshImplementationBase::SupportNavMeshViewing()
	{
		return false;
	}

	void NavMeshImplementationBase::DrawNavMesh(application::gl::Camera& Camera)
	{

	}

	void NavMeshImplementationBase::DrawImGuiGeneratorMenu()
	{

	}
}