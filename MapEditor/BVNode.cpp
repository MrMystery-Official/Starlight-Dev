#include "BVNode.h"

#include "BVHBuilder.h"
#include "Logger.h"

struct NativeBVHNode
{
	float mMinX;
	float mMaxX;
	float mMinY;
	float mMaxY;
	float mMinZ;
	float mMaxZ;
	uint32_t mPrimitiveCount;
	uint32_t mFirstChildOrPrimitive;

	bool IsLeaf() {
		return mPrimitiveCount > 0;
	}
};

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
		Logger::Error("BVNode", "Could not build BVH from mesh");
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
		Logger::Error("BVNode", "Could not build BVH from domains");
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