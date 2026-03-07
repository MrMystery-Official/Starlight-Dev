#pragma once

#include <cstdint>

#include <glm/vec3.hpp>

namespace application::file::game::phive::navmesh
{
	class PhiveNavMesh;

	class PhiveNavMeshStreaming
	{
	public:
		struct BuildResult
		{
			uint32_t mMeshConnectionCount = 0;
			uint32_t mGraphConnectionCount = 0;
		};

		static BuildResult RebuildPair(PhiveNavMesh& NavMeshA, PhiveNavMesh& NavMeshB, const glm::vec3& NavMeshBOffsetFromA, float EdgeMatchTolerance = 0.05f, uint32_t ClusteringSetupIdentifier = 0);
	};
}
