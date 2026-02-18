#include <file/game/phive/starlight_physics/shape/hknpMeshShapeConversionUtils.h>
#include <glm/common.hpp>
#include <glm/vec4.hpp>
#include <glm/glm.hpp>
#include <algorithm>

namespace application::file::game::phive::starlight_physics::shape::hknpMeshShapeConversionUtils
{
    namespace
    {
        struct FloatBits {
            static int extractExponent(float val) 
            {
                uint32_t bits = *reinterpret_cast<uint32_t*>(&val);
                return static_cast<int>(bits >> 23) - 127;
            }

            static float computeQuantStep(float precision) 
            {
                int exp = extractExponent(precision);
                return std::pow(2.0f, exp + 1);
            }
        };

        inline glm::vec4 quantizeToGrid(const glm::vec4& v, const glm::vec4& gridScale) 
        {
            return glm::round(v * gridScale);
        }
    }

    Vertex::Vertex(float maxVertexError)
    {
        float quantizationStep = FloatBits::computeQuantStep(maxVertexError);
        assert(quantizationStep <= 2.0 * maxVertexError);

        float invStep = 1.0f / quantizationStep;
        mBitScale16 = glm::vec4(invStep, invStep, invStep, 0.0f);
        mBitScale16Inv = glm::vec4(quantizationStep, quantizationStep, quantizationStep, 0.0f);
    }

    glm::u32vec4 Vertex::getSectionOffset(const common::hkAabb& sectionDomain) const
    {
        glm::vec4 quantized = quantizeToGrid(sectionDomain.mMin, mBitScale16);

        glm::u32vec4 result;
        int32_t* signedResult = reinterpret_cast<int32_t*>(&result);
        for (int i = 0; i < 4; i++) {
            signedResult[i] = static_cast<int32_t>(quantized[i]);
        }
        return result;
    }

    void Vertex::pack(const glm::vec4& vertex, const glm::u32vec4& sectionOffset, uint16_t* packedVertexOut) const
    {
        glm::vec4 gridAligned = quantizeToGrid(vertex, mBitScale16);

        glm::i32vec4 asInt(
            static_cast<int32_t>(gridAligned.x),
            static_cast<int32_t>(gridAligned.y),
            static_cast<int32_t>(gridAligned.z),
            static_cast<int32_t>(gridAligned.w)
        );

        glm::i32vec4 baseOffset(
            static_cast<int32_t>(sectionOffset.x),
            static_cast<int32_t>(sectionOffset.y),
            static_cast<int32_t>(sectionOffset.z),
            static_cast<int32_t>(sectionOffset.w)
        );

        glm::u32vec4 relative = glm::u32vec4(asInt - baseOffset);

        assert(relative.x <= 65535 && "X coordinate exceeds uint16 range");
        assert(relative.y <= 65535 && "Y coordinate exceeds uint16 range");
        assert(relative.z <= 65535 && "Z coordinate exceeds uint16 range");

        packedVertexOut[0] = static_cast<uint16_t>(glm::clamp(relative.x, 0u, 65535u));
        packedVertexOut[1] = static_cast<uint16_t>(glm::clamp(relative.y, 0u, 65535u));
        packedVertexOut[2] = static_cast<uint16_t>(glm::clamp(relative.z, 0u, 65535u));
    }

    glm::vec4 Vertex::addError(const glm::vec4& vertex) const
    {
        glm::vec4 snapped = quantizeToGrid(vertex, mBitScale16);
        glm::vec4 reconstructed = snapped * mBitScale16Inv;
        return reconstructed;
    }

    bool Vertex::isErrorAchievable(float maxVertexError, float maxRepresentableValue)
    {
        float step = FloatBits::computeQuantStep(maxVertexError);
        int stepExp = FloatBits::extractExponent(step);
        int scaleExp = -stepExp;

        int maxValueExp = FloatBits::extractExponent(maxRepresentableValue);

        return (maxValueExp + scaleExp) <= 24;
    }

    float Vertex::calcMaxExtent(float maxVertexError)
    {
        float step = FloatBits::computeQuantStep(maxVertexError);
        const float maxCoord = static_cast<float>(std::numeric_limits<uint16_t>::max() - 1);
        return maxCoord * step;
    }

