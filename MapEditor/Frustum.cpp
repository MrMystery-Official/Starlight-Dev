#include "Frustum.h"

#include <glm/gtc/type_ptr.hpp>

float Frustum::FrustumValues[6][4];

enum FrustumSide
{
	RIGHT = 0,
	LEFT = 1,
	BOTTOM = 2,
	TOP = 3,
	BACK = 4,
	FRONT = 5
};

enum PlaneData
{
	A = 0,
	B = 1,
	C = 2,
	D = 3
};

void Frustum::NormalizePlane(float FurstumArr[6][4], int Side)
{
	float Magnitude = (float)sqrt(FurstumArr[Side][A] * FurstumArr[Side][A] +
		FurstumArr[Side][B] * FurstumArr[Side][B] +
		FurstumArr[Side][C] * FurstumArr[Side][C]);

	// Then we divide the plane's values by it's magnitude.
	// This makes it easier to work with.
	FurstumArr[Side][A] /= Magnitude;
	FurstumArr[Side][B] /= Magnitude;
	FurstumArr[Side][C] /= Magnitude;
	FurstumArr[Side][D] /= Magnitude;
}

void Frustum::CalculateFrustum(Camera* CameraView)
{
	const float* proj = glm::value_ptr(CameraView->GetProjectionMatrix());
	const float* modl = glm::value_ptr(CameraView->GetViewMatrix());
	float clip[16];

	clip[0] = modl[0] * proj[0] + modl[1] * proj[4] + modl[2] * proj[8] + modl[3] * proj[12];
	clip[1] = modl[0] * proj[1] + modl[1] * proj[5] + modl[2] * proj[9] + modl[3] * proj[13];
	clip[2] = modl[0] * proj[2] + modl[1] * proj[6] + modl[2] * proj[10] + modl[3] * proj[14];
	clip[3] = modl[0] * proj[3] + modl[1] * proj[7] + modl[2] * proj[11] + modl[3] * proj[15];

	clip[4] = modl[4] * proj[0] + modl[5] * proj[4] + modl[6] * proj[8] + modl[7] * proj[12];
	clip[5] = modl[4] * proj[1] + modl[5] * proj[5] + modl[6] * proj[9] + modl[7] * proj[13];
	clip[6] = modl[4] * proj[2] + modl[5] * proj[6] + modl[6] * proj[10] + modl[7] * proj[14];
	clip[7] = modl[4] * proj[3] + modl[5] * proj[7] + modl[6] * proj[11] + modl[7] * proj[15];

	clip[8] = modl[8] * proj[0] + modl[9] * proj[4] + modl[10] * proj[8] + modl[11] * proj[12];
	clip[9] = modl[8] * proj[1] + modl[9] * proj[5] + modl[10] * proj[9] + modl[11] * proj[13];
	clip[10] = modl[8] * proj[2] + modl[9] * proj[6] + modl[10] * proj[10] + modl[11] * proj[14];
	clip[11] = modl[8] * proj[3] + modl[9] * proj[7] + modl[10] * proj[11] + modl[11] * proj[15];

	clip[12] = modl[12] * proj[0] + modl[13] * proj[4] + modl[14] * proj[8] + modl[15] * proj[12];
	clip[13] = modl[12] * proj[1] + modl[13] * proj[5] + modl[14] * proj[9] + modl[15] * proj[13];
	clip[14] = modl[12] * proj[2] + modl[13] * proj[6] + modl[14] * proj[10] + modl[15] * proj[14];
	clip[15] = modl[12] * proj[3] + modl[13] * proj[7] + modl[14] * proj[11] + modl[15] * proj[15];

	FrustumValues[RIGHT][A] = clip[3] - clip[0];
	FrustumValues[RIGHT][B] = clip[7] - clip[4];
	FrustumValues[RIGHT][C] = clip[11] - clip[8];
	FrustumValues[RIGHT][D] = clip[15] - clip[12];
	NormalizePlane(FrustumValues, RIGHT);

	FrustumValues[LEFT][A] = clip[3] + clip[0];
	FrustumValues[LEFT][B] = clip[7] + clip[4];
	FrustumValues[LEFT][C] = clip[11] + clip[8];
	FrustumValues[LEFT][D] = clip[15] + clip[12];
	NormalizePlane(FrustumValues, LEFT);

	FrustumValues[BOTTOM][A] = clip[3] + clip[1];
	FrustumValues[BOTTOM][B] = clip[7] + clip[5];
	FrustumValues[BOTTOM][C] = clip[11] + clip[9];
	FrustumValues[BOTTOM][D] = clip[15] + clip[13];
	NormalizePlane(FrustumValues, BOTTOM);

	FrustumValues[TOP][A] = clip[3] - clip[1];
	FrustumValues[TOP][B] = clip[7] - clip[5];
	FrustumValues[TOP][C] = clip[11] - clip[9];
	FrustumValues[TOP][D] = clip[15] - clip[13];
	NormalizePlane(FrustumValues, TOP);

	FrustumValues[BACK][A] = clip[3] - clip[2];
	FrustumValues[BACK][B] = clip[7] - clip[6];
	FrustumValues[BACK][C] = clip[11] - clip[10];
	FrustumValues[BACK][D] = clip[15] - clip[14];
	NormalizePlane(FrustumValues, BACK);

	FrustumValues[FRONT][A] = clip[3] + clip[2];
	FrustumValues[FRONT][B] = clip[7] + clip[6];
	FrustumValues[FRONT][C] = clip[11] + clip[10];
	FrustumValues[FRONT][D] = clip[15] + clip[14];
	NormalizePlane(FrustumValues, FRONT);
}

bool Frustum::SphereInFrustum(float X, float Y, float Z, float Radius)
{
	for (int i = 0; i < 6; i++)
	{
		if (FrustumValues[i][A] * X + FrustumValues[i][B] * Y + FrustumValues[i][C] * Z + FrustumValues[i][D] <= -Radius)
		{
			return false;
		}
	}

	return true;
}