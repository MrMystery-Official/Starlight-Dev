#include <file/game/phive/starlight_physics/ai/hkaiNavMeshGeometryGenerator.h>

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace
{
    constexpr float DEG_TO_RAD = 0.01745329251994329576923690768489f;
    constexpr uint16_t INVALID_FACE_INDEX = 0xFFFF;

    struct VertexKey
    {
        int64_t mX = 0;
        int64_t mY = 0;
        int64_t mZ = 0;

        bool operator==(const VertexKey& other) const
        {
            return mX == other.mX && mY == other.mY && mZ == other.mZ;
        }
    };

    struct VertexKeyHash
    {
        size_t operator()(const VertexKey& key) const
        {
            const uint64_t h0 = static_cast<uint64_t>(key.mX) * 0x9E3779B185EBCA87ull;
            const uint64_t h1 = static_cast<uint64_t>(key.mY) * 0xC2B2AE3D27D4EB4Full;
            const uint64_t h2 = static_cast<uint64_t>(key.mZ) * 0x165667B19E3779F9ull;
            return static_cast<size_t>(h0 ^ h1 ^ h2);
        }
    };

    struct QuantizedVertexKey
    {
        uint16_t mX = 0;
        uint16_t mY = 0;
        uint16_t mZ = 0;

        bool operator==(const QuantizedVertexKey& other) const
        {
            return mX == other.mX && mY == other.mY && mZ == other.mZ;
        }
    };

    struct QuantizedVertexKeyHash
    {
        size_t operator()(const QuantizedVertexKey& key) const
        {
            const uint64_t h0 = static_cast<uint64_t>(key.mX) * 0x9E3779B185EBCA87ull;
            const uint64_t h1 = static_cast<uint64_t>(key.mY) * 0xC2B2AE3D27D4EB4Full;
            const uint64_t h2 = static_cast<uint64_t>(key.mZ) * 0x165667B19E3779F9ull;
            return static_cast<size_t>(h0 ^ h1 ^ h2);
        }
    };

    struct TriangleKey
    {
        uint32_t mA = 0;
        uint32_t mB = 0;
        uint32_t mC = 0;

        bool operator==(const TriangleKey& other) const
        {
            return mA == other.mA && mB == other.mB && mC == other.mC;
        }
    };

    struct TriangleKeyHash
    {
        size_t operator()(const TriangleKey& key) const
        {
            const uint64_t h0 = static_cast<uint64_t>(key.mA) * 0x9E3779B185EBCA87ull;
            const uint64_t h1 = static_cast<uint64_t>(key.mB) * 0xC2B2AE3D27D4EB4Full;
            const uint64_t h2 = static_cast<uint64_t>(key.mC) * 0x165667B19E3779F9ull;
            return static_cast<size_t>(h0 ^ h1 ^ h2);
        }
    };

    struct UndirectedEdgeKey
    {
        uint32_t mMin = 0;
        uint32_t mMax = 0;

        bool operator==(const UndirectedEdgeKey& other) const
        {
            return mMin == other.mMin && mMax == other.mMax;
        }
    };

    struct UndirectedEdgeKeyHash
    {
        size_t operator()(const UndirectedEdgeKey& key) const
        {
            const uint64_t h0 = static_cast<uint64_t>(key.mMin) * 0x9E3779B185EBCA87ull;
            const uint64_t h1 = static_cast<uint64_t>(key.mMax) * 0xC2B2AE3D27D4EB4Full;
            return static_cast<size_t>(h0 ^ h1);
        }
    };

    struct WorkingTriangle
    {
        uint32_t mVertices[3] = { 0, 0, 0 };
        glm::vec3 mNormal = glm::vec3(0.0f);
        float mArea = 0.0f;
    };

    struct EdgeRef
    {
        uint32_t mFace = 0;
        uint8_t mLocalEdge = 0;
        uint32_t mA = 0;
        uint32_t mB = 0;
    };

    struct SimplificationSettings
    {
        bool mEnabled = true;
        float mTargetRatio = 0.18f;
        float mMaxError = 0.20f;
    };

    inline bool IsFiniteVec3(const glm::vec3& value)
    {
        return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
    }

    inline float SnapToBoundary(float value, float minValue, float maxValue, float tolerance)
    {
        if (std::fabs(value - minValue) <= tolerance)
        {
            return minValue;
        }

        if (std::fabs(value - maxValue) <= tolerance)
        {
            return maxValue;
        }

        return value;
    }

    inline TriangleKey MakeTriangleKey(uint32_t a, uint32_t b, uint32_t c)
    {
        uint32_t sorted[3] = { a, b, c };
        std::sort(sorted, sorted + 3);
        return TriangleKey{ sorted[0], sorted[1], sorted[2] };
    }

    inline UndirectedEdgeKey MakeUndirectedEdgeKey(uint32_t a, uint32_t b)
    {
        return (a < b) ? UndirectedEdgeKey{ a, b } : UndirectedEdgeKey{ b, a };
    }

    inline int32_t QuantizeToInt(float value, float step)
    {
        return static_cast<int32_t>(std::llround(value / step));
    }

    inline float ClampToRange(float value, float minValue, float maxValue)
    {
        return std::max(minValue, std::min(value, maxValue));
    }

    inline uint16_t ClampToU16(int32_t value)
    {
        if (value < 0)
        {
            return 0;
        }

        if (value > 0xFFFF)
        {
            return 0xFFFF;
        }

        return static_cast<uint16_t>(value);
    }

    inline glm::vec3 DequantizeVertex(const QuantizedVertexKey& vertex, const float boundsMin[3], float cs, float ch)
    {
        return glm::vec3(
            boundsMin[0] + static_cast<float>(vertex.mX) * cs,
            boundsMin[1] + static_cast<float>(vertex.mY) * ch,
            boundsMin[2] + static_cast<float>(vertex.mZ) * cs);
    }

    struct Bucket2DKey
    {
        int32_t mX = 0;
        int32_t mZ = 0;

        bool operator==(const Bucket2DKey& other) const
        {
            return mX == other.mX && mZ == other.mZ;
        }
    };

    struct Bucket2DKeyHash
    {
        size_t operator()(const Bucket2DKey& key) const
        {
            const uint64_t x = static_cast<uint32_t>(key.mX);
            const uint64_t z = static_cast<uint32_t>(key.mZ);
            return static_cast<size_t>((x * 73856093ull) ^ (z * 19349663ull));
        }
    };

    bool SampleTriangleHeightAtXZ(
        const glm::vec3& a,
        const glm::vec3& b,
        const glm::vec3& c,
        float sampleX,
        float sampleZ,
        float& sampleYOut)
    {
        const float det = (b.z - c.z) * (a.x - c.x) + (c.x - b.x) * (a.z - c.z);
        if (std::fabs(det) <= 1e-8f)
        {
            return false;
        }

        const float u = ((b.z - c.z) * (sampleX - c.x) + (c.x - b.x) * (sampleZ - c.z)) / det;
        const float v = ((c.z - a.z) * (sampleX - c.x) + (a.x - c.x) * (sampleZ - c.z)) / det;
        const float w = 1.0f - u - v;
        constexpr float baryEps = 1e-4f;
        if (u < -baryEps || v < -baryEps || w < -baryEps)
        {
            return false;
        }

        sampleYOut = u * a.y + v * b.y + w * c.y;
        return std::isfinite(sampleYOut);
    }

    inline float Cross2D(const glm::vec2& a, const glm::vec2& b, const glm::vec2& c)
    {
        return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    }

    bool PointInsideTriangleStrictXZ(
        const glm::vec3& point,
        const glm::vec3& a,
        const glm::vec3& b,
        const glm::vec3& c,
        float eps)
    {
        const glm::vec2 p(point.x, point.z);
        const glm::vec2 v0(a.x, a.z);
        const glm::vec2 v1(b.x, b.z);
        const glm::vec2 v2(c.x, c.z);

        const float area = Cross2D(v0, v1, v2);
        if (std::fabs(area) <= eps)
        {
            return false;
        }

        const float invArea = 1.0f / area;
        const float w0 = Cross2D(v1, v2, p) * invArea;
        const float w1 = Cross2D(v2, v0, p) * invArea;
        const float w2 = 1.0f - w0 - w1;
        return w0 > eps && w1 > eps && w2 > eps;
    }

    bool SegmentsIntersectStrictXZ(
        const glm::vec3& a0,
        const glm::vec3& a1,
        const glm::vec3& b0,
        const glm::vec3& b1,
        float eps)
    {
        const glm::vec2 p0(a0.x, a0.z);
        const glm::vec2 p1(a1.x, a1.z);
        const glm::vec2 q0(b0.x, b0.z);
        const glm::vec2 q1(b1.x, b1.z);

        const float d0 = Cross2D(p0, p1, q0);
        const float d1 = Cross2D(p0, p1, q1);
        const float d2 = Cross2D(q0, q1, p0);
        const float d3 = Cross2D(q0, q1, p1);

        const bool straddleA = (d0 > eps && d1 < -eps) || (d0 < -eps && d1 > eps);
        const bool straddleB = (d2 > eps && d3 < -eps) || (d2 < -eps && d3 > eps);
        return straddleA && straddleB;
    }

    bool TrianglesOverlapInXZStrict(
        const glm::vec3& a0,
        const glm::vec3& a1,
        const glm::vec3& a2,
        const glm::vec3& b0,
        const glm::vec3& b1,
        const glm::vec3& b2,
        float eps)
    {
        if (PointInsideTriangleStrictXZ(a0, b0, b1, b2, eps) ||
            PointInsideTriangleStrictXZ(a1, b0, b1, b2, eps) ||
            PointInsideTriangleStrictXZ(a2, b0, b1, b2, eps) ||
            PointInsideTriangleStrictXZ(b0, a0, a1, a2, eps) ||
            PointInsideTriangleStrictXZ(b1, a0, a1, a2, eps) ||
            PointInsideTriangleStrictXZ(b2, a0, a1, a2, eps))
        {
            return true;
        }

        const glm::vec3 triA[3] = { a0, a1, a2 };
        const glm::vec3 triB[3] = { b0, b1, b2 };
        for (int ea = 0; ea < 3; ++ea)
        {
            const glm::vec3& aStart = triA[ea];
            const glm::vec3& aEnd = triA[(ea + 1) % 3];
            for (int eb = 0; eb < 3; ++eb)
            {
                const glm::vec3& bStart = triB[eb];
                const glm::vec3& bEnd = triB[(eb + 1) % 3];
                if (SegmentsIntersectStrictXZ(aStart, aEnd, bStart, bEnd, eps))
                {
                    return true;
                }
            }
        }

        return false;
    }

    uint32_t CountSharedVertices(const WorkingTriangle& a, const WorkingTriangle& b)
    {
        uint32_t shared = 0;
        for (int i = 0; i < 3; ++i)
        {
            for (int j = 0; j < 3; ++j)
            {
                if (a.mVertices[i] == b.mVertices[j])
                {
                    ++shared;
                }
            }
        }

        return shared;
    }

    uint32_t CountSharedVertices(
        uint32_t a0,
        uint32_t a1,
        uint32_t a2,
        uint32_t b0,
        uint32_t b1,
        uint32_t b2)
    {
        const uint32_t triA[3] = { a0, a1, a2 };
        const uint32_t triB[3] = { b0, b1, b2 };

        uint32_t shared = 0;
        for (uint32_t va : triA)
        {
            for (uint32_t vb : triB)
            {
                if (va == vb)
                {
                    ++shared;
                }
            }
        }

        return shared;
    }

    void RemoveCoveredLowerSurfaces(
        const std::vector<glm::vec3>& vertices,
        float cellSize,
        float cellHeight,
        float maxOverheadDistance,
        std::vector<WorkingTriangle>& trianglesInOut)
    {
        if (trianglesInOut.size() < 2 || vertices.empty())
        {
            return;
        }

        struct TriInfo
        {
            glm::vec3 mV0 = glm::vec3(0.0f);
            glm::vec3 mV1 = glm::vec3(0.0f);
            glm::vec3 mV2 = glm::vec3(0.0f);
            float mMinX = 0.0f;
            float mMaxX = 0.0f;
            float mMinZ = 0.0f;
            float mMaxZ = 0.0f;
            bool mValid = false;
        };

        const float bucketSize = std::max(cellSize * 3.0f, 0.5f);
        const float invBucket = 1.0f / bucketSize;
        const float heightEps = std::max(cellHeight * 0.5f, 0.02f);
        const float overheadLimit = std::max(maxOverheadDistance, heightEps * 2.0f);
        const float boundsEps = std::max(cellSize * 0.2f, 0.02f);

        std::vector<TriInfo> triInfos(trianglesInOut.size());
        std::unordered_map<Bucket2DKey, std::vector<uint32_t>, Bucket2DKeyHash> buckets;
        buckets.reserve(trianglesInOut.size() * 2);

        for (uint32_t triIndex = 0; triIndex < trianglesInOut.size(); ++triIndex)
        {
            const WorkingTriangle& tri = trianglesInOut[triIndex];
            if (tri.mVertices[0] >= vertices.size() || tri.mVertices[1] >= vertices.size() || tri.mVertices[2] >= vertices.size())
            {
                continue;
            }

            TriInfo& info = triInfos[triIndex];
            info.mV0 = vertices[tri.mVertices[0]];
            info.mV1 = vertices[tri.mVertices[1]];
            info.mV2 = vertices[tri.mVertices[2]];
            if (!IsFiniteVec3(info.mV0) || !IsFiniteVec3(info.mV1) || !IsFiniteVec3(info.mV2))
            {
                continue;
            }

            info.mMinX = std::min({ info.mV0.x, info.mV1.x, info.mV2.x });
            info.mMaxX = std::max({ info.mV0.x, info.mV1.x, info.mV2.x });
            info.mMinZ = std::min({ info.mV0.z, info.mV1.z, info.mV2.z });
            info.mMaxZ = std::max({ info.mV0.z, info.mV1.z, info.mV2.z });

            const int32_t minBX = static_cast<int32_t>(std::floor(info.mMinX * invBucket));
            const int32_t maxBX = static_cast<int32_t>(std::floor(info.mMaxX * invBucket));
            const int32_t minBZ = static_cast<int32_t>(std::floor(info.mMinZ * invBucket));
            const int32_t maxBZ = static_cast<int32_t>(std::floor(info.mMaxZ * invBucket));

            for (int32_t bz = minBZ; bz <= maxBZ; ++bz)
            {
                for (int32_t bx = minBX; bx <= maxBX; ++bx)
                {
                    buckets[Bucket2DKey{ bx, bz }].push_back(triIndex);
                }
            }

            info.mValid = true;
        }

        std::vector<uint8_t> removeTriangle(trianglesInOut.size(), 0);
        for (uint32_t triIndex = 0; triIndex < trianglesInOut.size(); ++triIndex)
        {
            if (!triInfos[triIndex].mValid)
            {
                continue;
            }

            const TriInfo& info = triInfos[triIndex];
            const glm::vec3 samplePoints[7] = {
                (info.mV0 + info.mV1 + info.mV2) / 3.0f,
                (info.mV0 + info.mV1) * 0.5f,
                (info.mV1 + info.mV2) * 0.5f,
                (info.mV2 + info.mV0) * 0.5f,
                info.mV0 * 0.80f + info.mV1 * 0.10f + info.mV2 * 0.10f,
                info.mV1 * 0.80f + info.mV2 * 0.10f + info.mV0 * 0.10f,
                info.mV2 * 0.80f + info.mV0 * 0.10f + info.mV1 * 0.10f
            };

            int coveredSamples = 0;
            for (const glm::vec3& sample : samplePoints)
            {
                const int32_t sampleBX = static_cast<int32_t>(std::floor(sample.x * invBucket));
                const int32_t sampleBZ = static_cast<int32_t>(std::floor(sample.z * invBucket));

                bool sampleCovered = false;
                for (int32_t dz = -1; dz <= 1 && !sampleCovered; ++dz)
                {
                    for (int32_t dx = -1; dx <= 1 && !sampleCovered; ++dx)
                    {
                        const Bucket2DKey key{ sampleBX + dx, sampleBZ + dz };
                        auto bucketIter = buckets.find(key);
                        if (bucketIter == buckets.end())
                        {
                            continue;
                        }

                        for (uint32_t candidateTri : bucketIter->second)
                        {
                            if (candidateTri == triIndex || !triInfos[candidateTri].mValid)
                            {
                                continue;
                            }

                            const TriInfo& candidate = triInfos[candidateTri];
                            if (sample.x < candidate.mMinX - boundsEps || sample.x > candidate.mMaxX + boundsEps ||
                                sample.z < candidate.mMinZ - boundsEps || sample.z > candidate.mMaxZ + boundsEps)
                            {
                                continue;
                            }

                            float candidateY = 0.0f;
                            if (!SampleTriangleHeightAtXZ(candidate.mV0, candidate.mV1, candidate.mV2, sample.x, sample.z, candidateY))
                            {
                                continue;
                            }

                            const float deltaY = candidateY - sample.y;
                            if (deltaY > heightEps && deltaY < overheadLimit + heightEps)
                            {
                                sampleCovered = true;
                                break;
                            }
                        }
                    }
                }

                if (sampleCovered)
                {
                    coveredSamples++;
                }
            }

            if (coveredSamples == static_cast<int>(std::size(samplePoints)))
            {
                removeTriangle[triIndex] = 1;
            }
        }

        bool anyRemoved = false;
        for (uint8_t removed : removeTriangle)
        {
            anyRemoved |= (removed != 0);
        }

        if (!anyRemoved)
        {
            return;
        }

        std::vector<WorkingTriangle> filtered;
        filtered.reserve(trianglesInOut.size());
        for (uint32_t triIndex = 0; triIndex < trianglesInOut.size(); ++triIndex)
        {
            if (!removeTriangle[triIndex])
            {
                filtered.push_back(trianglesInOut[triIndex]);
            }
        }

        trianglesInOut.swap(filtered);
    }

    void RemoveOverlappingWalkableSurfaces(
        const std::vector<glm::vec3>& vertices,
        float cellSize,
        float cellHeight,
        std::vector<WorkingTriangle>& trianglesInOut)
    {
        if (trianglesInOut.size() < 2 || vertices.empty())
        {
            return;
        }

        struct TriInfo
        {
            glm::vec3 mV0 = glm::vec3(0.0f);
            glm::vec3 mV1 = glm::vec3(0.0f);
            glm::vec3 mV2 = glm::vec3(0.0f);
            float mMinX = 0.0f;
            float mMaxX = 0.0f;
            float mMinZ = 0.0f;
            float mMaxZ = 0.0f;
            float mCentroidY = 0.0f;
            float mArea = 0.0f;
            bool mValid = false;
        };

        const float bucketSize = std::max(cellSize * 3.0f, 0.5f);
        const float invBucket = 1.0f / bucketSize;
        const float overlapEps = std::max(cellSize * 0.04f, 1e-4f);
        const float heightEps = std::max(cellHeight * 0.8f, 0.04f);

        std::vector<TriInfo> infos(trianglesInOut.size());
        std::unordered_map<Bucket2DKey, std::vector<uint32_t>, Bucket2DKeyHash> buckets;
        buckets.reserve(trianglesInOut.size() * 2);

        for (uint32_t triIndex = 0; triIndex < trianglesInOut.size(); ++triIndex)
        {
            const WorkingTriangle& tri = trianglesInOut[triIndex];
            if (tri.mVertices[0] >= vertices.size() || tri.mVertices[1] >= vertices.size() || tri.mVertices[2] >= vertices.size())
            {
                continue;
            }

            TriInfo& info = infos[triIndex];
            info.mV0 = vertices[tri.mVertices[0]];
            info.mV1 = vertices[tri.mVertices[1]];
            info.mV2 = vertices[tri.mVertices[2]];
            if (!IsFiniteVec3(info.mV0) || !IsFiniteVec3(info.mV1) || !IsFiniteVec3(info.mV2))
            {
                continue;
            }

            const glm::vec3 cross = glm::cross(info.mV1 - info.mV0, info.mV2 - info.mV0);
            const float crossLen = glm::length(cross);
            if (crossLen <= 1e-8f)
            {
                continue;
            }

            info.mArea = crossLen * 0.5f;
            info.mCentroidY = (info.mV0.y + info.mV1.y + info.mV2.y) / 3.0f;
            info.mMinX = std::min({ info.mV0.x, info.mV1.x, info.mV2.x });
            info.mMaxX = std::max({ info.mV0.x, info.mV1.x, info.mV2.x });
            info.mMinZ = std::min({ info.mV0.z, info.mV1.z, info.mV2.z });
            info.mMaxZ = std::max({ info.mV0.z, info.mV1.z, info.mV2.z });

            const int32_t minBX = static_cast<int32_t>(std::floor(info.mMinX * invBucket));
            const int32_t maxBX = static_cast<int32_t>(std::floor(info.mMaxX * invBucket));
            const int32_t minBZ = static_cast<int32_t>(std::floor(info.mMinZ * invBucket));
            const int32_t maxBZ = static_cast<int32_t>(std::floor(info.mMaxZ * invBucket));

            for (int32_t bz = minBZ; bz <= maxBZ; ++bz)
            {
                for (int32_t bx = minBX; bx <= maxBX; ++bx)
                {
                    buckets[Bucket2DKey{ bx, bz }].push_back(triIndex);
                }
            }

            info.mValid = true;
        }

        std::vector<uint8_t> removeTriangle(trianglesInOut.size(), 0);
        std::vector<uint32_t> seenStamp(trianglesInOut.size(), 0);
        uint32_t stamp = 1;
        std::vector<uint32_t> candidates;
        candidates.reserve(256);

        for (uint32_t triIndex = 0; triIndex < trianglesInOut.size(); ++triIndex)
        {
            if (!infos[triIndex].mValid || removeTriangle[triIndex])
            {
                continue;
            }

            if (++stamp == 0)
            {
                stamp = 1;
                std::fill(seenStamp.begin(), seenStamp.end(), 0);
            }

            const TriInfo& triInfo = infos[triIndex];
            const int32_t minBX = static_cast<int32_t>(std::floor(triInfo.mMinX * invBucket));
            const int32_t maxBX = static_cast<int32_t>(std::floor(triInfo.mMaxX * invBucket));
            const int32_t minBZ = static_cast<int32_t>(std::floor(triInfo.mMinZ * invBucket));
            const int32_t maxBZ = static_cast<int32_t>(std::floor(triInfo.mMaxZ * invBucket));

            candidates.clear();
            for (int32_t bz = minBZ; bz <= maxBZ; ++bz)
            {
                for (int32_t bx = minBX; bx <= maxBX; ++bx)
                {
                    auto bucketIter = buckets.find(Bucket2DKey{ bx, bz });
                    if (bucketIter == buckets.end())
                    {
                        continue;
                    }

                    for (uint32_t candidateIndex : bucketIter->second)
                    {
                        if (candidateIndex <= triIndex || !infos[candidateIndex].mValid || removeTriangle[candidateIndex])
                        {
                            continue;
                        }

                        if (seenStamp[candidateIndex] == stamp)
                        {
                            continue;
                        }

                        seenStamp[candidateIndex] = stamp;
                        candidates.push_back(candidateIndex);
                    }
                }
            }

            for (uint32_t candidateIndex : candidates)
            {
                if (removeTriangle[triIndex] || removeTriangle[candidateIndex])
                {
                    continue;
                }

                if (CountSharedVertices(trianglesInOut[triIndex], trianglesInOut[candidateIndex]) >= 2)
                {
                    continue;
                }

                const TriInfo& candidateInfo = infos[candidateIndex];
                if (triInfo.mMaxX < candidateInfo.mMinX - overlapEps || triInfo.mMinX > candidateInfo.mMaxX + overlapEps ||
                    triInfo.mMaxZ < candidateInfo.mMinZ - overlapEps || triInfo.mMinZ > candidateInfo.mMaxZ + overlapEps)
                {
                    continue;
                }

                if (!TrianglesOverlapInXZStrict(
                        triInfo.mV0,
                        triInfo.mV1,
                        triInfo.mV2,
                        candidateInfo.mV0,
                        candidateInfo.mV1,
                        candidateInfo.mV2,
                        overlapEps))
                {
                    continue;
                }

                if (std::fabs(triInfo.mCentroidY - candidateInfo.mCentroidY) > heightEps)
                {
                    continue;
                }

                const bool firstIsSmaller = triInfo.mArea < candidateInfo.mArea;
                const TriInfo& smaller = firstIsSmaller ? triInfo : candidateInfo;
                const TriInfo& larger = firstIsSmaller ? candidateInfo : triInfo;

                const glm::vec3 samplePoints[7] = {
                    (smaller.mV0 + smaller.mV1 + smaller.mV2) / 3.0f,
                    (smaller.mV0 + smaller.mV1) * 0.5f,
                    (smaller.mV1 + smaller.mV2) * 0.5f,
                    (smaller.mV2 + smaller.mV0) * 0.5f,
                    smaller.mV0 * 0.80f + smaller.mV1 * 0.10f + smaller.mV2 * 0.10f,
                    smaller.mV1 * 0.80f + smaller.mV2 * 0.10f + smaller.mV0 * 0.10f,
                    smaller.mV2 * 0.80f + smaller.mV0 * 0.10f + smaller.mV1 * 0.10f
                };

                int coveredSamples = 0;
                for (const glm::vec3& sample : samplePoints)
                {
                    float largerY = 0.0f;
                    if (!SampleTriangleHeightAtXZ(larger.mV0, larger.mV1, larger.mV2, sample.x, sample.z, largerY))
                    {
                        continue;
                    }

                    if (std::fabs(largerY - sample.y) <= heightEps)
                    {
                        coveredSamples++;
                    }
                }

                if (coveredSamples == static_cast<int>(std::size(samplePoints)))
                {
                    if (firstIsSmaller)
                    {
                        removeTriangle[triIndex] = 1;
                    }
                    else
                    {
                        removeTriangle[candidateIndex] = 1;
                    }
                }
            }
        }

        bool anyRemoved = false;
        for (uint8_t removed : removeTriangle)
        {
            anyRemoved |= (removed != 0);
        }

        if (!anyRemoved)
        {
            return;
        }

        std::vector<WorkingTriangle> filtered;
        filtered.reserve(trianglesInOut.size());
        for (uint32_t triIndex = 0; triIndex < trianglesInOut.size(); ++triIndex)
        {
            if (!removeTriangle[triIndex])
            {
                filtered.push_back(trianglesInOut[triIndex]);
            }
        }

        trianglesInOut.swap(filtered);
    }

    void RebuildWorkingTrianglesFromIndexBuffer(
        const std::vector<uint32_t>& indices,
        const std::vector<glm::vec3>& vertices,
        float walkableCos,
        float minTriangleArea,
        std::vector<WorkingTriangle>& trianglesOut)
    {
        trianglesOut.clear();
        if (indices.size() < 3 || vertices.empty())
        {
            return;
        }

        trianglesOut.reserve(indices.size() / 3);
        std::unordered_set<TriangleKey, TriangleKeyHash> uniqueTriangles;
        uniqueTriangles.reserve(indices.size() / 3);

        for (size_t i = 0; i + 2 < indices.size(); i += 3)
        {
            uint32_t a = indices[i + 0];
            uint32_t b = indices[i + 1];
            uint32_t c = indices[i + 2];

            if (a >= vertices.size() || b >= vertices.size() || c >= vertices.size())
            {
                continue;
            }

            if (a == b || b == c || c == a)
            {
                continue;
            }

            const glm::vec3& va = vertices[a];
            const glm::vec3& vb = vertices[b];
            const glm::vec3& vc = vertices[c];

            glm::vec3 cross = glm::cross(vb - va, vc - va);
            const float crossLength = glm::length(cross);
            if (crossLength <= 1e-8f)
            {
                continue;
            }

            const float area = crossLength * 0.5f;
            if (area <= minTriangleArea)
            {
                continue;
            }

            glm::vec3 normal = cross / crossLength;
            if (std::fabs(normal.y) < walkableCos)
            {
                continue;
            }

            if (cross.y < 0.0f)
            {
                std::swap(b, c);
                normal = -normal;
            }
            if (normal.y < 0.0f)
            {
                normal = -normal;
            }

            const TriangleKey key = MakeTriangleKey(a, b, c);
            if (!uniqueTriangles.insert(key).second)
            {
                continue;
            }

            WorkingTriangle triangle;
            triangle.mVertices[0] = a;
            triangle.mVertices[1] = b;
            triangle.mVertices[2] = c;
            triangle.mNormal = normal;
            triangle.mArea = area;
            trianglesOut.push_back(triangle);
        }
    }

    void NormalizeInputTriangleWinding(
        const float* inputVertices,
        int inputVertexCount,
        const int* inputIndices,
        int inputIndexCount,
        std::vector<int32_t>& woundIndicesOut)
    {
        woundIndicesOut.clear();
        if (inputVertices == nullptr || inputIndices == nullptr || inputVertexCount <= 0 || inputIndexCount < 3)
        {
            return;
        }

        woundIndicesOut.reserve(static_cast<size_t>(inputIndexCount - (inputIndexCount % 3)));
        for (int32_t tri = 0; tri + 2 < inputIndexCount; tri += 3)
        {
            int32_t ia = inputIndices[tri + 0];
            int32_t ib = inputIndices[tri + 1];
            int32_t ic = inputIndices[tri + 2];

            if (ia >= 0 && ib >= 0 && ic >= 0 &&
                ia < inputVertexCount && ib < inputVertexCount && ic < inputVertexCount)
            {
                const glm::vec3 va(
                    inputVertices[ia * 3 + 0],
                    inputVertices[ia * 3 + 1],
                    inputVertices[ia * 3 + 2]);
                const glm::vec3 vb(
                    inputVertices[ib * 3 + 0],
                    inputVertices[ib * 3 + 1],
                    inputVertices[ib * 3 + 2]);
                const glm::vec3 vc(
                    inputVertices[ic * 3 + 0],
                    inputVertices[ic * 3 + 1],
                    inputVertices[ic * 3 + 2]);

                if (IsFiniteVec3(va) && IsFiniteVec3(vb) && IsFiniteVec3(vc))
                {
                    const glm::vec3 cross = glm::cross(vb - va, vc - va);
                    if (cross.y < 0.0f)
                    {
                        std::swap(ib, ic);
                    }
                }
            }

            woundIndicesOut.push_back(ia);
            woundIndicesOut.push_back(ib);
            woundIndicesOut.push_back(ic);
        }
    }

    float TriangleMinAngleRadians(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c)
    {
        auto AngleAt = [](const glm::vec3& p, const glm::vec3& p0, const glm::vec3& p1)
            {
                const glm::vec3 v0 = p0 - p;
                const glm::vec3 v1 = p1 - p;
                const float l0 = glm::length(v0);
                const float l1 = glm::length(v1);
                if (l0 <= 1e-8f || l1 <= 1e-8f)
                {
                    return 0.0f;
                }

                const float cosine = std::clamp(glm::dot(v0 / l0, v1 / l1), -1.0f, 1.0f);
                return std::acos(cosine);
            };

        const float a0 = AngleAt(a, b, c);
        const float a1 = AngleAt(b, c, a);
        const float a2 = AngleAt(c, a, b);
        return std::min(a0, std::min(a1, a2));
    }

    void RemoveDegenerateAndDuplicateTriangleIndices(
        std::vector<uint32_t>& indicesInOut,
        const std::vector<glm::vec3>& vertices,
        float minTriangleArea)
    {
        if (indicesInOut.size() < 3)
        {
            indicesInOut.clear();
            return;
        }

        std::vector<uint32_t> filtered;
        filtered.reserve(indicesInOut.size());

        std::unordered_set<TriangleKey, TriangleKeyHash> seen;
        seen.reserve(indicesInOut.size() / 3);

        for (size_t i = 0; i + 2 < indicesInOut.size(); i += 3)
        {
            uint32_t a = indicesInOut[i + 0];
            uint32_t b = indicesInOut[i + 1];
            uint32_t c = indicesInOut[i + 2];

            if (a >= vertices.size() || b >= vertices.size() || c >= vertices.size())
            {
                continue;
            }

            if (a == b || b == c || c == a)
            {
                continue;
            }

            const glm::vec3& va = vertices[a];
            const glm::vec3& vb = vertices[b];
            const glm::vec3& vc = vertices[c];
            const glm::vec3 cross = glm::cross(vb - va, vc - va);
            const float area = glm::length(cross) * 0.5f;
            if (area <= minTriangleArea)
            {
                continue;
            }

            if (cross.y < 0.0f)
            {
                std::swap(b, c);
            }

            const TriangleKey key = MakeTriangleKey(a, b, c);
            if (!seen.insert(key).second)
            {
                continue;
            }

            filtered.push_back(a);
            filtered.push_back(b);
            filtered.push_back(c);
        }

        indicesInOut.swap(filtered);
    }

    void TrySimplifyWorkingTriangles(
        std::vector<glm::vec3>& vertices,
        const SimplificationSettings& settings,
        float walkableCos,
        float minTriangleArea,
        bool clampToForcedBounds,
        float forcedMinX,
        float forcedMaxX,
        float forcedMinZ,
        float forcedMaxZ,
        std::vector<WorkingTriangle>& trianglesInOut)
    {
        if (!settings.mEnabled)
        {
            return;
        }

        if (trianglesInOut.size() < 16 || vertices.size() < 8)
        {
            return;
        }

        const float ratio = std::clamp(settings.mTargetRatio, 0.003f, 1.0f);
        if (ratio >= 0.999f)
        {
            return;
        }

        const float aggressiveness = std::clamp(settings.mMaxError, 0.02f, 1.0f);

        auto ClampVertexToBounds = [&](glm::vec3& vertex)
            {
                if (!clampToForcedBounds)
                {
                    return;
                }

                vertex.x = ClampToRange(vertex.x, forcedMinX, forcedMaxX);
                vertex.z = ClampToRange(vertex.z, forcedMinZ, forcedMaxZ);
            };

        std::vector<uint32_t> indices;
        indices.reserve(trianglesInOut.size() * 3);
        for (const WorkingTriangle& tri : trianglesInOut)
        {
            indices.push_back(tri.mVertices[0]);
            indices.push_back(tri.mVertices[1]);
            indices.push_back(tri.mVertices[2]);
        }

        for (glm::vec3& vertex : vertices)
        {
            ClampVertexToBounds(vertex);
        }

        RemoveDegenerateAndDuplicateTriangleIndices(indices, vertices, minTriangleArea);
        if (indices.size() < 9)
        {
            return;
        }

        const size_t initialTriangleCount = indices.size() / 3;
        const size_t targetTriangleCount = std::max<size_t>(
            1,
            static_cast<size_t>(std::floor(static_cast<float>(initialTriangleCount) * ratio)));

        if (targetTriangleCount >= initialTriangleCount)
        {
            return;
        }

        struct EdgeCandidate
        {
            uint32_t a = 0;
            uint32_t b = 0;
            uint32_t usage = 0;
            float lenSq = 0.0f;
        };

        struct CollapseCandidate
        {
            uint32_t a = 0;
            uint32_t b = 0;
            float cost = 0.0f;
        };

        auto BuildEdgeData = [&](const std::vector<uint32_t>& triIndices,
                                 std::vector<int32_t>& valenceOut,
                                 std::vector<uint8_t>& boundaryOut,
                                 std::vector<EdgeCandidate>& edgesOut,
                                 float& avgLenSqOut)
            {
                valenceOut.assign(vertices.size(), 0);
                boundaryOut.assign(vertices.size(), 0);
                edgesOut.clear();

                std::unordered_map<UndirectedEdgeKey, uint32_t, UndirectedEdgeKeyHash> edgeUse;
                edgeUse.reserve(triIndices.size());

                for (size_t i = 0; i + 2 < triIndices.size(); i += 3)
                {
                    const uint32_t tri[3] = { triIndices[i + 0], triIndices[i + 1], triIndices[i + 2] };
                    for (int e = 0; e < 3; ++e)
                    {
                        const uint32_t v0 = tri[e];
                        const uint32_t v1 = tri[(e + 1) % 3];
                        if (v0 == v1 || v0 >= vertices.size() || v1 >= vertices.size())
                        {
                            continue;
                        }

                        edgeUse[MakeUndirectedEdgeKey(v0, v1)]++;
                    }
                }

                edgesOut.reserve(edgeUse.size());
                float lenSqSum = 0.0f;
                for (const auto& [key, usage] : edgeUse)
                {
                    EdgeCandidate edge;
                    edge.a = key.mMin;
                    edge.b = key.mMax;
                    edge.usage = usage;
                    const glm::vec3 edgeDelta = vertices[edge.a] - vertices[edge.b];
                    edge.lenSq = glm::dot(edgeDelta, edgeDelta);
                    if (edge.lenSq <= 1e-12f)
                    {
                        continue;
                    }

                    lenSqSum += edge.lenSq;
                    valenceOut[edge.a]++;
                    valenceOut[edge.b]++;
                    if (usage == 1)
                    {
                        boundaryOut[edge.a] = 1;
                        boundaryOut[edge.b] = 1;
                    }

                    edgesOut.push_back(edge);
                }

                avgLenSqOut = edgesOut.empty() ? 0.0f : lenSqSum / static_cast<float>(edgesOut.size());
            };

        std::vector<int32_t> valence;
        std::vector<uint8_t> boundaryVertex;
        std::vector<EdgeCandidate> edges;
        std::vector<CollapseCandidate> candidates;

        const bool veryLargeMesh = initialTriangleCount > 120000;
        const int maxCollapsePasses = veryLargeMesh ? 10 : 16;
        const size_t maxCandidateCount = veryLargeMesh ? 220000 : 500000;

        for (int pass = 0; pass < maxCollapsePasses && indices.size() / 3 > targetTriangleCount; ++pass)
        {
            float avgLenSq = 0.0f;
            BuildEdgeData(indices, valence, boundaryVertex, edges, avgLenSq);
            if (edges.empty() || avgLenSq <= 1e-12f)
            {
                break;
            }

            const float maxLenSq = avgLenSq * (12.0f + aggressiveness * 72.0f + static_cast<float>(pass) * 4.0f);
            candidates.clear();
            candidates.reserve(edges.size());

            for (const EdgeCandidate& edge : edges)
            {
                if (edge.usage != 2 || edge.lenSq > maxLenSq)
                {
                    continue;
                }

                const uint32_t a = edge.a;
                const uint32_t b = edge.b;
                if (a >= boundaryVertex.size() || b >= boundaryVertex.size())
                {
                    continue;
                }

                if (boundaryVertex[a] || boundaryVertex[b])
                {
                    continue;
                }

                if (valence[a] <= 2 || valence[b] <= 2)
                {
                    continue;
                }

                const int32_t newValence = std::max<int32_t>(3, valence[a] + valence[b] - 4);
                const float valencePenalty =
                    0.30f * static_cast<float>(std::abs(newValence - 6)) +
                    0.95f * static_cast<float>(std::max(0, newValence - 7) * std::max(0, newValence - 7));

                // Penalize collapsing edges that are too far from local average length
                // so the final triangle sizes stay more evenly distributed.
                const float normalizedLen = edge.lenSq / std::max(avgLenSq, 1e-8f);
                const float distributionPenalty = std::fabs(normalizedLen - 1.0f);

                candidates.push_back(CollapseCandidate{
                    a,
                    b,
                    edge.lenSq * (1.0f + 0.28f * distributionPenalty) +
                        valencePenalty * avgLenSq
                });
            }

            if (candidates.empty())
            {
                break;
            }

            if (candidates.size() > maxCandidateCount)
            {
                std::nth_element(
                    candidates.begin(),
                    candidates.begin() + static_cast<std::ptrdiff_t>(maxCandidateCount),
                    candidates.end(),
                    [](const CollapseCandidate& lhs, const CollapseCandidate& rhs) { return lhs.cost < rhs.cost; });
                candidates.resize(maxCandidateCount);
            }

            std::sort(
                candidates.begin(),
                candidates.end(),
                [](const CollapseCandidate& lhs, const CollapseCandidate& rhs) { return lhs.cost < rhs.cost; });

            const size_t currentTriCount = indices.size() / 3;
            const size_t targetReduction = (currentTriCount > targetTriangleCount) ? (currentTriCount - targetTriangleCount) : 0;
            const size_t desiredCollapses = std::max<size_t>(
                128,
                std::min<size_t>(targetReduction * 20, currentTriCount));

            std::vector<std::vector<uint32_t>> incidentTriangles(vertices.size());
            for (uint32_t triIndex = 0; triIndex < static_cast<uint32_t>(currentTriCount); ++triIndex)
            {
                const uint32_t ia = indices[triIndex * 3 + 0];
                const uint32_t ib = indices[triIndex * 3 + 1];
                const uint32_t ic = indices[triIndex * 3 + 2];
                if (ia < incidentTriangles.size()) incidentTriangles[ia].push_back(triIndex);
                if (ib < incidentTriangles.size()) incidentTriangles[ib].push_back(triIndex);
                if (ic < incidentTriangles.size()) incidentTriangles[ic].push_back(triIndex);
            }

            struct SimplificationTriInfo
            {
                uint32_t mA = 0;
                uint32_t mB = 0;
                uint32_t mC = 0;
                glm::vec3 mV0 = glm::vec3(0.0f);
                glm::vec3 mV1 = glm::vec3(0.0f);
                glm::vec3 mV2 = glm::vec3(0.0f);
                float mMinX = 0.0f;
                float mMaxX = 0.0f;
                float mMinZ = 0.0f;
                float mMaxZ = 0.0f;
                float mCentroidY = 0.0f;
                bool mValid = false;
            };

            const float simplifyBucketSize = std::max(std::sqrt(avgLenSq) * 3.0f, 0.5f);
            const float simplifyInvBucket = 1.0f / simplifyBucketSize;
            const float simplifyOverlapEps = std::max(simplifyBucketSize * 0.015f, 1e-4f);
            const float simplifyOverlapHeightEps = std::max(0.05f, std::sqrt(std::max(minTriangleArea, 1e-8f)) * 6.0f);

            std::vector<SimplificationTriInfo> simplificationTriInfos(currentTriCount);
            std::unordered_map<Bucket2DKey, std::vector<uint32_t>, Bucket2DKeyHash> simplificationBuckets;
            simplificationBuckets.reserve(currentTriCount * 2);

            for (uint32_t triIndex = 0; triIndex < static_cast<uint32_t>(currentTriCount); ++triIndex)
            {
                const uint32_t ia = indices[triIndex * 3 + 0];
                const uint32_t ib = indices[triIndex * 3 + 1];
                const uint32_t ic = indices[triIndex * 3 + 2];
                if (ia >= vertices.size() || ib >= vertices.size() || ic >= vertices.size())
                {
                    continue;
                }

                if (ia == ib || ib == ic || ic == ia)
                {
                    continue;
                }

                const glm::vec3& v0 = vertices[ia];
                const glm::vec3& v1 = vertices[ib];
                const glm::vec3& v2 = vertices[ic];
                if (!IsFiniteVec3(v0) || !IsFiniteVec3(v1) || !IsFiniteVec3(v2))
                {
                    continue;
                }

                SimplificationTriInfo& info = simplificationTriInfos[triIndex];
                info.mA = ia;
                info.mB = ib;
                info.mC = ic;
                info.mV0 = v0;
                info.mV1 = v1;
                info.mV2 = v2;
                info.mMinX = std::min({ v0.x, v1.x, v2.x });
                info.mMaxX = std::max({ v0.x, v1.x, v2.x });
                info.mMinZ = std::min({ v0.z, v1.z, v2.z });
                info.mMaxZ = std::max({ v0.z, v1.z, v2.z });
                info.mCentroidY = (v0.y + v1.y + v2.y) / 3.0f;
                info.mValid = true;

                const int32_t minBX = static_cast<int32_t>(std::floor(info.mMinX * simplifyInvBucket));
                const int32_t maxBX = static_cast<int32_t>(std::floor(info.mMaxX * simplifyInvBucket));
                const int32_t minBZ = static_cast<int32_t>(std::floor(info.mMinZ * simplifyInvBucket));
                const int32_t maxBZ = static_cast<int32_t>(std::floor(info.mMaxZ * simplifyInvBucket));

                for (int32_t bz = minBZ; bz <= maxBZ; ++bz)
                {
                    for (int32_t bx = minBX; bx <= maxBX; ++bx)
                    {
                        simplificationBuckets[Bucket2DKey{ bx, bz }].push_back(triIndex);
                    }
                }
            }

            std::vector<uint32_t> affectedStamp(currentTriCount, 0);
            std::vector<uint32_t> candidateStamp(currentTriCount, 0);
            uint32_t affectedStampValue = 1;
            uint32_t candidateStampValue = 1;

            const auto IsCollapseLocallyValid = [&](uint32_t keep, uint32_t remove, const glm::vec3& mergedPos)
                {
                    if (keep >= incidentTriangles.size() || remove >= incidentTriangles.size())
                    {
                        return false;
                    }

                    std::vector<uint32_t> affectedTriangles;
                    affectedTriangles.reserve(incidentTriangles[keep].size() + incidentTriangles[remove].size());
                    affectedTriangles.insert(
                        affectedTriangles.end(),
                        incidentTriangles[keep].begin(),
                        incidentTriangles[keep].end());
                    affectedTriangles.insert(
                        affectedTriangles.end(),
                        incidentTriangles[remove].begin(),
                        incidentTriangles[remove].end());
                    std::sort(affectedTriangles.begin(), affectedTriangles.end());
                    affectedTriangles.erase(std::unique(affectedTriangles.begin(), affectedTriangles.end()), affectedTriangles.end());

                    if (++affectedStampValue == 0)
                    {
                        std::fill(affectedStamp.begin(), affectedStamp.end(), 0);
                        affectedStampValue = 1;
                    }
                    for (uint32_t triIndex : affectedTriangles)
                    {
                        if (triIndex < currentTriCount)
                        {
                            affectedStamp[triIndex] = affectedStampValue;
                        }
                    }

                    struct ProposedTriangle
                    {
                        uint32_t mA = 0;
                        uint32_t mB = 0;
                        uint32_t mC = 0;
                        glm::vec3 mV0 = glm::vec3(0.0f);
                        glm::vec3 mV1 = glm::vec3(0.0f);
                        glm::vec3 mV2 = glm::vec3(0.0f);
                        float mMinX = 0.0f;
                        float mMaxX = 0.0f;
                        float mMinZ = 0.0f;
                        float mMaxZ = 0.0f;
                        float mCentroidY = 0.0f;
                    };

                    std::vector<ProposedTriangle> proposedTriangles;
                    proposedTriangles.reserve(affectedTriangles.size());

                    for (uint32_t triIndex : affectedTriangles)
                    {
                        if (triIndex >= currentTriCount)
                        {
                            continue;
                        }

                        uint32_t t0 = indices[triIndex * 3 + 0];
                        uint32_t t1 = indices[triIndex * 3 + 1];
                        uint32_t t2 = indices[triIndex * 3 + 2];
                        if (t0 == remove) t0 = keep;
                        if (t1 == remove) t1 = keep;
                        if (t2 == remove) t2 = keep;

                        if (t0 == t1 || t1 == t2 || t2 == t0)
                        {
                            continue;
                        }

                        if (t0 >= vertices.size() || t1 >= vertices.size() || t2 >= vertices.size())
                        {
                            return false;
                        }

                        const glm::vec3 v0 = (t0 == keep) ? mergedPos : vertices[t0];
                        const glm::vec3 v1 = (t1 == keep) ? mergedPos : vertices[t1];
                        const glm::vec3 v2 = (t2 == keep) ? mergedPos : vertices[t2];
                        if (!IsFiniteVec3(v0) || !IsFiniteVec3(v1) || !IsFiniteVec3(v2))
                        {
                            return false;
                        }

                        const glm::vec3 cross = glm::cross(v1 - v0, v2 - v0);
                        const float crossLen = glm::length(cross);
                        if (crossLen <= 1e-8f)
                        {
                            return false;
                        }

                        const float area = crossLen * 0.5f;
                        if (area <= minTriangleArea)
                        {
                            return false;
                        }

                        const float normalY = cross.y / crossLen;
                        if (std::fabs(normalY) < walkableCos)
                        {
                            return false;
                        }

                        ProposedTriangle tri;
                        tri.mA = t0;
                        tri.mB = t1;
                        tri.mC = t2;
                        tri.mV0 = v0;
                        tri.mV1 = v1;
                        tri.mV2 = v2;
                        tri.mMinX = std::min({ v0.x, v1.x, v2.x });
                        tri.mMaxX = std::max({ v0.x, v1.x, v2.x });
                        tri.mMinZ = std::min({ v0.z, v1.z, v2.z });
                        tri.mMaxZ = std::max({ v0.z, v1.z, v2.z });
                        tri.mCentroidY = (v0.y + v1.y + v2.y) / 3.0f;
                        proposedTriangles.push_back(tri);
                    }

                    std::vector<uint32_t> nearbyCandidates;
                    nearbyCandidates.reserve(128);
                    for (const ProposedTriangle& tri : proposedTriangles)
                    {
                        if (++candidateStampValue == 0)
                        {
                            std::fill(candidateStamp.begin(), candidateStamp.end(), 0);
                            candidateStampValue = 1;
                        }

                        nearbyCandidates.clear();
                        const int32_t minBX = static_cast<int32_t>(std::floor(tri.mMinX * simplifyInvBucket));
                        const int32_t maxBX = static_cast<int32_t>(std::floor(tri.mMaxX * simplifyInvBucket));
                        const int32_t minBZ = static_cast<int32_t>(std::floor(tri.mMinZ * simplifyInvBucket));
                        const int32_t maxBZ = static_cast<int32_t>(std::floor(tri.mMaxZ * simplifyInvBucket));

                        for (int32_t bz = minBZ; bz <= maxBZ; ++bz)
                        {
                            for (int32_t bx = minBX; bx <= maxBX; ++bx)
                            {
                                auto bucketIter = simplificationBuckets.find(Bucket2DKey{ bx, bz });
                                if (bucketIter == simplificationBuckets.end())
                                {
                                    continue;
                                }

                                for (uint32_t candidateTriIndex : bucketIter->second)
                                {
                                    if (candidateTriIndex >= currentTriCount || !simplificationTriInfos[candidateTriIndex].mValid)
                                    {
                                        continue;
                                    }

                                    if (affectedStamp[candidateTriIndex] == affectedStampValue)
                                    {
                                        continue;
                                    }

                                    if (candidateStamp[candidateTriIndex] == candidateStampValue)
                                    {
                                        continue;
                                    }

                                    candidateStamp[candidateTriIndex] = candidateStampValue;
                                    nearbyCandidates.push_back(candidateTriIndex);
                                }
                            }
                        }

                        for (uint32_t candidateTriIndex : nearbyCandidates)
                        {
                            const SimplificationTriInfo& candidateTri = simplificationTriInfos[candidateTriIndex];
                            if (tri.mMaxX < candidateTri.mMinX - simplifyOverlapEps || tri.mMinX > candidateTri.mMaxX + simplifyOverlapEps ||
                                tri.mMaxZ < candidateTri.mMinZ - simplifyOverlapEps || tri.mMinZ > candidateTri.mMaxZ + simplifyOverlapEps)
                            {
                                continue;
                            }

                            if (CountSharedVertices(tri.mA, tri.mB, tri.mC, candidateTri.mA, candidateTri.mB, candidateTri.mC) >= 2)
                            {
                                continue;
                            }

                            if (!TrianglesOverlapInXZStrict(
                                    tri.mV0,
                                    tri.mV1,
                                    tri.mV2,
                                    candidateTri.mV0,
                                    candidateTri.mV1,
                                    candidateTri.mV2,
                                    simplifyOverlapEps))
                            {
                                continue;
                            }

                            if (std::fabs(tri.mCentroidY - candidateTri.mCentroidY) <= simplifyOverlapHeightEps)
                            {
                                return false;
                            }
                        }
                    }

                    return true;
                };

            std::vector<uint8_t> lockedVertex(vertices.size(), 0);
            std::vector<uint32_t> remap(vertices.size());
            for (uint32_t i = 0; i < remap.size(); ++i)
            {
                remap[i] = i;
            }

            size_t selectedCollapses = 0;
            for (const CollapseCandidate& candidate : candidates)
            {
                uint32_t a = candidate.a;
                uint32_t b = candidate.b;
                if (a >= vertices.size() || b >= vertices.size() || lockedVertex[a] || lockedVertex[b])
                {
                    continue;
                }

                uint32_t keep = a;
                uint32_t remove = b;
                if (valence[keep] > valence[remove] || (valence[keep] == valence[remove] && keep > remove))
                {
                    std::swap(keep, remove);
                }

                if (keep >= vertices.size() || remove >= vertices.size() || keep == remove)
                {
                    continue;
                }

                glm::vec3 merged = (vertices[keep] + vertices[remove]) * 0.5f;
                ClampVertexToBounds(merged);
                if (!IsCollapseLocallyValid(keep, remove, merged))
                {
                    continue;
                }

                vertices[keep] = merged;
                remap[remove] = keep;
                lockedVertex[a] = 1;
                lockedVertex[b] = 1;

                selectedCollapses++;
                if (selectedCollapses >= desiredCollapses)
                {
                    break;
                }
            }

            if (selectedCollapses == 0)
            {
                break;
            }

            for (uint32_t i = 0; i < remap.size(); ++i)
            {
                uint32_t r = i;
                while (remap[r] != r)
                {
                    r = remap[r];
                }

                uint32_t cur = i;
                while (remap[cur] != cur)
                {
                    const uint32_t next = remap[cur];
                    remap[cur] = r;
                    cur = next;
                }
            }

            for (uint32_t& index : indices)
            {
                if (index < remap.size())
                {
                    index = remap[index];
                }
            }

            const size_t triBeforeCleanup = indices.size() / 3;
            RemoveDegenerateAndDuplicateTriangleIndices(indices, vertices, minTriangleArea);
            const size_t triAfterCleanup = indices.size() / 3;
            if (indices.size() < 9 || triAfterCleanup >= triBeforeCleanup)
            {
                break;
            }
        }

        const size_t triAfterCollapse = indices.size() / 3;
        const bool doFlip = triAfterCollapse > 1 && triAfterCollapse <= 25000;
        if (doFlip)
        {
            struct EdgeIncident
            {
                uint32_t mTri = 0;
                uint8_t mSlot = 0;
                uint32_t mA = 0;
                uint32_t mB = 0;
                uint32_t mOpp = 0;
            };

            const int flipPassCount = (triAfterCollapse <= 20000) ? 2 : 1;
            for (int pass = 0; pass < flipPassCount; ++pass)
            {
                const size_t triCount = indices.size() / 3;
                std::vector<int32_t> flipValence(vertices.size(), 0);
                for (size_t t = 0; t < triCount; ++t)
                {
                    const uint32_t a = indices[t * 3 + 0];
                    const uint32_t b = indices[t * 3 + 1];
                    const uint32_t c = indices[t * 3 + 2];
                    if (a < flipValence.size()) flipValence[a]++;
                    if (b < flipValence.size()) flipValence[b]++;
                    if (c < flipValence.size()) flipValence[c]++;
                }

                std::unordered_map<UndirectedEdgeKey, std::vector<EdgeIncident>, UndirectedEdgeKeyHash> edgeIncidents;
                edgeIncidents.reserve(indices.size());
                for (size_t t = 0; t < triCount; ++t)
                {
                    const uint32_t tri[3] = {
                        indices[t * 3 + 0],
                        indices[t * 3 + 1],
                        indices[t * 3 + 2]
                    };

                    for (uint8_t e = 0; e < 3; ++e)
                    {
                        const uint32_t v0 = tri[e];
                        const uint32_t v1 = tri[(e + 1) % 3];
                        const uint32_t opp = tri[(e + 2) % 3];
                        if (v0 == v1 || v0 >= vertices.size() || v1 >= vertices.size() || opp >= vertices.size())
                        {
                            continue;
                        }

                        edgeIncidents[MakeUndirectedEdgeKey(v0, v1)].push_back(
                            EdgeIncident{ static_cast<uint32_t>(t), e, v0, v1, opp });
                    }
                }

                std::vector<uint8_t> triLocked(triCount, 0);
                bool anyFlip = false;

                for (const auto& [edgeKey, incidents] : edgeIncidents)
                {
                    if (incidents.size() != 2)
                    {
                        continue;
                    }

                    const EdgeIncident& i0 = incidents[0];
                    const EdgeIncident& i1 = incidents[1];
                    if (i0.mTri >= triLocked.size() || i1.mTri >= triLocked.size() || triLocked[i0.mTri] || triLocked[i1.mTri])
                    {
                        continue;
                    }

                    const uint32_t a = i0.mA;
                    const uint32_t b = i0.mB;
                    const uint32_t c = i0.mOpp;
                    const uint32_t d = i1.mOpp;

                    if (a == b || c == d || a == c || a == d || b == c || b == d)
                    {
                        continue;
                    }

                    const float flipIntersectionEps = 1e-5f;
                    if (!SegmentsIntersectStrictXZ(
                            vertices[a],
                            vertices[b],
                            vertices[c],
                            vertices[d],
                            flipIntersectionEps))
                    {
                        continue;
                    }

                    const uint32_t before0[3] = {
                        indices[i0.mTri * 3 + 0], indices[i0.mTri * 3 + 1], indices[i0.mTri * 3 + 2]
                    };
                    const uint32_t before1[3] = {
                        indices[i1.mTri * 3 + 0], indices[i1.mTri * 3 + 1], indices[i1.mTri * 3 + 2]
                    };

                    const float beforeQuality = std::min(
                        TriangleMinAngleRadians(vertices[before0[0]], vertices[before0[1]], vertices[before0[2]]),
                        TriangleMinAngleRadians(vertices[before1[0]], vertices[before1[1]], vertices[before1[2]]));

                    uint32_t after0[3] = { c, d, b };
                    uint32_t after1[3] = { d, c, a };

                    auto OrientUp = [&](uint32_t tri[3])
                        {
                            const glm::vec3& va = vertices[tri[0]];
                            const glm::vec3& vb = vertices[tri[1]];
                            const glm::vec3& vc = vertices[tri[2]];
                            if (glm::cross(vb - va, vc - va).y < 0.0f)
                            {
                                std::swap(tri[1], tri[2]);
                            }
                        };

                    OrientUp(after0);
                    OrientUp(after1);

                    const auto IsAcceptableTriangle = [&](const uint32_t tri[3])
                        {
                            if (tri[0] == tri[1] || tri[1] == tri[2] || tri[2] == tri[0])
                            {
                                return false;
                            }

                            const glm::vec3& va = vertices[tri[0]];
                            const glm::vec3& vb = vertices[tri[1]];
                            const glm::vec3& vc = vertices[tri[2]];
                            const float area = glm::length(glm::cross(vb - va, vc - va)) * 0.5f;
                            return area > minTriangleArea;
                        };

                    if (!IsAcceptableTriangle(after0) || !IsAcceptableTriangle(after1))
                    {
                        continue;
                    }

                    const float afterQuality = std::min(
                        TriangleMinAngleRadians(vertices[after0[0]], vertices[after0[1]], vertices[after0[2]]),
                        TriangleMinAngleRadians(vertices[after1[0]], vertices[after1[1]], vertices[after1[2]]));

                    const auto ValenceScore = [&](int32_t va, int32_t vb, int32_t vc, int32_t vd)
                        {
                            return std::abs(va - 6) + std::abs(vb - 6) + std::abs(vc - 6) + std::abs(vd - 6);
                        };

                    const int beforeValenceScore = ValenceScore(
                        flipValence[a], flipValence[b], flipValence[c], flipValence[d]);
                    const int afterValenceScore = ValenceScore(
                        flipValence[a] - 1, flipValence[b] - 1, flipValence[c] + 1, flipValence[d] + 1);

                    const bool qualityImprovedEnough =
                        (afterQuality > beforeQuality * 1.02f) ||
                        (afterQuality >= beforeQuality && afterValenceScore < beforeValenceScore);
                    if (!qualityImprovedEnough)
                    {
                        continue;
                    }

                    indices[i0.mTri * 3 + 0] = after0[0];
                    indices[i0.mTri * 3 + 1] = after0[1];
                    indices[i0.mTri * 3 + 2] = after0[2];

                    indices[i1.mTri * 3 + 0] = after1[0];
                    indices[i1.mTri * 3 + 1] = after1[1];
                    indices[i1.mTri * 3 + 2] = after1[2];

                    triLocked[i0.mTri] = 1;
                    triLocked[i1.mTri] = 1;
                    anyFlip = true;
                }

                if (!anyFlip)
                {
                    break;
                }

                RemoveDegenerateAndDuplicateTriangleIndices(indices, vertices, minTriangleArea);
            }
        }

        const bool doSmooth = (indices.size() / 3) <= 40000;
        if (doSmooth)
        {
            std::vector<int32_t> smoothValence;
            std::vector<uint8_t> smoothBoundary;
            std::vector<EdgeCandidate> smoothEdges;
            float avgLenSq = 0.0f;
            BuildEdgeData(indices, smoothValence, smoothBoundary, smoothEdges, avgLenSq);
            if (!smoothEdges.empty())
            {
                const int smoothPassCount = (indices.size() / 3 <= 25000) ? 2 : 1;
                for (int smoothPass = 0; smoothPass < smoothPassCount; ++smoothPass)
                {
                    std::vector<glm::vec3> neighbourSum(vertices.size(), glm::vec3(0.0f));
                    std::vector<uint16_t> neighbourCount(vertices.size(), 0);
                    for (const EdgeCandidate& edge : smoothEdges)
                    {
                        if (edge.a >= vertices.size() || edge.b >= vertices.size())
                        {
                            continue;
                        }

                        neighbourSum[edge.a] += vertices[edge.b];
                        neighbourSum[edge.b] += vertices[edge.a];
                        neighbourCount[edge.a]++;
                        neighbourCount[edge.b]++;
                    }

                    std::vector<glm::vec3> nextVertices = vertices;
                    for (uint32_t v = 0; v < vertices.size(); ++v)
                    {
                        if (v >= smoothBoundary.size() || smoothBoundary[v] || neighbourCount[v] < 3)
                        {
                            continue;
                        }

                        glm::vec3 avg = neighbourSum[v] / static_cast<float>(neighbourCount[v]);
                        nextVertices[v] = vertices[v] * 0.66f + avg * 0.34f;
                        ClampVertexToBounds(nextVertices[v]);
                    }

                    vertices.swap(nextVertices);
                }
            }
        }

        RemoveDegenerateAndDuplicateTriangleIndices(indices, vertices, minTriangleArea);
        std::vector<WorkingTriangle> rebuiltTriangles;
        RebuildWorkingTrianglesFromIndexBuffer(
            indices,
            vertices,
            walkableCos,
            minTriangleArea,
            rebuiltTriangles);

        if (rebuiltTriangles.size() >= 2)
        {
            trianglesInOut.swap(rebuiltTriangles);
        }
    }

    bool BuildHavokStyleGeometry(
        const float* inputVertices,
        int inputVertexCount,
        const int* inputIndices,
        int inputIndexCount,
        const application::file::game::phive::starlight_physics::ai::hkaiNavMeshGeometryGenerator::BuildConfig& config,
        bool useForcedXZBounds,
        float forcedMinX,
        float forcedMaxX,
        float forcedMinZ,
        float forcedMaxZ,
        std::vector<uint16_t>& outVertices,
        std::vector<uint16_t>& outTriangles,
        float outBoundsMin[3],
        float outBoundsMax[3])
    {
        outVertices.clear();
        outTriangles.clear();

        if (inputVertices == nullptr || inputIndices == nullptr || inputVertexCount <= 0 || inputIndexCount < 3)
        {
            return false;
        }

        const float cellSize = std::max(1e-4f, config.cs);
        const float cellHeight = std::max(1e-4f, config.ch);
        const float weldStep = std::max(1e-4f, std::min(cellSize * 0.125f, 0.05f));
        const float boundarySnapTolerance = std::max(cellSize * 0.6f, weldStep * 2.0f);
        const float walkableCos = std::cos(config.walkableSlopeAngle * DEG_TO_RAD);
        const float minTriangleArea = std::max(1e-7f, cellSize * cellSize * 1e-4f);
        const float coveredSurfaceDistanceLimit = std::max(2.0f, static_cast<float>(config.walkableHeight) * cellHeight);
        const SimplificationSettings simplificationSettings{
            config.enableSimplification,
            config.simplificationTargetRatio,
            config.simplificationMaxError,
        };

        std::vector<int32_t> sourceToWelded(static_cast<size_t>(inputVertexCount), -1);
        std::vector<glm::vec3> weldedVertices;
        weldedVertices.reserve(static_cast<size_t>(inputVertexCount));

        std::unordered_map<VertexKey, uint32_t, VertexKeyHash> weldedMap;
        weldedMap.reserve(static_cast<size_t>(inputVertexCount));

        for (int32_t i = 0; i < inputVertexCount; ++i)
        {
            glm::vec3 vertex(
                inputVertices[i * 3 + 0],
                inputVertices[i * 3 + 1],
                inputVertices[i * 3 + 2]);

            if (!IsFiniteVec3(vertex))
            {
                continue;
            }

            if (useForcedXZBounds)
            {
                vertex.x = SnapToBoundary(vertex.x, forcedMinX, forcedMaxX, boundarySnapTolerance);
                vertex.z = SnapToBoundary(vertex.z, forcedMinZ, forcedMaxZ, boundarySnapTolerance);
                vertex.x = ClampToRange(vertex.x, forcedMinX, forcedMaxX);
                vertex.z = ClampToRange(vertex.z, forcedMinZ, forcedMaxZ);
            }

            vertex.x = static_cast<float>(QuantizeToInt(vertex.x, weldStep)) * weldStep;
            vertex.y = static_cast<float>(QuantizeToInt(vertex.y, weldStep)) * weldStep;
            vertex.z = static_cast<float>(QuantizeToInt(vertex.z, weldStep)) * weldStep;

            const VertexKey key{
                static_cast<int64_t>(QuantizeToInt(vertex.x, weldStep)),
                static_cast<int64_t>(QuantizeToInt(vertex.y, weldStep)),
                static_cast<int64_t>(QuantizeToInt(vertex.z, weldStep)),
            };

            auto existing = weldedMap.find(key);
            if (existing != weldedMap.end())
            {
                sourceToWelded[static_cast<size_t>(i)] = static_cast<int32_t>(existing->second);
                continue;
            }

            const uint32_t newIndex = static_cast<uint32_t>(weldedVertices.size());
            weldedVertices.push_back(vertex);
            weldedMap.emplace(key, newIndex);
            sourceToWelded[static_cast<size_t>(i)] = static_cast<int32_t>(newIndex);
        }

        if (weldedVertices.empty())
        {
            return false;
        }

        std::vector<int32_t> woundInputIndices;
        NormalizeInputTriangleWinding(
            inputVertices,
            inputVertexCount,
            inputIndices,
            inputIndexCount,
            woundInputIndices);
        if (woundInputIndices.empty())
        {
            return false;
        }

        std::vector<WorkingTriangle> workingTriangles;
        workingTriangles.reserve(static_cast<size_t>(woundInputIndices.size() / 3));

        std::unordered_set<TriangleKey, TriangleKeyHash> seenInputTriangles;
        seenInputTriangles.reserve(static_cast<size_t>(woundInputIndices.size() / 3));

        for (size_t tri = 0; tri + 2 < woundInputIndices.size(); tri += 3)
        {
            const int32_t ia = woundInputIndices[tri + 0];
            const int32_t ib = woundInputIndices[tri + 1];
            const int32_t ic = woundInputIndices[tri + 2];

            if (ia < 0 || ib < 0 || ic < 0 || ia >= inputVertexCount || ib >= inputVertexCount || ic >= inputVertexCount)
            {
                continue;
            }

            const int32_t mappedA = sourceToWelded[static_cast<size_t>(ia)];
            const int32_t mappedB = sourceToWelded[static_cast<size_t>(ib)];
            const int32_t mappedC = sourceToWelded[static_cast<size_t>(ic)];
            if (mappedA < 0 || mappedB < 0 || mappedC < 0)
            {
                continue;
            }

            uint32_t a = static_cast<uint32_t>(mappedA);
            uint32_t b = static_cast<uint32_t>(mappedB);
            uint32_t c = static_cast<uint32_t>(mappedC);

            if (a >= weldedVertices.size() || b >= weldedVertices.size() || c >= weldedVertices.size())
            {
                continue;
            }

            if (a == b || b == c || c == a)
            {
                continue;
            }

            const glm::vec3& va = weldedVertices[a];
            const glm::vec3& vb = weldedVertices[b];
            const glm::vec3& vc = weldedVertices[c];

            glm::vec3 cross = glm::cross(vb - va, vc - va);
            const float crossLength = glm::length(cross);
            if (crossLength <= 1e-8f)
            {
                continue;
            }

            const float area = crossLength * 0.5f;
            if (area <= minTriangleArea)
            {
                continue;
            }

            glm::vec3 normal = cross / crossLength;
            if (std::fabs(normal.y) < walkableCos)
            {
                continue;
            }

            if (cross.y < 0.0f)
            {
                std::swap(b, c);
                normal = -normal;
            }
            if (normal.y < 0.0f)
            {
                normal = -normal;
            }

            const TriangleKey triangleKey = MakeTriangleKey(a, b, c);
            if (!seenInputTriangles.insert(triangleKey).second)
            {
                continue;
            }

            WorkingTriangle triangle;
            triangle.mVertices[0] = a;
            triangle.mVertices[1] = b;
            triangle.mVertices[2] = c;
            triangle.mNormal = normal;
            triangle.mArea = area;
            workingTriangles.push_back(triangle);
        }

        if (workingTriangles.empty())
        {
            return false;
        }

        RemoveCoveredLowerSurfaces(weldedVertices, cellSize, cellHeight, coveredSurfaceDistanceLimit, workingTriangles);
        RemoveOverlappingWalkableSurfaces(weldedVertices, cellSize, cellHeight, workingTriangles);
        if (workingTriangles.empty())
        {
            return false;
        }

        std::vector<std::vector<uint32_t>> faceAdjacency(workingTriangles.size());
        std::unordered_map<UndirectedEdgeKey, std::vector<uint32_t>, UndirectedEdgeKeyHash> edgeToFaces;
        edgeToFaces.reserve(workingTriangles.size() * 2);

        for (uint32_t faceIndex = 0; faceIndex < workingTriangles.size(); ++faceIndex)
        {
            const WorkingTriangle& face = workingTriangles[faceIndex];
            edgeToFaces[MakeUndirectedEdgeKey(face.mVertices[0], face.mVertices[1])].push_back(faceIndex);
            edgeToFaces[MakeUndirectedEdgeKey(face.mVertices[1], face.mVertices[2])].push_back(faceIndex);
            edgeToFaces[MakeUndirectedEdgeKey(face.mVertices[2], face.mVertices[0])].push_back(faceIndex);
        }

        for (const auto& edgeFacesPair : edgeToFaces)
        {
            const std::vector<uint32_t>& faces = edgeFacesPair.second;
            for (size_t i = 0; i < faces.size(); ++i)
            {
                for (size_t j = i + 1; j < faces.size(); ++j)
                {
                    const uint32_t a = faces[i];
                    const uint32_t b = faces[j];
                    faceAdjacency[a].push_back(b);
                    faceAdjacency[b].push_back(a);
                }
            }
        }

        const float thresholdArea = std::max(0.0f, static_cast<float>(config.minRegionArea) * cellSize * cellSize);
        if (thresholdArea > 0.0f && workingTriangles.size() > 1)
        {
            std::vector<uint8_t> visited(workingTriangles.size(), 0);
            struct RegionInfo
            {
                float mArea = 0.0f;
                std::vector<uint32_t> mFaces;
            };

            std::vector<RegionInfo> regions;

            for (uint32_t seedFace = 0; seedFace < workingTriangles.size(); ++seedFace)
            {
                if (visited[seedFace])
                {
                    continue;
                }

                RegionInfo region;
                std::queue<uint32_t> queue;
                queue.push(seedFace);
                visited[seedFace] = 1;

                while (!queue.empty())
                {
                    const uint32_t faceIndex = queue.front();
                    queue.pop();

                    region.mFaces.push_back(faceIndex);
                    region.mArea += workingTriangles[faceIndex].mArea;

                    for (uint32_t neighbour : faceAdjacency[faceIndex])
                    {
                        if (visited[neighbour])
                        {
                            continue;
                        }

                        visited[neighbour] = 1;
                        queue.push(neighbour);
                    }
                }

                regions.push_back(std::move(region));
            }

            size_t largestRegionIndex = 0;
            float largestRegionArea = -1.0f;
            for (size_t i = 0; i < regions.size(); ++i)
            {
                if (regions[i].mArea > largestRegionArea)
                {
                    largestRegionArea = regions[i].mArea;
                    largestRegionIndex = i;
                }
            }

            std::vector<uint8_t> keepFace(workingTriangles.size(), 0);
            for (size_t i = 0; i < regions.size(); ++i)
            {
                const bool keepRegion = (regions[i].mArea >= thresholdArea) || (i == largestRegionIndex);
                if (!keepRegion)
                {
                    continue;
                }

                for (uint32_t faceIndex : regions[i].mFaces)
                {
                    keepFace[faceIndex] = 1;
                }
            }

            std::vector<WorkingTriangle> filteredTriangles;
            filteredTriangles.reserve(workingTriangles.size());
            for (size_t i = 0; i < workingTriangles.size(); ++i)
            {
                if (keepFace[i])
                {
                    filteredTriangles.push_back(workingTriangles[i]);
                }
            }

            workingTriangles.swap(filteredTriangles);
            if (workingTriangles.empty())
            {
                return false;
            }
        }

        TrySimplifyWorkingTriangles(
            weldedVertices,
            simplificationSettings,
            walkableCos,
            minTriangleArea,
            useForcedXZBounds,
            forcedMinX,
            forcedMaxX,
            forcedMinZ,
            forcedMaxZ,
            workingTriangles);

        RemoveCoveredLowerSurfaces(weldedVertices, cellSize, cellHeight, coveredSurfaceDistanceLimit, workingTriangles);
        RemoveOverlappingWalkableSurfaces(weldedVertices, cellSize, cellHeight, workingTriangles);
        if (workingTriangles.empty())
        {
            return false;
        }

        std::vector<uint8_t> isVertexUsed(weldedVertices.size(), 0);
        for (const WorkingTriangle& face : workingTriangles)
        {
            isVertexUsed[face.mVertices[0]] = 1;
            isVertexUsed[face.mVertices[1]] = 1;
            isVertexUsed[face.mVertices[2]] = 1;
        }

        std::vector<uint32_t> weldedToCompact(weldedVertices.size(), std::numeric_limits<uint32_t>::max());
        std::vector<glm::vec3> compactVertices;
        compactVertices.reserve(weldedVertices.size());

        for (uint32_t i = 0; i < weldedVertices.size(); ++i)
        {
            if (!isVertexUsed[i])
            {
                continue;
            }

            weldedToCompact[i] = static_cast<uint32_t>(compactVertices.size());
            compactVertices.push_back(weldedVertices[i]);
        }

        for (WorkingTriangle& face : workingTriangles)
        {
            face.mVertices[0] = weldedToCompact[face.mVertices[0]];
            face.mVertices[1] = weldedToCompact[face.mVertices[1]];
            face.mVertices[2] = weldedToCompact[face.mVertices[2]];
        }

        if (compactVertices.empty())
        {
            return false;
        }

        outBoundsMin[0] = compactVertices[0].x;
        outBoundsMin[1] = compactVertices[0].y;
        outBoundsMin[2] = compactVertices[0].z;
        outBoundsMax[0] = compactVertices[0].x;
        outBoundsMax[1] = compactVertices[0].y;
        outBoundsMax[2] = compactVertices[0].z;

        for (const glm::vec3& vertex : compactVertices)
        {
            outBoundsMin[0] = std::min(outBoundsMin[0], vertex.x);
            outBoundsMin[1] = std::min(outBoundsMin[1], vertex.y);
            outBoundsMin[2] = std::min(outBoundsMin[2], vertex.z);
            outBoundsMax[0] = std::max(outBoundsMax[0], vertex.x);
            outBoundsMax[1] = std::max(outBoundsMax[1], vertex.y);
            outBoundsMax[2] = std::max(outBoundsMax[2], vertex.z);
        }

        if (useForcedXZBounds)
        {
            outBoundsMin[0] = forcedMinX;
            outBoundsMax[0] = forcedMaxX;
            outBoundsMin[2] = forcedMinZ;
            outBoundsMax[2] = forcedMaxZ;
        }

        if (outBoundsMax[1] - outBoundsMin[1] < cellHeight)
        {
            outBoundsMax[1] = outBoundsMin[1] + cellHeight;
        }

        std::vector<QuantizedVertexKey> quantizedVertices(compactVertices.size());
        for (size_t i = 0; i < compactVertices.size(); ++i)
        {
            glm::vec3 vertex = compactVertices[i];

            if (useForcedXZBounds)
            {
                vertex.x = SnapToBoundary(vertex.x, forcedMinX, forcedMaxX, boundarySnapTolerance);
                vertex.z = SnapToBoundary(vertex.z, forcedMinZ, forcedMaxZ, boundarySnapTolerance);
                vertex.x = ClampToRange(vertex.x, forcedMinX, forcedMaxX);
                vertex.z = ClampToRange(vertex.z, forcedMinZ, forcedMaxZ);
            }

            int32_t qx = static_cast<int32_t>(std::llround((vertex.x - outBoundsMin[0]) / cellSize));
            const int32_t qy = static_cast<int32_t>(std::llround((vertex.y - outBoundsMin[1]) / cellHeight));
            int32_t qz = static_cast<int32_t>(std::llround((vertex.z - outBoundsMin[2]) / cellSize));

            if (useForcedXZBounds)
            {
                const int32_t maxQX = std::max(0, static_cast<int32_t>(std::floor((forcedMaxX - forcedMinX) / cellSize + 1e-5f)));
                const int32_t maxQZ = std::max(0, static_cast<int32_t>(std::floor((forcedMaxZ - forcedMinZ) / cellSize + 1e-5f)));
                qx = std::clamp(qx, 0, maxQX);
                qz = std::clamp(qz, 0, maxQZ);
            }

            quantizedVertices[i] = QuantizedVertexKey{ ClampToU16(qx), ClampToU16(qy), ClampToU16(qz) };
        }

        std::unordered_map<QuantizedVertexKey, uint32_t, QuantizedVertexKeyHash> quantizedMap;
        quantizedMap.reserve(quantizedVertices.size());

        std::vector<QuantizedVertexKey> compactQuantizedVertices;
        compactQuantizedVertices.reserve(quantizedVertices.size());
        std::vector<uint32_t> quantizedRemap(quantizedVertices.size(), 0);

        for (uint32_t i = 0; i < quantizedVertices.size(); ++i)
        {
            const QuantizedVertexKey& key = quantizedVertices[i];
            auto existing = quantizedMap.find(key);
            if (existing != quantizedMap.end())
            {
                quantizedRemap[i] = existing->second;
                continue;
            }

            const uint32_t newIndex = static_cast<uint32_t>(compactQuantizedVertices.size());
            compactQuantizedVertices.push_back(key);
            quantizedMap.emplace(key, newIndex);
            quantizedRemap[i] = newIndex;
        }

        std::vector<WorkingTriangle> quantizedTriangles;
        quantizedTriangles.reserve(workingTriangles.size());
        std::unordered_set<TriangleKey, TriangleKeyHash> seenQuantizedTriangles;
        seenQuantizedTriangles.reserve(workingTriangles.size());

        for (WorkingTriangle face : workingTriangles)
        {
            uint32_t a = quantizedRemap[face.mVertices[0]];
            uint32_t b = quantizedRemap[face.mVertices[1]];
            uint32_t c = quantizedRemap[face.mVertices[2]];

            if (a == b || b == c || c == a)
            {
                continue;
            }

            glm::vec3 va = DequantizeVertex(compactQuantizedVertices[a], outBoundsMin, cellSize, cellHeight);
            glm::vec3 vb = DequantizeVertex(compactQuantizedVertices[b], outBoundsMin, cellSize, cellHeight);
            glm::vec3 vc = DequantizeVertex(compactQuantizedVertices[c], outBoundsMin, cellSize, cellHeight);

            glm::vec3 cross = glm::cross(vb - va, vc - va);
            const float crossLength = glm::length(cross);
            if (crossLength <= 1e-8f)
            {
                continue;
            }

            const float area = crossLength * 0.5f;
            if (area <= minTriangleArea)
            {
                continue;
            }

            glm::vec3 normal = cross / crossLength;
            if (std::fabs(normal.y) < walkableCos)
            {
                continue;
            }

            if (cross.y < 0.0f)
            {
                std::swap(b, c);
                normal = -normal;
            }
            if (normal.y < 0.0f)
            {
                normal = -normal;
            }

            const TriangleKey triangleKey = MakeTriangleKey(a, b, c);
            if (!seenQuantizedTriangles.insert(triangleKey).second)
            {
                continue;
            }

            face.mVertices[0] = a;
            face.mVertices[1] = b;
            face.mVertices[2] = c;
            face.mNormal = normal;
            face.mArea = area;
            quantizedTriangles.push_back(face);
        }

        if (quantizedTriangles.empty())
        {
            return false;
        }

        std::vector<glm::vec3> dequantizedVertices;
        dequantizedVertices.reserve(compactQuantizedVertices.size());
        for (const QuantizedVertexKey& vertex : compactQuantizedVertices)
        {
            dequantizedVertices.push_back(DequantizeVertex(vertex, outBoundsMin, cellSize, cellHeight));
        }

        RemoveCoveredLowerSurfaces(dequantizedVertices, cellSize, cellHeight, coveredSurfaceDistanceLimit, quantizedTriangles);
        RemoveOverlappingWalkableSurfaces(dequantizedVertices, cellSize, cellHeight, quantizedTriangles);
        if (quantizedTriangles.empty())
        {
            return false;
        }

        std::vector<uint8_t> finalVertexUsed(compactQuantizedVertices.size(), 0);
        for (const WorkingTriangle& face : quantizedTriangles)
        {
            finalVertexUsed[face.mVertices[0]] = 1;
            finalVertexUsed[face.mVertices[1]] = 1;
            finalVertexUsed[face.mVertices[2]] = 1;
        }

        std::vector<uint32_t> finalRemap(compactQuantizedVertices.size(), std::numeric_limits<uint32_t>::max());
        std::vector<QuantizedVertexKey> finalVertices;
        finalVertices.reserve(compactQuantizedVertices.size());

        for (uint32_t i = 0; i < compactQuantizedVertices.size(); ++i)
        {
            if (!finalVertexUsed[i])
            {
                continue;
            }

            finalRemap[i] = static_cast<uint32_t>(finalVertices.size());
            finalVertices.push_back(compactQuantizedVertices[i]);
        }

        for (WorkingTriangle& face : quantizedTriangles)
        {
            face.mVertices[0] = finalRemap[face.mVertices[0]];
            face.mVertices[1] = finalRemap[face.mVertices[1]];
            face.mVertices[2] = finalRemap[face.mVertices[2]];
        }

        std::vector<glm::vec3> faceCentroids(quantizedTriangles.size(), glm::vec3(0.0f));
        std::vector<glm::vec3> faceNormals(quantizedTriangles.size(), glm::vec3(0.0f));

        for (uint32_t faceIndex = 0; faceIndex < quantizedTriangles.size(); ++faceIndex)
        {
            const WorkingTriangle& face = quantizedTriangles[faceIndex];
            const glm::vec3 va = DequantizeVertex(finalVertices[face.mVertices[0]], outBoundsMin, cellSize, cellHeight);
            const glm::vec3 vb = DequantizeVertex(finalVertices[face.mVertices[1]], outBoundsMin, cellSize, cellHeight);
            const glm::vec3 vc = DequantizeVertex(finalVertices[face.mVertices[2]], outBoundsMin, cellSize, cellHeight);

            faceCentroids[faceIndex] = (va + vb + vc) / 3.0f;

            const glm::vec3 cross = glm::cross(vb - va, vc - va);
            const float crossLength = glm::length(cross);
            if (crossLength > 1e-8f)
            {
                faceNormals[faceIndex] = cross / crossLength;
            }
            else
            {
                faceNormals[faceIndex] = glm::vec3(0.0f, 1.0f, 0.0f);
            }
        }

        std::unordered_map<UndirectedEdgeKey, std::vector<EdgeRef>, UndirectedEdgeKeyHash> edgeBuckets;
        edgeBuckets.reserve(quantizedTriangles.size() * 2);

        for (uint32_t faceIndex = 0; faceIndex < quantizedTriangles.size(); ++faceIndex)
        {
            const WorkingTriangle& face = quantizedTriangles[faceIndex];
            const uint32_t v0 = face.mVertices[0];
            const uint32_t v1 = face.mVertices[1];
            const uint32_t v2 = face.mVertices[2];

            edgeBuckets[MakeUndirectedEdgeKey(v0, v1)].push_back(EdgeRef{ faceIndex, 0, v0, v1 });
            edgeBuckets[MakeUndirectedEdgeKey(v1, v2)].push_back(EdgeRef{ faceIndex, 1, v1, v2 });
            edgeBuckets[MakeUndirectedEdgeKey(v2, v0)].push_back(EdgeRef{ faceIndex, 2, v2, v0 });
        }

        std::vector<uint16_t> oppositeFaces(quantizedTriangles.size() * 3, INVALID_FACE_INDEX);

        for (const auto& bucketPair : edgeBuckets)
        {
            const std::vector<EdgeRef>& refs = bucketPair.second;
            if (refs.size() < 2)
            {
                continue;
            }

            std::vector<uint8_t> used(refs.size(), 0);

            for (size_t i = 0; i < refs.size(); ++i)
            {
                if (used[i])
                {
                    continue;
                }

                int bestMatch = -1;
                float bestScore = -std::numeric_limits<float>::infinity();

                for (size_t j = i + 1; j < refs.size(); ++j)
                {
                    if (used[j])
                    {
                        continue;
                    }

                    if (refs[i].mA != refs[j].mB || refs[i].mB != refs[j].mA)
                    {
                        continue;
                    }

                    if (refs[i].mFace == refs[j].mFace)
                    {
                        continue;
                    }

                    const float normalSimilarity = glm::dot(faceNormals[refs[i].mFace], faceNormals[refs[j].mFace]);
                    const float centroidDeltaY = std::fabs(faceCentroids[refs[i].mFace].y - faceCentroids[refs[j].mFace].y);
                    const float score = normalSimilarity - centroidDeltaY * 0.05f;

                    if (score > bestScore)
                    {
                        bestScore = score;
                        bestMatch = static_cast<int>(j);
                    }
                }

                if (bestMatch < 0)
                {
                    continue;
                }

                used[i] = 1;
                used[static_cast<size_t>(bestMatch)] = 1;

                const EdgeRef& a = refs[i];
                const EdgeRef& b = refs[static_cast<size_t>(bestMatch)];

                const uint16_t oppositeFaceForA = (b.mFace <= 0xFFFFu) ? static_cast<uint16_t>(b.mFace) : INVALID_FACE_INDEX;
                const uint16_t oppositeFaceForB = (a.mFace <= 0xFFFFu) ? static_cast<uint16_t>(a.mFace) : INVALID_FACE_INDEX;

                oppositeFaces[a.mFace * 3 + a.mLocalEdge] = oppositeFaceForA;
                oppositeFaces[b.mFace * 3 + b.mLocalEdge] = oppositeFaceForB;
            }
        }

        outVertices.resize(finalVertices.size() * 3);
        for (size_t i = 0; i < finalVertices.size(); ++i)
        {
            outVertices[i * 3 + 0] = finalVertices[i].mX;
            outVertices[i * 3 + 1] = finalVertices[i].mY;
            outVertices[i * 3 + 2] = finalVertices[i].mZ;
        }

        outTriangles.resize(quantizedTriangles.size() * 6);
        for (size_t faceIndex = 0; faceIndex < quantizedTriangles.size(); ++faceIndex)
        {
            const WorkingTriangle& face = quantizedTriangles[faceIndex];
            outTriangles[faceIndex * 6 + 0] = static_cast<uint16_t>(face.mVertices[0]);
            outTriangles[faceIndex * 6 + 1] = static_cast<uint16_t>(face.mVertices[1]);
            outTriangles[faceIndex * 6 + 2] = static_cast<uint16_t>(face.mVertices[2]);
            outTriangles[faceIndex * 6 + 3] = oppositeFaces[faceIndex * 3 + 0];
            outTriangles[faceIndex * 6 + 4] = oppositeFaces[faceIndex * 3 + 1];
            outTriangles[faceIndex * 6 + 5] = oppositeFaces[faceIndex * 3 + 2];
        }

        return !outVertices.empty() && !outTriangles.empty();
    }
}

