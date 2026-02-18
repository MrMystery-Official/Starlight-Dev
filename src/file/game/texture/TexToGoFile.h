#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <util/BinaryVectorReader.h>
#include <file/game/texture/TextureFormat.h>

namespace application::file::game::texture
{
	class TexToGoFile
	{
	public:
		struct Surface
		{
			uint16_t mArrayIndex;
			uint8_t mMipLevel;
			uint8_t mSurfaceCount = 1;
			uint32_t mZSTDCompressedSize;
			uint32_t mUnk = 6;

			std::vector<unsigned char> mData;
			uint32_t mDataSize;

			uint32_t mWidth;
			uint32_t mHeight;

			TextureFormat::Format mPolishedFormat;
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
		application::util::BinaryVectorReader& GetReader();
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

		std::vector<TexToGoFile::Surface> mSurfaces;
		TextureFormat::Format mPolishedFormat;
		std::string mFileName = "";
		application::util::BinaryVectorReader mReader;
		bool mLoaded = false;
	};
}