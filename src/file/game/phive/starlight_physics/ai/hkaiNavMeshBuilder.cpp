#include <file/game/phive/starlight_physics/ai/hkaiNavMeshBuilder.h>

#include <file/game/phive/starlight_physics/ai/hkaiNavMeshGeometryGenerator.h>
#include <file/game/phive/starlight_physics/common/hkcdStaticAabbTreeGeneration.h>
#include <util/Math.h>
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <queue>
#include <unordered_map>
#include <vector>

namespace
{
	bool ValidateBuiltNavMesh(const application::file::game::phive::classes::HavokClasses::hkaiNavMesh& mesh)
	{
		const auto& faces = mesh.mFaces.mElements;
		const auto& edges = mesh.mEdges.mElements;
		const auto& vertices = mesh.mVertices.mElements;
		if (faces.empty() || edges.empty() || vertices.empty())
		{
			application::util::Logger::Error("PhiveNavMesh", "Validation failed: navmesh has empty faces/edges/vertices");
			return false;
		}

		if (mesh.mFaceDataStriding > 0 && mesh.mFaceData.mElements.size() != faces.size() * static_cast<size_t>(mesh.mFaceDataStriding))
		{
			application::util::Logger::Error("PhiveNavMesh", "Validation failed: faceData striding mismatch (%u faces, stride %d, data %u)",
				static_cast<uint32_t>(faces.size()),
				mesh.mFaceDataStriding,
				static_cast<uint32_t>(mesh.mFaceData.mElements.size()));
			return false;
		}

		if (mesh.mEdgeDataStriding > 0 && mesh.mEdgeData.mElements.size() != edges.size() * static_cast<size_t>(mesh.mEdgeDataStriding))
		{
			application::util::Logger::Error("PhiveNavMesh", "Validation failed: edgeData striding mismatch (%u edges, stride %d, data %u)",
				static_cast<uint32_t>(edges.size()),
				mesh.mEdgeDataStriding,
				static_cast<uint32_t>(mesh.mEdgeData.mElements.size()));
			return false;
		}

		std::vector<uint8_t> edgeCoverage(edges.size(), 0);
		for (uint32_t faceIndex = 0; faceIndex < static_cast<uint32_t>(faces.size()); ++faceIndex)
		{
			const auto& face = faces[faceIndex];
			const int32_t startEdge = face.mStartEdgeIndex.mParent.mParent;
			const int32_t numEdges = face.mNumEdges.mParent;
			if (numEdges < 3 || startEdge < 0 || startEdge + numEdges > static_cast<int32_t>(edges.size()))
			{
				application::util::Logger::Error("PhiveNavMesh", "Validation failed: invalid edge range for face %u", faceIndex);
				return false;
			}

			for (int32_t localEdge = 0; localEdge < numEdges; ++localEdge)
			{
				const int32_t edgeIndex = startEdge + localEdge;
				const int32_t prevEdgeIndex = startEdge + ((localEdge + numEdges - 1) % numEdges);
				const auto& edge = edges[edgeIndex];
				const auto& prevEdge = edges[prevEdgeIndex];

				if (edge.mA.mParent.mParent < 0 || edge.mB.mParent.mParent < 0 ||
					edge.mA.mParent.mParent >= static_cast<int32_t>(vertices.size()) ||
					edge.mB.mParent.mParent >= static_cast<int32_t>(vertices.size()) ||
					edge.mA.mParent.mParent == edge.mB.mParent.mParent)
				{
					application::util::Logger::Error("PhiveNavMesh", "Validation failed: invalid vertex indices for edge %u", static_cast<uint32_t>(edgeIndex));
					return false;
				}

				if (prevEdge.mB.mParent.mParent != edge.mA.mParent.mParent)
				{
					application::util::Logger::Error("PhiveNavMesh", "Validation failed: broken face loop on face %u", faceIndex);
					return false;
				}

				const bool hasOppositeEdge = edge.mOppositeEdge.mParent != 0xFFFFFFFFu;
				const bool hasOppositeFace = edge.mOppositeFace.mParent != 0xFFFFFFFFu;
				if (hasOppositeEdge != hasOppositeFace)
				{
					application::util::Logger::Error("PhiveNavMesh", "Validation failed: opposite edge/face mismatch on edge %u", static_cast<uint32_t>(edgeIndex));
					return false;
				}

				if (hasOppositeEdge)
				{
					const uint32_t oppositeEdgeIndex = edge.mOppositeEdge.mParent;
					const uint32_t oppositeFaceIndex = edge.mOppositeFace.mParent;
					if (oppositeEdgeIndex >= edges.size() || oppositeFaceIndex >= faces.size())
					{
						application::util::Logger::Error("PhiveNavMesh", "Validation failed: opposite index out of range on edge %u", static_cast<uint32_t>(edgeIndex));
						return false;
					}

					const auto& oppositeEdge = edges[oppositeEdgeIndex];
					if (oppositeEdge.mOppositeEdge.mParent != static_cast<uint32_t>(edgeIndex) ||
						oppositeEdge.mOppositeFace.mParent != faceIndex ||
						edge.mA.mParent.mParent != oppositeEdge.mB.mParent.mParent ||
						edge.mB.mParent.mParent != oppositeEdge.mA.mParent.mParent)
					{
						application::util::Logger::Error("PhiveNavMesh", "Validation failed: opposite edge pairing mismatch on edge %u", static_cast<uint32_t>(edgeIndex));
						return false;
					}
				}

				if (++edgeCoverage[static_cast<size_t>(edgeIndex)] != 1u)
				{
					application::util::Logger::Error("PhiveNavMesh", "Validation failed: edge %u owned by multiple faces", static_cast<uint32_t>(edgeIndex));
					return false;
				}
			}
		}

		const glm::vec4 up = mesh.mUp.mUp.ToVec4();
		const float upLenSq = up.x * up.x + up.y * up.y + up.z * up.z;
		if (!std::isfinite(upLenSq) || std::fabs(upLenSq - 1.0f) > 0.1f)
		{
			application::util::Logger::Error("PhiveNavMesh", "Validation failed: invalid up vector (%.3f, %.3f, %.3f)", up.x, up.y, up.z);
			return false;
		}

		return true;
	}

