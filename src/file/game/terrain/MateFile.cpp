#include "MateFile.h"

#include <util/BinaryVectorReader.h>
#include <util/FileUtil.h>
#include <glm/vec2.hpp>

namespace application::file::game::terrain
{
	/*
	const std::vector<float> TEXTURE_UV_MAP = { 0.1f, 0.1f, 0.05f, 0.05f, 0.1f, 0.1f, 0.04f, 0.04f, 0.05f, 0.05f, 0.1f, 0.1f, 0.05f, 0.05f, 0.05f, 0.05f, 0.1f, 0.1f, 0.1f, 0.1f, 0.05f, 0.05f, 0.09f, 0.09f, 0.05f, 0.05f, 0.1f, 0.1f, 0.2f, 0.2f, 0.14f, 0.14f, 0.05f, 0.05f, 0.05f, 0.05f, 0.05f, 0.05f, 0.05f, 0.05f, 0.05f, 0.05f, 0.07f, 0.07f, 0.07f, 0.07f, 0.05f, 0.05f, 0.15f, 0.15f, 0.1f, 0.1f, 0.1f, 0.1f, 0.07f, 0.07f, 0.04f, 0.04f, 0.05f, 0.16f, 0.03f, 0.03f, 0.05f, 0.05f, 0.05f, 0.05f, 0.03f, 0.03f, 0.05f, 0.05f, 0.45f, 0.45f, 0.2f, 0.2f, 0.1f, 0.1f, 0.59f, 0.59f, 0.15f, 0.15f, 0.2f, 0.2f, 0.35f, 0.35f, 0.2f, 0.2f, 0.1f, 0.1f, 0.15f, 0.15f, 0.2f, 0.2f, 0.15f, 0.15f, 0.2f, 0.2f, 0.05f, 0.05f, 0.05f, 0.05f, 0.05f, 0.05f, 0.05f, 0.05f, 0.05f, 0.05f, 0.1f, 0.1f, 0.1f, 0.1f, 0.05f, 0.05f, 0.05f, 0.05f, 0.05f, 0.05f, 0.1f, 0.1f, 0.08f, 0.08f, 0.04f, 0.04f, 0.1f, 0.1f, 0.05f, 0.05f, 0.05f, 0.05f, 0.1f, 0.1f, 0.25f, 0.25f, 0.04f, 0.05f, 0.08f, 0.08f, 0.08f, 0.08f, 0.2f, 0.2f, 0.1f, 0.1f, 0.15f, 0.15f, 0.04f, 0.04f, 0.25f, 0.25f, 0.05f, 0.05f, 0.15f, 0.15f, 0.05f, 0.05f, 0.08f, 0.08f, 0.1f, 0.1f, 0.07f, 0.07f, 0.05f, 0.05f, 0.23f, 0.23f, 0.16f, 0.16f, 0.16f, 0.16f, 0.04f, 0.04f, 0.1f, 0.1f, 0.05f, 0.05f, 0.1f, 0.1f
		, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f };
	*/
	const std::vector<uint8_t> MateFile::TEXTURE_INDEX_MAP = 
	{
			0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
		21, 22, 23, 24, 25, 26, 27, 28, 17, 18, 0, 29, 30, 31, 32, 33, 34, 35, 36,
		37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55,
		56, 57, 58, 59, 7, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 0, 72,
		73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91,
		92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108,
		109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120
	};

	std::vector<glm::vec4> GetTexCoords(std::vector<unsigned char>& materialBuffer, int mapTileSize, int mapTileLength, int indexCountSide)
	{
		std::vector<glm::vec4> vertices(mapTileSize);

		float uvBaseScale = 10;
		int vertexIndex = 0;
		int matIndex = 0;

		for (float y = 0; y < mapTileLength; y++)
		{
			float normY = y / (float)indexCountSide;
			for (float x = 0; x < mapTileLength; x++)
			{
				float normX = x / (float)indexCountSide;

				/*
				glm::vec2 uvScaleA = glm::vec2(
					TEXTURE_UV_MAP[materialBuffer[matIndex]],
					TEXTURE_UV_MAP[materialBuffer[matIndex] + 1]);
				glm::vec2 uvScaleB = glm::vec2(
					TEXTURE_UV_MAP[materialBuffer[matIndex + 1]],
					TEXTURE_UV_MAP[materialBuffer[matIndex + 1] + 1]);

					*/
				vertices[vertexIndex++] = glm::vec4(
					uvBaseScale * normX,
					uvBaseScale * normY,
					uvBaseScale * normX,
					uvBaseScale * normY);

				matIndex += 4;
			}
		}
		return vertices;
	}

