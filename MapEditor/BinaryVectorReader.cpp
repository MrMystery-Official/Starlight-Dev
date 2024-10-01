#include "BinaryVectorReader.h"

#include <cstring>
#include <intrin.h>

BinaryVectorReader::BinaryVectorReader(std::vector<unsigned char>& Bytes, bool BigEndian) : m_Bytes(Bytes) {}

void BinaryVectorReader::Seek(int Offset, BinaryVectorReader::Position Position)
{
	if (Position == BinaryVectorReader::Position::Begin)
	{
		this->m_Offset = Offset - 1;
	} 
	else if (Position == BinaryVectorReader::Position::Current)
	{
		this->m_Offset += Offset;
	}
	else
	{
		this->m_Offset = this->m_Bytes.size() - Offset - 2;
	}
}

int BinaryVectorReader::GetPosition()
{
	return this->m_Offset + 1;
}

uint32_t BinaryVectorReader::GetSize()
{
	return this->m_Bytes.size();
}

void BinaryVectorReader::Read(char* Data, int Length)
{
	for (int i = 0; i < Length; i++) {
		this->m_Offset++;
		Data[i] = this->m_Bytes[this->m_Offset];
	}
}

uint8_t BinaryVectorReader::ReadUInt8()
{
	this->m_Offset++;
	return this->m_Bytes[this->m_Offset];
}

int8_t BinaryVectorReader::ReadInt8()
{
	this->m_Offset++;
	return this->m_Bytes[this->m_Offset];
}

uint16_t BinaryVectorReader::ReadUInt16(bool BigEndian)
{
	this->m_Offset += 2;
	return !BigEndian ? ((static_cast<uint16_t>(this->m_Bytes[this->m_Offset - 1])) | (this->m_Bytes[this->m_Offset] << 8)) : _byteswap_ushort(((static_cast<uint16_t>(this->m_Bytes[this->m_Offset - 1])) | (this->m_Bytes[this->m_Offset] << 8)));
}

int16_t BinaryVectorReader::ReadInt16(bool BigEndian)
{
	this->m_Offset += 2;
	return !BigEndian ? ((static_cast<int16_t>(this->m_Bytes[this->m_Offset - 1])) | (this->m_Bytes[this->m_Offset] << 8)) : _byteswap_ushort(((static_cast<int16_t>(this->m_Bytes[this->m_Offset - 1])) | (this->m_Bytes[this->m_Offset] << 8)));
}

uint32_t BinaryVectorReader::ReadUInt24(bool BigEndian) {
	this->m_Offset += 3;
	uint32_t Value = (static_cast<uint32_t>(this->m_Bytes[this->m_Offset - 2])) |
		(static_cast<uint32_t>(this->m_Bytes[this->m_Offset - 1]) << 8) |
		(static_cast<uint32_t>(this->m_Bytes[this->m_Offset]) << 16);
	return !BigEndian ? Value : (_byteswap_ulong(Value) >> 8);
}

uint32_t BinaryVectorReader::ReadUInt32(bool BigEndian)
{
	this->m_Offset += 4;
	return !BigEndian ? (static_cast<uint32_t>(this->m_Bytes[this->m_Offset - 3])) |
		(static_cast<uint32_t>(this->m_Bytes[this->m_Offset - 2]) << 8) |
		(static_cast<uint32_t>(this->m_Bytes[this->m_Offset - 1]) << 16) |
		static_cast<uint32_t>(this->m_Bytes[this->m_Offset] << 24) : _byteswap_ulong(static_cast<uint32_t>(this->m_Bytes[this->m_Offset - 3]) |
			(static_cast<uint32_t>(this->m_Bytes[this->m_Offset - 2]) << 8) |
			(static_cast<uint32_t>(this->m_Bytes[this->m_Offset - 1]) << 16) |
			static_cast<uint32_t>(this->m_Bytes[this->m_Offset] << 24));
}

int32_t BinaryVectorReader::ReadInt32(bool BigEndian)
{
	this->m_Offset += 4;
	return !BigEndian ? (static_cast<int32_t>(this->m_Bytes[this->m_Offset - 3])) |
		(static_cast<int32_t>(this->m_Bytes[this->m_Offset - 2]) << 8) |
		(static_cast<int32_t>(this->m_Bytes[this->m_Offset - 1]) << 16) |
		static_cast<int32_t>(this->m_Bytes[this->m_Offset] << 24) : _byteswap_ulong(static_cast<int32_t>(this->m_Bytes[this->m_Offset - 3]) |
			(static_cast<int32_t>(this->m_Bytes[this->m_Offset - 2]) << 8) |
			(static_cast<int32_t>(this->m_Bytes[this->m_Offset - 1]) << 16) |
			static_cast<int32_t>(this->m_Bytes[this->m_Offset] << 24));
}

