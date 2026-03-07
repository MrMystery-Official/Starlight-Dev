#include "PhiveNavMeshStreaming.h"

#include "PhiveNavMesh.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <unordered_map>
#include <unordered_set>

#include <util/Logger.h>
#include <util/Math.h>

namespace application::file::game::phive::navmesh
{
	namespace
	{
		namespace Havok = application::file::game::phive::classes::HavokClasses;

		constexpr uint32_t INVALID_PACKED_KEY = 0xFFFFFFFFu;
		constexpr uint32_t INVALID_SECTION_UID = std::numeric_limits<uint32_t>::max();

		struct BoundaryEdge
		{
			int32_t mFaceIndex = -1;
			int32_t mEdgeIndex = -1;
			glm::vec3 mALocal = glm::vec3(0.0f);
			glm::vec3 mBLocal = glm::vec3(0.0f);
			glm::vec3 mAWorld = glm::vec3(0.0f);
			glm::vec3 mBWorld = glm::vec3(0.0f);
		};

			struct MatchedEdgePair
			{
				int32_t mFaceA = -1;
				int32_t mEdgeA = -1;
				int32_t mFaceB = -1;
				int32_t mEdgeB = -1;
				glm::vec3 mALocalStart = glm::vec3(0.0f);
				glm::vec3 mALocalEnd = glm::vec3(0.0f);
				glm::vec3 mBLocalStart = glm::vec3(0.0f);
				glm::vec3 mBLocalEnd = glm::vec3(0.0f);
				float mStartTA = 0.0f;
				float mEndTA = 1.0f;
				float mStartTB = 0.0f;
				float mEndTB = 1.0f;
			};

			struct CandidateEdgePair
			{
				size_t mIndexA = 0;
				size_t mIndexB = 0;
				float mStartTA = 0.0f;
				float mEndTA = 1.0f;
				float mStartTB = 0.0f;
				float mEndTB = 1.0f;
				float mOverlapLength = 0.0f;
				float mScore = 0.0f;
				bool mIsExact = false;
			};

		bool IsBoundaryEdge(const Havok::hkaiNavMesh__Edge& Edge)
		{
			return Edge.mOppositeEdge.mParent == INVALID_PACKED_KEY || Edge.mOppositeFace.mParent == INVALID_PACKED_KEY;
		}

		glm::vec3 ReadVertex(const Havok::hkaiNavMesh& Mesh, int32_t VertexIndex)
		{
			const glm::vec4 Vertex = Mesh.mVertices.mElements[VertexIndex].ToVec4();
			return glm::vec3(Vertex.x, Vertex.y, Vertex.z);
		}

		std::vector<BoundaryEdge> CollectBoundaryEdges(const Havok::hkaiNavMesh& Mesh, const glm::vec3& WorldOffset)
		{
			std::vector<BoundaryEdge> Result;
			Result.reserve(Mesh.mEdges.mElements.size());

			for (int32_t FaceIndex = 0; FaceIndex < static_cast<int32_t>(Mesh.mFaces.mElements.size()); ++FaceIndex)
			{
				const Havok::hkaiNavMesh__Face& Face = Mesh.mFaces.mElements[FaceIndex];
				const int32_t StartEdge = Face.mStartEdgeIndex.mParent.mParent;
				const int32_t EdgeCount = Face.mNumEdges.mParent;

				for (int32_t EdgeOffset = 0; EdgeOffset < EdgeCount; ++EdgeOffset)
				{
					const int32_t EdgeIndex = StartEdge + EdgeOffset;
					if (EdgeIndex < 0 || EdgeIndex >= static_cast<int32_t>(Mesh.mEdges.mElements.size()))
					{
						continue;
					}

					const Havok::hkaiNavMesh__Edge& Edge = Mesh.mEdges.mElements[EdgeIndex];
					if (!IsBoundaryEdge(Edge))
					{
						continue;
					}

					if (Edge.mA.mParent.mParent < 0 || Edge.mA.mParent.mParent >= static_cast<int32_t>(Mesh.mVertices.mElements.size()) ||
						Edge.mB.mParent.mParent < 0 || Edge.mB.mParent.mParent >= static_cast<int32_t>(Mesh.mVertices.mElements.size()))
					{
						continue;
					}

					BoundaryEdge Boundary;
					Boundary.mFaceIndex = FaceIndex;
					Boundary.mEdgeIndex = EdgeIndex;
					Boundary.mALocal = ReadVertex(Mesh, Edge.mA.mParent.mParent);
					Boundary.mBLocal = ReadVertex(Mesh, Edge.mB.mParent.mParent);
					Boundary.mAWorld = Boundary.mALocal + WorldOffset;
					Boundary.mBWorld = Boundary.mBLocal + WorldOffset;

					Result.push_back(Boundary);
				}
			}

			return Result;
		}

		struct BucketKey
		{
			int32_t mX = 0;
			int32_t mY = 0;
			int32_t mZ = 0;

			bool operator==(const BucketKey& other) const
			{
				return mX == other.mX && mY == other.mY && mZ == other.mZ;
			}
		};

