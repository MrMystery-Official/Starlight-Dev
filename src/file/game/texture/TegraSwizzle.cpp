#include "TegraSwizzle.h"

namespace application::file::game::texture
{
	int32_t TegraSwizzle::CalculateBlockHeightLog(uint32_t BlockHeightLog2, uint32_t Width, uint32_t BLLWidth)
	{
		int32_t LinesPerBlockHeight = (1 << (int)BlockHeightLog2) * 8;

		int32_t BlockHeightShift = 0;
		if (Pow2RoundUp(DIV_ROUND_UP(Width, BLLWidth)) < LinesPerBlockHeight)
			BlockHeightShift += 1;

		return (int32_t)std::max(0, (int32_t)BlockHeightLog2 - BlockHeightShift);
	}

	uint32_t TegraSwizzle::GetBlockHeight(uint32_t Height)
	{
		uint32_t HeightAndHalf = Height + (Height / 2);
		if (HeightAndHalf >= 128)
			return 16;
		else if (HeightAndHalf >= 64)
			return 8;
		else if (HeightAndHalf >= 32)
			return 4;
		else if (HeightAndHalf >= 16)
			return 2;
		else
			return 1;
	}

	uint32_t TegraSwizzle::GetBlockHeight(uint32_t MipHeight, uint32_t BlockHeightMip0)
	{
		uint32_t BlockHeight = BlockHeightMip0;

		while (MipHeight <= (BlockHeight / 2) * 8 && BlockHeight > 1)
		{
			BlockHeight /= 2;
		}

		return BlockHeight;
	}

	uint32_t TegraSwizzle::DIV_ROUND_UP(uint32_t n, uint32_t d)
	{
		return (n + d - 1) / d;
	}

	uint32_t TegraSwizzle::RoundUp(uint32_t x, uint32_t y)
	{
		return ((x - 1) | (y - 1)) + 1;
	}

	uint32_t TegraSwizzle::Pow2RoundUp(uint32_t x)
	{
		x -= 1;
		x |= x >> 1;
		x |= x >> 2;
		x |= x >> 4;
		x |= x >> 8;
		x |= x >> 16;
		return x + 1;
	}

	std::vector<unsigned char> TegraSwizzle::_Swizzle(uint32_t Width, uint32_t Height, uint32_t Depth, uint32_t BlkWidth, uint32_t BlkHeight, uint32_t BlkDepth, int32_t RoundPitch, uint32_t BPP, uint32_t TileMode, int32_t BlockHeightLog2, std::vector<unsigned char> Data, bool ToSwizzle)
	{
		/*
		int32_t BlockHeightShift = 0;
		Width = std::max((uint32_t)1, Width);
		Height = std::max((uint32_t)1, Height);
		Depth = std::max((uint32_t)1, Depth);

		int32_t LinesPerBlockHeight = (1 << (int32_t)BlockHeightLog2) * 8;

		if (Pow2RoundUp(DIV_ROUND_UP(Height, BlkWidth)) < LinesPerBlockHeight)
			BlockHeightShift++;

		BlockHeightLog2 = std::max(0, BlockHeightLog2 - BlockHeightShift);
		*/

		uint32_t Size = DIV_ROUND_UP(Width, BlkWidth) * DIV_ROUND_UP(Height, BlkHeight) * BPP;

		uint32_t BlockHeight = (uint32_t)(1 << BlockHeightLog2);

		Width = DIV_ROUND_UP(Width, BlkWidth);
		Height = DIV_ROUND_UP(Height, BlkHeight);
		Depth = DIV_ROUND_UP(Depth, BlkDepth);

		uint32_t Pitch;
		uint32_t SurfSize;
		if (TileMode == 1)
		{
			Pitch = Width * BPP;

			if (RoundPitch == 1)
				Pitch = RoundUp(Pitch, 32);

			SurfSize = Pitch * Height;
		}
		else
		{
			Pitch = RoundUp(Width * BPP, 64);
			SurfSize = Pitch * RoundUp(Height, BlockHeight * 8);
		}

		std::vector<unsigned char> Result(SurfSize);

		for (uint32_t y = 0; y < Height; y++)
		{
			for (uint32_t x = 0; x < Width; x++)
			{
				uint32_t Pos;
				uint32_t Pos_;

				if (TileMode == 1)
					Pos = y * Pitch + x * BPP;
				else
					Pos = GetAddrBlockLinear(x, y, Width, BPP, 0, BlockHeight);

				Pos_ = (y * Width + x) * BPP;

				if (Pos + BPP <= Result.size())
				{
					if (ToSwizzle == false)
						//std::copy(Data.begin() + Pos, Data.begin() + Pos + BPP, Result.begin() + Pos_);
						memcpy(Result.data() + Pos_, Data.data() + Pos, BPP);
					else
						memcpy(Result.data() + Pos, Data.data() + Pos_, BPP);
					//std::copy(Data.begin() + Pos_, Data.begin() + Pos_ + BPP, Result.begin() + Pos);
				}
			}
		}

		Result.resize(Size);

		return Result;
	}

	std::vector<unsigned char> TegraSwizzle::Deswizzle(uint32_t Width, uint32_t Height, uint32_t Depth, uint32_t BlkWidth, uint32_t BlkHeight, uint32_t BlkDepth, int32_t RoundPitch, uint32_t BPP, uint32_t TileMode, int32_t SizeRange, std::vector<unsigned char> Data)
	{
		return _Swizzle(Width, Height, Depth, BlkWidth, BlkHeight, BlkDepth, RoundPitch, BPP, TileMode, SizeRange, Data, false);
	}

	std::vector<unsigned char> TegraSwizzle::Swizzle(uint32_t Width, uint32_t Height, uint32_t Depth, uint32_t BlkWidth, uint32_t BlkHeight, uint32_t BlkDepth, int32_t RoundPitch, uint32_t BPP, uint32_t TileMode, int32_t SizeRange, std::vector<unsigned char> Data)
	{
		return _Swizzle(Width, Height, Depth, BlkWidth, BlkHeight, BlkDepth, RoundPitch, BPP, TileMode, SizeRange, Data, true);
	}

	uint32_t TegraSwizzle::GetAddrBlockLinear(uint32_t x, uint32_t y, uint32_t Width, uint32_t BytesPerPixel, uint32_t BaseAddress, uint32_t BlockHeight)
	{
		/*
	  From Tega X1 TRM
					   */
		uint32_t image_width_in_gobs = DIV_ROUND_UP(Width * BytesPerPixel, 64);


		uint32_t GOB_address = (BaseAddress
			+ (y / (8 * BlockHeight)) * 512 * BlockHeight * image_width_in_gobs
			+ (x * BytesPerPixel / 64) * 512 * BlockHeight
			+ (y % (8 * BlockHeight) / 8) * 512);

		x *= BytesPerPixel;

		uint32_t Address = (GOB_address + ((x % 64) / 32) * 256 + ((y % 8) / 2) * 64
			+ ((x % 32) / 16) * 32 + (y % 2) * 16 + (x % 16));

		return Address;
	}
}