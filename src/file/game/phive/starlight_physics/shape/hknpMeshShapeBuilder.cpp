#include <file/game/phive/starlight_physics/shape/hknpMeshShapeBuilder.h>

#include <file/game/phive/starlight_physics/common/hkcdSimdTreeOperations.h>
#include <file/game/phive/starlight_physics/common/hkcdSimdTreeGeneration.h>
#include <util/Logger.h>
#include <util/Math.h>
#include <deque>
#include <algorithm>
#include <numeric>
#include <functional>
#include <queue>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/component_wise.hpp>

#include <bvh/bvh.hpp>
#include <bvh/binned_sah_builder.hpp>
#include <bvh/sweep_sah_builder.hpp>

namespace application::file::game::phive::starlight_physics::shape
{
    // =========================================================================
    // Module-level constants
    // =========================================================================
    static constexpr float   kVertexError = 1e-3f;
    static constexpr float   kCoplanarTol = 0.01f;
    static constexpr int     kMaxLeafFaces = 4;
    static constexpr int     kSahBinCount = 16;
    static constexpr uint32_t kUnquantizedSentinel = static_cast<uint32_t>(std::numeric_limits<int32_t>::max());

    // =========================================================================
    // Primitive key encoding
    // =========================================================================
    static inline uint32_t encodePrimitiveKey(uint32_t section, uint32_t prim, uint32_t tri)
    {
        assert(prim <= 0xff);
        return (section << 9) | (prim << 1) | tri;
    }

    // =========================================================================
    // Vertex decoding – unified path for quantized and float layouts
    // =========================================================================
    static void decodeFaceVerts(
        const classes::HavokClasses::hknpMeshShape__GeometrySection__Primitive& face,
        const uint8_t* vertBuf,
        const glm::u32vec4& origin,
        const hknpMeshShapeConversionUtils::Vertex& packer,
        glm::vec4* out)
    {
        if (origin.x != kUnquantizedSentinel)
        {
            const hknpMeshShapeBuilder::PackedVec3* qv =
                reinterpret_cast<const hknpMeshShapeBuilder::PackedVec3*>(vertBuf);
            for (int i = 0; i < 4; ++i)
            {
                const uint8_t ids[4] = {
                    face.mAId.mParent, face.mBId.mParent,
                    face.mCId.mParent, face.mDId.mParent
                };
                out[i] = packer.unpack(&qv[ids[i]].x, origin);
            }
        }
        else
        {
            const glm::vec3* rv = reinterpret_cast<const glm::vec3*>(vertBuf);
            const uint8_t ids[4] = {
                face.mAId.mParent, face.mBId.mParent,
                face.mCId.mParent, face.mDId.mParent
            };
            for (int i = 0; i < 4; ++i)
                out[i] = glm::vec4(rv[ids[i]], 0.f);
        }
    }

    // Returns the vertex buffer pointer for a chunk (quantized or raw)
    static const uint8_t* getVertBuf(const hknpMeshShapeBuilder::MeshChunk& chunk)
    {
        return chunk.mCanQuantize
            ? reinterpret_cast<const uint8_t*>(chunk.mCompressedVerts.data())
            : reinterpret_cast<const uint8_t*>(chunk.mRawVerts.data());
    }

    static common::hkAabb computeFaceAabb(
        uint32_t fi,
        const hknpMeshShapeBuilder::MeshChunk& chunk,
        const hknpMeshShapeConversionUtils::Vertex& packer)
    {
        glm::vec4 v[4];
        decodeFaceVerts(chunk.mFaces[fi], getVertBuf(chunk), chunk.mVertexOrigin, packer, v);
        common::hkAabb box;
        box.SetFromTetrahedron(v[0], v[1], v[2], v[3]);
        return box;
    }

    static classes::HavokClasses::hkFlags<classes::HavokClasses::hknpMeshShapeFlags, uint8_t>
        makePrimitiveFlags(int, const common::hkGeometry&)
    {
        classes::HavokClasses::hkFlags<classes::HavokClasses::hknpMeshShapeFlags, uint8_t> f;
        f.mStorage = 0;
        f.mStorage |= classes::HavokClasses::hknpMeshShapeFlags::IS_TRIANGLE;
        return f;
    }

    static int findVertex(const std::vector<glm::vec3>& verts, const glm::vec4& target)
    {
        assert(verts.size() <= 256);
        const glm::vec3 t(target);
        auto it = std::find_if(verts.begin(), verts.end(),
            [&](const glm::vec3& v) { return glm::all(glm::equal(t, v)); });
        return it == verts.end() ? -1 : (int)std::distance(verts.begin(), it);
    }

    // =========================================================================
    // Subtree vertex uniqueness test
    // =========================================================================
    static bool subtreeFitsInSection(
        int nodeIdx,
        const common::hkAabb& box,
        classes::HavokClasses::hkcdSimdTree& tree,
        const common::hkGeometry& geo,
        const std::vector<int>& primCounts,
        float maxExtent)
    {
        // Fast reject: spatial extent
        glm::vec4 ext = box.mMax - box.mMin;
        if (glm::any(glm::bvec3(glm::greaterThan(ext, glm::vec4(maxExtent))))) return false;

        // Fast reject: primitive count estimate
        if (static_cast<int>(1.1f * primCounts[nodeIdx]) > 256) return false;

        // Precise count: walk the subtree, hash unique vertices
        struct VertHash
        {
            uint64_t h; glm::vec3 v;
            bool operator<(const VertHash& o) const { return h < o.h; }
        };

        auto hashPt = [](const glm::vec3& p) -> uint64_t
            {
                union { float f[3]; uint32_t u[3]; } c;
                c.f[0] = p.x; c.f[1] = p.y; c.f[2] = p.z;
                return (uint64_t(c.u[0]) * 73856093u) ^ (uint64_t(c.u[1]) * 19349663u) ^ (uint64_t(c.u[2]) * 83492791u);
            };

        std::vector<VertHash> all;
        common::hkcdSimdTreeOperations::FixedStack<uint32_t> stk(nodeIdx);
        do
        {
            auto& node = tree.mNodes.mElements[stk.pop()];
            auto activeMask = node.mParent.ActiveMask();

            if (node.mIsLeaf)
            {
                for (int i = 0; i < 4; ++i)
                {
                    if (!activeMask[i]) continue;
                    glm::vec4 v[4];
                    geo.getTriangle(node.mData[i].mParent, v);
                    for (int j = 0; j < 3; ++j)
                        all.push_back({ hashPt(glm::vec3(v[j])), glm::vec3(v[j]) });
                }
            }
            else
            {
                for (int i = 0; i < 4; ++i)
                    if (activeMask[i]) stk.push(node.mData[i].mParent);
            }
        } while (!stk.empty());

        std::sort(all.begin(), all.end());
        auto newEnd = std::unique(all.begin(), all.end(), [](const VertHash& a, const VertHash& b)
            {
                return a.h == b.h && glm::all(glm::equal(a.v, b.v));
            });

        return std::distance(all.begin(), newEnd) <= 256;
    }

    // =========================================================================
    // SAH BVH construction
    // =========================================================================
    struct BvhPoint { glm::vec3 centroid; int faceId; };
    struct SahBucket { common::hkAabb bounds; int count = 0; };