		struct BucketKeyHash
		{
			size_t operator()(const BucketKey& key) const
			{
				const uint64_t x = static_cast<uint32_t>(key.mX);
				const uint64_t y = static_cast<uint32_t>(key.mY);
				const uint64_t z = static_cast<uint32_t>(key.mZ);
				return static_cast<size_t>((x * 73856093ull) ^ (y * 19349663ull) ^ (z * 83492791ull));
			}
		};

		BucketKey GetBucketKey(const BoundaryEdge& edge, float invBucketSize)
		{
			const glm::vec3 mid = (edge.mAWorld + edge.mBWorld) * 0.5f;
			return BucketKey{
				static_cast<int32_t>(std::floor(mid.x * invBucketSize)),
				static_cast<int32_t>(std::floor(mid.y * invBucketSize)),
				static_cast<int32_t>(std::floor(mid.z * invBucketSize)),
			};
		}

		std::unordered_map<BucketKey, std::vector<size_t>, BucketKeyHash> BuildBoundaryBuckets(
			const std::vector<BoundaryEdge>& edges,
			float bucketSize)
		{
			const float safeBucket = std::max(1e-3f, bucketSize);
			const float invBucket = 1.0f / safeBucket;

			std::unordered_map<BucketKey, std::vector<size_t>, BucketKeyHash> buckets;
			buckets.reserve(edges.size());

			for (size_t i = 0; i < edges.size(); ++i)
			{
				buckets[GetBucketKey(edges[i], invBucket)].push_back(i);
			}

			return buckets;
		}

		bool IsEdgeMatch(const BoundaryEdge& EdgeA, const BoundaryEdge& EdgeB, float Tolerance)
		{
			const float LengthA = glm::length(EdgeA.mAWorld - EdgeA.mBWorld);
			const float LengthB = glm::length(EdgeB.mAWorld - EdgeB.mBWorld);
			if (LengthA <= Tolerance || LengthB <= Tolerance)
			{
				return false;
			}

			if (std::fabs(LengthA - LengthB) > Tolerance * 2.0f)
			{
				return false;
			}

			const bool EndpointsOppositeAligned =
				glm::distance(EdgeA.mAWorld, EdgeB.mBWorld) <= Tolerance &&
				glm::distance(EdgeA.mBWorld, EdgeB.mAWorld) <= Tolerance;
			const bool EndpointsSameAligned =
				glm::distance(EdgeA.mAWorld, EdgeB.mAWorld) <= Tolerance &&
				glm::distance(EdgeA.mBWorld, EdgeB.mBWorld) <= Tolerance;

			return EndpointsOppositeAligned || EndpointsSameAligned;
		}

		float DistancePointToSegmentSquared(const glm::vec3& Point, const glm::vec3& SegmentStart, const glm::vec3& SegmentEnd)
		{
			const glm::vec3 Segment = SegmentEnd - SegmentStart;
			const float SegmentLengthSquared = glm::dot(Segment, Segment);
			if (SegmentLengthSquared <= 1e-12f)
			{
				return glm::dot(Point - SegmentStart, Point - SegmentStart);
			}

			const float Projection = glm::clamp(glm::dot(Point - SegmentStart, Segment) / SegmentLengthSquared, 0.0f, 1.0f);
			const glm::vec3 ClosestPoint = SegmentStart + Segment * Projection;
			const glm::vec3 Delta = Point - ClosestPoint;
			return glm::dot(Delta, Delta);
		}

