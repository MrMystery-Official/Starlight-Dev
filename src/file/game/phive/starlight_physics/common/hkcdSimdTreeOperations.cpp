#include <file/game/phive/starlight_physics/common/hkcdSimdTreeOperations.h>

#include <algorithm>

namespace application::file::game::phive::starlight_physics::common::hkcdSimdTreeOperations
{
    // -------------------------------------------------------------------------
    // Internal helpers
    // -------------------------------------------------------------------------

    struct AxisPivot
    {
        int   mAxis;
        float mMidpoint;
    };

    static AxisPivot longestAxisSplit(const hkAabb& box)
    {
        AxisPivot ap;
        glm::vec4 span = box.mMax - box.mMin;

        ap.mAxis = 0;
        if (span.y > span.x)          ap.mAxis = 1;
        if (span.z > span[ap.mAxis])  ap.mAxis = 2;

        ap.mMidpoint = (box.mMin[ap.mAxis] + box.mMax[ap.mAxis]) * 0.5f;
        return ap;
    }

    static void reorderByAxis(hkcdSimdTreeGeneration::TreeAssembler& assembler, const hkcdSimdTreeGeneration::Interval& interval, int axis)
    {
        if (interval.mCount <= 1) return;

        auto first = assembler.mPrimitives.data() + interval.mStart;
        auto last = first + interval.mCount;

        switch (axis)
        {
        case 0:
            std::sort(first, last, [](const hkcdSimdTreeGeneration::Primitive& a, const hkcdSimdTreeGeneration::Primitive& b)
                { return a.fetchPosition().x < b.fetchPosition().x; });
            break;
        case 1:
            std::sort(first, last, [](const hkcdSimdTreeGeneration::Primitive& a, const hkcdSimdTreeGeneration::Primitive& b)
                { return a.fetchPosition().y < b.fetchPosition().y; });
            break;
        case 2:
            std::sort(first, last, [](const hkcdSimdTreeGeneration::Primitive& a, const hkcdSimdTreeGeneration::Primitive& b)
                { return a.fetchPosition().z < b.fetchPosition().z; });
            break;
        }
    }

    static void sahFixedPartition(
        hkcdSimdTreeGeneration::TreeAssembler& assembler,
        const hkcdSimdTreeGeneration::Interval& interval,
        int splitPivot,
        hkcdSimdTreeGeneration::Interval* outLeft,
        hkcdSimdTreeGeneration::Interval* outRight)
    {
        const int N = interval.mCount;
        std::vector<hkcdSimdTreeGeneration::Primitive> sorted[3];
        int   bestAxis = -1;
        float bestScore = std::numeric_limits<float>::max();

        for (int axis = 0; axis < 3; ++axis)
        {
            sorted[axis].insert(sorted[axis].end(),
                assembler.mPrimitives.data() + interval.mStart,
                assembler.mPrimitives.data() + interval.mStart + N);

            std::sort(sorted[axis].begin(), sorted[axis].end(),
                [axis](const hkcdSimdTreeGeneration::Primitive& a, const hkcdSimdTreeGeneration::Primitive& b)
                { return a(axis) < b(axis); });

            hkAabb leftBox, rightBox;
            leftBox.SetEmpty();
            rightBox = leftBox;

            hkcdSimdTreeGeneration::Primitive* pts = sorted[axis].data();
            for (int i = 0; i < N; ++i)
            {
                hkAabb leafBox;
                assembler.mBoundsSource->fetchLeafBox(pts[i].fetchPayload(), &leafBox);
                if (i < splitPivot) leftBox.IncludeAabb(leafBox);
                else                rightBox.IncludeAabb(leafBox);
            }

            const float score = leftBox.SurfaceArea() + rightBox.SurfaceArea();
            if (score < bestScore)
            {
                bestScore = score;
                bestAxis = axis;
            }
        }

        std::copy(sorted[bestAxis].begin(), sorted[bestAxis].begin() + N,
            assembler.mPrimitives.begin() + interval.mStart);

        interval.bisect(splitPivot, outLeft, outRight);
    }