    static int sahBinPartition(std::vector<BvhPoint>& pts, const std::vector<common::hkAabb>& boxes, int lo, int hi)
    {
        if (hi - lo <= kMaxLeafFaces) return lo;

        common::hkAabb centBounds;
        std::for_each(pts.begin() + lo, pts.begin() + hi, [&](const BvhPoint& p)
            {
                centBounds.IncludePoint(glm::vec4(p.centroid, 0.f));
            });

        float best = std::numeric_limits<float>::max();
        int bestAxis = -1;
        float bestPos = 0.f;

        for (int ax = 0; ax < 3; ++ax)
        {
            float loC = centBounds.mMin[ax], hiC = centBounds.mMax[ax];
            if (hiC - loC < 1e-5f) continue;

            SahBucket bins[kSahBinCount] = {};
            float scale = kSahBinCount / (hiC - loC);

            for (int i = lo; i < hi; ++i)
            {
                int b = std::min((int)(scale * (pts[i].centroid[ax] - loC)), kSahBinCount - 1);
                bins[b].count++;
                bins[b].bounds.IncludeAabb(boxes[pts[i].faceId]);
            }

            // Evaluate all split planes by sweeping left and right in one pass each
            common::hkAabb prefixBox[kSahBinCount];
            int prefixCnt[kSahBinCount] = {};
            prefixBox[0] = bins[0].bounds; prefixCnt[0] = bins[0].count;
            for (int s = 1; s < kSahBinCount; ++s)
            {
                prefixBox[s] = prefixBox[s - 1]; prefixBox[s].IncludeAabb(bins[s].bounds);
                prefixCnt[s] = prefixCnt[s - 1] + bins[s].count;
            }

            common::hkAabb suffixBox; int suffixCnt = 0;
            for (int s = kSahBinCount - 2; s >= 0; --s)
            {
                suffixBox.IncludeAabb(bins[s + 1].bounds);
                suffixCnt += bins[s + 1].count;
                if (!prefixCnt[s] || !suffixCnt) continue;
                float cost = prefixBox[s].SurfaceArea() * prefixCnt[s] + suffixBox.SurfaceArea() * suffixCnt;
                if (cost < best)
                {
                    best = cost; bestAxis = ax;
                    bestPos = loC + (s + 1) * (hiC - loC) / kSahBinCount;
                }
            }
        }

        if (bestAxis == -1) return lo + (hi - lo) / 2;

        auto it = std::partition(pts.begin() + lo, pts.begin() + hi,
            [&](const BvhPoint& p) { return p.centroid[bestAxis] < bestPos; });
        return (int)std::distance(pts.begin(), it);
    }

    static int buildBvhNode(std::vector<BvhPoint>& pts, const std::vector<common::hkAabb>& boxes, int lo, int hi, classes::HavokClasses::hkcdSimdTree& tree)
    {
        const int idx = (int)tree.mNodes.mElements.size();
        tree.mNodes.mElements.emplace_back();
        const int n = hi - lo;

        if (n <= kMaxLeafFaces)
        {
            tree.mNodes.mElements[idx].ClearLeaf();
            for (int i = 0; i < 4; ++i)
            {
                if (i < n) tree.mNodes.mElements[idx].SetChild(i, boxes[pts[lo + i].faceId], pts[lo + i].faceId);
                else { tree.mNodes.mElements[idx].mData[i].mParent = (uint32_t)-1; tree.mNodes.mElements[idx].ClearAabb(i); }
            }
            return idx;
        }

        tree.mNodes.mElements[idx].ClearInternal();
        int mid = sahBinPartition(pts, boxes, lo, hi);
        int q0 = (mid - lo > kMaxLeafFaces) ? sahBinPartition(pts, boxes, lo, mid) : lo;
        int q1 = (hi - mid > kMaxLeafFaces) ? sahBinPartition(pts, boxes, mid, hi) : mid;
        int segs[5] = { lo, q0, mid, q1, hi };

        for (int i = 0; i < 4; ++i)
        {
            if (segs[i] >= segs[i + 1]) { tree.mNodes.mElements[idx].mData[i].mParent = 0; tree.mNodes.mElements[idx].ClearAabb(i); continue; }
            int child = buildBvhNode(pts, boxes, segs[i], segs[i + 1], tree);
            // Re-fetch idx pointer since emplace_back may have reallocated
            common::hkAabb cb; tree.mNodes.mElements[child].mParent.GetCompoundAabb(&cb);
            tree.mNodes.mElements[idx].SetChild(i, cb, child);
        }
        return idx;
    }

    static void buildTopLevelBvh(const common::hkGeometry& geo, classes::HavokClasses::hkcdSimdTree& outTree)
    {
        const int N = (int)geo.mTriangles.size();
        std::vector<common::hkAabb> boxes(N);

        common::hkcdSimdTreeGeneration::TreeAssembler asm2;
        asm2.mPrimitives.reserve(N);
        asm2.mOrderBySize = true;
        asm2.mSahLeafSort = true;

        for (int i = 0; i < N; ++i)
        {
            glm::vec4 v[4]; geo.getTriangle(i, v);
            boxes[i].SetFromTriangle(v[0], v[1], v[2]);
            glm::vec4 c; boxes[i].GetCenter(c);
            asm2.mPrimitives.emplace_back(c, i);
        }
        common::hkcdSimdTreeGeneration::buildFromAabbs(outTree, asm2, boxes.data());
    }

    // =========================================================================
    // hknpMeshShapeBuilder — member implementations
    // =========================================================================

    bool hknpMeshShapeBuilder::isDegenerate(const glm::vec4& a4, const glm::vec4& b4, const glm::vec4& c4)
    {
        constexpr float kMinAngle = 4e-4f;
        const glm::vec3 a(a4), b(b4), c(c4);

        // Compute edge vectors and their squared lengths simultaneously
        const glm::vec3 edges[3] = { a - b, b - c, c - a };
        const float lenSq[3] = {
            glm::dot(edges[0], edges[0]),
            glm::dot(edges[1], edges[1]),
            glm::dot(edges[2], edges[2])
        };

        const glm::vec3 cross = glm::cross(edges[0], edges[1]);
        const float crossSq = glm::dot(cross, cross);

        if (crossSq <= 0.f) return true;

        // Compare cross product magnitude to the most constrained angle formed by the longest edge pair
        const float maxPair = std::max({ lenSq[0] * lenSq[1], lenSq[1] * lenSq[2], lenSq[2] * lenSq[0] });
        return crossSq < (kMinAngle * kMinAngle) * maxPair;
    }

    void hknpMeshShapeBuilder::compressChunkVerts(
        const hknpMeshShapeConversionUtils::Vertex& packer,
        MeshChunk& chunk,
        float /*maxErr*/)
    {
        if (!chunk.mCanQuantize)
        {
            for (auto& v : chunk.mRawVerts)
                v = packer.addError(glm::vec4(v, 0.f));
            return;
        }

        chunk.mVertexOrigin = packer.getSectionOffset(chunk.mUnexpandedBounds);
        for (int axis = 0; axis < 3; ++axis)
            assert(chunk.mVertexOrigin[axis] != kUnquantizedSentinel);

        chunk.mCompressedVerts.resize(chunk.mRawVerts.size());
        std::transform(chunk.mRawVerts.begin(), chunk.mRawVerts.end(), chunk.mCompressedVerts.begin(),
            [&](const glm::vec3& v) -> PackedVec3
            {
                PackedVec3 out;
                packer.pack(glm::vec4(v, 0.f), chunk.mVertexOrigin, &out.x);
                return out;
            });
    }

    void hknpMeshShapeBuilder::tagQuadPlanarity(MeshChunk& chunk, int faceIdx, bool isFlat)
    {
        int off[2] = { 0, 0 };
        bool swapped;
        chunk.mFaces[faceIdx].SetIsFlatConvexQuad(isFlat, swapped, off);
    }

