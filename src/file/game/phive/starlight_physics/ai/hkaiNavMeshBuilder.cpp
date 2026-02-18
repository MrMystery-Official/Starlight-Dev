#include <file/game/phive/starlight_physics/ai/hkaiNavMeshBuilder.h>

#include <file/game/phive/starlight_physics/ai/hkaiNavMeshGeometryGenerator.h>
#include <file/game/phive/starlight_physics/common/hkcdStaticAabbTreeGeneration.h>
#include <util/Math.h>
#include <queue>

namespace application::file::game::phive::starlight_physics::ai
{
	void hkaiNavMeshBuilder::initialize(const hkaiNavMeshGeometryGenerator::Config& config)
	{
		geometryGenerator.setNavmeshBuildParams(config);
	}

	bool hkaiNavMeshBuilder::buildNavMesh(classes::HavokClasses::hkaiNavMesh* dst, std::vector<glm::vec3>& vertices, std::vector<uint32_t>& indices)
	{
		if (!geometryGenerator.buildNavmeshForMesh(reinterpret_cast<float*>(vertices.data()), vertices.size(), reinterpret_cast<const int*>(indices.data()), indices.size()))
		{
			application::util::Logger::Error("PhiveNavMesh", "Failed to generate navigation mesh");
			return false;
		}

		int VCount = geometryGenerator.getMeshVertexCount();
		int ICount = geometryGenerator.getMeshTriangleCount();
		if (VCount == 0 || ICount == 0)
		{
			application::util::Logger::Error("PhiveNavMesh", "Resulting navigation mesh is empty");
			return false;
		}

		std::vector<uint16_t> BVerts;
		std::vector<uint16_t> BIndices;
		BVerts.resize(VCount * 3);
		BIndices.resize(ICount * 3 * 2);
		geometryGenerator.getMeshVertices(BVerts.data());
		geometryGenerator.getMeshTriangles(BIndices.data());

		glm::vec3 Bounds[2];
		Bounds[0].x = geometryGenerator.mBoundingBoxMin[0];
		Bounds[0].y = geometryGenerator.mBoundingBoxMin[1];
		Bounds[0].z = geometryGenerator.mBoundingBoxMin[2];
		Bounds[1].x = geometryGenerator.mBoundingBoxMax[0];
		Bounds[1].y = geometryGenerator.mBoundingBoxMax[1];
		Bounds[1].z = geometryGenerator.mBoundingBoxMax[2];


		std::vector<glm::vec3> OptimizedVertices;
		for (size_t i = 0; i < BVerts.size() / 3; i++)
		{
			uint16_t VX = BVerts[i * 3];
			uint16_t VY = BVerts[i * 3 + 1];
			uint16_t VZ = BVerts[i * 3 + 2];

			glm::vec3 Vertex = glm::vec3(Bounds[0].x + VX * geometryGenerator.mConfig.cs,
				Bounds[0].y + VY * geometryGenerator.mConfig.ch,
				Bounds[0].z + VZ * geometryGenerator.mConfig.cs);

			OptimizedVertices.push_back(Vertex);
		}

		dst->mFaces.mElements.clear();
		dst->mEdges.mElements.clear();
		dst->mVertices.mElements.clear();
		dst->mFaceData.mElements.clear();
		dst->mEdgeData.mElements.clear();
		dst->mStreamingSets.mElements.clear();

		dst->mAabb.mMin.FromVec4(glm::vec4(Bounds[0], 1.0f));
		dst->mAabb.mMax.FromVec4(glm::vec4(Bounds[1], 1.0f));

		dst->mErosionRadius.mParent = 0;
		dst->mUserData.mParent = 0;

		for (const glm::vec3& Vertex : OptimizedVertices)
		{
			classes::HavokClasses::hkVector4 Vector;
			Vector.FromVec4(glm::vec4(Vertex, 1.0f));
			dst->mVertices.mElements.push_back(Vector);
		}

		classes::HavokClasses::hkInt32 hkInt32Null;
		hkInt32Null.mParent = 0;

		for (size_t t = 0; t < BIndices.size() / 2; t += 3)
		{
			classes::HavokClasses::hkaiNavMesh__Face Face;
			Face.mNumEdges.mParent = 3;
			Face.mStartUserEdgeIndex.mParent.mParent = -1;
			Face.mNumUserEdges.mParent = 0;
			Face.mStartEdgeIndex.mParent.mParent = dst->mEdges.mElements.size();
			dst->mFaces.mElements.push_back(Face);
			dst->mFaceData.mElements.push_back(hkInt32Null);

			for (int i = 0; i < 3; i++)
			{
				classes::HavokClasses::hkaiNavMesh__Edge Edge;
				Edge.mA.mParent.mParent = BIndices[t * 2 + i];
				Edge.mB.mParent.mParent = BIndices[t * 2 + (i + 1) % 3];
				Edge.mFlags.mStorage.mParent = static_cast<unsigned char>(classes::HavokClasses::hkaiNavMesh__EdgeFlagBits::EDGE_ORIGINAL);
				Edge.mPaddingByte.mParent = 0;
				Edge.mUserEdgeCost.mValue.mParent = 0;

				if (BIndices[t * 2 + 3 + i] == 0xFFFF)
				{
					Edge.mOppositeEdge.mParent = 0xFFFFFFFF;
					Edge.mOppositeFace.mParent = 0xFFFFFFFF;
				}
				else
				{
					Edge.mOppositeFace.mParent = BIndices[t * 2 + 3 + i];
					for (int j = 0; j < 3; j++)
					{
						uint16_t AnotherEdge = BIndices[t * 2 + 3 + i] * 6 + 3 + j;
						if (BIndices[AnotherEdge] == t / 3)
							Edge.mOppositeEdge.mParent = (uint32_t)BIndices[t * 2 + 3 + i] * 3 + (uint32_t)j;
					}
				}

				dst->mEdges.mElements.push_back(Edge);
				dst->mEdgeData.mElements.push_back(hkInt32Null);
			}
		}




		uint32_t numFaces = (uint32_t)dst->mFaces.mElements.size();
		std::vector<common::hkAabb> aabbs;
		aabbs.reserve(numFaces);

		for (uint32_t fIdx = 0; fIdx < numFaces; ++fIdx)
		{
			const auto& face = dst->mFaces.mElements[fIdx];

			glm::vec3 minBound(std::numeric_limits<float>::max());
			glm::vec3 maxBound(-std::numeric_limits<float>::max());

			int startEdge = face.mStartEdgeIndex.mParent.mParent;
			int numEdges = face.mNumEdges.mParent;

			for (int e = 0; e < numEdges; ++e)
			{
				int edgeIdx = startEdge + e;
				const auto& edge = dst->mEdges.mElements[edgeIdx];

				int vertIdx = edge.mA.mParent.mParent;
				const auto& vertex = dst->mVertices.mElements[vertIdx];

				glm::vec4 v = vertex.ToVec4();

				minBound.x = std::min(minBound.x, v.x);
				minBound.y = std::min(minBound.y, v.y);
				minBound.z = std::min(minBound.z, v.z);

				maxBound.x = std::max(maxBound.x, v.x);
				maxBound.y = std::max(maxBound.y, v.y);
				maxBound.z = std::max(maxBound.z, v.z);
			}

			common::hkAabb faceAabb;
			faceAabb.mMin = glm::vec4(minBound, 1.0f);
			faceAabb.mMax = glm::vec4(maxBound, 1.0f);

			aabbs.push_back(faceAabb);
		}

		common::hkcdStaticAabbTreeGeneration::buildFromAabbs(&dst->mCachedFaceIterator.mObj.mTree.mObj, aabbs);

		return true;
	}

