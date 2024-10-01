#include "TextureToGo.h"

#include "BinaryVectorReader.h"
#include "BinaryVectorWriter.h"
#include "Util.h"
#include "ZStdFile.h"
#include "Logger.h"
#include "TegraSwizzle.h"
#include "Editor.h"
#include <iostream>
#include <zstd.h>

bool TextureToGo::Parse()
{
	mReader.ReadStruct(&mHeader, sizeof(TextureToGo::Header));
	if (mHeader.mMagic[0] != '6' || mHeader.mMagic[1] != 'P' || mHeader.mMagic[2] != 'K' || mHeader.mMagic[3] != '0')
	{
		Logger::Error("TexToGo", "Wrong magic, expected 6PK0");
		return false;
	}

	if (mHeader.mVersion != 0x11)
	{
		Logger::Error("TexToGo", "Wrong version, expected 0x11");
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

	mReader.Read(reinterpret_cast<char*>(&mHash[0]), 32);

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
		mSurfaces[i] = TextureToGo::Surface
		{
			.ArrayIndex = ArrayIndex,
			.MipLevel = MipLevel,
			.SurfaceCount = SurfaceCount
		};
	}
	for (int i = 0; i < mMipCount * mDepth; i++)
	{
		mSurfaces[i].ZSTDCompressedSize = mReader.ReadUInt32();
		mSurfaces[i].Unk = mReader.ReadUInt32();
	}

	mPolishedFormat = GetFormat();
	if (mPolishedFormat == TextureFormat::Format::UNKNOWN)
	{
		Logger::Error("TexToGo", "Texture format unknown: " + std::to_string(mFormat) + ", " + mFileName);
		return false;
	}

	uint32_t BlkWidth = TextureFormat::GetFormatBlockWidth(mPolishedFormat);
	uint32_t BlkHeight = TextureFormat::GetFormatBlockHeight(mPolishedFormat);
	uint32_t BytesPerPixel = TextureFormat::GetFormatBytesPerBlock(mPolishedFormat);

	for (int i = 0; i < mDepth; i++)
	{
		std::vector<unsigned char> CompressedSurface(mSurfaces[i].ZSTDCompressedSize);
		mReader.Read(reinterpret_cast<char*>(CompressedSurface.data()), mSurfaces[i].ZSTDCompressedSize);

		mSurfaces[i].Width = std::max(1, mWidth >> mSurfaces[i].MipLevel);
		mSurfaces[i].Height = std::max(1, mHeight >> mSurfaces[i].MipLevel);

		mSurfaces[i].PolishedFormat = mPolishedFormat;

		std::vector<unsigned char> Decompressed;

		Decompressed.resize(ZSTD_getFrameContentSize(CompressedSurface.data(), CompressedSurface.size()));
		ZSTD_DCtx* const DCtx = ZSTD_createDCtx();
		ZSTD_decompressDCtx(DCtx, (void*)Decompressed.data(), Decompressed.size(), CompressedSurface.data(), CompressedSurface.size());
		ZSTD_freeDCtx(DCtx);

		uint32_t BlockHeight = TegraSwizzle::GetBlockHeight(TegraSwizzle::DIV_ROUND_UP(mSurfaces[i].Height, BlkHeight));
		if (BlockHeight == 0) BlockHeight = BlkHeight;

		mSurfaces[i].Data = TegraSwizzle::Deswizzle(mSurfaces[i].Width, mSurfaces[i].Height, 1, BlkWidth, BlkHeight, 1, 1, BytesPerPixel, 0, std::log10(BlockHeight) / std::log10(2), Decompressed);
		mSurfaces[i].DataSize = mSurfaces[i].Data.size();
	}

	mLoaded = true;

	{
		BinaryVectorWriter Writer;
		Writer.WriteBytes("ETEX");
		Writer.WriteByte(0x01);
		Writer.WriteByte(0x00);
		Writer.WriteInteger(mFormat, sizeof(uint16_t));
		Writer.WriteInteger(mDepth, sizeof(uint32_t));
		Writer.WriteInteger(0, sizeof(uint32_t));
		Writer.Seek(8 * mDepth, BinaryVectorWriter::Position::Current);
		Writer.Align(16);
		std::vector<std::pair<uint32_t, uint32_t>> Offsets;
		for (size_t i = 0; i < mDepth; i++)
		{
			uint32_t Start = Writer.GetPosition();
			Writer.WriteInteger(mSurfaces[i].Width, sizeof(uint32_t));
			Writer.WriteInteger(mSurfaces[i].Height, sizeof(uint32_t));
			Writer.WriteInteger(0, sizeof(uint64_t));
			Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(mSurfaces[i].Data.data()), mSurfaces[i].Data.size());
			Offsets.push_back({ Start, mSurfaces[i].Data.size() });
			Writer.Align(16);
		}
		uint32_t FileEnd = Writer.GetPosition();
		Writer.Seek(16, BinaryVectorWriter::Position::Begin);
		for (auto& [Offset, Size] : Offsets)
		{
			Writer.WriteInteger(Offset, sizeof(uint32_t));
			Writer.WriteInteger(Size, sizeof(uint32_t));
		}
		std::vector<unsigned char> Data = Writer.GetData();
		Data.resize(FileEnd);
		Util::WriteFile(Editor::GetWorkingDirFile("Cache/" + mFileName + ".etex"), Data);
	}

	return true;
}

