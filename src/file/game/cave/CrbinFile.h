#pragma once

#include <vector>
#include <glm/vec3.hpp>
#include <glm/mat3x4.hpp>
#include <util/BinaryVectorReader.h>
#include <util/Logger.h>

namespace application::file::game::cave
{
	class CrbinFile
	{
	public:
		struct Readable
		{
			virtual void Read(application::util::BinaryVectorReader& Reader) = 0;
		};

		struct Vector3f : public Readable
		{
			glm::vec3 mVec;

			virtual void Read(application::util::BinaryVectorReader& Reader) override;
		};

		struct Mat34 : public Readable
		{
			glm::mat3x4 mMatrix;

			virtual void Read(application::util::BinaryVectorReader& Reader) override;
		};

		struct BoundingBox3f : public Readable
		{
			Vector3f mMin;
			Vector3f mMax;

			virtual void Read(application::util::BinaryVectorReader& Reader) override;
		};

		struct ResChunk : public Readable
		{
			uint16_t mX;
			uint16_t mY;
			uint16_t mZ;
			uint16_t mLOD;

			virtual void Read(application::util::BinaryVectorReader& Reader) override;
		};

		struct ResChunkRelation : public Readable
		{
			uint32_t mChildStart;
			uint32_t mChildEnd;

			virtual void Read(application::util::BinaryVectorReader& Reader) override;
		};

		struct ResPageFileDependency : public Readable
		{
			uint16_t mStartIndex;
			uint16_t mEndIndex;

			virtual void Read(application::util::BinaryVectorReader& Reader) override;
		};

		struct Section3Quad : public Readable
		{
			uint32_t mReserve0;
			uint32_t mReserve1;

			virtual void Read(application::util::BinaryVectorReader& Reader) override;
		};

		struct ResChunkBounds : public Readable
		{
			BoundingBox3f mBounds;

			virtual void Read(application::util::BinaryVectorReader& Reader) override;
		};

		struct Section4 : public Readable
		{
			uint8_t mReserve0;

			virtual void Read(application::util::BinaryVectorReader& Reader) override;
		};

		struct ResIndexInfo : public Readable 
		{
			uint32_t mBaseVertexStreamIndex;
			uint16_t mBaseMaterialPaletteIndex;
			uint8_t mVertexStreamCount;
			uint8_t mReserve1;
			uint32_t mReserve2;
			uint32_t mReserve3;

			virtual void Read(application::util::BinaryVectorReader& Reader) override;
		};

		struct Section5 : public Readable 
		{
			uint32_t mReserve0;

			virtual void Read(application::util::BinaryVectorReader& Reader) override;
		};

		struct ResIgnoreChunks : public Readable 
		{
			uint32_t mMask;

			virtual void Read(application::util::BinaryVectorReader& Reader) override;
		};

		struct ResDataStreamInfo : public Readable 
		{
			uint32_t mBaseIndex; // first index is at index_buffer + base_index * stride
			uint32_t mTriangleCount; // index count / 3
			uint16_t mFlags;
			uint16_t mPageFileIndex;

			virtual void Read(application::util::BinaryVectorReader& Reader) override;
		};

		struct ResMaterialPalette : public Readable 
		{
			Vector3f mUBias; // u = (pos.x * u_bias.x + pos.y * u_bias.y + pos.z * u_bias.z) * uv_scale
			float mArrayLayer; // material texture is an array texture, this is just the index
			Vector3f mVBias; // v = (pos.x * v_bias.x + pos.y * v_bias.y + pos.z * v_bias.z) * uv_scale
			float mUVScale;

			virtual void Read(application::util::BinaryVectorReader& Reader) override;
		};

		struct Section8Quad : public Readable 
		{
			uint8_t mReserve[6];

			virtual void Read(application::util::BinaryVectorReader& Reader) override;
		};

		struct ResPageFile : public Readable 
		{
			uint32_t mFileSize; // decompressed size
			uint32_t mFileIndex;