    glm::vec4 Vertex::unpack(const uint16_t* packedVertex, const glm::u32vec4& sectionOffset) const
    {
        uint32_t lowPair = *reinterpret_cast<const uint32_t*>(packedVertex);
        uint32_t highPair = *reinterpret_cast<const uint32_t*>(packedVertex + 2);

        glm::i32vec4 unpacked(
            static_cast<int32_t>(static_cast<uint16_t>(lowPair & 0xFFFF)),
            static_cast<int32_t>(static_cast<uint16_t>(lowPair >> 16)),
            static_cast<int32_t>(static_cast<uint16_t>(highPair & 0xFFFF)),
            static_cast<int32_t>(static_cast<uint16_t>(highPair >> 16))
        );

        glm::i32vec4 baseOffset(
            static_cast<int32_t>(sectionOffset.x),
            static_cast<int32_t>(sectionOffset.y),
            static_cast<int32_t>(sectionOffset.z),
            static_cast<int32_t>(sectionOffset.w)
        );

        glm::i32vec4 absolute = unpacked + baseOffset;
        glm::vec4 worldSpace = glm::vec4(absolute) * mBitScale16Inv;

        return worldSpace;
    }



    glm::vec4 CalcExpandedDomainSpan(starlight_physics::common::hkAabb& domain)
    {
        glm::bvec3 isFlat = glm::equal(glm::vec3(domain.mMin), glm::vec3(domain.mMax));
        const float domainEps = std::numeric_limits<float>::epsilon();

        if (glm::any(isFlat)) {
            if (isFlat.x) { domain.mMin.x -= domainEps; domain.mMax.x += domainEps; }
            if (isFlat.y) { domain.mMin.y -= domainEps; domain.mMax.y += domainEps; }
            if (isFlat.z) { domain.mMin.z -= domainEps; domain.mMax.z += domainEps; }
        }

        glm::vec4 aabbMinAbs = glm::abs(domain.mMin / 255.0f);

        glm::vec4 extents = domain.mMax - domain.mMin;
        glm::vec4 expand = extents / 255.0f;

        expand = glm::max(expand, aabbMinAbs);

        domain.mMax += expand;
        domain.mMin -= expand;

        glm::vec4 span = domain.mMax - domain.mMin;
        span.w = 1.0f;

        return span;
    }

    Aabb8::Aabb8(const common::hkAabb& sectionDomain)
    {
        starlight_physics::common::hkAabb expandedDomain = sectionDomain;

        glm::vec4 span = CalcExpandedDomainSpan(expandedDomain);

        mBitScale = glm::vec4(255.0f) / span;

        glm::vec4 rangeMin = expandedDomain.mMin * mBitScale;

        mBitOffset = glm::ivec4(glm::round(rangeMin));

        mBitScaleInv = 1.0f / mBitScale;

        mBitScale.w = 0.0f;
        mBitScaleInv.w = 0.0f;
        mBitOffset.w = 0;

        mBitOffset16 = glm::i16vec4(
            (int16_t)std::clamp((int)mBitOffset.x, -32768, 32767),
            (int16_t)std::clamp((int)mBitOffset.y, -32768, 32767),
            (int16_t)std::clamp((int)mBitOffset.z, -32768, 32767),
            (int16_t)std::clamp((int)mBitOffset.w, -32768, 32767)
        );
    }

    template <int RoundDir>
    uint32_t ConvertVector(const glm::vec4& vectorF, const glm::vec4& bitScale8, const glm::vec4& bitScale8Inv, glm::u32vec4 bitOffset8)
    {
        glm::vec4 scaled = bitScale8 * vectorF;
        glm::vec4 roundedF;
        if constexpr (RoundDir == -1) {
            roundedF = glm::floor(scaled);
        }
        else {
            roundedF = glm::ceil(scaled);
        }

        glm::ivec4 compressedVector = glm::ivec4(roundedF);
        glm::ivec4 roundedCompressed = compressedVector;

        compressedVector -= bitOffset8;

        {
            glm::vec4 tmpRestoredVector = glm::vec4(roundedCompressed) * bitScale8Inv;

            glm::ivec4 intSpaceFixUp(0);

            glm::bvec4 breached;
            if constexpr (RoundDir == -1) {
                breached = glm::greaterThan(tmpRestoredVector, vectorF);
            }
            else {
                breached = glm::lessThan(tmpRestoredVector, vectorF);
            }

            for (int i = 0; i < 4; ++i) {
                if (breached[i]) {
                    compressedVector[i] += RoundDir;
                }
            }
        }

        uint8_t packed[4];
        for (int i = 0; i < 4; ++i) {
            packed[i] = (uint8_t)std::clamp(compressedVector[i], 0, 255);
        }

        uint32_t result;
        std::memcpy(&result, packed, 4);
        return result;
    }

