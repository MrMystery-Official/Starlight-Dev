#include "TexToGoFile.h"

#include <util/BinaryVectorReader.h>
#include <util/BinaryVectorWriter.h>
#include <util/FileUtil.h>
#include <file/game/zstd/ZStdBackend.h>
#include <file/game/texture/TegraSwizzle.h>
#include <util/Logger.h>
#include "Editor.h"
#include <zstd.h>
#include <glad/glad.h>
#include <astcenc.h>
#include <manager/UIMgr.h>

namespace application::file::game::texture
{
	bool TexToGoFile::Parse()
	{
		mReader.ReadStruct(&mHeader, sizeof(TexToGoFile::Header));
		if (mHeader.mMagic[0] != '6' || mHeader.mMagic[1] != 'P' || mHeader.mMagic[2] != 'K' || mHeader.mMagic[3] != '0')
		{
			application::util::Logger::Error("TexToGoFile", "Wrong magic, expected 6PK0");
			return false;
		}

		if (mHeader.mVersion != 0x11)
		{
			application::util::Logger::Error("TexToGoFile", "Wrong version, expected 0x11");
			return false;
		}

		mWidth = mReader.ReadUInt16();
		mHeight = mReader.ReadUInt16();
		mDepth = mReader.ReadUInt16();
		mMipCount = mReader.ReadUInt8();
		mUnk1 = mReader.ReadUInt8();
		mUnk2 = mReader.ReadUInt8();
		mReader.ReadUInt16(); //Padding

		mFormatFlag = mReader.ReadUInt8();
		mFormatSetting = mReader.ReadUInt32();

		mCompSelectR = mReader.ReadUInt8();
		mCompSelectG = mReader.ReadUInt8();
		mCompSelectB = mReader.ReadUInt8();
		mCompSelectA = mReader.ReadUInt8();

		mReader.ReadStruct(reinterpret_cast<char*>(&mHash[0]), 32);

		mFormat = mReader.ReadUInt16();
		mUnk3 = mReader.ReadUInt16();

		mTextureSetting1 = mReader.ReadUInt32();
		mTextureSetting2 = mReader.ReadUInt32();
		mTextureSetting3 = mReader.ReadUInt32();
		mTextureSetting4 = mReader.ReadUInt32();

		//Copied from https://github.com/KillzXGaming/Switch-Toolbox/blob/master/File_Format_Library/FileFormats/Texture/TXTG.cs
		//Dumb hack. Terrain is oddly 8x8 astc, but the format seems to be 0x101
		//Use some of the different texture settings, as they likely configure the astc blocks in some way
		if (mTextureSetting2 == 32628)
		{
			mFormat = 0x101;
		}
		else if (mTextureSetting2 == 32631)
		{
			mFormat = 0x102;
		}

		mSurfaces.resize(mMipCount * mDepth);
		for (int i = 0; i < mMipCount * mDepth; i++)
		{
			uint16_t ArrayIndex = mReader.ReadUInt16();
			uint8_t MipLevel = mReader.ReadUInt8();
			uint8_t SurfaceCount = mReader.ReadUInt8();
			mSurfaces[i] = TexToGoFile::Surface
			{
				.mArrayIndex = ArrayIndex,
				.mMipLevel = MipLevel,
				.mSurfaceCount = SurfaceCount
			};
		}
		for (int i = 0; i < mMipCount * mDepth; i++)
		{
			mSurfaces[i].mZSTDCompressedSize = mReader.ReadUInt32();
			mSurfaces[i].mUnk = mReader.ReadUInt32();
		}

		mPolishedFormat = GetFormat();
		if (mPolishedFormat == TextureFormat::Format::UNKNOWN)
		{
			application::util::Logger::Error("TexToGoFile", "Texture format unknown: %u, %s", (int)mFormat, mFileName.c_str());
			return false;
		}

		uint32_t BlkWidth = TextureFormat::GetFormatBlockWidth(mPolishedFormat);
		uint32_t BlkHeight = TextureFormat::GetFormatBlockHeight(mPolishedFormat);
		uint32_t BytesPerPixel = TextureFormat::GetFormatBytesPerBlock(mPolishedFormat);

		for (uint16_t i = 0; i < mDepth; i++)
		{
			std::vector<unsigned char> CompressedSurface(mSurfaces[i].mZSTDCompressedSize);
			mReader.ReadStruct(reinterpret_cast<char*>(CompressedSurface.data()), mSurfaces[i].mZSTDCompressedSize);

			mSurfaces[i].mWidth = std::max(1, mWidth >> mSurfaces[i].mMipLevel);
			mSurfaces[i].mHeight = std::max(1, mHeight >> mSurfaces[i].mMipLevel);

			mSurfaces[i].mPolishedFormat = mPolishedFormat;

			std::vector<unsigned char> Decompressed;

			Decompressed.resize(ZSTD_getFrameContentSize(CompressedSurface.data(), CompressedSurface.size()));
			ZSTD_DCtx* const DCtx = ZSTD_createDCtx();
			ZSTD_decompressDCtx(DCtx, (void*)Decompressed.data(), Decompressed.size(), CompressedSurface.data(), CompressedSurface.size());
			ZSTD_freeDCtx(DCtx);

			uint32_t BlockHeight = TegraSwizzle::GetBlockHeight(TegraSwizzle::DIV_ROUND_UP(mSurfaces[i].mHeight, BlkHeight));
			if (BlockHeight == 0) BlockHeight = BlkHeight;

			mSurfaces[i].mData = TegraSwizzle::Deswizzle(mSurfaces[i].mWidth, mSurfaces[i].mHeight, 1, BlkWidth, BlkHeight, 1, 1, BytesPerPixel, 0, std::log10(BlockHeight) / std::log10(2), Decompressed);
			mSurfaces[i].mDataSize = mSurfaces[i].mData.size();
		}

		/*
				case 0x101:
			return TextureFormat::Format::ASTC_8x5_UNORM;
		case 0x102:
			return TextureFormat::Format::ASTC_8x8_UNORM;
		case 0x105:
			return TextureFormat::Format::ASTC_8x8_SRGB;
		case 0x109:
			return TextureFormat::Format::ASTC_4x4_SRGB;
		*/

		if (!application::manager::UIMgr::gASTCSupported)
		{
			if (mFormat == 0x101 || mFormat == 0x102 || mFormat == 0x105 || mFormat == 0x109)
			{
				unsigned int BlockX = 0;
				unsigned int BlockY = 0;

				switch (mFormat)
				{
				case 0x101:
					BlockX = 8;
					BlockY = 5;
					break;
				case 0x102:
				case 0x105:
					BlockX = 8;
					BlockY = 8;
					break;
				case 0x109:
					BlockX = 4;
					BlockY = 4;
					break;
				default:
					break;
				}

				if (BlockX > 0 && BlockY > 0)
				{
					astcenc_profile profile = ASTCENC_PRF_LDR;  // Low Dynamic Range
					astcenc_config config;
					astcenc_context* ctx;

					astcenc_error status = astcenc_config_init(profile, BlockX, BlockY, 1, ASTCENC_PRE_FAST, ASTCENC_FLG_DECOMPRESS_ONLY, &config);
					if (status != ASTCENC_SUCCESS) {
						std::cerr << "Failed to initialize ASTC config!" << std::endl;
						return -1;
					}

					status = astcenc_context_alloc(&config, 1, &ctx);
					if (status != ASTCENC_SUCCESS) {
						std::cerr << "Failed to allocate ASTC context!" << std::endl;
						return -1;
					}

					astcenc_swizzle swizzle = { ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, ASTCENC_SWZ_A };

					for (uint16_t i = 0; i < mDepth; i++)
					{
						std::vector<float> RGBAFloatBuffer(mSurfaces[i].mWidth * mSurfaces[i].mHeight * 4, 0);

						astcenc_image image_out;
						image_out.dim_x = mSurfaces[i].mWidth;
						image_out.dim_y = mSurfaces[i].mHeight;
						image_out.dim_z = 1;
						image_out.data_type = ASTCENC_TYPE_F32;  // Decompressed as float
						image_out.data = new void* [1];  // One plane (for 2D textures)
						image_out.data[0] = RGBAFloatBuffer.data();

						// Decompress ASTC
						status = astcenc_decompress_image(ctx, mSurfaces[i].mData.data(), mSurfaces[i].mData.size(), &image_out, &swizzle, 0);
						if (status != ASTCENC_SUCCESS) 
						{
							std::cerr << "ASTC decompression failed!" << std::endl;
							delete[] image_out.data;
							return -1;
						}

						delete[] image_out.data;

						std::vector<unsigned char> RGBABuffer(mSurfaces[i].mWidth* mSurfaces[i].mHeight * 4);
						for (size_t j = 0; j < RGBABuffer.size(); j++)
						{
							RGBABuffer[j] = static_cast<uint8_t>(RGBAFloatBuffer[j] * 255.0f);
						}

						//mSurfaces[i].mData.resize(squish::GetStorageRequirements(mSurfaces[i].mWidth, mSurfaces[i].mHeight, squish::kDxt5));
						
						mSurfaces[i].mData = RGBABuffer;
						mSurfaces[i].mDataSize = mSurfaces[i].mData.size();
						mSurfaces[i].mPolishedFormat = application::file::game::texture::TextureFormat::Format::CPU_DECODED;
						application::util::Logger::Info("TexToGoFile", "Translated ASTC texture surface %u of %u", i, mDepth);
					}

					mPolishedFormat = application::file::game::texture::TextureFormat::Format::CPU_DECODED;
					mFormat = GetRawFormat();

					astcenc_context_free(ctx);
				}
			}
		}

		mLoaded = true;

		{
			application::util::BinaryVectorWriter Writer;
			Writer.WriteBytes("ETEX");
			Writer.WriteByte(0x01);
			Writer.WriteByte(0x00);
			Writer.WriteInteger(mFormat, sizeof(uint16_t));
			Writer.WriteInteger(mDepth, sizeof(uint32_t));
			Writer.WriteInteger(0, sizeof(uint32_t));
			Writer.Seek(8 * mDepth, application::util::BinaryVectorWriter::Position::Current);
			Writer.Align(16);
			std::vector<std::pair<uint32_t, uint32_t>> Offsets;
			for (size_t i = 0; i < mDepth; i++)
			{
				uint32_t Start = Writer.GetPosition();
				Writer.WriteInteger(mSurfaces[i].mWidth, sizeof(uint32_t));
				Writer.WriteInteger(mSurfaces[i].mHeight, sizeof(uint32_t));
				Writer.WriteInteger(0, sizeof(uint64_t));
				Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(mSurfaces[i].mData.data()), mSurfaces[i].mData.size());
				Offsets.push_back({ Start, mSurfaces[i].mData.size() });
				Writer.Align(16);
			}
			uint32_t FileEnd = Writer.GetPosition();
			Writer.Seek(16, application::util::BinaryVectorWriter::Position::Begin);
			for (auto& [Offset, Size] : Offsets)
			{
				Writer.WriteInteger(Offset, sizeof(uint32_t));
				Writer.WriteInteger(Size, sizeof(uint32_t));
			}
			std::vector<unsigned char> Data = Writer.GetData();
			Data.resize(FileEnd);
			application::util::FileUtil::WriteFile(application::util::FileUtil::GetWorkingDirFilePath("Cache/" + mFileName + ".etex"), Data);
		}

