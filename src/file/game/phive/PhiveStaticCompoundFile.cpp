#include "PhiveStaticCompoundFile.h"

#include <util/Logger.h>
#include <util/FileUtil.h>
#include <util/BinaryVectorReader.h>
#include <util/BinaryVectorWriter.h>
#include <iostream>
#include <fstream>

namespace application::file::game::phive
{
	PhiveStaticCompoundFile::PhiveStaticCompoundFile(std::vector<unsigned char> Bytes)
	{
		application::util::BinaryVectorReader Reader(Bytes);

		Reader.ReadStruct(&mHeader, sizeof(PhiveStaticCompoundHeader));
		Reader.Seek(mHeader.mImageOffset, application::util::BinaryVectorReader::Position::Begin);
		Reader.ReadStruct(&mImage, sizeof(PhiveStaticCompoundImage));

		Reader.Seek(mHeader.mActorLinkOffset, application::util::BinaryVectorReader::Position::Begin);
		mActorLinks.resize(mImage.mActorLinkCount);
		Reader.ReadStruct(mActorLinks.data(), sizeof(PhiveStaticCompoundActorLink) * mImage.mActorLinkCount);

		Reader.Seek(mHeader.mActorHashMapOffset, application::util::BinaryVectorReader::Position::Begin);
		mActorHashMap.resize(mImage.mActorLinkCount * 2);
		Reader.ReadStruct(mActorHashMap.data(), sizeof(uint64_t) * mImage.mActorLinkCount * 2);

		/*
		Reader.Seek(mHeader.mActorIndexMapOffset, BinaryVectorReader::Position::Begin);
		mActorIndexMap.resize(mImage.mActorLinkCount * 2);
		Reader.ReadStruct(mActorIndexMap.data(), sizeof(uint16_t) * mImage.mActorLinkCount * 2);
		*/

		Reader.Seek(mHeader.mFixedOffset0, application::util::BinaryVectorReader::Position::Begin);
		mFixedOffset0Array.resize(mImage.mFixedCount0);
		Reader.ReadStruct(mFixedOffset0Array.data(), sizeof(PhiveStaticCompoundFixedOffset0) * mImage.mFixedCount0);

		Reader.Seek(mHeader.mFixedOffset1, application::util::BinaryVectorReader::Position::Begin);
		mFixedOffset1Array.resize(mImage.mFixedCount1);
		Reader.ReadStruct(mFixedOffset1Array.data(), sizeof(PhiveStaticCompoundFixedOffset1) * mImage.mFixedCount1);

		Reader.Seek(mHeader.mFixedOffset2, application::util::BinaryVectorReader::Position::Begin);
		mFixedOffset2Array.resize(mImage.mFixedCount2);
		Reader.ReadStruct(mFixedOffset2Array.data(), sizeof(PhiveStaticCompoundFixedOffset2) * mImage.mFixedCount2);

		Reader.Seek(mHeader.mTeraWaterOffset, application::util::BinaryVectorReader::Position::Begin);
		Reader.ReadStruct(&mTeraWater, sizeof(PhiveStaticCompoundTeraWater));

		if (mTeraWater.mWidth * mTeraWater.mHeight * 8 == mTeraWater.mTerrainSize)
		{
			Reader.Seek(mHeader.mTeraWaterTerrainOffset, application::util::BinaryVectorReader::Position::Begin);
			mTeraWaterTerrain.resize(mTeraWater.mWidth * mTeraWater.mHeight);
			Reader.ReadStruct(mTeraWaterTerrain.data(), sizeof(PhiveStaticCompoundTeraWaterTerrain) * mTeraWater.mWidth * mTeraWater.mHeight);
		}
		if (mTeraWater.mSubTerrainSize0 != 0)
		{
			Reader.Seek(mHeader.mTeraWaterSubTerrainOffset0, application::util::BinaryVectorReader::Position::Begin);
			mTeraWaterSubTerrain0.resize(mTeraWater.mSubTerrainSize0);
			Reader.ReadStruct(mTeraWaterSubTerrain0.data(), sizeof(uint8_t) * mTeraWater.mSubTerrainSize0);
		}
		if (mTeraWater.mSubTerrainSize1 != 0)
		{
			Reader.Seek(mHeader.mTeraWaterSubTerrainOffset1, application::util::BinaryVectorReader::Position::Begin);
			mTeraWaterSubTerrain1.resize(mTeraWater.mSubTerrainSize1);
			Reader.ReadStruct(mTeraWaterSubTerrain1.data(), sizeof(uint8_t) * mTeraWater.mSubTerrainSize1);
		}

		uint32_t CompoundBodyCount = mImage.mRigidBodyEntryEntityCount + mImage.mRigidBodyEntrySensorCount;
		mCompoundTagFile.resize(CompoundBodyCount);
		uint32_t ImageIter = 0;
		for (uint32_t i = 0; i < CompoundBodyCount; i++)
		{
			Reader.Seek(mHeader.mCompoundShapeImageOffsetArray[ImageIter], application::util::BinaryVectorReader::Position::Begin);
			Reader.ReadStruct(&mCompoundTagFile[ImageIter], 8);
			mCompoundTagFile[ImageIter].mSizeFlag = bswap_32(mCompoundTagFile[ImageIter].mSizeFlag);
			mCompoundTagFile[ImageIter].mData.resize(mCompoundTagFile[ImageIter].mSizeFlag);
			Reader.Seek(-8, application::util::BinaryVectorReader::Position::Current);
			Reader.ReadStruct(mCompoundTagFile[ImageIter].mData.data(), mCompoundTagFile[ImageIter].mData.size());

			mCompoundTagFile[ImageIter].mTagFile.LoadFromBytes(mCompoundTagFile[ImageIter].mData, false);

			//mCompoundTagFile[ImageIter].ExtractItems();
			ImageIter++;
		}

		if (mImage.mWaterBoxCount != 0)
		{
			Reader.Seek(mHeader.mCompoundShapeImageOffsetArray[ImageIter], application::util::BinaryVectorReader::Position::Begin);
			mWaterBoxArray.resize(mImage.mWaterBoxCount);
			Reader.ReadStruct(mWaterBoxArray.data(), sizeof(PhiveStaticCompoundWaterBox) * mImage.mWaterBoxCount);
			ImageIter++;
		}
		if (mImage.mWaterCylinderCount != 0)
		{
			Reader.Seek(mHeader.mCompoundShapeImageOffsetArray[ImageIter], application::util::BinaryVectorReader::Position::Begin);
			mWaterCylinderArray.resize(mImage.mWaterCylinderCount);
			Reader.ReadStruct(mWaterCylinderArray.data(), sizeof(PhiveStaticCompoundWaterCylinder) * mImage.mWaterCylinderCount);
			ImageIter++;
		}
		if (mImage.mWaterFallCount != 0)
		{
			Reader.Seek(mHeader.mCompoundShapeImageOffsetArray[ImageIter], application::util::BinaryVectorReader::Position::Begin);
			mWaterFallArray.resize(mImage.mWaterFallCount);
			Reader.ReadStruct(mWaterFallArray.data(), sizeof(PhiveStaticCompoundWaterFall) * mImage.mWaterFallCount);
			ImageIter++;
		}
		if (mImage.mExternalMeshCount != 0)
		{
			Reader.Seek(mHeader.mCompoundShapeImageOffsetArray[ImageIter], application::util::BinaryVectorReader::Position::Begin);
			mExternalMeshArray.resize(mImage.mExternalMeshCount);
			Reader.ReadStruct(mExternalMeshArray.data(), sizeof(PhiveStaticCompoundExternalMesh) * mImage.mExternalMeshCount);
			ImageIter++;
		}

		mExternalBphshMeshes.resize(mImage.mExternalMeshCount);
		for (uint32_t i = 0; i < mImage.mExternalMeshCount; i++)
		{
			Reader.Seek(mHeader.mCompoundShapeImageOffsetArray[ImageIter], application::util::BinaryVectorReader::Position::Begin);
			Reader.ReadStruct(&mExternalBphshMeshes[i], 48);

			Reader.Seek(mHeader.mCompoundShapeImageOffsetArray[ImageIter] + mExternalBphshMeshes[i].mImageOffset, application::util::BinaryVectorReader::Position::Begin);
			Reader.ReadStruct(&mExternalBphshMeshes[i].mTagFile, 8);
			mExternalBphshMeshes[i].mTagFile.mSizeFlag = bswap_32(mExternalBphshMeshes[i].mTagFile.mSizeFlag);
			mExternalBphshMeshes[i].mTagFile.mData.resize(mExternalBphshMeshes[i].mTagFile.mSizeFlag - 8);
			Reader.ReadStruct(mExternalBphshMeshes[i].mTagFile.mData.data(), mExternalBphshMeshes[i].mTagFile.mData.size());

			Reader.Seek(mHeader.mCompoundShapeImageOffsetArray[ImageIter] + mExternalBphshMeshes[i].mReserve0Offset, application::util::BinaryVectorReader::Position::Begin);
			mExternalBphshMeshes[i].mReserve0Array.resize(mExternalBphshMeshes[i].mReserve0Size >> 3);
			Reader.ReadStruct(mExternalBphshMeshes[i].mReserve0Array.data(), mExternalBphshMeshes[i].mReserve0Array.size() * 8);

			Reader.Seek(mHeader.mCompoundShapeImageOffsetArray[ImageIter] + mExternalBphshMeshes[i].mReserve1Offset, application::util::BinaryVectorReader::Position::Begin);
			mExternalBphshMeshes[i].mReserve1Array.resize(mExternalBphshMeshes[i].mReserve1Size);
			Reader.ReadStruct(mExternalBphshMeshes[i].mReserve1Array.data(), mExternalBphshMeshes[i].mReserve1Array.size());

			Reader.Seek(mHeader.mCompoundShapeImageOffsetArray[ImageIter], application::util::BinaryVectorReader::Position::Begin);
			std::vector<unsigned char> BphshByteData(mExternalBphshMeshes[i].mFileSize);
			Reader.ReadStruct(BphshByteData.data(), BphshByteData.size());
			mExternalBphshMeshes[i].mPhiveShape = application::file::game::phive::shape::PhiveShape(BphshByteData);

			ImageIter++;
		}
		application::util::Logger::Info("PhiveStaticCompoundFile", "Decoded successfully");
	}

