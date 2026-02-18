#include "BVNode.h"

#include <util/Logger.h>
#include <util/General.h>
#include "BVHBuilder.h"

namespace application::file::game::phive::util
{
	std::vector<BVNode> BVNode::BuildBVHForMesh(std::vector<glm::vec3> Vertices, std::vector<uint32_t> Indices, int32_t IndicesCount)
	{
		std::vector<float> ConvertedVertices;
		for (glm::vec3& Vec : Vertices)
		{
			ConvertedVertices.push_back(Vec.x);
			ConvertedVertices.push_back(Vec.y);
			ConvertedVertices.push_back(Vec.z);
		}
		if (!BVHBuilder::BuildBVHForMesh(ConvertedVertices.data(), Indices.data(), IndicesCount))
		{
			application::util::Logger::Error("BVNode", "Could not build BVH from mesh");
			return std::vector<BVNode>();
		}

		return BuildFriendlyBVH();
	}

	std::vector<BVNode> BVNode::BuildBVHForMeshQuad(std::vector<glm::vec3> Vertices, std::vector<uint32_t> Indices, int32_t IndicesCount)
	{
		std::vector<float> ConvertedVertices;
		for (glm::vec3& Vec : Vertices)
		{
			ConvertedVertices.push_back(Vec.x);
			ConvertedVertices.push_back(Vec.y);
			ConvertedVertices.push_back(Vec.z);
		}
		if (!BVHBuilder::BuildBVHForQuadMesh(ConvertedVertices.data(), Indices.data(), IndicesCount))
		{
			application::util::Logger::Error("BVNode", "Could not build BVH from quad mesh");
			return std::vector<BVNode>();
		}

		return BuildFriendlyBVH();
	}

	std::vector<BVNode> BVNode::BuildBVHForDomains(std::vector<glm::vec3> Domains, int32_t DomainCount)
	{
		std::vector<float> ConvertedVertices;
		for (glm::vec3& Vec : Domains)
		{
			ConvertedVertices.push_back(Vec.x);
			ConvertedVertices.push_back(Vec.y);
			ConvertedVertices.push_back(Vec.z);
		}
		if (!BVHBuilder::BuildBVHForDomains(ConvertedVertices.data(), DomainCount))
		{
			application::util::Logger::Error("BVNode", "Could not build BVH from domains");
			return std::vector<BVNode>();
		}

		return BuildFriendlyBVH();

	}

	std::vector<BVNode> BVNode::BuildFriendlyBVH()
	{
		std::vector<Bvh::Node> Nodes = BVHBuilder::GetBVHNodes();

		std::vector<BVNode> BNodes;
		for (Bvh::Node Node : Nodes)
		{
			BVNode NewNode;
			NewNode.mMin = glm::vec3(Node.bounds[0], Node.bounds[2], Node.bounds[4]);
			NewNode.mMax = glm::vec3(Node.bounds[1], Node.bounds[3], Node.bounds[5]);
			NewNode.mIsLeaf = Node.is_leaf();
			NewNode.mPrimitiveCount = Node.primitive_count;
			NewNode.mPrimitive = Node.first_child_or_primitive;
			BNodes.push_back(NewNode);
		}


		for (size_t i = 0; i < Nodes.size(); i++)
		{
			if (Nodes[i].is_leaf()) continue;
			BNodes[i].mLeft = &BNodes[(int)Nodes[i].first_child_or_primitive];
			BNodes[i].mRight = &BNodes[(int)Nodes[i].first_child_or_primitive + 1];
		}

		return BNodes;

	}

	uint32_t BVNode::ComputePrimitiveCounts()
	{
		if (mIsLeaf)
			return mPrimitiveCount;

		mPrimitiveCount = mLeft->ComputePrimitiveCounts() + mRight->ComputePrimitiveCounts();
		return mPrimitiveCount;
	}

