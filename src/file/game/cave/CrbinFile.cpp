#define ZSTD_STATIC_LINKING_ONLY

#include "CrbinFile.h"

#include <util/BinaryVectorReader.h>
#include <filesystem>
#include <util/FileUtil.h>
#include <zstd.h>

namespace application::file::game::cave
{
	void CrbinFile::Vector3f::Read(application::util::BinaryVectorReader& Reader)
	{
		mVec.x = Reader.ReadFloat();
		mVec.y = Reader.ReadFloat();
		mVec.z = Reader.ReadFloat();
	}

	void CrbinFile::Mat34::Read(application::util::BinaryVectorReader& Reader)
	{
		mMatrix[0].x = Reader.ReadFloat();
		mMatrix[0].y = Reader.ReadFloat();
		mMatrix[0].z = Reader.ReadFloat();
		mMatrix[0].w = Reader.ReadFloat();

		mMatrix[1].x = Reader.ReadFloat();
		mMatrix[1].y = Reader.ReadFloat();
		mMatrix[1].z = Reader.ReadFloat();
		mMatrix[1].w = Reader.ReadFloat();

		mMatrix[2].x = Reader.ReadFloat();
		mMatrix[2].y = Reader.ReadFloat();
		mMatrix[2].z = Reader.ReadFloat();
		mMatrix[2].w = Reader.ReadFloat();
	}

	void CrbinFile::BoundingBox3f::Read(application::util::BinaryVectorReader& Reader)
	{
		mMin.Read(Reader);
		mMax.Read(Reader);
	}

	void CrbinFile::ResChunk::Read(application::util::BinaryVectorReader& Reader)
	{
		mX = Reader.ReadUInt16();
		mY = Reader.ReadUInt16();
		mZ = Reader.ReadUInt16();
		mLOD = Reader.ReadUInt16();
	}

	void CrbinFile::ResChunkRelation::Read(application::util::BinaryVectorReader& Reader)
	{
		mChildStart = Reader.ReadUInt32();
		mChildEnd = Reader.ReadUInt32();
	}

	void CrbinFile::ResPageFileDependency::Read(application::util::BinaryVectorReader& Reader)
	{
		mStartIndex = Reader.ReadUInt16();
		mEndIndex = Reader.ReadUInt16();
	}

	void CrbinFile::Section3Quad::Read(application::util::BinaryVectorReader& Reader)
	{
		mReserve0 = Reader.ReadUInt32();
		mReserve1 = Reader.ReadUInt32();
	}

	void CrbinFile::ResChunkBounds::Read(application::util::BinaryVectorReader& Reader)
	{
		mBounds.Read(Reader);
	}

	void CrbinFile::Section4::Read(application::util::BinaryVectorReader& Reader)
	{
		mReserve0 = Reader.ReadUInt8();
	}

	void CrbinFile::ResIndexInfo::Read(application::util::BinaryVectorReader& Reader)
	{
		mBaseVertexStreamIndex = Reader.ReadUInt32();
		mBaseMaterialPaletteIndex = Reader.ReadUInt16();
		mVertexStreamCount = Reader.ReadUInt8();
		mReserve1 = Reader.ReadUInt8();
		mReserve2 = Reader.ReadUInt32();
		mReserve3 = Reader.ReadUInt32();
	}

	void CrbinFile::Section5::Read(application::util::BinaryVectorReader& Reader)
	{
		mReserve0 = Reader.ReadUInt32();
	}

	void CrbinFile::ResIgnoreChunks::Read(application::util::BinaryVectorReader& Reader)
	{
		mMask = Reader.ReadUInt32();
	}

	void CrbinFile::ResDataStreamInfo::Read(application::util::BinaryVectorReader& Reader)
	{
		mBaseIndex = Reader.ReadUInt32();
		mTriangleCount = Reader.ReadUInt32();
		mFlags = Reader.ReadUInt16();
		mPageFileIndex = Reader.ReadUInt16();
	}

	void CrbinFile::ResMaterialPalette::Read(application::util::BinaryVectorReader& Reader)
	{
		mUBias.Read(Reader);
		mArrayLayer = Reader.ReadFloat();
		mVBias.Read(Reader);
		mUVScale = Reader.ReadFloat();
	}