			bool BuildConnectivityCandidate(const BoundaryEdge& EdgeA, const BoundaryEdge& EdgeB, float Tolerance, CandidateEdgePair& CandidateOut)
			{
				const glm::vec3 SegmentA = EdgeA.mBWorld - EdgeA.mAWorld;
				const glm::vec3 SegmentB = EdgeB.mBWorld - EdgeB.mAWorld;

				const float LengthA = glm::length(SegmentA);
				const float LengthB = glm::length(SegmentB);
				if (LengthA <= Tolerance || LengthB <= Tolerance)
				{
					return false;
				}

				const glm::vec3 DirectionA = SegmentA / LengthA;
				const glm::vec3 DirectionB = SegmentB / LengthB;
				if (std::fabs(glm::dot(DirectionA, DirectionB)) < 0.90f)
				{
					return false;
				}

				const float MaxEndpointDistanceSquared = (Tolerance * 3.0f) * (Tolerance * 3.0f);
				const float EndpointDistanceA0 = DistancePointToSegmentSquared(EdgeA.mAWorld, EdgeB.mAWorld, EdgeB.mBWorld);
				const float EndpointDistanceA1 = DistancePointToSegmentSquared(EdgeA.mBWorld, EdgeB.mAWorld, EdgeB.mBWorld);
				const float EndpointDistanceB0 = DistancePointToSegmentSquared(EdgeB.mAWorld, EdgeA.mAWorld, EdgeA.mBWorld);
				const float EndpointDistanceB1 = DistancePointToSegmentSquared(EdgeB.mBWorld, EdgeA.mAWorld, EdgeA.mBWorld);

				if (EndpointDistanceA0 > MaxEndpointDistanceSquared &&
					EndpointDistanceA1 > MaxEndpointDistanceSquared &&
					EndpointDistanceB0 > MaxEndpointDistanceSquared &&
					EndpointDistanceB1 > MaxEndpointDistanceSquared)
				{
					return false;
				}

				const float ProjectionB0OnA = glm::dot(EdgeB.mAWorld - EdgeA.mAWorld, DirectionA) / LengthA;
				const float ProjectionB1OnA = glm::dot(EdgeB.mBWorld - EdgeA.mAWorld, DirectionA) / LengthA;
				const float ProjectionA0OnB = glm::dot(EdgeA.mAWorld - EdgeB.mAWorld, DirectionB) / LengthB;
				const float ProjectionA1OnB = glm::dot(EdgeA.mBWorld - EdgeB.mAWorld, DirectionB) / LengthB;

				const float StartTA = glm::clamp(std::min(ProjectionB0OnA, ProjectionB1OnA), 0.0f, 1.0f);
				const float EndTA = glm::clamp(std::max(ProjectionB0OnA, ProjectionB1OnA), 0.0f, 1.0f);
				const float StartTB = glm::clamp(std::min(ProjectionA0OnB, ProjectionA1OnB), 0.0f, 1.0f);
				const float EndTB = glm::clamp(std::max(ProjectionA0OnB, ProjectionA1OnB), 0.0f, 1.0f);

				if (EndTA - StartTA <= 1e-4f || EndTB - StartTB <= 1e-4f)
				{
					return false;
				}

				const float OverlapLengthA = (EndTA - StartTA) * LengthA;
				const float OverlapLengthB = (EndTB - StartTB) * LengthB;
				const float OverlapLength = std::min(OverlapLengthA, OverlapLengthB);
				const float RequiredOverlap = std::max(Tolerance * 1.5f, std::min(LengthA, LengthB) * 0.10f);
				if (OverlapLength < RequiredOverlap)
				{
					return false;
				}

				const glm::vec3 AStart = glm::mix(EdgeA.mAWorld, EdgeA.mBWorld, StartTA);
				const glm::vec3 AEnd = glm::mix(EdgeA.mAWorld, EdgeA.mBWorld, EndTA);
				const glm::vec3 BStart = glm::mix(EdgeB.mAWorld, EdgeB.mBWorld, StartTB);
				const glm::vec3 BEnd = glm::mix(EdgeB.mAWorld, EdgeB.mBWorld, EndTB);

				const float EndpointCostForward = glm::dot(AStart - BStart, AStart - BStart) + glm::dot(AEnd - BEnd, AEnd - BEnd);
				const float EndpointCostReverse = glm::dot(AStart - BEnd, AStart - BEnd) + glm::dot(AEnd - BStart, AEnd - BStart);
				const float EndpointCost = std::min(EndpointCostForward, EndpointCostReverse);

				const float MaximumEndpointCost = (Tolerance * 4.0f) * (Tolerance * 4.0f) * 2.0f;
				if (EndpointCost > MaximumEndpointCost)
				{
					return false;
				}

				const float LengthDelta = std::fabs(LengthA - LengthB);
				CandidateOut.mStartTA = StartTA;
				CandidateOut.mEndTA = EndTA;
				CandidateOut.mStartTB = StartTB;
				CandidateOut.mEndTB = EndTB;
				CandidateOut.mOverlapLength = OverlapLength;
				CandidateOut.mScore = EndpointCost + LengthDelta * 0.25f - OverlapLength * 0.05f;
				return true;
			}

			float ComputeEdgeScore(const BoundaryEdge& EdgeA, const BoundaryEdge& EdgeB)
			{
				const glm::vec3 DeltaA = EdgeA.mAWorld - EdgeB.mBWorld;
				const glm::vec3 DeltaB = EdgeA.mBWorld - EdgeB.mAWorld;
				const float DeltaAMagSq = DeltaA.x * DeltaA.x + DeltaA.y * DeltaA.y + DeltaA.z * DeltaA.z;
				const float DeltaBMagSq = DeltaB.x * DeltaB.x + DeltaB.y * DeltaB.y + DeltaB.z * DeltaB.z;
				return DeltaAMagSq + DeltaBMagSq;
			}

			bool IntervalsOverlap(float StartA, float EndA, float StartB, float EndB, float Threshold)
			{
				const float Overlap = std::min(EndA, EndB) - std::max(StartA, StartB);
				return Overlap > Threshold;
			}

			MatchedEdgePair BuildMatchedEdgePair(
				const BoundaryEdge& EdgeA,
				const BoundaryEdge& EdgeB,
				const CandidateEdgePair& Candidate)
			{
				MatchedEdgePair Match;
				Match.mFaceA = EdgeA.mFaceIndex;
				Match.mEdgeA = EdgeA.mEdgeIndex;
				Match.mFaceB = EdgeB.mFaceIndex;
				Match.mEdgeB = EdgeB.mEdgeIndex;

				Match.mStartTA = Candidate.mStartTA;
				Match.mEndTA = Candidate.mEndTA;
				Match.mStartTB = Candidate.mStartTB;
				Match.mEndTB = Candidate.mEndTB;

				Match.mALocalStart = glm::mix(EdgeA.mALocal, EdgeA.mBLocal, Candidate.mStartTA);
				Match.mALocalEnd = glm::mix(EdgeA.mALocal, EdgeA.mBLocal, Candidate.mEndTA);
				Match.mBLocalStart = glm::mix(EdgeB.mALocal, EdgeB.mBLocal, Candidate.mStartTB);
				Match.mBLocalEnd = glm::mix(EdgeB.mALocal, EdgeB.mBLocal, Candidate.mEndTB);

				return Match;
			}