    void hknpMeshShapeBuilder::evaluateAllQuads(MeshChunk& chunk)
    {
        using Flags = classes::HavokClasses::hknpMeshShapeFlags;
        for (int i = 0; i < (int)chunk.mFaces.size(); ++i)
        {
            if (!chunk.mFaces[i].IsTriangle())
                tagQuadPlanarity(chunk, i, chunk.mFaceFlags[i].IsFlagSet(Flags::IS_FLAT_CONVEX_QUAD));
        }
    }

    void hknpMeshShapeBuilder::readFaceVerts(
        const classes::HavokClasses::hknpMeshShape__GeometrySection__Primitive& face,
        const uint8_t* vertBuf, const glm::u32vec4& origin,
        const hknpMeshShapeConversionUtils::Vertex& packer, glm::vec4* out)
    {
        decodeFaceVerts(face, vertBuf, origin, packer, out);
    }

    void hknpMeshShapeBuilder::readFaceVerts(
        uint32_t faceIdx, const uint8_t* vertBuf,
        const classes::HavokClasses::hknpMeshShape__GeometrySection__Primitive* faces,
        const glm::u32vec4& origin,
        const hknpMeshShapeConversionUtils::Vertex& packer, glm::vec4* out)
    {
        decodeFaceVerts(faces[faceIdx], vertBuf, origin, packer, out);
    }

    bool hknpMeshShapeBuilder::isQuadPlanarAndConvex(
        const glm::vec4& p0, const glm::vec4& p1, const glm::vec4& p2, const glm::vec4& p3,
        float tol, float* outErr, QuadAreaPolicy policy)
    {
        const glm::vec3 A(p0), B(p1), C(p2), D(p3);
        const glm::vec3 AB = B - A, AC = C - A, AD = D - A;
        const glm::vec3 n = glm::cross(AB, AC);
        const float nLenSq = glm::dot(n, n);
        const float oop = std::abs(glm::dot(A - D, n));

        if (outErr) *outErr = oop / std::sqrt(nLenSq);
        if (oop * oop > tol * tol * nLenSq) return false;

        if (policy == QuadAreaPolicy::EnforceAreaCheck)
        {
            if (glm::dot(glm::cross(AC, AD), glm::cross(AC, AD)) < nLenSq * 0.01f) return false;
        }

        return glm::dot(glm::cross(AB, AD), glm::cross(D - C, B - C)) >= 0.f;
    }

    void hknpMeshShapeBuilder::pruneDegenerateFaces(
        MeshChunk& chunk, int fi, const glm::vec4 v[4],
        bool& outModified, bool& outDrop, int& outDegCount)
    {
        using Flags = classes::HavokClasses::hknpMeshShapeFlags;
        outDrop = outModified = false;

        auto& flags = chunk.mFaceFlags[fi];
        auto& face = chunk.mFaces[fi];
        const bool isQuad = flags.IsFlagSet(Flags::IS_QUAD);
        const int numTris = isQuad ? 2 : 1;

        bool deg[2] = { isDegenerate(v[0], v[1], v[2]), false };
        if (numTris == 2) deg[1] = isDegenerate(v[0], v[2], v[3]);

        outDegCount += (int)deg[0] + (int)deg[1];
        if (!deg[0] && !deg[1]) return;

        outModified = true;
        if (numTris == 1 || (deg[0] && deg[1])) { outDrop = true; return; }

        // Salvage one triangle from a quad by collapsing
        if (deg[0]) { face.mBId = face.mCId; face.mCId = face.mDId; }
        face.mDId = face.mCId;

        for (auto f : { Flags::IS_FLAT_CONVEX_QUAD, Flags::IS_QUAD, Flags::DISABLE_ALL_EDGES_TRIANGLE_1, Flags::DISABLE_ALL_EDGES_TRIANGLE_2 })
            flags.SetFlag(f, false);
        flags.SetFlag(Flags::IS_TRIANGLE, true);
    }

    void hknpMeshShapeBuilder::dropUnreferencedVerts(MeshChunk& chunk)
    {
        // Build a compact usage set from all face vertex indices
        std::vector<bool> used(chunk.mRawVerts.size(), false);
        for (const auto& f : chunk.mFaces)
        {
            used[f.mAId.mParent] = used[f.mBId.mParent] =
                used[f.mCId.mParent] = used[f.mDId.mParent] = true;
        }

        // Compute prefix-sum remap table: new index = number of used verts before position i
        std::vector<int> remap(used.size());
        int count = 0;
        for (int i = 0; i < (int)used.size(); ++i)
        {
            remap[i] = count;
            if (used[i])
            {
                if (chunk.mCanQuantize) chunk.mCompressedVerts[count] = chunk.mCompressedVerts[i];
                else                    chunk.mRawVerts[count] = chunk.mRawVerts[i];
                ++count;
            }
        }
        chunk.mRawVerts.resize(count);
        chunk.mCompressedVerts.resize(count);

        for (auto& f : chunk.mFaces)
        {
            f.mAId.mParent = (uint8_t)remap[f.mAId.mParent];
            f.mBId.mParent = (uint8_t)remap[f.mBId.mParent];
            f.mCId.mParent = (uint8_t)remap[f.mCId.mParent];
            f.mDId.mParent = (uint8_t)remap[f.mDId.mParent];
        }
    }

    void hknpMeshShapeBuilder::compactLocalBvh(const std::vector<int>& dead, MeshChunk& chunk)
    {
        // Build a forward map: old index → new index (-1 = removed)
        std::vector<int> newIdx(chunk.mLocalBvh.size(), -1);
        int out = 0;
        for (int i = 0; i < (int)chunk.mLocalBvh.size(); ++i)
        {
            if (!std::binary_search(dead.begin(), dead.end(), i))
                newIdx[i] = out++;
        }

        // Compact the array and fixup child references
        auto& nodes = chunk.mLocalBvh;
        for (int i = 0; i < (int)nodes.size(); ++i)
        {
            if (newIdx[i] < 0) continue;
            nodes[newIdx[i]] = nodes[i];
            auto& dst = nodes[newIdx[i]];
            if (!dst.IsLeaf())
            {
                for (int ci = 0; ci < 4; ++ci)
                    if (dst.IsChildValid(ci))
                        dst.mData[ci].mParent = (uint8_t)newIdx[dst.mData[ci].mParent];
                dst.ConfigureAsLeafOrInternal(false);
            }
        }
        nodes.resize(out);
    }

