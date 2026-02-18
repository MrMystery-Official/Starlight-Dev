#pragma once

#include <cstdint>
#include <file/game/phive/starlight_physics/common/hkAabb.h>
#include <file/game/phive/classes/HavokClasses.h>

namespace application::file::game::phive::starlight_physics::common::hkcdSimdTreeGeneration
{
    struct Interval
    {
        Interval() = default;
        Interval(int start, int count, uint32_t nodeIdx)
            : mStart(start), mCount(count), mNodeIdx(nodeIdx)
        {
        }
        Interval(const Interval& src, uint32_t nodeIdx)
            : mBounds(src.mBounds), mStart(src.mStart), mCount(src.mCount), mNodeIdx(nodeIdx)
        {
        }

        void bisect(int leftCount, Interval* outLeft, Interval* outRight) const;

        common::hkAabb mBounds;
        int mStart;
        int mCount;
        int mNodeIdx;
    };

    struct ILeafBoundsQuery
    {
        virtual ~ILeafBoundsQuery() {}
        virtual void fetchLeafBox(uint32_t leafId, hkAabb* outBox) const = 0;
    };

    struct Vec4Sortable
    {
        Vec4Sortable() = default;
        Vec4Sortable(const glm::vec4& v);
        void storePositive(const glm::vec4& v);
        const Vec4Sortable& operator=(const glm::vec4& v);
        const glm::vec4 decode() const;
        operator const glm::vec4() const;

        void assignMin(const Vec4Sortable& a, const Vec4Sortable& b);
        void assignMax(const Vec4Sortable& a, const Vec4Sortable& b);

        int& operator()(int i);
        int  operator()(int i) const;

        glm::u32vec4 mBits;
    };

    struct Primitive
    {
        Primitive() = default;
        Primitive(const glm::vec4& pos, uint32_t payload);
        void assign(const glm::vec4& pos, uint32_t payload);
        void assignPosition(const glm::vec4& pos);
        glm::vec4 fetchPosition() const;
        const Vec4Sortable& rawSortable() const;
        uint32_t fetchPayload() const;
        void storePayload(uint32_t val);
        static Primitive makeZero();
        void markInvalid(uint32_t payload);
        bool checkValid() const;

        int operator()(int i) const;
        operator glm::vec4() const;

    protected:
        Vec4Sortable mKey;
    };

    struct TreeAssembler
    {
        virtual ~TreeAssembler() {}

        virtual void copyFrom(const TreeAssembler& src);
        virtual void onHierarchyBegin(bool mt, int branches, int reservedNodes) {}
        virtual void subdivideIntoFour(const Interval& interval, Interval* outQuad);

        int  mDepthLevels = 1;
        bool mShrink = true;
        bool mReuseOldBounds = false;
        bool mOrderBySize = false;
        bool mSahLeafSort = false;
        bool mNoValidation = false;

        std::vector<Primitive> mPrimitives;
        std::vector<hkAabb>    mNodeBounds;
        const ILeafBoundsQuery* mBoundsSource = nullptr;
    };

    void buildFromAabbs(classes::HavokClasses::hkcdSimdTree& outTree, TreeAssembler& assembler, const hkAabb* boxes);
}