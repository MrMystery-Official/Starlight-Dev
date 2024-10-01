#include "PhiveStaticCompound.h"

#include "BinaryVectorReader.h"
#include "BinaryVectorWriter.h"
#include "Logger.h"
#include <iostream>
#include <fstream>

uint32_t PhiveStaticCompound::ActorFluidTypeToInternalFluidType(Actor::WaterFluidType Type)
{
	switch (Type)
	{
	case Actor::WaterFluidType::Water:
		return 38;
	case Actor::WaterFluidType::Lava:
		return 41;
	case Actor::WaterFluidType::DriftSand:
		return 3;
	default:
		return 38; //Water
	}
}

Actor::WaterFluidType PhiveStaticCompound::InternalFluidTypeToActorFluidType(uint32_t Type)
{
	switch (Type)
	{
	case 38:
		return Actor::WaterFluidType::Water;
	case 41:
		return Actor::WaterFluidType::Lava;
	case 3:
		return Actor::WaterFluidType::DriftSand;
	default:
		return Actor::WaterFluidType::Water;
	}
}

std::optional<glm::vec2> PhiveStaticCompound::GetActorModelScale(Actor& SceneActor)
{
	if (SceneActor.Model->mBfres->Models.Nodes.empty())
		return std::nullopt;

	if (SceneActor.Model->mBfres->Models.GetByIndex(0).Value.Shapes.Nodes.empty())
		return std::nullopt;

	std::vector<glm::vec3> ModelVertices;

	glm::mat4 GLMModel = glm::mat4(1.0f);  // Identity matrix
	GLMModel = glm::scale(GLMModel, glm::vec3(SceneActor.Scale.GetX(), SceneActor.Scale.GetY(), SceneActor.Scale.GetZ()));
	for (uint32_t SubModelIndex = 0; SubModelIndex < SceneActor.Model->mBfres->Models.GetByIndex(0).Value.Shapes.Nodes.size(); SubModelIndex++)
	{
		std::vector<glm::vec4> Positions = SceneActor.Model->mBfres->Models.GetByIndex(0).Value.VertexBuffers[SceneActor.Model->mBfres->Models.GetByIndex(0).Value.Shapes.Nodes[SceneActor.Model->mBfres->Models.GetByIndex(0).Value.Shapes.GetKey(SubModelIndex)].Value.VertexBufferIndex].GetPositions();
		for (glm::vec4& Vertex : Positions)
		{
			Vertex = GLMModel * Vertex;
			ModelVertices.push_back(glm::vec3(Vertex.x, Vertex.y, Vertex.z));
		}
	}

	glm::vec2 Max(std::numeric_limits<float>::lowest());

	for (glm::vec3& Vertex : ModelVertices) {
		if (Vertex.x > Max.x) Max.x = Vertex.x * 2;
		if (Vertex.z > Max.y) Max.y = Vertex.z * 2;
	}

	return Max;
}

