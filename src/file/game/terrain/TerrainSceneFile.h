#pragma once

#include <vector>
#include <string>
#include <util/BinaryVectorReader.h>
#include <util/BinaryVectorWriter.h>
#include <util/Logger.h>
#include <glm/vec2.hpp>
#include <variant>

namespace application::file::game::terrain
{
	class TerrainSceneFile
	{
	public:
		struct ReadableWriteable
		{
			virtual void Read(application::util::BinaryVectorReader& Reader) = 0;

			virtual void Write(application::util::BinaryVectorWriter& Writer, TerrainSceneFile& SceneFile) {};
			virtual void Skip(application::util::BinaryVectorWriter& Writer, TerrainSceneFile& SceneFile) {};
		};

		struct RelOffset : public ReadableWriteable
		{
			uint32_t mValue = 0; //Relative offset
			uint64_t mOffset = 0; //this struct offset

			uint32_t mBaseOffset = 0;

			virtual void Read(application::util::BinaryVectorReader& Reader) override;
			virtual void Write(application::util::BinaryVectorWriter& Writer, TerrainSceneFile& SceneFile) override;
			virtual void Skip(application::util::BinaryVectorWriter& Writer, TerrainSceneFile& SceneFile) override;
		};

		template <typename T>
		struct RelPtr : public ReadableWriteable
		{
			uint32_t mOffset;
			T mObj;

			virtual void Read(application::util::BinaryVectorReader& Reader) override
			{
				mOffset = Reader.ReadUInt32();
				uint64_t Jumpback = Reader.GetPosition();
				
				Reader.Seek(mOffset - 4, application::util::BinaryVectorReader::Position::Current);

				ReadableWriteable* BaseObj = dynamic_cast<ReadableWriteable*>(&mObj);
				if (BaseObj)
				{
					BaseObj->Read(Reader);
				}
				else
				{
					application::util::Logger::Error("TerrainSceneFile", "Could not cast T to ReadableWriteable*, thrown while reading");
				}

				Reader.Seek(Jumpback, application::util::BinaryVectorReader::Position::Begin);
			}

			virtual void Write(application::util::BinaryVectorWriter& Writer, TerrainSceneFile& SceneFile) override
			{
				uint32_t Offset = mOffset;
				mOffset = Writer.GetPosition() - mOffset;

				ReadableWriteable* BaseObj = dynamic_cast<ReadableWriteable*>(&mObj);
				if (BaseObj)
				{
					BaseObj->Write(Writer, SceneFile);
				}
				else
				{
					application::util::Logger::Error("TerrainSceneFile", "Could not cast T to ReadableWriteable*, thrown while writing");
				}

				uint64_t Jumpback = Writer.GetPosition();
				Writer.Seek(Offset, application::util::BinaryVectorWriter::Position::Begin);
				Writer.WriteInteger(mOffset, sizeof(uint32_t));
				Writer.Seek(Jumpback, application::util::BinaryVectorWriter::Position::Begin);
			}

			virtual void Skip(application::util::BinaryVectorWriter& Writer, TerrainSceneFile& SceneFile) override
			{
				mOffset = Writer.GetPosition();
				Writer.Seek(4, application::util::BinaryVectorWriter::Position::Current);
			}
		};

		struct ResString : public ReadableWriteable
		{
			std::string mString;

			virtual void Read(application::util::BinaryVectorReader& Reader) override;
			virtual void Write(application::util::BinaryVectorWriter& Writer, TerrainSceneFile& SceneFile) override;
		};

		struct ResWString : public ReadableWriteable
		{
			std::wstring mWString;

			virtual void Read(application::util::BinaryVectorReader& Reader) override;
			virtual void Write(application::util::BinaryVectorWriter& Writer, TerrainSceneFile& SceneFile) override;
		};

		struct ResStringPtr : public ReadableWriteable
		{
			ResString mResString;
			uint32_t mOffset;