	void MateFile::GenerateTexCoords(application::file::game::terrain::TerrainSceneFile* SceneFile)
	{
		const float UVBaseScale = 100;

		mTexCoords.resize(mWidth * mHeight);

		for (uint16_t y = 0; y < mHeight; y++)
		{
			float normY = y / (float)(mHeight - 1);
			for (uint16_t x = 0; x < mWidth; x++)
			{
				float normX = x / (float)(mWidth - 1);

				mTexCoords[x + y * mWidth].x = UVBaseScale * normX * SceneFile->mTerrainScene.mMaterialInfo[mMaterials[x + y * mWidth].mIndexA].mObj.mUScale;
				mTexCoords[x + y * mWidth].y = UVBaseScale * normY * SceneFile->mTerrainScene.mMaterialInfo[mMaterials[x + y * mWidth].mIndexA].mObj.mVScale;
				mTexCoords[x + y * mWidth].z = UVBaseScale * normX * SceneFile->mTerrainScene.mMaterialInfo[mMaterials[x + y * mWidth].mIndexB].mObj.mUScale;
				mTexCoords[x + y * mWidth].w = UVBaseScale * normY * SceneFile->mTerrainScene.mMaterialInfo[mMaterials[x + y * mWidth].mIndexB].mObj.mVScale;
			}
		}
	}

	void MateFile::Initialize(std::vector<unsigned char> Data, application::file::game::terrain::TerrainSceneFile* SceneFile, uint16_t Width, uint16_t Height)
	{
		mWidth = Width;
		mHeight = Height;

		std::vector<unsigned char> out_buf(Data.size());

		mMaterials.resize(Data.size() / 4);
		mTexCoords.resize(Data.size() / 4);

		const float UVBaseScale = 100;

		for (uint16_t y = 0; y < Height; y++)
		{
			uint32_t in_offset = y * mWidth;
			uint32_t out_offset = y * (2 * 8 + (mWidth - 4) * 4);
			uint32_t indexA = 0;
			uint32_t indexB = 0;
			uint32_t blend = 0;
			uint32_t unk = 0;
			for (uint16_t x = 0; x < Width; x++)
			{
				indexA += Data[in_offset];
				out_buf[out_offset] = indexA & 0xff;
				indexB += Data[in_offset + (mWidth * mHeight)];
				out_buf[out_offset + 1] = indexB & 0xff;
				blend += Data[in_offset + (mWidth * mHeight) * 2];
				out_buf[out_offset + 2] = blend & 0xff;
				unk += Data[in_offset + (mWidth * mHeight) * 3];
				out_buf[out_offset + 3] = unk & 0xff;

				out_buf[out_offset] = std::min(out_buf[out_offset], (unsigned char)125);
				out_buf[out_offset + 1] = std::min(out_buf[out_offset + 1], (unsigned char)125);

				MaterialIndexInfo Info;
				Info.mIndexA = TEXTURE_INDEX_MAP[out_buf[out_offset]];
				Info.mIndexB = TEXTURE_INDEX_MAP[out_buf[out_offset + 1]];
				Info.mBlendFactor = out_buf[out_offset + 2];
				mMaterials[x + y * mWidth] = Info;

				out_offset += 4;
				in_offset += 1;
			}
		}

		GenerateTexCoords(SceneFile);
	}