    // =========================================================================
    // repairChunkAfterQuantization
    //
    // Pass 1: Scan all faces, compute which need their quad-planarity revoked,
    //         and which are fully degenerate (should be dropped).
    // Pass 2: Erase dropped faces in reverse index order so that
    //         swap-with-last semantics remain consistent.
    // Pass 3: Apply quad planarity updates to surviving faces.
    // Pass 4: dropUnreferencedVerts / compact BVH.
    // =========================================================================
    bool hknpMeshShapeBuilder::repairChunkAfterQuantization(
        const hknpMeshShapeConversionUtils::Vertex& packer,
        MeshChunk& chunk,
        float coplanarTol,
        bool dropDeg,
        int& outDegCount)
    {
        using Flags = classes::HavokClasses::hknpMeshShapeFlags;
        outDegCount = 0;

        const uint8_t* vBuf = getVertBuf(chunk);

        // Snapshot which BVH nodes are currently leaves before any mutation
        std::vector<bool> wasLeaf(chunk.mLocalBvh.size());
        for (int ni = 0; ni < (int)chunk.mLocalBvh.size(); ++ni)
            wasLeaf[ni] = chunk.mLocalBvh[ni].IsLeaf();

        // Pass 1: classify every face
        struct FaceDecision { bool revokeQuad; bool drop; bool modified; };
        std::vector<FaceDecision> decisions(chunk.mFaces.size());

        for (int fi = 0; fi < (int)chunk.mFaces.size(); ++fi)
        {
            auto& d = decisions[fi];
            d = { false, false, false };

            glm::vec4 v[4];
            decodeFaceVerts(chunk.mFaces[fi], vBuf, chunk.mVertexOrigin, packer, v);

            // Determine if a previously-marked flat convex quad is still valid after quantization
            if (chunk.mFaceFlags[fi].IsFlagSet(Flags::IS_FLAT_CONVEX_QUAD))
            {
                if (!isQuadPlanarAndConvex(v[0], v[1], v[2], v[3], coplanarTol))
                {
                    d.revokeQuad = true;
                    d.modified = true;

                    // Re-decode after planarity will be revoked (affects vertex ordering)
                    chunk.mFaceFlags[fi].SetFlag(Flags::IS_FLAT_CONVEX_QUAD, false);
                    tagQuadPlanarity(chunk, fi, false);
                    decodeFaceVerts(chunk.mFaces[fi], vBuf, chunk.mVertexOrigin, packer, v);
                }
            }

            if (dropDeg)
            {
                bool modified = false;
                pruneDegenerateFaces(chunk, fi, v, modified, d.drop, outDegCount);
                d.modified |= modified;
            }
        }

        // Pass 2: Remove dropped faces in reverse order so swap-with-last stays valid.
        // Build a parent-child map once, then process removals.
        {
            const int leafBase = (int)chunk.mLocalBvh.size();
            struct Slot { uint8_t parentIdx; uint8_t childSlot; bool valid; };
            std::vector<Slot> leafSlots(chunk.mFaces.size(), { 0, 0, false });
            std::vector<Slot> nodeSlots(chunk.mLocalBvh.size(), { 0, 0, false });

            for (int ni = 0; ni < (int)chunk.mLocalBvh.size(); ++ni)
            {
                const auto& n = chunk.mLocalBvh[ni];
                for (int ci = 0; ci < 4; ++ci)
                {
                    if (!n.IsChildValid(ci)) continue;
                    if (n.IsLeaf())
                    {
                        auto& s = leafSlots[n.mData[ci].mParent];
                        s = { (uint8_t)ni, (uint8_t)ci, true };
                    }
                    else
                    {
                        auto& s = nodeSlots[n.mData[ci].mParent];
                        s = { (uint8_t)ni, (uint8_t)ci, true };
                    }
                }
            }

            // Collect drop indices in sorted descending order
            std::vector<int> toRemove;
            for (int fi = (int)decisions.size() - 1; fi >= 0; --fi)
                if (decisions[fi].drop) toRemove.push_back(fi);
            // Already descending since we iterated backwards

            std::vector<int> deadBvhNodes;
            bool sectionEmpty = false;

            for (int faceIdx : toRemove)
            {
                const int last = (int)chunk.mFaces.size() - 1;
                chunk.mFaces.erase(chunk.mFaces.begin() + faceIdx);
                chunk.mMaterialIds.erase(chunk.mMaterialIds.begin() + faceIdx);
                chunk.mFaceFlags.erase(chunk.mFaceFlags.begin() + faceIdx);
                decisions.erase(decisions.begin() + faceIdx);

                // Redirect the 'last' face's node slot to point to faceIdx (swap-with-last)
                const Slot& swapSlot = leafSlots[last];
                assert(swapSlot.valid);
                chunk.mLocalBvh[swapSlot.parentIdx].mData[swapSlot.childSlot].mParent = (uint8_t)faceIdx;
                leafSlots[faceIdx] = swapSlot;
                leafSlots[last] = { 0, 0, false };

                // Walk up the BVH from the removed face and clear empty nodes
                const Slot removed = leafSlots[faceIdx]; // the original removed face's slot
                classes::HavokClasses::hknpAabb8 empty; empty.SetEmpty();
                int activeKids = 0;
                bool searching = true;
                for (int ni = 0; ni < (int)chunk.mLocalBvh.size() && searching; ++ni)
                {
                    auto& node = chunk.mLocalBvh[ni];
                    if (!node.IsLeaf()) continue;
                    for (int ci = 0; ci < 4; ++ci)
                    {
                        if (!node.IsChildValid(ci)) continue;
                        if ((int)node.mData[ci].mParent >= (int)chunk.mFaces.size())
                        {
                            int curNi = ni;
                            int curCi = ci;
                            do
                            {
                                auto& curNode = chunk.mLocalBvh[curNi];
                                curNode.mParent.SetAabb(curCi, empty);
                                curNode.mData[curCi].mParent = 0;
                                activeKids = curNode.CountActiveAabbs();
                                if (activeKids == 0) deadBvhNodes.push_back(curNi);

                                const Slot& upSlot = nodeSlots[curNi];
                                if (!upSlot.valid || activeKids != 0) break;
                                curNi = upSlot.parentIdx;
                                curCi = upSlot.childSlot;
                            } while (true);

                            if (activeKids == 0 && !nodeSlots[curNi].valid)
                                sectionEmpty = true;

                            searching = false;
                            break;
                        }
                    }
                }

                if (sectionEmpty)
                {
                    assert(chunk.mFaces.empty());
                    chunk.mLocalBvh.clear();
                    return true;
                }
            }

            if (!deadBvhNodes.empty())
            {
                std::sort(deadBvhNodes.begin(), deadBvhNodes.end());
                deadBvhNodes.erase(std::unique(deadBvhNodes.begin(), deadBvhNodes.end()), deadBvhNodes.end());
            }

            // Pass 3: compact BVH node children and remove dead nodes
            for (int ni = 0; ni < (int)chunk.mLocalBvh.size(); ++ni)
                if (!chunk.mLocalBvh[ni].IsEmpty())
                    chunk.mLocalBvh[ni].CompactChildren(wasLeaf[ni]);

            if (!deadBvhNodes.empty())
                compactLocalBvh(deadBvhNodes, chunk);
        }

        // Pass 4: drop unused vertices if any face was modified
        const bool anyModified = std::any_of(decisions.begin(), decisions.end(),
            [](const FaceDecision& d) { return d.modified; });
        if (anyModified) dropUnreferencedVerts(chunk);

        return false;
    }

    void hknpMeshShapeBuilder::locateSectionInTree(
        const classes::HavokClasses::hkcdSimdTree& tree,
        int sectionIdx, int& outParent, int& outSlot)
    {
        // Walk backwards to find the leaf node referencing sectionIdx
        outParent = outSlot = -1;
        for (int ni = (int)tree.mNodes.mElements.size() - 1; ni >= 0; --ni)
        {
            const auto& node = tree.mNodes.mElements[ni];
            if (!node.mIsLeaf) continue;
            for (int ci = 0; ci < 4; ++ci)
            {
                if (node.IsLeafValid(ci) && (int)node.mData[ci].mParent == sectionIdx)
                {
                    outParent = ni; outSlot = ci; return;
                }
            }
        }
    }

