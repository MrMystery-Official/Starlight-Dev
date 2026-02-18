#include "PhiveShapeBVH.h"

#include <algorithm>
#include <iostream>
#include <string>
#include <map>
#include <util/General.h>
#include <glm/vec3.hpp>
#include <file/game/phive/shape/PhiveShapeUtil.h>

/*

THIS IS AN OUTDATED IMPLEMENTATION

*/

namespace application::file::game::phive::shape
{
    namespace PhiveShapeBVH
    {
        BoundingBox::BoundingBox(float minX, float minY, float minZ, float maxX, float maxY, float maxZ)
        {
            mMin[0] = minX; mMin[1] = minY; mMin[2] = minZ;
            mMax[0] = maxX; mMax[1] = maxY; mMax[2] = maxZ;
        }

        void BoundingBox::Expand(const BoundingBox& other)
        {
            for (int i = 0; i < 3; i++)
            {
                mMin[i] = std::min(mMin[i], other.mMin[i]);
                mMax[i] = std::max(mMax[i], other.mMax[i]);
            }
        }

        void BoundingBox::GetCenter(float center[3]) const
        {
            for (int i = 0; i < 3; i++)
            {
                center[i] = (mMin[i] + mMax[i]) * 0.5f;
            }
        }

        float BoundingBox::SurfaceArea() const 
        {
            if (!IsValid()) return 0.0f;
            float dx = mMax[0] - mMin[0];
            float dy = mMax[1] - mMin[1];
            float dz = mMax[2] - mMin[2];
            return 2.0f * (dx * dy + dy * dz + dx * dz);
        }

        bool BoundingBox::IsValid() const
        {
            return mMin[0] <= mMax[0] && mMin[1] <= mMax[1] && mMin[2] <= mMax[2];
        }

        uint32_t BVHNode::ComputePrimitiveCounts()
        {
            if (mIsLeaf)
            {
                for (int i = 0; i < 4; ++i)
                {
                    if (mPrimitiveIndices[i] != -1)
                    {
                        mPrimitiveCount++;
                    }
                }

                return mPrimitiveCount;
            }

            for (int i = 0; i < 4; i++)
            {
                if (mChildren[i])
                {
                    mPrimitiveCount += mChildren[i]->ComputePrimitiveCounts();
                }
            }

            return mPrimitiveCount;
        }

        std::unordered_set<uint32_t> BVHNode::ComputeUniqueIndicesCounts(const std::vector<uint32_t>& Indices)
        {
            if (mIsLeaf)
            {
                std::unordered_set<uint32_t> s;

                for (int i = 0; i < 4; ++i)
                {
                    if (mPrimitiveIndices[i] != -1)
                    {
                        s.insert(Indices[mPrimitiveIndices[i] * 4]);
                        s.insert(Indices[mPrimitiveIndices[i] * 4 + 1]);
                        s.insert(Indices[mPrimitiveIndices[i] * 4 + 2]);
                        s.insert(Indices[mPrimitiveIndices[i] * 4 + 3]);
                        mUniqueIndicesCount += s.size();
                    }
                }
                return s;
            }

            std::unordered_set<uint32_t> Root;
            for (int i = 0; i < 4; i++)
            {
                if (mChildren[i])
                {
                    std::unordered_set<uint32_t> Child = mChildren[i]->ComputeUniqueIndicesCounts(Indices);
                    Root.merge(Child);
                }
            }


            mUniqueIndicesCount = Root.size();
            return Root;
        }

        void BVHNode::AttemptSectionSplit()
        {
            if (mIsLeaf || mPrimitiveCount <= 255 && mUniqueIndicesCount <= 255) return;
            mIsSectionHead = false;

            for (int i = 0; i < 4; i++)
            {
                if (mChildren[i])
                {
                    mChildren[i]->mIsSectionHead = true;
                }
            }

            for (int i = 0; i < 4; i++)
            {
                if (mChildren[i])
                {
                    mChildren[i]->AttemptSectionSplit();
                }
            }
        }

