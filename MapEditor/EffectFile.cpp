#include "EffectFile.h"

#include "BinaryVectorReader.h"
#include "Byml.h"
#include "Logger.h"
#include <iostream>

EffectFile::EffectFile(std::vector<unsigned char> Bytes)
{
	//BymlFile EffectConfig(Bytes, true); //Passing all bytes, unnecesary ones will be skipped by the byml reader -> byml v5

	std::vector<unsigned char> EffectBytes(Bytes.size() - 0x1000);
	std::memcpy(EffectBytes.data(), Bytes.data() + 0x1000, EffectBytes.size());

	BinaryVectorReader Reader(EffectBytes);
	Reader.ReadStruct(&Header, sizeof(Header));
}