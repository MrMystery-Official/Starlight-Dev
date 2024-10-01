#include "BVHBuilder.h"

#include "bvh/vector.hpp"
#include "bvh/triangle.hpp"
#include "bvh/sweep_sah_builder.hpp"

using Vector3 = bvh::Vector3<Scalar>;
using BoundingBox = bvh::BoundingBox<Scalar>;
using Triangle = bvh::Triangle<Scalar>;

Bvh BVHBuilder::mBVH;

bool BVHBuilder::BuildBVHForDomains(const float* domains, int domainCount)
{
    // Bounding box array for processing
    std::vector<BoundingBox> bboxes;
    for (int i = 0; i < domainCount; i++) {
        bboxes.emplace_back(
            Vector3(domains[i * 6], domains[i * 6 + 1], domains[i * 6 + 2]),
            Vector3(domains[i * 6 + 3], domains[i * 6 + 4], domains[i * 6 + 5])
        );
    }

    std::vector<Vector3> centers;
    for (const auto& bbox : bboxes) {
        centers.emplace_back(
            bbox.center()
        );
    }

    // Build bvh
    bvh::SweepSahBuilder<Bvh> builder(mBVH);
    builder.max_leaf_size = 1;
    auto globalBBox = bvh::compute_bounding_boxes_union(bboxes.data(), bboxes.size());
    builder.build(globalBBox, bboxes.data(), centers.data(), bboxes.size());

    for (int i = 0; i < mBVH.node_count; i++) {
        if (mBVH.nodes[i].is_leaf()) {
            mBVH.nodes[i].first_child_or_primitive = mBVH.primitive_indices[mBVH.nodes[i].first_child_or_primitive];
        }
    }

    return true;

}

bool BVHBuilder::BuildBVHForMesh(const float* verts, const unsigned int* indices, int icount)
{
    // Triangle array for processing
    std::vector<Triangle> triangles;
    for (int i = 0; i < icount / 3; i++) {
        int i1 = (int)indices[i * 3];
        int i2 = (int)indices[i * 3 + 1];
        int i3 = (int)indices[i * 3 + 2];
        float v1x = verts[i1 * 3];
        float v1y = verts[i1 * 3 + 1];
        float v1z = verts[i1 * 3 + 2];
        float v2x = verts[i2 * 3];
        float v2y = verts[i2 * 3 + 1];
        float v2z = verts[i2 * 3 + 2];
        float v3x = verts[i3 * 3];
        float v3y = verts[i3 * 3 + 1];
        float v3z = verts[i3 * 3 + 2];
        triangles.emplace_back(
            Vector3(v1x, v1y, v1z),
            Vector3(v2x, v2y, v2z),
            Vector3(v3x, v3y, v3z)
        );
    }

    // Build bvh
    bvh::SweepSahBuilder<Bvh> builder(mBVH);
    builder.max_leaf_size = 1;
    auto [bboxes, centers] = bvh::compute_bounding_boxes_and_centers(triangles.data(), triangles.size());
    auto globalBBox = bvh::compute_bounding_boxes_union(bboxes.get(), triangles.size());
    builder.build(globalBBox, bboxes.get(), centers.get(), triangles.size());

    for (int i = 0; i < mBVH.node_count; i++) {
        if (mBVH.nodes[i].is_leaf()) {
            mBVH.nodes[i].first_child_or_primitive = mBVH.primitive_indices[mBVH.nodes[i].first_child_or_primitive];
        }
    }

    return true;

}

size_t BVHBuilder::GetNodeSize()
{
    return sizeof(Bvh::Node);
}

size_t BVHBuilder::GetBVHSize()
{
    return mBVH.node_count;
}

std::vector<Bvh::Node> BVHBuilder::GetBVHNodes()
{
    std::vector<Bvh::Node> Result;
    Result.resize(mBVH.node_count);
    for (int i = 0; i < mBVH.node_count; i++)
    {
        Result[i] = mBVH.nodes[i];
    }
    return Result;
}