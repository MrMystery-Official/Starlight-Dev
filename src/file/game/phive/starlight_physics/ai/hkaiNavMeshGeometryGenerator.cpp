#include <file/game/phive/starlight_physics/ai/hkaiNavMeshGeometryGenerator.h>

#include <memory>
#include <cmath>

namespace application::file::game::phive::starlight_physics::ai
{
    void hkaiNavMeshGeometryGenerator::setNavmeshBuildParams(const Config& config)
    {
        memset(&mConfig, 0, sizeof(mConfig));
        mConfig.cs = config.mCellSize;
        mConfig.ch = config.mCellHeight;
        mConfig.walkableSlopeAngle = config.mWalkableSlopeAngle;
        mConfig.walkableHeight = (int)std::ceilf(config.mWalkableHeight / config.mCellHeight);
        mConfig.walkableClimb = (int)std::floorf(config.mWalkableClimb / config.mCellHeight);
        mConfig.walkableRadius = (int)std::ceilf(config.mWalkableRadius / config.mCellSize);
        mConfig.maxEdgeLen = (int)12;
        mConfig.maxSimplificationError = 1.3f;
        mConfig.minRegionArea = (int)rcSqr(config.mMinRegionArea); // Note: area = size*size
        mConfig.mergeRegionArea = (int)rcSqr(20); // Note: area = size*size
        mConfig.maxVertsPerPoly = 3;
        mConfig.detailSampleDist = 6;
        mConfig.detailSampleMaxError = 1;

        mConfig.maxSimplificationError = 1.5f;
        mConfig.maxEdgeLen = config.mWalkableRadius * 16.0f;
    }

    bool hkaiNavMeshGeometryGenerator::buildNavmeshForMesh(const float* vertices, int verticesCount, const int* indices, int indicesCount)
    {
        if (mContext == nullptr)
            mContext = new rcContext();

        // Calculate bounding box
        for (int i = 0; i < verticesCount; i++) 
        {
            for (int j = 0; j < 3; j++) 
            {
                if (i == 0) 
                {
                    mBoundingBoxMin[j] = vertices[j];
                    mBoundingBoxMax[j] = vertices[j];
                }
                else 
                {
                    float v = vertices[i * 3 + j];
                    if (v < mBoundingBoxMin[j]) 
                    {
                        mBoundingBoxMin[j] = v;
                    }
                    if (v > mBoundingBoxMax[j]) 
                    {
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
        if (!HeightField) 
        {
            return false;
        }
        if (!rcCreateHeightfield(mContext, *HeightField, mConfig.width, mConfig.height, mConfig.bmin, mConfig.bmax, mConfig.cs,
            mConfig.ch)) 
        {
            return false;
        }

        auto* triareas = new unsigned char[indicesCount / 3];
        memset(triareas, 0, (indicesCount / 3) * sizeof(unsigned char));
        rcMarkWalkableTriangles(mContext, mConfig.walkableSlopeAngle, vertices, verticesCount, indices, indicesCount / 3, triareas);
        if (!rcRasterizeTriangles(mContext, vertices, verticesCount, indices, triareas, indicesCount / 3, *HeightField,
            mConfig.walkableClimb)) 
        {
            return false;
        }

        delete[] triareas;

        // Step 3: Filter walkable surfaces

        // Step 4: Partitioning
        rcCompactHeightfield* cHeightField = rcAllocCompactHeightfield();
        if (!cHeightField) 
        {
            return false;
        }
        if (!rcBuildCompactHeightfield(mContext, mConfig.walkableHeight, mConfig.walkableClimb, *HeightField, *cHeightField)) 
        {
            return false;
        }
        rcFreeHeightField(HeightField);

        if (!rcErodeWalkableArea(mContext, mConfig.walkableRadius, *cHeightField)) 
        {
            return false;
        }

        // Use watershed partitioning
        if (!rcBuildDistanceField(mContext, *cHeightField)) 
        {
            return false;
        }

        if (!rcBuildRegions(mContext, *cHeightField, 0, mConfig.minRegionArea, mConfig.mergeRegionArea)) 
        {
            return false;
        }

        // Step 5: Contours
        rcContourSet* cSet = rcAllocContourSet();
        if (!cSet) 
        {
            return false;
        }
        if (!rcBuildContours(mContext, *cHeightField, mConfig.maxSimplificationError, mConfig.maxEdgeLen, *cSet)) 
        {
            return false;
        }

        // Step 6: poly mesh
        if (mCurrentMesh != nullptr) 
        {
            rcFreePolyMesh(mCurrentMesh);
        }
        mCurrentMesh = rcAllocPolyMesh();
        if (!mCurrentMesh) 
        {
            return false;
        }
        if (!rcBuildPolyMesh(mContext, *cSet, mConfig.maxVertsPerPoly, *mCurrentMesh)) 
        {
            return false;
        }

        return true;

    }

    int hkaiNavMeshGeometryGenerator::getMeshVertexCount()
    {
        if (mCurrentMesh != nullptr) {
            return mCurrentMesh->nverts;
        }
        return -1;
    }

    int hkaiNavMeshGeometryGenerator::getMeshTriangleCount()
    {
        if (mCurrentMesh != nullptr) {
            return mCurrentMesh->npolys;
        }
        return -1;
    }

    void hkaiNavMeshGeometryGenerator::getMeshVertices(void* Buffer)
    {
        if (mCurrentMesh != nullptr) {
            memcpy(Buffer, mCurrentMesh->verts, mCurrentMesh->nverts * sizeof(short) * 3);
        }
    }

    void hkaiNavMeshGeometryGenerator::getMeshTriangles(void* Buffer)
    {
        if (mCurrentMesh != nullptr) {
            memcpy(Buffer, mCurrentMesh->polys, mCurrentMesh->npolys * sizeof(short) * 2 * mCurrentMesh->nvp);
        }
    }

    hkaiNavMeshGeometryGenerator::~hkaiNavMeshGeometryGenerator()
    {
        rcFreePolyMesh(mCurrentMesh);
    }
}