#include "HghtFile.h"

#include <util/FileUtil.h>
#include <stb/stb_image.h>
#include <stb/stb_image_write.h>
#include <util/BinaryVectorWriter.h>

namespace application::file::game::terrain
{
	void HghtFile::Initialize(std::vector<unsigned char> Data, uint16_t Width, uint16_t Height)
	{
        mWidth = Width;
        mHeight = Height;
        mDataSize = Data.size();
        mHeightMap.resize(Width * Height);

		uint32_t PtrOut = 0;
		uint32_t PtrLod0 = 0;
		uint32_t PtrLod1 = PtrLod0 + (Width * Height / 2);
		uint32_t PtrLod2 = PtrLod1 + (Width * Height / 2);

        for (uint16_t y = 0; y < Height; y++)
        {
            int16_t currVal = 0;
            for (uint16_t x = 0; x < Width; x++)
            {
                int shift = (x & 1) ? 0 : 4;

                int8_t lod0 = ((Data[PtrLod0] >> shift) & 0xF) - 8;
                int8_t lod1 = ((Data[PtrLod1] >> shift) & 0xF) - 7;
                int8_t lod2 = *reinterpret_cast<int8_t*>(&Data[PtrLod2++]);

                currVal += (lod2 * 0x100) + (lod1 * 0x10) + lod0;
                mHeightMap[PtrOut] = static_cast<uint16_t>(currVal);
                PtrOut++;

                if (x & 1) 
                {
                    ++PtrLod0;
                    ++PtrLod1;
                }
            }
        }
	}

    std::vector<unsigned char> HghtFile::ToBinary()
    {
        std::vector<unsigned char> Data(mDataSize, 0); // Initialize with zeros
        uint32_t PtrOut = 0;
        uint32_t PtrLod0 = 0;
        uint32_t PtrLod1 = PtrLod0 + (mWidth * mHeight / 2);
        uint32_t PtrLod2 = PtrLod1 + (mWidth * mHeight / 2);

        for (uint16_t y = 0; y < mHeight; y++)
        {
            int16_t prevVal = 0;
            for (uint16_t x = 0; x < mWidth; x++)
            {
                int shift = (x & 1) ? 0 : 4;
                int16_t currVal = static_cast<int16_t>(mHeightMap[PtrOut++]); // Increment PtrOut here
                int16_t delta = currVal - prevVal;
                prevVal = currVal; // Update prevVal after calculating delta

                // Extract lod0 - Range is -8 to 7
                int8_t lod0 = static_cast<int8_t>((delta & 0xF) - ((delta & 0x8) ? 16 : 0));

                // Subtract lod0's contribution
                int16_t remainder = delta - lod0;

                // Extract lod1 - Range is -7 to 8 (different from lod0!)
                // We need to handle the different range here
                int8_t lod1 = static_cast<int8_t>((remainder >> 4) & 0xF);
                if (lod1 > 8) lod1 -= 16; // Convert to range -7 to 8

                // Subtract lod1's contribution
                remainder -= (lod1 << 4);

                // Extract lod2
                int8_t lod2 = static_cast<int8_t>(remainder >> 8);

                // Apply the offsets that are subtracted during decompression
                lod0 += 8;  // In decompression: lod0 = value - 8
                lod1 += 7;  // In decompression: lod1 = value - 7
                lod0 &= 0xF;
                lod1 &= 0xF;

                // Set the values in the output buffer
                Data[PtrLod0] = (Data[PtrLod0] & ~(0xF << shift)) | ((lod0 & 0xF) << shift);
                Data[PtrLod1] = (Data[PtrLod1] & ~(0xF << shift)) | ((lod1 & 0xF) << shift);
                Data[PtrLod2++] = static_cast<unsigned char>(lod2);

                if (x & 1)
                {
                    ++PtrLod0;
                    ++PtrLod1;
                }
            }
        }

        return Data;
    }

    uint16_t HghtFile::GetHeightAtGridPos(const uint16_t& X, const uint16_t& Y)
    {
        if (mHeightMap.empty() || X >= mWidth || Y >= mHeight)
            return 0;

        return mHeightMap[X + Y * mWidth];
    }

    uint16_t& HghtFile::GetWidth()
    {
        return mWidth;
    }

    uint16_t& HghtFile::GetHeight()
    {
        return mHeight;
    }

	HghtFile::HghtFile(std::vector<unsigned char> Data, uint16_t Width, uint16_t Height)
	{
		Initialize(Data, Width, Height);
	}
}