        BoundingBox BVH::ComputeBounds(const std::vector<int>& Indices, int Start, int Count)
        {
            BoundingBox Bounds;
            for (int i = Start; i < Start + Count; ++i)
            {
                if (i < Indices.size())
                {
                    Bounds.Expand(mPrimitives[Indices[i]].mBBox);
                }
            }
            return Bounds;
        }

        void BVH::PartitionPrimitives(std::vector<int>& Indices, int Start, int Count,
            std::array<int, 4>& ChildCounts, std::array<int, 4>& ChildStarts)
        {
            const int MinGroupSize = 2;

            // Initialize all counts to 0
            ChildCounts.fill(0);

            // Find the axis with the largest extent
            BoundingBox Bounds = ComputeBounds(Indices, Start, Count);
            int Axis = 0;
            float MaxExtent = Bounds.mMax[0] - Bounds.mMin[0];
            for (int i = 1; i < 3; ++i) {
                float Extent = Bounds.mMax[i] - Bounds.mMin[i];
                if (Extent > MaxExtent) {
                    MaxExtent = Extent;
                    Axis = i;
                }
            }

            // Sort primitives along the chosen axis
            std::sort(Indices.begin() + Start, Indices.begin() + Start + Count,
                [this, Axis](int A, int B) {
                    float CenterA[3], CenterB[3];
                    mPrimitives[A].mBBox.GetCenter(CenterA);
                    mPrimitives[B].mBBox.GetCenter(CenterB);
                    return CenterA[Axis] < CenterB[Axis];
                });

            // Strategy: Distribute ALL primitives ensuring minimum group size
            int Remaining = Count;

            // Determine how many children we can actually use
            int NumChildren = std::min(4, Count / MinGroupSize);
            if (NumChildren == 0) NumChildren = 1; // At least one child

            // First pass: give each active child at least MinGroupSize primitives
            for (int i = 0; i < NumChildren; ++i) {
                int assignCount = std::min(MinGroupSize, Remaining);
                ChildCounts[i] = assignCount;
                Remaining -= assignCount;
            }

            // Second pass: distribute remaining primitives evenly
            int ChildIndex = 0;
            while (Remaining > 0) {
                ChildCounts[ChildIndex % NumChildren]++;
                Remaining--;
                ChildIndex++;
            }

            // Calculate start positions
            ChildStarts[0] = Start;
            for (int i = 1; i < 4; ++i) {
                ChildStarts[i] = ChildStarts[i - 1] + ChildCounts[i - 1];
            }

            // Verification: ensure we've assigned all primitives
            int TotalAssigned = 0;
            for (int i = 0; i < 4; ++i) {
                TotalAssigned += ChildCounts[i];
            }

            if (TotalAssigned != Count)
            {
                std::cout << "All primitives must be assigned\n";
            }
        }

        std::unique_ptr<BVHNode> BVH::BuildRecursive(std::vector<int>& Indices, int Start, int Count)
        {
            auto node = std::make_unique<BVHNode>();

            // Determine if this should be a leaf node
            // A node becomes a leaf if:
            // 1. We have 4 or fewer primitives, OR
            // 2. Subdividing would create groups smaller than minimum size (2)
            bool shouldBeLeaf = (Count <= 4);

            if (shouldBeLeaf) {
                // Create leaf node - store ALL primitives (up to 4)
                node->mIsLeaf = true;
                for (int i = 0; i < 4; ++i) {
                    if (i < Count) {
                        int primIndex = Indices[Start + i];
                        node->mPrimitiveIndices[i] = primIndex;
                        node->mChildBounds[i] = mPrimitives[primIndex].mBBox;

                    }
                    else
                    {
                        node->mPrimitiveIndices[i] = -1;
                    }
                }
                return node;
            }

            // Internal node: partition primitives into 4 groups
            node->mIsLeaf = false;
            std::array<int, 4> childCounts;
            std::array<int, 4> childStarts;

            PartitionPrimitives(Indices, Start, Count, childCounts, childStarts);

            // Recursively build children and compute their bounding boxes
            for (int i = 0; i < 4; ++i) {
                if (childCounts[i] > 0) {
                    node->mChildren[i] = BuildRecursive(Indices, childStarts[i], childCounts[i]);
                    node->mChildBounds[i] = ComputeBounds(Indices, childStarts[i], childCounts[i]);
                }
                // If childCounts[i] == 0, child remains nullptr and bounds remain invalid
            }

            return node;
        }

