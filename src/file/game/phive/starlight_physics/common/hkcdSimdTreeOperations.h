#pragma once

#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cassert>

#include <file/game/phive/starlight_physics/common/hkcdSimdTreeGeneration.h>

namespace application::file::game::phive::starlight_physics::common::hkcdSimdTreeOperations
{
    // -------------------------------------------------------------------------
    // FixedStack — simple fixed-capacity stack
    // -------------------------------------------------------------------------
    template <typename T>
    struct FixedStack
    {
        static constexpr int kCapacity = 256;

        inline FixedStack()
        {
            mTop = mStorage;
        }

        inline FixedStack(T root) { reset(root); }

        inline T pop()
        {
            assert(!empty());
            return *(--mTop);
        }

        inline void push(T val)
        {
            assert((depth() + 1) < kCapacity);
            *mTop++ = val;
        }

        inline void pushMany(const glm::u32vec4& indices, int n)
        {
            assert(n <= 4);
            assert((depth() + 4) < kCapacity);
            for (int i = 0; i < n; i++) assert(indices[i] != 0);
            std::memcpy(mTop, glm::value_ptr(indices), sizeof(indices));
            mTop += n;
        }

        inline void reset(T root)
        {
            mStorage[0] = root;
            mTop = mStorage + 1;
        }

        inline bool empty() const { return mStorage == mTop; }
        inline int  depth() const { return static_cast<int>(mTop - mStorage); }

        T  mStorage[kCapacity];
        T* mTop;
    };

    // -------------------------------------------------------------------------
    // BoxArrayLeafQuery — retrieves leaf AABBs from a flat array
    // -------------------------------------------------------------------------
    struct BoxArrayLeafQuery
    {
        inline BoxArrayLeafQuery(const hkAabb* src) : mSource(src) {}
        inline void operator()(uint32_t id, hkAabb* out) const { *out = mSource[id]; }
        const hkAabb* mSource;
    };

    // -------------------------------------------------------------------------
    // IBoundsUpdater
    // -------------------------------------------------------------------------
    struct IBoundsUpdater : hkcdSimdTreeGeneration::ILeafBoundsQuery
    {
        virtual ~IBoundsUpdater() {}

        inline IBoundsUpdater() : mNodeBoxes(nullptr) {}

        virtual void updateSubtree(
            classes::HavokClasses::hkcdSimdTree__Node* nodes,
            int firstNode,
            int numNodes,
            bool linearLayout = true) = 0;

        virtual void updateSingleNode(
            const classes::HavokClasses::hkcdSimdTree__Node* nodes,
            classes::HavokClasses::hkcdSimdTree__Node* node) = 0;

        // ---- shared implementation helpers ----

