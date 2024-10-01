#pragma once

#include "PhiveShape.h"
#include "BinaryVectorReader.h"
#include <string>

namespace SplatoonShapeToTotK
{
	struct SectionBvhNode
	{
		uint32_t MinX;
		uint32_t MaxX;
		uint32_t MinY;
		uint32_t MaxY;
		uint32_t MinZ;
		uint32_t MaxZ;

		uint8_t Data0;
		uint8_t Data1;
		uint8_t Data2;
		uint8_t Data3;
	};

	struct Primitive
	{
		uint8_t IdA;
		uint8_t IdB;
		uint8_t IdC;
		uint8_t IdD;
	};

	struct Vertex
	{
		uint16_t X;
		uint16_t Y;
		uint16_t Z;
	};

	struct MaterialSettings
	{
		uint32_t MaterialIndex;
		bool Climbable = true;
	};

	extern std::vector<const char*> Materials;

	uint32_t FindSection(BinaryVectorReader Reader, std::string Section);
	inline uint32_t BitFieldToSize(uint32_t Input);
	void ReadStringTable(BinaryVectorReader Reader, std::vector<std::string>* Dest);
	std::vector<unsigned char> Convert(std::string SplatoonFile, MaterialSettings PhiveMaterial);
	std::vector<unsigned char> Convert(std::vector<unsigned char> Bytes, MaterialSettings PhiveMaterial);
}