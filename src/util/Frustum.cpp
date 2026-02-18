#include "Frustum.h"

#include <glm/gtc/type_ptr.hpp>

namespace application::util
{
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

	void Frustum::CalculateFrustum(application::gl::Camera& CameraView)
	{
		const float* proj = glm::value_ptr(CameraView.GetProjectionMatrix());
		const float* modl = glm::value_ptr(CameraView.GetViewMatrix());
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

		mFrustumValues[RIGHT][A] = clip[3] - clip[0];
		mFrustumValues[RIGHT][B] = clip[7] - clip[4];
		mFrustumValues[RIGHT][C] = clip[11] - clip[8];
		mFrustumValues[RIGHT][D] = clip[15] - clip[12];
		NormalizePlane(mFrustumValues, RIGHT);

		mFrustumValues[LEFT][A] = clip[3] + clip[0];
		mFrustumValues[LEFT][B] = clip[7] + clip[4];
		mFrustumValues[LEFT][C] = clip[11] + clip[8];
		mFrustumValues[LEFT][D] = clip[15] + clip[12];
		NormalizePlane(mFrustumValues, LEFT);

		mFrustumValues[BOTTOM][A] = clip[3] + clip[1];
		mFrustumValues[BOTTOM][B] = clip[7] + clip[5];
		mFrustumValues[BOTTOM][C] = clip[11] + clip[9];
		mFrustumValues[BOTTOM][D] = clip[15] + clip[13];
		NormalizePlane(mFrustumValues, BOTTOM);

		mFrustumValues[TOP][A] = clip[3] - clip[1];
		mFrustumValues[TOP][B] = clip[7] - clip[5];
		mFrustumValues[TOP][C] = clip[11] - clip[9];
		mFrustumValues[TOP][D] = clip[15] - clip[13];
		NormalizePlane(mFrustumValues, TOP);

		mFrustumValues[BACK][A] = clip[3] - clip[2];
		mFrustumValues[BACK][B] = clip[7] - clip[6];
		mFrustumValues[BACK][C] = clip[11] - clip[10];
		mFrustumValues[BACK][D] = clip[15] - clip[14];
		NormalizePlane(mFrustumValues, BACK);

		mFrustumValues[FRONT][A] = clip[3] + clip[2];
		mFrustumValues[FRONT][B] = clip[7] + clip[6];
		mFrustumValues[FRONT][C] = clip[11] + clip[10];
		mFrustumValues[FRONT][D] = clip[15] + clip[14];
		NormalizePlane(mFrustumValues, FRONT);
	}

	bool Frustum::SphereInFrustum(float X, float Y, float Z, float Radius)
	{
		for (int i = 0; i < 6; i++)
		{
			if (mFrustumValues[i][A] * X + mFrustumValues[i][B] * Y + mFrustumValues[i][C] * Z + mFrustumValues[i][D] <= -Radius)
			{
				return false;
			}
		}

		return true;
	}
}