	std::vector<uint32_t> BVNode::ComputeUniqueIndicesCounts(const std::vector<uint32_t>& Indices)
	{
		if (mIsLeaf)
		{
			std::vector<uint32_t> s(4);
			s[0] = mPrimitive * 4;
			s[1] = mPrimitive * 4 + 1;
			s[2] = mPrimitive * 4 + 2;
			s[3] = mPrimitive * 4 + 3;

			mUniqueIndicesCount = 4;
			return s;
		}

		std::vector<uint32_t> Left = mLeft->ComputeUniqueIndicesCounts(Indices);
		std::vector<uint32_t> Right = mRight->ComputeUniqueIndicesCounts(Indices);
		application::util::General::UnionWith<uint32_t>(Left, Right);
		mUniqueIndicesCount = Left.size();
		return Left;
	}

	void BVNode::AttemptSectionSplit()
	{
		if (mIsLeaf || mPrimitiveCount <= 255 && mUniqueIndicesCount <= (255 * 4)) return;
		mIsSectionHead = false;
		mLeft->mIsSectionHead = true;
		mRight->mIsSectionHead = true;
		mLeft->AttemptSectionSplit();
		mRight->AttemptSectionSplit();
	}

	std::vector<BVNode::QuadBVHNode> BVNode::BuildQuadTree(std::vector<BVNode*>& Nodes)
	{
		std::vector<QuadBVHNode> QuadNodes;
		
		std::vector<BVNode*> Leafs;
		for (BVNode* Node : Nodes)
		{
			if (Node->mIsLeaf)
				Leafs.push_back(Node);
		}

		//Adding leafs
		uint32_t PrimitiveIndex = 0;
		for (size_t j = 0; j < std::ceil(Leafs.size() / 4.0f); j++)
		{
			QuadBVHNode Node;
			Node.mIsLeaf = true;

			for (uint8_t k = 0; k < 4; k++)
			{
				if (Leafs.size() > PrimitiveIndex)
				{
					Node.mMin[k] = Leafs[PrimitiveIndex]->mMin;
					Node.mMax[k] = Leafs[PrimitiveIndex]->mMax;
					Node.mPrimitiveIndices[k] = Leafs[PrimitiveIndex]->mPrimitive;
					PrimitiveIndex++;
				}
				else
				{
					Node.mMax[k] = glm::vec3(-1.0f, -1.0f, -1.0f);
					Node.mMin[k] = glm::vec3(1.0f, 1.0f, 1.0f);
					Node.mPrimitiveIndices[k] = 0;
				}
			}

			QuadNodes.push_back(Node);
		}

		{
			for (QuadBVHNode& Node : QuadNodes)
			{
				std::vector<std::pair<uint8_t, uint8_t>> IndexToPrimitive;
				IndexToPrimitive.push_back({ 0, Node.mPrimitiveIndices[0] });
				IndexToPrimitive.push_back({ 1, Node.mPrimitiveIndices[1] });
				IndexToPrimitive.push_back({ 2, Node.mPrimitiveIndices[2] });
				IndexToPrimitive.push_back({ 3, Node.mPrimitiveIndices[3] });

				std::sort(IndexToPrimitive.begin(), IndexToPrimitive.end(), [&Node](auto& left, auto& right) {
					bool LeftValid = Node.mMin[left.first].x < Node.mMax[left.first].x &&
						Node.mMin[left.first].y < Node.mMax[left.first].y &&
						Node.mMin[left.first].z < Node.mMax[left.first].z;
					LeftValid |= Node.mMin[left.first] == Node.mMax[left.first];

					bool RightValid = Node.mMin[right.first].x < Node.mMax[right.first].x &&
						Node.mMin[right.first].y < Node.mMax[right.first].y &&
						Node.mMin[right.first].z < Node.mMax[right.first].z;
					RightValid |= Node.mMin[right.first] == Node.mMax[right.first];

					if (left.second == right.second)
					{
						return !LeftValid && RightValid;
					}

					return left.second < right.second;
					});

				if (IndexToPrimitive[3].second > IndexToPrimitive[2].second)
				{
					std::pair<uint32_t, uint32_t> Tmp = IndexToPrimitive[2];
					IndexToPrimitive[2] = IndexToPrimitive[3];
					IndexToPrimitive[3] = Tmp;
				}

				QuadBVHNode NodeCopy = Node;

				for (size_t j = 0; j < IndexToPrimitive.size(); j++)
				{
					uint8_t Index = IndexToPrimitive[j].first;

					Node.mMin[j] = NodeCopy.mMin[Index];
					Node.mMax[j] = NodeCopy.mMax[Index];
					Node.mPrimitiveIndices[j] = NodeCopy.mPrimitiveIndices[Index];
				}
			}

			//In rare cases the primitive 0 is referenced in a QuadBVHNode that only contains the primitive 0. As a result, the data is 0 0 0 0, which makes the condition data[2] > data[3] false and thus makes it a connector instead of a leaf, which creates a recursive and invalid tree
			{
				for (QuadBVHNode& Node : QuadNodes)
				{
					if (Node.mPrimitiveIndices[0] == 0 && Node.mPrimitiveIndices[1] == 0 && Node.mPrimitiveIndices[2] == 0 && Node.mPrimitiveIndices[3] == 0)
					{
						int32_t Index = -1;
						for (int j = 0; j < 4; j++)
						{
							//Is valid reference
							if (Node.mMin[j].x <= Node.mMax[j].x && Node.mMin[j].y <= Node.mMax[j].y && Node.mMin[j].z <= Node.mMax[j].z)
							{
								Index = j;
								break;
							}
						}

						if (Index != -1)
						{
							QuadBVHNode NewNode;
							NewNode.mIsLeaf = true;

							for (int j = 0; j < 4; j++)
							{
								if (j == 3)
								{
									NewNode.mMin[j] = Node.mMin[Index];
									NewNode.mMax[j] = Node.mMax[Index];
									NewNode.mPrimitiveIndices[j] = 0;
									continue;
								}

								if (j == Index)
								{
									NewNode.mMin[j] = glm::vec3(1.0f, 1.0f, 1.0f);
									NewNode.mMax[j] = glm::vec3(-1.0f, -1.0f, -1.0f);
									NewNode.mPrimitiveIndices[j] = 0;
									continue;
								}

								NewNode.mMin[j] = Node.mMin[j];
								NewNode.mMax[j] = Node.mMax[j];
								NewNode.mPrimitiveIndices[j] = 0;
							}

							for (QuadBVHNode& ReplacementNode : QuadNodes)
							{
								if (ReplacementNode.mPrimitiveIndices[0] == 0 && ReplacementNode.mPrimitiveIndices[1] == 0 && ReplacementNode.mPrimitiveIndices[2] == 0 && ReplacementNode.mPrimitiveIndices[3] == 0)
									continue;

								NewNode.mMin[2] = ReplacementNode.mMin[0];
								NewNode.mMax[2] = ReplacementNode.mMax[0];
								NewNode.mPrimitiveIndices[2] = ReplacementNode.mPrimitiveIndices[0];

								ReplacementNode.mMin[0] = glm::vec3(1.0f, 1.0f, 1.0f);
								ReplacementNode.mMax[0] = glm::vec3(-1.0f, -1.0f, -1.0f);
								ReplacementNode.mPrimitiveIndices[0] = 0;
								break;
							}

							Node = NewNode;
						}
					}
				}
			}
		}

		//Adding connectors - new
		if(QuadNodes.size() > 1)
		{
			uint32_t NodeCount = QuadNodes.size();
			uint32_t NodeOffset = 0;
			uint32_t ConnectorChildCount = 0;
			QuadBVHNode QuadNode;
			QuadNode.mIsLeaf = false;
			while (true)
			{
				if (NodeCount == 0)
					break;

				uint32_t NewNodeCount = 0;

				for (uint32_t i = 0; i < NodeCount; i++)
				{
					if (ConnectorChildCount == 4)
					{
						ConnectorChildCount = 0;
						NewNodeCount++;

						std::vector<std::pair<uint8_t, uint16_t>> IndexToPrimitive;
						IndexToPrimitive.push_back({ 0, QuadNode.mChildrenIndices[0] });
						IndexToPrimitive.push_back({ 1, QuadNode.mChildrenIndices[1] });
						IndexToPrimitive.push_back({ 2, QuadNode.mChildrenIndices[2] });
						IndexToPrimitive.push_back({ 3, QuadNode.mChildrenIndices[3] });

						std::sort(IndexToPrimitive.begin(), IndexToPrimitive.end(), [](auto& left, auto& right)
							{
								return left.second > right.second;
							});

						QuadBVHNode NewNode;
						NewNode.mIsLeaf = false;

						for (size_t j = 0; j < IndexToPrimitive.size(); j++)
						{
							uint8_t Index = IndexToPrimitive[j].first;

							NewNode.mMin[j] = QuadNode.mMin[Index];
							NewNode.mMax[j] = QuadNode.mMax[Index];
							NewNode.mChildrenIndices[j] = QuadNode.mChildrenIndices[Index];
						}

						QuadNodes.push_back(NewNode);

						QuadNode = QuadBVHNode{};
						QuadNode.mIsLeaf = false;
					}

					QuadNode.mChildrenIndices[ConnectorChildCount] = NodeOffset + i;

					glm::vec3 Min = glm::vec3(std::numeric_limits<float>::max());
					glm::vec3 Max = glm::vec3(-std::numeric_limits<float>::max());

					for (int j = 0; j < 4; j++)
					{
						Min.x = std::min(Min.x, QuadNodes[NodeOffset + i].mMin[j].x);
						Min.y = std::min(Min.y, QuadNodes[NodeOffset + i].mMin[j].y);
						Min.z = std::min(Min.z, QuadNodes[NodeOffset + i].mMin[j].z);

						Max.x = std::max(Max.x, QuadNodes[NodeOffset + i].mMax[j].x);
						Max.y = std::max(Max.y, QuadNodes[NodeOffset + i].mMax[j].y);
						Max.z = std::max(Max.z, QuadNodes[NodeOffset + i].mMax[j].z);
					}

					QuadNode.mMin[ConnectorChildCount] = Min;
					QuadNode.mMax[ConnectorChildCount] = Max;

					ConnectorChildCount++;
				}

				NodeOffset += NodeCount;
				NodeCount = NewNodeCount;
			}

			std::vector<std::pair<uint8_t, uint16_t>> IndexToPrimitive;
			IndexToPrimitive.push_back({ 0, QuadNode.mChildrenIndices[0] });
			IndexToPrimitive.push_back({ 1, QuadNode.mChildrenIndices[1] });
			IndexToPrimitive.push_back({ 2, QuadNode.mChildrenIndices[2] });
			IndexToPrimitive.push_back({ 3, QuadNode.mChildrenIndices[3] });

			std::sort(IndexToPrimitive.begin(), IndexToPrimitive.end(), [](auto& left, auto& right)
				{
					return left.second > right.second;
				});

			QuadBVHNode NewNode;
			NewNode.mIsLeaf = false;

			for (size_t j = 0; j < IndexToPrimitive.size(); j++)
			{
				uint8_t Index = IndexToPrimitive[j].first;

				NewNode.mMin[j] = QuadNode.mMin[Index];
				NewNode.mMax[j] = QuadNode.mMax[Index];
				NewNode.mChildrenIndices[j] = QuadNode.mChildrenIndices[Index];
			}

			QuadNodes.push_back(NewNode);
		}

		std::vector<QuadBVHNode> OrderedNodes;
		for (std::vector<QuadBVHNode>::reverse_iterator RIter = QuadNodes.rbegin();
			RIter != QuadNodes.rend(); ++RIter)
		{
			QuadBVHNode QuadNode = *RIter;
			if (!QuadNode.mIsLeaf)
			{
				for (int j = 0; j < 4; j++)
				{
					if (QuadNode.mMax[j].x > QuadNode.mMin[j].x && QuadNode.mMax[j].y > QuadNode.mMin[j].y && QuadNode.mMax[j].z > QuadNode.mMin[j].z)
					{
						QuadNode.mChildrenIndices[j] = QuadNodes.size() - 1 - QuadNode.mChildrenIndices[j];
					}
				}
			}
			OrderedNodes.push_back(QuadNode);
		}

		return OrderedNodes;
	}