    void hknpMeshShapeBuilder::evictSection(
        std::vector<MeshChunk>& chunks,
        int idx,
        classes::HavokClasses::hkcdSimdTree& tree)
    {
        // Find and clear the section's slot in the top-level tree
        int pIdx, pSlot;
        locateSectionInTree(tree, idx, pIdx, pSlot);
        assert(pIdx != -1);

        auto& parent = tree.mNodes.mElements[pIdx];
        common::hkAabb oldBox; parent.mParent.GetAabb(pSlot, &oldBox);

        glm::vec4 center; oldBox.GetCenter(center);
        reinterpret_cast<uint32_t*>(&center[0])[3] = static_cast<uint32_t>(idx) | 0x3f000000;
        common::hkAabb empty; empty.SetEmpty();
        tree.RefitLeaf(center, empty);

        // Clear inactive slots across all nodes
        for (auto& node : tree.mNodes.mElements)
        {
            const auto active = node.mParent.ActiveMask();
            for (int i = 0; i < 4; ++i)
            {
                if (!active[i])
                    node.mIsLeaf ? node.ClearLeafData(i) : node.ClearInternalData(i);
            }
        }

        parent.ClearAabb(pSlot);
        parent.ClearLeafData(pSlot);

        // Remap last section to fill the gap
        const int last = (int)chunks.size() - 1;
        if (idx != last)
        {
            int lPIdx, lPSlot;
            locateSectionInTree(tree, last, lPIdx, lPSlot);
            assert(lPIdx != -1);
            tree.mNodes.mElements[lPIdx].SetChildData(lPSlot, static_cast<uint32_t>(idx));
        }
        chunks.erase(chunks.begin() + idx);
    }

    void hknpMeshShapeBuilder::resyncSection(
        const hknpMeshShapeConversionUtils::Vertex& packer,
        MeshChunk& chunk,
        classes::HavokClasses::hkcdSimdTree& tree,
        int sectionIdx)
    {
        hknpMeshShapeConversionUtils::Aabb8 conv(chunk.mSpatialBounds);
        common::hkAabb newDomain; newDomain.SetEmpty();

        // Refit local BVH bottom-up and accumulate the section domain
        for (int ni = (int)chunk.mLocalBvh.size() - 1; ni >= 0; --ni)
        {
            auto& node = chunk.mLocalBvh[ni];
            if (node.IsLeaf())
            {
                for (int ci = 0; ci < 4; ++ci)
                {
                    if (!node.IsChildValid(ci)) continue;
                    const common::hkAabb fbox = computeFaceAabb(node.mData[ci].mParent, chunk, packer);
                    newDomain.IncludeAabb(fbox);
                    assert(chunk.mSpatialBounds.Contains(fbox));
                    const auto q = conv.convertAabb(fbox);
                    if (q != node.mParent.GetAabb(ci)) node.mParent.SetAabb(ci, q);
                }
            }
            else
            {
                for (int ci = 0; ci < 4; ++ci)
                    if (node.IsChildValid(ci))
                        node.mParent.SetAabb(ci, chunk.mLocalBvh[node.mData[ci].mParent].mParent.GetCompoundAabb());
            }
        }

        glm::vec4 dc = (chunk.mUnexpandedBounds.mMin + chunk.mUnexpandedBounds.mMax) * 0.5f;
        reinterpret_cast<uint32_t*>(&dc[0])[3] = static_cast<uint32_t>(sectionIdx) | 0x3f000000;
        tree.RefitLeaf(dc, newDomain);
    }

    void hknpMeshShapeBuilder::sortFacesByMaterial(MeshChunk& chunk)
    {
        const int n = (int)chunk.mFaces.size();
        if (n <= 1) return;

        // Build a combined sort record so we only sort once
        struct SortRecord
        {
            uint16_t material;
            int      origIdx;
        };
        std::vector<SortRecord> recs(n);
        for (int i = 0; i < n; ++i) recs[i] = { chunk.mMaterialIds[i], i };
        std::stable_sort(recs.begin(), recs.end(),
            [](const SortRecord& a, const SortRecord& b) { return a.material < b.material; });

        // Build forward and inverse permutations in one pass
        std::vector<int> fwd(n), inv(n);
        for (int i = 0; i < n; ++i) { fwd[i] = recs[i].origIdx; inv[fwd[i]] = i; }

        // Apply permutation to all face arrays using the inverse (destination) permutation
        auto applyPerm = [&](auto& arr, const std::vector<int>& perm)
            {
                std::remove_reference_t<decltype(arr)> tmp(n);
                for (int i = 0; i < n; ++i) tmp[i] = arr[perm[i]];
                arr = std::move(tmp);
            };

        applyPerm(chunk.mFaces, fwd);
        applyPerm(chunk.mFaceFlags, fwd);
        applyPerm(chunk.mMaterialIds, fwd);

        // Patch BVH leaf node child references using the inverse permutation
        for (auto& node : chunk.mLocalBvh)
        {
            if (!node.IsLeaf()) continue;
            for (int i = 0; i < 4; ++i)
                if (node.IsChildValid(i))
                    node.mData[i].mParent = (uint8_t)inv[node.mData[i].mParent];
            node.ConfigureAsLeafOrInternal(true);
        }
    }

    std::vector<classes::HavokClasses::hknpMeshShape__ShapeTagTableEntry>
        hknpMeshShapeBuilder::buildMaterialTable(const std::vector<MeshChunk>& chunks)
    {
        std::vector<classes::HavokClasses::hknpMeshShape__ShapeTagTableEntry> tbl;

        classes::HavokClasses::hknpMeshShape__ShapeTagTableEntry entry;
        entry.mMeshPrimitiveKey.mParent = 0;
        entry.mShapeTag.mParent = !chunks.empty() ? chunks[0].mMaterialIds[0] : 0xFFFF;
        tbl.push_back(entry);

        for (int si = 0; si < (int)chunks.size(); ++si)
        {
            for (int pi = 0; pi < (int)chunks[si].mMaterialIds.size(); ++pi)
            {
                const uint16_t mat = chunks[si].mMaterialIds[pi];
                if (mat != tbl.back().mShapeTag.mParent)
                {
                    entry.mMeshPrimitiveKey.mParent = encodePrimitiveKey(si, pi, 0);
                    entry.mShapeTag.mParent = mat;
                    tbl.push_back(entry);
                }
            }
        }

        entry.mMeshPrimitiveKey.mParent = encodePrimitiveKey((uint32_t)chunks.size(), 0xff, 0);
        entry.mShapeTag.mParent = 0xFFFF;
        tbl.emplace_back(entry);
        return tbl;
    }

    void hknpMeshShapeBuilder::countPrimitivesPerNode(
        classes::HavokClasses::hkcdSimdTree& tree, std::vector<int>& outCounts)
    {
        assert(tree.mIsCompact);
        const int nodeCount = (int)tree.mNodes.mElements.size();
        outCounts.assign(nodeCount, 0);

        // Bottom-up accumulation: leaves report their face count, internal nodes sum children
        for (int ni = nodeCount - 1; ni >= 0; --ni)
        {
            auto& node = tree.mNodes.mElements[ni];
            outCounts[ni] = node.mIsLeaf
                ? node.CountLeaves()
                : std::accumulate(std::begin(node.mData), std::end(node.mData), 0,
                    [&](int acc, const auto& d) { return acc + outCounts[d.mParent]; });
        }
    }

