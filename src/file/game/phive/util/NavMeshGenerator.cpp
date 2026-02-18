#include "NavMeshGenerator.h"

#include <memory>
#include <cmath>

namespace application::file::game::phive::util
{
    void NavMeshGenerator::SetNavmeshBuildParams(const PhiveNavMeshGeneratorConfig& Config)
    {
        memset(&mConfig, 0, sizeof(mConfig));
        mConfig.cs = Config.mCellSize;
        mConfig.ch = Config.mCellHeight;
        mConfig.walkableSlopeAngle = Config.mWalkableSlopeAngle;
        mConfig.walkableHeight = (int)std::ceilf(Config.mWalkableHeight / Config.mCellHeight);
        mConfig.walkableClimb = (int)std::floorf(Config.mWalkableClimb / Config.mCellHeight);
        mConfig.walkableRadius = (int)std::ceilf(Config.mWalkableRadius / Config.mCellSize);
        mConfig.maxEdgeLen = (int)12;
        mConfig.maxSimplificationError = 1.3f;
        mConfig.minRegionArea = (int)rcSqr(Config.mMinRegionArea); // Note: area = size*size
        mConfig.mergeRegionArea = (int)rcSqr(20); // Note: area = size*size
        mConfig.maxVertsPerPoly = 3;
        mConfig.detailSampleDist = 6;
        mConfig.detailSampleMaxError = 1;

        mConfig.maxSimplificationError = 1.5f;
        mConfig.maxEdgeLen = Config.mWalkableRadius * 16.0f;
    }

    bool NavMeshGenerator::BuildNavmeshForMesh(const float* Vertices, int VerticesCount, const int* Indices, int IndicesCount)
    {
        if (mContext == nullptr)
            mContext = new rcContext();

        // Calculate bounding box
        for (int i = 0; i < VerticesCount; i++) {
            for (int j = 0; j < 3; j++) {
                if (i == 0) {
                    mBoundingBoxMin[j] = Vertices[j];
                    mBoundingBoxMax[j] = Vertices[j];
                }
                else {
                    float v = Vertices[i * 3 + j];
                    if (v < mBoundingBoxMin[j]) {
                        mBoundingBoxMin[j] = v;
                    }
                    if (v > mBoundingBoxMax[j]) {
                        mBoundingBoxMax[j] = v;
                    }
                }
            }
        }

        rcVcopy(mConfig.bmin, mBoundingBoxMin);
        rcVcopy(mConfig.bmax, mBoundingBoxMax);
        rcCalcGridSize(mConfig.bmin, mConfig.bmax, mConfig.cs, &mConfig.width, &mConfig.height);

        // Step 2: rasterization
        rcHeightfield* HeightField = rcAllocHeightfield();
        if (!HeightField) {
            return false;
        }
        if (!rcCreateHeightfield(mContext, *HeightField, mConfig.width, mConfig.height, mConfig.bmin, mConfig.bmax, mConfig.cs,
            mConfig.ch)) {
            return false;
        }

        auto* triareas = new unsigned char[IndicesCount / 3];
        memset(triareas, 0, (IndicesCount / 3) * sizeof(unsigned char));
        rcMarkWalkableTriangles(mContext, mConfig.walkableSlopeAngle, Vertices, VerticesCount, Indices, IndicesCount / 3, triareas);
        if (!rcRasterizeTriangles(mContext, Vertices, VerticesCount, Indices, triareas, IndicesCount / 3, *HeightField,
            mConfig.walkableClimb)) {
            return false;
        }

        delete[] triareas;

        // Step 3: Filter walkable surfaces

        // Step 4: Partitioning
        rcCompactHeightfield* cHeightField = rcAllocCompactHeightfield();
        if (!cHeightField) {
            return false;
        }
        if (!rcBuildCompactHeightfield(mContext, mConfig.walkableHeight, mConfig.walkableClimb, *HeightField, *cHeightField)) {
            return false;
        }
        rcFreeHeightField(HeightField);

        if (!rcErodeWalkableArea(mContext, mConfig.walkableRadius, *cHeightField)) {
            return false;
        }

        // Use watershed partitioning
        if (!rcBuildDistanceField(mContext, *cHeightField)) {
            return false;
        }

        if (!rcBuildRegions(mContext, *cHeightField, 0, mConfig.minRegionArea, mConfig.mergeRegionArea)) {
            return false;
        }

        // Step 5: Contours
        rcContourSet* cSet = rcAllocContourSet();
        if (!cSet) {
            return false;
        }
        if (!rcBuildContours(mContext, *cHeightField, mConfig.maxSimplificationError, mConfig.maxEdgeLen, *cSet)) {
            return false;
        }

        // Step 6: poly mesh
        if (mCurrentMesh != nullptr) {
            rcFreePolyMesh(mCurrentMesh);
        }
        mCurrentMesh = rcAllocPolyMesh();
        if (!mCurrentMesh) {
            return false;
        }
        if (!rcBuildPolyMesh(mContext, *cSet, mConfig.maxVertsPerPoly, *mCurrentMesh)) {
            return false;
        }

        return true;

    }

    int NavMeshGenerator::GetMeshVertexCount()
    {
        if (mCurrentMesh != nullptr) {
            return mCurrentMesh->nverts;
        }
        return -1;
    }

    int NavMeshGenerator::GetMeshTriangleCount()
    {
        if (mCurrentMesh != nullptr) {
            return mCurrentMesh->npolys;
        }
        return -1;
    }

    void NavMeshGenerator::GetMeshVertices(void* Buffer)
    {
        if (mCurrentMesh != nullptr) {
            memcpy(Buffer, mCurrentMesh->verts, mCurrentMesh->nverts * sizeof(short) * 3);
        }
    }

    void NavMeshGenerator::GetMeshTriangles(void* Buffer)
    {
        if (mCurrentMesh != nullptr) {
            memcpy(Buffer, mCurrentMesh->polys, mCurrentMesh->npolys * sizeof(short) * 2 * mCurrentMesh->nvp);
        }
    }

    NavMeshGenerator::~NavMeshGenerator()
    {
        rcFreePolyMesh(mCurrentMesh);
    }
}