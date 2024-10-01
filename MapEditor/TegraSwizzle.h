#pragma once

#include <cmath>
#include "TextureToGo.h"
#include <unordered_map>

namespace TegraSwizzle
{
	int32_t CalculateBlockHeightLog(uint32_t BlockHeightLog2, uint32_t Width, uint32_t BLLWidth);
	uint32_t GetBlockHeight(uint32_t Height);
	uint32_t GetBlockHeight(uint32_t MipHeight, uint32_t BlockHeightMip0);
	uint32_t DIV_ROUND_UP(uint32_t n, uint32_t d);
	uint32_t RoundUp(uint32_t x, uint32_t y);
	uint32_t Pow2RoundUp(uint32_t x);
	std::vector<unsigned char> _Swizzle(uint32_t Width, uint32_t Height, uint32_t Depth, uint32_t BlkWidth, uint32_t BlkHeight, uint32_t BlkDepth, int32_t RoundPitch, uint32_t BPP, uint32_t TileMode, int32_t BlockHeightLog2, std::vector<unsigned char> Data, bool ToSwizzle);
	std::vector<unsigned char> Deswizzle(uint32_t Width, uint32_t Height, uint32_t Depth, uint32_t BlkWidth, uint32_t BlkHeight, uint32_t BlkDepth, int32_t RoundPitch, uint32_t BPP, uint32_t TileMode, int32_t SizeRange, std::vector<unsigned char> Data);
	std::vector<unsigned char> Swizzle(uint32_t Width, uint32_t Height, uint32_t Depth, uint32_t BlkWidth, uint32_t BlkHeight, uint32_t BlkDepth, int32_t RoundPitch, uint32_t BPP, uint32_t TileMode, int32_t SizeRange, std::vector<unsigned char> Data);
	uint32_t GetAddrBlockLinear(uint32_t x, uint32_t y, uint32_t Width, uint32_t BytesPerPixel, uint32_t BaseAddress, uint32_t BlockHeight);
};