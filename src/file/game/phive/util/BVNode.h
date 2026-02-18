#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <file/game/phive/PhiveNavMesh.h>

namespace application::file::game::phive::util
{
	struct BVNode
	{
		bool mIsLeaf = false;
		bool mIsSectionHead = false;
		BVNode* mLeft = nullptr;
		glm::vec3 mMax;
		glm::vec3 mMin;
		uint32_t mPrimitive = 0;
		uint32_t mPrimitiveCount = 0;
		BVNode* mRight = nullptr;
		uint32_t mUniqueIndicesCount = 0;

		struct QuadBVHNode
		{
			glm::vec3 mMin[4];
			glm::vec3 mMax[4];
			bool mIsLeaf = false;

			uint16_t mChildrenIndices[4] = { 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF }; //mIsLeaf == true
			uint8_t mPrimitiveIndices[4] = { 0 }; //mIsLeaf == false
		};

		static std::vector<BVNode> BuildBVHForMesh(std::vector<glm::vec3> Vertices, std::vector<uint32_t> Indices, int32_t IndicesCount);
		static std::vector<BVNode> BuildBVHForMeshQuad(std::vector<glm::vec3> Vertices, std::vector<uint32_t> Indices, int32_t IndicesCount);
		static std::vector<BVNode> BuildBVHForDomains(std::vector<glm::vec3> Domains, int32_t DomainCount);
		static std::vector<BVNode> BuildFriendlyBVH();
		static std::vector<QuadBVHNode> BuildQuadTree(std::vector<BVNode*>& Nodes);
		uint32_t ComputePrimitiveCounts();
		std::vector<uint32_t> ComputeUniqueIndicesCounts(const std::vector<uint32_t>& Indices);
		void AttemptSectionSplit();
		std::vector<application::file::game::phive::PhiveNavMesh::hkcdCompressedAabbCodecs__Aabb6BytesCodec> BuildAxis6ByteTree();

		BVNode() = default;
	};
}