    static void recursiveSahOrder(hkcdSimdTreeGeneration::TreeAssembler& assembler, const hkcdSimdTreeGeneration::Interval& interval)
    {
        if (interval.mCount <= 4) return;

        int nextPow2 = 4;
        while (nextPow2 < interval.mCount) nextPow2 *= 2;

        const int pivot = nextPow2 / 2;

        hkcdSimdTreeGeneration::Interval lo, hi;
        sahFixedPartition(assembler, interval, pivot, &lo, &hi);

        recursiveSahOrder(assembler, lo);
        recursiveSahOrder(assembler, hi);
    }

    // -------------------------------------------------------------------------
    // linkChildNodes — wires child node records into the tree and pushes work
    // -------------------------------------------------------------------------
    static void linkChildNodes(
        const hkcdSimdTreeGeneration::TreeAssembler& assembler,
        const hkcdSimdTreeGeneration::Interval* children,
        int childCount,
        const classes::HavokClasses::hkcdSimdTree__Node* nodeBase,
        classes::HavokClasses::hkcdSimdTree__Node* parent,
        classes::HavokClasses::hkcdSimdTree__Node*& freeSlot,
        hkcdSimdTreeGeneration::Interval* workStack,
        int& workTop)
    {
        const hkcdSimdTreeGeneration::Primitive* prims = assembler.mPrimitives.data();
        glm::u32vec4 refs(0);

        for (int i = 0; i < childCount; ++i)
        {
            const hkcdSimdTreeGeneration::Interval& child = children[i];
            const int childIdx = int(freeSlot - nodeBase);
            refs[i] = childIdx;

            if (child.mCount > 4)
            {
                workStack[workTop++] = hkcdSimdTreeGeneration::Interval(child, childIdx);
            }
            else
            {
                const int* raw = reinterpret_cast<const int*>(prims + child.mStart);
                glm::u32vec4 payloads(raw[4 * 0 + 3], raw[4 * 1 + 3], raw[4 * 2 + 3], raw[4 * 3 + 3]);

                freeSlot->mData[0].mParent = payloads.x;
                freeSlot->mData[1].mParent = payloads.y;
                freeSlot->mData[2].mParent = payloads.z;
                freeSlot->mData[3].mParent = payloads.w;

                for (int j = child.mCount; j < 4; ++j)
                    freeSlot->ClearLeafData(j);

                freeSlot->mIsLeaf = true;
            }

            ++freeSlot;
        }

        parent->mData[0].mParent = refs.x;
        parent->mData[1].mParent = refs.y;
        parent->mData[2].mParent = refs.z;
        parent->mData[3].mParent = refs.w;
        parent->mIsLeaf = false;
    }

    // -------------------------------------------------------------------------
    // handleLeafGroup — builds a sub-tree for a small range (<=32 primitives)
    // -------------------------------------------------------------------------
    static void handleLeafGroup(
        hkcdSimdTreeGeneration::TreeAssembler& assembler,
        const hkcdSimdTreeGeneration::Interval& base,
        classes::HavokClasses::hkcdSimdTree__Node* allNodes,
        classes::HavokClasses::hkcdSimdTree__Node*& freeSlot)
    {
        if (assembler.mSahLeafSort)
        {
            recursiveSahOrder(assembler, base);
        }
        else
        {
            BoxAccumulator acc;
            acc.reset();
            auto* begin = assembler.mPrimitives.data() + base.mStart;
            auto* end = begin + base.mCount;
            for (auto* p = begin; p != end; ++p)
                acc.expand(p->rawSortable());

            reorderByAxis(assembler, base, longestAxisSplit(acc.toBox()).mAxis);
        }

        hkcdSimdTreeGeneration::Interval quads[4];
        hkcdSimdTreeGeneration::Interval cursor = base;
        int leftover = 1;

        do
        {
            int numQuads = 0;
            while (cursor.mCount > 4 && numQuads < 3)
            {
                quads[numQuads].mStart = cursor.mStart;
                quads[numQuads].mCount = 4;
                ++numQuads;
                cursor.mStart += 4;
                cursor.mCount -= 4;
            }

            if (cursor.mCount > 0)
            {
                quads[numQuads].mStart = cursor.mStart;
                quads[numQuads].mCount = cursor.mCount;
                ++numQuads;
            }

            leftover = 0;
            linkChildNodes(assembler, quads, numQuads, allNodes,
                &allNodes[cursor.mNodeIdx], freeSlot, &cursor, leftover);

            assert(leftover <= 1 && "Internal error in leaf group");
        } while (leftover);
    }

