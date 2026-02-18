#pragma once

#include <Recast.h>
#include <glm/vec3.hpp>

namespace application::file::game::phive::util
{
	struct NavMeshGenerator
	{
		struct PhiveNavMeshGeneratorConfig
		{
			float mCellSize = 0.3f;
			float mCellHeight = 0.3f;
			float mWalkableSlopeAngle = 30.0f;
			float mWalkableHeight = 2.0f;
			float mWalkableClimb = 1.0f;
			float mWalkableRadius = 0.5f;
			int mMinRegionArea = 3;
			int mKMeansClusteringK = 30;
			bool mEverythingBakeable = false;
		};

		rcConfig mConfig;
		rcPolyMesh* mCurrentMesh = nullptr;
		rcContext* mContext = nullptr;

		float mBoundingBoxMin[3] = { 0 };
		float mBoundingBoxMax[3] = { 0 };

		void SetNavmeshBuildParams(const PhiveNavMeshGeneratorConfig& Config);
		bool BuildNavmeshForMesh(const float* Vertices, int VerticesCount, const int* Indices, int IndicesCount);
		int GetMeshVertexCount();
		int GetMeshTriangleCount();
		void GetMeshVertices(void* Buffer);
		void GetMeshTriangles(void* Buffer);

		~NavMeshGenerator();
	};
}