		Havok::hkAabb BuildEdgeAabb(const glm::vec3& PointA, const glm::vec3& PointB)
		{
			Havok::hkAabb Aabb;
			const glm::vec3 Min = glm::min(PointA, PointB);
			const glm::vec3 Max = glm::max(PointA, PointB);
			Aabb.mMin.FromVec4(glm::vec4(Min, 1.0f));
			Aabb.mMax.FromVec4(glm::vec4(Max, 1.0f));
			return Aabb;
		}

		int32_t GetNodeForFace(const Havok::hkaiClusterGraph& Graph, int32_t FaceIndex)
		{
			if (FaceIndex < 0 || FaceIndex >= static_cast<int32_t>(Graph.mFeatureToNodeIndex.mElements.size()))
			{
				return -1;
			}

			const int32_t NodeIndex = Graph.mFeatureToNodeIndex.mElements[FaceIndex].mParent.mParent;
			if (NodeIndex < 0 || NodeIndex >= static_cast<int32_t>(Graph.mNodes.mElements.size()))
			{
				return -1;
			}

			return NodeIndex;
		}

		float ComputeNodeDistance(
			const Havok::hkaiClusterGraph& GraphA,
			const Havok::hkaiClusterGraph& GraphB,
			int32_t NodeA,
			int32_t NodeB,
			const glm::vec3& NavMeshBOffsetFromA)
		{
			const glm::vec3 PositionA = glm::vec3(GraphA.mPositions.mElements[NodeA].ToVec4());
			const glm::vec3 PositionB = glm::vec3(GraphB.mPositions.mElements[NodeB].ToVec4()) + NavMeshBOffsetFromA;
			const float dx = PositionA.x - PositionB.x;
			const float dz = PositionA.z - PositionB.z;
			float cost = std::sqrt(dx * dx + dz * dz);
			cost *= 0.35f;
			if (!std::isfinite(cost) || cost <= 1e-3f)
			{
				cost = 1e-3f;
			}
			return cost;
		}

		void ResetGraphConnectionMap(Havok::hkHashMap<Havok::hkUint32, Havok::hkArray<Havok::hkaiStreamingSet__ClusterGraphConnection>>& GraphConnections)
		{
			GraphConnections.mParent.mItems.mElements.clear();
			GraphConnections.mParent.mIndex.mEntries.mParent = 0;
			GraphConnections.mParent.mIndex.mHashMod = 0;
		}

		void CollectClusteringSetupIdentifiers(
			const Havok::hkaiClusterGraph& GraphA,
			const Havok::hkaiClusterGraph& GraphB,
			uint32_t FallbackIdentifier,
			std::vector<uint32_t>& IdentifiersOut)
		{
			IdentifiersOut.clear();

			auto AddIdentifier = [&IdentifiersOut](uint32_t Identifier)
				{
					if (std::find(IdentifiersOut.begin(), IdentifiersOut.end(), Identifier) == IdentifiersOut.end())
					{
						IdentifiersOut.push_back(Identifier);
					}
				};

			auto CollectFromGraph = [&AddIdentifier](const Havok::hkaiClusterGraph& Graph)
				{
					for (const Havok::hkaiAnnotatedStreamingSet& Annotated : Graph.mStreamingSets.mElements)
					{
						if (Annotated.mStreamingSet.mIsNullPtr)
						{
							continue;
						}

						const auto& GraphItems = Annotated.mStreamingSet.mObj.mGraphConnections.mParent.mItems.mElements;
						for (const auto& Item : GraphItems)
						{
							AddIdentifier(Item.m0.mParent);
						}
					}
				};

			CollectFromGraph(GraphA);
			CollectFromGraph(GraphB);
			if (IdentifiersOut.empty())
			{
				IdentifiersOut.push_back(FallbackIdentifier);
			}
		}

