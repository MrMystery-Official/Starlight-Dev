#include "hkaiClusterGraphBuilder.h"

#include <random>
#include <util/delaunator.h>
#include <util/Math.h>
#include <unordered_map>

namespace application::file::game::phive::util
{
    struct IndexedPoint
    {
        float mX = 0.0f;
        float mY = 0.0f;
        int mIndex = 0;
    };

	application::file::game::phive::PhiveNavMesh::hkaiClusterGraph hkaiClusterGraphBuilder::Build(application::file::game::phive::PhiveNavMesh::hkaiNavMesh& NavMesh, const application::file::game::phive::util::NavMeshGenerator::PhiveNavMeshGeneratorConfig& Config)
	{
		application::file::game::phive::PhiveNavMesh::hkaiClusterGraph Graph;
		Graph.m_positions.reserve(NavMesh.m_faces.size());
		Graph.m_nodes.reserve(NavMesh.m_faces.size());
		Graph.m_edges.reserve(NavMesh.m_faces.size());
		Graph.m_edgeData.clear();
		Graph.m_nodeData.clear();
		Graph.m_edgeDataStriding = 0;
		Graph.m_nodeDataStriding = 0;
		Graph.m_featureToNodeIndex.clear();
		Graph.m_userData.m_primitiveBase = 0;

		application::file::game::phive::PhiveNavMesh::hkaiAnnotatedStreamingSet StreamingSet;
		StreamingSet.m_side.m_storage.m_primitiveBase = 0;
		StreamingSet.m_streamingSet.m_ptr.m_aSectionUid.m_primitiveBase = 0;
		StreamingSet.m_streamingSet.m_ptr.m_bSectionUid.m_primitiveBase = 0;
		StreamingSet.m_streamingSet.m_ptr.m_aConnectionAabbs.clear();
		StreamingSet.m_streamingSet.m_ptr.m_bConnectionAabbs.clear();
		StreamingSet.m_streamingSet.m_ptr.m_graphConnections.mParent.m_items.clear();
		StreamingSet.m_streamingSet.m_ptr.m_meshConnections.clear();
		StreamingSet.m_streamingSet.m_ptr.m_volumeConnections.clear();
		
        Graph.m_streamingSets.clear();

		std::vector<glm::vec4> Positions;
		Positions.resize(NavMesh.m_faces.size());

		// Set positions to center of all polygons
		for (size_t i = 0; i < NavMesh.m_faces.size(); i++)
		{
			application::file::game::phive::PhiveNavMesh::hkaiNavMesh__Face& Face = NavMesh.m_faces[i];
			for (int j = Face.m_startEdgeIndex.m_primitiveBase; j < Face.m_numEdges.m_primitiveBase + Face.m_startEdgeIndex.m_primitiveBase; j++)
			{
				application::file::game::phive::PhiveNavMesh::hkaiNavMesh__Edge& edge = NavMesh.m_edges[j];
				application::file::game::phive::PhiveNavMesh::hkVector4 Vector = NavMesh.m_vertices[edge.m_a.m_primitiveBase];
				Positions[i] += glm::vec4(Vector.m_subTypeArray[0], Vector.m_subTypeArray[1], Vector.m_subTypeArray[2], Vector.m_subTypeArray[3]);
			}
			Positions[i] /= Face.m_numEdges.m_primitiveBase;
		}


		if (Positions.size() < 3)
			return Graph; // Maybe eventually we can just manually link them

		std::vector<std::pair<int, std::vector<int>>> Clusters = KMeansCluster(Positions, Config.mKMeansClusteringK < Positions.size() ? Config.mKMeansClusteringK : Positions.size() - 1);

        std::vector<glm::vec4> clusteredPositions;
        clusteredPositions.resize(Clusters.size());
		for (int i = 0; i < Clusters.size(); i++)
			clusteredPositions[i] = Positions[Clusters[i].first];

		// Here we would usually project our 3D points to a 2D plane for delaunay triangulation.
		// However, for navigation stuff we can assume the normal of that plane is the Y-axis.
		// We can therefore just omit the Y value of our points.
        std::vector<IndexedPoint> indexedPoints;
        indexedPoints.resize(clusteredPositions.size());
		for (size_t i = 0; i < clusteredPositions.size(); i++)
		{
			indexedPoints[i].mX = clusteredPositions[i].x;
			indexedPoints[i].mY = clusteredPositions[i].z;
			indexedPoints[i].mIndex = i;
		}

        std::vector<double> IndexedPointsConverted;
        for (const IndexedPoint& Point : indexedPoints)
        {
            IndexedPointsConverted.push_back(Point.mX);
            IndexedPointsConverted.push_back(Point.mY);
        }
        delaunator::Delaunator d(IndexedPointsConverted);

        std::unordered_map<int, std::vector<int>> directedEdges;

        for (size_t e = 0; e < d.triangles.size(); e++) 
        {
            if (e > d.halfedges[e]) 
            {
                /*
                float tx0 = d.coords[2 * d.triangles[e]];
                float ty0 = d.coords[2 * d.triangles[e] + 1];

                float tx1 = d.coords[2 * d.triangles[(e % 3 == 2) ? e - 2 : e + 1]];
                float ty1 = d.coords[2 * d.triangles[(e % 3 == 2) ? e - 2 : e + 1] + 1];
                */

                uint32_t pIndex = d.triangles[e];
                uint32_t qIndex = d.triangles[(e % 3 == 2) ? e - 2 : e + 1];

                
                if (glm::abs(glm::normalize((clusteredPositions[pIndex] - clusteredPositions[qIndex]))).y > Config.mWalkableSlopeAngle / 90.0f)
                    continue;

                if (directedEdges.contains(pIndex))
                    directedEdges[pIndex].push_back(qIndex);
                else
                    directedEdges.insert({ pIndex, std::vector<int> { (int)qIndex } });

                if (directedEdges.contains(qIndex))
                    directedEdges[qIndex].push_back(pIndex);
                else
                    directedEdges.insert({ qIndex, std::vector<int> { (int)pIndex } });
            }
        }

        int posStartIdx = Graph.m_positions.size();

        auto CalcEdgeCost = [](glm::vec4 self, glm::vec4 target)
            {
                glm::vec4 vec = target - self;
                return (float)(glm::length(vec) / 20.0f /*+ (vec.Y * config.CostYScale)*/);
            };

        for (int i = 0; i < clusteredPositions.size(); i++)
        {
            application::file::game::phive::PhiveNavMesh::hkVector4 Vec;
            Vec.m_subTypeArray[0] = clusteredPositions[i].x;
            Vec.m_subTypeArray[1] = clusteredPositions[i].y;
            Vec.m_subTypeArray[2] = clusteredPositions[i].z;
            Vec.m_subTypeArray[3] = 0.0f;
            Graph.m_positions.push_back(Vec);

            int edgeStartIdx = Graph.m_edges.size();

            std::vector<int>* targets = nullptr;
            if (directedEdges.contains(i))
            {
                targets = &directedEdges[i];
            }
            if (targets != nullptr)
            {
                for(int& target : *targets)
                {
                    if (CalcEdgeCost(clusteredPositions[i], clusteredPositions[target]) > 5.0f)
                        continue;

                    application::file::game::phive::PhiveNavMesh::hkaiClusterGraph__Edge Edge;
                    Edge.m_flags.m_storage.m_primitiveBase = 0x0;
                    Edge.m_source.m_primitiveBase = posStartIdx + target;
                    float Cost = CalcEdgeCost(clusteredPositions[i], clusteredPositions[target]);
                    Edge.m_cost.m_value.m_primitiveBase = application::util::Math::ConvertIntToHalf(*reinterpret_cast<int*>(&Cost));
                    Graph.m_edges.push_back(Edge);
                }
            }

            application::file::game::phive::PhiveNavMesh::hkaiClusterGraph__Node Node;
            Node.m_numEdges = Graph.m_edges.size() - edgeStartIdx;
            Node.m_startEdgeIndex = edgeStartIdx;
            Graph.m_nodes.push_back(Node);
        }

        for (auto& Face : NavMesh.m_faces)
        {
            application::file::game::phive::PhiveNavMesh::hkaiIndex NullIndex;
            NullIndex.m_primitiveBase = 0;
            Graph.m_featureToNodeIndex.push_back(NullIndex);
        }

        return Graph;
	}