	std::vector<unsigned char> PhiveStaticCompoundFile::ToBinary()
	{
		application::util::BinaryVectorWriter Writer;

		Writer.Seek(sizeof(PhiveStaticCompoundHeader), application::util::BinaryVectorWriter::Position::Begin); //Skipping header
		Writer.Align(16);
		Writer.Seek(sizeof(PhiveStaticCompoundImage), application::util::BinaryVectorWriter::Position::Current); //Skipping image
		Writer.Align(16);

		std::vector<uint64_t> ActorHashMap;
		std::vector<uint16_t> ActorIndexMap;
		ActorHashMap.resize(mActorLinks.size() * 2);
		ActorIndexMap.resize(mActorLinks.size() * 2);
		std::fill(ActorHashMap.begin(), ActorHashMap.end(), 0);
		std::fill(ActorIndexMap.begin(), ActorIndexMap.end(), 0);

		for (size_t i = 0; i < mActorLinks.size(); i++)
		{
			PhiveStaticCompoundActorLink& Link = mActorLinks[i];

			uint32_t HashIndex = Link.mActorHash % (mActorLinks.size() * 2);

			while (ActorHashMap[HashIndex] > 0)
			{
				HashIndex++;
				if (HashIndex >= ActorHashMap.size())
				{
					HashIndex = 0;
				}
			}

			if (HashIndex >= ActorHashMap.size())
			{
				application::util::Logger::Error("PhiveStaticCompoundFile", "Tried to write hash out of bounds at %u out of %u actor links. Aborting...", (unsigned int)i, (unsigned int)mActorLinks.size());
				break;
			}

			if (ActorHashMap[HashIndex] > 0)
			{
				application::util::Logger::Error("PhiveStaticCompoundFile", "Hash collision detected at index %u", HashIndex);
			}

			ActorHashMap[HashIndex] = Link.mActorHash;
			ActorIndexMap[HashIndex] = i;
		}

		mHeader.mActorLinkOffset = Writer.GetPosition();
		Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(mActorLinks.data()), sizeof(PhiveStaticCompoundActorLink) * mActorLinks.size());
		Writer.Align(16);
		mHeader.mActorLinkArraySize = Writer.GetPosition() - mHeader.mActorLinkOffset;

