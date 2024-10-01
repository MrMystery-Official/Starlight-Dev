#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include "Actor.h"
#include <glm/glm.hpp>
#include <optional>
#include <utility>
#include <map>

class PhiveStaticCompound
{
public:
	static uint32_t ActorFluidTypeToInternalFluidType(Actor::WaterFluidType Type);
	static Actor::WaterFluidType InternalFluidTypeToActorFluidType(uint32_t Type);
	static std::optional<glm::vec2> GetActorModelScale(Actor& SceneActor);

	struct PhiveStaticCompoundHeader
	{
		char mMagic[5];
		uint8_t mNullTerminator;
		uint16_t mVersion;
		uint16_t mEndianess;
		uint16_t mReserve0;
		uint32_t mImageOffset;
		uint32_t mActorLinkOffset;
		uint32_t mActorHashMapOffset;
		uint32_t mActorIndexMapOffset;
		uint32_t mFixedOffset0;
		uint32_t mFixedOffset1;
		uint32_t mFixedOffset2;
		uint32_t mTeraWaterOffset;
		uint32_t mTeraWaterTerrainOffset;
		uint32_t mTeraWaterSubTerrainOffset0;
		uint32_t mTeraWaterSubTerrainOffset1;
		uint32_t mCompoundShapeImageOffsetArray[85];
		uint32_t mImageSize;
		uint32_t mActorLinkArraySize;
		uint32_t mActorHashMapSize;
		uint32_t mActorIndexMapSize;
		uint32_t mFixedCountArray0Size;
		uint32_t mFixedCountArray1Size;
		uint32_t mFixedCountArray2Size;
		uint32_t mTeraWaterSize;
		uint32_t mReserve9;
		uint32_t mReserve10;
		uint32_t mReserve11;
		uint32_t mCompoundShapeImageSizeArray[85];
	};

	struct PhiveStaticCompoundFixedOffset0
	{
		uint32_t mReserve0;
		uint8_t mReserve1;
		uint8_t mReserve2;
		uint8_t mReserve3;
		uint8_t mStackIndex;
		uint16_t mReserve4;
		uint16_t mInstanceIndex;
		uint32_t mReserve5;
		uint32_t mReserve6;
	};

	struct PhiveStaticCompoundFixedOffset1
	{
		uint32_t mReserve0;
		uint32_t mReserve1;
		uint16_t mReserve2;
		uint16_t mReserve3;
		uint32_t mReserve4;
	};

	struct PhiveStaticCompoundFixedOffset2
	{
		uint32_t mReserve0;
		uint32_t mReserve1;
	};

	struct PhiveStaticCompoundRigidBodyEntry
	{
		uint8_t mReserve0;
		uint8_t mReserve1;
		uint8_t mInfoIndex;
		uint8_t mLayerCollection;
	};

	struct PhiveStaticCompoundRigidBodyInfo
	{
		float mPosition[3];
		float mRotation[3];
		uint16_t mReserve0;
		uint16_t mWaterBoxCount;
		uint16_t mWaterFallCount;
		uint16_t WaterCylinderCount;
		uint64_t mPlacementGroupId;
	};

	struct PhiveStaticCompoundImage
	{
		PhiveStaticCompoundRigidBodyEntry mRigidBodyEntryEntityArray[0x800];
		PhiveStaticCompoundRigidBodyEntry mRigidBodyEntrySensorArray[0x800];
		uint32_t mReserve0;
		uint32_t mReserve1;
		uint32_t mRigidBodyEntryEntityCount;
		uint32_t mRigidBodyEntrySensorCount;
		uint32_t mActorLinkCount;
		uint32_t mFixedCount0;
		uint32_t mFixedCount1;
		uint32_t mFixedCount2;
		PhiveStaticCompoundRigidBodyInfo mRigidBodyInfoArray[0x40];
		char mCompoundName[0x80];
		uint32_t mWaterBoxCount;
		uint32_t mWaterCylinderCount;
		uint32_t mWaterFallCount;
		uint32_t mExternalMeshCount;
	};

	struct PhiveStaticCompoundActorLink
	{
		uint32_t mBaseIndex;
		uint32_t mEndIndex;
		uint64_t mActorHash;
		uint32_t mReserve0;
		uint32_t mReserve1;
		uint16_t mCompoundRigidBodyIndex;
		uint8_t mReserve2;
		uint8_t mIsValid;
		uint32_t mReserve3;
	};

	struct PhiveStaticCompoundTeraWater
	{
		float mReserve0;
		float mReserve1;
		float mReserve2;
		float mTranslateX;
		float mTranslateY;
		float mTranslateZ;
		uint32_t mWidth;
		uint32_t mHeight;
		uint64_t mTerrainSize;
		uint64_t mSubTerrainSize0;
		uint64_t mSubTerrainSize1;
		uint64_t mRuntime_terrain;
		uint64_t mRuntimeSubTerrain0;
		uint64_t mRuntimeSubTerrain1;
	};

	struct PhiveStaticCompoundTeraWaterTerrain
	{
		uint64_t mReserve0;
	};