        void BVH::Build(const std::vector<Primitive>& InputPrimitives)
        {
            mPrimitives = InputPrimitives;

            if (mPrimitives.empty())
            {
                mRoot = nullptr;
                return;
            }

            // Create indices array
            std::vector<int> Indices(mPrimitives.size());
            for (size_t i = 0; i < mPrimitives.size(); i++)
            {
                Indices[i] = static_cast<int>(i);
            }

            // Build the tree
            mRoot = BuildRecursive(Indices, 0, static_cast<int>(mPrimitives.size()));
        }

        BVHNode* BVH::GetRoot() const
        {
            return mRoot.get();
        }

        void BVH::PrintTree(const BVHNode* node, int depth) const
        {
            if (node == nullptr)
            {
                node = mRoot.get();
            }

            if (node == nullptr) return;

            std::string indent(depth * 2, ' ');

            if (node->mIsLeaf)
            {
                std::cout << indent << "Leaf: primitives [";
                for (int i = 0; i < 4; ++i)
                {
                    if (node->mPrimitiveIndices[i] != -1)
                    {
                        std::cout << node->mPrimitiveIndices[i];
                        if (i < 3 && node->mPrimitiveIndices[i + 1] != -1) std::cout << ", ";
                    }
                }
                std::cout << "], IsSectionHead: " << node->mIsSectionHead << std::endl;
            }
            else {
                std::cout << indent << "Internal node, IsSectionHead: " << node->mIsSectionHead << ":\n";
                for (int i = 0; i < 4; ++i)
                {
                    if (node->mChildren[i])
                    {
                        std::cout << indent << "  Child " << i << ":\n";
                        PrintTree(node->mChildren[i].get(), depth + 2);
                    }
                }
            }
        }

        template <int value>
        glm::u8vec3 convertVector(const glm::vec3& vectorF, const glm::vec3& bitScale8, const glm::vec3& bitScale8Inv, const glm::ivec3& bitOffset8)
        {
            auto RoundVector = [](const glm::vec3& v) -> glm::vec3 {
                if constexpr (value == -1) {
                    return glm::floor(v);
                }
                else {
                    return glm::ceil(v);
                }
                };

            glm::i32vec3 CompressedVector = RoundVector(bitScale8 * vectorF);
            glm::i32vec3 RoundedCompressed = CompressedVector;
            CompressedVector -= bitOffset8;

            {
                glm::vec3 TmpRestoredVector = RoundedCompressed;
                TmpRestoredVector *= bitScale8Inv;

                glm::i32vec3 IntSpaceFixUp(0);
                glm::bvec3 Breached;
                if constexpr (value == -1) {
                    Breached = glm::greaterThan(TmpRestoredVector, vectorF);
                }
                else {
                    Breached = glm::lessThan(TmpRestoredVector, vectorF);
                }

                for (int i = 0; i < 3; ++i) 
                {
                    if (Breached[i])
                    {
                        IntSpaceFixUp[i] = value;
                    }
                }

                CompressedVector += IntSpaceFixUp;
            }

            glm::u8vec3 Result;
            for (int i = 0; i < 3; ++i) 
            {
                Result[i] = static_cast<int8_t>(CompressedVector[i]);
            }
            return Result;
        }

