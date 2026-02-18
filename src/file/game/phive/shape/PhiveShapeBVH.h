#pragma once

#include <limits>
#include <array>
#include <memory>
#include <vector>
#include <unordered_set>
#include <file/game/phive/classes/HavokClasses.h>
#include <file/game/phive/shape/PhiveShapeUtil.h>

/*

THIS IS AN OUTDATED IMPLEMENTATION

*/

namespace application::file::game::phive::shape
{
    namespace PhiveShapeBVH
    {
        struct BoundingBox 
        {
            float mMin[3] = { std::numeric_limits<float>::max(),
                            std::numeric_limits<float>::max(),
                            std::numeric_limits<float>::max() };
            float mMax[3] = { std::numeric_limits<float>::lowest(),
                            std::numeric_limits<float>::lowest(),
                            std::numeric_limits<float>::lowest() };

            BoundingBox(float minX, float minY, float minZ, float maxX, float maxY, float maxZ);
            BoundingBox() = default;

            // Expand this bounding box to include another
            void Expand(const BoundingBox& other);

            // Get center point
            void GetCenter(float center[3]) const;

            float SurfaceArea() const;

            // Check if bounding box is valid
            bool IsValid() const;
        };

        struct Primitive
        {
            BoundingBox mBBox;
            int mId;

            Primitive(int PrimitiveId, const BoundingBox& BoundingBox) : mId(PrimitiveId), mBBox(BoundingBox) {}
        };

        struct BVHNode
        {
            std::array<BoundingBox, 4> mChildBounds;  // Four bounding boxes for children/leaves
            std::array<std::unique_ptr<BVHNode>, 4> mChildren;  // Four child nodes (nullptr if leaf)
            std::array<int, 4> mPrimitiveIndices;  // Four primitive indices (only used in leaf nodes)
            bool mIsLeaf;

            uint32_t mPrimitiveCount = 0;
            uint32_t mUniqueIndicesCount = 0;
            bool mIsSectionHead = false;
            uint16_t _mIndex = 0;

            BVHNode() : mIsLeaf(false)
            {
                mPrimitiveIndices.fill(-1);
            }

            uint32_t ComputePrimitiveCounts();
            std::unordered_set<uint32_t> ComputeUniqueIndicesCounts(const std::vector<uint32_t>& Indices);
            void AttemptSectionSplit();
        };

        class BVH
        {
        private:
            std::unique_ptr<BVHNode> mRoot;
            std::vector<Primitive> mPrimitives;

            // Helper function to compute bounding box for a range of primitives
            BoundingBox ComputeBounds(const std::vector<int>& Indices, int Start, int Count);

            // Partition primitives into 4 groups based on their centroids
            void PartitionPrimitives(std::vector<int>& Indices, int Start, int Count,
                std::array<int, 4>& ChildCounts, std::array<int, 4>& ChildStarts);

            // Recursive function to build the BVH
            std::unique_ptr<BVHNode> BuildRecursive(std::vector<int>& Indices, int Start, int Count);

        public:
            // Build the BVH from a vector of primitives
            void Build(const std::vector<Primitive>& InputPrimitives);

            // Get the root node
            BVHNode* GetRoot() const;

            application::file::game::phive::classes::HavokClasses::hkRelArrayView<application::file::game::phive::classes::HavokClasses::hknpAabb8TreeNode, int> BuildAabb8Tree(BVHNode* Node, application::file::game::phive::shape::PhiveShapeUtil::GeometrySectionConversionUtil& Util);
            application::file::game::phive::classes::HavokClasses::hkcdSimdTree BuildHkcdSimdTree(BVHNode* Node, bool ForceNonLeafRoot);

            // Helper function to print the tree structure (for debugging)
            void PrintTree(const BVHNode* node = nullptr, int depth = 0) const;
        };
    }
}