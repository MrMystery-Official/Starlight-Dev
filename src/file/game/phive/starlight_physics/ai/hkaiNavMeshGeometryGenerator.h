#pragma once

#include <Recast.h>
#include <file/game/phive/starlight_physics/common/hkAabb.h>
#include <file/game/phive/starlight_physics/common/hkGeometry.h>

namespace application::file::game::phive::starlight_physics::ai
{
	struct hkaiNavMeshGeometryGenerator
	{
		struct Config
		{
			float mCellSize = 0.3f;
			float mCellHeight = 0.3f;
			float mWalkableSlopeAngle = 30.0f;
			float mWalkableHeight = 2.0f;
			float mWalkableClimb = 1.0f;
			float mWalkableRadius = 0.5f;
			int mMinRegionArea = 3;
		};

		rcConfig mConfig;
		rcPolyMesh* mCurrentMesh = nullptr;
		rcContext* mContext = nullptr;

		float mBoundingBoxMin[3] = { 0 };
		float mBoundingBoxMax[3] = { 0 };

		void setNavmeshBuildParams(const Config& config);
		bool buildNavmeshForMesh(const float* vertices, int verticesCount, const int* indices, int indicesCount);
		int getMeshVertexCount();
		int getMeshTriangleCount();
		void getMeshVertices(void* buffer);
		void getMeshTriangles(void* buffer);

		~hkaiNavMeshGeometryGenerator();
	};
}