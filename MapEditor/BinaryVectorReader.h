#pragma once

#include <vector>
#include <string>

class BinaryVectorReader
{
public:
	enum class Position : uint8_t
	{
		Begin = 0,
		Current = 1,
		End = 2
	};

	BinaryVectorReader() = default;
	BinaryVectorReader(std::vector<unsigned char>& Bytes, bool BigEndian = false);

	void Seek(int Offset, BinaryVectorReader::Position Position);
	int GetPosition();
	uint32_t GetSize();
	void Read(char* Data, int Length);
	uint8_t ReadUInt8();
	int8_t ReadInt8();
	uint16_t ReadUInt16(bool BigEndian = false);
	int16_t ReadInt16(bool BigEndian = false);
	uint32_t ReadUInt24(bool BigEndian = false);
	uint32_t ReadUInt32(bool BigEndian = false);
	int32_t ReadInt32(bool BigEndian = false);
	uint64_t ReadUInt64(bool BigEndian = false);
	int64_t ReadInt64(bool BigEndian = false);
	float ReadFloat(bool BigEndian = false);
	double ReadDouble(bool BigEndian = false);
	void ReadStruct(void* Dest, uint32_t Size, int Offset = -1);
	std::string ReadString(int Size = -1);
	std::wstring ReadWString(int Size);
	void Align(uint32_t Alignment);
private:
	int m_Offset = -1;
	std::vector<unsigned char> m_Bytes;
};