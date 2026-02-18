#pragma once

#define GLM_ENABLE_EXPERIMENTAL

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <file/game/phive/classes/HavokClasses.h>
#include <file/game/phive/shape/PhiveShape.h>
#include <vector>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <glm/glm.hpp>
#include <glm/gtx/normal.hpp>

/*

THIS IS AN OUTDATED IMPLEMENTATION

*/

namespace application::file::game::phive::shape
{
	namespace PhiveShapeUtil
	{
		struct GeometrySectionConversionUtil
		{
			glm::vec3 mBitScale;
			glm::i32vec3 mBitOffset;
			glm::i16vec3 mBitOffset16;
			glm::vec3 mBitScaleInv;

			glm::vec3 mOriginalMin;
			glm::vec3 mOriginalMax;

			static glm::vec3 PrepareDomain(glm::vec3& Min, glm::vec3 Max);

			glm::i32vec3 GetSectionOffset(const application::file::game::phive::classes::HavokClasses::hknpMeshShapeVertexConversionUtil& VertexConversionUtil);
			glm::i16vec3 CompressVertex(const application::file::game::phive::classes::HavokClasses::hknpMeshShapeVertexConversionUtil& VertexConversionUtil, const glm::i32vec3& SectionOffset, const glm::vec3& Vertex);

			GeometrySectionConversionUtil(const glm::vec3& Min, const glm::vec3& Max);
		};

		namespace TriangleToQuadConversionUtil
		{
			struct Edge
			{
				uint32_t mV1, mV2;

				Edge(uint32_t a, uint32_t b) : mV1(std::min(a, b)), mV2(std::max(a, b)) {}

				bool operator==(const Edge& other) const 
				{
					return mV1 == other.mV1 && mV2 == other.mV2;
				}
			};

			struct EdgeHash 
			{
				std::size_t operator()(const Edge& e) const 
				{
					return std::hash<uint64_t>{}((static_cast<uint64_t>(e.mV1) << 32) | e.mV2);
				}
			};

            struct Quad 
			{
                uint32_t mIndices[4];

                Quad(uint32_t i0, uint32_t i1, uint32_t i2, uint32_t i3) 
				{
                    mIndices[0] = i0;
                    mIndices[1] = i1;
                    mIndices[2] = i2;
                    mIndices[3] = i3;
                }
            };

			uint32_t GetThirdVertex(const std::tuple<uint32_t, uint32_t, uint32_t>& Triangle, const Edge& SharedEdge);
			Quad CreateQuadFromTriangles(const std::tuple<uint32_t, uint32_t, uint32_t>& Tri1, const std::tuple<uint32_t, uint32_t, uint32_t>& Tri2, const Edge& SharedEdge);
			std::vector<std::pair<Quad, uint32_t>> ConvertTrianglesToQuads(const application::file::game::phive::shape::PhiveShape::GeneratorData& generatorData);
		};
	}
}