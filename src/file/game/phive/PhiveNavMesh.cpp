#include <file/game/phive/PhiveNavMesh.h>

#include <util/Logger.h>
#include <unordered_map>
#include <algorithm>
#include <fstream>
#include <glm/glm.hpp>
#include <file/game/phive/util/BVNode.h>
#include <file/game/phive/util/hkaiClusterGraphBuilder.h>
#include <util/MeshOptimizer.h>

/*

THIS IS AN OUTDATED IMPLEMENTATION, USE file/game/phive/navmesh/PhiveNavMesh INSTEAD

*/

namespace application::file::game::phive
{
	std::pair<std::vector<glm::vec3>, std::vector<uint32_t>> PhiveNavMesh::ToVerticesIndices()
	{
		std::vector<uint32_t> Indices;
		std::vector<glm::vec3> Vertices;

		for (hkaiNavMesh__Face& Face : mContainer.m_namedVariantNavMesh.m_variant.m_ptr.m_faces)
		{
			if (Face.m_numEdges.m_primitiveBase == 3)
			{
				Indices.push_back(mContainer.m_namedVariantNavMesh.m_variant.m_ptr.m_edges[Face.m_startEdgeIndex.m_primitiveBase].m_a.m_primitiveBase);
				Indices.push_back(mContainer.m_namedVariantNavMesh.m_variant.m_ptr.m_edges[Face.m_startEdgeIndex.m_primitiveBase].m_b.m_primitiveBase);
				Indices.push_back(mContainer.m_namedVariantNavMesh.m_variant.m_ptr.m_edges[Face.m_startEdgeIndex.m_primitiveBase + 1].m_b.m_primitiveBase);
				continue;
			}

			std::vector<std::pair<glm::vec3, int32_t>> FaceVertices;
			std::vector<hkaiNavMesh__Edge> FaceEdges(Face.m_numEdges.m_primitiveBase);
			for (size_t i = 0; i < FaceEdges.size(); i++)
			{
				FaceEdges[i] = mContainer.m_namedVariantNavMesh.m_variant.m_ptr.m_edges[Face.m_startEdgeIndex.m_primitiveBase + i];
			}

			int32_t EdgeIndex = 0;
			for (size_t i = 0; i < FaceEdges.size(); i++)
			{
				hkVector4& Vertex = mContainer.m_namedVariantNavMesh.m_variant.m_ptr.m_vertices[FaceEdges[EdgeIndex].m_b.m_primitiveBase];
				FaceVertices.push_back(std::make_pair(glm::vec3(Vertex.m_subTypeArray[0], Vertex.m_subTypeArray[1], Vertex.m_subTypeArray[2]), FaceEdges[EdgeIndex].m_b.m_primitiveBase));

				for (size_t j = 0; j < FaceEdges.size(); j++)
				{
					if (FaceEdges[j].m_a.m_primitiveBase == FaceEdges[EdgeIndex].m_b.m_primitiveBase)
					{
						EdgeIndex = j;
						break;
					}
				}
			}

			for (uint32_t i = 0; i < FaceVertices.size() - 2; i++)
			{
				Indices.push_back(FaceVertices[0].second);
				Indices.push_back(FaceVertices[i + 1].second);
				Indices.push_back(FaceVertices[i + 2].second);
			}
		}

		for (hkVector4& Vertex : mContainer.m_namedVariantNavMesh.m_variant.m_ptr.m_vertices)
		{
			Vertices.emplace_back(Vertex.m_subTypeArray[0], Vertex.m_subTypeArray[1], Vertex.m_subTypeArray[2]);
		}

		return std::make_pair(Vertices, Indices);
	}


	struct BVHNode
	{
		uint32_t mIndex = 0;
		bool mIsTerminal = false;
		BVHNode* mLeft = nullptr;
		glm::vec3 mMax;
		glm::vec3 mMin;
		BVHNode* mRight = nullptr;
	};

