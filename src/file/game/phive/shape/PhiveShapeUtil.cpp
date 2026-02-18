#include "PhiveShapeUtil.h"

#include <limits>
#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

/*

THIS IS AN OUTDATED IMPLEMENTATION

*/

namespace application::file::game::phive::shape
{
	namespace PhiveShapeUtil
	{
        /*
            GeometrySectionConversionUtil
        */

		glm::vec3 GeometrySectionConversionUtil::PrepareDomain(glm::vec3& Min, glm::vec3 Max)
		{
			//Make sure that the bounding box is not flat
			constexpr float Epsilon = std::numeric_limits<float>::epsilon();
			for (int i = 0; i < 3; i++)
			{
				if (Min[i] == Max[i])
				{
					Min[i] -= Epsilon;
					Max[i] += Epsilon;
				}
			}

			//Make sure that compression works out
			glm::vec3 MinAbsolute = Min / glm::vec3(std::numeric_limits<uint8_t>::max());
			MinAbsolute = glm::abs(MinAbsolute);

			glm::vec3 Extents = (Max - Min) / glm::vec3(std::numeric_limits<uint8_t>::max());
			Extents = glm::max(Extents, MinAbsolute);

			Min -= Extents;
			Max += Extents;

			return (Max - Min);
		}

		glm::i32vec3 GeometrySectionConversionUtil::GetSectionOffset(const application::file::game::phive::classes::HavokClasses::hknpMeshShapeVertexConversionUtil& VertexConversionUtil)
		{
			glm::vec3 SectionOffsetPrecise = glm::vec3(mOriginalMin.x * VertexConversionUtil.mBitScale16.mParent.mData[0],
				mOriginalMin.y * VertexConversionUtil.mBitScale16.mParent.mData[1],
				mOriginalMin.z * VertexConversionUtil.mBitScale16.mParent.mData[2]);

			glm::i32vec3 Result = glm::round(SectionOffsetPrecise);
			return Result;
		}

		glm::i16vec3 GeometrySectionConversionUtil::CompressVertex(const application::file::game::phive::classes::HavokClasses::hknpMeshShapeVertexConversionUtil& VertexConversionUtil, const glm::i32vec3& SectionOffset, const glm::vec3& Vertex)
		{
			glm::vec3 BitScale16(VertexConversionUtil.mBitScale16.mParent.mData[0], VertexConversionUtil.mBitScale16.mParent.mData[1], VertexConversionUtil.mBitScale16.mParent.mData[2]);
			glm::vec3 ScaledVertex = BitScale16 * Vertex;
			ScaledVertex = glm::round(ScaledVertex);

			glm::i32vec3 IntScaledVertex = ScaledVertex;
			IntScaledVertex -= SectionOffset;

			glm::i16vec3 Result = glm::clamp(IntScaledVertex, glm::i32vec3(std::numeric_limits<uint16_t>::min()), glm::i32vec3(std::numeric_limits<uint16_t>::max()));
			return Result;
		}

		GeometrySectionConversionUtil::GeometrySectionConversionUtil(const glm::vec3& Min, const glm::vec3& Max)
		{
			mOriginalMin = Min;
			mOriginalMax = Max;

			glm::vec3 NewMin = Min;
			glm::vec3 NewMax = Max;

			glm::vec3 Span = PrepareDomain(NewMin, NewMax);
			mBitScale = glm::vec3(std::numeric_limits<uint8_t>::max()) / Span;

			glm::vec3 RangeMin = NewMin * mBitScale;
			mBitOffset = glm::round(RangeMin);

			mBitScaleInv = glm::vec3(1.0f) / mBitScale;
			mBitOffset16 = glm::clamp(mBitOffset, glm::i32vec3(std::numeric_limits<int16_t>::min()), glm::i32vec3(std::numeric_limits<int16_t>::max()));
		}



        /*
            TriangleToQuadConversionUtil
        */

        uint32_t TriangleToQuadConversionUtil::GetThirdVertex(const std::tuple<uint32_t, uint32_t, uint32_t>& Triangle, const Edge& SharedEdge)
        {
            uint32_t a = std::get<0>(Triangle);
            uint32_t b = std::get<1>(Triangle);
            uint32_t c = std::get<2>(Triangle);

            if ((a == SharedEdge.mV1 || a == SharedEdge.mV2) &&
                (b == SharedEdge.mV1 || b == SharedEdge.mV2)) 
            {
                return c;
            }
            else if ((a == SharedEdge.mV1 || a == SharedEdge.mV2) &&
                (c == SharedEdge.mV1 || c == SharedEdge.mV2)) 
            {
                return b;
            }
                
            return a;
        }