namespace application::file::game::phive::starlight_physics::ai
{
    void hkaiNavMeshGeometryGenerator::setNavmeshBuildParams(const Config& config)
    {
        mConfig.cs = std::max(1e-4f, config.mCellSize);
        mConfig.ch = std::max(1e-4f, config.mCellHeight);
        mConfig.walkableSlopeAngle = config.mWalkableSlopeAngle;
        mConfig.walkableHeight = static_cast<int>(std::ceil(config.mWalkableHeight / mConfig.ch)); //Unused
        mConfig.walkableClimb = static_cast<int>(std::floor(config.mWalkableClimb / mConfig.ch));  //Unused
        mConfig.walkableRadius = static_cast<int>(std::ceil(config.mWalkableRadius / mConfig.cs));  //Unused
        mConfig.minRegionArea = std::max(0, config.mMinRegionArea * config.mMinRegionArea);
        mConfig.mergeRegionArea = std::max(0, config.mMinRegionArea * config.mMinRegionArea * 2);  //Unused
        mConfig.maxVertsPerPoly = 3;  //Unused
        mConfig.enableSimplification = config.mEnableSimplification;
        const float requestedTargetRatio = std::clamp(config.mSimplificationTargetRatio, 0.003f, 1.0f);
        const float requestedMaxError = std::clamp(config.mSimplificationMaxError, 0.001f, 1.0f);
        mConfig.simplificationTargetRatio = std::clamp(requestedTargetRatio * 0.72f, 0.003f, 1.0f);
        mConfig.simplificationMaxError = std::clamp(requestedMaxError * 1.35f, 0.001f, 1.0f);
    }

