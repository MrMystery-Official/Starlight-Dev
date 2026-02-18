#pragma once

#include <vector>
#include <glm/vec3.hpp>

#include <OpenMesh/Core/IO/MeshIO.hh>
#include <OpenMesh/Core/Mesh/TriMesh_ArrayKernelT.hh>

typedef OpenMesh::TriMesh_ArrayKernelT<> Mesh;

namespace application::util
{
	struct MeshOptimizer
	{
		void Optimize(float TargetPercentage);
		void GetOptimizedMesh(std::vector<glm::vec3>& OutVertices, std::vector<uint32_t>& OutIndices);
		
		MeshOptimizer(const std::vector<glm::vec3>& Vertices, const std::vector<uint32_t>& Indices);

		Mesh mOpenMesh;
	};
}