	bool ValidateBuiltClusterGraph(
		const application::file::game::phive::classes::HavokClasses::hkaiClusterGraph& graph,
		uint32_t expectedFaceCount)
	{
		const auto& nodes = graph.mNodes.mElements;
		const auto& edges = graph.mEdges.mElements;
		if (graph.mPositions.mElements.size() != nodes.size())
		{
			application::util::Logger::Error("PhiveNavMesh", "Validation failed: cluster graph node/position count mismatch (%u != %u)",
				static_cast<uint32_t>(graph.mPositions.mElements.size()),
				static_cast<uint32_t>(nodes.size()));
			return false;
		}

		if (graph.mFeatureToNodeIndex.mElements.size() != expectedFaceCount)
		{
			application::util::Logger::Error("PhiveNavMesh", "Validation failed: featureToNodeIndex mismatch (%u != %u)",
				static_cast<uint32_t>(graph.mFeatureToNodeIndex.mElements.size()),
				expectedFaceCount);
			return false;
		}

		std::vector<uint8_t> edgeCoverage(edges.size(), 0);
		for (int32_t nodeIndex = 0; nodeIndex < static_cast<int32_t>(nodes.size()); ++nodeIndex)
		{
			const auto& node = nodes[static_cast<size_t>(nodeIndex)];
			if (node.mStartEdgeIndex < 0 || node.mNumEdges < 0 || node.mStartEdgeIndex + node.mNumEdges > static_cast<int32_t>(edges.size()))
			{
				application::util::Logger::Error("PhiveNavMesh", "Validation failed: invalid edge range on cluster node %d", nodeIndex);
				return false;
			}

			for (int32_t e = 0; e < node.mNumEdges; ++e)
			{
				const int32_t edgeIndex = node.mStartEdgeIndex + e;
				const auto& edge = edges[static_cast<size_t>(edgeIndex)];
				if (edge.mSource.mParent >= nodes.size())
				{
					application::util::Logger::Error("PhiveNavMesh", "Validation failed: cluster edge %d source out of range", edgeIndex);
					return false;
				}

				if (++edgeCoverage[static_cast<size_t>(edgeIndex)] != 1u)
				{
					application::util::Logger::Error("PhiveNavMesh", "Validation failed: cluster edge %d owned by multiple nodes", edgeIndex);
					return false;
				}
			}
		}

		for (uint32_t faceIndex = 0; faceIndex < expectedFaceCount; ++faceIndex)
		{
			const int32_t nodeIndex = graph.mFeatureToNodeIndex.mElements[faceIndex].mParent.mParent;
			if (nodeIndex < 0 || nodeIndex >= static_cast<int32_t>(nodes.size()))
			{
				application::util::Logger::Error("PhiveNavMesh", "Validation failed: featureToNodeIndex[%u]=%d is invalid", faceIndex, nodeIndex);
				return false;
			}
		}

		return true;
	}

	void BuildConservativeClusterGraph(
		const application::file::game::phive::classes::HavokClasses::hkaiNavMesh& mesh,
		application::file::game::phive::classes::HavokClasses::hkaiClusterGraph& graphOut)
	{
		graphOut.mPositions.mElements.clear();
		graphOut.mNodes.mElements.clear();
		graphOut.mEdges.mElements.clear();
		graphOut.mNodeData.mElements.clear();
		graphOut.mEdgeData.mElements.clear();
		graphOut.mStreamingSets.mElements.clear();
		graphOut.mFeatureToNodeIndex.mElements.resize(mesh.mFaces.mElements.size());
		graphOut.mNodeDataStriding = 0;
		graphOut.mEdgeDataStriding = 0;
		graphOut.mUserData.mParent = 0;

		if (mesh.mFaces.mElements.empty() || mesh.mVertices.mElements.empty())
		{
			return;
		}

		glm::vec3 sum(0.0f);
		uint32_t count = 0;
		for (const auto& face : mesh.mFaces.mElements)
		{
			const int32_t startEdge = face.mStartEdgeIndex.mParent.mParent;
			if (startEdge < 0 || startEdge >= static_cast<int32_t>(mesh.mEdges.mElements.size()))
			{
				continue;
			}

			const int32_t vertexIndex = mesh.mEdges.mElements[startEdge].mA.mParent.mParent;
			if (vertexIndex < 0 || vertexIndex >= static_cast<int32_t>(mesh.mVertices.mElements.size()))
			{
				continue;
			}

			const glm::vec4 v = mesh.mVertices.mElements[vertexIndex].ToVec4();
			sum += glm::vec3(v.x, v.y, v.z);
			++count;
		}

		if (count == 0)
		{
			const glm::vec4 v = mesh.mVertices.mElements.front().ToVec4();
			sum = glm::vec3(v.x, v.y, v.z);
			count = 1;
		}
		const glm::vec3 centroid = sum / static_cast<float>(count);

		graphOut.mPositions.mElements.resize(1);
		graphOut.mPositions.mElements[0].FromVec4(glm::vec4(centroid, 1.0f));
		graphOut.mNodes.mElements.resize(1);
		graphOut.mNodes.mElements[0].mStartEdgeIndex = 0;
		graphOut.mNodes.mElements[0].mNumEdges = 0;

		for (auto& entry : graphOut.mFeatureToNodeIndex.mElements)
		{
			entry.mParent.mParent = 0;
		}
	}
}

namespace application::file::game::phive::starlight_physics::ai
{
	void hkaiNavMeshBuilder::initialize(const hkaiNavMeshGeometryGenerator::Config& config)
	{
		geometryGenerator.setNavmeshBuildParams(config);
	}

