#pragma once

#include <vector>

class BinaryVectorWriter
{
public:

	enum class Position : uint8_t
	{
		Begin = 0,
		Current = 1,
		End = 2
	};

	int GetPosition();
	void WriteByte(char Byte);
	void WriteBytes(const char* Bytes);
	void WriteInteger(int64_t Data, int Size, bool BigEndian = false);
	void WriteRawUnsafe(const char* Bytes, int Size);
	void WriteRawUnsafeFixed(const char* Bytes, int Size, bool BigEndian = false);
	void Seek(int Offset, BinaryVectorWriter::Position Start);
	void Align(uint32_t Alignment, uint8_t Aligner = 0x00);
	std::vector<unsigned char>& GetData();
private:
	std::vector<unsigned char> m_Data;
	int m_Offset = 0;
};