		Havok::hkaiStreamingSet BuildMeshStreamingSet(
			uint32_t UidA,
			uint32_t UidB,
			const std::vector<MatchedEdgePair>& Matches,
			bool SwapSides)
		{
			Havok::hkaiStreamingSet Set;
			Set.mASectionUid.mParent = UidA;
			Set.mBSectionUid.mParent = UidB;
			Set.mMeshConnections.mElements.reserve(Matches.size());
			Set.mAConnectionAabbs.mElements.reserve(Matches.size());
			Set.mBConnectionAabbs.mElements.reserve(Matches.size());

			for (const MatchedEdgePair& Match : Matches)
			{
				Havok::hkaiStreamingSet__NavMeshConnection Connection;
				if (!SwapSides)
				{
					Connection.mAFaceEdgeIndex.mFaceIndex.mParent.mParent = Match.mFaceA;
					Connection.mAFaceEdgeIndex.mEdgeIndex.mParent.mParent = Match.mEdgeA;
					Connection.mBFaceEdgeIndex.mFaceIndex.mParent.mParent = Match.mFaceB;
					Connection.mBFaceEdgeIndex.mEdgeIndex.mParent.mParent = Match.mEdgeB;

					Set.mAConnectionAabbs.mElements.push_back(BuildEdgeAabb(Match.mALocalStart, Match.mALocalEnd));
					Set.mBConnectionAabbs.mElements.push_back(BuildEdgeAabb(Match.mBLocalStart, Match.mBLocalEnd));
				}
				else
				{
					Connection.mAFaceEdgeIndex.mFaceIndex.mParent.mParent = Match.mFaceB;
					Connection.mAFaceEdgeIndex.mEdgeIndex.mParent.mParent = Match.mEdgeB;
					Connection.mBFaceEdgeIndex.mFaceIndex.mParent.mParent = Match.mFaceA;
					Connection.mBFaceEdgeIndex.mEdgeIndex.mParent.mParent = Match.mEdgeA;

					Set.mAConnectionAabbs.mElements.push_back(BuildEdgeAabb(Match.mBLocalStart, Match.mBLocalEnd));
					Set.mBConnectionAabbs.mElements.push_back(BuildEdgeAabb(Match.mALocalStart, Match.mALocalEnd));
				}

				Set.mMeshConnections.mElements.push_back(Connection);
			}

			Set.mVolumeConnections.mElements.clear();
			ResetGraphConnectionMap(Set.mGraphConnections);
			return Set;
		}

		Havok::hkaiStreamingSet BuildGraphStreamingSet(
			uint32_t UidA,
			uint32_t UidB,
			const std::vector<MatchedEdgePair>& Matches,
			const Havok::hkaiClusterGraph& GraphA,
			const Havok::hkaiClusterGraph& GraphB,
			const glm::vec3& NavMeshBOffsetFromA,
			bool SwapSides,
			const std::vector<uint32_t>& ClusteringSetupIdentifiers,
			uint32_t& GraphConnectionCountOut)
		{
			Havok::hkaiStreamingSet Set;
			Set.mASectionUid.mParent = UidA;
			Set.mBSectionUid.mParent = UidB;
			Set.mMeshConnections.mElements.clear();
			Set.mAConnectionAabbs.mElements.clear();
			Set.mBConnectionAabbs.mElements.clear();
			Set.mVolumeConnections.mElements.clear();

			std::vector<Havok::hkaiStreamingSet__ClusterGraphConnection> GraphConnections;
			GraphConnections.reserve(Matches.size());

			std::unordered_set<uint64_t> SeenNodePairs;
			SeenNodePairs.reserve(Matches.size());

			for (const MatchedEdgePair& Match : Matches)
			{
				const int32_t NodeA = GetNodeForFace(GraphA, Match.mFaceA);
				const int32_t NodeB = GetNodeForFace(GraphB, Match.mFaceB);
				if (NodeA < 0 || NodeB < 0)
				{
					continue;
				}

				const int32_t OutNodeA = SwapSides ? NodeB : NodeA;
				const int32_t OutNodeB = SwapSides ? NodeA : NodeB;
				const uint64_t DedupKey = (static_cast<uint64_t>(static_cast<uint32_t>(OutNodeA)) << 32) | static_cast<uint32_t>(OutNodeB);
				if (!SeenNodePairs.insert(DedupKey).second)
				{
					continue;
				}

				const float Cost = ComputeNodeDistance(GraphA, GraphB, NodeA, NodeB, NavMeshBOffsetFromA);
				const uint16_t HalfCost = application::util::Math::ConvertIntToHalf(std::bit_cast<int>(Cost));

				Havok::hkaiStreamingSet__ClusterGraphConnection Connection;
				Connection.mANodeIndex = OutNodeA;
				Connection.mBNodeIndex = OutNodeB;
				Connection.mAEdgeData.mParent = 0;
				Connection.mBEdgeData.mParent = 0;
				Connection.mAEdgeCost.mValue.mParent = HalfCost;
				Connection.mBEdgeCost.mValue.mParent = HalfCost;
				GraphConnections.push_back(Connection);
			}

			std::sort(GraphConnections.begin(), GraphConnections.end(), [](const Havok::hkaiStreamingSet__ClusterGraphConnection& lhs, const Havok::hkaiStreamingSet__ClusterGraphConnection& rhs)
				{
					if (lhs.mANodeIndex != rhs.mANodeIndex)
					{
						return lhs.mANodeIndex < rhs.mANodeIndex;
					}
					return lhs.mBNodeIndex < rhs.mBNodeIndex;
				});

			ResetGraphConnectionMap(Set.mGraphConnections);
			if (!GraphConnections.empty())
			{
				for (uint32_t ClusteringSetupIdentifier : ClusteringSetupIdentifiers)
				{
					Havok::hkPair<Havok::hkUint32, Havok::hkArray<Havok::hkaiStreamingSet__ClusterGraphConnection>> Pair;
					Pair.m0.mParent = ClusteringSetupIdentifier;
					Pair.m0x31.mElements = GraphConnections;
					Set.mGraphConnections.mParent.mItems.mElements.push_back(std::move(Pair));
				}
			}

			GraphConnectionCountOut = static_cast<uint32_t>(GraphConnections.size());

			return Set;
		}

