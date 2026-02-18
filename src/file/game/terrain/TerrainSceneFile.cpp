#include "TerrainSceneFile.h"

#include <util/FileUtil.h>
#include <glm/glm.hpp>
#include <map>

namespace application::file::game::terrain
{
	void TerrainSceneFile::RelOffset::Read(application::util::BinaryVectorReader& Reader)
	{
		mOffset = Reader.GetPosition();
		mValue = Reader.ReadUInt32();
	}

	void TerrainSceneFile::RelOffset::Skip(application::util::BinaryVectorWriter& Writer, TerrainSceneFile& SceneFile)
	{
		mOffset = Writer.GetPosition();
		Writer.Seek(4, application::util::BinaryVectorWriter::Position::Current);
	}

	void TerrainSceneFile::RelOffset::Write(application::util::BinaryVectorWriter& Writer, TerrainSceneFile& SceneFile)
	{
		mValue = Writer.GetPosition() - mOffset;

		uint32_t Jumpback = Writer.GetPosition();

		Writer.Seek(mOffset, application::util::BinaryVectorWriter::Position::Begin);
		Writer.WriteInteger(mValue, sizeof(uint32_t));
		Writer.Seek(Jumpback, application::util::BinaryVectorWriter::Position::Begin);
	}

	void TerrainSceneFile::ResString::Read(application::util::BinaryVectorReader& Reader)
	{
		char Char = Reader.ReadUInt8();
		while (Char != 0x00)
		{
			mString += Char;
			Char = Reader.ReadUInt8();
		}
	}

	void TerrainSceneFile::ResString::Write(application::util::BinaryVectorWriter& Writer, TerrainSceneFile& SceneFile)
	{
		for (uint8_t c : mString)
		{
			Writer.WriteInteger(c, sizeof(uint8_t));
		}
		Writer.WriteInteger(0x00, sizeof(uint8_t));
	}

	void TerrainSceneFile::ResWString::Read(application::util::BinaryVectorReader& Reader)
	{
		wchar_t Char = Reader.ReadUInt16();
		while (Char != 0x0000)
		{
			mWString += Char;
			Char = Reader.ReadUInt16();
		}
	}

	void TerrainSceneFile::ResWString::Write(application::util::BinaryVectorWriter& Writer, TerrainSceneFile& SceneFile)
	{
		for (uint16_t c : mWString)
		{
			Writer.WriteInteger(c, sizeof(uint16_t));
		}
		Writer.WriteInteger(0x0000, sizeof(uint16_t));
	}

	void TerrainSceneFile::ResStringPtr::Read(application::util::BinaryVectorReader& Reader)
	{
		mOffset = Reader.ReadUInt32();
		uint64_t Jumpback = Reader.GetPosition();

		Reader.Seek(mOffset - 4, application::util::BinaryVectorReader::Position::Current);
		mResString.Read(Reader);
		Reader.Seek(Jumpback, application::util::BinaryVectorReader::Position::Begin);
	}

	void TerrainSceneFile::ResStringPtr::Skip(application::util::BinaryVectorWriter& Writer, TerrainSceneFile& SceneFile)
	{
		mOffset = Writer.GetPosition();
		Writer.Seek(4, application::util::BinaryVectorWriter::Position::Current);

		SceneFile.mStringQuery.push_back(this);
	}

	void TerrainSceneFile::ResStringPtr::Write(application::util::BinaryVectorWriter& Writer, TerrainSceneFile& SceneFile, uint64_t AbsoluteOffset)
	{
		uint64_t Jumpback = Writer.GetPosition();
		Writer.Seek(mOffset, application::util::BinaryVectorWriter::Position::Begin);
		Writer.WriteInteger(AbsoluteOffset - mOffset, sizeof(uint32_t));
		Writer.Seek(Jumpback, application::util::BinaryVectorWriter::Position::Begin);
	}

	void TerrainSceneFile::ResWStringPtr::Read(application::util::BinaryVectorReader& Reader)
	{
		mOffset = Reader.ReadUInt32();
		uint64_t Jumpback = Reader.GetPosition();

		Reader.Seek(mOffset - 4, application::util::BinaryVectorReader::Position::Current);
		mResWString.Read(Reader);
		Reader.Seek(Jumpback, application::util::BinaryVectorReader::Position::Begin);
	}

