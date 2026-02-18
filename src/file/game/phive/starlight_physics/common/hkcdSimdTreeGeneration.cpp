#include <file/game/phive/starlight_physics/common/hkcdSimdTreeGeneration.h>

#include <file/game/phive/starlight_physics/common/hkcdSimdTreeOperations.h>

#include <util/Logger.h>

namespace application::file::game::phive::starlight_physics::common::hkcdSimdTreeGeneration
{
    // -------------------------------------------------------------------------
    // Interval
    // -------------------------------------------------------------------------

    void Interval::bisect(int leftCount, Interval* outLeft, Interval* outRight) const
    {
        assert(this != outLeft && this != outRight);
        assert(leftCount > 0 && leftCount < mCount);
        assert(outLeft != nullptr);
        assert(outRight != nullptr);

        outLeft->mStart = mStart;
        outLeft->mCount = leftCount;

        outRight->mStart = mStart + leftCount;
        outRight->mCount = mCount - leftCount;

        assert(outRight->mCount >= 1);
    }

    // -------------------------------------------------------------------------
    // Vec4Sortable
    // -------------------------------------------------------------------------

    Vec4Sortable::Vec4Sortable(const glm::vec4& v)
    {
        glm::u32vec4 raw = glm::floatBitsToUint(v);
        glm::i32vec4 sign = glm::i32vec4(raw) >> 31;
        glm::u32vec4 flip = glm::u32vec4(sign) >> (uint32_t)1;
        mBits = raw ^ flip;
    }

    void Vec4Sortable::storePositive(const glm::vec4& v)
    {
        mBits = glm::floatBitsToUint(v);
    }

    const Vec4Sortable& Vec4Sortable::operator=(const glm::vec4& v)
    {
        *this = Vec4Sortable(v);
        return *this;
    }

    const glm::vec4 Vec4Sortable::decode() const
    {
        glm::i32vec4 sign = glm::i32vec4(mBits) >> 31;
        glm::u32vec4 flip = glm::u32vec4(sign) >> (uint32_t)1;
        glm::u32vec4 result = mBits ^ flip;
        return glm::uintBitsToFloat(result);
    }

    Vec4Sortable::operator const glm::vec4() const
    {
        return decode();
    }

    void Vec4Sortable::assignMin(const Vec4Sortable& a, const Vec4Sortable& b)
    {
        mBits = glm::min(a.mBits, b.mBits);
    }

    void Vec4Sortable::assignMax(const Vec4Sortable& a, const Vec4Sortable& b)
    {
        mBits = glm::max(a.mBits, b.mBits);
    }

    int& Vec4Sortable::operator()(int i)
    {
        return *(((int*)&mBits) + i);
    }

    int Vec4Sortable::operator()(int i) const
    {
        return (int)mBits[i];
    }

    // -------------------------------------------------------------------------
    // Primitive
    // -------------------------------------------------------------------------

    Primitive::Primitive(const glm::vec4& pos, uint32_t payload)
    {
        assign(pos, payload);
    }

    void Primitive::assign(const glm::vec4& pos, uint32_t payload)
    {
        mKey = pos;
        mKey(3) = payload;
    }

    void Primitive::assignPosition(const glm::vec4& pos)
    {
        Vec4Sortable tmp = pos;
        mKey.mBits.x = tmp.mBits.x;
        mKey.mBits.y = tmp.mBits.y;
        mKey.mBits.z = tmp.mBits.z;
    }

    glm::vec4 Primitive::fetchPosition() const
    {
        return mKey.decode();
    }

    const Vec4Sortable& Primitive::rawSortable() const
    {
        return mKey;
    }

    uint32_t Primitive::fetchPayload() const
    {
        return mKey(3);
    }

    void Primitive::storePayload(uint32_t val)
    {
        mKey(3) = val;
    }

    Primitive Primitive::makeZero()
    {
        Primitive p;
        p.mKey.mBits = glm::u32vec4(0, 0, 0, 0);
        return p;
    }

    void Primitive::markInvalid(uint32_t payload)
    {
        constexpr float kNan = std::numeric_limits<float>::quiet_NaN();
        mKey.storePositive(glm::vec4(kNan, kNan, kNan, 0.0f));
        mKey(3) = payload;
    }

    bool Primitive::checkValid() const
    {
        return mKey(3) != 0x7fc00000;
    }

    int Primitive::operator()(int i) const
    {
        return mKey(i);
    }

    Primitive::operator glm::vec4() const
    {
        return fetchPosition();
    }

    void TreeAssembler::copyFrom(const TreeAssembler& src)
    {
        mDepthLevels = src.mDepthLevels;
        mShrink = src.mShrink;
        mReuseOldBounds = src.mReuseOldBounds;
        mPrimitives = src.mPrimitives;
    }

    void TreeAssembler::subdivideIntoFour(const Interval& interval, Interval* outQuad)
    {
        Interval left, right;
        hkcdSimdTreeOperations::sahSplit(*this, interval, &left, &right, 2);
        hkcdSimdTreeOperations::sahSplit(*this, left, outQuad + 0, outQuad + 1, 1);
        hkcdSimdTreeOperations::sahSplit(*this, right, outQuad + 2, outQuad + 3, 1);
    }

    void buildFromAabbs(classes::HavokClasses::hkcdSimdTree& outTree, TreeAssembler& assembler, const hkAabb* boxes)
    {
        hkcdSimdTreeOperations::BoxArrayLeafQuery leafQuery(boxes);
        hkcdSimdTreeOperations::BoxRefitter refit(leafQuery);
        hkcdSimdTreeOperations::construct(assembler, outTree, &refit);
    }
}