    // Returns centroids as indices into points array, as well as what points the clusters contain.
    // Because of this, these centroids are not perfectly accurate,
    // but they *do* land on already defined points.
    std::vector<std::pair<int, std::vector<int>>> hkaiClusterGraphBuilder::KMeansCluster(std::vector<glm::vec4> Points, int k)
    {
        std::random_device RDev;
        std::mt19937 RGen(RDev());
        std::uniform_int_distribution<int> IDist(0, Points.size() - 1);

        std::vector<int> ClusterCentroids; // We want to make sure that centroids fall on points to match vanilla behaviour. We use indexes instead of Vector3s to represent.
        ClusterCentroids.resize(k);
        for (int i = 0; i < k; i++)
        {
            int randIdx = -1;
            while (randIdx == -1 || std::find(ClusterCentroids.begin(), ClusterCentroids.end(), randIdx) != ClusterCentroids.end())
                randIdx = IDist(RGen);
            ClusterCentroids[i] = randIdx;
        }

        std::vector<std::vector<int>>* LastClusters = nullptr;

        std::vector<std::vector<int>> CurrentClusters(k);
        while (LastClusters == nullptr || !std::equal(CurrentClusters.begin(), CurrentClusters.end(),
            LastClusters->begin(),
            [](const std::vector<int>& a, const std::vector<int>& b) {
                return a == b;
            }))
        {
            LastClusters = &CurrentClusters;
            CurrentClusters.clear();
            CurrentClusters.resize(k);

            for (size_t i = 0; i < Points.size(); i++)
            {
                int clusterIdx = -1;
                float minLength = FLT_MAX;
                for (size_t j = 0; j < ClusterCentroids.size(); j++)
                {
                    float length = glm::length(Points[i] - Points[ClusterCentroids[j]]);
                    if (length < minLength)
                    {
                        minLength = length;
                        clusterIdx = j;
                    }
                }

                CurrentClusters[clusterIdx].push_back(i);
            }

            // Update centroids
            for (size_t i = 0; i < CurrentClusters.size(); i++)
            {
                glm::vec4 centroid = glm::vec4(0, 0, 0, 0);

                for(int& pointIdx : CurrentClusters[i])
                    centroid += Points[pointIdx];
                centroid /= (int)CurrentClusters[i].size();

                // Find closest point, again, this is done to emulate vanilla behaviour.
                int centroidPointIdx = -1;
                float minLength = FLT_MAX;
                for(int& pointIdx : CurrentClusters[i])
                {
                    float length = glm::length(Points[pointIdx] - centroid);
                    if (length < minLength)
                    {
                        minLength = length;
                        centroidPointIdx = pointIdx;
                    }
                }

                ClusterCentroids[i] = centroidPointIdx;
            }
        }

        std::vector<std::pair<int, std::vector<int>>> res;
        res.resize(CurrentClusters.size());
        for (size_t i = 0; i < CurrentClusters.size(); i++)
        {
            res[i] = std::make_pair(ClusterCentroids[i], CurrentClusters[i]);
        }

        return res;
    }
}