PhiveStaticCompound::PhiveStaticCompound(std::vector<unsigned char> Bytes)
{
	BinaryVectorReader Reader(Bytes);

	Reader.ReadStruct(&mHeader, sizeof(PhiveStaticCompoundHeader));
	Reader.Seek(mHeader.mImageOffset, BinaryVectorReader::Position::Begin);
	Reader.ReadStruct(&mImage, sizeof(PhiveStaticCompoundImage));

	Reader.Seek(mHeader.mActorLinkOffset, BinaryVectorReader::Position::Begin);
	mActorLinks.resize(mImage.mActorLinkCount);
	Reader.ReadStruct(mActorLinks.data(), sizeof(PhiveStaticCompoundActorLink) * mImage.mActorLinkCount);

	Reader.Seek(mHeader.mActorHashMapOffset, BinaryVectorReader::Position::Begin);
	mActorHashMap.resize(mImage.mActorLinkCount * 2);
	Reader.ReadStruct(mActorHashMap.data(), sizeof(uint64_t) * mImage.mActorLinkCount * 2);

	/*
	Reader.Seek(mHeader.mActorIndexMapOffset, BinaryVectorReader::Position::Begin);
	mActorIndexMap.resize(mImage.mActorLinkCount * 2);
	Reader.ReadStruct(mActorIndexMap.data(), sizeof(uint16_t) * mImage.mActorLinkCount * 2);
	*/

	Reader.Seek(mHeader.mFixedOffset0, BinaryVectorReader::Position::Begin);
	mFixedOffset0Array.resize(mImage.mFixedCount0);
	Reader.ReadStruct(mFixedOffset0Array.data(), sizeof(PhiveStaticCompoundFixedOffset0) * mImage.mFixedCount0);

	Reader.Seek(mHeader.mFixedOffset1, BinaryVectorReader::Position::Begin);
	mFixedOffset1Array.resize(mImage.mFixedCount1);
	Reader.ReadStruct(mFixedOffset1Array.data(), sizeof(PhiveStaticCompoundFixedOffset1) * mImage.mFixedCount1);

	Reader.Seek(mHeader.mFixedOffset2, BinaryVectorReader::Position::Begin);
	mFixedOffset2Array.resize(mImage.mFixedCount2);
	Reader.ReadStruct(mFixedOffset2Array.data(), sizeof(PhiveStaticCompoundFixedOffset2) * mImage.mFixedCount2);

	Reader.Seek(mHeader.mTeraWaterOffset, BinaryVectorReader::Position::Begin);
	Reader.ReadStruct(&mTeraWater, sizeof(PhiveStaticCompoundTeraWater));

	if (mTeraWater.mWidth * mTeraWater.mHeight * 8 == mTeraWater.mTerrainSize)
	{
		Reader.Seek(mHeader.mTeraWaterTerrainOffset, BinaryVectorReader::Position::Begin);
		mTeraWaterTerrain.resize(mTeraWater.mWidth * mTeraWater.mHeight);
		Reader.ReadStruct(mTeraWaterTerrain.data(), sizeof(PhiveStaticCompoundTeraWaterTerrain) * mTeraWater.mWidth * mTeraWater.mHeight);
	}
	if (mTeraWater.mSubTerrainSize0 != 0)
	{
		Reader.Seek(mHeader.mTeraWaterSubTerrainOffset0, BinaryVectorReader::Position::Begin);
		mTeraWaterSubTerrain0.resize(mTeraWater.mSubTerrainSize0);
		Reader.ReadStruct(mTeraWaterSubTerrain0.data(), sizeof(uint8_t) * mTeraWater.mSubTerrainSize0);
	}
	if (mTeraWater.mSubTerrainSize1 != 0)
	{
		Reader.Seek(mHeader.mTeraWaterSubTerrainOffset1, BinaryVectorReader::Position::Begin);
		mTeraWaterSubTerrain1.resize(mTeraWater.mSubTerrainSize1);
		Reader.ReadStruct(mTeraWaterSubTerrain1.data(), sizeof(uint8_t) * mTeraWater.mSubTerrainSize1);
	}

	uint32_t CompoundBodyCount = mImage.mRigidBodyEntryEntityCount + mImage.mRigidBodyEntrySensorCount;
	mCompoundTagFile.resize(CompoundBodyCount);
	uint32_t ImageIter = 0;
	for (uint32_t i = 0; i < CompoundBodyCount; i++)
	{
		Reader.Seek(mHeader.mCompoundShapeImageOffsetArray[ImageIter], BinaryVectorReader::Position::Begin);
		Reader.ReadStruct(&mCompoundTagFile[ImageIter], 8);
		mCompoundTagFile[ImageIter].mSizeFlag = _byteswap_ulong(mCompoundTagFile[ImageIter].mSizeFlag);
		mCompoundTagFile[ImageIter].mData.resize(mCompoundTagFile[ImageIter].mSizeFlag - 8);
		Reader.ReadStruct(mCompoundTagFile[ImageIter].mData.data(), mCompoundTagFile[ImageIter].mData.size());
		ImageIter++;
	}

	if (mImage.mWaterBoxCount != 0)
	{
		Reader.Seek(mHeader.mCompoundShapeImageOffsetArray[ImageIter], BinaryVectorReader::Position::Begin);
		mWaterBoxArray.resize(mImage.mWaterBoxCount);
		Reader.ReadStruct(mWaterBoxArray.data(), sizeof(PhiveStaticCompoundWaterBox) * mImage.mWaterBoxCount);
		ImageIter++;
	}
	if (mImage.mWaterCylinderCount != 0)
	{
		Reader.Seek(mHeader.mCompoundShapeImageOffsetArray[ImageIter], BinaryVectorReader::Position::Begin);
		mWaterCylinderArray.resize(mImage.mWaterCylinderCount);
		Reader.ReadStruct(mWaterCylinderArray.data(), sizeof(PhiveStaticCompoundWaterCylinder) * mImage.mWaterCylinderCount);
		ImageIter++;
	}
	if (mImage.mWaterFallCount != 0)
	{
		Reader.Seek(mHeader.mCompoundShapeImageOffsetArray[ImageIter], BinaryVectorReader::Position::Begin);
		mWaterFallArray.resize(mImage.mWaterFallCount);
		Reader.ReadStruct(mWaterFallArray.data(), sizeof(PhiveStaticCompoundWaterFall) * mImage.mWaterFallCount);
		ImageIter++;
	}
	if (mImage.mExternalMeshCount != 0)
	{
		Reader.Seek(mHeader.mCompoundShapeImageOffsetArray[ImageIter], BinaryVectorReader::Position::Begin);
		mExternalMeshArray.resize(mImage.mExternalMeshCount);
		Reader.ReadStruct(mExternalMeshArray.data(), sizeof(PhiveStaticCompoundExternalMesh) * mImage.mExternalMeshCount);
		ImageIter++;
	}

	mExternalBphshMeshes.resize(mImage.mExternalMeshCount);
	for (uint32_t i = 0; i < mImage.mExternalMeshCount; i++)
	{
		Reader.Seek(mHeader.mCompoundShapeImageOffsetArray[ImageIter], BinaryVectorReader::Position::Begin);
		Reader.ReadStruct(&mExternalBphshMeshes[i], 48);

		Reader.Seek(mHeader.mCompoundShapeImageOffsetArray[ImageIter] + mExternalBphshMeshes[i].mImageOffset, BinaryVectorReader::Position::Begin);
		Reader.ReadStruct(&mExternalBphshMeshes[i].mTagFile, 8);
		mExternalBphshMeshes[i].mTagFile.mSizeFlag = _byteswap_ulong(mExternalBphshMeshes[i].mTagFile.mSizeFlag);
		mExternalBphshMeshes[i].mTagFile.mData.resize(mExternalBphshMeshes[i].mTagFile.mSizeFlag - 8);
		Reader.ReadStruct(mExternalBphshMeshes[i].mTagFile.mData.data(), mExternalBphshMeshes[i].mTagFile.mData.size());

		Reader.Seek(mHeader.mCompoundShapeImageOffsetArray[ImageIter] + mExternalBphshMeshes[i].mReserve0Offset, BinaryVectorReader::Position::Begin);
		mExternalBphshMeshes[i].mReserve0Array.resize(mExternalBphshMeshes[i].mReserve0Size >> 3);
		Reader.ReadStruct(mExternalBphshMeshes[i].mReserve0Array.data(), mExternalBphshMeshes[i].mReserve0Array.size() * 8);

		Reader.Seek(mHeader.mCompoundShapeImageOffsetArray[ImageIter] + mExternalBphshMeshes[i].mReserve1Offset, BinaryVectorReader::Position::Begin);
		mExternalBphshMeshes[i].mReserve1Array.resize(mExternalBphshMeshes[i].mReserve1Size >> 4);
		Reader.ReadStruct(mExternalBphshMeshes[i].mReserve1Array.data(), mExternalBphshMeshes[i].mReserve1Array.size() * sizeof(PhiveStaticCompoundBphshReserve1));

		ImageIter++;
	}
	std::cout << "COMPLETED\n";
}

