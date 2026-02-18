#include "Math.h"

#include <glm/trigonometric.hpp>
#include <glm/common.hpp>
#include <glm/glm.hpp>
#include <iomanip>
#include <sstream>
#include <unordered_map>
#include <random>
#include <ctime>

namespace application::util
{
	bool Math::CompareFloatsWithTolerance(float a, float b, float Tolerance)
	{
		return std::fabs(a - b) <= Tolerance;
	}

	glm::vec3 Math::RadiansToDegrees(glm::vec3 Degress)
	{
		return glm::degrees(Degress);
	}

	
	uint16_t Math::ConvertIntToHalf(int i)
	{
		//
		// Our floating point number, f, is represented by the bit
		// pattern in integer i.  Disassemble that bit pattern into
		// the sign, s, the exponent, e, and the significand, m.
		// Shift s into the position where it will go in in the
		// resulting half number.
		// Adjust e, accounting for the different exponent bias
		// of float and half (127 versus 15).
		//

		int s = (i >> 16) & 0x00008000;
		int e = ((i >> 23) & 0x000000ff) - (127 - 15);
		int m = i & 0x007fffff;

		//
		// Now reassemble s, e and m into a half:
		//

		if (e <= 0)
		{
			if (e < -10)
			{
				//
				// E is less than -10.  The absolute value of f is
				// less than HALF_MIN (f may be a small normalized
				// float, a denormalized float or a zero).
				//
				// We convert f to a half zero with the same sign as f.
				//

				return s;
			}

			//
			// E is between -10 and 0.  F is a normalized float
			// whose magnitude is less than HALF_NRM_MIN.
			//
			// We convert f to a denormalized half.
			//

			//
			// Add an explicit leading 1 to the significand.
			// 

			m = m | 0x00800000;

			//
			// Round to m to the nearest (10+e)-bit value (with e between
			// -10 and 0); in case of a tie, round to the nearest even value.
			//
			// Rounding may cause the significand to overflow and make
			// our number normalized.  Because of the way a half's bits
			// are laid out, we don't have to treat this case separately;
			// the code below will handle it correctly.
			// 

			int t = 14 - e;
			int a = (1 << (t - 1)) - 1;
			int b = (m >> t) & 1;

			m = (m + a + b) >> t;

			//
			// Assemble the half from s, e (zero) and m.
			//

			return s | m;
		}
		else if (e == 0xff - (127 - 15))
		{
			if (m == 0)
			{
				//
				// F is an infinity; convert f to a half
				// infinity with the same sign as f.
				//

				return s | 0x7c00;
			}
			else
			{
				//
				// F is a NAN; we produce a half NAN that preserves
				// the sign bit and the 10 leftmost bits of the
				// significand of f, with one exception: If the 10
				// leftmost bits are all zero, the NAN would turn 
				// into an infinity, so we have to set at least one
				// bit in the significand.
				//

				m >>= 13;
				return s | 0x7c00 | m | (m == 0);
			}
		}
		else
		{
			//
			// E is greater than zero.  F is a normalized float.
			// We try to convert f to a normalized half.
			//

			//
			// Round to m to the nearest 10-bit value.  In case of
			// a tie, round to the nearest even value.
			//

			m = m + 0x00000fff + ((m >> 13) & 1);

			if (m & 0x00800000)
			{
				m = 0;		// overflow in significand,
				e += 1;		// adjust exponent
			}

			//
			// Handle exponent overflow
			//

			if (e > 30)
			{
				return s | 0x7c00;	// if this returns, the half becomes an
			}   			// infinity with the same sign as f.

			//
			// Assemble the half from s, e and m.
			//

			return s | (e << 10) | (m >> 13);
		}
	}