    void hknpMeshShapeBuilder::populateChunkFromSubtree(
        const common::hkGeometry& geo,
        uint32_t subtreeRoot,
        const classes::HavokClasses::hkcdSimdTree__Node* nodes,
        std::vector<MeshChunk>& chunks,
        float maxErr)
    {
        MeshChunk& chunk = chunks.emplace_back();
        nodes[subtreeRoot].mParent.GetCompoundAabb(&chunk.mSpatialBounds);
        chunk.mUnexpandedBounds = chunk.mSpatialBounds;
        chunk.mSpatialBounds.ExpandBy(maxErr);
        chunk.mSpatialBounds.mMin[3] = chunk.mSpatialBounds.mMax[3] = 0.f;

        hknpMeshShapeConversionUtils::Aabb8 conv(chunk.mSpatialBounds);
        chunk.mQuantBitOffset = conv.mBitOffset16;
        chunk.mInvBitScale = conv.mBitScaleInv;

        // BFS traversal using a queue of {node ptr, parent local-bvh index}
        using WorkItem = std::pair<const classes::HavokClasses::hkcdSimdTree__Node*, int>;
        std::queue<WorkItem> bfsQueue;
        bfsQueue.push({ &nodes[subtreeRoot], -1 });

        std::vector<bool> isLeafRecord;

        while (!bfsQueue.empty())
        {
            auto [pNode, parentNi] = bfsQueue.front();
            bfsQueue.pop();

            const int curNi = (int)chunk.mLocalBvh.size();
            auto& aNode = chunk.mLocalBvh.emplace_back();
            aNode.Clear();
            isLeafRecord.push_back(pNode->mIsLeaf);

            conv.convertAabb(
                hknpMeshShapeConversionUtils::Aabb8::clipFourAabbsToDomain(chunk.mSpatialBounds, pNode->mParent),
                aNode.mParent);
            for (int i = 0; i < 4; ++i)
                if (!pNode->IsDataValid(i)) aNode.mParent.ClearAabb(i);

            // Link ourselves into the parent's first free child slot
            if (parentNi != -1)
            {
                auto& par = chunk.mLocalBvh[parentNi];
                for (int i = 0; i < 4; ++i)
                    if (par.mData[i].mParent == 0) { par.mData[i].mParent = (uint8_t)curNi; break; }
            }

            if (pNode->mIsLeaf)
            {
                for (int i = 0; i < 4; ++i)
                {
                    if (!pNode->IsLeafValid(i)) continue;
                    const int pid = pNode->mData[i].mParent;
                    chunk.mMaterialIds.push_back(geo.mTriangles[pid].mMaterial);

                    auto& face = chunk.mFaces.emplace_back();
                    glm::vec4 v[4]; geo.getTriangle(pid, v);

                    auto* vIds = &face.mAId;
                    for (int vi = 0; vi < 3; ++vi)
                    {
                        int idx = findVertex(chunk.mRawVerts, v[vi]);
                        if (idx == -1)
                        {
                            chunk.mRawVerts.emplace_back(v[vi]);
                            idx = (int)chunk.mRawVerts.size() - 1;
                            assert(idx <= 256);
                        }
                        *vIds++ = (classes::HavokClasses::hkUint8)idx;
                    }
                    face.mDId.mParent = face.mCId.mParent;
                    aNode.mData[i].mParent = (uint8_t)(chunk.mFaces.size() - 1);
                    chunk.mFaceFlags.push_back(makePrimitiveFlags(pid, geo));
                }
            }
            else
            {
                // Queue children (forward order for BFS; parent stack was reverse for DFS)
                for (int i = 0; i < 4; ++i)
                    if (pNode->IsInternalValid(i))
                        bfsQueue.push({ &nodes[pNode->mData[i].mParent], curNi });
            }

            aNode.ConfigureAsLeafOrInternal(pNode->mIsLeaf);
        }

        // Re-apply leaf/internal flags (ConfigureAsLeafOrInternal above is the canonical set)
        for (int ni = 0; ni < (int)chunk.mLocalBvh.size(); ++ni)
        {
            chunk.mLocalBvh[ni].ConfigureAsLeafOrInternal(isLeafRecord[ni]);
            assert(chunk.mLocalBvh[ni].IsLeaf() == isLeafRecord[ni]);
        }
    }

    void hknpMeshShapeBuilder::populateChunkFromFaceList(
        const common::hkGeometry& geo,
        const int* faceIds, int faceCount,
        std::vector<MeshChunk>& chunks,
        float maxErr, float maxExtent)
    {
        MeshChunk& chunk = chunks.emplace_back();
        common::hkAabb totalBox; totalBox.SetEmpty();
        common::hkAabb faceBoxes[4];

        for (int i = 0; i < faceCount; ++i)
        {
            glm::vec4 v[4]; geo.getTriangle(faceIds[i], v);
            faceBoxes[i].SetEmpty();
            for (int vi = 0; vi < 3; ++vi) faceBoxes[i].IncludePoint(v[vi]);

            const bool faceExtentOk = glm::all(glm::bvec3(glm::lessThanEqual(
                faceBoxes[i].mMax - faceBoxes[i].mMin, glm::vec4(maxExtent))));
            chunk.mCanQuantize &= faceExtentOk;

            const int base = (int)chunk.mRawVerts.size();
            for (int vi = 0; vi < 3; ++vi) chunk.mRawVerts.emplace_back(v[vi]);

            auto& face = chunk.mFaces.emplace_back();
            face.mAId.mParent = (uint8_t)base;
            face.mBId.mParent = (uint8_t)(base + 1);
            face.mCId.mParent = (uint8_t)(base + 2);
            face.mDId = face.mCId;

            chunk.mFaceFlags.push_back(makePrimitiveFlags(faceIds[i], geo));
            chunk.mMaterialIds.push_back(geo.mTriangles[faceIds[i]].mMaterial);
            totalBox.IncludePoints(v, 3);
        }

        chunk.mUnexpandedBounds = chunk.mSpatialBounds = totalBox;
        chunk.mSpatialBounds.ExpandBy(maxErr);
        chunk.mCanQuantize &= glm::all(glm::bvec3(glm::lessThanEqual(
            totalBox.mMax - totalBox.mMin, glm::vec4(maxExtent))));

        if (!chunk.mCanQuantize)
            chunk.mVertexOrigin = glm::u32vec4(std::numeric_limits<uint32_t>::max());

        hknpMeshShapeConversionUtils::Aabb8 conv(chunk.mSpatialBounds);
        chunk.mQuantBitOffset = conv.mBitOffset16;
        chunk.mInvBitScale = conv.mBitScaleInv;

        auto& leaf = chunk.mLocalBvh.emplace_back();
        leaf.mParent.SetEmpty();
        for (int i = 0; i < faceCount; ++i)
        {
            leaf.mParent.SetAabb(i, conv.convertAabb(faceBoxes[i]));
            leaf.mData[i].mParent = (uint8_t)i;
        }
        for (int i = faceCount; i < 4; ++i) leaf.mData[i].mParent = 0;
        leaf.ConfigureAsLeafOrInternal(true);
    }

