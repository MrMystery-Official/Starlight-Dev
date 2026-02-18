#include "ZStdBackend.h"

#include <util/FileUtil.h>
#include <util/Logger.h>
#include <file/game/sarc/SarcFile.h>

namespace application::file::game
{
	ZSTD_DDict* ZStdBackend::gDecompressDictBase = nullptr;
	ZSTD_DDict* ZStdBackend::gDecompressDictPack = nullptr;
	ZSTD_DDict* ZStdBackend::gDecompressDictBcettByaml = nullptr;

	ZSTD_CDict* ZStdBackend::gCompressDictBase = nullptr;
	ZSTD_CDict* ZStdBackend::gCompressDictPack = nullptr;
	ZSTD_CDict* ZStdBackend::gCompressDictBcettByaml = nullptr;

	void ZStdBackend::InitializeDicts(const std::string& Path)
	{
		application::file::game::SarcFile DictSarc(ZStdBackend::Decompress(application::util::FileUtil::ReadFile(Path))); //This has to work, if not, the whole editor won't. So no need to check if it was decompressed successfully, if not, a crash is the best option.

		std::vector<unsigned char>* DictPack = &DictSarc.GetEntry("pack.zsdic").mBytes;
		std::vector<unsigned char>* DictBase = &DictSarc.GetEntry("zs.zsdic").mBytes;
		std::vector<unsigned char>* DictBcettByaml = &DictSarc.GetEntry("bcett.byml.zsdic").mBytes;

		gDecompressDictBase = ZSTD_createDDict(DictBase->data(), DictBase->size());
		gDecompressDictPack = ZSTD_createDDict(DictPack->data(), DictPack->size());
		gDecompressDictBcettByaml = ZSTD_createDDict(DictBcettByaml->data(), DictBcettByaml->size());

		//16 = compression level
		gCompressDictBase = ZSTD_createCDict(DictBase->data(), DictBase->size(), 16);
		gCompressDictPack = ZSTD_createCDict(DictPack->data(), DictPack->size(), 16);
		gCompressDictBcettByaml = ZSTD_createCDict(DictBcettByaml->data(), DictBcettByaml->size(), 16);
	}

	std::vector<unsigned char> ZStdBackend::Decompress(const std::string& Path)
	{
		return Decompress(application::util::FileUtil::ReadFile(Path));
	}

	uint32_t ZStdBackend::GetDecompressedSize(std::vector<unsigned char> Input)
	{
		unsigned long long DecompSize = ZSTD_getFrameContentSize(Input.data(), Input.size());
		if (DecompSize == 18446744073709551614) //Means the size could not be calculated
		{
			return -1;
		}
		return DecompSize;
	}

	std::vector<unsigned char> ZStdBackend::Decompress(std::vector<unsigned char> Input)
	{
		unsigned long long DecompSize = ZSTD_getFrameContentSize(Input.data(), Input.size());
		if (DecompSize >= 18000000000000000000) //Means the size could not be calculated
		{
			application::util::Logger::Error("ZStdBackend", "Estimated DecompSize too big: %u", DecompSize);
			return std::vector<unsigned char>();
		}

		std::vector<unsigned char> Result(DecompSize);

		ZSTD_DCtx* const DCtx = ZSTD_createDCtx();

		const unsigned int ActualDictID = ZSTD_getDictID_fromFrame(Input.data(), Input.size());

		if (ActualDictID == 0)
		{
			DecompSize = ZSTD_decompressDCtx(DCtx, (void*)Result.data(), Result.size(), Input.data(), Input.size());
		}
		else
		{
			ZSTD_DDict* DecompressionDict = nullptr;
			switch (ActualDictID)
			{
			case 1:
				DecompressionDict = ZStdBackend::gDecompressDictBase;
				break;
			case 2:
				DecompressionDict = ZStdBackend::gDecompressDictBcettByaml;
				break;
			case 3:
				DecompressionDict = ZStdBackend::gDecompressDictPack;
				break;
			default:
				application::util::Logger::Error("ZStdBackend", "Invalid dict id: %u", ActualDictID);
				return std::vector<unsigned char>();
			}

			DecompSize = ZSTD_decompress_usingDDict(DCtx, (void*)Result.data(), Result.size(), Input.data(), Input.size(), DecompressionDict);
		}

		ZSTD_freeDCtx(DCtx);
		if (DecompSize >= 18000000000000000000) //Means the size could not be calculated
		{
			application::util::Logger::Error("ZStdBackend", "DecompSize too big: %lu", DecompSize);
			return std::vector<unsigned char>();
		}
		Result.resize(DecompSize);

		return Result;
	}

	std::vector<unsigned char> ZStdBackend::Compress(std::vector<unsigned char> Input, Dictionary Dict)
	{
		std::vector<unsigned char> Data;
		Data.resize(ZSTD_compressBound(Input.size()));

		if (Dict == ZStdBackend::Dictionary::None)
		{
			ZSTD_CCtx* cctx = ZSTD_createCCtx();
			ZSTD_CCtx_setParameter(cctx, ZSTD_c_compressionLevel, 16); //16 = compression level
			const size_t CompSize = ZSTD_compressCCtx(cctx, (void*)Data.data(), Data.size(), Input.data(), Input.size(), 16);
			Data.resize(CompSize);
			ZSTD_freeCCtx(cctx);
		}
		else
		{
			ZSTD_CDict* CompressionDict = nullptr;
			switch (Dict)
			{
			case ZStdBackend::Dictionary::Base:
				CompressionDict = gCompressDictBase;
				break;
			case ZStdBackend::Dictionary::Pack:
				CompressionDict = gCompressDictPack;
				break;
			case ZStdBackend::Dictionary::BcettByaml:
				CompressionDict = gCompressDictBcettByaml;
				break;
			}

			ZSTD_CCtx* cctx = ZSTD_createCCtx();
			ZSTD_CCtx_setParameter(cctx, ZSTD_c_compressionLevel, 16); //16 = compression level
			const size_t CompSize = ZSTD_compress_usingCDict(cctx, (void*)Data.data(), Data.size(), Input.data(), Input.size(), CompressionDict);
			Data.resize(CompSize);
			ZSTD_freeCCtx(cctx);
		}

		return Data;
	}
}