	/*
	std::pair<std::vector<glm::vec3>, std::vector<unsigned int>> Math::ClipMeshToBoxes(const std::pair<std::vector<glm::vec3>, std::vector<unsigned int>>& Model, std::vector<std::pair<glm::vec3, glm::vec3>> Boxes)
	{
		std::pair<std::vector<glm::vec3>, std::vector<unsigned int>> Result;
		std::unordered_map<uint32_t, uint32_t> OldToNewIndexMap;

		// Helper function to check if a point is inside the box
		auto IsInsideAnyBox = [&](const glm::vec3& Point) {
			for (auto& [Min, Max] : Boxes) {
				if (Point.x >= Min.x && Point.x <= Max.x &&
					Point.y >= Min.y && Point.y <= Max.y &&
					Point.z >= Min.z && Point.z <= Max.z) {
					return true;
				}
			}
			return false;
			};

		// Helper function to clamp a point to the nearest box
		auto ClampToNearestBox = [&](const glm::vec3& point) {
			if (Boxes.empty()) return point;

			float minDistance = std::numeric_limits<float>::max();
			glm::vec3 clampedPoint = point;

			for (auto& [Min, Max] : Boxes) {
				// Clamp to current box
				glm::vec3 currentClamped(
					glm::clamp(point.x, Min.x, Max.x),
					glm::clamp(point.y, Min.y, Max.y),
					glm::clamp(point.z, Min.z, Max.z)
				);

				// Calculate distance to original point
				float distance = glm::distance(point, currentClamped);

				// Update if this is the closest clamped point so far
				if (distance < minDistance) {
					minDistance = distance;
					clampedPoint = currentClamped;
				}
			}

			return clampedPoint;
			};


		// First pass: Process triangles
		for (size_t i = 0; i < Model.second.size(); i += 3) {
			glm::vec3 v1 = Model.first[Model.second[i]];
			glm::vec3 v2 = Model.first[Model.second[i + 1]];
			glm::vec3 v3 = Model.first[Model.second[i + 2]];

			// Check if any vertex is inside the box
			bool v1Inside = IsInsideAnyBox(v1);
			bool v2Inside = IsInsideAnyBox(v2);
			bool v3Inside = IsInsideAnyBox(v3);

			// If at least one vertex is inside, we'll keep the triangle (potentially modified)
			if (v1Inside || v2Inside || v3Inside) {
				// Clamp vertices to box boundaries if they're outside
				if (!v1Inside) v1 = ClampToNearestBox(v1);
				if (!v2Inside) v2 = ClampToNearestBox(v2);
				if (!v3Inside) v3 = ClampToNearestBox(v3);

				// Add vertices and update indices
				for (size_t j = 0; j < 3; j++) {
					uint32_t oldIndex = Model.second[i + j];
					if (OldToNewIndexMap.find(oldIndex) == OldToNewIndexMap.end()) {
						// Add new vertex
						OldToNewIndexMap[oldIndex] = static_cast<uint32_t>(Result.first.size());
						Result.first.push_back(j == 0 ? v1 : (j == 1 ? v2 : v3));
					}
					Result.second.push_back(OldToNewIndexMap[oldIndex]);
				}
			}
		}

		return Result;
	}

	std::pair<std::vector<glm::vec3>, std::vector<unsigned int>> Math::ClipMeshToOutsideOfBoxes(const std::pair<std::vector<glm::vec3>, std::vector<unsigned int>>& Model, std::vector<std::pair<glm::vec3, glm::vec3>> Boxes)
	{
		std::pair<std::vector<glm::vec3>, std::vector<unsigned int>> Result;
		std::unordered_map<uint32_t, uint32_t> OldToNewIndexMap;

		// Helper function to check if a point is inside the box
		auto IsInsideAnyBox = [&](const glm::vec3& Point) {
			for (auto& [Min, Max] : Boxes) {
				if (Point.x >= Min.x && Point.x <= Max.x &&
					Point.y >= Min.y && Point.y <= Max.y &&
					Point.z >= Min.z && Point.z <= Max.z) {
					return true;
				}
			}
			return false;
			};

		// Helper function to push a point outside of a box
		auto PushOutsideBox = [&](const glm::vec3& Point) {
			if (Boxes.empty()) return Point;

			// Find the box that contains this point
			for (auto& [Min, Max] : Boxes) {
				if (Point.x >= Min.x && Point.x <= Max.x &&
					Point.y >= Min.y && Point.y <= Max.y &&
					Point.z >= Min.z && Point.z <= Max.z) {

					// Find the closest face of the box to push the point to
					float distToMinX = std::abs(Point.x - Min.x);
					float distToMaxX = std::abs(Point.x - Max.x);
					float distToMinY = std::abs(Point.y - Min.y);
					float distToMaxY = std::abs(Point.y - Max.y);
					float distToMinZ = std::abs(Point.z - Min.z);
					float distToMaxZ = std::abs(Point.z - Max.z);

					// Find minimum distance
					float minDist = std::min({ distToMinX, distToMaxX, distToMinY,
											distToMaxY, distToMinZ, distToMaxZ });

					glm::vec3 PushedPoint = Point;

					// Push to closest face
					if (minDist == distToMinX) PushedPoint.x = Min.x;
					else if (minDist == distToMaxX) PushedPoint.x = Max.x;
					else if (minDist == distToMinY) PushedPoint.y = Min.y;
					else if (minDist == distToMaxY) PushedPoint.y = Max.y;
					else if (minDist == distToMinZ) PushedPoint.z = Min.z;
					else if (minDist == distToMaxZ) PushedPoint.z = Max.z;

					return PushedPoint;
				}
			}
			return Point;  // Return original point if not inside any box
			};

		// Process triangles
		for (size_t i = 0; i < Model.second.size(); i += 3) {
			glm::vec3 v1 = Model.first[Model.second[i]];
			glm::vec3 v2 = Model.first[Model.second[i + 1]];
			glm::vec3 v3 = Model.first[Model.second[i + 2]];

			// Check if vertices are inside any box
			bool v1Inside = IsInsideAnyBox(v1);
			bool v2Inside = IsInsideAnyBox(v2);
			bool v3Inside = IsInsideAnyBox(v3);

			// Keep triangle if at least one vertex is outside all boxes
			if (!v1Inside || !v2Inside || !v3Inside) {
				// Push vertices outside if they're inside any box
				if (v1Inside) v1 = PushOutsideBox(v1);
				if (v2Inside) v2 = PushOutsideBox(v2);
				if (v3Inside) v3 = PushOutsideBox(v3);

				// Add vertices and update indices
				for (size_t j = 0; j < 3; j++) {
					uint32_t oldIndex = Model.second[i + j];
					if (OldToNewIndexMap.find(oldIndex) == OldToNewIndexMap.end()) {
						// Add new vertex
						OldToNewIndexMap[oldIndex] = static_cast<uint32_t>(Result.first.size());
						Result.first.push_back(j == 0 ? v1 : (j == 1 ? v2 : v3));
					}
					Result.second.push_back(OldToNewIndexMap[oldIndex]);
				}
			}
		}

		return Result;
	}
	*/

