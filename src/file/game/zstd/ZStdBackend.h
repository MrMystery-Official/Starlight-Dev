#pragma once

#include <vector>
#include <string>
#include <zstd.h>

namespace application::file::game
{
	namespace ZStdBackend
	{
		extern ZSTD_DDict* gDecompressDictBase;
		extern ZSTD_DDict* gDecompressDictPack;
		extern ZSTD_DDict* gDecompressDictBcettByaml;

		extern ZSTD_CDict* gCompressDictBase;
		extern ZSTD_CDict* gCompressDictPack;
		extern ZSTD_CDict* gCompressDictBcettByaml;

		enum class Dictionary : uint8_t
		{
			Base = 0,
			Pack = 1,
			BcettByaml = 2,
			None = 3
		};

		void InitializeDicts(const std::string& Path);
		uint32_t GetDecompressedSize(std::vector<unsigned char> Input);
		std::vector<unsigned char> Decompress(const std::string& Path);
		std::vector<unsigned char> Decompress(std::vector<unsigned char> Input);
		std::vector<unsigned char> Compress(std::vector<unsigned char> Input, Dictionary Dict);
	}
};