    // =========================================================================
    // buildTopologyAndTree
    // =========================================================================
    void hknpMeshShapeBuilder::buildTopologyAndTree(
        const common::hkGeometry& geo,
        classes::HavokClasses::hkcdSimdTree& outTree,
        std::vector<MeshChunk>& outChunks,
        float maxExtent,
        int& /*outDegCount*/)
    {
        classes::HavokClasses::hkcdSimdTree tmpTree; tmpTree.Clear();
        buildTopLevelBvh(geo, tmpTree);
        const auto* nodes = tmpTree.mNodes.mElements.data();

        std::vector<int> primCounts;
        countPrimitivesPerNode(tmpTree, primCounts);

        common::hkAabb meshBox; tmpTree.GetDomain(&meshBox);

        // Fast path: entire mesh fits in a single section
        if (subtreeFitsInSection(1, meshBox, tmpTree, geo, primCounts, maxExtent))
        {
            if (!tmpTree.IsEmpty())
            {
                populateChunkFromSubtree(geo, 1, nodes, outChunks, kVertexError);
                classes::HavokClasses::hkcdSimdTree__Node leaf;
                leaf.ClearLeaf();
                leaf.mData[0].mParent = 0;
                leaf.mParent.SetAabb(0, meshBox);
                outTree.mNodes.mElements.emplace_back(leaf);
                outTree.mNodes.mElements[1].mData[0].mParent = 2;
                outTree.mNodes.mElements[1].mParent.SetAabb(0, meshBox);
            }
            return;
        }

        // Pending work: each item is a BVH node to partition into sections.
        // For each node we track: which output section-tree node is its parent, and in which slot.
        struct WorkEntry
        {
            uint32_t bvhNodeIdx;   // index into tmpTree
            int      secParentIdx; // output section-node parent (-1 = root)
            int      secParentSlot;
        };

        // Output section tree nodes (will be copied to outTree at end)
        std::vector<classes::HavokClasses::hkcdSimdTree__Node> secNodes;
        secNodes.emplace_back().ClearInternal(); // sentinel at 0
        secNodes.emplace_back().ClearInternal(); // root at 1

        std::vector<WorkEntry> workList;
        workList.push_back({ 1, 1, 0 }); // process tmpTree root, write to secNodes[1]

        while (!workList.empty())
        {
            WorkEntry we = workList.back();
            workList.pop_back();

            const auto& rootNode = nodes[we.bvhNodeIdx];

            // Determine which children can become sections immediately
            int validKids = 0;
            bool toSection[4] = {};
            for (int i = 0; i < 4; ++i)
            {
                if (!rootNode.IsDataValid(i)) continue;
                ++validKids;
                common::hkAabb aabb; rootNode.mParent.GetAabb(i, &aabb);
                toSection[i] = rootNode.mIsLeaf
                    || subtreeFitsInSection(rootNode.mData[i].mParent, aabb, tmpTree, geo, primCounts, maxExtent);
            }

            // Convert eligible subtrees/leaves to chunks
            std::vector<int> newChunkIds;
            if (!rootNode.mIsLeaf)
            {
                for (int i = 0; i < 4; ++i)
                {
                    if (!rootNode.IsDataValid(i) || !toSection[i]) continue;
                    populateChunkFromSubtree(geo, rootNode.mData[i].mParent, nodes, outChunks, kVertexError);
                    newChunkIds.push_back((int)outChunks.size() - 1);
                }
            }
            else
            {
                int pids[4]; int pidCount = 0;
                for (int i = 0; i < validKids; ++i) pids[pidCount++] = rootNode.mData[i].mParent;
                populateChunkFromFaceList(geo, pids, pidCount, outChunks, kVertexError, maxExtent);
                newChunkIds.push_back((int)outChunks.size() - 1);
            }

            const bool allConverted = (rootNode.mIsLeaf || (int)newChunkIds.size() == validKids);
            const int  newNi = we.secParentIdx; // reuse the pre-allocated node

            // If all children become sections, this secNode is a leaf
            if (allConverted)
            {
                // Copy AABBs from the original BVH node, but override data refs to chunk indices
                secNodes[newNi] = rootNode;
                secNodes[newNi].mIsLeaf = true;

                if (!rootNode.mIsLeaf)
                {
                    secNodes[newNi].ClearLeavesData();
                    int chunkIdxInList = 0;
                    for (int i = 0; i < 4; ++i)
                        if (rootNode.IsDataValid(i) && toSection[i])
                            secNodes[newNi].mData[i].mParent = newChunkIds[chunkIdxInList++];
                }
                else
                {
                    secNodes[newNi].ClearLeaf();
                    secNodes[newNi].SetChild(0, outChunks[newChunkIds[0]].mUnexpandedBounds, newChunkIds[0]);
                }
            }
            else
            {
                // Mixed: some children become sections, others need further subdivision.
                // Emit a leaf sub-node for the converted ones, and queue work for the rest.
                secNodes[newNi] = rootNode;
                secNodes[newNi].mIsLeaf = false;
                secNodes[newNi].ClearInternalsData();

                // Allocate a new leaf node to hold the converted sections
                const int leafNi = (int)secNodes.size();
                secNodes.emplace_back().ClearLeaf();
                {
                    int slot = 0;
                    for (int cid : newChunkIds)
                        secNodes[leafNi].SetChild(slot++, outChunks[cid].mUnexpandedBounds, cid);
                }

                // Find a free slot in newNi for the leaf node
                int leafNiSlot = -1;
                for (int i = 0; i < validKids; ++i)
                {
                    if (!rootNode.IsDataValid(i) || !toSection[i]) continue;
                    common::hkAabb leafBox; secNodes[leafNi].mParent.GetCompoundAabb(&leafBox);
                    secNodes[newNi].mData[i].mParent = leafNi;
                    secNodes[newNi].mParent.SetAabb(i, leafBox);
                    leafNiSlot = i; break;
                }

                // Queue unconverted children for further processing, allocating secNodes slots now
                for (int i = 0; i < 4; ++i)
                {
                    if (!rootNode.IsDataValid(i) || toSection[i] || i == leafNiSlot) continue;
                    const int childSecNi = (int)secNodes.size();
                    secNodes.emplace_back().ClearInternal();
                    common::hkAabb aabb; rootNode.mParent.GetAabb(i, &aabb);
                    secNodes[newNi].mData[i].mParent = childSecNi;
                    secNodes[newNi].mParent.SetAabb(i, aabb);
                    workList.push_back({ rootNode.mData[i].mParent, childSecNi, i });
                }
            }
        }

        outTree.mNodes.mElements.assign(secNodes.begin(), secNodes.end());
        outTree.SortByAabbsSize();
    }