		return true;
	}

	bool TexToGoFile::IsCached()
	{
		if (application::util::FileUtil::FileExists(application::util::FileUtil::GetWorkingDirFilePath("Cache/" + mFileName + ".etex")))
		{
			std::vector<unsigned char> Data = application::util::FileUtil::ReadFile(application::util::FileUtil::GetWorkingDirFilePath("Cache/" + mFileName + ".etex"));
			application::util::BinaryVectorReader Reader(Data);
			char Magic[4];
			Reader.ReadStruct(&Magic[0], 4);
			if (Magic[0] != 'E' || Magic[1] != 'T' || Magic[2] != 'E' || Magic[3] != 'X')
			{
				application::util::Logger::Error("TexToGoFile", "Cached TexToGo called %s is invalid", mFileName.c_str());
				return false;
			}
			uint8_t Version = Reader.ReadUInt8();
			if (Version != 0x01)
			{
				application::util::Logger::Error("TexToGoFile", "Cached TexToGo called %s has wrong version, expected 0x01, got %u", mFileName.c_str(), Version);
				return false;
			}
			Reader.Seek(1, application::util::BinaryVectorReader::Position::Current);

			mFormat = Reader.ReadUInt16();
			mPolishedFormat = GetFormat();
			uint32_t SurfCount = Reader.ReadUInt32();
			mDepth = SurfCount;
			mSurfaces.resize(SurfCount);

			Reader.Seek(4, application::util::BinaryVectorReader::Position::Current);

			std::vector<std::pair<uint32_t, uint32_t>> Offsets;
			for (uint32_t i = 0; i < SurfCount; i++)
			{
				uint32_t Offset = Reader.ReadUInt32();
				uint32_t Size = Reader.ReadUInt32();
				Offsets.push_back({ Offset, Size });
			}

			Reader.Align(16);

			uint32_t SurfaceIndex = 0;
			for (auto [Offset, Size] : Offsets)
			{
				Reader.Seek(Offset, application::util::BinaryVectorReader::Position::Begin);

				mSurfaces[SurfaceIndex].mWidth = Reader.ReadUInt32();
				mSurfaces[SurfaceIndex].mHeight = Reader.ReadUInt32();
				Reader.Seek(8, application::util::BinaryVectorReader::Position::Current);
				mSurfaces[SurfaceIndex].mDataSize = Size;
				mSurfaces[SurfaceIndex].mData.resize(Size);
				Reader.ReadStruct(mSurfaces[SurfaceIndex].mData.data(), Size);
				mSurfaces[SurfaceIndex].mArrayIndex = SurfaceIndex;
				mSurfaces[SurfaceIndex].mMipLevel = 0;
				mSurfaces[SurfaceIndex].mPolishedFormat = mPolishedFormat;

				SurfaceIndex++;
			}

			mWidth = mSurfaces[0].mWidth;
			mHeight = mSurfaces[0].mHeight;

			mLoaded = true;

			return mLoaded;
		}
		return false;
	}

	bool& TexToGoFile::IsLoaded()
	{
		return mLoaded;
	}

	TextureFormat::Format& TexToGoFile::GetPolishedFormat()
	{
		return mPolishedFormat;
	}

	uint16_t& TexToGoFile::GetWidth()
	{
		return mWidth;
	}

	uint16_t& TexToGoFile::GetHeight()
	{
		return mHeight;
	}

	uint16_t& TexToGoFile::GetDepth()
	{
		return mDepth;
	}

	uint8_t& TexToGoFile::GetMipCount()
	{
		return mMipCount;
	}

	TexToGoFile::Surface& TexToGoFile::GetSurface(uint16_t ArrayIndex)
	{
		return mSurfaces[mSurfaces.size() > ArrayIndex ? ArrayIndex : 0];
	}

	uint16_t TexToGoFile::GetRawFormat()
	{
		switch (mPolishedFormat)
		{
		case TextureFormat::Format::ASTC_8x5_UNORM:
			return 0x101;
		case TextureFormat::Format::ASTC_8x8_UNORM:
			return 0x102;
		case TextureFormat::Format::ASTC_8x8_SRGB:
			return 0x105;
		case TextureFormat::Format::ASTC_4x4_SRGB:
			return 0x109;
		case TextureFormat::Format::BC1_UNORM:
			return 0x202;
		case TextureFormat::Format::BC1_UNORM_SRGB:
			return 0x203;

		case TextureFormat::Format::BC3_UNORM_SRGB:
			return 0x505;
		case TextureFormat::Format::BC4_UNORM:
			return 0x602;
		case TextureFormat::Format::BC5_UNORM:
			return 0x702;
		case TextureFormat::Format::BC7_UNORM:
			return 0x901;

		case TextureFormat::Format::CPU_DECODED:
			return 0xFFF0;

		default:
			return 0x0; //UNKOWN
		}
	}

	TextureFormat::Format TexToGoFile::GetFormat()
	{
		switch (mFormat)
		{
		case 0x101:
			return TextureFormat::Format::ASTC_8x5_UNORM;
		case 0x102:
			return TextureFormat::Format::ASTC_8x8_UNORM;
		case 0x105:
			return TextureFormat::Format::ASTC_8x8_SRGB;
		case 0x109:
			return TextureFormat::Format::ASTC_4x4_SRGB;
		case 0x202:
			return TextureFormat::Format::BC1_UNORM;
		case 0x203:
			return TextureFormat::Format::BC1_UNORM_SRGB;
		case 0x302:
			return TextureFormat::Format::BC1_UNORM;

		case 0x505:
			return TextureFormat::Format::BC3_UNORM_SRGB;
		case 0x602:
			return TextureFormat::Format::BC4_UNORM;
		case 0x606:
			return TextureFormat::Format::BC4_UNORM;
		case 0x607:
			return TextureFormat::Format::BC4_UNORM;
		case 0x702:
			return TextureFormat::Format::BC5_UNORM;
		case 0x703:
			return TextureFormat::Format::BC5_UNORM;
		case 0x707:
			return TextureFormat::Format::BC5_UNORM;
		case 0x901:
			return TextureFormat::Format::BC7_UNORM;

		case 0xFFF0:
			return TextureFormat::Format::CPU_DECODED;

		default:
			return TextureFormat::Format::UNKNOWN;
		}
	}

	std::string& TexToGoFile::GetFileName()
	{
		return mFileName;
	}

	application::util::BinaryVectorReader& TexToGoFile::GetReader()
	{
		return mReader;
	}

	void TexToGoFile::SetData(std::vector<unsigned char> Bytes)
	{
		mReader = application::util::BinaryVectorReader(Bytes);
	}
}