	// Helper for hashing edges to reuse intersection vertices
	struct Edge {
		uint32_t v1, v2;
		bool operator==(const Edge& other) const {
			return (v1 == other.v1 && v2 == other.v2) || (v1 == other.v2 && v2 == other.v1);
		}
	};

	struct EdgeHash {
		size_t operator()(const Edge& e) const {
			return std::hash<uint32_t>{}(e.v1) ^ std::hash<uint32_t>{}(e.v2);
		}
	};

	std::pair<std::vector<glm::vec3>, std::vector<unsigned int>> Math::SplitMeshByPlane(
		const std::pair<std::vector<glm::vec3>, std::vector<unsigned int>>& Model,
		glm::vec3 PlaneNormal,
		float PlaneDistance,
		bool WantSecond)
	{
		std::pair<std::vector<glm::vec3>, std::vector<unsigned int>> Result;

		// Maps original vertex index to new mesh index
		std::unordered_map<uint32_t, uint32_t> OriginalToNewMap;
		// Maps an edge (v1, v2) to the index of the intersection vertex created on it
		std::unordered_map<Edge, uint32_t, EdgeHash> EdgeToIntersectionMap;

		const float epsilon = 1e-6f;

		auto IsInFront = [&](const glm::vec3& Point) {
			return glm::dot(Point, PlaneNormal) - PlaneDistance >= -epsilon;
			};

		auto GetOrAddOriginalVertex = [&](uint32_t oldIdx) {
			auto it = OriginalToNewMap.find(oldIdx);
			if (it != OriginalToNewMap.end()) return it->second;

			uint32_t newIdx = (uint32_t)Result.first.size();
			Result.first.push_back(Model.first[oldIdx]);
			OriginalToNewMap[oldIdx] = newIdx;
			return newIdx;
			};

		auto GetOrAddIntersection = [&](uint32_t idxA, uint32_t idxB) {
			Edge edge = { idxA, idxB };
			auto it = EdgeToIntersectionMap.find(edge);
			if (it != EdgeToIntersectionMap.end()) return it->second;

			const glm::vec3& p1 = Model.first[idxA];
			const glm::vec3& p2 = Model.first[idxB];
			glm::vec3 dir = p2 - p1;
			float denom = glm::dot(dir, PlaneNormal);

			float t = (std::abs(denom) < 1e-8f) ? 0.0f : (PlaneDistance - glm::dot(p1, PlaneNormal)) / denom;
			t = glm::clamp(t, 0.0f, 1.0f);

			uint32_t newIdx = (uint32_t)Result.first.size();
			Result.first.push_back(p1 + dir * t);
			EdgeToIntersectionMap[edge] = newIdx;
			return newIdx;
			};

		for (size_t i = 0; i < Model.second.size(); i += 3) {
			uint32_t indices[3] = { Model.second[i], Model.second[i + 1], Model.second[i + 2] };
			bool isFront[3] = { IsInFront(Model.first[indices[0]]),
								IsInFront(Model.first[indices[1]]),
								IsInFront(Model.first[indices[2]]) };

			int frontCount = isFront[0] + isFront[1] + isFront[2];
			bool wantFront = !WantSecond;

			// 1. Entirely on the side we want
			if ((frontCount == 3 && wantFront) || (frontCount == 0 && !wantFront)) {
				Result.second.push_back(GetOrAddOriginalVertex(indices[0]));
				Result.second.push_back(GetOrAddOriginalVertex(indices[1]));
				Result.second.push_back(GetOrAddOriginalVertex(indices[2]));
			}
			// 2. Triangle is split
			else if (frontCount == 1 || frontCount == 2) {
				// Identify the lone vertex index (0, 1, or 2)
				int lone = 0;
				if (frontCount == 1) {
					while (!isFront[lone]) lone++; // Lone vertex is the only one in Front
				}
				else {
					while (isFront[lone]) lone++;  // Lone vertex is the only one in Back
				}

				uint32_t i1 = indices[lone];
				uint32_t i2 = indices[(lone + 1) % 3];
				uint32_t i3 = indices[(lone + 2) % 3];

				uint32_t inter12 = GetOrAddIntersection(i1, i2);
				uint32_t inter13 = GetOrAddIntersection(i1, i3);

				bool loneIsWhatWeWant = (frontCount == 1 && wantFront) || (frontCount == 2 && !wantFront);

				if (loneIsWhatWeWant) {
					// Keep the single triangle containing the lone vertex
					Result.second.push_back(GetOrAddOriginalVertex(i1));
					Result.second.push_back(inter12);
					Result.second.push_back(inter13);
				}
				else {
					// Keep the remaining quadrilateral (as two triangles)
					uint32_t ni2 = GetOrAddOriginalVertex(i2);
					uint32_t ni3 = GetOrAddOriginalVertex(i3);

					Result.second.push_back(inter12);
					Result.second.push_back(ni2);
					Result.second.push_back(ni3);

					Result.second.push_back(inter12);
					Result.second.push_back(ni3);
					Result.second.push_back(inter13);
				}
			}
		}

		return Result;
	}

