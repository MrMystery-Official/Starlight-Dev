#pragma once

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <string>
#include <ctime>
#include <utility>
#include <vector>

#ifndef MATH_MIN
#define MATH_MIN(a,b) ((a)<(b)?(a):(b))
#endif

#ifndef MATH_MAX
#define MATH_MAX(a,b) ((a)>(b)?(a):(b))
#endif

namespace application::util
{
	namespace Math
	{
		bool CompareFloatsWithTolerance(float a, float b, float Tolerance);
		glm::vec3 RadiansToDegrees(glm::vec3 Degress);
		uint16_t ConvertIntToHalf(int i);
		//std::pair<std::vector<glm::vec3>, std::vector<unsigned int>> ClipMeshToBoxes(const std::pair<std::vector<glm::vec3>, std::vector<unsigned int>>& Model, std::vector<std::pair<glm::vec3, glm::vec3>> Boxes);
		//std::pair<std::vector<glm::vec3>, std::vector<unsigned int>> ClipMeshToOutsideOfBoxes(const std::pair<std::vector<glm::vec3>, std::vector<unsigned int>>& Model, std::vector<std::pair<glm::vec3, glm::vec3>> Boxes);
		
		std::pair<std::vector<glm::vec3>, std::vector<unsigned int>> SplitMeshByPlane(const std::pair<std::vector<glm::vec3>, std::vector<unsigned int>>& Model, glm::vec3 PlaneNormal, float PlaneDistance, bool WantSecond);
		std::pair<std::vector<glm::vec3>, std::vector<unsigned int>> ClipMeshToBox(const std::pair<std::vector<glm::vec3>, std::vector<unsigned int>>& Model, std::pair<glm::vec3, glm::vec3> Box);
		std::pair<std::vector<glm::vec3>, std::vector<unsigned int>> ClipOutsideMeshToBox(const std::pair<std::vector<glm::vec3>, std::vector<unsigned int>>& Model, std::pair<glm::vec3, glm::vec3> Box);
		std::pair<std::vector<glm::vec3>, std::vector<unsigned int>> ClipMeshToBoxes(const std::pair<std::vector<glm::vec3>, std::vector<unsigned int>>& Model, std::vector<std::pair<glm::vec3, glm::vec3>> Boxes);
		std::pair<std::vector<glm::vec3>, std::vector<unsigned int>> ClipOutsideMeshToBoxes(const std::pair<std::vector<glm::vec3>, std::vector<unsigned int>>& Model, std::vector<std::pair<glm::vec3, glm::vec3>> Boxes);

		int NumberOfSetBits(uint32_t i);
		int CountLeadingZeros(uint32_t x);
		uint64_t StringHashMurMur3(const std::string& str);
		uint32_t StringHashMurMur3_32(const std::string& str);
		std::string GenerateRandomString();
		std::string GenerateLicenseKey(const std::string& seed);

		float CalculateScreenRadius(const glm::vec3& SphereCenter, float SphereRadius, const glm::vec3& CameraPos, float FovY, float ScreenHeight);

		std::pair<std::vector<float>, std::vector<uint32_t>> GenerateSphere(
			float Radius,
			unsigned int LatitudeBands,
			unsigned int LongitudeBands
		);

		glm::vec3 GetRayFromMouse(float MouseX, float MouseY, const glm::mat4& ProjectionMatrix, const glm::mat4& ViewMatrix, int ScreenWidth, int ScreenHeight);
		bool RayTriangleIntersect(const glm::vec3& RayOrigin, const glm::vec3& RayDir, glm::vec3 VertexA, glm::vec3 VertexB, glm::vec3 VertexC, float& t);
		glm::vec3 FindTerrainIntersectionModelData(const glm::vec3& RayOrigin, const glm::vec3& RayDir, const std::vector<glm::vec3>& Vertices, const std::vector<uint32_t>& Indices);
		std::pair<glm::vec3, glm::vec3> FindTerrainIntersectionAndNormalVecModelData(const glm::vec3& RayOrigin, const glm::vec3& RayDir, const std::vector<glm::vec3>& Vertices, const std::vector<uint32_t>& Indices);
		std::string GenerateRandomHexString(int Length);
	}
}