    void hkaiNavMeshGeometryGenerator::setForcedXZBounds(float minX, float maxX, float minZ, float maxZ)
    {
        mUseForcedXZBounds = true;
        mForcedMinX = minX;
        mForcedMaxX = maxX;
        mForcedMinZ = minZ;
        mForcedMaxZ = maxZ;
    }

    void hkaiNavMeshGeometryGenerator::clearForcedXZBounds()
    {
        mUseForcedXZBounds = false;
    }

    bool hkaiNavMeshGeometryGenerator::buildNavmeshForMesh(const float* vertices, int verticesCount, const int* indices, int indicesCount)
    {
        mMeshVertices.clear();
        mMeshTriangles.clear();

        if (vertices == nullptr || indices == nullptr || verticesCount <= 0 || indicesCount < 3)
        {
            return false;
        }

        float boundsMin[3] = { 0.0f, 0.0f, 0.0f };
        float boundsMax[3] = { 0.0f, 0.0f, 0.0f };

        if (!BuildHavokStyleGeometry(
                vertices,
                verticesCount,
                indices,
                indicesCount,
                mConfig,
                mUseForcedXZBounds,
                mForcedMinX,
                mForcedMaxX,
                mForcedMinZ,
                mForcedMaxZ,
                mMeshVertices,
                mMeshTriangles,
                boundsMin,
                boundsMax))
        {
            return false;
        }

        mBoundingBoxMin[0] = boundsMin[0];
        mBoundingBoxMin[1] = boundsMin[1];
        mBoundingBoxMin[2] = boundsMin[2];

        mBoundingBoxMax[0] = boundsMax[0];
        mBoundingBoxMax[1] = boundsMax[1];
        mBoundingBoxMax[2] = boundsMax[2];

        return true;
    }

    int hkaiNavMeshGeometryGenerator::getMeshVertexCount()
    {
        return static_cast<int>(mMeshVertices.size() / 3);
    }

    int hkaiNavMeshGeometryGenerator::getMeshTriangleCount()
    {
        return static_cast<int>(mMeshTriangles.size() / 6);
    }

    void hkaiNavMeshGeometryGenerator::getMeshVertices(void* buffer)
    {
        if (buffer == nullptr || mMeshVertices.empty())
        {
            return;
        }

        std::memcpy(buffer, mMeshVertices.data(), mMeshVertices.size() * sizeof(uint16_t));
    }

    void hkaiNavMeshGeometryGenerator::getMeshTriangles(void* buffer)
    {
        if (buffer == nullptr || mMeshTriangles.empty())
        {
            return;
        }

        std::memcpy(buffer, mMeshTriangles.data(), mMeshTriangles.size() * sizeof(uint16_t));
    }

    hkaiNavMeshGeometryGenerator::~hkaiNavMeshGeometryGenerator() = default;
}