	BVHNode* BuildBVHTree(std::vector<BVHNode>& Nodes, PhiveNavMesh::hkcdStaticTree__AabbTree& Tree, glm::vec3 ParentMin, glm::vec3 ParentMax, uint32_t NodeIndex, uint32_t& VecIndex)
	{
		PhiveNavMesh::hkcdCompressedAabbCodecs__Aabb6BytesCodec& CNode = Tree.m_nodes[NodeIndex];
		BVHNode Node;
		Node.mMin = CNode.DecompressMin(ParentMin, ParentMax);
		Node.mMax = CNode.DecompressMax(ParentMin, ParentMax);

		if ((CNode.m_hiData.m_primitiveBase & 0x80) > 0)
		{
			Node.mLeft = BuildBVHTree(Nodes, Tree, Node.mMin, Node.mMax, NodeIndex + 1, VecIndex);
			Node.mRight = BuildBVHTree(Nodes, Tree, Node.mMin, Node.mMax,
				NodeIndex + ((((uint32_t)CNode.m_hiData.m_primitiveBase & 0x7F) << 8) | CNode.m_loData.m_primitiveBase) * 2, VecIndex);
		}
		else
		{
			Node.mIsTerminal = true;
			Node.mIndex = (((uint32_t)CNode.m_hiData.m_primitiveBase & 0x7F) << 8) | CNode.m_loData.m_primitiveBase;
		}

		Nodes[VecIndex] = Node;
		VecIndex++;
		return &Nodes[VecIndex - 1];
	}

	void ReadBVH(PhiveNavMesh::hkcdStaticTree__AabbTree& Tree)
	{
		if (Tree.m_nodes.empty())
			return;

		std::vector<BVHNode> Nodes;
		Nodes.resize(Tree.m_nodes.size() - 1);
		BVHNode RootNode;
		RootNode.mMin = glm::vec3(Tree.m_domain.m_min.m_subTypeArray[0],
			Tree.m_domain.m_min.m_subTypeArray[1],
			Tree.m_domain.m_min.m_subTypeArray[2]);
		RootNode.mMax = glm::vec3(Tree.m_domain.m_max.m_subTypeArray[0],
			Tree.m_domain.m_max.m_subTypeArray[1],
			Tree.m_domain.m_max.m_subTypeArray[2]);

		PhiveNavMesh::hkcdCompressedAabbCodecs__Aabb6BytesCodec& CNode = Tree.m_nodes[0];
		uint32_t VecIndex = 0;
		if ((CNode.m_hiData.m_primitiveBase & 0x80) > 0)
		{
			RootNode.mLeft = BuildBVHTree(Nodes, Tree, RootNode.mMin, RootNode.mMax, 1, VecIndex);
			RootNode.mRight = BuildBVHTree(Nodes, Tree, RootNode.mMin, RootNode.mMax,
				((((uint32_t)CNode.m_hiData.m_primitiveBase & 0x7F) << 8) | CNode.m_loData.m_primitiveBase) * 2, VecIndex);
		}
		else
		{
			RootNode.mIsTerminal = true;
			RootNode.mIndex = (((uint32_t)CNode.m_hiData.m_primitiveBase & 0x7F) << 8) | CNode.m_loData.m_primitiveBase;
		}
	}

