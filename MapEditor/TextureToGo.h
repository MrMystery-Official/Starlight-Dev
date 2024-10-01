#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include "TextureFormat.h"
#include "BinaryVectorReader.h"

class TextureToGo
{
public:
	struct Surface
	{
		uint16_t ArrayIndex;
		uint8_t MipLevel;
		uint8_t SurfaceCount = 1;
		uint32_t ZSTDCompressedSize;
		uint32_t Unk = 6;

		std::vector<unsigned char> Data;
		uint32_t DataSize;

		uint32_t Width;
		uint32_t Height;

		TextureFormat::Format PolishedFormat;
	};

	bool Parse();
	bool IsCached();

	uint16_t& GetWidth();
	uint16_t& GetHeight();
	uint16_t& GetDepth();
	uint8_t& GetMipCount();
	Surface& GetSurface(uint16_t ArrayIndex);
	TextureFormat::Format& GetPolishedFormat();
	uint16_t GetRawFormat();
	std::string& GetFileName();
	bool& IsLoaded();
	BinaryVectorReader& GetReader();
	void SetData(std::vector<unsigned char> Bytes);
private:
	TextureFormat::Format GetFormat();

	struct Header
	{
		uint16_t mHeaderSize;
		uint16_t mVersion;
		char mMagic[4];
	};

	Header mHeader;

	uint16_t mWidth;
	uint16_t mHeight;
	uint16_t mDepth;
	uint8_t mMipCount;
	uint8_t mUnk1;
	uint8_t mUnk2;

	uint8_t mFormatFlag;
	uint32_t mFormatSetting;

	uint8_t mCompSelectR;
	uint8_t mCompSelectG;
	uint8_t mCompSelectB;
	uint8_t mCompSelectA;

	uint8_t mHash[32];

	uint16_t mFormat;
	uint16_t mUnk3;

	uint32_t mTextureSetting1;
	uint32_t mTextureSetting2;
	uint32_t mTextureSetting3;
	uint32_t mTextureSetting4;

	std::vector<TextureToGo::Surface> mSurfaces;
	TextureFormat::Format mPolishedFormat;
	std::string mFileName = "";
	BinaryVectorReader mReader;
	bool mLoaded = false;
};

namespace TextureToGoLibrary
{
	extern std::unordered_map<std::string, TextureToGo> mTextures;

	bool IsTextureLoaded(std::string Name);
	TextureToGo* GetTexture(std::string Name, std::string Directory = "");
	std::unordered_map<std::string, TextureToGo>& GetTextures();
};