	std::pair<std::vector<glm::vec3>, std::vector<unsigned int>> Math::ClipMeshToBox(const std::pair<std::vector<glm::vec3>, std::vector<unsigned int>>& Model, std::pair<glm::vec3, glm::vec3> Box)
	{
		std::pair<std::vector<glm::vec3>, std::vector<unsigned int>> Result;

		Result = SplitMeshByPlane(Model, glm::normalize(glm::vec3(1, 0, 0)), Box.second.x, true); //X Max
		Result = SplitMeshByPlane(Result, glm::normalize(glm::vec3(1, 0, 0)), Box.first.x, false); //X Min

		Result = SplitMeshByPlane(Result, glm::normalize(glm::vec3(0, 1, 0)), Box.second.y, true); //Y Max
		Result = SplitMeshByPlane(Result, glm::normalize(glm::vec3(0, 1, 0)), Box.first.y, false); //Y Min

		Result = SplitMeshByPlane(Result, glm::normalize(glm::vec3(0, 0, 1)), Box.second.z, true); //Z Max
		Result = SplitMeshByPlane(Result, glm::normalize(glm::vec3(0, 0, 1)), Box.first.z, false); //Z Min

		return Result;
	}

	std::pair<std::vector<glm::vec3>, std::vector<unsigned int>> Math::ClipOutsideMeshToBox(const std::pair<std::vector<glm::vec3>, std::vector<unsigned int>>& Model, std::pair<glm::vec3, glm::vec3> Box)
	{
		std::pair<std::vector<glm::vec3>, std::vector<unsigned int>> Result;

		std::pair<std::vector<glm::vec3>, std::vector<unsigned int>> SubResults[6];

		SubResults[0] = SplitMeshByPlane(Model, glm::normalize(glm::vec3(1, 0, 0)), Box.second.x, false); //X Max
		SubResults[1] = SplitMeshByPlane(Model, glm::normalize(glm::vec3(1, 0, 0)), Box.first.x, true); //X Min

		SubResults[2] = SplitMeshByPlane(Model, glm::normalize(glm::vec3(0, 1, 0)), Box.second.y, false); //Y Max
		SubResults[3] = SplitMeshByPlane(Model, glm::normalize(glm::vec3(0, 1, 0)), Box.first.y, true); //Y Min

		SubResults[4] = SplitMeshByPlane(Model, glm::normalize(glm::vec3(0, 0, 1)), Box.second.z, false); //Z Max
		SubResults[5] = SplitMeshByPlane(Model, glm::normalize(glm::vec3(0, 0, 1)), Box.first.z, true); //Z Min

		auto WriteVertex = [](std::vector<glm::vec3>& VertexBuffer, const glm::vec3& Vertex)
			{
				for (size_t i = 0; i < VertexBuffer.size(); i++)
				{
					if (VertexBuffer[i] == Vertex)
					{
						return i;
					}
				}

				VertexBuffer.push_back(Vertex);
				return VertexBuffer.size() - 1;
			};

		for (size_t i = 0; i < 6; i++)
		{
			//Merging mesh
			Result.second.reserve(Result.second.size() + SubResults[i].second.size() * 3);
			Result.first.reserve(Result.first.size() + SubResults[i].first.size());
			{
				for (size_t j = 0; j < SubResults[i].second.size(); j += 3)
				{
					const glm::vec3& VertexA = SubResults[i].first[SubResults[i].second[j]];
					const glm::vec3& VertexB = SubResults[i].first[SubResults[i].second[j + 1]];
					const glm::vec3& VertexC = SubResults[i].first[SubResults[i].second[j + 2]];

					Result.second.push_back(WriteVertex(Result.first, VertexA));
					Result.second.push_back(WriteVertex(Result.first, VertexB));
					Result.second.push_back(WriteVertex(Result.first, VertexC));
				}
			}
		}

		Result.first.shrink_to_fit();
		Result.second.shrink_to_fit();

		return Result;
	}