	void PhiveNavMesh::SetNavMeshModel(const std::vector<glm::vec3>& Vertices, const std::vector<uint32_t>& Indices, const application::file::game::phive::util::NavMeshGenerator::PhiveNavMeshGeneratorConfig& Config)
	{
		hkaiNavMesh& NavMesh = mContainer.m_namedVariantNavMesh.m_variant.m_ptr;

		std::vector<float> VerticesFloatConverted;
		VerticesFloatConverted.reserve(Vertices.size() * 3);
		for (const glm::vec3& Vec : Vertices)
		{
			VerticesFloatConverted.push_back(Vec.x);
			VerticesFloatConverted.push_back(Vec.y);
			VerticesFloatConverted.push_back(Vec.z);
		}

		application::file::game::phive::util::NavMeshGenerator MeshGenerator;
		MeshGenerator.SetNavmeshBuildParams(Config);
		if (!MeshGenerator.BuildNavmeshForMesh(VerticesFloatConverted.data(), VerticesFloatConverted.size() / 3, reinterpret_cast<const int*>(Indices.data()), Indices.size()))
		{
			application::util::Logger::Error("PhiveNavMesh", "Failed to generate navigation mesh");
			return;
		}

		int VCount = MeshGenerator.GetMeshVertexCount();
		int ICount = MeshGenerator.GetMeshTriangleCount();
		if (VCount == 0 || ICount == 0)
		{
			application::util::Logger::Error("PhiveNavMesh", "Resulting navigation mesh is empty");
			return;
		}

		std::vector<uint16_t> BVerts;
		std::vector<uint16_t> BIndices;
		BVerts.resize(VCount * 3);
		BIndices.resize(ICount * 3 * 2);
		MeshGenerator.GetMeshVertices(BVerts.data());
		MeshGenerator.GetMeshTriangles(BIndices.data());

		glm::vec3 Bounds[2];
		Bounds[0].x = MeshGenerator.mBoundingBoxMin[0];
		Bounds[0].y = MeshGenerator.mBoundingBoxMin[1];
		Bounds[0].z = MeshGenerator.mBoundingBoxMin[2];
		Bounds[1].x = MeshGenerator.mBoundingBoxMax[0];
		Bounds[1].y = MeshGenerator.mBoundingBoxMax[1];
		Bounds[1].z = MeshGenerator.mBoundingBoxMax[2];

		/* Optimizing model - Begin */

		std::vector<glm::vec3> OptimizedVertices;
		for (size_t i = 0; i < BVerts.size() / 3; i++)
		{
			uint16_t VX = BVerts[i * 3];
			uint16_t VY = BVerts[i * 3 + 1];
			uint16_t VZ = BVerts[i * 3 + 2];

			glm::vec3 Vertex = glm::vec3(Bounds[0].x + VX * MeshGenerator.mConfig.cs,
				Bounds[0].y + VY * MeshGenerator.mConfig.ch,
				Bounds[0].z + VZ * MeshGenerator.mConfig.cs);

			OptimizedVertices.push_back(Vertex);
		}
		/*
		application::util::MeshOptimizer Optimizer(OptimizedVertices, OptimizedIndices);
		Optimizer.Optimize(0.5);
		Optimizer.GetOptimizedMesh(OptimizedVertices, OptimizedIndices);

		BIndices.clear();
		for (const uint32_t& Index : OptimizedIndices)
		{
			BIndices.push_back(Index);
		}
		*/

		/* Optimizing model - End */

		NavMesh.m_faces.clear();
		NavMesh.m_edges.clear();
		NavMesh.m_vertices.clear();
		NavMesh.m_faceData.clear();
		NavMesh.m_edgeData.clear();
		NavMesh.m_streamingSets.clear();

		NavMesh.m_aabb.m_min.m_subTypeArray[0] = Bounds[0].x;
		NavMesh.m_aabb.m_min.m_subTypeArray[1] = Bounds[0].y;
		NavMesh.m_aabb.m_min.m_subTypeArray[2] = Bounds[0].z;
		NavMesh.m_aabb.m_max.m_subTypeArray[0] = Bounds[1].x;
		NavMesh.m_aabb.m_max.m_subTypeArray[1] = Bounds[1].y;
		NavMesh.m_aabb.m_max.m_subTypeArray[2] = Bounds[1].z;

		NavMesh.m_erosionRadius.m_primitiveBase = 0;
		NavMesh.m_userData.m_primitiveBase = 0;
		
		for (const glm::vec3& Vertex : OptimizedVertices)
		{
			hkVector4 Vector;
			Vector.m_subTypeArray[0] = Vertex.x;
			Vector.m_subTypeArray[1] = Vertex.y;
			Vector.m_subTypeArray[2] = Vertex.z;
			Vector.m_subTypeArray[3] = 1.0f;

			NavMesh.m_vertices.push_back(Vector);
		}

		hkInt32 hkInt32Null;
		hkInt32Null.m_primitiveBase = 0;

		for (size_t t = 0; t < BIndices.size() / 2; t += 3)
		{
			hkaiNavMesh__Face Face;
			Face.m_numEdges.m_primitiveBase = 3;
			Face.m_startUserEdgeIndex.m_primitiveBase = -1;
			Face.m_numUserEdges.m_primitiveBase = 0;
			Face.m_startEdgeIndex.m_primitiveBase = NavMesh.m_edges.size();
			NavMesh.m_faces.push_back(Face);
			NavMesh.m_faceData.push_back(hkInt32Null);

			for (int i = 0; i < 3; i++)
			{
				hkaiNavMesh__Edge Edge;
				Edge.m_a.m_primitiveBase = BIndices[t * 2 + i];
				Edge.m_b.m_primitiveBase = BIndices[t * 2 + (i + 1) % 3];
				Edge.m_flags.m_storage.m_primitiveBase = 0x04; //EDGE_ORIGINAL
				Edge.m_paddingByte.m_primitiveBase = 0;
				Edge.m_userEdgeCost.m_value.m_primitiveBase = 0;

				if (BIndices[t * 2 + 3 + i] == 0xFFFF)
				{
					Edge.m_oppositeEdge.m_primitiveBase = 0xFFFFFFFF;
					Edge.m_oppositeFace.m_primitiveBase = 0xFFFFFFFF;
				}
				else
				{
					Edge.m_oppositeFace.m_primitiveBase = BIndices[t * 2 + 3 + i];
					for (int j = 0; j < 3; j++)
					{
						uint16_t AnotherEdge = BIndices[t * 2 + 3 + i] * 6 + 3 + j;
						if (BIndices[AnotherEdge] == t / 3)
							Edge.m_oppositeEdge.m_primitiveBase = (uint32_t)BIndices[t * 2 + 3 + i] * 3 + (uint32_t)j;
					}
				}

				NavMesh.m_edges.push_back(Edge);
				NavMesh.m_edgeData.push_back(hkInt32Null);
			}
		}

		std::vector<uint16_t> ShortIndices;
		ShortIndices.resize(BIndices.size() / 2);
		for (size_t i = 0; i < BIndices.size() / 2; i += 3)
		{
			ShortIndices[i] = BIndices[i * 2];
			ShortIndices[i + 1] = BIndices[i * 2 + 1];
			ShortIndices[i + 2] = BIndices[i * 2 + 2];
		}

		std::vector<uint32_t> ShortIndicesConverted;
		for (uint16_t Index : ShortIndices)
		{
			ShortIndicesConverted.push_back(Index);
		}

		std::vector<application::file::game::phive::util::BVNode> BNodes = application::file::game::phive::util::BVNode::BuildBVHForMesh(OptimizedVertices, ShortIndicesConverted, ShortIndicesConverted.size());
		glm::vec3 Min = BNodes[0].mMin;
		glm::vec3 Max = BNodes[0].mMax;

		hkcdStaticTree__Aabb6BytesTree& AabbTree = NavMesh.m_cachedFaceIterator.m_ptr.m_tree.m_ptr.m_treePtr.m_ptr.m_tree;
		hkVector4 AabbTreeMin;
		hkVector4 AabbTreeMax;
		AabbTreeMin.m_subTypeArray[0] = Min.x;
		AabbTreeMin.m_subTypeArray[1] = Min.y;
		AabbTreeMin.m_subTypeArray[2] = Min.z;
		AabbTreeMin.m_subTypeArray[3] = 1.0f;
		AabbTreeMax.m_subTypeArray[0] = Max.x;
		AabbTreeMax.m_subTypeArray[1] = Max.y;
		AabbTreeMax.m_subTypeArray[2] = Max.z;
		AabbTreeMax.m_subTypeArray[3] = 1.0f;
		AabbTree.m_domain.m_min = AabbTreeMin;
		AabbTree.m_domain.m_max = AabbTreeMax;
		std::vector<hkcdCompressedAabbCodecs__Aabb6BytesCodec> AabbTreeCodec = BNodes[0].BuildAxis6ByteTree();
		AabbTree.m_nodes = AabbTreeCodec;

		hkAabb& Domain = AabbTree.m_domain;
		hkVector4 Center;
		Center.m_subTypeArray[0] = (AabbTree.m_domain.m_max.m_subTypeArray[0] - AabbTree.m_domain.m_min.m_subTypeArray[0]) / 2;
		Center.m_subTypeArray[1] = (AabbTree.m_domain.m_max.m_subTypeArray[1] - AabbTree.m_domain.m_min.m_subTypeArray[1]) / 2;
		Center.m_subTypeArray[2] = (AabbTree.m_domain.m_max.m_subTypeArray[2] - AabbTree.m_domain.m_min.m_subTypeArray[2]) / 2;
		Center.m_subTypeArray[3] = (AabbTree.m_domain.m_max.m_subTypeArray[3] - AabbTree.m_domain.m_min.m_subTypeArray[3]) / 2;

		//mContainer.m_namedVariantClusterGraph.m_variant.m_ptr = application::file::game::phive::util::hkaiClusterGraphBuilder::Build(NavMesh, Config);

		hkaiClusterGraph& Graph = mContainer.m_namedVariantClusterGraph.m_variant.m_ptr;
		Graph.m_positions.resize(1);
		Graph.m_positions[0] = Center;
		Graph.m_positions[0].m_subTypeArray[3] = 1.0f;

		Graph.m_nodes.resize(1);
		Graph.m_nodes[0].m_numEdges = 0;
		Graph.m_nodes[0].m_startEdgeIndex = 0;

		Graph.m_edges.clear();
		Graph.m_edgeData.clear();
		Graph.m_nodeData.clear();
		Graph.m_streamingSets.clear();
		Graph.m_featureToNodeIndex.clear();
		Graph.m_userData.m_primitiveBase = 0;
		Graph.m_nodeDataStriding = 0;
		Graph.m_edgeDataStriding = 0;


		//m_featureToNodeIndex probably maps each face to a node (or maybe an edge) in the cluster graph
		application::file::game::phive::PhiveNavMesh::hkaiIndex NullIndex;
		NullIndex.m_primitiveBase = 0;

		for (auto& Face : NavMesh.m_faces)
		{
			Graph.m_featureToNodeIndex.push_back(NullIndex);
		}

		//ReadBVH(mContainer.m_namedVariantNavMesh.m_variant.m_ptr.m_cachedFaceIterator.m_ptr.m_tree.m_ptr.m_treePtr.m_ptr.m_tree);
	}

