#pragma once

#include <file/game/phive/starlight_physics/common/hkGeometry.h>
#include <file/game/phive/starlight_physics/ai/hkaiNavMeshGeometryGenerator.h>
#include <file/game/phive/classes/HavokClasses.h>

#include <cstdint>
#include <vector>

namespace application::file::game::phive::starlight_physics::ai
{
	class hkaiNavMeshBuilder
	{
	public:
		void initialize(const hkaiNavMeshGeometryGenerator::Config& config);
		bool buildNavMesh(classes::HavokClasses::hkaiNavMesh* dst, std::vector<glm::vec3>& vertices, std::vector<uint32_t>& indices);
		void buildClusterGraph(const classes::HavokClasses::hkaiNavMesh& mesh, classes::HavokClasses::hkaiClusterGraph& graphOut, uint32_t desiredFaces = 20);

		application::file::game::phive::starlight_physics::ai::hkaiNavMeshGeometryGenerator geometryGenerator;
	};
}