	std::pair<std::vector<glm::vec3>, std::vector<unsigned int>> Math::ClipMeshToBoxes(const std::pair<std::vector<glm::vec3>, std::vector<unsigned int>>& Model, std::vector<std::pair<glm::vec3, glm::vec3>> Boxes)
	{
		if(Boxes.empty())
			return std::make_pair(std::vector<glm::vec3>(), std::vector<unsigned int>());

		std::pair<std::vector<glm::vec3>, std::vector<unsigned int>> Result = ClipMeshToBox(Model, Boxes[0]);

		std::unordered_map<std::string, uint32_t> VertexMap;
		auto CreateVertexKey = [](const glm::vec3& v) {
			char buffer[128];
			snprintf(buffer, sizeof(buffer), "%i,%i,%i", (int32_t)(v.x * 10000.0f), (int32_t)(v.y * 10000.0f), (int32_t)(v.z * 10000.0f));
			return std::string(buffer);
			};

		auto WriteVertex = [&CreateVertexKey, &VertexMap](std::vector<glm::vec3>& VertexBuffer, const glm::vec3& Vertex)
			{
				std::string Key = CreateVertexKey(Vertex);
				auto Iter = VertexMap.find(Key);
				if (Iter != VertexMap.end())
					return Iter->second;

				VertexBuffer.push_back(Vertex);
				VertexMap.insert({ Key, VertexBuffer.size() - 1 });
				return (uint32_t)(VertexBuffer.size() - 1);
			};

		for (size_t i = 1; i < Boxes.size(); i++)
		{
			std::pair<std::vector<glm::vec3>, std::vector<unsigned int>> SubMesh = ClipMeshToBox(Model, Boxes[i]);
			
			Result.second.reserve(Result.second.size() + SubMesh.second.size() * 3);
			Result.first.reserve(Result.first.size() + SubMesh.first.size());

			//Merging mesh
			{
				for (size_t j = 0; j < SubMesh.second.size(); j += 3)
				{
					const glm::vec3& VertexA = SubMesh.first[SubMesh.second[j]];
					const glm::vec3& VertexB = SubMesh.first[SubMesh.second[j + 1]];
					const glm::vec3& VertexC = SubMesh.first[SubMesh.second[j + 2]];

					Result.second.push_back(WriteVertex(Result.first, VertexA));
					Result.second.push_back(WriteVertex(Result.first, VertexB));
					Result.second.push_back(WriteVertex(Result.first, VertexC));
				}
			}
		}

		Result.first.shrink_to_fit();
		Result.second.shrink_to_fit();

		return Result;
	}

	std::pair<std::vector<glm::vec3>, std::vector<unsigned int>> Math::ClipOutsideMeshToBoxes(const std::pair<std::vector<glm::vec3>, std::vector<unsigned int>>& Model, std::vector<std::pair<glm::vec3, glm::vec3>> Boxes)
	{
		if (Boxes.empty())
			return Model;

		std::pair<std::vector<glm::vec3>, std::vector<unsigned int>> Result = ClipOutsideMeshToBox(Model, Boxes[0]);

		std::unordered_map<std::string, uint32_t> VertexMap;
		auto CreateVertexKey = [](const glm::vec3& v) {
			char buffer[128];
			snprintf(buffer, sizeof(buffer), "%i,%i,%i", (int32_t)(v.x * 10000.0f), (int32_t)(v.y * 10000.0f), (int32_t)(v.z * 10000.0f));
			return std::string(buffer);
		};

		auto WriteVertex = [&CreateVertexKey, &VertexMap](std::vector<glm::vec3>& VertexBuffer, const glm::vec3& Vertex)
			{
				std::string Key = CreateVertexKey(Vertex);
				auto Iter = VertexMap.find(Key);
				if (Iter != VertexMap.end())
					return Iter->second;

				VertexBuffer.push_back(Vertex);
				VertexMap.insert({ Key, VertexBuffer.size() - 1});
				return (uint32_t)(VertexBuffer.size() - 1);
			};

		for (size_t i = 1; i < Boxes.size(); i++)
		{
			std::pair<std::vector<glm::vec3>, std::vector<unsigned int>> SubMesh = ClipOutsideMeshToBox(Model, Boxes[i]);

			//Merging mesh
			{
				for (size_t j = 0; j < SubMesh.second.size(); j += 3)
				{
					const glm::vec3& VertexA = SubMesh.first[SubMesh.second[j]];
					const glm::vec3& VertexB = SubMesh.first[SubMesh.second[j + 1]];
					const glm::vec3& VertexC = SubMesh.first[SubMesh.second[j + 2]];

					Result.second.push_back(WriteVertex(Result.first, VertexA));
					Result.second.push_back(WriteVertex(Result.first, VertexB));
					Result.second.push_back(WriteVertex(Result.first, VertexC));
				}
			}
		}

		return Result;
	}