    classes::HavokClasses::hknpAabb8 Aabb8::convertAabb(const common::hkAabb& aabbF) const
    {
        classes::HavokClasses::hknpAabb8 aabbOut;
        aabbOut.mMin.mParent = ConvertVector<-1>(aabbF.mMin, mBitScale, mBitScaleInv, mBitOffset);
        aabbOut.mMax.mParent = ConvertVector<1>(aabbF.mMax, mBitScale, mBitScaleInv, mBitOffset);
        return aabbOut;
    }

    void Aabb8::convertAabb(const classes::HavokClasses::hkcdFourAabb& aabbF, classes::HavokClasses::hknpTransposedFourAabbs8& aabbOut) const
    {
        glm::vec4 bitScale;
        glm::vec4 bitScaleInv;
        glm::u32vec4 bitOffset;

        constexpr int value = 1;

        bitScale = glm::vec4(mBitScale[0]);
        bitScaleInv = glm::vec4(mBitScaleInv[0]);
        bitOffset = glm::u32vec4(mBitOffset[0]);
        aabbOut.mLx.mParent = ConvertVector<-value>(aabbF.mLx.ToVec4(), bitScale, bitScaleInv, bitOffset);
        aabbOut.mHx.mParent = ConvertVector<value>(aabbF.mHx.ToVec4(), bitScale, bitScaleInv, bitOffset);

        bitScale = glm::vec4(mBitScale[1]);
        bitScaleInv = glm::vec4(mBitScaleInv[1]);
        bitOffset = glm::u32vec4(mBitOffset[1]);
        aabbOut.mLy.mParent = ConvertVector<-value>(aabbF.mLy.ToVec4(), bitScale, bitScaleInv, bitOffset);
        aabbOut.mHy.mParent = ConvertVector<value>(aabbF.mHy.ToVec4(), bitScale, bitScaleInv, bitOffset);

        bitScale = glm::vec4(mBitScale[2]);
        bitScaleInv = glm::vec4(mBitScaleInv[2]);
        bitOffset = glm::u32vec4(mBitOffset[2]);
        aabbOut.mLz.mParent = ConvertVector<-value>(aabbF.mLz.ToVec4(), bitScale, bitScaleInv, bitOffset);
        aabbOut.mHz.mParent = ConvertVector<value>(aabbF.mHz.ToVec4(), bitScale, bitScaleInv, bitOffset);
    }

    classes::HavokClasses::hkcdFourAabb Aabb8::clipFourAabbsToDomain(const common::hkAabb& domain, const classes::HavokClasses::hkcdFourAabb& fourAabbs)
    {
        classes::HavokClasses::hkcdFourAabb clipped = fourAabbs;

        const float fMax = std::numeric_limits<float>::max();
        const float fMin = -fMax;

        auto clipAxis = [&](glm::vec4& low, glm::vec4& high, float domMin, float domMax)
            {
                for (int i = 0; i < 4; ++i)
                {
                    if (low[i] == fMax) low[i] = domMax;
                    low[i] = glm::max(domMin, low[i]);

                    if (high[i] == fMin) high[i] = domMin;
                    high[i] = glm::min(domMax, high[i]);
                }
            };

        glm::vec4 lx = clipped.mLx.ToVec4();
        glm::vec4 ly = clipped.mLy.ToVec4();
        glm::vec4 lz = clipped.mLz.ToVec4();

        glm::vec4 hx = clipped.mHx.ToVec4();
        glm::vec4 hy = clipped.mHy.ToVec4();
        glm::vec4 hz = clipped.mHz.ToVec4();

        clipAxis(lx, hx, domain.mMin.x, domain.mMax.x);
        clipAxis(ly, hy, domain.mMin.y, domain.mMax.y);
        clipAxis(lz, hz, domain.mMin.z, domain.mMax.z);

        clipped.mLx.FromVec4(lx);
        clipped.mLy.FromVec4(ly);
        clipped.mLz.FromVec4(lz);

        clipped.mHx.FromVec4(hx);
        clipped.mHy.FromVec4(hy);
        clipped.mHz.FromVec4(hz);

        return clipped;
    }
}