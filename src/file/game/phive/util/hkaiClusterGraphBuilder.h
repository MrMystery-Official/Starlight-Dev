#pragma once

#include <file/game/phive/PhiveNavMesh.h>
#include <vector>
#include <glm/vec4.hpp>

namespace application::file::game::phive::util
{
	namespace hkaiClusterGraphBuilder
	{
		application::file::game::phive::PhiveNavMesh::hkaiClusterGraph Build(application::file::game::phive::PhiveNavMesh::hkaiNavMesh& NavMesh, const application::file::game::phive::util::NavMeshGenerator::PhiveNavMeshGeneratorConfig& Config);
		std::vector<std::pair<int, std::vector<int>>> KMeansCluster(std::vector<glm::vec4> Points, int k);
	};
}