	int Math::NumberOfSetBits(uint32_t i)
	{
		// Java: use int, and use >>> instead of >>. Or use Integer.bitCount()
		// C or C++: use uint32_t
		i = i - ((i >> 1) & 0x55555555);        // add pairs of bits
		i = (i & 0x33333333) + ((i >> 2) & 0x33333333);  // quads
		i = (i + (i >> 4)) & 0x0F0F0F0F;        // groups of 8
		i *= 0x01010101;                        // horizontal sum of bytes
		return i >> 24;               // return just that top byte (after truncating to 32-bit even when int is wider than uint32_t)
	}

	int Math::CountLeadingZeros(uint32_t x)
	{
		// Fill all the bits to the right of set bits
		x |= (x >> 1);
		x |= (x >> 2);
		x |= (x >> 4);
		x |= (x >> 8);
		x |= (x >> 16);

		// Do a bit population count (number of ones)
		return 32 - NumberOfSetBits(x);
	}

	uint64_t Math::StringHashMurMur3(const std::string& str) 
	{
		static constexpr uint64_t MULTIPLY = 0xc6a4a7935bd1e995ULL;
		static constexpr int R = 47;

		uint64_t h = 0x1234567812345678ULL; // Seed

		const uint64_t* data = reinterpret_cast<const uint64_t*>(str.data());
		const uint64_t* end = data + (str.size() / 8);

		while (data != end) 
		{
			uint64_t k = *data++;

			k *= MULTIPLY;
			k ^= k >> R;
			k *= MULTIPLY;

			h ^= k;
			h *= MULTIPLY;
		}

		const unsigned char* data2 = reinterpret_cast<const unsigned char*>(data);

		switch (str.size() & 7) 
		{
		case 7: h ^= uint64_t(data2[6]) << 48;
		case 6: h ^= uint64_t(data2[5]) << 40;
		case 5: h ^= uint64_t(data2[4]) << 32;
		case 4: h ^= uint64_t(data2[3]) << 24;
		case 3: h ^= uint64_t(data2[2]) << 16;
		case 2: h ^= uint64_t(data2[1]) << 8;
		case 1: h ^= uint64_t(data2[0]);
			h *= MULTIPLY;
		}

		h ^= h >> R;
		h *= MULTIPLY;
		h ^= h >> R;

		return h;
	}

	uint32_t Math::StringHashMurMur3_32(const std::string& str)
	{
		static constexpr uint32_t c1 = 0xcc9e2d51;
		static constexpr uint32_t c2 = 0x1b873593;
		static constexpr uint32_t seed = 0x12345678;

		uint32_t h = seed;
		const uint8_t* data = reinterpret_cast<const uint8_t*>(str.data());
		const size_t nblocks = str.size() / 4;

		// Body - process 4-byte blocks
		const uint32_t* blocks = reinterpret_cast<const uint32_t*>(data);
		for (size_t i = 0; i < nblocks; i++)
		{
			uint32_t k = blocks[i];

			k *= c1;
			k = (k << 15) | (k >> 17); // rotl32(k, 15)
			k *= c2;

			h ^= k;
			h = (h << 13) | (h >> 19); // rotl32(h, 13)
			h = h * 5 + 0xe6546b64;
		}

		// Tail - process remaining bytes
		const uint8_t* tail = data + nblocks * 4;
		uint32_t k1 = 0;

		switch (str.size() & 3)
		{
		case 3: k1 ^= tail[2] << 16; [[fallthrough]];
		case 2: k1 ^= tail[1] << 8; [[fallthrough]];
		case 1: k1 ^= tail[0];
			k1 *= c1;
			k1 = (k1 << 15) | (k1 >> 17);
			k1 *= c2;
			h ^= k1;
		}

		// Finalization
		h ^= str.size();
		h ^= h >> 16;
		h *= 0x85ebca6b;
		h ^= h >> 13;
		h *= 0xc2b2ae35;
		h ^= h >> 16;

		return h;
	}