uint64_t BinaryVectorReader::ReadUInt64(bool BigEndian)
{
	this->m_Offset += 8;
	uint64_t Data = (static_cast<uint64_t>(this->m_Bytes[this->m_Offset - 7])) |
		(static_cast<uint64_t>(this->m_Bytes[this->m_Offset - 6]) << 8) |
		(static_cast<uint64_t>(this->m_Bytes[this->m_Offset - 5]) << 16) |
		(static_cast<uint64_t>(this->m_Bytes[this->m_Offset - 4]) << 24) |
		(static_cast<uint64_t>(this->m_Bytes[this->m_Offset - 3]) << 32) |
		(static_cast<uint64_t>(this->m_Bytes[this->m_Offset - 2]) << 40) |
		(static_cast<uint64_t>(this->m_Bytes[this->m_Offset - 1]) << 48) |
		(static_cast<uint64_t>(this->m_Bytes[this->m_Offset]) << 56);
	return BigEndian ? _byteswap_uint64(Data) : Data;
}

int64_t BinaryVectorReader::ReadInt64(bool BigEndian)
{
	this->m_Offset += 8;
	uint64_t Data = (static_cast<int64_t>(this->m_Bytes[this->m_Offset - 7])) |
		(static_cast<int64_t>(this->m_Bytes[this->m_Offset - 6]) << 8) |
		(static_cast<int64_t>(this->m_Bytes[this->m_Offset - 5]) << 16) |
		(static_cast<int64_t>(this->m_Bytes[this->m_Offset - 4]) << 24) |
		(static_cast<int64_t>(this->m_Bytes[this->m_Offset - 3]) << 32) |
		(static_cast<int64_t>(this->m_Bytes[this->m_Offset - 2]) << 40) |
		(static_cast<int64_t>(this->m_Bytes[this->m_Offset - 1]) << 48) |
		(static_cast<int64_t>(this->m_Bytes[this->m_Offset]) << 56);
	return BigEndian ? _byteswap_uint64(Data) : Data;
}

float BinaryVectorReader::ReadFloat(bool BigEndian)
{
	float Result;
	uint32_t Temp = this->ReadUInt32(BigEndian);
	memcpy(&Result, &Temp, sizeof(Result));
	return Result;
}

double BinaryVectorReader::ReadDouble(bool BigEndian)
{
	double Result;
	uint32_t Temp = this->ReadUInt64(BigEndian);
	memcpy(&Result, &Temp, sizeof(Result));
	return Result;
}

void BinaryVectorReader::ReadStruct(void* Dest, uint32_t Size, int Offset)
{
	if (Offset != -1) this->Seek(Offset, BinaryVectorReader::Position::Begin);
	
	std::memcpy(Dest, this->m_Bytes.data() + this->m_Offset + 1, Size);

	//std::memcopy(this->m_Bytes.begin() + this->m_Offset + 1, this->m_Bytes.begin() + this->m_Offset + 1 + Size, static_cast<char*>(Dest));

	this->m_Offset += Size;
}

std::string BinaryVectorReader::ReadString(int Size)
{
	std::string Result;
	char CurrentCharacter = this->ReadInt8();
	Result += CurrentCharacter;

	if (Size == -1)
	{
		while (CurrentCharacter != 0x00)
		{
			CurrentCharacter = this->ReadInt8();
			Result += CurrentCharacter;
		}
		Result.pop_back();
	}
	else
	{
		for (int i = 0; i < Size-1; i++)
			Result += this->ReadInt8();
	}

	return Result;
}

std::wstring BinaryVectorReader::ReadWString(int Size)
{
	std::wstring Result;
	wchar_t CurrentCharacter = static_cast<wchar_t>(this->ReadUInt16());
	Result += CurrentCharacter;
	for (int i = 0; i < Size - 1; i++)
		Result += static_cast<wchar_t>(this->ReadUInt16());

	return Result;
}

void BinaryVectorReader::Align(uint32_t Alignment)
{
	while (GetPosition() % Alignment != 0)
		Seek(1, BinaryVectorReader::Position::Current);
}