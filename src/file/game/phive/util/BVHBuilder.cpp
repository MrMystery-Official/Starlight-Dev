#include "BVHBuilder.h"

#include <bvh/vector.hpp>
#include <bvh/triangle.hpp>
#include <bvh/sweep_sah_builder.hpp>
#include <glm/vec3.hpp>

using Vector3 = bvh::Vector3<Scalar>;
using BoundingBox = bvh::BoundingBox<Scalar>;
using Triangle = bvh::Triangle<Scalar>;

namespace application::file::game::phive::util
{
    Bvh BVHBuilder::gBVH;

    struct Quad
    {
        Vector3 mVertexA;
        Vector3 mVertexB;
        Vector3 mVertexC;
        Vector3 mVertexD;

        Quad(const Vector3& a, const Vector3& b, const Vector3& c, const Vector3& d) : mVertexA(a), mVertexB(b), mVertexC(c), mVertexD(d) {}
    };

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
        bvh::SweepSahBuilder<Bvh> builder(gBVH);
        builder.max_leaf_size = 1;
        auto globalBBox = bvh::compute_bounding_boxes_union(bboxes.data(), bboxes.size());
        builder.build(globalBBox, bboxes.data(), centers.data(), bboxes.size());

        for (int i = 0; i < gBVH.node_count; i++) {
            if (gBVH.nodes[i].is_leaf()) {
                gBVH.nodes[i].first_child_or_primitive = gBVH.primitive_indices[gBVH.nodes[i].first_child_or_primitive];
            }
        }

        return true;

    }

    std::pair<std::unique_ptr<bvh::BoundingBox<Scalar>[]>, std::unique_ptr<bvh::Vector3<Scalar>[]>>
        compute_bounding_boxes_and_centers_quad(const Quad* primitives, size_t primitive_count) {
        auto bounding_boxes = std::make_unique<bvh::BoundingBox<Scalar>[]>(primitive_count);
        auto centers = std::make_unique<bvh::Vector3<Scalar>[]>(primitive_count);

#pragma omp parallel for
        for (size_t i = 0; i < primitive_count; ++i) {

            bvh::BoundingBox<Scalar> bbox(primitives[i].mVertexA);
            bbox.extend(primitives[i].mVertexB);
            bbox.extend(primitives[i].mVertexC);
            bbox.extend(primitives[i].mVertexD);

            bounding_boxes[i] = bbox;
            centers[i] = (primitives[i].mVertexA + primitives[i].mVertexB + primitives[i].mVertexC + primitives[i].mVertexD) * (Scalar(1.0) / Scalar(4.0));
        }

        return std::make_pair(std::move(bounding_boxes), std::move(centers));
    }

    bool BVHBuilder::BuildBVHForQuadMesh(const float* verts, const unsigned int* indices, int icount)
    {
        // Quad array for processing
        std::vector<Quad> Quads;
        for (int i = 0; i < icount / 4; i++) 
        {
            int i1 = (int)indices[i * 4];
            int i2 = (int)indices[i * 4 + 1];
            int i3 = (int)indices[i * 4 + 2];
            int i4 = (int)indices[i * 4 + 3];
            float v1x = verts[i1 * 3];
            float v1y = verts[i1 * 3 + 1];
            float v1z = verts[i1 * 3 + 2];
            float v2x = verts[i2 * 3];
            float v2y = verts[i2 * 3 + 1];
            float v2z = verts[i2 * 3 + 2];
            float v3x = verts[i3 * 3];
            float v3y = verts[i3 * 3 + 1];
            float v3z = verts[i3 * 3 + 2];
            float v4x = verts[i4 * 3];
            float v4y = verts[i4 * 3 + 1];
            float v4z = verts[i4 * 3 + 2];
            Quads.emplace_back(
                Vector3(v1x, v1y, v1z),
                Vector3(v2x, v2y, v2z),
                Vector3(v3x, v3y, v3z),
                Vector3(v4x, v4y, v4z)
            );
        }

        // Build bvh
        bvh::SweepSahBuilder<Bvh> builder(gBVH);
        builder.max_leaf_size = 1;
        auto [bboxes, centers] = compute_bounding_boxes_and_centers_quad(Quads.data(), Quads.size());
        auto globalBBox = bvh::compute_bounding_boxes_union(bboxes.get(), Quads.size());
        builder.build(globalBBox, bboxes.get(), centers.get(), Quads.size());

        for (int i = 0; i < gBVH.node_count; i++) {
            if (gBVH.nodes[i].is_leaf()) {
                gBVH.nodes[i].first_child_or_primitive = gBVH.primitive_indices[gBVH.nodes[i].first_child_or_primitive];
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
        bvh::SweepSahBuilder<Bvh> builder(gBVH);
        builder.max_leaf_size = 1;
        auto [bboxes, centers] = bvh::compute_bounding_boxes_and_centers(triangles.data(), triangles.size());
        auto globalBBox = bvh::compute_bounding_boxes_union(bboxes.get(), triangles.size());
        builder.build(globalBBox, bboxes.get(), centers.get(), triangles.size());

        for (int i = 0; i < gBVH.node_count; i++) {
            if (gBVH.nodes[i].is_leaf()) {
                gBVH.nodes[i].first_child_or_primitive = gBVH.primitive_indices[gBVH.nodes[i].first_child_or_primitive];
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
        return gBVH.node_count;
    }

    std::vector<Bvh::Node> BVHBuilder::GetBVHNodes()
    {
        std::vector<Bvh::Node> Result;
        Result.resize(gBVH.node_count);
        for (int i = 0; i < gBVH.node_count; i++)
        {
            Result[i] = gBVH.nodes[i];
        }
        return Result;
    }
}