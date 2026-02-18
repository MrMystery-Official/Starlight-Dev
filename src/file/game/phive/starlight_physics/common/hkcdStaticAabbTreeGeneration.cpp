#include <file/game/phive/starlight_physics/common/hkcdStaticAabbTreeGeneration.h>

#include <bvh/sweep_sah_builder.hpp>
#include <bvh/triangle.hpp>

using Scalar = float;
using BoundingBox = bvh::BoundingBox<Scalar>;
using Vector3 = bvh::Vector3<Scalar>;
using Triangle = bvh::Triangle<Scalar>;
using Bvh = bvh::Bvh<Scalar>;

namespace application::file::game::phive::starlight_physics::common
{
    uint8_t compressDim(float Min, float Max, float PMin, float PMax)
    {
        float snorm = 226.0f / (PMax - PMin);
        float rmin = std::sqrtf(std::fmax((Min - PMin) * snorm, 0));
        float rmax = std::sqrtf(std::fmax((Max - PMax) * -snorm, 0));
        uint8_t a = (uint8_t)std::min(0xF, (int)std::floorf(rmin));
        uint8_t b = (uint8_t)std::min(0xF, (int)std::floorf(rmax));
        return (uint8_t)((a << 4) | b);
    }

    void compressNode(std::vector<classes::HavokClasses::hkcdCompressedAabbCodecs__Aabb6BytesCodec>& ret, hkcdStaticAabbTreeGeneration::InternalNode* node, glm::vec3 parentMin, glm::vec3 parentMax, bool root)
    {
        size_t currIndex = ret.size();
        ret.resize(ret.size() + 1);
        classes::HavokClasses::hkcdCompressedAabbCodecs__Aabb6BytesCodec compressed;

        compressed.mParent.mXyz[0].mParent = compressDim(node->mMin.x, node->mMax.x, parentMin.x, parentMax.x);
        compressed.mParent.mXyz[1].mParent = compressDim(node->mMin.y, node->mMax.y, parentMin.y, parentMax.y);
        compressed.mParent.mXyz[2].mParent = compressDim(node->mMin.z, node->mMax.z, parentMin.z, parentMax.z);

        glm::vec3 Min = compressed.DecompressMin(parentMin, parentMax);
        glm::vec3 Max = compressed.DecompressMax(parentMin, parentMax);

        if (node->mIsLeaf)
        {
            uint32_t Data = node->mPrimitive;
            compressed.mLoData.mParent = (uint16_t)(Data & 0xFFFF);
            compressed.mHiData.mParent = (uint8_t)((Data >> 16) & 0x7F);
        }
        else
        {
            compressNode(ret, node->mLeft, Min, Max, false);

            uint16_t Data = (uint16_t)((ret.size() - currIndex) / 2);
            compressed.mLoData.mParent = (uint16_t)(Data & 0xFFFF);
            compressed.mHiData.mParent = (uint8_t)(((Data >> 16) & 0x7F) | 0x80);

            compressNode(ret, node->mRight, Min, Max, false);
        }

        if (root)
        {
            compressed.mParent.mXyz[0].mParent = 0;
            compressed.mParent.mXyz[1].mParent = 0;
            compressed.mParent.mXyz[2].mParent = 0;
        }

        ret[currIndex] = compressed;
    }

    std::vector<classes::HavokClasses::hkcdCompressedAabbCodecs__Aabb6BytesCodec> hkcdStaticAabbTreeGeneration::InternalNode::buildAxis6ByteTree()
    {
        std::vector<classes::HavokClasses::hkcdCompressedAabbCodecs__Aabb6BytesCodec> Ret;
        compressNode(Ret, this, mMin, mMax, true);
        return Ret;
    }

	void hkcdStaticAabbTreeGeneration::buildFromAabbs(classes::HavokClasses::hkcdStaticAabbTree* dst, const std::vector<common::hkAabb>& aabbs)
	{
        size_t numAabbs = aabbs.size();
        if (numAabbs == 0) return;

        std::vector<BoundingBox> bboxes;
        std::vector<Vector3> centers;
        bboxes.resize(numAabbs);
		centers.resize(numAabbs);

        for (size_t i = 0; i < numAabbs; ++i)
        {
            const auto& hkAabb = aabbs[i];

            Vector3 min(hkAabb.mMin.x,
                hkAabb.mMin.y,
                hkAabb.mMin.z);

            Vector3 max(hkAabb.mMax.x,
                hkAabb.mMax.y,
                hkAabb.mMax.z);

            bboxes[i] = BoundingBox(min, max);

            centers[i] = bvh::Vector3<float>(
                (min[0] + max[0]) * 0.5f,
                (min[1] + max[1]) * 0.5f,
                (min[2] + max[2]) * 0.5f
            );
        }

        Bvh bvh;

        bvh::SweepSahBuilder<Bvh> builder(bvh);
        builder.max_leaf_size = 1;
        auto globalBBox = bvh::compute_bounding_boxes_union(bboxes.data(), numAabbs);
        builder.build(globalBBox, bboxes.data(), centers.data(), numAabbs);

        for (int i = 0; i < bvh.node_count; i++) 
        {
            if (bvh.nodes[i].is_leaf()) 
            {
                bvh.nodes[i].first_child_or_primitive = bvh.primitive_indices[bvh.nodes[i].first_child_or_primitive];
            }
        }

        std::vector<Bvh::Node> nodes;
        nodes.resize(bvh.node_count);
        for (int i = 0; i < bvh.node_count; i++)
        {
            nodes[i] = bvh.nodes[i];
        }

        std::vector<InternalNode> internalNodes;
        for (const Bvh::Node& node : nodes)
        {
            InternalNode newNode;
            newNode.mMin = glm::vec3(node.bounds[0], node.bounds[2], node.bounds[4]);
            newNode.mMax = glm::vec3(node.bounds[1], node.bounds[3], node.bounds[5]);
            newNode.mIsLeaf = node.is_leaf();
            newNode.mPrimitiveCount = node.primitive_count;
            newNode.mPrimitive = node.first_child_or_primitive;
            internalNodes.push_back(newNode);
        }


        for (size_t i = 0; i < nodes.size(); i++)
        {
            if (nodes[i].is_leaf()) continue;
            internalNodes[i].mLeft = &internalNodes[(int)nodes[i].first_child_or_primitive];
            internalNodes[i].mRight = &internalNodes[(int)nodes[i].first_child_or_primitive + 1];
        }

		dst->mTreePtr.mObj.mTree.mParent.mDomain.mMin.FromVec4(glm::vec4(internalNodes[0].mMin, 1.0f));
		dst->mTreePtr.mObj.mTree.mParent.mDomain.mMax.FromVec4(glm::vec4(internalNodes[0].mMax, 1.0f));
        dst->mTreePtr.mObj.mTree.mParent.mNodes.mElements = internalNodes[0].buildAxis6ByteTree();
	}
}