	bool hkaiNavMeshBuilder::buildNavMesh(classes::HavokClasses::hkaiNavMesh* dst, std::vector<glm::vec3>& vertices, std::vector<uint32_t>& indices, std::vector<glm::vec3>& nonOptimizeVertices, std::vector<uint32_t>& nonOptimizeIndices)
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

			if (geometryGenerator.mUseForcedXZBounds)
			{
				Vertex.x = std::clamp(Vertex.x, geometryGenerator.mForcedMinX, geometryGenerator.mForcedMaxX);
				Vertex.z = std::clamp(Vertex.z, geometryGenerator.mForcedMinZ, geometryGenerator.mForcedMaxZ);
			}

			OptimizedVertices.push_back(Vertex);
		}

		const uint32_t nonOptimizedVertexBase = static_cast<uint32_t>(OptimizedVertices.size());
		OptimizedVertices.insert(OptimizedVertices.end(), nonOptimizeVertices.begin(), nonOptimizeVertices.end());

		if (!nonOptimizeIndices.empty())
		{
			if (nonOptimizeIndices.size() % 3 != 0)
			{
				application::util::Logger::Warning(
					"PhiveNavMesh",
					"Non-optimized index buffer has trailing indices (%u values), dropping remainder",
					static_cast<uint32_t>(nonOptimizeIndices.size()));
			}

			const size_t nonOptimizedTriangleCount = nonOptimizeIndices.size() / 3;
			BIndices.reserve(BIndices.size() + nonOptimizedTriangleCount * 6);
			for (size_t tri = 0; tri < nonOptimizedTriangleCount; ++tri)
			{
				const uint32_t localA = nonOptimizeIndices[tri * 3 + 0];
				const uint32_t localB = nonOptimizeIndices[tri * 3 + 1];
				const uint32_t localC = nonOptimizeIndices[tri * 3 + 2];
				if (localA >= nonOptimizeVertices.size() ||
					localB >= nonOptimizeVertices.size() ||
					localC >= nonOptimizeVertices.size())
				{
					continue;
				}

				const uint32_t a = nonOptimizedVertexBase + localA;
				const uint32_t b = nonOptimizedVertexBase + localB;
				const uint32_t c = nonOptimizedVertexBase + localC;
				if (a > 0xFFFFu || b > 0xFFFFu || c > 0xFFFFu)
				{
					application::util::Logger::Error(
						"PhiveNavMesh",
						"Non-optimized triangle vertex index overflow (%u, %u, %u); max supported is 65535",
						a,
						b,
						c);
					return false;
				}

				BIndices.push_back(static_cast<uint16_t>(a));
				BIndices.push_back(static_cast<uint16_t>(b));
				BIndices.push_back(static_cast<uint16_t>(c));
				BIndices.push_back(0xFFFFu);
				BIndices.push_back(0xFFFFu);
				BIndices.push_back(0xFFFFu);
			}
		}

		glm::vec3 BoundingBoxMin(
			std::numeric_limits<float>::max(),
			std::numeric_limits<float>::max(),
			std::numeric_limits<float>::max()
		);

		glm::vec3 BoundingBoxMax(
			std::numeric_limits<float>::lowest(),
			std::numeric_limits<float>::lowest(),
			std::numeric_limits<float>::lowest()
		);

		for (const auto& p : OptimizedVertices)
		{
			BoundingBoxMin.x = std::min(BoundingBoxMin.x, p.x);
			BoundingBoxMin.y = std::min(BoundingBoxMin.y, p.y);
			BoundingBoxMin.z = std::min(BoundingBoxMin.z, p.z);

			BoundingBoxMax.x = std::max(BoundingBoxMax.x, p.x);
			BoundingBoxMax.y = std::max(BoundingBoxMax.y, p.y);
			BoundingBoxMax.z = std::max(BoundingBoxMax.z, p.z);
		}

		dst->mFaces.mElements.clear();
		dst->mEdges.mElements.clear();
		dst->mVertices.mElements.clear();
		dst->mFaceData.mElements.clear();
		dst->mEdgeData.mElements.clear();
		dst->mStreamingSets.mElements.clear();
		dst->mFaceDataStriding = 1;
		dst->mEdgeDataStriding = 1;
		dst->mFlags = 0;

		classes::HavokClasses::hkVector4 Up;
		Up.FromVec4(glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));
		dst->mUp.mUp = Up;

		dst->mAabb.mMin.FromVec4(glm::vec4(BoundingBoxMin, 1.0f));
		dst->mAabb.mMax.FromVec4(glm::vec4(BoundingBoxMax, 1.0f));

		dst->mErosionRadius.mParent = 0;
		dst->mUserData.mParent = 0;
		dst->mCachedFaceIterator.mIsNullPtr = false;
		dst->mClearanceCacheSeedingDataSet.mObj.mCacheDatas.mElements.clear();
		dst->mClearanceCacheSeedingDataSet.mIsNullPtr = true;

		for (const glm::vec3& Vertex : OptimizedVertices)
		{
			classes::HavokClasses::hkVector4 Vector;
			Vector.FromVec4(glm::vec4(Vertex, 1.0f));
			dst->mVertices.mElements.push_back(Vector);
		}

		classes::HavokClasses::hkInt32 hkInt32Null;
		hkInt32Null.mParent = 0;

		constexpr uint32_t INVALID_EDGE_OR_FACE = 0xFFFFFFFFu;
		constexpr size_t PACKED_TRIANGLE_STRIDE = 6;
		if (BIndices.size() % PACKED_TRIANGLE_STRIDE != 0)
		{
			application::util::Logger::Error("PhiveNavMesh", "NavMesh generation failed: invalid packed triangle buffer size (%u)",
				static_cast<uint32_t>(BIndices.size()));
			return false;
		}

		const uint32_t packedFaceCount = static_cast<uint32_t>(BIndices.size() / PACKED_TRIANGLE_STRIDE);
		std::vector<int32_t> edgeToFace;
		edgeToFace.reserve(static_cast<size_t>(packedFaceCount) * 3);

		for (uint32_t faceIndex = 0; faceIndex < packedFaceCount; ++faceIndex)
		{
			const size_t triBase = static_cast<size_t>(faceIndex) * PACKED_TRIANGLE_STRIDE;

			classes::HavokClasses::hkaiNavMesh__Face Face;
			Face.mNumEdges.mParent = 3;
			Face.mStartUserEdgeIndex.mParent.mParent = -1;
			Face.mNumUserEdges.mParent = 0;
			Face.mStartEdgeIndex.mParent.mParent = dst->mEdges.mElements.size();
			dst->mFaces.mElements.push_back(Face);
			dst->mFaceData.mElements.push_back(hkInt32Null);

			for (int localEdge = 0; localEdge < 3; ++localEdge)
			{
				classes::HavokClasses::hkaiNavMesh__Edge Edge;
				Edge.mA.mParent.mParent = BIndices[triBase + static_cast<size_t>(localEdge)];
				Edge.mB.mParent.mParent = BIndices[triBase + static_cast<size_t>((localEdge + 1) % 3)];
				Edge.mFlags.mStorage.mParent = static_cast<unsigned char>(classes::HavokClasses::hkaiNavMesh__EdgeFlagBits::EDGE_ORIGINAL);
				Edge.mPaddingByte.mParent = 0;
				Edge.mUserEdgeCost.mValue.mParent = 0;
				Edge.mOppositeEdge.mParent = INVALID_EDGE_OR_FACE;
				Edge.mOppositeFace.mParent = INVALID_EDGE_OR_FACE;

				dst->mEdges.mElements.push_back(Edge);
				dst->mEdgeData.mElements.push_back(hkInt32Null);
				edgeToFace.push_back(static_cast<int32_t>(faceIndex));
			}
		}

		const auto clearOppositeLink = [&](uint32_t edgeIndex)
			{
				if (edgeIndex >= dst->mEdges.mElements.size())
				{
					return;
				}

				auto& edge = dst->mEdges.mElements[edgeIndex];
				const uint32_t oppositeEdgeIndex = edge.mOppositeEdge.mParent;
				if (oppositeEdgeIndex != INVALID_EDGE_OR_FACE && oppositeEdgeIndex < dst->mEdges.mElements.size())
				{
					auto& oppositeEdge = dst->mEdges.mElements[oppositeEdgeIndex];
					if (oppositeEdge.mOppositeEdge.mParent == edgeIndex)
					{
						oppositeEdge.mOppositeEdge.mParent = INVALID_EDGE_OR_FACE;
						oppositeEdge.mOppositeFace.mParent = INVALID_EDGE_OR_FACE;
					}
				}

				edge.mOppositeEdge.mParent = INVALID_EDGE_OR_FACE;
				edge.mOppositeFace.mParent = INVALID_EDGE_OR_FACE;
			};

		for (uint32_t faceIndex = 0; faceIndex < packedFaceCount; ++faceIndex)
		{
			const size_t triBase = static_cast<size_t>(faceIndex) * PACKED_TRIANGLE_STRIDE;
			for (int localEdge = 0; localEdge < 3; ++localEdge)
			{
				const uint32_t edgeIndex = faceIndex * 3 + static_cast<uint32_t>(localEdge);
				auto& edge = dst->mEdges.mElements[edgeIndex];
				if (edge.mOppositeEdge.mParent != INVALID_EDGE_OR_FACE || edge.mOppositeFace.mParent != INVALID_EDGE_OR_FACE)
				{
					continue;
				}

				const uint16_t packedOppositeFace = BIndices[triBase + 3 + static_cast<size_t>(localEdge)];
				if (packedOppositeFace == 0xFFFFu)
				{
					continue;
				}

				const uint32_t oppositeFace = static_cast<uint32_t>(packedOppositeFace);
				if (oppositeFace >= packedFaceCount || oppositeFace == faceIndex)
				{
					continue;
				}

				const size_t oppositeTriBase = static_cast<size_t>(oppositeFace) * PACKED_TRIANGLE_STRIDE;
				const uint32_t edgeA = static_cast<uint32_t>(edge.mA.mParent.mParent);
				const uint32_t edgeB = static_cast<uint32_t>(edge.mB.mParent.mParent);

				uint32_t bestOppositeEdge = INVALID_EDGE_OR_FACE;
				bool foundReciprocal = false;

				for (int oppositeLocalEdge = 0; oppositeLocalEdge < 3; ++oppositeLocalEdge)
				{
					const uint32_t oppositeEdgeA = BIndices[oppositeTriBase + static_cast<size_t>(oppositeLocalEdge)];
					const uint32_t oppositeEdgeB = BIndices[oppositeTriBase + static_cast<size_t>((oppositeLocalEdge + 1) % 3)];
					if (oppositeEdgeA != edgeB || oppositeEdgeB != edgeA)
					{
						continue;
					}

					const uint32_t candidateOppositeEdge = oppositeFace * 3 + static_cast<uint32_t>(oppositeLocalEdge);
					const uint16_t packedBackFace = BIndices[oppositeTriBase + 3 + static_cast<size_t>(oppositeLocalEdge)];
					if (packedBackFace == faceIndex)
					{
						bestOppositeEdge = candidateOppositeEdge;
						foundReciprocal = true;
						break;
					}

					if (bestOppositeEdge == INVALID_EDGE_OR_FACE)
					{
						bestOppositeEdge = candidateOppositeEdge;
					}
				}

				if (bestOppositeEdge == INVALID_EDGE_OR_FACE)
				{
					continue;
				}

				auto& oppositeEdge = dst->mEdges.mElements[bestOppositeEdge];
				if (oppositeEdge.mOppositeEdge.mParent != INVALID_EDGE_OR_FACE &&
					oppositeEdge.mOppositeEdge.mParent != edgeIndex &&
					foundReciprocal)
				{
					continue;
				}

				edge.mOppositeEdge.mParent = bestOppositeEdge;
				edge.mOppositeFace.mParent = oppositeFace;
				oppositeEdge.mOppositeEdge.mParent = edgeIndex;
				oppositeEdge.mOppositeFace.mParent = faceIndex;
			}
		}

		struct DirectedEdgeKey
		{
			uint32_t mA = 0;
			uint32_t mB = 0;

			bool operator==(const DirectedEdgeKey& other) const
			{
				return mA == other.mA && mB == other.mB;
			}
		};

		struct DirectedEdgeKeyHash
		{
			size_t operator()(const DirectedEdgeKey& key) const
			{
				return (static_cast<size_t>(key.mA) << 32) ^ static_cast<size_t>(key.mB);
			}
		};

		std::unordered_map<DirectedEdgeKey, std::vector<uint32_t>, DirectedEdgeKeyHash> directedEdgeMap;
		directedEdgeMap.reserve(dst->mEdges.mElements.size() * 2);

		for (uint32_t edgeIndex = 0; edgeIndex < static_cast<uint32_t>(dst->mEdges.mElements.size()); ++edgeIndex)
		{
			const auto& edge = dst->mEdges.mElements[edgeIndex];
			const uint32_t a = static_cast<uint32_t>(edge.mA.mParent.mParent);
			const uint32_t b = static_cast<uint32_t>(edge.mB.mParent.mParent);
			if (a >= dst->mVertices.mElements.size() || b >= dst->mVertices.mElements.size() || a == b)
			{
				continue;
			}

			directedEdgeMap[DirectedEdgeKey{ a, b }].push_back(edgeIndex);
		}

		for (uint32_t edgeIndex = 0; edgeIndex < static_cast<uint32_t>(dst->mEdges.mElements.size()); ++edgeIndex)
		{
			auto& edge = dst->mEdges.mElements[edgeIndex];
			if (edge.mOppositeEdge.mParent != INVALID_EDGE_OR_FACE || edge.mOppositeFace.mParent != INVALID_EDGE_OR_FACE)
			{
				continue;
			}

			const uint32_t a = static_cast<uint32_t>(edge.mA.mParent.mParent);
			const uint32_t b = static_cast<uint32_t>(edge.mB.mParent.mParent);
			const int32_t faceIndex = edgeToFace[edgeIndex];

			auto reverseIter = directedEdgeMap.find(DirectedEdgeKey{ b, a });
			if (reverseIter == directedEdgeMap.end())
			{
				continue;
			}

			uint32_t chosenOpposite = INVALID_EDGE_OR_FACE;
			for (uint32_t candidate : reverseIter->second)
			{
				if (candidate == edgeIndex)
				{
					continue;
				}

				if (candidate >= edgeToFace.size() || edgeToFace[candidate] == faceIndex)
				{
					continue;
				}

				const auto& candidateEdge = dst->mEdges.mElements[candidate];
				if (candidateEdge.mOppositeEdge.mParent != INVALID_EDGE_OR_FACE &&
					candidateEdge.mOppositeEdge.mParent != edgeIndex)
				{
					continue;
				}

				chosenOpposite = candidate;
				break;
			}

			if (chosenOpposite == INVALID_EDGE_OR_FACE)
			{
				continue;
			}

			auto& oppositeEdge = dst->mEdges.mElements[chosenOpposite];
			const int32_t oppositeFaceIndex = edgeToFace[chosenOpposite];

			edge.mOppositeEdge.mParent = chosenOpposite;
			edge.mOppositeFace.mParent = static_cast<uint32_t>(oppositeFaceIndex);
			oppositeEdge.mOppositeEdge.mParent = edgeIndex;
			oppositeEdge.mOppositeFace.mParent = static_cast<uint32_t>(faceIndex);
		}

		for (uint32_t edgeIndex = 0; edgeIndex < static_cast<uint32_t>(dst->mEdges.mElements.size()); ++edgeIndex)
		{
			auto& edge = dst->mEdges.mElements[edgeIndex];
			const uint32_t oppositeEdgeIndex = edge.mOppositeEdge.mParent;
			const uint32_t oppositeFaceIndex = edge.mOppositeFace.mParent;
			const int32_t faceIndex = edgeToFace[edgeIndex];

			const bool hasOppositeEdge = oppositeEdgeIndex != INVALID_EDGE_OR_FACE;
			const bool hasOppositeFace = oppositeFaceIndex != INVALID_EDGE_OR_FACE;
			if (hasOppositeEdge != hasOppositeFace)
			{
				clearOppositeLink(edgeIndex);
				continue;
			}

			if (!hasOppositeEdge)
			{
				continue;
			}

			if (oppositeEdgeIndex >= dst->mEdges.mElements.size() ||
				oppositeFaceIndex >= dst->mFaces.mElements.size() ||
				edgeToFace[oppositeEdgeIndex] != static_cast<int32_t>(oppositeFaceIndex))
			{
				clearOppositeLink(edgeIndex);
				continue;
			}

			auto& oppositeEdge = dst->mEdges.mElements[oppositeEdgeIndex];
			const bool reversedVertices =
				edge.mA.mParent.mParent == oppositeEdge.mB.mParent.mParent &&
				edge.mB.mParent.mParent == oppositeEdge.mA.mParent.mParent;
			if (!reversedVertices)
			{
				clearOppositeLink(edgeIndex);
				continue;
			}

			oppositeEdge.mOppositeEdge.mParent = edgeIndex;
			oppositeEdge.mOppositeFace.mParent = static_cast<uint32_t>(faceIndex);
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
				if (edgeIdx < 0 || edgeIdx >= static_cast<int>(dst->mEdges.mElements.size()))
				{
					application::util::Logger::Error("PhiveNavMesh", "NavMesh generation failed: invalid edge index %d for face %u", edgeIdx, fIdx);
					return false;
				}
				const auto& edge = dst->mEdges.mElements[edgeIdx];

				int vertIdx = edge.mA.mParent.mParent;
				if (vertIdx < 0 || vertIdx >= static_cast<int>(dst->mVertices.mElements.size()))
				{
					application::util::Logger::Error("PhiveNavMesh", "NavMesh generation failed: invalid vertex index %d for edge %d", vertIdx, edgeIdx);
					return false;
				}
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
		const bool navMeshValid = ValidateBuiltNavMesh(*dst);
		if (!navMeshValid)
		{
			application::util::Logger::Error("PhiveNavMesh", "NavMesh validation failed");
			return false;
		}

		application::util::Logger::Info(
			"PhiveNavMesh",
			"NavMesh validation succeeded (%u faces, %u edges, %u vertices)",
			static_cast<uint32_t>(dst->mFaces.mElements.size()),
			static_cast<uint32_t>(dst->mEdges.mElements.size()),
			static_cast<uint32_t>(dst->mVertices.mElements.size()));

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

			const uint32_t numFaces = static_cast<uint32_t>(mesh.mFaces.mElements.size());
			if (numFaces == 0)
			{
				return;
			}

			constexpr uint32_t INVALID_PACKED_KEY = 0xFFFFFFFFu;
			const uint8_t blockedMask = static_cast<uint8_t>(classes::HavokClasses::hkaiNavMesh__EdgeFlagBits::EDGE_BLOCKED);
			const uint8_t externalMask = static_cast<uint8_t>(classes::HavokClasses::hkaiNavMesh__EdgeFlagBits::EDGE_EXTERNAL_OPPOSITE);

			auto computeFaceCentroidAndArea = [&mesh](int32_t faceIndex, glm::vec3& centroidOut, float& areaOut)
				{
					centroidOut = glm::vec3(0.0f);
					areaOut = 0.0f;

					if (faceIndex < 0 || faceIndex >= static_cast<int32_t>(mesh.mFaces.mElements.size()))
					{
						return;
					}

					const auto& face = mesh.mFaces.mElements[faceIndex];
					const int32_t startEdge = face.mStartEdgeIndex.mParent.mParent;
					const int32_t edgeCount = face.mNumEdges.mParent;
					if (edgeCount < 3)
					{
						return;
					}

					std::vector<glm::vec3> polygonVertices;
					polygonVertices.reserve(static_cast<size_t>(edgeCount));
					for (int32_t e = 0; e < edgeCount; ++e)
					{
						const int32_t edgeIndex = startEdge + e;
						if (edgeIndex < 0 || edgeIndex >= static_cast<int32_t>(mesh.mEdges.mElements.size()))
						{
							continue;
						}

						const int32_t vertexIndex = mesh.mEdges.mElements[edgeIndex].mA.mParent.mParent;
						if (vertexIndex < 0 || vertexIndex >= static_cast<int32_t>(mesh.mVertices.mElements.size()))
						{
							continue;
						}

						const glm::vec4 vertex = mesh.mVertices.mElements[vertexIndex].ToVec4();
						polygonVertices.emplace_back(vertex.x, vertex.y, vertex.z);
					}

					if (polygonVertices.size() < 3)
					{
						return;
					}

					const glm::vec3& v0 = polygonVertices[0];
					glm::vec3 weightedCentroid(0.0f);
					float weightedArea = 0.0f;
					for (size_t i = 1; i + 1 < polygonVertices.size(); ++i)
					{
						const glm::vec3& v1 = polygonVertices[i];
						const glm::vec3& v2 = polygonVertices[i + 1];

						const glm::vec3 cross = glm::cross(v1 - v0, v2 - v0);
						const float triArea = glm::length(cross) * 0.5f;
						if (triArea <= 1e-6f)
						{
							continue;
						}

						weightedCentroid += ((v0 + v1 + v2) / 3.0f) * triArea;
						weightedArea += triArea;
					}

					if (weightedArea <= 1e-6f)
					{
						for (const glm::vec3& v : polygonVertices)
						{
							centroidOut += v;
						}
						centroidOut /= static_cast<float>(polygonVertices.size());
						areaOut = 1.0f;
						return;
					}

					centroidOut = weightedCentroid / weightedArea;
					areaOut = weightedArea;
				};

			std::vector<glm::vec3> faceCentroids(numFaces, glm::vec3(0.0f));
			std::vector<float> faceAreas(numFaces, 1.0f);
			for (uint32_t faceIdx = 0; faceIdx < numFaces; ++faceIdx)
			{
				float area = 0.0f;
				glm::vec3 centroid(0.0f);
				computeFaceCentroidAndArea(static_cast<int32_t>(faceIdx), centroid, area);
				faceCentroids[faceIdx] = centroid;
				faceAreas[faceIdx] = area > 1e-6f ? area : 1.0f;
			}

			struct FaceTransition
			{
				int32_t mFromFace = -1;
				int32_t mToFace = -1;
			};

			std::vector<std::vector<int32_t>> undirectedAdjacency(numFaces);
			std::vector<FaceTransition> directedTransitions;
			directedTransitions.reserve(mesh.mEdges.mElements.size());

			for (uint32_t faceIdx = 0; faceIdx < numFaces; ++faceIdx)
			{
				const auto& face = mesh.mFaces.mElements[faceIdx];
				const int32_t startEdge = face.mStartEdgeIndex.mParent.mParent;
				const int32_t edgeCount = face.mNumEdges.mParent;

				undirectedAdjacency[faceIdx].reserve(static_cast<size_t>(edgeCount));

				for (int32_t e = 0; e < edgeCount; ++e)
				{
					const int32_t edgeIdx = startEdge + e;
					if (edgeIdx < 0 || edgeIdx >= static_cast<int32_t>(mesh.mEdges.mElements.size()))
					{
						continue;
					}

					const auto& edge = mesh.mEdges.mElements[edgeIdx];
					if (edge.mOppositeFace.mParent == INVALID_PACKED_KEY)
					{
						continue;
					}

					const uint8_t edgeFlags = edge.mFlags.mStorage.mParent;
					if ((edgeFlags & blockedMask) != 0)
					{
						continue;
					}

					if ((edgeFlags & externalMask) != 0)
					{
						continue;
					}

					const int32_t oppositeFace = static_cast<int32_t>(edge.mOppositeFace.mParent);
					if (oppositeFace < 0 || oppositeFace >= static_cast<int32_t>(numFaces) || oppositeFace == static_cast<int32_t>(faceIdx))
					{
						continue;
					}

					undirectedAdjacency[faceIdx].push_back(oppositeFace);
					undirectedAdjacency[static_cast<uint32_t>(oppositeFace)].push_back(static_cast<int32_t>(faceIdx));

					directedTransitions.push_back(FaceTransition{
						static_cast<int32_t>(faceIdx),
						oppositeFace });
				}
			}

			for (auto& neighbours : undirectedAdjacency)
			{
				std::sort(neighbours.begin(), neighbours.end());
				neighbours.erase(std::unique(neighbours.begin(), neighbours.end()), neighbours.end());
			}

			const int32_t requestedFacesPerCluster = MATH_MAX((numFaces > 1) ? 2 : 1, static_cast<int32_t>(desiredFaces));
			const int32_t targetFacesPerCluster = requestedFacesPerCluster;

			std::vector<int32_t> faceToCluster(numFaces, -1);
			int32_t numClusters = 0;

			for (uint32_t seedFace = 0; seedFace < numFaces; ++seedFace)
			{
				if (faceToCluster[seedFace] >= 0)
				{
					continue;
				}

				std::queue<int32_t> queue;
				queue.push(static_cast<int32_t>(seedFace));
				faceToCluster[seedFace] = numClusters;

				int32_t clusterFaceCount = 0;
				while (!queue.empty())
				{
					const int32_t faceIdx = queue.front();
					queue.pop();
					clusterFaceCount++;

					if (clusterFaceCount >= targetFacesPerCluster)
					{
						continue;
					}

					for (int32_t neighbour : undirectedAdjacency[static_cast<uint32_t>(faceIdx)])
					{
						if (faceToCluster[static_cast<uint32_t>(neighbour)] >= 0)
						{
							continue;
						}

						faceToCluster[static_cast<uint32_t>(neighbour)] = numClusters;
						queue.push(neighbour);
					}
				}

				numClusters++;
			}

			if (numClusters <= 0)
			{
				return;
			}

			// Merge singleton clusters into neighboring clusters when possible
			// to keep cluster graph granularity meaningful (more than one face).
			std::vector<int32_t> clusterFaceCounts(static_cast<size_t>(numClusters), 0);
			for (uint32_t faceIdx = 0; faceIdx < numFaces; ++faceIdx)
			{
				const int32_t clusterIndex = faceToCluster[faceIdx];
				if (clusterIndex >= 0 && clusterIndex < numClusters)
				{
					clusterFaceCounts[clusterIndex]++;
				}
			}

			if (numFaces > 1 && numClusters > 1)
			{
				for (uint32_t faceIdx = 0; faceIdx < numFaces; ++faceIdx)
				{
					const int32_t singletonCluster = faceToCluster[faceIdx];
					if (singletonCluster < 0 || singletonCluster >= numClusters || clusterFaceCounts[singletonCluster] != 1)
					{
						continue;
					}

					int32_t bestCluster = -1;
					int32_t bestSharedNeighbours = -1;
					float bestDistSq = std::numeric_limits<float>::max();

					for (int32_t neighbour : undirectedAdjacency[faceIdx])
					{
						if (neighbour < 0 || neighbour >= static_cast<int32_t>(numFaces))
						{
							continue;
						}

						const int32_t neighbourCluster = faceToCluster[static_cast<uint32_t>(neighbour)];
						if (neighbourCluster < 0 || neighbourCluster == singletonCluster)
						{
							continue;
						}

						int32_t sharedNeighbours = 0;
						for (int32_t neighbour2 : undirectedAdjacency[faceIdx])
						{
							if (neighbour2 < 0 || neighbour2 >= static_cast<int32_t>(numFaces))
							{
								continue;
							}

							if (faceToCluster[static_cast<uint32_t>(neighbour2)] == neighbourCluster)
							{
								sharedNeighbours++;
							}
						}

						const glm::vec3 d = faceCentroids[faceIdx] - faceCentroids[static_cast<uint32_t>(neighbour)];
						const float distSq = glm::dot(d, d);
						const bool betterCluster =
							(sharedNeighbours > bestSharedNeighbours) ||
							(sharedNeighbours == bestSharedNeighbours && distSq < bestDistSq);
						if (betterCluster)
						{
							bestCluster = neighbourCluster;
							bestSharedNeighbours = sharedNeighbours;
							bestDistSq = distSq;
						}
					}

					if (bestCluster < 0)
					{
						continue;
					}

					faceToCluster[faceIdx] = bestCluster;
					clusterFaceCounts[singletonCluster]--;
					clusterFaceCounts[bestCluster]++;
				}

				std::vector<int32_t> clusterRemap(static_cast<size_t>(numClusters), -1);
				int32_t compactedClusterCount = 0;
				for (int32_t clusterIndex = 0; clusterIndex < numClusters; ++clusterIndex)
				{
					if (clusterFaceCounts[clusterIndex] > 0)
					{
						clusterRemap[clusterIndex] = compactedClusterCount++;
					}
				}

				if (compactedClusterCount > 0 && compactedClusterCount != numClusters)
				{
					for (uint32_t faceIdx = 0; faceIdx < numFaces; ++faceIdx)
					{
						const int32_t clusterIndex = faceToCluster[faceIdx];
						if (clusterIndex < 0 || clusterIndex >= numClusters)
						{
							continue;
						}

						faceToCluster[faceIdx] = clusterRemap[clusterIndex];
					}

					numClusters = compactedClusterCount;
				}
			}

			std::vector<glm::vec3> clusterCentroidSums(static_cast<size_t>(numClusters), glm::vec3(0.0f));
			std::vector<float> clusterAreaSums(static_cast<size_t>(numClusters), 0.0f);

			for (uint32_t faceIdx = 0; faceIdx < numFaces; ++faceIdx)
			{
				const int32_t clusterIndex = faceToCluster[faceIdx];
				if (clusterIndex < 0)
				{
					continue;
				}

				clusterCentroidSums[clusterIndex] += faceCentroids[faceIdx] * faceAreas[faceIdx];
				clusterAreaSums[clusterIndex] += faceAreas[faceIdx];
			}

			graphOut.mPositions.mElements.resize(static_cast<size_t>(numClusters));
			std::vector<glm::vec3> clusterPositions(static_cast<size_t>(numClusters), glm::vec3(0.0f));
			for (int32_t clusterIndex = 0; clusterIndex < numClusters; ++clusterIndex)
			{
				if (clusterAreaSums[clusterIndex] > 1e-6f)
				{
					clusterPositions[clusterIndex] = clusterCentroidSums[clusterIndex] / clusterAreaSums[clusterIndex];
				}

				graphOut.mPositions.mElements[clusterIndex].FromVec4(glm::vec4(clusterPositions[clusterIndex], 1.0f));
			}

			auto makeDirectedKey = [](int32_t targetCluster, int32_t sourceCluster) -> uint64_t
				{
					return (static_cast<uint64_t>(static_cast<uint32_t>(targetCluster)) << 32) |
						static_cast<uint32_t>(sourceCluster);
				};

			std::unordered_map<uint64_t, float> directedClusterEdges;
			directedClusterEdges.reserve(directedTransitions.size());

			for (const FaceTransition& transition : directedTransitions)
			{
				if (transition.mFromFace < 0 || transition.mToFace < 0)
				{
					continue;
				}

				const int32_t clusterSource = faceToCluster[static_cast<uint32_t>(transition.mFromFace)];
				const int32_t clusterTarget = faceToCluster[static_cast<uint32_t>(transition.mToFace)];
				if (clusterSource < 0 || clusterTarget < 0 || clusterSource == clusterTarget)
				{
					continue;
				}

				const glm::vec3 delta = clusterPositions[clusterSource] - clusterPositions[clusterTarget];
				float connectionCost = std::sqrt(delta.x * delta.x + delta.z * delta.z);
				connectionCost *= 0.35f;
				if (!std::isfinite(connectionCost) || connectionCost <= 1e-3f)
				{
					connectionCost = 1e-3f;
				}
			const uint64_t key = makeDirectedKey(clusterTarget, clusterSource);
			auto iter = directedClusterEdges.find(key);
			if (iter == directedClusterEdges.end())
			{
				directedClusterEdges.emplace(key, connectionCost);
			}
			else if (connectionCost < iter->second)
			{
				iter->second = connectionCost;
			}
		}

		std::vector<std::vector<std::pair<int32_t, float>>> inboundEdges(static_cast<size_t>(numClusters));
		for (const auto& entry : directedClusterEdges)
		{
			const int32_t targetCluster = static_cast<int32_t>(entry.first >> 32);
			const int32_t sourceCluster = static_cast<int32_t>(entry.first & 0xFFFFFFFFu);
			if (targetCluster < 0 || sourceCluster < 0 || targetCluster >= numClusters || sourceCluster >= numClusters)
			{
				continue;
			}

			inboundEdges[static_cast<size_t>(targetCluster)].emplace_back(sourceCluster, entry.second);
		}

		for (auto& edges : inboundEdges)
		{
			std::sort(edges.begin(), edges.end(), [](const auto& lhs, const auto& rhs)
				{
					if (lhs.first != rhs.first)
					{
						return lhs.first < rhs.first;
					}
					return lhs.second < rhs.second;
				});
		}

		graphOut.mNodes.mElements.resize(static_cast<size_t>(numClusters));
		graphOut.mEdges.mElements.clear();
		for (int32_t clusterIndex = 0; clusterIndex < numClusters; ++clusterIndex)
		{
			auto& node = graphOut.mNodes.mElements[clusterIndex];
			node.mStartEdgeIndex = static_cast<int32_t>(graphOut.mEdges.mElements.size());
			node.mNumEdges = static_cast<int32_t>(inboundEdges[static_cast<size_t>(clusterIndex)].size());

			for (const auto& [sourceCluster, edgeCost] : inboundEdges[static_cast<size_t>(clusterIndex)])
			{
				classes::HavokClasses::hkaiClusterGraph__Edge edge;
				edge.mCost.mValue.mParent = application::util::Math::ConvertIntToHalf(std::bit_cast<int>(edgeCost));
				edge.mFlags.mStorage.mParent = 0;
				edge.mSource.mParent = static_cast<uint32_t>(sourceCluster);
				graphOut.mEdges.mElements.push_back(edge);
			}
		}

		graphOut.mFeatureToNodeIndex.mElements.resize(numFaces);
		for (uint32_t faceIdx = 0; faceIdx < numFaces; ++faceIdx)
		{
			graphOut.mFeatureToNodeIndex.mElements[faceIdx].mParent.mParent = faceToCluster[faceIdx];
		}

		const bool clusterGraphValid = ValidateBuiltClusterGraph(graphOut, numFaces);
		if (!clusterGraphValid)
		{
			application::util::Logger::Warning("PhiveNavMesh", "Cluster graph validation failed, falling back to conservative single-node graph");
			BuildConservativeClusterGraph(mesh, graphOut);
			const bool fallbackValid = ValidateBuiltClusterGraph(graphOut, numFaces);
			application::util::Logger::Warning(
				"PhiveNavMesh",
				"Cluster graph fallback validation: %s",
				fallbackValid ? "succeeded" : "failed");
			return;
		}

		application::util::Logger::Info(
			"PhiveNavMesh",
			"Cluster graph validation succeeded (%u nodes, %u edges, faces/cluster target %u)",
			static_cast<uint32_t>(graphOut.mNodes.mElements.size()),
			static_cast<uint32_t>(graphOut.mEdges.mElements.size()),
			static_cast<uint32_t>(targetFacesPerCluster));
	}
		}