        TriangleToQuadConversionUtil::Quad TriangleToQuadConversionUtil::CreateQuadFromTriangles(const std::tuple<uint32_t, uint32_t, uint32_t>& Tri1,
            const std::tuple<uint32_t, uint32_t, uint32_t>& Tri2,
            const Edge& SharedEdge) 
        {
            uint32_t Third1 = GetThirdVertex(Tri1, SharedEdge);
            uint32_t Third2 = GetThirdVertex(Tri2, SharedEdge);

            return Quad(SharedEdge.mV1, Third1, SharedEdge.mV2, Third2);
        }

        std::vector<std::pair<TriangleToQuadConversionUtil::Quad, uint32_t>> TriangleToQuadConversionUtil::ConvertTrianglesToQuads(const application::file::game::phive::shape::PhiveShape::GeneratorData& GeneratorData)
        {
            std::vector<std::pair<Quad, uint32_t>> Result;
            std::unordered_set<size_t> ProcessedTriangles;

            std::unordered_map<Edge, std::vector<size_t>, EdgeHash> EdgeToTriangles;

            for (size_t i = 0; i < GeneratorData.mIndices.size(); i++)
            {
                const auto& Triangle = GeneratorData.mIndices[i].first;

                uint32_t IdA = std::get<0>(Triangle);
                uint32_t IdB = std::get<1>(Triangle);
                uint32_t IdC = std::get<2>(Triangle);

                EdgeToTriangles[Edge(IdA, IdB)].push_back(i);
                EdgeToTriangles[Edge(IdB, IdC)].push_back(i);
                EdgeToTriangles[Edge(IdC, IdA)].push_back(i);
            }

            for (size_t i = 0; i < GeneratorData.mIndices.size(); i++)
            {
                if (ProcessedTriangles.contains(i)) continue;

                const auto& Triangle1 = GeneratorData.mIndices[i].first;

                uint32_t a1 = std::get<0>(Triangle1);
                uint32_t b1 = std::get<1>(Triangle1);
                uint32_t c1 = std::get<2>(Triangle1);

                bool FoundQuad = false;

                std::vector<Edge> Edges = { Edge(a1, b1), Edge(b1, c1), Edge(c1, a1) };

                for (const Edge& edge : Edges) 
                {
                    auto EdgeIt = EdgeToTriangles.find(edge);
                    if (EdgeIt == EdgeToTriangles.end()) continue;

                    const auto& TrianglesWithEdge = EdgeIt->second;

                    for (size_t j : TrianglesWithEdge) 
                    {
                        if (j == i || ProcessedTriangles.contains(j)) continue;

                        if (GeneratorData.mIndices[j].second != GeneratorData.mIndices[i].second) continue;

                        const auto& Triangle2 = GeneratorData.mIndices[j].first;
                        uint32_t a2 = std::get<0>(Triangle2);
                        uint32_t b2 = std::get<1>(Triangle2);
                        uint32_t c2 = std::get<2>(Triangle2);

                        std::unordered_set<uint32_t> Vertices1 = { a1, b1, c1 };
                        std::unordered_set<uint32_t> Vertices2 = { a2, b2, c2 };

                        int SharedCount = 0;
                        for (uint32_t v : Vertices1) 
                        {
                            if (Vertices2.count(v)) SharedCount++;
                        }

                        if (SharedCount == 2)
                        {
                            Quad Primitive = CreateQuadFromTriangles(Triangle1, Triangle2, edge);
                            Result.emplace_back(Primitive, GeneratorData.mIndices[i].second);

                            ProcessedTriangles.insert(i);
                            ProcessedTriangles.insert(j);
                            FoundQuad = true;
                            break;
                        }
                    }

                    if (FoundQuad) break;
                }

                if (!FoundQuad) 
                {
                    Quad Primitive(a1, b1, c1, c1); //Practically a triangle stored as quad
                    Result.emplace_back(Primitive, GeneratorData.mIndices[i].second);
                    ProcessedTriangles.insert(i);
                }
            }

            return Result;
        }
	}
}