std::vector<unsigned char> PhiveStaticCompound::ToBinary()
{
	BinaryVectorWriter Writer;

	Writer.Seek(sizeof(PhiveStaticCompoundHeader), BinaryVectorWriter::Position::Begin); //Skipping header
	Writer.Align(16);
	Writer.Seek(sizeof(PhiveStaticCompoundImage), BinaryVectorWriter::Position::Current); //Skipping image
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
			Logger::Error("PhiveStaticCompound", "Tried to write hash out of bounds at " + std::to_string(i) + " out of " + std::to_string(mActorLinks.size()) + " actor links. Aborting...");
			break;
		}

		if (ActorHashMap[HashIndex] > 0)
		{
			Logger::Error("PhiveStaticCompound", "Hash collision detected at index " + std::to_string(HashIndex));
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
		Writer.WriteInteger(_byteswap_ulong(mCompoundTagFile[i].mSizeFlag), sizeof(uint32_t));
		Writer.WriteRawUnsafeFixed(&mCompoundTagFile[i].mMagic[0], 4);
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

		Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mExternalBphshMeshes[i]), 48);

		Writer.WriteInteger(_byteswap_ulong(mExternalBphshMeshes[i].mTagFile.mSizeFlag), sizeof(uint32_t));
		Writer.WriteRawUnsafeFixed(&mExternalBphshMeshes[i].mTagFile.mMagic[0], 4);
		Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(mExternalBphshMeshes[i].mTagFile.mData.data()), mExternalBphshMeshes[i].mTagFile.mData.size());
		Writer.Align(16);

		Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(mExternalBphshMeshes[i].mReserve0Array.data()), sizeof(uint64_t) * mExternalBphshMeshes[i].mReserve0Array.size());
		Writer.Align(16);

		Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(mExternalBphshMeshes[i].mReserve1Array.data()), sizeof(PhiveStaticCompoundBphshReserve1) * mExternalBphshMeshes[i].mReserve1Array.size());
		Writer.Align(16);

		uint32_t End = Writer.GetPosition();
		mHeader.mCompoundShapeImageSizeArray[ImageIter] = End - Start;
		ImageIter++;
	}

	uint32_t FileEnd = Writer.GetPosition();

	Writer.Seek(0, BinaryVectorWriter::Position::Begin);
	Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mHeader), sizeof(PhiveStaticCompoundHeader));
	Writer.Align(16);
	Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mImage), sizeof(PhiveStaticCompoundImage));

	std::vector<unsigned char> Data = Writer.GetData();
	Data.resize(FileEnd);

	return Data;
}

void PhiveStaticCompound::WriteToFile(std::string Path)
{
	std::ofstream File(Path, std::ios::binary);
	std::vector<unsigned char> Binary = ToBinary();
	std::copy(Binary.cbegin(), Binary.cend(),
		std::ostream_iterator<unsigned char>(File));
	File.close();
}