			virtual void Read(application::util::BinaryVectorReader& Reader) override;
			void Write(application::util::BinaryVectorWriter& Writer, TerrainSceneFile& SceneFile, uint64_t AbsoluteOffset);
			virtual void Skip(application::util::BinaryVectorWriter& Writer, TerrainSceneFile& SceneFile) override;
		};

		struct ResWStringPtr : public ReadableWriteable
		{
			ResWString mResWString;
			uint32_t mOffset;

			virtual void Read(application::util::BinaryVectorReader& Reader) override;
			void Write(application::util::BinaryVectorWriter& Writer, TerrainSceneFile& SceneFile, uint64_t AbsoluteOffset);
			virtual void Skip(application::util::BinaryVectorWriter& Writer, TerrainSceneFile& SceneFile) override;
		};

		enum ImageFormat : uint8_t
		{
			cNONE = 0,
			cR8 = 1,
			cR8UI = 2,
			cR8SN = 3,
			cR8I = 4,
			cR16 = 5,
			cR16UI = 6,
			cR16SN = 7,
			cR16I = 8,
			cR16F = 9,
			cRG8 = 10,
			cRG8UI = 11,
			cRG8SN = 12,
			cRG8I = 13,
			cRGB565 = 14,
			cA1BGR5 = 15,
			cRGBA4 = 16,
			cRGB5A1 = 17,
			cR32UI = 18,
			cR32I = 19,
			cR32F = 20,
			cRG16 = 21,
			cRG16UI = 22,
			cRG16SN = 23,
			cRG16I = 24,
			cRG16F = 25,
			cR11G11B10F = 26,
			cRGB10A2 = 27,
			cRGB10A2UI = 28,
			cRGBA8 = 29,
			cRGBA8UI = 30,
			cRGBA8SN = 31,
			cRGBA8I = 32,
			cRGBA8_SRGB = 33,
			cRGB10A2_1 = 34,
			cRGB10A2UI_1 = 35,
			cRG32UI = 36,
			cRG32I = 37,
			cRG32F = 38,
			cRGBA16 = 39,
			cRGBA16UI = 40,
			cRGBA16SN = 41,
			cRGBA16I = 42,
			cRGBA16F = 43,
			cRGBA32UI = 44,
			cRGBA32I = 45,
			cRGBA32F = 46,
			cRGBA_DXT1 = 47,
			cRGBA_DXT1_SRGB = 48,
			cRGBA_DXT3 = 49,
			cRGBA_DXT3_SRGB = 50,
			cRGBA_DXT5 = 51,
			cRGBA_DXT5_SRGB = 52,
			cRGTC1_UNORM = 53,
			cRGTC1_SNORM = 54,
			cRGTC2_UNORM = 55,
			cRGTC2_SNORM = 56,
			cBPTC_SFLOAT = 57,
			cBPTC_UFLOAT = 58,
			cBPTC_UNORM = 59,
			cBPTC_UNORM_SRGB = 60,
			cRGBA_ASTC_4x4 = 61,
			cRGBA_ASTC_4x4_SRGB = 62,
			cRGBA_ASTC_5x4 = 63,
			cRGBA_ASTC_5x4_SRGB = 64,
			cRGBA_ASTC_5x5 = 65,
			cRGBA_ASTC_5x5_SRGB = 66,
			cRGBA_ASTC_6x5 = 67,
			cRGBA_ASTC_6x5_SRGB = 68,
			cRGBA_ASTC_6x6 = 69,
			cRGBA_ASTC_6x6_SRGB = 70,
			cRGBA_ASTC_8x5 = 71,
			cRGBA_ASTC_8x5_SRGB = 72,
			cRGBA_ASTC_8x6 = 73,
			cRGBA_ASTC_8x6_SRGB = 74,
			cRGBA_ASTC_8x8 = 75,
			cRGBA_ASTC_8x8_SRGB = 76,
			cRGBA_ASTC_10x5 = 77,
			cRGBA_ASTC_10x5_SRGB = 78,
			cRGBA_ASTC_10x6 = 79,
			cRGBA_ASTC_10x6_SRGB = 80,
			cRGBA_ASTC_10x8 = 81,
			cRGBA_ASTC_10x8_SRGB = 82,
			cRGBA_ASTC_10x10 = 83,
			cRGBA_ASTC_10x10_SRGB = 84,
			cRGBA_ASTC_12x10 = 85,
			cRGBA_ASTC_12x10_SRGB = 86,
			cRGBA_ASTC_12x12 = 87,
			cRGBA_ASTC_12x12_SRGB = 88,
			cDEPTH16 = 89,
			cDEPTH32F = 90,
			cDEPTH24_STENCIL8 = 91,
			cDEPTH32F_STENCIL8 = 92,
		};
		