	uint8_t CompressDim(float Min, float Max, float PMin, float PMax)
	{
		float snorm = 226.0f / (PMax - PMin);
		float rmin = std::sqrtf(std::fmax((Min - PMin) * snorm, 0));
		float rmax = std::sqrtf(std::fmax((Max - PMax) * -snorm, 0));
		uint8_t a = (uint8_t)std::min(0xF, (int)std::floorf(rmin));
		uint8_t b = (uint8_t)std::min(0xF, (int)std::floorf(rmax));
		return (uint8_t)((a << 4) | b);
	}

	void CompressNode(std::vector<PhiveNavMesh::hkcdCompressedAabbCodecs__Aabb6BytesCodec>& Ret, BVNode* Node, glm::vec3 ParentMin, glm::vec3 ParentMax, bool Root)
	{
		size_t CurrIndex = Ret.size();
		Ret.resize(Ret.size() + 1);
		PhiveNavMesh::hkcdCompressedAabbCodecs__Aabb6BytesCodec Compressed;

		Compressed.m_xyz[0] = CompressDim(Node->mMin.x, Node->mMax.x, ParentMin.x, ParentMax.x);
		Compressed.m_xyz[1] = CompressDim(Node->mMin.y, Node->mMax.y, ParentMin.y, ParentMax.y);
		Compressed.m_xyz[2] = CompressDim(Node->mMin.z, Node->mMax.z, ParentMin.z, ParentMax.z);

		glm::vec3 Min = Compressed.DecompressMin(ParentMin, ParentMax);
		glm::vec3 Max = Compressed.DecompressMax(ParentMin, ParentMax);

		if (Node->mIsLeaf)
		{
			uint32_t Data = Node->mPrimitive;
			Compressed.m_loData.m_primitiveBase = (uint16_t)(Data & 0xFFFF);
			Compressed.m_hiData.m_primitiveBase = (uint8_t)((Data >> 16) & 0x7F);
		}
		else
		{
			CompressNode(Ret, Node->mLeft, Min, Max, false);

			uint16_t Data = (uint16_t)((Ret.size() - CurrIndex) / 2);
			Compressed.m_loData.m_primitiveBase = (uint16_t)(Data & 0xFFFF);
			Compressed.m_hiData.m_primitiveBase = (uint8_t)(((Data >> 16) & 0x7F) | 0x80);

			CompressNode(Ret, Node->mRight, Min, Max, false);
		}

		if (Root)
		{
			Compressed.m_xyz[0] = 0;
			Compressed.m_xyz[1] = 0;
			Compressed.m_xyz[2] = 0;
		}

		Ret[CurrIndex] = Compressed;
	}

	std::vector<PhiveNavMesh::hkcdCompressedAabbCodecs__Aabb6BytesCodec> BVNode::BuildAxis6ByteTree()
	{
		std::vector<PhiveNavMesh::hkcdCompressedAabbCodecs__Aabb6BytesCodec> Ret;
		CompressNode(Ret, this, mMin, mMax, true);
		return Ret;
	}
}