		Havok::hkaiAnnotatedStreamingSet MakeAnnotatedStreamingSet(const Havok::hkaiStreamingSet& Set, Havok::hkaiAnnotatedStreamingSet__Side Side)
		{
			Havok::hkaiAnnotatedStreamingSet Annotated;
			Annotated.mSide.mEnumValue = Side;
			Annotated.mStreamingSet.mIsNullPtr = false;
			Annotated.mStreamingSet.mObj = Set;
			return Annotated;
		}

		void RemovePairSets(std::vector<Havok::hkaiAnnotatedStreamingSet>& StreamingSets, uint32_t UidA, uint32_t UidB)
		{
			StreamingSets.erase(std::remove_if(StreamingSets.begin(), StreamingSets.end(), [UidA, UidB](const Havok::hkaiAnnotatedStreamingSet& Annotated)
				{
					if (Annotated.mStreamingSet.mIsNullPtr)
					{
						return false;
					}

					const uint32_t StreamUidA = Annotated.mStreamingSet.mObj.mASectionUid.mParent;
					const uint32_t StreamUidB = Annotated.mStreamingSet.mObj.mBSectionUid.mParent;
					return (StreamUidA == UidA && StreamUidB == UidB) || (StreamUidA == UidB && StreamUidB == UidA);
				}),
				StreamingSets.end());
		}
	}