			virtual void Read(application::util::BinaryVectorReader& Reader) override;
		};

		template <typename T>
		struct ResOffset : public Readable
		{
			uint32_t mOffset;
			uint32_t mCount;

			std::vector<T> mEntries;

			virtual void Read(application::util::BinaryVectorReader& Reader) override
			{
				mOffset = Reader.ReadUInt32();
				mCount = Reader.ReadUInt32();

				uint64_t Jumpback = Reader.GetPosition();

				Reader.Seek(mOffset, application::util::BinaryVectorReader::Position::Begin);

				mEntries.resize(mCount);
				for (size_t i = 0; i < mCount; i++)
				{
					if constexpr (std::is_base_of<Readable, T>::value)
					{
						Readable* BaseObj = dynamic_cast<Readable*>(&mEntries[i]);
						if (BaseObj)
						{
							BaseObj->Read(Reader);
						}
						else
						{
							application::util::Logger::Error("CrbinFile", "Cannot cast T* to Readable*");
						}
					}
					else
					{
						Reader.ReadStruct(&mEntries[i], sizeof(T));
					}
				}

				Reader.Seek(Jumpback, application::util::BinaryVectorReader::Position::Begin);
			}
		};

		struct ResChunkMeshData : public Readable
		{
			ResOffset<ResChunk> mChunks;
			ResOffset<ResChunkRelation> mChunkRelations;
			ResOffset<ResPageFileDependency> mFileDependencies;
			ResOffset<ResChunkBounds> mChunkBounds;
			ResOffset<ResIndexInfo> mIndexInfo;
			ResOffset<ResIgnoreChunks> mNoBlendChunks; // adjacent chunks to ignore when blending
			ResOffset<ResDataStreamInfo> mStreamInfo;
			ResOffset<ResMaterialPalette> mMaterialPaletteData;
			ResOffset<ResPageFile> mChunkFiles;
			uint32_t mNumRootChunks;
			ResPageFileDependency mRootChunkDependencies;
			Vector3f mBasePos;
			float mMinSidelength;
			uint32_t mSubdivisions;
			BoundingBox3f mBounds; // for the centers of chunks I believe?

			virtual void Read(application::util::BinaryVectorReader& Reader) override;
		};

		struct SectionsQuad : public Readable 
		{
			ResOffset<ResChunk> mChunks;
			ResOffset<ResChunkRelation> mSection2;
			ResOffset<Section3Quad> mSection3;
			ResOffset<Section4> mSection4;
			ResOffset<Section5> mSection5;
			ResOffset<ResIgnoreChunks> mNoBlendChunks;
			ResOffset<ResChunkBounds> mVertexData;
			ResOffset<Section8Quad> mSection8;
			ResOffset<ResPageFile> mQuads;

			virtual void Read(application::util::BinaryVectorReader& Reader) override;
		};

		struct Header : public Readable
		{
			char mMagic[8]; // cave017\x00
			uint32_t mFileSize;
			uint32_t mReserve0; // version?
			uint32_t mCrbinId;
			Mat34 mTransform;
			ResOffset<uint8_t> mReserve14[4];
			uint32_t mReserve15;
			ResChunkMeshData mChunkMesh;
			uint32_t mReserve29[4];
			SectionsQuad mQuadSections;
			float mReserve31[12]; // probably belongs to the quad mesh
			uint8_t mReserve32[112];

			virtual void Read(application::util::BinaryVectorReader& Reader) override;
		};

		struct ChunkFile
		{
			std::vector<unsigned char> mData;
		};

		Header mHeader;
		std::vector<ChunkFile> mChunkFiles;

		void Initialize(std::vector<unsigned char> Data);
		void LoadChunkFiles(const std::string& AbsoluteDir);

		CrbinFile() = default;
		CrbinFile(std::vector<unsigned char> Data);
	};
}