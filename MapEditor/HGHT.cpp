#include "HGHT.h"

#include "Util.h"
#include "BinaryVectorReader.h"
#include <stb/stb_image_write.h>
#include <stb/stb_image.h>
#include "Editor.h"
#include <iostream>
#include <algorithm>

HGHTFile::HGHTFile(std::string Path)
{
	this->HGHTFile::HGHTFile(Util::ReadFile(Path));
}

HGHTFile::HGHTFile(std::vector<unsigned char> Bytes)
{
	BinaryVectorReader Reader(Bytes);
	std::vector<unsigned char> HeightData;
	HeightData.resize(130 * 130);
	for (int i = 0; i < 130; i++)
	{
		Reader.ReadStruct(HeightData.data() + i * 130, 130);
		Reader.Seek(130, BinaryVectorReader::Position::Current);
	}

	std::vector<unsigned char> OffsetData;
	OffsetData.resize(130 * 130);
	for (int i = 0; i < 130; i++)
	{
		Reader.ReadStruct(OffsetData.data() + i * 130, 130);
		Reader.Seek(130, BinaryVectorReader::Position::Current);
	}

	mHeights.resize(130 * 130);
	for (size_t i = 0; i < mHeights.size(); i++)
	{
		mHeights[i] = (static_cast<uint16_t>(OffsetData[i]) << 8) | static_cast<uint16_t>(HeightData[i]);
	}

	std::vector<unsigned char> NormalizedImageData;
	NormalizedImageData.resize(130 * 130);
	uint16_t MinValue = *std::min_element(mHeights.begin(), mHeights.end());
	uint16_t MaxValue = *std::max_element(mHeights.begin(), mHeights.end());
	for (size_t i = 0; i < NormalizedImageData.size(); i++) {
		NormalizedImageData[i] = static_cast<uint8_t>(
			((mHeights[i] - MinValue) * 255) / (MaxValue - MinValue)
			);
	}

	stbi_write_png(Editor::GetWorkingDirFile("HeightMap_Overlay16.png").c_str(), 130, 130, 1, NormalizedImageData.data(), 130);
	
	std::cout << "READ\n";
}

void HGHTFile::InitMesh()
{
	std::vector<float> Vertices(130 * 130 * 3);

	for (int i = 0; i < 130 * 130; i++)
	{
		Vertices[i * 3] = (i % 130) * 10;
		Vertices[i * 3 + 1] = mHeights[i] / 10.0f;
		Vertices[i * 3 + 2] = (i / 130) * 10;
	}

	std::vector<unsigned int> Indices;

	for (int row = 0; row < 130 - 1; row++) {
		for (int col = 0; col < 130 - 1; col++) {
			short TopLeftIndexNum = (short)(row * 130 + col);
			short TopRightIndexNum = (short)(row * 130 + col + 1);
			short BottomLeftIndexNum = (short)((row + 1) * 130 + col);
			short BottomRightIndexNum = (short)((row + 1) * 130 + col + 1);

			// write out two triangles
			Indices.push_back(TopLeftIndexNum);
			Indices.push_back(BottomLeftIndexNum);
			Indices.push_back(TopRightIndexNum);

			Indices.push_back(TopRightIndexNum);
			Indices.push_back(BottomLeftIndexNum);
			Indices.push_back(BottomRightIndexNum);
		}
	}

	mMesh = Mesh(Vertices, Indices);
}