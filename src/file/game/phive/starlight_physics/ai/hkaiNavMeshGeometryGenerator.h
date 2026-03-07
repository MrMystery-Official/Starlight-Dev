#pragma once

#include <cstdint>
#include <vector>

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
			bool mEnableSimplification = true;
			float mSimplificationTargetRatio = 0.14f;
			float mSimplificationMaxError = 0.28f;
		};

		struct BuildConfig
		{
			float cs = 0.3f;
			float ch = 0.3f;
			float walkableSlopeAngle = 30.0f;
			int walkableHeight = 1;
			int walkableClimb = 0;
			int walkableRadius = 0;
			int minRegionArea = 0;
			int mergeRegionArea = 0;
			int maxVertsPerPoly = 3;
			bool enableSimplification = true;
			float simplificationTargetRatio = 0.14f;
			float simplificationMaxError = 0.28f;
		};

		BuildConfig mConfig;
		std::vector<uint16_t> mMeshVertices;
		std::vector<uint16_t> mMeshTriangles;

		float mBoundingBoxMin[3] = { 0 };
		float mBoundingBoxMax[3] = { 0 };
		bool mUseForcedXZBounds = false;
		float mForcedMinX = 0.0f;
		float mForcedMaxX = 0.0f;
		float mForcedMinZ = 0.0f;
		float mForcedMaxZ = 0.0f;

		void setNavmeshBuildParams(const Config& config);
		void setForcedXZBounds(float minX, float maxX, float minZ, float maxZ);
		void clearForcedXZBounds();
		bool buildNavmeshForMesh(const float* vertices, int verticesCount, const int* indices, int indicesCount);
		int getMeshVertexCount();
		int getMeshTriangleCount();
		void getMeshVertices(void* buffer);
		void getMeshTriangles(void* buffer);

		~hkaiNavMeshGeometryGenerator();
	};
}