		mHeader.mActorHashMapOffset = Writer.GetPosition();
		Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(ActorHashMap.data()), sizeof(uint64_t) * ActorHashMap.size());
		Writer.Align(16);
		mHeader.mActorHashMapSize = Writer.GetPosition() - mHeader.mActorHashMapOffset;

		mHeader.mActorIndexMapOffset = Writer.GetPosition();
		Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(ActorIndexMap.data()), sizeof(uint16_t) * ActorIndexMap.size());
		Writer.Align(16);
		mHeader.mActorIndexMapSize = Writer.GetPosition() - mHeader.mActorIndexMapOffset;

		mHeader.mFixedOffset0 = Writer.GetPosition();
		Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(mFixedOffset0Array.data()), sizeof(PhiveStaticCompoundFixedOffset0) * mFixedOffset0Array.size());
		Writer.Align(16);

		mHeader.mFixedOffset1 = Writer.GetPosition();
		Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(mFixedOffset1Array.data()), sizeof(PhiveStaticCompoundFixedOffset1) * mFixedOffset1Array.size());
		Writer.Align(16);

		mHeader.mFixedOffset2 = Writer.GetPosition();
		Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(mFixedOffset2Array.data()), sizeof(PhiveStaticCompoundFixedOffset2) * mFixedOffset2Array.size());
		Writer.Align(16);

		mTeraWater.mSubTerrainSize0 = mTeraWaterSubTerrain0.size();
		mTeraWater.mSubTerrainSize1 = mTeraWaterSubTerrain1.size();

		mHeader.mTeraWaterOffset = Writer.GetPosition();
		Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mTeraWater), sizeof(PhiveStaticCompoundTeraWater));
		Writer.Align(16);

		mHeader.mTeraWaterTerrainOffset = Writer.GetPosition();
		Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(mTeraWaterTerrain.data()), sizeof(PhiveStaticCompoundTeraWaterTerrain) * mTeraWaterTerrain.size());
		Writer.Align(16);

		mHeader.mTeraWaterSubTerrainOffset0 = Writer.GetPosition();
		if (mTeraWater.mSubTerrainSize0 != 0)
		{
			Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(mTeraWaterSubTerrain0.data()), sizeof(uint8_t) * mTeraWaterSubTerrain0.size());
			Writer.Align(16);
		}

		mHeader.mTeraWaterSubTerrainOffset1 = Writer.GetPosition();
		if (mTeraWater.mSubTerrainSize1 != 0)
		{
			Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(mTeraWaterSubTerrain1.data()), sizeof(uint8_t) * mTeraWaterSubTerrain1.size());
			Writer.Align(16);
		}

		uint32_t ImageIter = 0;

		memset(mHeader.mCompoundShapeImageOffsetArray, 0, (sizeof(mHeader.mCompoundShapeImageOffsetArray) / sizeof(mHeader.mCompoundShapeImageOffsetArray[0])) * sizeof(mHeader.mCompoundShapeImageOffsetArray[0]));
		memset(mHeader.mCompoundShapeImageSizeArray, 0, (sizeof(mHeader.mCompoundShapeImageSizeArray) / sizeof(mHeader.mCompoundShapeImageSizeArray[0])) * sizeof(mHeader.mCompoundShapeImageSizeArray[0]));

		mImage.mActorLinkCount = mActorLinks.size();
		mImage.mExternalMeshCount = mExternalMeshArray.size();
		mImage.mWaterBoxCount = mWaterBoxArray.size();
		mImage.mWaterCylinderCount = mWaterCylinderArray.size();
		mImage.mWaterFallCount = mWaterFallArray.size();
		mImage.mFixedCount0 = mFixedOffset0Array.size();
		mImage.mFixedCount1 = mFixedOffset1Array.size();
		mImage.mFixedCount2 = mFixedOffset2Array.size();

		mImage.mRigidBodyInfoArray[0].mWaterBoxCount = mWaterBoxArray.size();
		mImage.mRigidBodyInfoArray[0].mWaterFallCount = mWaterFallArray.size();

		std::fill(mHeader.mCompoundShapeImageSizeArray, mHeader.mCompoundShapeImageSizeArray + 85, 0);
		std::fill(mHeader.mCompoundShapeImageOffsetArray, mHeader.mCompoundShapeImageOffsetArray + 85, 0);

		for (size_t i = 0; i < mCompoundTagFile.size(); i++)
		{
			uint32_t Start = Writer.GetPosition();
			mHeader.mCompoundShapeImageOffsetArray[ImageIter] = Start;
			//Writer.WriteInteger(bswap_32(mCompoundTagFile[i].mSizeFlag), sizeof(uint32_t));
			//Writer.WriteRawUnsafeFixed(&mCompoundTagFile[i].mMagic[0], 4);

			size_t OldSize = mCompoundTagFile[i].mData.size();

			application::file::game::phive::classes::HavokClassBinaryVectorWriter TagFileWriter;
			TagFileWriter.GetData() = mCompoundTagFile[i].mData;

			for (auto& MeshInstance : mCompoundTagFile[i].mTagFile.mRootClass.mInstances.mParent.mElements.mParent.mElements)
			{
				MeshInstance.mParent.ApplyPatch(TagFileWriter);
			}

			mCompoundTagFile[i].mData = TagFileWriter.GetData();
			mCompoundTagFile[i].mData.resize(OldSize);
			Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(mCompoundTagFile[i].mData.data()), mCompoundTagFile[i].mData.size());
			Writer.Align(16);
			uint32_t End = Writer.GetPosition();
			mHeader.mCompoundShapeImageSizeArray[ImageIter] = End - Start;
			ImageIter++;
		}

		if (mImage.mWaterBoxCount != 0)
		{
			uint32_t Start = Writer.GetPosition();
			mHeader.mCompoundShapeImageOffsetArray[ImageIter] = Start;
			Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(mWaterBoxArray.data()), sizeof(PhiveStaticCompoundWaterBox) * mWaterBoxArray.size());
			Writer.Align(16);
			uint32_t End = Writer.GetPosition();
			mHeader.mCompoundShapeImageSizeArray[ImageIter] = End - Start;
			ImageIter++;
		}
		if (mImage.mWaterCylinderCount != 0)
		{
			mHeader.mCompoundShapeImageOffsetArray[ImageIter] = Writer.GetPosition();
			Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(mWaterCylinderArray.data()), sizeof(PhiveStaticCompoundWaterCylinder) * mWaterCylinderArray.size());
			Writer.Align(16);
			ImageIter++;
		}
		if (mImage.mWaterFallCount != 0)
		{
			mHeader.mCompoundShapeImageOffsetArray[ImageIter] = Writer.GetPosition();
			Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(mWaterFallArray.data()), sizeof(PhiveStaticCompoundWaterFall) * mWaterFallArray.size());
			Writer.Align(16);
			ImageIter++;
		}
		if (mImage.mExternalMeshCount != 0)
		{
			uint32_t Start = Writer.GetPosition();
			mHeader.mCompoundShapeImageOffsetArray[ImageIter] = Start;
			Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(mExternalMeshArray.data()), sizeof(PhiveStaticCompoundExternalMesh) * mExternalMeshArray.size());
			Writer.Align(16);

			uint32_t End = Writer.GetPosition();
			mHeader.mCompoundShapeImageSizeArray[ImageIter] = End - Start;
			ImageIter++;
		}

		for (size_t i = 0; i < mExternalBphshMeshes.size(); i++)
		{
			uint32_t Start = Writer.GetPosition();
			mHeader.mCompoundShapeImageOffsetArray[ImageIter] = Start;

			if (mReserializeCollision || mExternalBphshMeshes[i].mReserializeCollision)
			{
				//Some bphsh debugging stuff not needed anymore
				/*
				application::util::BinaryVectorWriter OtherWriter;
				OtherWriter.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mExternalBphshMeshes[i]), 48);

				OtherWriter.WriteInteger(bswap_32(mExternalBphshMeshes[i].mTagFile.mSizeFlag), sizeof(uint32_t));
				OtherWriter.WriteRawUnsafeFixed(&mExternalBphshMeshes[i].mTagFile.mMagic[0], 4);
				OtherWriter.WriteRawUnsafeFixed(reinterpret_cast<const char*>(mExternalBphshMeshes[i].mTagFile.mData.data()), mExternalBphshMeshes[i].mTagFile.mData.size());
				OtherWriter.Align(16);

				OtherWriter.WriteRawUnsafeFixed(reinterpret_cast<const char*>(mExternalBphshMeshes[i].mReserve0Array.data()), sizeof(uint64_t)* mExternalBphshMeshes[i].mReserve0Array.size());
				OtherWriter.Align(16);

				OtherWriter.WriteRawUnsafeFixed(reinterpret_cast<const char*>(mExternalBphshMeshes[i].mReserve1Array.data()), sizeof(PhiveStaticCompoundBphshReserve1)* mExternalBphshMeshes[i].mReserve1Array.size());
				OtherWriter.Align(16);

				application::util::FileUtil::WriteFile(application::util::FileUtil::GetWorkingDirFilePath("Original.bphsh"), OtherWriter.GetData());
				*/

				std::vector<unsigned char> Binary = mExternalBphshMeshes[i].mPhiveShape.ToBinary();
				Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(Binary.data()), Binary.size());
				Writer.Align(16);

				//application::util::FileUtil::WriteFile(application::util::FileUtil::GetWorkingDirFilePath("Reserialized.bphsh"), Binary);
			}
			else
			{
				Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mExternalBphshMeshes[i]), 48);

				Writer.WriteInteger(bswap_32(mExternalBphshMeshes[i].mTagFile.mSizeFlag), sizeof(uint32_t));
				Writer.WriteRawUnsafeFixed(&mExternalBphshMeshes[i].mTagFile.mMagic[0], 4);
				Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(mExternalBphshMeshes[i].mTagFile.mData.data()), mExternalBphshMeshes[i].mTagFile.mData.size());
				Writer.Align(16);

				Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(mExternalBphshMeshes[i].mReserve0Array.data()), sizeof(uint64_t) * mExternalBphshMeshes[i].mReserve0Array.size());
				Writer.Align(16);

				Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(mExternalBphshMeshes[i].mReserve1Array.data()), mExternalBphshMeshes[i].mReserve1Array.size());
				Writer.Align(16);
			}

			uint32_t End = Writer.GetPosition();
			mHeader.mCompoundShapeImageSizeArray[ImageIter] = End - Start;
			ImageIter++;
		}

		uint32_t FileEnd = Writer.GetPosition();

		mHeader.mCompoundShapeImageOffsetArray[ImageIter] = FileEnd;

		Writer.Seek(0, application::util::BinaryVectorWriter::Position::Begin);
		Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mHeader), sizeof(PhiveStaticCompoundHeader));
		Writer.Align(16);
		Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mImage), sizeof(PhiveStaticCompoundImage));

		std::vector<unsigned char> Data = Writer.GetData();
		Data.resize(FileEnd);

		return Data;
	}

	void PhiveStaticCompoundFile::WriteToFile(const std::string& Path)
	{
		std::ofstream File(Path, std::ios::binary);
		std::vector<unsigned char> Binary = ToBinary();
		std::copy(Binary.cbegin(), Binary.cend(),
			std::ostream_iterator<unsigned char>(File));
		File.close();
	}
}