	std::vector<unsigned char> MateFile::ToBinary()
	{
		std::vector<int> inverseMap(TEXTURE_INDEX_MAP.size(), 0);
		for (int b = 0; b < TEXTURE_INDEX_MAP.size(); ++b) {
			uint8_t mapped = TEXTURE_INDEX_MAP[b];
			inverseMap[mapped] = b;
		}


		// 2) Vorbereitung des Ausgabepuffers
		size_t totalPixels = static_cast<size_t>(mWidth) * mHeight;
		std::vector<uint8_t> out_buf(totalPixels * 4);

		// 3) Rekonstruktion pro Pixel
		for (uint16_t y = 0; y < mHeight; ++y) {

			uint32_t out_offset = y * (2 * 8 + (mWidth - 4) * 4);

			for (uint16_t x = 0; x < mWidth; ++x) {
				size_t idxMat = size_t(x) + size_t(y) * mWidth;
				const auto& Info = mMaterials[idxMat];

				// Inverses Mapping + Clamp
				uint8_t rawA = static_cast<uint8_t>(Info.mIndexA == 120 ? 139 : inverseMap[Info.mIndexA]);
				uint8_t rawB = static_cast<uint8_t>(Info.mIndexB == 120 ? 139 : inverseMap[Info.mIndexB]);
				//rawA = std::min<uint8_t>(rawA, 125);
				//rawB = std::min<uint8_t>(rawB, 125);

				uint8_t blend = Info.mBlendFactor;
				uint8_t unk = 0;  // Kann nicht rekonstruiert werden

				out_buf[out_offset + 0] = rawA;
				out_buf[out_offset + 1] = rawB;
				out_buf[out_offset + 2] = blend;
				out_buf[out_offset + 3] = unk;

				out_offset += 4;
			}
		}

		std::vector<uint8_t> delta_data(totalPixels * 4, 0);

		// Process each component (indexA, indexB, blend, unk) separately
		for (size_t component = 0; component < 4; ++component) {
			for (uint16_t y = 0; y < mHeight; ++y) {
				// Previous value for this row (reset per row)
				uint8_t prevValue = 0;

				for (uint16_t x = 0; x < mWidth; ++x) {
					// Calculate the offset in out_buf
					uint32_t out_offset = y * (2 * 8 + (mWidth - 4) * 4) + x * 4;
					// Get the absolute value for this pixel and component
					uint8_t absValue = out_buf[out_offset + component];

					// Calculate the delta value
					// For the first pixel in each row, delta = absolute value
					// For subsequent pixels, delta = absolute value - previous absolute value
					uint8_t deltaValue;
					if (x == 0) {
						deltaValue = absValue;
					}
					else {
						// Handle underflow with wrap-around (byte arithmetic)
						deltaValue = static_cast<uint8_t>(absValue - prevValue);
					}

					// Store the delta value in the correct section of delta_data
					// Each component has its own section of size totalPixels
					size_t deltaOffset = component * totalPixels + y * mWidth + x;
					delta_data[deltaOffset] = deltaValue;

					// Update the previous value for the next iteration
					prevValue = absValue;
				}
			}
		}

		return delta_data;
	}

	MateFile::MaterialIndexInfo* MateFile::GetMaterialAtGridPos(const uint16_t& X, const uint16_t& Y)
	{
		if (mMaterials.empty() || X >= mWidth || Y >= mHeight)
			return nullptr;

		return &mMaterials[X + Y * mWidth];
	}

	glm::vec4* MateFile::GetTexCoordAtGridPos(const uint16_t& X, const uint16_t& Y)
	{
		if (mMaterials.empty() || X >= mWidth || Y >= mHeight)
			return nullptr;

		return &mTexCoords[X + Y * mWidth];
	}

	uint16_t& MateFile::GetWidth()
	{
		return mWidth;
	}

	uint16_t& MateFile::GetHeight()
	{
		return mHeight;
	}

	MateFile::MateFile(std::vector<unsigned char> Data, application::file::game::terrain::TerrainSceneFile* SceneFile, uint16_t Width, uint16_t Height)
	{
		Initialize(Data, SceneFile, Width, Height);
	}
}