	std::string Math::GenerateRandomString()
	{
		const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<> dis(0, chars.size() - 1);

		std::string result;
		result.reserve(16);

		for (int i = 0; i < 16; ++i) 
		{
			result += chars[dis(gen)];
		}

		return result;
	}

	//Thanks ChatGPT <3
	std::string Math::GenerateLicenseKey(const std::string& seed) 
	{
		if (seed.length() != 16) 
		{
			throw std::invalid_argument("Seed must be exactly 16 characters");
		}

		// License character set (excludes confusing characters like 0, O, 1, I)
		const std::string LICENSE_CHARS = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";

		// Simple hash function for mixing
		auto hash32 = [](uint32_t x) -> uint32_t {
			x ^= x >> 16;
			x *= 0x85ebca6b;
			x ^= x >> 13;
			x *= 0xc2b2ae35;
			x ^= x >> 16;
			return x;
			};

		// Convert character to numeric value
		auto charToValue = [](char c) -> uint8_t {
			if (c >= 'A' && c <= 'Z') return c - 'A';
			if (c >= 'a' && c <= 'z') return (c - 'a') + 26;
			if (c >= '0' && c <= '9') return (c - '0') + 52;
			return 0;
			};

		std::string licenseKey;
		licenseKey.reserve(16);

		// Convert seed to numeric values
		uint8_t seedValues[16];
		for (int i = 0; i < 16; ++i) {
			seedValues[i] = charToValue(seed[i]);
		}

		// Transform each position using interdependent algorithm
		for (int i = 0; i < 16; ++i) {
			uint32_t accumulator = 0;

			// Mix current character
			accumulator += seedValues[i] * 17;

			// Add influence from other positions
			accumulator += seedValues[(i + 3) % 16] * 7;
			accumulator += seedValues[(i + 7) % 16] * 13;
			accumulator += seedValues[(i + 11) % 16] * 19;

			// Add position-dependent salt
			accumulator += i * 23;

			// Apply hash mixing
			accumulator = hash32(accumulator);

			// Convert to license character
			licenseKey += LICENSE_CHARS[accumulator % LICENSE_CHARS.length()];
		}

		return licenseKey;
	}

	float Math::CalculateScreenRadius(
		const glm::vec3& SphereCenter,
		float SphereRadius,
		const glm::vec3& CameraPos,
		float FovY,
		float ScreenHeight)
	{
		float Distance = glm::length(SphereCenter - CameraPos);
		if (Distance <= 0.0f) 
		{
			return std::numeric_limits<float>::max(); // Avoid division by zero.
		}

		// Convert FOV from degrees to radians.
		float FovYRad = glm::radians(FovY);

		// Calculate focal length in pixel units.
		float FocalLength = (ScreenHeight * 0.5f) / tan(FovYRad * 0.5f);

		// Compute projected radius in pixels.
		float ScreenRadius = (SphereRadius / Distance) * FocalLength;

		return ScreenRadius;
	}

	std::pair<std::vector<float>, std::vector<uint32_t>> Math::GenerateSphere(
		float Radius,
		unsigned int LatitudeBands,
		unsigned int LongitudeBands
	) {
		std::vector<float> Vertices;
		std::vector<uint32_t> Indices;

		// Generate vertices
		for (unsigned int lat = 0; lat <= LatitudeBands; lat++) {
			float theta = lat * M_PI / LatitudeBands;
			float sinTheta = std::sin(theta);
			float cosTheta = std::cos(theta);

			for (unsigned int lon = 0; lon <= LongitudeBands; lon++) {
				float phi = lon * 2 * M_PI / LongitudeBands;
				float sinPhi = std::sin(phi);
				float cosPhi = std::cos(phi);

				// Calculate vertex position
				float x = cosPhi * sinTheta;
				float y = cosTheta;
				float z = sinPhi * sinTheta;

				// Push vertex
				Vertices.push_back(x * Radius);
				Vertices.push_back(y * Radius);
				Vertices.push_back(z * Radius);
			}
		}

		// Generate indices
		for (unsigned int lat = 0; lat < LatitudeBands; lat++) {
			for (unsigned int lon = 0; lon < LongitudeBands; lon++) {
				unsigned int first = lat * (LongitudeBands + 1) + lon;
				unsigned int second = first + LongitudeBands + 1;

				// First triangle
				Indices.push_back(first);
				Indices.push_back(second);
				Indices.push_back(first + 1);

				// Second triangle
				Indices.push_back(second);
				Indices.push_back(second + 1);
				Indices.push_back(first + 1);
			}
		}

		return std::make_pair(Vertices, Indices);
	}

