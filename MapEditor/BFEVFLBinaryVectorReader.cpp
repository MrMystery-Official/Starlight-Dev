#include "BFEVFLBinaryVectorReader.h"

std::string BFEVFLBinaryVectorReader::ReadStringPtr()
{
	uint64_t Offset = ReadUInt64();
	uint32_t Jumpback = GetPosition();
	
	Seek(Offset, BinaryVectorReader::Position::Begin);
	uint16_t Size = ReadUInt16();
	std::string Data = ReadString(Size);

	Logger::Info("Debug", "Size: " + std::to_string(Size) + ", Data: " + Data + ", Jumpback: " + std::to_string(Jumpback));

	Seek(Jumpback, BinaryVectorReader::Position::Begin);
	return Data;
}