bool TextureToGo::IsCached()
{
	if (Util::FileExists(Editor::GetWorkingDirFile("Cache/" + mFileName + ".etex")))
	{
		std::vector<unsigned char> Data = Util::ReadFile(Editor::GetWorkingDirFile("Cache/" + mFileName + ".etex"));
		BinaryVectorReader Reader(Data);
		char Magic[4];
		Reader.ReadStruct(&Magic[0], 4);
		if (Magic[0] != 'E' || Magic[1] != 'T' || Magic[2] != 'E' || Magic[3] != 'X')
		{
			Logger::Error("TexToGo", "Cached TexToGo called " + mFileName + " is invalid");
			return false;
		}
		uint8_t Version = Reader.ReadUInt8();
		if (Version != 0x01)
		{
			Logger::Error("TexToGo", "Cached TexToGo called " + mFileName + " has wrong version, expected 0x01, got " + std::to_string(Version));
			return false;
		}
		Reader.Seek(1, BinaryVectorReader::Position::Current);

		mFormat = Reader.ReadUInt16();
		mPolishedFormat = GetFormat();
		uint32_t SurfCount = Reader.ReadUInt32();
		mDepth = SurfCount;
		mSurfaces.resize(SurfCount);

		Reader.Seek(4, BinaryVectorReader::Position::Current);

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
			Reader.Seek(Offset, BinaryVectorReader::Position::Begin);

			mSurfaces[SurfaceIndex].Width = Reader.ReadUInt32();
			mSurfaces[SurfaceIndex].Height = Reader.ReadUInt32();
			Reader.Seek(8, BinaryVectorReader::Position::Current);
			mSurfaces[SurfaceIndex].DataSize = Size;
			mSurfaces[SurfaceIndex].Data.resize(Size);
			Reader.ReadStruct(mSurfaces[SurfaceIndex].Data.data(), Size);
			mSurfaces[SurfaceIndex].ArrayIndex = SurfaceIndex;
			mSurfaces[SurfaceIndex].MipLevel = 0;
			mSurfaces[SurfaceIndex].PolishedFormat = mPolishedFormat;

			SurfaceIndex++;
		}

		mWidth = mSurfaces[0].Width;
		mHeight = mSurfaces[0].Height;

		mLoaded = true;

		return mLoaded;
	}
	return false;
}

bool& TextureToGo::IsLoaded()
{
	return mLoaded;
}

TextureFormat::Format& TextureToGo::GetPolishedFormat()
{
	return mPolishedFormat;
}

uint16_t& TextureToGo::GetWidth()
{
	return mWidth;
}

uint16_t& TextureToGo::GetHeight()
{
	return mHeight;
}

uint16_t& TextureToGo::GetDepth()
{
	return mDepth;
}

uint8_t& TextureToGo::GetMipCount()
{
	return mMipCount;
}

TextureToGo::Surface& TextureToGo::GetSurface(uint16_t ArrayIndex)
{
	return mSurfaces[mSurfaces.size() > ArrayIndex ? ArrayIndex : 0];
}

uint16_t TextureToGo::GetRawFormat()
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

	default:
		return 0x0; //UNKOWN
	}
}

TextureFormat::Format TextureToGo::GetFormat()
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

	default:
		return TextureFormat::Format::UNKNOWN;
	}
}

std::string& TextureToGo::GetFileName()
{
	return mFileName;
}

BinaryVectorReader& TextureToGo::GetReader()
{
	return mReader;
}

void TextureToGo::SetData(std::vector<unsigned char> Bytes)
{
	mReader = BinaryVectorReader(Bytes);
}

std::unordered_map<std::string, TextureToGo> TextureToGoLibrary::mTextures;

bool TextureToGoLibrary::IsTextureLoaded(std::string Name)
{
	return TextureToGoLibrary::mTextures.count(Name);
}

TextureToGo* TextureToGoLibrary::GetTexture(std::string Name, std::string Directory)
{
	if (!TextureToGoLibrary::IsTextureLoaded(Name))
	{
		TextureToGo TexToGo;
		TexToGo.GetFileName() = Name;
		if (!TexToGo.IsCached())
		{
			TexToGo.SetData(Util::ReadFile(Directory.empty() ? Editor::GetRomFSFile("TexToGo/" + Name) : (Directory + "/" + Name)));
			TexToGo.Parse();
		}
		TextureToGoLibrary::mTextures.insert({ Name, TexToGo });
	}
	return &TextureToGoLibrary::mTextures[Name];
}

std::unordered_map<std::string, TextureToGo>& TextureToGoLibrary::GetTextures()
{
	return TextureToGoLibrary::mTextures;
}