    // -------------------------------------------------------------------------
    // handleBranchGroup — splits a large range (>32) into four sub-ranges
    // -------------------------------------------------------------------------
    static void handleBranchGroup(
        hkcdSimdTreeGeneration::TreeAssembler& assembler,
        const hkcdSimdTreeGeneration::Interval& interval,
        hkcdSimdTreeGeneration::Interval* outQuad)
    {
        assert(interval.mCount > 32 && "Range too small for branch split");
        assembler.subdivideIntoFour(interval, outQuad);
    }

    // -------------------------------------------------------------------------
    // buildTree — main recursive dispatcher
    // -------------------------------------------------------------------------
    static void buildTree(
        hkcdSimdTreeGeneration::TreeAssembler& assembler,
        const hkcdSimdTreeGeneration::Interval& root,
        classes::HavokClasses::hkcdSimdTree__Node* allNodes,
        classes::HavokClasses::hkcdSimdTree__Node*& freeSlot)
    {
        hkcdSimdTreeGeneration::Interval stack[256];
        int top = 1;
        stack[0] = root;

        if (root.mCount > 4)
        {
            do
            {
                hkcdSimdTreeGeneration::Interval current = stack[--top];

                if (current.mCount <= 32)
                {
                    handleLeafGroup(assembler, current, allNodes, freeSlot);
                }
                else
                {
                    hkcdSimdTreeGeneration::Interval quads[4];
                    handleBranchGroup(assembler, current, quads);
                    linkChildNodes(assembler, quads, 4, allNodes,
                        &allNodes[current.mNodeIdx], freeSlot, stack, top);
                }
            } while (top > 0);
        }
        else
        {
            linkChildNodes(assembler, stack, 1, allNodes,
                &allNodes[root.mNodeIdx], freeSlot, stack, top);
        }
    }

    // -------------------------------------------------------------------------
    // sahSplit — Surface Area Heuristic partition (public)
    // -------------------------------------------------------------------------
    void sahSplit(
        hkcdSimdTreeGeneration::TreeAssembler& assembler,
        const hkcdSimdTreeGeneration::Interval& interval,
        hkcdSimdTreeGeneration::Interval* outLeft,
        hkcdSimdTreeGeneration::Interval* outRight,
        int minPartitionSize)
    {
        const int N = interval.mCount;
        std::vector<float> cumCost(N);
        std::vector<hkcdSimdTreeGeneration::Primitive> sorted[3];

        int   bestAxis = -1;
        int   bestPivot = -1;
        float bestScore = std::numeric_limits<float>::max();

        for (int axis = 0; axis < 3; ++axis)
        {
            sorted[axis].insert(sorted[axis].end(),
                assembler.mPrimitives.data() + interval.mStart,
                assembler.mPrimitives.data() + interval.mStart + N);

            hkcdSimdTreeGeneration::Primitive* pts = sorted[axis].data();

            std::sort(sorted[axis].begin(), sorted[axis].end(),
                [axis](const hkcdSimdTreeGeneration::Primitive& a, const hkcdSimdTreeGeneration::Primitive& b)
                { return a(axis) < b(axis); });

            // Forward sweep: accumulate left surface area
            hkAabb sweep;
            sweep.SetEmpty();
            for (int i = 0; i < N; ++i)
            {
                hkAabb leaf;
                assembler.mBoundsSource->fetchLeafBox(pts[i].fetchPayload(), &leaf);
                sweep.IncludeAabb(leaf);
                cumCost[i] = (i + 1) * sweep.SurfaceArea();
            }

            // Backward sweep: combine right SA with stored left SA
            sweep.SetEmpty();
            for (int i = N - 1, right = 1; i > 0; --i, ++right)
            {
                hkAabb leaf;
                assembler.mBoundsSource->fetchLeafBox(pts[i].fetchPayload(), &leaf);
                sweep.IncludeAabb(leaf);
                const float totalScore = cumCost[i - 1] + right * sweep.SurfaceArea();
                if (totalScore < bestScore)
                {
                    bestPivot = i;
                    bestAxis = axis;
                    bestScore = totalScore;
                }
            }
        }

        interval.bisect(bestPivot, outLeft, outRight);

        if (outLeft->mCount < minPartitionSize || outRight->mCount < minPartitionSize)
            interval.bisect(N / 2, outLeft, outRight);

        std::copy(sorted[bestAxis].begin(), sorted[bestAxis].begin() + N,
            assembler.mPrimitives.begin() + interval.mStart);
    }

