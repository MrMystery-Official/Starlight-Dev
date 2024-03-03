#pragma once

#include "Camera.h"

namespace Frustum
{
	extern float FrustumValues[6][4];

	void NormalizePlane(float FurstumArr[6][4], int Side);
	void CalculateFrustum(Camera* CameraView);
	bool SphereInFrustum(float X, float Y, float Z, float Radius);
};