        std::pair<glm::u8vec3, glm::u8vec3> ConvertAabb(const glm::vec3& Min, const glm::vec3& Max, application::file::game::phive::shape::PhiveShapeUtil::GeometrySectionConversionUtil& Util)
        {
            std::pair<glm::u8vec3, glm::u8vec3> aabbOut;
            aabbOut.first = convertVector<-1>(Min, Util.mBitScale, Util.mBitScaleInv, Util.mBitOffset);
            aabbOut.second = convertVector<1>(Max, Util.mBitScale, Util.mBitScaleInv, Util.mBitOffset);
            return aabbOut;
        }

        application::file::game::phive::classes::HavokClasses::hkRelArrayView<application::file::game::phive::classes::HavokClasses::hknpAabb8TreeNode, int> BVH::BuildAabb8Tree(BVHNode* Node, application::file::game::phive::shape::PhiveShapeUtil::GeometrySectionConversionUtil& Util)
        {
            application::file::game::phive::classes::HavokClasses::hkRelArrayView<application::file::game::phive::classes::HavokClasses::hknpAabb8TreeNode, int> Result;

            glm::vec3 Min(std::numeric_limits<float>::max());
            glm::vec3 Max(std::numeric_limits<float>::lowest());

            for (int i = 0; i < 4; i++) 
            {
                if (Node->mChildBounds[i].IsValid())
                {
                    Min.x = std::min(Min.x, Node->mChildBounds[i].mMin[0]);
                    Min.y = std::min(Min.y, Node->mChildBounds[i].mMin[1]);
                    Min.z = std::min(Min.z, Node->mChildBounds[i].mMin[2]);

                    Max.x = std::max(Max.x, Node->mChildBounds[i].mMax[0]);
                    Max.y = std::max(Max.y, Node->mChildBounds[i].mMax[1]);
                    Max.z = std::max(Max.z, Node->mChildBounds[i].mMax[2]);
                }
            }

            uint16_t IndexCounter = 0;
            std::function<void(BVHNode* Child)> SetupIndices = [&SetupIndices, &IndexCounter](BVHNode* Child)
                {
                    Child->_mIndex = IndexCounter++;

                    for (int i = 0; i < 4; i++)
                    {
                        if (Child->mChildren[i])
                        {
                            SetupIndices(Child->mChildren[i].get());
                        }
                    }
                };
            SetupIndices(Node);
            Result.mElements.resize(IndexCounter);

            std::function<void(BVHNode* Child)> ConstructTree = [&ConstructTree, &Result, &Min, &Max, &Util](BVHNode* Child)
                {
                    std::vector<std::pair<int16_t, uint8_t>> Ordered;
                    for (int i = 0; i < 4; i++)
                    {
                        if (Child->mChildBounds[i].IsValid())
                        {
                            if (Child->mIsLeaf)
                            {
                                Ordered.push_back({ Child->mPrimitiveIndices[i], i });
                            }
                            else
                            {
                                Ordered.push_back({ Child->mChildren[i]->_mIndex, i });
                            }
                        }
                        else
                        {
                            Ordered.push_back({ -1000, i });
                        }
                    }

                    std::sort(Ordered.begin(), Ordered.end(),
                        [](const std::pair<int16_t, uint8_t>& a, const std::pair<int16_t, uint8_t>& b) {
                            return a.first < b.first; // sort by the first element (key)
                        });

                    for (auto& [Key, Index] : Ordered)
                    {
                        if (Key == -1000)
                            Key = 0;
                    }

                    if (Child->mIsLeaf)
                    {
                        std::pair<int16_t, uint8_t> Temp = Ordered[2];
                        Ordered[2] = Ordered[3];
                        Ordered[3] = Temp;
                    }
                    

                    application::file::game::phive::classes::HavokClasses::hknpAabb8TreeNode& TreeNode = Result.mElements[Child->_mIndex];
                    for (int i = 0; i < 4; i++)
                    {
                        if (Child->mChildBounds[Ordered[i].second].IsValid())
                        {
                            std::pair<glm::u8vec3, glm::u8vec3> CompressedAabb = ConvertAabb(glm::vec3(Child->mChildBounds[Ordered[i].second].mMin[0], Child->mChildBounds[Ordered[i].second].mMin[1], Child->mChildBounds[Ordered[i].second].mMin[2]), glm::vec3(Child->mChildBounds[Ordered[i].second].mMax[0], Child->mChildBounds[Ordered[i].second].mMax[1], Child->mChildBounds[Ordered[i].second].mMax[2]), Util);

                            //What a silly apprach... why should it be that simply? I mean, its a combination of Havok and Phive, how could something be SiMpLe???? (im slowly but surely going insane)
                            /*
                            TreeNode.mParent.GetLowX()[i] = std::max((uint8_t)1, (uint8_t)(std::floor((Child->mChildBounds[Ordered[i].second].mMin[0] - Min.x) / (Max.x - Min.x) * 255.0f)));
                            TreeNode.mParent.GetLowY()[i] = std::max((uint8_t)1, (uint8_t)(std::floor((Child->mChildBounds[Ordered[i].second].mMin[1] - Min.y) / (Max.y - Min.y) * 255.0f)));
                            TreeNode.mParent.GetLowZ()[i] = std::max((uint8_t)1, (uint8_t)(std::floor((Child->mChildBounds[Ordered[i].second].mMin[2] - Min.z) / (Max.z - Min.z) * 255.0f)));

                            TreeNode.mParent.GetHighX()[i] = std::max((uint8_t)1, (uint8_t)(std::ceil((Child->mChildBounds[Ordered[i].second].mMax[0] - Min.x) / (Max.x - Min.x) * 255.0f)));
                            TreeNode.mParent.GetHighY()[i] = std::max((uint8_t)1, (uint8_t)(std::ceil((Child->mChildBounds[Ordered[i].second].mMax[1] - Min.y) / (Max.y - Min.y) * 255.0f)));
                            TreeNode.mParent.GetHighZ()[i] = std::max((uint8_t)1, (uint8_t)(std::ceil((Child->mChildBounds[Ordered[i].second].mMax[2] - Min.z) / (Max.z - Min.z) * 255.0f)));
                            */

                            TreeNode.mParent.GetLowX()[i] = CompressedAabb.first.x;
                            TreeNode.mParent.GetLowY()[i] = CompressedAabb.first.y;
                            TreeNode.mParent.GetLowZ()[i] = CompressedAabb.first.z;

                            TreeNode.mParent.GetHighX()[i] = CompressedAabb.second.x;
                            TreeNode.mParent.GetHighY()[i] = CompressedAabb.second.y;
                            TreeNode.mParent.GetHighZ()[i] = CompressedAabb.second.z;

                            if (Child->mIsLeaf)
                            {
                                TreeNode.mData[i].mParent = Child->mPrimitiveIndices[Ordered[i].second];
                            }
                            else
                            {
                                TreeNode.mData[i].mParent = Child->mChildren[Ordered[i].second]->_mIndex;
                                ConstructTree(Child->mChildren[Ordered[i].second].get());
                            }
                        }
                        else
                        {
                            TreeNode.mParent.GetLowX()[i] = 0xFF;
                            TreeNode.mParent.GetLowY()[i] = 0xFF;
                            TreeNode.mParent.GetLowZ()[i] = 0xFF;

                            TreeNode.mParent.GetHighX()[i] = 0;
                            TreeNode.mParent.GetHighY()[i] = 0;
                            TreeNode.mParent.GetHighZ()[i] = 0;

                            TreeNode.mData[i].mParent = 0;
                        }
                    }
                };
            ConstructTree(Node);

            return Result;
        }