	void TerrainSceneFile::ResWStringPtr::Skip(application::util::BinaryVectorWriter& Writer, TerrainSceneFile& SceneFile)
	{
		mOffset = Writer.GetPosition();
		Writer.Seek(4, application::util::BinaryVectorWriter::Position::Current);

		SceneFile.mStringQuery.push_back(this);
	}

	void TerrainSceneFile::ResWStringPtr::Write(application::util::BinaryVectorWriter& Writer, TerrainSceneFile& SceneFile, uint64_t AbsoluteOffset)
	{
		uint64_t Jumpback = Writer.GetPosition();
		Writer.Seek(mOffset, application::util::BinaryVectorWriter::Position::Begin);
		Writer.WriteInteger(AbsoluteOffset - mOffset, sizeof(uint32_t));
		Writer.Seek(Jumpback, application::util::BinaryVectorWriter::Position::Begin);
	}

	void TerrainSceneFile::Color3f::Read(application::util::BinaryVectorReader& Reader)
	{
		mR = Reader.ReadFloat();
		mG = Reader.ReadFloat();
		mB = Reader.ReadFloat();
	}

	void TerrainSceneFile::Color3f::Write(application::util::BinaryVectorWriter& Writer, TerrainSceneFile& SceneFile)
	{
		Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mR), 4);
		Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mG), 4);
		Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mB), 4);
	}

	void TerrainSceneFile::Color4f::Read(application::util::BinaryVectorReader& Reader)
	{
		mR = Reader.ReadFloat();
		mG = Reader.ReadFloat();
		mB = Reader.ReadFloat();
		mA = Reader.ReadFloat();
	}

	void TerrainSceneFile::Color4f::Write(application::util::BinaryVectorWriter& Writer, TerrainSceneFile& SceneFile)
	{
		Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mR), 4);
		Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mG), 4);
		Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mB), 4);
		Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mA), 4);
	}

	void TerrainSceneFile::ResImageInfo::Read(application::util::BinaryVectorReader& Reader)
	{
		mType = static_cast<ResFileType>(Reader.ReadUInt32());
		_04 = Reader.ReadUInt32();
		_08 = Reader.ReadUInt32();
		_0C = Reader.ReadUInt32();
		mWidth = Reader.ReadUInt16();
		_12 = Reader.ReadUInt16();
		mMargin = Reader.ReadUInt8();
		Reader.ReadStruct(&_15[0], 0x3);
		mMipCount = Reader.ReadUInt32();
		mFormat = static_cast<ImageFormat>(Reader.ReadUInt8());
		Reader.ReadStruct(&_1D[0], 0x3);
		mArchiveLevel = Reader.ReadUInt32();
		mDefFilterType = static_cast<DefFilterType>(Reader.ReadUInt32());
		_28 = Reader.ReadUInt32();
		_2C = Reader.ReadUInt32();
		mDependencyCount = Reader.ReadUInt32();
		mDependencies[0] = Reader.ReadUInt32();
		mDependencies[1] = Reader.ReadUInt32();
		mDependencies[2] = Reader.ReadUInt32();
		mClearColor.Read(Reader);
		mFlags = Reader.ReadUInt32();
		_54 = Reader.ReadUInt32();
		_58 = Reader.ReadUInt32();
		_5C = Reader.ReadUInt32();
		mNameOffset.Read(Reader);
		mFileTypeOffset.Read(Reader);
		mDescriptionOffset.Read(Reader);
		mExtensionOffset.Read(Reader);
		_70 = Reader.ReadUInt32();
		_74 = Reader.ReadUInt32();
		_78 = Reader.ReadUInt32();
		_7C = Reader.ReadUInt32();
	}

	void TerrainSceneFile::ResImageInfo::Write(application::util::BinaryVectorWriter& Writer, TerrainSceneFile& SceneFile)
	{
		Writer.WriteInteger(mType, sizeof(uint32_t));
		Writer.WriteInteger(_04, sizeof(uint32_t));
		Writer.WriteInteger(_08, sizeof(uint32_t));
		Writer.WriteInteger(_0C, sizeof(uint32_t));
		Writer.WriteInteger(mWidth, sizeof(uint16_t));
		Writer.WriteInteger(_12, sizeof(uint16_t));
		Writer.WriteInteger(mMargin, sizeof(uint8_t));
		Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&_15[0]), 3);
		Writer.WriteInteger(mMipCount, sizeof(uint32_t));
		Writer.WriteInteger(mFormat, sizeof(uint8_t));
		Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&_1D[0]), 3);
		Writer.WriteInteger(mArchiveLevel, sizeof(uint32_t));
		Writer.WriteInteger(mDefFilterType, sizeof(uint32_t));
		Writer.WriteInteger(_28, sizeof(uint32_t));
		Writer.WriteInteger(_2C, sizeof(uint32_t));
		Writer.WriteInteger(mDependencyCount, sizeof(uint32_t));
		Writer.WriteInteger(mDependencies[0], sizeof(uint32_t));
		Writer.WriteInteger(mDependencies[1], sizeof(uint32_t));
		Writer.WriteInteger(mDependencies[2], sizeof(uint32_t));
		mClearColor.Write(Writer, SceneFile);
		Writer.WriteInteger(mFlags, sizeof(uint32_t));
		Writer.WriteInteger(_54, sizeof(uint32_t));
		Writer.WriteInteger(_58, sizeof(uint32_t));
		Writer.WriteInteger(_5C, sizeof(uint32_t));
		mNameOffset.Skip(Writer, SceneFile);
		mFileTypeOffset.Skip(Writer, SceneFile);
		mDescriptionOffset.Skip(Writer, SceneFile);
		mExtensionOffset.Skip(Writer, SceneFile);
		Writer.WriteInteger(_70, sizeof(uint32_t));
		Writer.WriteInteger(_74, sizeof(uint32_t));
		Writer.WriteInteger(_78, sizeof(uint32_t));
		Writer.WriteInteger(_7C, sizeof(uint32_t));
	}

	void TerrainSceneFile::ResMaterialInfo::Read(application::util::BinaryVectorReader& Reader)
	{
		mTexIndex = Reader.ReadUInt32();
		_04 = Reader.ReadUInt32();
		_08 = Reader.ReadUInt32();
		_0C = Reader.ReadUInt32();
		mUScale = Reader.ReadFloat();
		mVScale = Reader.ReadFloat();
		_18 = Reader.ReadUInt32();
		_1C = Reader.ReadUInt32();
		mFabricationMicro = Reader.ReadFloat();
		mFabricationTiling = Reader.ReadFloat();
		_28 = Reader.ReadUInt32();
		mHeightCorrection = Reader.ReadUInt32();
		mColor.Read(Reader);
		_3C = Reader.ReadUInt32();
	}

	void TerrainSceneFile::ResMaterialInfo::Write(application::util::BinaryVectorWriter& Writer, TerrainSceneFile& SceneFile)
	{
		Writer.WriteInteger(mTexIndex, sizeof(uint32_t));
		Writer.WriteInteger(_04, sizeof(uint32_t));
		Writer.WriteInteger(_08, sizeof(uint32_t));
		Writer.WriteInteger(_0C, sizeof(uint32_t));
		Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mUScale), 4);
		Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mVScale), 4);
		Writer.WriteInteger(_18, sizeof(uint32_t));
		Writer.WriteInteger(_1C, sizeof(uint32_t));
		Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mFabricationMicro), 4);
		Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mFabricationTiling), 4);
		Writer.WriteInteger(_28, sizeof(uint32_t));
		Writer.WriteInteger(mHeightCorrection, sizeof(uint32_t));
		mColor.Write(Writer, SceneFile);
		Writer.WriteInteger(_3C, sizeof(uint32_t));
	}

	void TerrainSceneFile::ResFile::Read(application::util::BinaryVectorReader& Reader)
	{
		mType = static_cast<ResFileType>(Reader.ReadUInt32());
		_04 = Reader.ReadUInt32();
		_08 = Reader.ReadUInt32();
		_0C = Reader.ReadUInt32();
		_10 = Reader.ReadUInt32();
		_14 = Reader.ReadUInt32();
		_18 = Reader.ReadUInt32();
		_1C = Reader.ReadUInt32();

		mMinHeight = Reader.ReadFloat();
		mMaxHeight = Reader.ReadFloat();

		_28 = Reader.ReadUInt32();
		_2C = Reader.ReadUInt32();
		_30 = Reader.ReadUInt32();
		_34 = Reader.ReadUInt32();
		_38 = Reader.ReadUInt32();
		_3C = Reader.ReadUInt32();
	}

	void TerrainSceneFile::ResFile::Write(application::util::BinaryVectorWriter& Writer, TerrainSceneFile& SceneFile)
	{
		Writer.WriteInteger(mType, sizeof(uint32_t));
		Writer.WriteInteger(_04, sizeof(uint32_t));
		Writer.WriteInteger(_08, sizeof(uint32_t));
		Writer.WriteInteger(_0C, sizeof(uint32_t));
		Writer.WriteInteger(_10, sizeof(uint32_t));
		Writer.WriteInteger(_14, sizeof(uint32_t));
		Writer.WriteInteger(_18, sizeof(uint32_t));
		Writer.WriteInteger(_1C, sizeof(uint32_t));
		Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mMinHeight), 4);
		Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mMaxHeight), 4);
		Writer.WriteInteger(_28, sizeof(uint32_t));
		Writer.WriteInteger(_2C, sizeof(uint32_t));
		Writer.WriteInteger(_30, sizeof(uint32_t));
		Writer.WriteInteger(_34, sizeof(uint32_t));
		Writer.WriteInteger(_38, sizeof(uint32_t));
		Writer.WriteInteger(_3C, sizeof(uint32_t));
	}

	void TerrainSceneFile::ResArea::Read(application::util::BinaryVectorReader& Reader)
	{
		mX = Reader.ReadFloat();
		mZ = Reader.ReadFloat();
		mScale = Reader.ReadFloat();

		_0C = Reader.ReadUInt32();
		_10 = Reader.ReadUInt32();
		_14 = Reader.ReadUInt32();
		_18 = Reader.ReadUInt32();
		_1C = Reader.ReadUInt32();

		mFileCount = Reader.ReadUInt32();

		_24 = Reader.ReadUInt32();
		_28 = Reader.ReadUInt32();
		_2C = Reader.ReadUInt32();
		_30 = Reader.ReadUInt32();
		_34 = Reader.ReadUInt32();
		_38 = Reader.ReadUInt32();
		_3C = Reader.ReadUInt32();
		_40 = Reader.ReadUInt32();
		_44 = Reader.ReadUInt32();
		_48 = Reader.ReadUInt32();
		_4C = Reader.ReadUInt32();
		_50 = Reader.ReadUInt32();
		_54 = Reader.ReadUInt32();
		_58 = Reader.ReadUInt32();
		_5C = Reader.ReadUInt32();

		mFilenameOffset.Read(Reader);
		mFileArrayOffset.Read(Reader);

		mFileResources.resize(mFileCount);
		for (size_t i = 0; i < mFileCount; i++)
		{
			mFileResources[i].Read(Reader);
		}
	}

	void TerrainSceneFile::ResArea::Write(application::util::BinaryVectorWriter& Writer, TerrainSceneFile& SceneFile)
	{
		mFileCount = mFileResources.size();

		Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mX), 4);
		Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mZ), 4);
		Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mScale), 4);

		Writer.WriteInteger(_0C, sizeof(uint32_t));
		Writer.WriteInteger(_10, sizeof(uint32_t));
		Writer.WriteInteger(_14, sizeof(uint32_t));
		Writer.WriteInteger(_18, sizeof(uint32_t));
		Writer.WriteInteger(_1C, sizeof(uint32_t));
		Writer.WriteInteger(mFileCount, sizeof(uint32_t));
		Writer.WriteInteger(_24, sizeof(uint32_t));
		Writer.WriteInteger(_28, sizeof(uint32_t));
		Writer.WriteInteger(_2C, sizeof(uint32_t));
		Writer.WriteInteger(_30, sizeof(uint32_t));
		Writer.WriteInteger(_34, sizeof(uint32_t));
		Writer.WriteInteger(_38, sizeof(uint32_t));
		Writer.WriteInteger(_3C, sizeof(uint32_t));
		Writer.WriteInteger(_40, sizeof(uint32_t));
		Writer.WriteInteger(_44, sizeof(uint32_t));
		Writer.WriteInteger(_48, sizeof(uint32_t));
		Writer.WriteInteger(_4C, sizeof(uint32_t));
		Writer.WriteInteger(_50, sizeof(uint32_t));
		Writer.WriteInteger(_54, sizeof(uint32_t));
		Writer.WriteInteger(_58, sizeof(uint32_t));
		Writer.WriteInteger(_5C, sizeof(uint32_t));
		mFilenameOffset.Skip(Writer, SceneFile);
		mFileArrayOffset.Skip(Writer, SceneFile);


		mFileArrayOffset.Write(Writer, SceneFile);
		for (auto& FileRes : mFileResources)
		{
			FileRes.Skip(Writer, SceneFile);
		}
		for (auto& FileRes : mFileResources)
		{
			FileRes.Write(Writer, SceneFile);
		}
	}

	void TerrainSceneFile::ResTerrainScene::Read(application::util::BinaryVectorReader& Reader)
	{
		Reader.ReadStruct(&mMagic[0], 4);
		mMajorVersion = Reader.ReadUInt8();
		mMinorVersion = Reader.ReadUInt8();
		Reader.ReadStruct(&mPadding[0], 2);
		mIsLittleEndian = Reader.ReadUInt32();
		mStringPoolOffset.Read(Reader);
		mWorldScale = Reader.ReadFloat();
		mHeightScale = Reader.ReadFloat();
		_18 = Reader.ReadUInt32();
		_1C = Reader.ReadUInt32();
		_20 = Reader.ReadUInt32();
		_24 = Reader.ReadUInt32();
		_28 = Reader.ReadUInt32();
		_2C = Reader.ReadUInt32();
		mImageInfoCount = Reader.ReadUInt32();
		mMatInfoCount = Reader.ReadUInt32();
		mAreaCount = Reader.ReadUInt32();
		mKnotLevel = Reader.ReadUInt32();
		_40 = Reader.ReadUInt32();
		_44 = Reader.ReadUInt32();
		_48 = Reader.ReadUInt32();
		_4C = Reader.ReadUInt32();
		mAreaX = Reader.ReadFloat();
		mAreaZ = Reader.ReadFloat();
		mAreaScale = Reader.ReadFloat();
		_5C = Reader.ReadUInt32();
		_60 = Reader.ReadUInt32();
		_64 = Reader.ReadUInt32();
		_68 = Reader.ReadUInt32();
		_6C = Reader.ReadUInt32();

		mImageInfoArrayOffset.Read(Reader);
		mMatInfoArrayOffset.Read(Reader);
		mAreaArrayOffset.Read(Reader);

		mImageInfo.resize(mImageInfoCount);
		Reader.Seek(mImageInfoArrayOffset.mOffset + mImageInfoArrayOffset.mValue, application::util::BinaryVectorReader::Position::Begin);
		for (size_t i = 0; i < mImageInfoCount; i++)
		{
			mImageInfo[i].Read(Reader);
		}

		mMaterialInfo.resize(mMatInfoCount);
		Reader.Seek(mMatInfoArrayOffset.mOffset + mMatInfoArrayOffset.mValue, application::util::BinaryVectorReader::Position::Begin);
		for (size_t i = 0; i < mMatInfoCount; i++)
		{
			mMaterialInfo[i].Read(Reader);
		}

		mAreas.resize(mAreaCount);
		Reader.Seek(mAreaArrayOffset.mOffset + mAreaArrayOffset.mValue, application::util::BinaryVectorReader::Position::Begin);
		for (size_t i = 0; i < mAreaCount; i++)
		{
			mAreas[i].Read(Reader);
		}

		application::util::Logger::Info("TerrainSceneFile", "Decoded successfully");
	}

	void TerrainSceneFile::ResTerrainScene::Write(application::util::BinaryVectorWriter& Writer, TerrainSceneFile& SceneFile)
	{
		mImageInfoCount = mImageInfo.size();
		mMatInfoCount = mMaterialInfo.size();
		mAreaCount = mAreas.size();

		Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mMagic[0]), 4);
		Writer.WriteInteger(mMajorVersion, sizeof(uint8_t));
		Writer.WriteInteger(mMinorVersion, sizeof(uint8_t));
		Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mPadding[0]), 2);
		Writer.WriteInteger(mIsLittleEndian, sizeof(uint32_t));
		mStringPoolOffset.Skip(Writer, SceneFile);
		Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mWorldScale), 4);
		Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mHeightScale), 4);
		Writer.WriteInteger(_18, sizeof(uint32_t));
		Writer.WriteInteger(_1C, sizeof(uint32_t));
		Writer.WriteInteger(_20, sizeof(uint32_t));
		Writer.WriteInteger(_24, sizeof(uint32_t));
		Writer.WriteInteger(_28, sizeof(uint32_t));
		Writer.WriteInteger(_2C, sizeof(uint32_t));
		Writer.WriteInteger(mImageInfoCount, sizeof(uint32_t));
		Writer.WriteInteger(mMatInfoCount, sizeof(uint32_t));
		Writer.WriteInteger(mAreaCount, sizeof(uint32_t));
		Writer.WriteInteger(mKnotLevel, sizeof(uint32_t));
		Writer.WriteInteger(_40, sizeof(uint32_t));
		Writer.WriteInteger(_44, sizeof(uint32_t));
		Writer.WriteInteger(_48, sizeof(uint32_t));
		Writer.WriteInteger(_4C, sizeof(uint32_t));
		Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mAreaX), 4);
		Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mAreaZ), 4);
		Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mAreaScale), 4);
		Writer.WriteInteger(_5C, sizeof(uint32_t));
		Writer.WriteInteger(_60, sizeof(uint32_t));
		Writer.WriteInteger(_64, sizeof(uint32_t));
		Writer.WriteInteger(_68, sizeof(uint32_t));
		Writer.WriteInteger(_6C, sizeof(uint32_t));

		mImageInfoArrayOffset.Skip(Writer, SceneFile);
		mMatInfoArrayOffset.Skip(Writer, SceneFile);
		mAreaArrayOffset.Skip(Writer, SceneFile);

		mStringPoolOffset.Write(Writer, SceneFile);
		mImageInfoArrayOffset.Write(Writer, SceneFile);
		for (auto& ImgInfo : mImageInfo)
		{
			ImgInfo.Skip(Writer, SceneFile);
		}
		for (auto& ImgInfo : mImageInfo)
		{
			ImgInfo.Write(Writer, SceneFile);
		}

		mMatInfoArrayOffset.Write(Writer, SceneFile);
		for (auto& MatInfo : mMaterialInfo)
		{
			MatInfo.Skip(Writer, SceneFile);
		}
		for (auto& MatInfo : mMaterialInfo)
		{
			MatInfo.Write(Writer, SceneFile);
		}

		mAreaArrayOffset.Write(Writer, SceneFile);
		for (auto& Area : mAreas)
		{
			Area.Skip(Writer, SceneFile);
		}
		for (auto& Area : mAreas)
		{
			Area.Write(Writer, SceneFile);
		}
	}

	std::vector<unsigned char> TerrainSceneFile::ToBinary()
	{
		application::util::BinaryVectorWriter Writer;
		mTerrainScene.Write(Writer, *this);

		std::map<std::string, ResStringPtr*> Strings;
		std::map<std::wstring, ResWStringPtr*> WStrings;
		std::map<std::string, uint64_t> StringOffsets;
		std::map<std::wstring, uint64_t> WStringOffsets;

		for (auto& StringPtr : mStringQuery)
		{
			if (std::holds_alternative<ResStringPtr*>(StringPtr))
			{
				Strings[std::get<ResStringPtr*>(StringPtr)->mResString.mString] = std::get<ResStringPtr*>(StringPtr);
			}
			else
			{
				WStrings[std::get<ResWStringPtr*>(StringPtr)->mResWString.mWString] = std::get<ResWStringPtr*>(StringPtr);
			}
		}

		for (auto& [String, Ptr] : Strings)
		{
			Writer.Align(4);
			StringOffsets[String] = Writer.GetPosition();
			Ptr->mResString.Write(Writer, *this);
		}

		for (auto& [String, Ptr] : WStrings)
		{
			Writer.Align(4);
			WStringOffsets[String] = Writer.GetPosition();
			Ptr->mResWString.Write(Writer, *this);
		}

		for (auto& StringPtr : mStringQuery)
		{
			if (std::holds_alternative<ResStringPtr*>(StringPtr))
			{
				std::get<ResStringPtr*>(StringPtr)->Write(Writer, *this, StringOffsets[std::get<ResStringPtr*>(StringPtr)->mResString.mString]);
			}
			else
			{
				std::get<ResWStringPtr*>(StringPtr)->Write(Writer, *this, WStringOffsets[std::get<ResWStringPtr*>(StringPtr)->mResWString.mWString]);
			}
		}

		mStringQuery.clear();

		return Writer.GetData();
	}

	bool TileOverlapsTile(const glm::vec2& t1Pos, float t1Scale, const glm::vec2& t2Pos, float t2Scale)
	{
		if (t1Pos == t2Pos && t1Scale == t2Scale)
			return true;

		float t1ScaleHalf = t1Scale / 2;
		float t2ScaleHalf = t2Scale / 2;

		float t1MinX = t1Pos.x - t1ScaleHalf;
		float t1MaxX = t1Pos.x + t1ScaleHalf;
		float t1MinY = t1Pos.y - t1ScaleHalf;
		float t1MaxY = t1Pos.y + t1ScaleHalf;

		float t2MinX = t2Pos.x - t2ScaleHalf;
		float t2MaxX = t2Pos.x + t2ScaleHalf;
		float t2MinY = t2Pos.y - t2ScaleHalf;
		float t2MaxY = t2Pos.y + t2ScaleHalf;

		glm::vec2 t1C1 = glm::vec2(t1MinX, t1MinY);
		glm::vec2 t1C2 = glm::vec2(t1MaxX, t1MinY);
		glm::vec2 t1C3 = glm::vec2(t1MinX, t1MaxY);
		glm::vec2 t1C4 = glm::vec2(t1MaxX, t1MaxY);

		glm::vec2 t2C1 = glm::vec2(t2MinX, t2MinY);
		glm::vec2 t2C2 = glm::vec2(t2MaxX, t2MinY);
		glm::vec2 t2C3 = glm::vec2(t2MinX, t2MaxY);
		glm::vec2 t2C4 = glm::vec2(t2MaxX, t2MaxY);


		if (
			(
				(t1C1.x > t2MinX && t1C1.x < t2MaxX && t1C1.y > t2MinY && t1C1.y < t2MaxY)
				|| (t1C2.x > t2MinX && t1C2.x < t2MaxX && t1C2.y > t2MinY && t1C2.y < t2MaxY)
				|| (t1C3.x > t2MinX && t1C3.x < t2MaxX && t1C3.y > t2MinY && t1C3.y < t2MaxY)
				|| (t1C4.x > t2MinX && t1C4.x < t2MaxX && t1C4.y > t2MinY && t1C4.y < t2MaxY)
				)
			||
			(
				(t2C1.x > t1MinX && t2C1.x < t1MaxX && t2C1.y > t1MinY && t2C1.y < t1MaxY)
				|| (t2C2.x > t1MinX && t2C2.x < t1MaxX && t2C2.y > t1MinY && t2C2.y < t1MaxY)
				|| (t2C3.x > t1MinX && t2C3.x < t1MaxX && t2C3.y > t1MinY && t2C3.y < t1MaxY)
				|| (t2C4.x > t1MinX && t2C4.x < t1MaxX && t2C4.y > t1MinY && t2C4.y < t1MaxY)
				))
		{
			return true;
		}

		return false;
	}

	bool TileInTile(const glm::vec2& t1Pos, float t1Scale, const glm::vec2& t2Pos, float t2Scale)
	{
		if (t1Scale >= t2Scale)
			return false; // It can't be inside if it's bigger/the same size!

		float t1ScaleHalf = t1Scale / 2;
		float t2ScaleHalf = t2Scale / 2;

		if ((t1Pos.x + t1ScaleHalf) <= (t2Pos.x + t2ScaleHalf) // X
			&& (t1Pos.x - t1ScaleHalf) >= (t2Pos.x - t2ScaleHalf)
			&& (t1Pos.y + t1ScaleHalf) <= (t2Pos.y + t2ScaleHalf) // Y
			&& (t1Pos.y - t1ScaleHalf) >= (t2Pos.y - t2ScaleHalf))
		{
			return true;
		}

		return false;
	}

	std::vector<TerrainSceneFile::ResArea*> TerrainSceneFile::GetSectionTilesByPos(float LODScale, glm::vec2 SectionMidpoint, float SectionWidth)
	{
		/*
		const float SectionWidthHalf = 0.5; // add an bit of size to get a edge around the section
		const float LODScaleHalf = LODScale / 2.0;
		const glm::vec2 SPos = glm::vec2(
			(SectionMidpoint.x / SectionWidth) / 6 * 12,
				(SectionMidpoint.y / SectionWidth) / 6 * 12
		);

		std::vector<TerrainSceneFile::ResArea*> ResultTiles;
		for (RelPtr<ResArea>& Area : mTerrainScene.mAreas)
		{
			if (Area.mObj.mScale == LODScale)
			{
				glm::vec2 Center = glm::vec2(Area.mObj.mX, Area.mObj.mZ);

				if (glm::distance(Center, SPos) <= SectionWidthHalf)
				{
					ResultTiles.push_back(&Area.mObj);
				}
			}
		}
		*/

		/*
		std::vector<TerrainSceneFile::ResArea*> ResultTiles;

		const glm::vec2 SectionPos = glm::vec2((SectionMidpoint.x / SectionWidth) / 6 * 12, (SectionMidpoint.y / SectionWidth) / 6 * 12);

		for (size_t x = 0; x < mTerrainScene.mAreas.size(); x++)
		{
			if (!ExactLOD)
			{
				if (mTerrainScene.mAreas[x].mObj.mScale < LODScale)
					continue;
			}
			else
			{
				if (mTerrainScene.mAreas[x].mObj.mScale != LODScale)
					continue;
			}
			const glm::vec2 CandidateTilePos = glm::vec2(mTerrainScene.mAreas[x].mObj.mX, mTerrainScene.mAreas[x].mObj.mZ);
			const float CandidateTileScale = mTerrainScene.mAreas[x].mObj.mScale;

			if (TileOverlapsTile(CandidateTilePos, CandidateTileScale, SectionPos, 2))
			{
				float PercentageHasHigherDetail = 0;
				for (size_t y = 0; y < mTerrainScene.mAreas.size(); y++) {
					if (x == y)
						continue;

					if (!ExactLOD)
					{
						if (mTerrainScene.mAreas[y].mObj.mScale < LODScale)
							continue;
					}
					else
					{
						if (mTerrainScene.mAreas[y].mObj.mScale != LODScale)
							continue;
					}

					const glm::vec2 ThisTilePos = glm::vec2(mTerrainScene.mAreas[y].mObj.mX, mTerrainScene.mAreas[y].mObj.mZ);
					const float ThisTileScale = mTerrainScene.mAreas[y].mObj.mScale;

					if (TileInTile(ThisTilePos, ThisTileScale, CandidateTilePos, CandidateTileScale))
					{
						PercentageHasHigherDetail += (ThisTileScale * ThisTileScale) / (CandidateTileScale * CandidateTileScale);
						if (PercentageHasHigherDetail >= 1.0f)
							break;
					}
				}

				if (PercentageHasHigherDetail < 1.0f)
					ResultTiles.push_back(&mTerrainScene.mAreas[x].mObj);
			}
		}
		*/

		std::vector<TerrainSceneFile::ResArea*> ResultTiles;

		const glm::vec2 SectionPos = glm::vec2((SectionMidpoint.x / SectionWidth) / 6 * 12, (SectionMidpoint.y / SectionWidth) / 6 * 12);

		for (size_t x = 0; x < mTerrainScene.mAreas.size(); x++)
		{
			if (mTerrainScene.mAreas[x].mObj.mScale > LODScale)
				continue;

			const glm::vec2 CandidateTilePos = glm::vec2(mTerrainScene.mAreas[x].mObj.mX, mTerrainScene.mAreas[x].mObj.mZ);
			const float CandidateTileScale = mTerrainScene.mAreas[x].mObj.mScale;

			if (TileOverlapsTile(CandidateTilePos, CandidateTileScale, SectionPos, 2))
			{
				ResultTiles.push_back(&mTerrainScene.mAreas[x].mObj); 
			}
		}

		return ResultTiles;
	}


	void TerrainSceneFile::Initialize(std::vector<unsigned char> Data)
	{
		application::util::BinaryVectorReader Reader(Data);

		mTerrainScene.Read(Reader);
	}

	TerrainSceneFile::TerrainSceneFile(const std::string& Path)
	{
		Initialize(application::util::FileUtil::ReadFile(Path));
	}

	TerrainSceneFile::TerrainSceneFile(std::vector<unsigned char> Data)
	{
		Initialize(Data);
	}
}