    // -------------------------------------------------------------------------
    // construct — entry point (public)
    // -------------------------------------------------------------------------
    void construct(
        hkcdSimdTreeGeneration::TreeAssembler& assembler,
        classes::HavokClasses::hkcdSimdTree& outTree,
        IBoundsUpdater* updater)
    {
        assembler.mBoundsSource = updater;
        updater->mNodeBoxes = nullptr;

        hkAabb previousDomain;
        outTree.GetDomain(&previousDomain);

        outTree.Clear();

        const int numPrims = (int)assembler.mPrimitives.size();
        if (numPrims == 0) return;

        // Pad with four sentinel zeros required by the build algorithm
        for (int i = 0; i < 4; ++i)
            assembler.mPrimitives.emplace_back(hkcdSimdTreeGeneration::Primitive::makeZero());

        outTree.mIsCompact = true;

        const int capacity = (numPrims / 2 + 64 + 3) & (~3);
        const size_t prevSize = outTree.mNodes.mElements.size();
        outTree.mNodes.mElements.resize(prevSize + capacity);

        classes::HavokClasses::hkcdSimdTree__Node* allNodes = &outTree.mNodes.mElements[prevSize];

        assembler.onHierarchyBegin(false, 1, (int)outTree.mNodes.mElements.size());

        hkcdSimdTreeGeneration::Interval rootInterval(0, numPrims, 1);

        if (assembler.mReuseOldBounds && !previousDomain.IsEmpty())
        {
            rootInterval.mBounds = previousDomain;
        }
        else
        {
            BoxAccumulator acc;
            acc.reset();
            for (const auto& prim : assembler.mPrimitives)
                acc.expand(prim.rawSortable());
            rootInterval.mBounds = acc.toBox();
        }

        buildTree(assembler, rootInterval, outTree.mNodes.mElements.data(), allNodes);

        assert(int(allNodes - outTree.mNodes.mElements.data()) <= capacity &&
            "Node count exceeded reserved capacity.");
        outTree.mNodes.mElements.resize(int(allNodes - outTree.mNodes.mElements.data()));

        {
            assembler.mNodeBounds.resize(outTree.mNodes.mElements.size());
            updater->mNodeBoxes = assembler.mNodeBounds.data();
            updater->updateSubtree(outTree.mNodes.mElements.data(), 1,
                (int)outTree.mNodes.mElements.size() - 1, true);
        }

        if (assembler.mShrink)
            outTree.mNodes.mElements.shrink_to_fit();

        if (assembler.mOrderBySize)
            outTree.SortByAabbsSize();

        // Remove the sentinel padding
        assembler.mPrimitives.resize(assembler.mPrimitives.size() - 4);
    }
}