        template <typename GET_BOX>
        inline void _updateSingleNode(
            const GET_BOX& getBox,
            const classes::HavokClasses::hkcdSimdTree__Node* nodes,
            classes::HavokClasses::hkcdSimdTree__Node* node)
        {
            hkAabb b0, b1, b2, b3;

            if (node->mIsLeaf)
            {
                b3.SetEmpty();
                assert(node->IsLeafValid(0));
                getBox(node->mData[0].mParent, &b0);

                b1 = node->IsLeafValid(1) ? (getBox(node->mData[1].mParent, &b1), b1) : b3;
                b2 = node->IsLeafValid(2) ? (getBox(node->mData[2].mParent, &b2), b2) : b3;
                b3 = node->IsLeafValid(3) ? (getBox(node->mData[3].mParent, &b3), b3) : b3;

                if (node->IsLeafValid(1)) getBox(node->mData[1].mParent, &b1);
                else                     b1 = hkAabb(), b1.SetEmpty();

                if (node->IsLeafValid(2)) getBox(node->mData[2].mParent, &b2);
                else                     b2 = hkAabb(), b2.SetEmpty();

                hkAabb empty; empty.SetEmpty();
                b3 = empty;
                assert(node->IsLeafValid(0));
                getBox(node->mData[0].mParent, &b0);
                if (node->IsLeafValid(1)) getBox(node->mData[1].mParent, &b1); else b1 = empty;
                if (node->IsLeafValid(2)) getBox(node->mData[2].mParent, &b2); else b2 = empty;
                if (node->IsLeafValid(3)) getBox(node->mData[3].mParent, &b3);
            }
            else
            {
                b0 = mNodeBoxes[node->mData[0].mParent];
                b1 = mNodeBoxes[node->mData[1].mParent];
                b2 = mNodeBoxes[node->mData[2].mParent];
                b3 = mNodeBoxes[node->mData[3].mParent];
            }

            hkAabb u01, u23;
            u01.SetUnion(b0, b1);
            u23.SetUnion(b2, b3);
            mNodeBoxes[int(node - nodes)].SetUnion(u01, u23);

            auto swizzle4 = [](glm::vec4& v0, glm::vec4& v1, glm::vec4& v2, glm::vec4& v3)
                {
                    glm::mat4 m(v0, v1, v2, v3);
                    m = glm::transpose(m);
                    v0 = m[0]; v1 = m[1]; v2 = m[2]; v3 = m[3];
                };

            swizzle4(b0.mMin, b1.mMin, b2.mMin, b3.mMin);
            swizzle4(b0.mMax, b1.mMax, b2.mMax, b3.mMax);

            node->mParent.mLx.FromVec4(b0.mMin);
            node->mParent.mHx.FromVec4(b0.mMax);
            node->mParent.mLy.FromVec4(b1.mMin);
            node->mParent.mHy.FromVec4(b1.mMax);
            node->mParent.mLz.FromVec4(b2.mMin);
            node->mParent.mHz.FromVec4(b2.mMax);
        }

        template <typename GET_BOX>
        void _updateSubtreeLinear(
            const GET_BOX& getBox,
            classes::HavokClasses::hkcdSimdTree__Node* nodes,
            int firstNode,
            int numNodes)
        {
            assert(numNodes >= 0);
            const classes::HavokClasses::hkcdSimdTree__Node* stop = nodes + firstNode;
            classes::HavokClasses::hkcdSimdTree__Node* current = nodes + firstNode + numNodes - 1;
            do { _updateSingleNode(getBox, nodes, current--); } while (current >= stop);
        }

        template <typename GET_BOX>
        void _updateSubtreeGeneric(
            const GET_BOX& getBox,
            classes::HavokClasses::hkcdSimdTree__Node* nodes,
            int firstNode)
        {
            FixedStack<uint32_t> stk((firstNode << 1) | 1);

            do
            {
                uint32_t nd = stk.pop();

            PROCESS:
                classes::HavokClasses::hkcdSimdTree__Node& node = nodes[nd >> 1];

                hkAabb b0, b1, b2, b3;

                if (node.mIsLeaf)
                {
                    hkAabb empty; empty.SetEmpty();
                    b3 = empty;
                    assert(node.IsLeafValid(0));
                    getBox(node.mData[0].mParent, &b0);
                    if (node.IsLeafValid(1)) getBox(node.mData[1].mParent, &b1); else b1 = empty;
                    if (node.IsLeafValid(2)) getBox(node.mData[2].mParent, &b2); else b2 = empty;
                    if (node.IsLeafValid(3)) getBox(node.mData[3].mParent, &b3);
                }
                else
                {
                    if (nd & 1)
                    {
                        stk.push(nd & ~1u);
                        assert(node.IsInternalValid(0));
                        nd = (node.mData[0].mParent << 1) | 1;
                        for (int i = 3; i >= 1; --i)
                            if (node.IsInternalValid(i))
                                stk.push((node.mData[i].mParent << 1) | 1);
                        goto PROCESS;
                    }
                    else
                    {
                        hkAabb empty; empty.SetEmpty();
                        b3 = empty;
                        assert(node.IsInternalValid(0));
                        nodes[node.mData[0].mParent].mParent.GetCompoundAabb(&b0);
                        if (node.IsInternalValid(1)) nodes[node.mData[1].mParent].mParent.GetCompoundAabb(&b1); else b1 = empty;
                        if (node.IsInternalValid(2)) nodes[node.mData[2].mParent].mParent.GetCompoundAabb(&b2); else b2 = empty;
                        if (node.IsInternalValid(3)) nodes[node.mData[3].mParent].mParent.GetCompoundAabb(&b3);
                    }
                }

                {
                    auto swizzle4 = [](glm::vec4& v0, glm::vec4& v1, glm::vec4& v2, glm::vec4& v3)
                        {
                            glm::mat4 m(v0, v1, v2, v3);
                            m = glm::transpose(m);
                            v0 = m[0]; v1 = m[1]; v2 = m[2]; v3 = m[3];
                        };

                    swizzle4(b0.mMin, b1.mMin, b2.mMin, b3.mMin);
                    swizzle4(b0.mMax, b1.mMax, b2.mMax, b3.mMax);

                    node.mParent.mLx.FromVec4(b0.mMin);
                    node.mParent.mHx.FromVec4(b0.mMax);
                    node.mParent.mLy.FromVec4(b1.mMin);
                    node.mParent.mHy.FromVec4(b1.mMax);
                    node.mParent.mLz.FromVec4(b2.mMin);
                    node.mParent.mHz.FromVec4(b2.mMax);
                }

            } while (!stk.empty());
        }

