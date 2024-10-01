#pragma once

#include "recastnavigation/recast/Recast.h"

class NavMeshGenerator
{
public:
	rcConfig mConfig;
	rcPolyMesh* mCurrentMesh = nullptr;
	rcContext* mContext = nullptr;

	float mBoundingBoxMin[3] = { 0 };
	float mBoundingBoxMax[3] = { 0 };

	void SetNavmeshBuildParams(float CS, float CH, float WalkableSlopeAngle, float WalkableHeight, float WalkableClimb, float WalkableRadius, int MinRegionArea);
	bool BuildNavmeshForMesh(float* Vertices, int VerticesCount, int* Indices, int IndicesCount);
	int GetMeshVertexCount();
	int GetMeshTriangleCount();
	void GetMeshVertices(void* Buffer);
	void GetMeshTriangles(void* Buffer);

	~NavMeshGenerator();
};