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
	auto ResizeSingleSection = [](std::vector<unsigned char>& Section)
		{
			std::vector<unsigned char> New(128 * 128);
			BinaryVectorReader Reader(Section);
			Reader.Seek(130, BinaryVectorReader::Position::Begin);
			
			for (int i = 0; i < 128; i++)
			{
				Reader.Seek(1, BinaryVectorReader::Position::Current);
				Reader.ReadStruct(New.data() + i * 128, 128);
				Reader.Seek(1, BinaryVectorReader::Position::Current);
			}

			return New;
		};

	BinaryVectorReader Reader(Bytes);
	std::vector<unsigned char> HeightData;
	HeightData.resize(130 * 130);
	//Reader.Seek(33800, BinaryVectorReader::Position::Begin);
	for (int i = 0; i < 130; i++)
	{
		Reader.ReadStruct(HeightData.data() + i * 130, 130);
		Reader.Seek(130, BinaryVectorReader::Position::Current);
	}

	std::vector<unsigned char> BaseHeightData;
	BaseHeightData.resize(130 * 130);
	for (int i = 0; i < 130; i++)
	{
		Reader.ReadStruct(BaseHeightData.data() + i * 130, 130);
		Reader.Seek(130, BinaryVectorReader::Position::Current);
	}

	/*
	mHeights.resize(130 * 130);
	for (size_t i = 0; i < mHeights.size(); i++)
	{
		mHeights[i] = (float)HeightData[i];
	}
	*/

	std::cout << "READ\n";

	std::vector<unsigned char> ResizedHeightData = ResizeSingleSection(HeightData);
	std::vector<unsigned char> ResizedBaseData = ResizeSingleSection(BaseHeightData);

	std::vector<uint16_t> Sum(ResizedHeightData.size());

	for (size_t i = 0; i < Sum.size(); ++i)
	{
		int16_t New = (signed char)ResizedBaseData[i] - 0x77;
		Sum[i] = static_cast<uint16_t>(ResizedHeightData[i]) + New;
	}

	// Step 2: Find the maximum value in the uint16 vector
	uint16_t maxVal = *std::max_element(Sum.begin(), Sum.end());

	// Step 3: Normalize the values and store them in a new vector of unsigned chars
	std::vector<unsigned char> normalizedVec(Sum.size());
	for (size_t i = 0; i < Sum.size(); ++i) {
		// Normalize to range [0, 255] and cast to unsigned char
		normalizedVec[i] = static_cast<unsigned char>((static_cast<float>(Sum[i]) / maxVal) * 255);
	}

	stbi_write_png(Editor::GetWorkingDirFile("HeightMap_Merge.png").c_str(), 128, 128, 1, normalizedVec.data(), 128);
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