        hkAabb* mNodeBoxes;
    };

    // -------------------------------------------------------------------------
    // BoxRefitter
    // -------------------------------------------------------------------------
    template <typename GET_BOX>
    struct BoundsUpdaterImpl : public IBoundsUpdater
    {
        inline BoundsUpdaterImpl(const GET_BOX& getBox) : mGetBox(getBox) {}

        void updateSubtree(
            classes::HavokClasses::hkcdSimdTree__Node* nodes,
            int firstNode,
            int numNodes,
            bool linearLayout = true) override
        {
            if (linearLayout)
            {
                mNodeBoxes[0].SetEmpty();
                _updateSubtreeLinear(mGetBox, nodes, firstNode, numNodes);
            }
            else
            {
                _updateSubtreeGeneric(mGetBox, nodes, firstNode);
            }
        }

        void updateSingleNode(
            const classes::HavokClasses::hkcdSimdTree__Node* nodes,
            classes::HavokClasses::hkcdSimdTree__Node* node) override
        {
            _updateSingleNode(mGetBox, nodes, node);
        }

        void fetchLeafBox(uint32_t id, hkAabb* out) const override
        {
            mGetBox(id, out);
        }

        GET_BOX mGetBox;
    };

    using BoxRefitter = BoundsUpdaterImpl<BoxArrayLeafQuery>;

    // -------------------------------------------------------------------------
    // BoxAccumulator
    // -------------------------------------------------------------------------
    struct BoxAccumulator
    {
        inline void reset()
        {
            mHi = glm::vec4(-std::numeric_limits<float>::infinity());
            mLo.storePositive(glm::vec4(std::numeric_limits<float>::infinity()));
        }

        inline void expand(const hkcdSimdTreeGeneration::Vec4Sortable& p)
        {
            mLo.assignMin(mLo, p);
            mHi.assignMax(mHi, p);
        }

        inline hkAabb toBox() const
        {
            hkAabb r;
            r.mMin = mLo.decode();
            r.mMax = mHi.decode();
            return r;
        }

        inline const BoxAccumulator& operator=(const hkAabb& v)
        {
            mLo = v.mMin;
            mHi = v.mMax;
            return *this;
        }

        hkcdSimdTreeGeneration::Vec4Sortable mLo;
        hkcdSimdTreeGeneration::Vec4Sortable mHi;
    };

    void sahSplit(
        hkcdSimdTreeGeneration::TreeAssembler& assembler,
        const hkcdSimdTreeGeneration ::Interval& interval,
        hkcdSimdTreeGeneration::Interval* outLeft,
        hkcdSimdTreeGeneration::Interval* outRight,
        int minPartitionSize);

    void construct(
        hkcdSimdTreeGeneration::TreeAssembler& assembler,
        classes::HavokClasses::hkcdSimdTree& outTree,
        IBoundsUpdater* updater);
}