	void CrbinFile::Section8Quad::Read(application::util::BinaryVectorReader& Reader)
	{
		Reader.ReadStruct(&mReserve[0], 6);
	}

	void CrbinFile::ResPageFile::Read(application::util::BinaryVectorReader& Reader)
	{
		mFileSize = Reader.ReadUInt32();
		mFileIndex = Reader.ReadUInt32();
	}

	void CrbinFile::ResChunkMeshData::Read(application::util::BinaryVectorReader& Reader)
	{
		mChunks.Read(Reader);
		mChunkRelations.Read(Reader);
		mFileDependencies.Read(Reader);
		mChunkBounds.Read(Reader);
		mIndexInfo.Read(Reader);
		mNoBlendChunks.Read(Reader);
		mStreamInfo.Read(Reader);
		mMaterialPaletteData.Read(Reader);
		mChunkFiles.Read(Reader);
		mNumRootChunks = Reader.ReadUInt32();
		mRootChunkDependencies.Read(Reader);
		mBasePos.Read(Reader);
		mMinSidelength = Reader.ReadFloat();
		mSubdivisions = Reader.ReadUInt32();
		mBounds.Read(Reader);
	}

	void CrbinFile::SectionsQuad::Read(application::util::BinaryVectorReader& Reader)
	{
		mChunks.Read(Reader);
		mSection2.Read(Reader);
		mSection3.Read(Reader);
		mSection4.Read(Reader);
		mSection5.Read(Reader);
		mNoBlendChunks.Read(Reader);
		mVertexData.Read(Reader);
		mSection8.Read(Reader);
		mQuads.Read(Reader);
	}

	void CrbinFile::Header::Read(application::util::BinaryVectorReader& Reader)
	{
		Reader.ReadStruct(&mMagic[0], 8);
		mFileSize = Reader.ReadUInt32();
		mReserve0 = Reader.ReadUInt32();
		mCrbinId = Reader.ReadUInt32();
		mTransform.Read(Reader);
		mReserve14[0].Read(Reader);
		mReserve14[1].Read(Reader);
		mReserve14[2].Read(Reader);
		mReserve14[3].Read(Reader);
		mReserve15 = Reader.ReadUInt32();
		mChunkMesh.Read(Reader);
		Reader.ReadStruct(&mReserve29[0], 4 * 4);
		mQuadSections.Read(Reader);
		Reader.ReadStruct(&mReserve31[0], 12 * sizeof(float));
		Reader.ReadStruct(&mReserve32[0], 112);
	}

	void CrbinFile::Initialize(std::vector<unsigned char> Data)
	{
		application::util::BinaryVectorReader Reader(Data);
		mHeader.Read(Reader);
	}

	void CrbinFile::LoadChunkFiles(const std::string& AbsoluteDir)
	{
		mChunkFiles.resize(mHeader.mChunkMesh.mChunkFiles.mEntries.size());
		for (const auto& Entry : std::filesystem::directory_iterator(AbsoluteDir)) 
		{
			/*
			uint32_t Index = std::stoi(Entry.path().stem().string());
			std::vector<unsigned char>& Target = mChunkFiles[Index].mData;

			std::vector<unsigned char> CompressedData = application::util::FileUtil::ReadFile(Entry.path().string());

			Target.resize(mHeader.mChunkMesh.mChunkFiles.mEntries[Index].mFileSize);

			ZSTD_DCtx* const DCtx = ZSTD_createDCtx();
			ZSTD_DCtx_setFormat(DCtx, ZSTD_format_e::ZSTD_f_zstd1_magicless);
			const size_t DecompSize = ZSTD_decompressDCtx(DCtx, (void*)Target.data(), Target.size(), CompressedData.data(), CompressedData.size());
			Target.resize(DecompSize);
			ZSTD_freeDCtx(DCtx);
			*/
		}
	}

	CrbinFile::CrbinFile(std::vector<unsigned char> Data)
	{
		Initialize(Data);
	}
}