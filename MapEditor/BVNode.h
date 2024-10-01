#pragma once

#include <glm/glm.hpp>
#include <vector>
#include "BVHBuilder.h"
#include "PhiveNavMesh.h"

class BVNode
{
public:
	bool mIsLeaf = false;
	bool mIsSectionHead = false;
	BVNode* mLeft = nullptr;
	glm::vec3 mMax;
	glm::vec3 mMin;
	uint32_t mPrimitive = 0;
	uint32_t mPrimitiveCount = 0;
	BVNode* mRight = nullptr;
	uint32_t mUniqueIndicesCount = 0;

	static std::vector<BVNode> BuildBVHForMesh(std::vector<glm::vec3> Vertices, std::vector<uint32_t> Indices, int32_t IndicesCount);
	static std::vector<BVNode> BuildBVHForDomains(std::vector<glm::vec3> Domains, int32_t DomainCount);
	static std::vector<BVNode> BuildFriendlyBVH();
	uint32_t ComputePrimitiveCounts();
	//std::vector<uint32_t> ComputeUniqueIndicesCount(std::vector<uint32_t> Indices);
	//void AttemptSectionSplit();
	//uint8_t CompressDim(float Min, float Max, float PMin, float PMax);
	std::vector<PhiveNavMesh::hkcdCompressedAabbCodecs__Aabb6BytesCodec> BuildAxis6ByteTree();

	BVNode() = default;
};