        application::file::game::phive::classes::HavokClasses::hkcdSimdTree BVH::BuildHkcdSimdTree(BVHNode* Node, bool ForceNonLeafRoot)
        {
            application::file::game::phive::classes::HavokClasses::hkcdSimdTree Tree;
            Tree.mIsCompact = true;

            uint16_t IndexCounter = 0;
            std::function<void(BVHNode* Child)> SetupIndices = [&SetupIndices, &IndexCounter](BVHNode* Child)
                {
                    Child->_mIndex = IndexCounter++;

                    for (int i = 0; i < 4; i++)
                    {
                        if (Child->mChildren[i])
                        {
                            SetupIndices(Child->mChildren[i].get());
                        }
                    }
                };
            SetupIndices(Node);
            Tree.mNodes.mElements.resize(IndexCounter + 1);

            const uint32_t DefaultNum = 0x7F7FFFEE;
            const float DefaultFloat = *((float*)&DefaultNum);

            const uint32_t DefaultNum2 = 0xFF7FFFEE;
            const float DefaultFloat2 = *((float*)&DefaultNum2);

            std::function<void(BVHNode* Child)> ConstructTree = [&ConstructTree, &Tree, &DefaultFloat, &DefaultFloat2](BVHNode* Child)
                {
                    application::file::game::phive::classes::HavokClasses::hkcdSimdTree__Node& Node = Tree.mNodes.mElements[Child->_mIndex + 1];
                    Node.mIsActive = false;
                    Node.mIsLeaf = Child->mIsLeaf;
                    for (int i = 0; i < 4; i++)
                    {
                        if (Child->mChildBounds[i].IsValid())
                        {
                            Node.mParent.mLx.mParent.mData[i] = Child->mChildBounds[i].mMin[0];
                            Node.mParent.mHx.mParent.mData[i] = Child->mChildBounds[i].mMax[0];
                            Node.mParent.mLy.mParent.mData[i] = Child->mChildBounds[i].mMin[1];
                            Node.mParent.mHy.mParent.mData[i] = Child->mChildBounds[i].mMax[1];
                            Node.mParent.mLz.mParent.mData[i] = Child->mChildBounds[i].mMin[2];
                            Node.mParent.mHz.mParent.mData[i] = Child->mChildBounds[i].mMax[2];

                            if (Child->mIsLeaf)
                            {
                                Node.mData[i].mParent = Child->mPrimitiveIndices[i];
                            }
                            else
                            {
                                Node.mData[i].mParent = Child->mChildren[i]->_mIndex + 1;
                                ConstructTree(Child->mChildren[i].get());
                            }
                        }
                        else
                        {
                            Node.mParent.mLx.mParent.mData[i] = DefaultFloat;
                            Node.mParent.mHx.mParent.mData[i] = DefaultFloat2;
                            Node.mParent.mLy.mParent.mData[i] = DefaultFloat;
                            Node.mParent.mHy.mParent.mData[i] = DefaultFloat2;
                            Node.mParent.mLz.mParent.mData[i] = DefaultFloat;
                            Node.mParent.mHz.mParent.mData[i] = DefaultFloat2;

                            Node.mData[i].mParent = Child->mIsLeaf ? 0xFFFFFFFF : 0x00000000;
                        }
                    }
                };


            auto PrepareNode = [&Tree, DefaultFloat, DefaultFloat2](uint32_t Index)
                {
                    Tree.mNodes.mElements[Index].mIsLeaf = false;
                    Tree.mNodes.mElements[Index].mIsActive = false;

                    Tree.mNodes.mElements[Index].mParent.mLx.mParent.mData[0] = DefaultFloat;
                    Tree.mNodes.mElements[Index].mParent.mLx.mParent.mData[1] = DefaultFloat;
                    Tree.mNodes.mElements[Index].mParent.mLx.mParent.mData[2] = DefaultFloat;
                    Tree.mNodes.mElements[Index].mParent.mLx.mParent.mData[3] = DefaultFloat;

                    Tree.mNodes.mElements[Index].mParent.mHx.mParent.mData[0] = DefaultFloat2;
                    Tree.mNodes.mElements[Index].mParent.mHx.mParent.mData[1] = DefaultFloat2;
                    Tree.mNodes.mElements[Index].mParent.mHx.mParent.mData[2] = DefaultFloat2;
                    Tree.mNodes.mElements[Index].mParent.mHx.mParent.mData[3] = DefaultFloat2;


                    Tree.mNodes.mElements[Index].mParent.mLy.mParent.mData[0] = DefaultFloat;
                    Tree.mNodes.mElements[Index].mParent.mLy.mParent.mData[1] = DefaultFloat;
                    Tree.mNodes.mElements[Index].mParent.mLy.mParent.mData[2] = DefaultFloat;
                    Tree.mNodes.mElements[Index].mParent.mLy.mParent.mData[3] = DefaultFloat;

                    Tree.mNodes.mElements[Index].mParent.mHy.mParent.mData[0] = DefaultFloat2;
                    Tree.mNodes.mElements[Index].mParent.mHy.mParent.mData[1] = DefaultFloat2;
                    Tree.mNodes.mElements[Index].mParent.mHy.mParent.mData[2] = DefaultFloat2;
                    Tree.mNodes.mElements[Index].mParent.mHy.mParent.mData[3] = DefaultFloat2;


                    Tree.mNodes.mElements[Index].mParent.mLz.mParent.mData[0] = DefaultFloat;
                    Tree.mNodes.mElements[Index].mParent.mLz.mParent.mData[1] = DefaultFloat;
                    Tree.mNodes.mElements[Index].mParent.mLz.mParent.mData[2] = DefaultFloat;
                    Tree.mNodes.mElements[Index].mParent.mLz.mParent.mData[3] = DefaultFloat;

                    Tree.mNodes.mElements[Index].mParent.mHz.mParent.mData[0] = DefaultFloat2;
                    Tree.mNodes.mElements[Index].mParent.mHz.mParent.mData[1] = DefaultFloat2;
                    Tree.mNodes.mElements[Index].mParent.mHz.mParent.mData[2] = DefaultFloat2;
                    Tree.mNodes.mElements[Index].mParent.mHz.mParent.mData[3] = DefaultFloat2;
                };
            PrepareNode(0); //Default node

            ConstructTree(Node);

            if (Tree.mNodes.mElements[1].mIsLeaf && ForceNonLeafRoot)
            {
                Tree.mNodes.mElements.insert(Tree.mNodes.mElements.begin() + 1, application::file::game::phive::classes::HavokClasses::hkcdSimdTree__Node{});

                PrepareNode(1);

                glm::vec3 MinPoint(std::numeric_limits<float>::max());
                glm::vec3 MaxPoint(std::numeric_limits<float>::lowest());

                for (int i = 0; i < 4; i++)
                {
                    if (!Node->mChildBounds[i].IsValid())
                        continue;

                    MinPoint.x = std::min(MinPoint.x, Node->mChildBounds[i].mMin[0]);
                    MinPoint.y = std::min(MinPoint.y, Node->mChildBounds[i].mMin[1]);
                    MinPoint.z = std::min(MinPoint.z, Node->mChildBounds[i].mMin[2]);

                    MaxPoint.x = std::max(MaxPoint.x, Node->mChildBounds[i].mMax[0]);
                    MaxPoint.y = std::max(MaxPoint.y, Node->mChildBounds[i].mMax[1]);
                    MaxPoint.z = std::max(MaxPoint.z, Node->mChildBounds[i].mMax[2]);
                }

                Tree.mNodes.mElements[1].mParent.mLx.mParent.mData[0] = MinPoint.x;
                Tree.mNodes.mElements[1].mParent.mLy.mParent.mData[0] = MinPoint.y;
                Tree.mNodes.mElements[1].mParent.mLz.mParent.mData[0] = MinPoint.z;

                Tree.mNodes.mElements[1].mParent.mHx.mParent.mData[0] = MaxPoint.x;
                Tree.mNodes.mElements[1].mParent.mHy.mParent.mData[0] = MaxPoint.y;
                Tree.mNodes.mElements[1].mParent.mHz.mParent.mData[0] = MaxPoint.z;

                Tree.mNodes.mElements[1].mData[0].mParent = 2;
            }

            return Tree;
        }
    }
}