	void hkaiNavMeshBuilder::buildClusterGraph(const classes::HavokClasses::hkaiNavMesh& mesh, classes::HavokClasses::hkaiClusterGraph& graphOut, uint32_t desiredFaces)
	{
		graphOut.mPositions.mElements.clear();
        graphOut.mNodes.mElements.clear();
        graphOut.mEdges.mElements.clear();
        graphOut.mNodeData.mElements.clear();
        graphOut.mEdgeData.mElements.clear();
        graphOut.mFeatureToNodeIndex.mElements.clear();
        graphOut.mStreamingSets.mElements.clear();
        graphOut.mUserData.mParent = 0;

        graphOut.mNodeDataStriding = 0;
        graphOut.mEdgeDataStriding = 0;

        uint32_t numFaces = mesh.mFaces.mElements.size();
        if (numFaces == 0) return;

        struct FaceAdj { std::vector<int> mNeighbors; };
        std::vector<FaceAdj> adjacency(numFaces);

        for (uint32_t faceIdx = 0; faceIdx < numFaces; ++faceIdx)
        {
            const auto& face = mesh.mFaces.mElements[faceIdx];
            for (int e = 0; e < face.mNumEdges.mParent; ++e)
            {
                int edgeIdx = face.mStartEdgeIndex.mParent.mParent + e;
                const auto& edge = mesh.mEdges.mElements[edgeIdx];
                if (edge.mOppositeFace.mParent != -1)
                {
                    adjacency[faceIdx].mNeighbors.push_back(edge.mOppositeFace.mParent);
                }
            }
        }

        int desiredSize = MATH_MAX(1, desiredFaces);
        int numClusters = (numFaces + desiredSize - 1) / desiredSize;

        std::vector<int> faceToCluster(numFaces, -1);

        struct QueueItem {
            int faceIdx;
            int clusterIdx;
        };
        std::queue<QueueItem> queue;

        for (int i = 0; i < numClusters; ++i)
        {
            int seedFace = (i * numFaces) / numClusters;

            if (seedFace < numFaces) {
                faceToCluster[seedFace] = i;
                queue.push({ seedFace, i });
            }
        }

        while (!queue.empty())
        {
            QueueItem item = queue.front();
            queue.pop();

            for (int neighbor : adjacency[item.faceIdx].mNeighbors)
            {
                if (faceToCluster[neighbor] == -1)
                {
                    faceToCluster[neighbor] = item.clusterIdx;
                    queue.push({ neighbor, item.clusterIdx });
                }
            }
        }

        for (int i = 0; i < numFaces; ++i)
        {
            if (faceToCluster[i] == -1)
            {
                int newClusterIdx = numClusters++;
                faceToCluster[i] = newClusterIdx;
                queue.push({ i, newClusterIdx });

                while (!queue.empty()) {
                    QueueItem item = queue.front();
                    queue.pop();
                    for (int neighbor : adjacency[item.faceIdx].mNeighbors) {
                        if (faceToCluster[neighbor] == -1) {
                            faceToCluster[neighbor] = item.clusterIdx;
                            queue.push({ neighbor, item.clusterIdx });
                        }
                    }
                }
            }
        }

        graphOut.mPositions.mElements.resize(numClusters);
        std::vector<int> clusterSize(numClusters, 0);

        for (int ci = 0; ci < numClusters; ++ci)
        {
            graphOut.mPositions.mElements[ci].FromVec4(glm::vec4(0, 0, 0, 0));
        }

        for (int f = 0; f < numFaces; ++f)
        {
            int cIdx = faceToCluster[f];
            const auto& face = mesh.mFaces.mElements[f];

            glm::vec4 faceCenter(0.0f);
            for (int e = 0; e < face.mNumEdges.mParent; ++e)
            {
                int edgeIdx = face.mStartEdgeIndex.mParent.mParent + e;
                int vertIdx = mesh.mEdges.mElements[edgeIdx].mA.mParent.mParent;
                faceCenter += mesh.mVertices.mElements[vertIdx].ToVec4();
            }
            faceCenter /= (float)face.mNumEdges.mParent;

            glm::vec4 currentPos = graphOut.mPositions.mElements[cIdx].ToVec4();
            currentPos += faceCenter;
            graphOut.mPositions.mElements[cIdx].FromVec4(currentPos);

            clusterSize[cIdx]++;
        }

        for (int ci = 0; ci < numClusters; ++ci)
        {
            if (clusterSize[ci] > 0)
            {
                glm::vec4 sum = graphOut.mPositions.mElements[ci].ToVec4();
                sum /= (float)clusterSize[ci];
                sum.w = 0.0f;
                graphOut.mPositions.mElements[ci].FromVec4(sum);
            }
        }

        struct EdgeKey {
            int target;
            float cost;
            bool operator<(const EdgeKey& other) const { return target < other.target; }
        };
        std::vector<std::vector<int>> clusterNeighbors(numClusters);

        for (int faceIdx = 0; faceIdx < numFaces; ++faceIdx)
        {
            int clusterA = faceToCluster[faceIdx];
            for (int neighbor : adjacency[faceIdx].mNeighbors)
            {
                int clusterB = faceToCluster[neighbor];
                if (clusterA != clusterB)
                {
                    bool found = false;
                    for (int existing : clusterNeighbors[clusterA]) {
                        if (existing == clusterB) { found = true; break; }
                    }

                    if (!found)
                    {
                        clusterNeighbors[clusterA].push_back(clusterB);
                    }
                }
            }
        }

        graphOut.mNodes.mElements.resize(numClusters);

        graphOut.mEdges.mElements.reserve(numClusters * 4);

        for (int ci = 0; ci < numClusters; ++ci)
        {
            auto& node = graphOut.mNodes.mElements[ci];
            node.mStartEdgeIndex = (int)graphOut.mEdges.mElements.size();

            const auto& neighbors = clusterNeighbors[ci];
            node.mNumEdges = (int)neighbors.size();

            glm::vec4 posA = graphOut.mPositions.mElements[ci].ToVec4();

            for (int targetCluster : neighbors)
            {
                classes::HavokClasses::hkaiClusterGraph__Edge edge;

                edge.mSource.mParent = targetCluster;

                glm::vec4 posB = graphOut.mPositions.mElements[targetCluster].ToVec4();
                float cost = glm::length(glm::vec3(posB - posA));

                edge.mCost.mValue.mParent = cost;

                graphOut.mEdges.mElements.push_back(edge);
            }
        }

        graphOut.mFeatureToNodeIndex.mElements.resize(numFaces);
        for (int f = 0; f < numFaces; ++f)
        {
            graphOut.mFeatureToNodeIndex.mElements[f].mParent.mParent = faceToCluster[f];
        }
	}
}