	struct PhiveStaticCompoundHkTagFile
	{
		uint32_t mSizeFlag;
		char mMagic[4];

		std::vector<unsigned char> mData;
	};

	/* Water box */
	struct PhiveStaticCompoundWaterBox {
		uint16_t mCompoundId;
		uint16_t mReserve0;
		float mFlowSpeed;
		float mReserve2;
		float mScaleTimesTenX;
		float mScaleTimesTenY;
		float mScaleTimesTenZ;
		float mRotationX;
		float mRotationY;
		float mRotationZ;
		float mPositionX;
		float mPositionY;
		float mPositionZ;
		uint32_t mFluidType;
		uint32_t mReserve7;
		uint64_t mReserve8;
		uint64_t mReserve9;
	};
	struct PhiveStaticCompoundWaterCylinder {
		uint16_t mCompoundId;
		uint16_t mReserve0;
		float mFlowSpeed;
		float mReserve2;
		float mScaleTimesTenX;
		float mScaleTimesTenY;
		float mScaleTimesTenZ;
		float mRotationX;
		float mRotationY;
		float mRotationZ;
		float mPositionX;
		float mPositionY;
		float mPositionZ;
		uint32_t mFluidType;
		uint32_t mReserve7;
		uint64_t mReserve8;
		uint64_t mReserve9;
		float mReserve10;
		float mReserve11;
		float mReserve12;
		float mReserve13;
	};
	struct PhiveStaticCompoundWaterFall {
		uint16_t mCompoundId;
		uint16_t mReserve0;
		float mReserve1;
		float mReserve2;
		float mReserve3;
		float mReserve4;
		float mReserve5;
		float mRotationX;
		float mRotationY;
		float mRotationZ;
		float mPositionX;
		float mPositionY;
		float mPositionZ;
		uint32_t mFluidType;
		uint32_t mReserve7;
		uint64_t mReserve8;
		uint64_t mReserve9;
		float mReserve10;
		float mReserve11;
		float mReserve12;
		float mReserve13;
	};
	struct PhiveStaticCompoundExternalMesh {
		float mScaleX;
		float mScaleY;
		float mScaleZ;
		float mRotationX;
		float mRotationY;
		float mRotationZ;
		float mPositionX;
		float mPositionY;
		float mPositionZ;
		uint32_t mReserve3;
		uint32_t mReserve4;
	};

	struct PhiveStaticCompoundBphshReserve1
	{
		uint32_t mReserve0;
		uint32_t mReserve1;
		uint64_t mReserve2;
	};

	struct PhiveStaticCompoundBphsh
	{
		char mMagic[6];
		uint16_t mVersion;
		uint16_t mEndianess;
		uint16_t mReserve0;
		uint32_t mImageOffset;
		uint32_t mReserve0Offset;
		uint32_t mReserve1Offset;
		uint32_t mFileSize;
		uint32_t mTag0Size;
		uint32_t mReserve0Size;
		uint32_t mReserve1Size;
		uint32_t mReserve1;
		uint32_t mReserve2;

		PhiveStaticCompoundHkTagFile mTagFile;
		std::vector<uint64_t> mReserve0Array;
		std::vector<PhiveStaticCompoundBphshReserve1> mReserve1Array;
	};

	PhiveStaticCompoundHeader mHeader;
	PhiveStaticCompoundImage mImage;

	std::vector<PhiveStaticCompoundActorLink> mActorLinks;
	std::vector<uint64_t> mActorHashMap; //Hash map is only read to calculate optimal physics hashes, not to be written
	//std::vector<uint16_t> mActorIndexMap;

	//std::map<uint64_t, PhiveStaticCompoundActorLink> mActorLinks;

	std::vector<PhiveStaticCompoundFixedOffset0> mFixedOffset0Array;
	std::vector<PhiveStaticCompoundFixedOffset1> mFixedOffset1Array;
	std::vector<PhiveStaticCompoundFixedOffset2> mFixedOffset2Array;

	PhiveStaticCompoundTeraWater mTeraWater;
	std::vector<PhiveStaticCompoundTeraWaterTerrain> mTeraWaterTerrain;
	std::vector<uint8_t> mTeraWaterSubTerrain0;
	std::vector<uint8_t> mTeraWaterSubTerrain1;

	//Images
	std::vector<PhiveStaticCompoundHkTagFile> mCompoundTagFile;

	std::vector<PhiveStaticCompoundWaterBox> mWaterBoxArray;
	std::vector<PhiveStaticCompoundWaterCylinder> mWaterCylinderArray;
	std::vector<PhiveStaticCompoundWaterFall> mWaterFallArray;
	std::vector<PhiveStaticCompoundExternalMesh> mExternalMeshArray;

	std::vector<PhiveStaticCompoundBphsh> mExternalBphshMeshes;

	std::vector<unsigned char> ToBinary();
	void WriteToFile(std::string Path);

	PhiveStaticCompound(std::vector<unsigned char> Bytes);
	PhiveStaticCompound() = default;
};