    // =========================================================================
    // finalizeShape
    // =========================================================================
    void hknpMeshShapeBuilder::finalizeShape(
        const hknpMeshShapeConversionUtils::Vertex& packer,
        const classes::HavokClasses::hkcdSimdTree& bvh,
        std::vector<MeshChunk>& chunks,
        const std::vector<classes::HavokClasses::hknpMeshShape__ShapeTagTableEntry>& matTable,
        bool tagInterior,
        int, int, int,
        classes::HavokClasses::hknpMeshShape& dst)
    {
        dst.mVertexConversionUtil.mBitScale16.FromVec4(packer.mBitScale16);
        dst.mVertexConversionUtil.mBitScale16Inv.FromVec4(packer.mBitScale16Inv);

        // Copy material table
        auto& tagTbl = dst.mShapeTagTable.mElements;
        tagTbl.assign(matTable.begin(), matTable.end());

        // Copy top-level BVH
        dst.mTopLevelTree.mIsCompact = true;
        dst.mTopLevelTree.mNodes.mElements.assign(bvh.mNodes.mElements.begin(), bvh.mNodes.mElements.end());

        const int numChunks = (int)chunks.size();
        dst.mGeometrySections.mElements.resize(numChunks);

        // Helper: write quantized vertices as 3 × 2-byte little-endian components
        auto writeQuantizedVerts = [](
            classes::HavokClasses::hknpMeshShape__GeometrySection& sec,
            const MeshChunk& chunk,
            bool isLast)
            {
                const int nv = (int)chunk.mRawVerts.size();
                const int pad = isLast ? 2 : 0;
                sec.mVertexBuffer.mElements.resize(nv * 6 + pad);
                auto* buf = reinterpret_cast<classes::HavokClasses::hkUint8*>(sec.mVertexBuffer.mElements.data());
                for (int vi = 0; vi < nv; ++vi)
                {
                    const PackedVec3& q = chunk.mCompressedVerts[vi];
                    buf[vi * 6 + 0].mParent = (uint8_t)(q.x & 0xFF);
                    buf[vi * 6 + 1].mParent = (uint8_t)(q.x >> 8);
                    buf[vi * 6 + 2].mParent = (uint8_t)(q.y & 0xFF);
                    buf[vi * 6 + 3].mParent = (uint8_t)(q.y >> 8);
                    buf[vi * 6 + 4].mParent = (uint8_t)(q.z & 0xFF);
                    buf[vi * 6 + 5].mParent = (uint8_t)(q.z >> 8);
                }
            };

        // Helper: write raw float vertices and mark section as unquantized
        auto writeRawVerts = [](
            classes::HavokClasses::hknpMeshShape__GeometrySection& sec,
            const MeshChunk& chunk)
            {
                const int nv = (int)chunk.mRawVerts.size();
                sec.mVertexBuffer.mElements.resize(nv * sizeof(float) * 3);
                auto* buf = reinterpret_cast<glm::vec3*>(sec.mVertexBuffer.mElements.data());
                std::copy(chunk.mRawVerts.begin(), chunk.mRawVerts.end(), buf);
                for (int i = 0; i < 3; ++i)
                    sec.mSectionOffset[i].mParent = std::numeric_limits<uint32_t>::max();
            };

        for (int ci = 0; ci < numChunks; ++ci)
        {
            MeshChunk& chunk = chunks[ci];
            auto& sec = dst.mGeometrySections.mElements[ci];

            // Encoding metadata
            sec.mSectionOffset[0].mParent = chunk.mVertexOrigin.x;
            sec.mSectionOffset[1].mParent = chunk.mVertexOrigin.y;
            sec.mSectionOffset[2].mParent = chunk.mVertexOrigin.z;
            sec.mBitOffset[0].mParent = (short)chunk.mQuantBitOffset.x;
            sec.mBitOffset[1].mParent = (short)chunk.mQuantBitOffset.y;
            sec.mBitOffset[2].mParent = (short)chunk.mQuantBitOffset.z;
            sec.mBitScale8Inv.mX = chunk.mInvBitScale.x;
            sec.mBitScale8Inv.mY = chunk.mInvBitScale.y;
            sec.mBitScale8Inv.mZ = chunk.mInvBitScale.z;

            // BVH and face arrays
            sec.mSectionBvh.mElements.assign(chunk.mLocalBvh.begin(), chunk.mLocalBvh.end());
            sec.mPrimitives.mElements.assign(chunk.mFaces.begin(), chunk.mFaces.end());

            // Interior triangle bitfield
            const int nf = (int)chunk.mFaces.size();
            const int bitBytes = tagInterior ? (int)std::ceil(2.f * nf / 8.f) : 0;
            sec.mInteriorPrimitiveBitField.mElements.assign(bitBytes, {});

            // Mark interior triangles (two sub-triangles of a quad can be independently interior)
            if (tagInterior)
            {
                using Flags = classes::HavokClasses::hknpMeshShapeFlags;
                for (int fi = 0; fi < nf; ++fi)
                {
                    if (chunk.mFaceFlags[fi].IsFlagSet(Flags::DISABLE_ALL_EDGES_TRIANGLE_1)) sec.MarkTriangleAsInterior(fi, 0);
                    if (chunk.mFaceFlags[fi].IsFlagSet(Flags::DISABLE_ALL_EDGES_TRIANGLE_2)) sec.MarkTriangleAsInterior(fi, 1);
                }
            }

            // Vertex buffer
            chunk.mCanQuantize
                ? writeQuantizedVerts(sec, chunk, ci == numChunks - 1)
                : writeRawVerts(sec, chunk);
        }
    }

    void hknpMeshShapeBuilder::buildIntermediateData(
        const common::hkGeometry& geo,
        const hknpMeshShapeConversionUtils::Vertex& packer,
        classes::HavokClasses::hkcdSimdTree& outTree,
        std::vector<MeshChunk>& outChunks,
        std::vector<classes::HavokClasses::hknpMeshShape__ShapeTagTableEntry>& outMatTable,
        int& outDegCount)
    {
        // Validate that the requested precision is achievable for the mesh's coordinate range
        const float maxCoord = std::accumulate(geo.mVertices.begin(), geo.mVertices.end(), 0.f,
            [](float acc, const glm::vec3& v)
            {
                return std::max(acc, glm::compMax(glm::abs(v)));
            });

        if (!hknpMeshShapeConversionUtils::Vertex::isErrorAchievable(kVertexError, maxCoord))
            application::util::Logger::Warning("hknpMeshShapeBuilder::buildIntermediateData", "Target error not achievable");

        const float maxExtent = hknpMeshShapeConversionUtils::Vertex::calcMaxExtent(kVertexError);

        outTree.Clear();
        buildTopologyAndTree(geo, outTree, outChunks, maxExtent, outDegCount);

        // Compress all chunk vertex buffers before repairing
        for (auto& chunk : outChunks)
            compressChunkVerts(packer, chunk, kVertexError);

        // Repair, evict empty sections, and resync
        for (int ci = 0; ci < (int)outChunks.size(); ++ci)
        {
            auto& chunk = outChunks[ci];
            evaluateAllQuads(chunk);

            int degInSection = 0;
            if (repairChunkAfterQuantization(packer, chunk, kCoplanarTol, true, degInSection))
            {
                outDegCount += degInSection;
                evictSection(outChunks, ci--, outTree);
                continue;
            }
            outDegCount += degInSection;
            resyncSection(packer, chunk, outTree, ci);
        }

        for (auto& chunk : outChunks)
            sortFacesByMaterial(chunk);

        outMatTable = buildMaterialTable(outChunks);
    }

    bool hknpMeshShapeBuilder::compile(classes::HavokClasses::hknpMeshShape* dst, const common::hkGeometry& geo)
    {
        assert(dst != nullptr && "Destination mesh shape is null");

        classes::HavokClasses::hkcdSimdTree bvh;
        std::vector<MeshChunk> chunks;
        std::vector<classes::HavokClasses::hknpMeshShape__ShapeTagTableEntry> matTable;
        hknpMeshShapeConversionUtils::Vertex packer(kVertexError);
        int degCount = 0;

        buildIntermediateData(geo, packer, bvh, chunks, matTable, degCount);
        finalizeShape(packer, bvh, chunks, matTable, true, 0, 0, (int)geo.mTriangles.size(), *dst);

        const int numSections = (int)dst->mGeometrySections.mElements.size();
        if (numSections > 0)
        {
            uint8_t bits = 32 - static_cast<uint8_t>(
                application::util::Math::CountLeadingZeros(numSections - 1));
            dst->mParent.mParent.mNumShapeKeyBits.mParent = bits + 8 + 1;
        }
        else
        {
            application::util::Logger::Warning("hknpMeshShapeBuilder::compile",
                "Building hknpMeshShape with no geometry. Geometry may be empty or fully degenerate.");
            dst->mParent.mParent.mNumShapeKeyBits.mParent = 0;
        }
        return true;
    }
}