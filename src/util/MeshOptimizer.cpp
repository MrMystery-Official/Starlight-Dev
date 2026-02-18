#include <util/MeshOptimizer.h>

#include <OpenMesh/Tools/Decimater/DecimaterT.hh>
#include <OpenMesh/Tools/Decimater/ModQuadricT.hh>

namespace application::util
{
	MeshOptimizer::MeshOptimizer(const std::vector<glm::vec3>& Vertices, const std::vector<uint32_t>& Indices)
	{
        for (const auto& Vertex : Vertices) 
        {
            mOpenMesh.add_vertex(OpenMesh::Vec3f(Vertex.x, Vertex.y, Vertex.z));
        }

        for (size_t i = 0; i < Indices.size(); i += 3) 
        {
            std::vector<Mesh::VertexHandle> FaceVHandles;
            FaceVHandles.push_back(Mesh::VertexHandle(Indices[i]));
            FaceVHandles.push_back(Mesh::VertexHandle(Indices[i + 1]));
            FaceVHandles.push_back(Mesh::VertexHandle(Indices[i + 2]));
            mOpenMesh.add_face(FaceVHandles);
        }
	}

	void MeshOptimizer::Optimize(float TargetPercentage)
	{
        // Create decimater with quadric error metric
        OpenMesh::Decimater::DecimaterT<Mesh> Decimater(mOpenMesh);
        OpenMesh::Decimater::ModQuadricT<Mesh>::Handle ModQuadric;

        // Add quadric module to decimater
        Decimater.add(ModQuadric);

        // Set maximum error tolerance and target reduction
        Decimater.module(ModQuadric).set_max_err(std::numeric_limits<double>::max());

        // Calculate target number of faces
        int TargetFaceCount = mOpenMesh.n_faces() * TargetPercentage;

        // Initialize decimation
        Decimater.initialize();

        // Perform decimation
        Decimater.decimate_to_faces(TargetFaceCount);
	}

	void MeshOptimizer::GetOptimizedMesh(std::vector<glm::vec3>& OutVertices, std::vector<uint32_t>& OutIndices)
	{
        OutVertices.clear();
        OutIndices.clear();

        for (auto vh : mOpenMesh.vertices()) 
        {
            OpenMesh::Vec3f Point = mOpenMesh.point(vh);
            OutVertices.emplace_back(Point[0], Point[1], Point[2]);
        }

        for (auto fh : mOpenMesh.faces()) 
        {
            for (auto fv : mOpenMesh.fv_range(fh))
            {
                OutIndices.push_back(fv.idx());
            }
        }
	}
}