	glm::vec3 Math::GetRayFromMouse(float MouseX, float MouseY, const glm::mat4& ProjectionMatrix, const glm::mat4& ViewMatrix, int ScreenWidth, int ScreenHeight)
	{
		// Convert mouse coordinates to normalized device coordinates
		float x = (2.0f * MouseX) / ScreenWidth - 1.0f;
		float y = 1.0f - (2.0f * MouseY) / ScreenHeight;
		glm::vec4 RayClip(x, y, -1.0f, 1.0f);

		// Convert to eye space
		glm::vec4 RayEye = glm::inverse(ProjectionMatrix) * RayClip;
		RayEye = glm::vec4(RayEye.x, RayEye.y, -1.0f, 0.0f);

		// Convert to world space
		glm::vec3 RayWorld = glm::vec3(glm::inverse(ViewMatrix) * RayEye);
		return glm::normalize(RayWorld);
	}

	bool Math::RayTriangleIntersect(const glm::vec3& RayOrigin, const glm::vec3& RayDir, glm::vec3 VertexA, glm::vec3 VertexB, glm::vec3 VertexC, float& t)
	{
		const float EPSILON = 0.0000001f;
		glm::vec3 Edge1 = VertexB - VertexA;
		glm::vec3 Edge2 = VertexC - VertexA;
		glm::vec3 h = glm::cross(RayDir, Edge2);
		float a = glm::dot(Edge1, h);

		if (a > -EPSILON && a < EPSILON) return false;

		float f = 1.0f / a;
		glm::vec3 s = RayOrigin - VertexA;
		float u = f * glm::dot(s, h);

		if (u < 0.0f || u > 1.0f) return false;

		glm::vec3 q = glm::cross(s, Edge1);
		float v = f * glm::dot(RayDir, q);

		if (v < 0.0f || u + v > 1.0f) return false;

		t = f * glm::dot(Edge2, q);
		return t > EPSILON;
	}

	glm::vec3 Math::FindTerrainIntersectionModelData(const glm::vec3& RayOrigin, const glm::vec3& RayDir, const std::vector<glm::vec3>& Vertices, const std::vector<uint32_t>& Indices)
	{
		float ClosestT = std::numeric_limits<float>::max();
		glm::vec3 Intersection;
		bool Found = false;

		for (size_t i = 0; i < Indices.size() / 3; i++)
		{
			const glm::vec3& VertexA = Vertices[Indices[i * 3]];
			const glm::vec3& VertexB = Vertices[Indices[i * 3 + 1]];
			const glm::vec3& VertexC = Vertices[Indices[i * 3 + 2]];

			float t = 0.0f;
			if (RayTriangleIntersect(RayOrigin, RayDir, VertexA, VertexB, VertexC, t))
			{
				if (t < ClosestT)
				{
					ClosestT = t;
					Intersection = RayOrigin + RayDir * t;
					Found = true;
				}
			}
		}

		return Found ? Intersection : glm::vec3(0.0f);
	}

	std::pair<glm::vec3, glm::vec3> Math::FindTerrainIntersectionAndNormalVecModelData(const glm::vec3& RayOrigin, const glm::vec3& RayDir, const std::vector<glm::vec3>& Vertices, const std::vector<uint32_t>& Indices)
	{
		float ClosestT = std::numeric_limits<float>::max();
		glm::vec3 Intersection;
		glm::vec3 Normal;
		bool Found = false;

		for (size_t i = 0; i < Indices.size() / 3; i++)
		{
			float t = 0.0f;
			const glm::vec3& VertexA = Vertices[Indices[i * 3]];
			const glm::vec3& VertexB = Vertices[Indices[i * 3 + 1]];
			const glm::vec3& VertexC = Vertices[Indices[i * 3 + 2]];
			if (RayTriangleIntersect(RayOrigin, RayDir, VertexA, VertexB, VertexC, t))
			{
				if (t < ClosestT)
				{
					ClosestT = t;
					Intersection = RayOrigin + RayDir * t;
					Normal = glm::cross(VertexB - VertexA, VertexC - VertexA);
					Found = true;
				}
			}
		}

		return Found ? std::make_pair(Intersection, glm::normalize(Normal)) : std::make_pair(glm::vec3(0.0f), glm::vec3(0.0f));
	}

	std::string Math::GenerateRandomHexString(int Length) 
	{
		// Characters to use (0-9, A-F)
		const std::string hex_chars = "0123456789ABCDEF";

		// Initialize random number generator
		std::random_device rd;
		std::mt19937 generator(rd());
		std::uniform_int_distribution<int> distribution(0, hex_chars.size() - 1);

		// Generate the random string
		std::string result;
		result.reserve(Length);

		for (int i = 0; i < Length; ++i)
		{
			result += hex_chars[distribution(generator)];
		}

		return result;
	}
}