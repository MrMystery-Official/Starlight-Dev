#pragma once

#include <gl/Camera.h>

namespace application::util
{
	class Frustum
	{
	public:
		float mFrustumValues[6][4] = { 0 };

		void NormalizePlane(float FurstumArr[6][4], int Side);
		void CalculateFrustum(application::gl::Camera& CameraView);
		bool SphereInFrustum(float X, float Y, float Z, float Radius);
	};
}