	PhiveNavMeshStreaming::BuildResult PhiveNavMeshStreaming::RebuildPair(
		PhiveNavMesh& NavMeshA,
		PhiveNavMesh& NavMeshB,
		const glm::vec3& NavMeshBOffsetFromA,
		float EdgeMatchTolerance,
		uint32_t ClusteringSetupIdentifier)
	{
		BuildResult Result;

		auto& TagFileA = NavMeshA.GetTagFile();
		auto& TagFileB = NavMeshB.GetTagFile();
		auto& MeshA = TagFileA.mRootClass.mNavMeshVariant.mVariant.mObj;
		auto& MeshB = TagFileB.mRootClass.mNavMeshVariant.mVariant.mObj;
		auto& GraphA = TagFileA.mRootClass.mClusterGraphVariant.mVariant.mObj;
		auto& GraphB = TagFileB.mRootClass.mClusterGraphVariant.mVariant.mObj;

		std::vector<uint32_t> ClusteringSetupIdentifiers;
		CollectClusteringSetupIdentifiers(GraphA, GraphB, ClusteringSetupIdentifier, ClusteringSetupIdentifiers);

		const uint32_t UidA = NavMeshA.mReferenceRotation.mSectionUid;
		const uint32_t UidB = NavMeshB.mReferenceRotation.mSectionUid;
		if (UidA == INVALID_SECTION_UID || UidB == INVALID_SECTION_UID)
		{
			application::util::Logger::Warning("PhiveNavMeshStreaming", "Skipping stitched NavMesh generation due to invalid section uid (%u, %u)", UidA, UidB);
			return Result;
		}

		RemovePairSets(MeshA.mStreamingSets.mElements, UidA, UidB);
		RemovePairSets(MeshB.mStreamingSets.mElements, UidA, UidB);
		RemovePairSets(GraphA.mStreamingSets.mElements, UidA, UidB);
		RemovePairSets(GraphB.mStreamingSets.mElements, UidA, UidB);

		const std::vector<BoundaryEdge> BoundaryEdgesA = CollectBoundaryEdges(MeshA, glm::vec3(0.0f));
		const std::vector<BoundaryEdge> BoundaryEdgesB = CollectBoundaryEdges(MeshB, NavMeshBOffsetFromA);

		if (BoundaryEdgesA.empty() || BoundaryEdgesB.empty())
		{
			application::util::Logger::Warning(
				"PhiveNavMeshStreaming",
				"Skipping pair (%u <-> %u): no boundary edges found (A=%u, B=%u)",
				UidA,
				UidB,
				static_cast<uint32_t>(BoundaryEdgesA.size()),
				static_cast<uint32_t>(BoundaryEdgesB.size()));
			return Result;
		}

			std::vector<CandidateEdgePair> CandidatePairs;
			CandidatePairs.reserve(BoundaryEdgesA.size() + BoundaryEdgesB.size());
			std::vector<bool> ExactMatchedA(BoundaryEdgesA.size(), false);
			std::vector<bool> ExactMatchedB(BoundaryEdgesB.size(), false);
			uint32_t ExactCandidateCount = 0;
			uint32_t RobustCandidateCount = 0;

			const float bucketSize = std::max(EdgeMatchTolerance * 6.0f, 0.35f);
			const auto BucketsB = BuildBoundaryBuckets(BoundaryEdgesB, bucketSize);
			const float invBucket = 1.0f / std::max(1e-3f, bucketSize);

			std::vector<uint32_t> bucketMarks(BoundaryEdgesB.size(), 0);
			uint32_t bucketPassId = 1;
			std::vector<size_t> nearbyBIndices;
			nearbyBIndices.reserve(64);

			auto gatherNearbyBEdges = [&](const BoundaryEdge& edgeA)
				{
					nearbyBIndices.clear();
					if (BoundaryEdgesB.empty())
					{
						return;
					}

					++bucketPassId;
					if (bucketPassId == 0)
					{
						std::fill(bucketMarks.begin(), bucketMarks.end(), 0);
						bucketPassId = 1;
					}

					constexpr int kSearchRadius = 2;
					const BucketKey baseKey = GetBucketKey(edgeA, invBucket);
					for (int dx = -kSearchRadius; dx <= kSearchRadius; ++dx)
					{
						for (int dy = -kSearchRadius; dy <= kSearchRadius; ++dy)
						{
							for (int dz = -kSearchRadius; dz <= kSearchRadius; ++dz)
							{
								const BucketKey queryKey{
									baseKey.mX + dx,
									baseKey.mY + dy,
									baseKey.mZ + dz,
								};

								auto bucketIt = BucketsB.find(queryKey);
								if (bucketIt == BucketsB.end())
								{
									continue;
								}

								for (size_t indexB : bucketIt->second)
								{
									if (indexB >= bucketMarks.size() || bucketMarks[indexB] == bucketPassId)
									{
										continue;
									}

									bucketMarks[indexB] = bucketPassId;
									nearbyBIndices.push_back(indexB);
								}
							}
						}
					}
				};

			for (size_t IndexA = 0; IndexA < BoundaryEdgesA.size(); ++IndexA)
			{
				const BoundaryEdge& EdgeA = BoundaryEdgesA[IndexA];
				gatherNearbyBEdges(EdgeA);

				int32_t BestIndexB = -1;
				float BestScore = std::numeric_limits<float>::max();

				const bool useFallbackScan = nearbyBIndices.empty();
				const size_t candidateCount = useFallbackScan ? BoundaryEdgesB.size() : nearbyBIndices.size();
				for (size_t i = 0; i < candidateCount; ++i)
				{
					const size_t IndexB = useFallbackScan ? i : nearbyBIndices[i];
					if (IndexB >= BoundaryEdgesB.size() || ExactMatchedB[IndexB])
					{
						continue;
					}

					const BoundaryEdge& EdgeB = BoundaryEdgesB[IndexB];
					if (!IsEdgeMatch(EdgeA, EdgeB, EdgeMatchTolerance))
					{
						continue;
					}

					const float Score = ComputeEdgeScore(EdgeA, EdgeB);
					if (Score < BestScore)
					{
						BestScore = Score;
						BestIndexB = static_cast<int32_t>(IndexB);
					}
				}

				if (BestIndexB < 0)
				{
					continue;
				}

				ExactMatchedA[IndexA] = true;
				ExactMatchedB[static_cast<size_t>(BestIndexB)] = true;

				CandidateEdgePair Candidate;
				Candidate.mIndexA = IndexA;
				Candidate.mIndexB = static_cast<size_t>(BestIndexB);
				Candidate.mStartTA = 0.0f;
				Candidate.mEndTA = 1.0f;
				Candidate.mStartTB = 0.0f;
				Candidate.mEndTB = 1.0f;
				Candidate.mOverlapLength = std::min(
					glm::length(EdgeA.mBWorld - EdgeA.mAWorld),
					glm::length(BoundaryEdgesB[static_cast<size_t>(BestIndexB)].mBWorld - BoundaryEdgesB[static_cast<size_t>(BestIndexB)].mAWorld));
				Candidate.mScore = BestScore;
				Candidate.mIsExact = true;
				CandidatePairs.push_back(Candidate);
				ExactCandidateCount++;
			}

			for (size_t IndexA = 0; IndexA < BoundaryEdgesA.size(); ++IndexA)
			{
				if (ExactMatchedA[IndexA])
				{
					continue;
				}

				gatherNearbyBEdges(BoundaryEdgesA[IndexA]);
				const bool useFallbackScan = nearbyBIndices.empty();
				const size_t candidateCount = useFallbackScan ? BoundaryEdgesB.size() : nearbyBIndices.size();

				for (size_t i = 0; i < candidateCount; ++i)
				{
					const size_t IndexB = useFallbackScan ? i : nearbyBIndices[i];
					if (IndexB >= BoundaryEdgesB.size() || ExactMatchedB[IndexB])
					{
						continue;
					}

					CandidateEdgePair Candidate;
					if (!BuildConnectivityCandidate(BoundaryEdgesA[IndexA], BoundaryEdgesB[IndexB], EdgeMatchTolerance, Candidate))
					{
						continue;
					}

					Candidate.mIndexA = IndexA;
					Candidate.mIndexB = IndexB;
					Candidate.mIsExact = false;
					CandidatePairs.push_back(Candidate);
					RobustCandidateCount++;
				}
			}

			std::sort(CandidatePairs.begin(), CandidatePairs.end(), [](const CandidateEdgePair& Lhs, const CandidateEdgePair& Rhs)
				{
					if (Lhs.mIsExact != Rhs.mIsExact)
					{
						return Lhs.mIsExact > Rhs.mIsExact;
					}
					if (Lhs.mScore != Rhs.mScore)
					{
						return Lhs.mScore < Rhs.mScore;
					}
					if (Lhs.mOverlapLength != Rhs.mOverlapLength)
					{
						return Lhs.mOverlapLength > Rhs.mOverlapLength;
					}
					if (Lhs.mIndexA != Rhs.mIndexA)
					{
						return Lhs.mIndexA < Rhs.mIndexA;
					}
					return Lhs.mIndexB < Rhs.mIndexB;
				});

			std::unordered_map<int32_t, std::vector<std::pair<float, float>>> UsedIntervalsA;
			std::unordered_map<int32_t, std::vector<std::pair<float, float>>> UsedIntervalsB;
			UsedIntervalsA.reserve(BoundaryEdgesA.size());
			UsedIntervalsB.reserve(BoundaryEdgesB.size());

			const float IntervalOverlapThreshold = 1e-3f;
			auto HasConflict = [IntervalOverlapThreshold](const std::unordered_map<int32_t, std::vector<std::pair<float, float>>>& IntervalMap, int32_t EdgeIndex, float StartT, float EndT)
				{
					const auto Iter = IntervalMap.find(EdgeIndex);
					if (Iter == IntervalMap.end())
					{
						return false;
					}

					for (const std::pair<float, float>& ExistingInterval : Iter->second)
					{
						if (IntervalsOverlap(StartT, EndT, ExistingInterval.first, ExistingInterval.second, IntervalOverlapThreshold))
						{
							return true;
						}
					}
					return false;
				};

			std::vector<MatchedEdgePair> Matches;
			Matches.reserve(CandidatePairs.size());
			for (const CandidateEdgePair& Candidate : CandidatePairs)
			{
				const BoundaryEdge& EdgeA = BoundaryEdgesA[Candidate.mIndexA];
				const BoundaryEdge& EdgeB = BoundaryEdgesB[Candidate.mIndexB];

				if (HasConflict(UsedIntervalsA, EdgeA.mEdgeIndex, Candidate.mStartTA, Candidate.mEndTA))
				{
					continue;
				}
				if (HasConflict(UsedIntervalsB, EdgeB.mEdgeIndex, Candidate.mStartTB, Candidate.mEndTB))
				{
					continue;
				}

				UsedIntervalsA[EdgeA.mEdgeIndex].push_back(std::make_pair(Candidate.mStartTA, Candidate.mEndTA));
				UsedIntervalsB[EdgeB.mEdgeIndex].push_back(std::make_pair(Candidate.mStartTB, Candidate.mEndTB));
				Matches.push_back(BuildMatchedEdgePair(EdgeA, EdgeB, Candidate));
			}

			std::sort(Matches.begin(), Matches.end(), [](const MatchedEdgePair& Lhs, const MatchedEdgePair& Rhs)
				{
					if (Lhs.mFaceA != Rhs.mFaceA) return Lhs.mFaceA < Rhs.mFaceA;
					if (Lhs.mEdgeA != Rhs.mEdgeA) return Lhs.mEdgeA < Rhs.mEdgeA;
					if (Lhs.mFaceB != Rhs.mFaceB) return Lhs.mFaceB < Rhs.mFaceB;
					return Lhs.mEdgeB < Rhs.mEdgeB;
				});

			if (Matches.empty())
			{
			application::util::Logger::Warning(
				"PhiveNavMeshStreaming",
				"No stitchable edges for pair (%u <-> %u). Boundary edges: A=%u, B=%u, tolerance=%.3f",
				UidA,
				UidB,
				static_cast<uint32_t>(BoundaryEdgesA.size()),
				static_cast<uint32_t>(BoundaryEdgesB.size()),
				EdgeMatchTolerance);
				return Result;
			}

			application::util::Logger::Info(
				"PhiveNavMeshStreaming",
				"Pair (%u <-> %u): exact candidates=%u, robust candidates=%u, accepted matches=%u",
				UidA,
				UidB,
				ExactCandidateCount,
				RobustCandidateCount,
				static_cast<uint32_t>(Matches.size()));

			Havok::hkaiStreamingSet MeshSet = BuildMeshStreamingSet(UidA, UidB, Matches, false);

			uint32_t GraphConnectionCount = 0;
			Havok::hkaiStreamingSet GraphSet = BuildGraphStreamingSet(
				UidA,
				UidB,
				Matches,
				GraphA,
				GraphB,
				NavMeshBOffsetFromA,
				false,
				ClusteringSetupIdentifiers,
				GraphConnectionCount);

			MeshA.mStreamingSets.mElements.push_back(MakeAnnotatedStreamingSet(MeshSet, Havok::hkaiAnnotatedStreamingSet__Side::SIDE_A));
			GraphA.mStreamingSets.mElements.push_back(MakeAnnotatedStreamingSet(GraphSet, Havok::hkaiAnnotatedStreamingSet__Side::SIDE_A));
			MeshB.mStreamingSets.mElements.push_back(MakeAnnotatedStreamingSet(MeshSet, Havok::hkaiAnnotatedStreamingSet__Side::SIDE_B));
			GraphB.mStreamingSets.mElements.push_back(MakeAnnotatedStreamingSet(GraphSet, Havok::hkaiAnnotatedStreamingSet__Side::SIDE_B));

			Result.mMeshConnectionCount = static_cast<uint32_t>(MeshSet.mMeshConnections.mElements.size());
			Result.mGraphConnectionCount = GraphConnectionCount;
			return Result;
		}
	}