	void PhiveNavMesh::Initialize(std::vector<unsigned char> Bytes)
	{
		mPhiveWrapper = application::file::game::phive::util::PhiveWrapper(Bytes, true);

		application::file::game::phive::util::PhiveBinaryVectorReader Reader(Bytes, mPhiveWrapper);
		Reader.Seek(0x50, application::util::BinaryVectorReader::Position::Begin);

		mContainer.Read(Reader);
	}

	PhiveNavMesh::PhiveNavMesh(std::vector<unsigned char> Bytes)
	{
		Initialize(Bytes);
	}

	std::vector<unsigned char> PhiveNavMesh::ToBinary()
	{
		application::file::game::phive::util::PhiveBinaryVectorWriter Writer(mPhiveWrapper);
		mContainer.Write(Writer);

		mPhiveWrapper.mHavokTagFile = Writer.GetData();

		//Creating patches
		mPhiveWrapper.mPatches.clear();
		mPhiveWrapper.mPatches.push_back(application::file::game::phive::util::PhiveWrapper::PhiveWrapperPatch{
			.mTypeIndex = (uint32_t)(mPhiveWrapper.GetTypeNameIndex("hkRootLevelContainer__NamedVariant") - 1),
			.mCount = 1,
			.mOffsets = {0}
			});

		mPhiveWrapper.mPatches.push_back(application::file::game::phive::util::PhiveWrapper::PhiveWrapperPatch{
		.mTypeIndex = (uint32_t)(mPhiveWrapper.GetTypeNameIndex("hkRefVariant") - 1),
		.mCount = 4,
		.mOffsets = {16, 24, 40, 48}
			});

		mPhiveWrapper.mPatches.push_back(application::file::game::phive::util::PhiveWrapper::PhiveWrapperPatch{
			.mTypeIndex = (uint32_t)(mPhiveWrapper.GetTypeNameIndex("char*") - 1),
			.mCount = 2,
			.mOffsets = {32, 56}
			});

		mPhiveWrapper.mPatches.push_back(application::file::game::phive::util::PhiveWrapper::PhiveWrapperPatch{
		.mTypeIndex = (uint32_t)(mPhiveWrapper.GetTypeNameIndex("hkaiNavMesh__Face") - 1),
		.mCount = 1,
		.mOffsets = {152}
			});

		mPhiveWrapper.mPatches.push_back(application::file::game::phive::util::PhiveWrapper::PhiveWrapperPatch{
			.mTypeIndex = (uint32_t)(mPhiveWrapper.GetTypeNameIndex("hkaiNavMesh__Edge") - 1),
			.mCount = 1,
			.mOffsets = {168}
			});

		uint32_t ClusterGraphOffset = 0;
		uint32_t StaticTreeFaceIteratorOffset = 0;
		uint32_t StaticAabbTreeOffset = 0;
		for (application::file::game::phive::util::PhiveWrapper::PhiveWrapperItem& Item : mPhiveWrapper.mItems)
		{
			if (Item.mTypeIndex == mPhiveWrapper.GetTypeNameIndex("hkaiClusterGraph"))
			{
				ClusterGraphOffset = Item.mDataOffset + 24;
			}
			if (Item.mTypeIndex == mPhiveWrapper.GetTypeNameIndex("hkaiNavMeshStaticTreeFaceIterator"))
			{
				StaticTreeFaceIteratorOffset = Item.mDataOffset;
			}
			if (Item.mTypeIndex == mPhiveWrapper.GetTypeNameIndex("hkcdStaticAabbTree"))
			{
				StaticAabbTreeOffset = Item.mDataOffset;
			}
		}
		if (ClusterGraphOffset == 0)
			application::util::Logger::Error("PhiveNavMesh", "Could not calculate hkaiClusterGraph offset");
		if (StaticTreeFaceIteratorOffset == 0)
			application::util::Logger::Error("PhiveNavMesh", "Could not calculate hkaiNavMeshStaticTreeFaceIterator offset");
		if (StaticAabbTreeOffset == 0)
			application::util::Logger::Error("PhiveNavMesh", "Could not calculate hkcdStaticAabbTree offset");

		mPhiveWrapper.mPatches.push_back(application::file::game::phive::util::PhiveWrapper::PhiveWrapperPatch{
			.mTypeIndex = (uint32_t)(mPhiveWrapper.GetTypeNameIndex("hkVector4") - 1),
			.mCount = 2,
			.mOffsets = {184, ClusterGraphOffset}
			});

		mPhiveWrapper.mPatches.push_back(application::file::game::phive::util::PhiveWrapper::PhiveWrapperPatch{
			.mTypeIndex = (uint32_t)(mPhiveWrapper.GetTypeNameIndex("hkInt32") - 1),
			.mCount = 2,
			.mOffsets = {216, 232}
			});

		mPhiveWrapper.mPatches.push_back(application::file::game::phive::util::PhiveWrapper::PhiveWrapperPatch{
			.mTypeIndex = (uint32_t)(mPhiveWrapper.GetTypeNameIndex("hkaiNavMeshStaticTreeFaceIterator") - 1),
			.mCount = 1,
			.mOffsets = {344}
			});

		mPhiveWrapper.mPatches.push_back(application::file::game::phive::util::PhiveWrapper::PhiveWrapperPatch{
			.mTypeIndex = (uint32_t)(mPhiveWrapper.GetTypeNameIndex("hkaiClusterGraph__Node") - 1),
			.mCount = 1,
			.mOffsets = {ClusterGraphOffset + 16}
			});

		if (!mContainer.m_namedVariantClusterGraph.m_variant.m_ptr.m_edges.empty())
		{
			mPhiveWrapper.mPatches.push_back(application::file::game::phive::util::PhiveWrapper::PhiveWrapperPatch{
				.mTypeIndex = (uint32_t)(mPhiveWrapper.GetTypeNameIndex("hkaiClusterGraph__Edge") - 1),
				.mCount = 1,
				.mOffsets = {ClusterGraphOffset + 32}
				});
		}

		uint32_t hkFlagsIndex = 0;
		for (size_t i = 0; i < mPhiveWrapper.mTypes.size(); i++)
		{
			if (mPhiveWrapper.mTypes[i].mName == "hkFlags")
				hkFlagsIndex = i;
		}

		mPhiveWrapper.mPatches.push_back(application::file::game::phive::util::PhiveWrapper::PhiveWrapperPatch{
			.mTypeIndex = hkFlagsIndex,
			.mCount = 1,
			.mOffsets = {ClusterGraphOffset + 80}
			});

		mPhiveWrapper.mPatches.push_back(application::file::game::phive::util::PhiveWrapper::PhiveWrapperPatch{
			.mTypeIndex = (uint32_t)(mPhiveWrapper.GetTypeNameIndex("hkcdStaticAabbTree") - 1),
			.mCount = 1,
			.mOffsets = {StaticTreeFaceIteratorOffset + 24}
			});

		mPhiveWrapper.mPatches.push_back(application::file::game::phive::util::PhiveWrapper::PhiveWrapperPatch{
			.mTypeIndex = (uint32_t)(mPhiveWrapper.GetTypeNameIndex("hkcdStaticAabbTree__Impl") - 1),
			.mCount = 1,
			.mOffsets = {StaticAabbTreeOffset + 24}
			});

		return mPhiveWrapper.ToBinary();
	}

	void PhiveNavMesh::WriteToFile(const std::string& Path)
	{
		std::ofstream File(Path, std::ios::binary);
		std::vector<unsigned char> Binary = ToBinary();
		std::copy(Binary.cbegin(), Binary.cend(),
			std::ostream_iterator<unsigned char>(File));
		File.close();
	}
}