		enum ResFileType : uint32_t 
		{
			HeightMap = 0x0000,
			NormalMap = 0x0100,
			Material = 0x2000,
			Grass = 0x4000,
			Water = 0x4100,
			Bake = 0x5000,
		};

		enum DefFilterType : uint32_t 
		{
			Linear = 0,
			Nearest = 1,
		};

		struct Color3f : public ReadableWriteable
		{
			float mR;
			float mG;
			float mB;

			virtual void Read(application::util::BinaryVectorReader& Reader) override;
			virtual void Write(application::util::BinaryVectorWriter& Writer, TerrainSceneFile& SceneFile) override;
		};

		struct Color4f : public ReadableWriteable
		{
			float mR;
			float mG;
			float mB;
			float mA;

			virtual void Read(application::util::BinaryVectorReader& Reader) override;
			virtual void Write(application::util::BinaryVectorWriter& Writer, TerrainSceneFile& SceneFile) override;
		};

		struct ResImageInfo : public ReadableWriteable
		{
			ResFileType mType;
			uint32_t _04;
			uint32_t _08;
			uint32_t _0C;
			uint16_t mWidth;
			uint16_t _12;
			uint8_t mMargin;
			uint8_t _15[0x3]; // might just be padding idk
			uint32_t mMipCount;
			ImageFormat mFormat;
			uint8_t _1D[0x3]; // probably just padding
			uint32_t mArchiveLevel; // 2 bits
			DefFilterType mDefFilterType; // bottom bit, 0 = linear, 1 = nearest
			uint32_t _28;
			uint32_t _2C;
			uint32_t mDependencyCount;
			uint32_t mDependencies[3];
			Color4f mClearColor;
			uint32_t mFlags;
			uint32_t _54;
			uint32_t _58;
			uint32_t _5C;
			ResStringPtr mNameOffset;
			ResStringPtr mFileTypeOffset;
			ResWStringPtr mDescriptionOffset;
			ResStringPtr mExtensionOffset;
			uint32_t _70;
			uint32_t _74;
			uint32_t _78;
			uint32_t _7C;

			virtual void Read(application::util::BinaryVectorReader& Reader) override;
			virtual void Write(application::util::BinaryVectorWriter& Writer, TerrainSceneFile& SceneFile) override;
		};

		struct ResMaterialInfo : public ReadableWriteable
		{
			uint32_t mTexIndex;
			uint32_t _04;
			uint32_t _08;
			uint32_t _0C;
			float mUScale;
			float mVScale;
			uint32_t _18;
			uint32_t _1C;
			float mFabricationMicro;
			float mFabricationTiling;
			uint32_t _28;
			uint32_t mHeightCorrection;
			Color3f mColor;
			uint32_t _3C;

			virtual void Read(application::util::BinaryVectorReader& Reader) override;
			virtual void Write(application::util::BinaryVectorWriter& Writer, TerrainSceneFile& SceneFile) override;
		};

