#pragma once

#include <vector>
#include <cstdint>
#include <glm/vec4.hpp>
#include <file/game/phive/classes/HavokClasses.h>
#include <file/game/phive/starlight_physics/common/hkGeometry.h>
#include <file/game/phive/starlight_physics/shape/hknpMeshShapeConversionUtils.h>

namespace application::file::game::phive::starlight_physics::shape
{
	class hknpMeshShapeBuilder
	{
	public:
        enum class QuadAreaPolicy
        {
            SkipAreaCheck,
            EnforceAreaCheck
        };

        struct PackedVec3
        {
            uint16_t x;
            uint16_t y;
            uint16_t z;
        };

        struct MeshChunk
        {
            common::hkAabb mSpatialBounds;
            common::hkAabb mUnexpandedBounds;
            glm::i16vec4  mQuantBitOffset;
            glm::u32vec4  mVertexOrigin;
            glm::vec3     mInvBitScale;

            std::vector<classes::HavokClasses::hknpAabb8TreeNode>                                mLocalBvh;
            std::vector<classes::HavokClasses::hknpMeshShape__GeometrySection__Primitive>        mFaces;
            std::vector<classes::HavokClasses::hkFlags<classes::HavokClasses::hknpMeshShapeFlags, uint8_t>> mFaceFlags;
            std::vector<glm::vec3>   mRawVerts;
            std::vector<PackedVec3>  mCompressedVerts;
            std::vector<uint16_t>    mMaterialIds;
            bool mCanQuantize = true;
        };

        static bool isDegenerate(const glm::vec4& a4, const glm::vec4& b4, const glm::vec4& c4);

        bool compile(classes::HavokClasses::hknpMeshShape* dst, const common::hkGeometry& geo);

    private:
        void compressChunkVerts(const hknpMeshShapeConversionUtils::Vertex& packer, MeshChunk& chunk, float maxErr);

        void tagQuadPlanarity(MeshChunk& chunk, int faceIdx, bool isFlat);
        void evaluateAllQuads(MeshChunk& chunk);

        void readFaceVerts(
            const classes::HavokClasses::hknpMeshShape__GeometrySection__Primitive& face,
            const uint8_t* vertBuf,
            const glm::u32vec4& origin,
            const hknpMeshShapeConversionUtils::Vertex& packer,
            glm::vec4* outVerts);

        void readFaceVerts(
            uint32_t faceIdx,
            const uint8_t* vertBuf,
            const classes::HavokClasses::hknpMeshShape__GeometrySection__Primitive* faces,
            const glm::u32vec4& origin,
            const hknpMeshShapeConversionUtils::Vertex& packer,
            glm::vec4* outVerts);

        bool isQuadPlanarAndConvex(
            const glm::vec4& p0,
            const glm::vec4& p1,
            const glm::vec4& p2,
            const glm::vec4& p3,
            float tol,
            float* outErr = nullptr,
            QuadAreaPolicy policy = QuadAreaPolicy::EnforceAreaCheck);

        void pruneDegenerateFaces(
            MeshChunk& chunk,
            int faceIdx,
            const glm::vec4 verts[4],
            bool& outWasModified,
            bool& outShouldDrop,
            int& outDegCount);

        void dropUnreferencedVerts(MeshChunk& chunk);

        void compactLocalBvh(const std::vector<int>& deadNodes, MeshChunk& chunk);

        bool repairChunkAfterQuantization(
            const hknpMeshShapeConversionUtils::Vertex& packer,
            MeshChunk& chunk,
            float coplanarTol,
            bool dropDegenerate,
            int& outDegCount);

        void locateSectionInTree(
            const classes::HavokClasses::hkcdSimdTree& tree,
            int sectionIdx,
            int& outParent,
            int& outSlot);

        void evictSection(
            std::vector<MeshChunk>& chunks,
            int sectionIdx,
            classes::HavokClasses::hkcdSimdTree& tree);

        void resyncSection(
            const hknpMeshShapeConversionUtils::Vertex& packer,
            MeshChunk& chunk,
            classes::HavokClasses::hkcdSimdTree& tree,
            int sectionIdx);

        void sortFacesByMaterial(MeshChunk& chunk);

        std::vector<classes::HavokClasses::hknpMeshShape__ShapeTagTableEntry>
            buildMaterialTable(const std::vector<MeshChunk>& chunks);

        void countPrimitivesPerNode(
            classes::HavokClasses::hkcdSimdTree& tree,
            std::vector<int>& outCounts);

        void populateChunkFromSubtree(
            const common::hkGeometry& geo,
            uint32_t subtreeRoot,
            const classes::HavokClasses::hkcdSimdTree__Node* nodes,
            std::vector<MeshChunk>& chunks,
            float maxErr);

        void populateChunkFromFaceList(
            const common::hkGeometry& geo,
            const int* faceIds,
            int faceCount,
            std::vector<MeshChunk>& chunks,
            float maxErr,
            float maxExtent);

        void buildTopologyAndTree(
            const common::hkGeometry& geo,
            classes::HavokClasses::hkcdSimdTree& outTree,
            std::vector<MeshChunk>& outChunks,
            float maxExtent,
            int& outDegCount);

        void finalizeShape(
            const hknpMeshShapeConversionUtils::Vertex& packer,
            const classes::HavokClasses::hkcdSimdTree& bvh,
            std::vector<MeshChunk>& chunks,
            const std::vector<classes::HavokClasses::hknpMeshShape__ShapeTagTableEntry>& matTable,
            bool tagInterior,
            int mappingSize,
            int mappingOffset,
            int origFaceCount,
            classes::HavokClasses::hknpMeshShape& dst);

        void buildIntermediateData(
            const common::hkGeometry& geo,
            const hknpMeshShapeConversionUtils::Vertex& packer,
            classes::HavokClasses::hkcdSimdTree& outTree,
            std::vector<MeshChunk>& outChunks,
            std::vector<classes::HavokClasses::hknpMeshShape__ShapeTagTableEntry>& outMatTable,
            int& outDegCount);
	};
}