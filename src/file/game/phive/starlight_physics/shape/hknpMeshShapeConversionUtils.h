#pragma once

#include <glm/vec4.hpp>
#include <file/game/phive/starlight_physics/common/hkAabb.h>
#include <file/game/phive/classes/HavokClasses.h>


namespace application::file::game::phive::starlight_physics::shape::hknpMeshShapeConversionUtils
{
    struct Vertex
	{
        Vertex() = default;
        Vertex(float maxVertexError);

        glm::vec4 mBitScale16;
        glm::vec4 mBitScale16Inv;

        glm::u32vec4 getSectionOffset(const common::hkAabb& sectionDomain) const;

        glm::vec4 addError(const glm::vec4& vertex) const;
        static bool isErrorAchievable(float maxVertexError, float maxRepresentableValue);
        static float calcMaxExtent(float maxVertexError);
        glm::vec4 unpack(const uint16_t* packedVertex, const glm::u32vec4& sectionOffset) const;
        void pack(const glm::vec4& vertex, const glm::u32vec4& sectionOffset, uint16_t* packedVertexOut) const;
    };

    struct Aabb8
    {
        Aabb8(const common::hkAabb& sectionDomain);
        classes::HavokClasses::hknpAabb8 convertAabb(const common::hkAabb& aabbF) const;
        void convertAabb(const classes::HavokClasses::hkcdFourAabb& aabbF, classes::HavokClasses::hknpTransposedFourAabbs8& aabbOut) const;
        static classes::HavokClasses::hkcdFourAabb clipFourAabbsToDomain(const common::hkAabb& domain, const classes::HavokClasses::hkcdFourAabb& fourAabbs);

        glm::vec4 mBitScaleInv;
        glm::u32vec4 mBitOffset;
        glm::i16vec4 mBitOffset16;
        glm::vec4 mBitScale;
    };
}