		struct ResFile : public ReadableWriteable
		{
			ResFileType mType = ResFileType::HeightMap;
			uint32_t _04 = 0;
			uint32_t _08 = 0;
			uint32_t _0C = 0;
			uint32_t _10 = 0;
			uint32_t _14 = 0;
			uint32_t _18 = 0;
			uint32_t _1C = 0;
			float mMinHeight = 0.0f;
			float mMaxHeight = 0.0f;
			uint32_t _28 = 0;
			uint32_t _2C = 0;
			uint32_t _30 = 0;
			uint32_t _34 = 0;
			uint32_t _38 = 0;
			uint32_t _3C = 0;

			virtual void Read(application::util::BinaryVectorReader& Reader) override;
			virtual void Write(application::util::BinaryVectorWriter& Writer, TerrainSceneFile& SceneFile) override;
		};

		struct ResArea : public ReadableWriteable
		{
			float mX = 0.0f;
			float mZ = 0.0f;
			float mScale = 0.0f;
			uint32_t _0C = 0;
			uint32_t _10 = 0;
			uint32_t _14 = 0;
			uint32_t _18 = 0;
			uint32_t _1C = 0;
			uint32_t mFileCount = 0;
			uint32_t _24 = 0;
			uint32_t _28 = 0;
			uint32_t _2C = 0;
			uint32_t _30 = 0;
			uint32_t _34 = 0;
			uint32_t _38 = 0;
			uint32_t _3C = 0;
			uint32_t _40 = 0; //All other unknowns are pretty much always 0, just this one equals 1 most of the times
			uint32_t _44 = 0;
			uint32_t _48 = 0;
			uint32_t _4C = 0;
			uint32_t _50 = 0;
			uint32_t _54 = 0;
			uint32_t _58 = 0;
			uint32_t _5C = 0;
			ResStringPtr mFilenameOffset;
			RelOffset mFileArrayOffset;

			std::vector<RelPtr<ResFile>> mFileResources;

			virtual void Read(application::util::BinaryVectorReader& Reader) override;
			virtual void Write(application::util::BinaryVectorWriter& Writer, TerrainSceneFile& SceneFile) override;
		};

		struct ResTerrainScene : public ReadableWriteable
		{
			char mMagic[4];
			uint8_t mMajorVersion;
			uint8_t mMinorVersion;
			uint8_t mPadding[2];
			uint32_t mIsLittleEndian;
			RelOffset mStringPoolOffset;
			float mWorldScale;
			float mHeightScale;
			uint32_t _18;
			uint32_t _1C;
			uint32_t _20;
			uint32_t _24;
			uint32_t _28;
			uint32_t _2C;
			uint32_t mImageInfoCount;
			uint32_t mMatInfoCount;
			uint32_t mAreaCount;
			uint32_t mKnotLevel;
			uint32_t _40;
			uint32_t _44;
			uint32_t _48;
			uint32_t _4C;
			float mAreaX;
			float mAreaZ;
			float mAreaScale;
			uint32_t _5C;
			uint32_t _60;
			uint32_t _64;
			uint32_t _68;
			uint32_t _6C;
			RelOffset mImageInfoArrayOffset;
			RelOffset mMatInfoArrayOffset;
			RelOffset mAreaArrayOffset;

			std::vector<RelPtr<ResImageInfo>> mImageInfo;
			std::vector<RelPtr<ResMaterialInfo>> mMaterialInfo;
			std::vector<RelPtr<ResArea>> mAreas;

			virtual void Read(application::util::BinaryVectorReader& Reader) override;
			virtual void Write(application::util::BinaryVectorWriter& Writer, TerrainSceneFile& SceneFile) override;
		};

		std::vector<ResArea*> GetSectionTilesByPos(float LODScale, glm::vec2 SectionMidpoint, float SectionWidth);

		void Initialize(std::vector<unsigned char> Data);
		std::vector<unsigned char> ToBinary();

		TerrainSceneFile(const std::string& Path);
		TerrainSceneFile(std::vector<unsigned char> Data);
		TerrainSceneFile() = default;

		ResTerrainScene mTerrainScene;
		std::vector<std::variant<ResStringPtr*, ResWStringPtr*>> mStringQuery;
		bool mModified = false;
	};
}