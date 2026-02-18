#include "BinaryVectorReader.h"

#include <cstring>

namespace application::util
{

	BinaryVectorReader::BinaryVectorReader(std::vector<unsigned char> Bytes, bool BigEndian) : mBytes(Bytes) {}

	void BinaryVectorReader::Seek(int Offset, BinaryVectorReader::Position Position)
	{
		if (Position == BinaryVectorReader::Position::Begin)
		{
			this->mOffset = Offset - 1;
		}
		else if (Position == BinaryVectorReader::Position::Current)
		{
			this->mOffset += Offset;
		}
		else
		{
			this->mOffset = this->mBytes.size() - Offset - 2;
		}
	}

	int BinaryVectorReader::GetPosition()
	{
		return this->mOffset + 1;
	}

	uint32_t BinaryVectorReader::GetSize()
	{
		return this->mBytes.size();
	}

	uint8_t BinaryVectorReader::ReadUInt8()
	{
		this->mOffset++;
		return this->mBytes[this->mOffset];
	}

	int8_t BinaryVectorReader::ReadInt8()
	{
		this->mOffset++;
		return this->mBytes[this->mOffset];
	}

	uint16_t BinaryVectorReader::ReadUInt16(bool BigEndian)
	{
		this->mOffset += 2;
		return !BigEndian ? ((static_cast<uint16_t>(this->mBytes[this->mOffset - 1])) | (this->mBytes[this->mOffset] << 8)) : bswap_16(((static_cast<uint16_t>(this->mBytes[this->mOffset - 1])) | (this->mBytes[this->mOffset] << 8)));
	}

	int16_t BinaryVectorReader::ReadInt16(bool BigEndian)
	{
		this->mOffset += 2;
		return !BigEndian ? ((static_cast<int16_t>(this->mBytes[this->mOffset - 1])) | (this->mBytes[this->mOffset] << 8)) : bswap_16(((static_cast<int16_t>(this->mBytes[this->mOffset - 1])) | (this->mBytes[this->mOffset] << 8)));
	}

	uint32_t BinaryVectorReader::ReadUInt24(bool BigEndian) {
		this->mOffset += 3;
		uint32_t Value = (static_cast<uint32_t>(this->mBytes[this->mOffset - 2])) |
			(static_cast<uint32_t>(this->mBytes[this->mOffset - 1]) << 8) |
			(static_cast<uint32_t>(this->mBytes[this->mOffset]) << 16);
		return !BigEndian ? Value : (bswap_32(Value) >> 8);
	}

	uint32_t BinaryVectorReader::ReadUInt32(bool BigEndian)
	{
		this->mOffset += 4;
		return !BigEndian ? (static_cast<uint32_t>(this->mBytes[this->mOffset - 3])) |
			(static_cast<uint32_t>(this->mBytes[this->mOffset - 2]) << 8) |
			(static_cast<uint32_t>(this->mBytes[this->mOffset - 1]) << 16) |
			static_cast<uint32_t>(this->mBytes[this->mOffset] << 24) : bswap_32(static_cast<uint32_t>(this->mBytes[this->mOffset - 3]) |
				(static_cast<uint32_t>(this->mBytes[this->mOffset - 2]) << 8) |
				(static_cast<uint32_t>(this->mBytes[this->mOffset - 1]) << 16) |
				static_cast<uint32_t>(this->mBytes[this->mOffset] << 24));
	}

	int32_t BinaryVectorReader::ReadInt32(bool BigEndian)
	{
		this->mOffset += 4;
		return !BigEndian ? (static_cast<int32_t>(this->mBytes[this->mOffset - 3])) |
			(static_cast<int32_t>(this->mBytes[this->mOffset - 2]) << 8) |
			(static_cast<int32_t>(this->mBytes[this->mOffset - 1]) << 16) |
			static_cast<int32_t>(this->mBytes[this->mOffset] << 24) : bswap_32(static_cast<int32_t>(this->mBytes[this->mOffset - 3]) |
				(static_cast<int32_t>(this->mBytes[this->mOffset - 2]) << 8) |
				(static_cast<int32_t>(this->mBytes[this->mOffset - 1]) << 16) |
				static_cast<int32_t>(this->mBytes[this->mOffset] << 24));
	}

	uint64_t BinaryVectorReader::ReadUInt64(bool BigEndian)
	{
		this->mOffset += 8;
		uint64_t Data = (static_cast<uint64_t>(this->mBytes[this->mOffset - 7])) |
			(static_cast<uint64_t>(this->mBytes[this->mOffset - 6]) << 8) |
			(static_cast<uint64_t>(this->mBytes[this->mOffset - 5]) << 16) |
			(static_cast<uint64_t>(this->mBytes[this->mOffset - 4]) << 24) |
			(static_cast<uint64_t>(this->mBytes[this->mOffset - 3]) << 32) |
			(static_cast<uint64_t>(this->mBytes[this->mOffset - 2]) << 40) |
			(static_cast<uint64_t>(this->mBytes[this->mOffset - 1]) << 48) |
			(static_cast<uint64_t>(this->mBytes[this->mOffset]) << 56);
		return BigEndian ? bswap_64(Data) : Data;
	}

	int64_t BinaryVectorReader::ReadInt64(bool BigEndian)
	{
		this->mOffset += 8;
		uint64_t Data = (static_cast<int64_t>(this->mBytes[this->mOffset - 7])) |
			(static_cast<int64_t>(this->mBytes[this->mOffset - 6]) << 8) |
			(static_cast<int64_t>(this->mBytes[this->mOffset - 5]) << 16) |
			(static_cast<int64_t>(this->mBytes[this->mOffset - 4]) << 24) |
			(static_cast<int64_t>(this->mBytes[this->mOffset - 3]) << 32) |
			(static_cast<int64_t>(this->mBytes[this->mOffset - 2]) << 40) |
			(static_cast<int64_t>(this->mBytes[this->mOffset - 1]) << 48) |
			(static_cast<int64_t>(this->mBytes[this->mOffset]) << 56);
		return BigEndian ? bswap_64(Data) : Data;
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

		std::memcpy(Dest, this->mBytes.data() + this->mOffset + 1, Size);

		//std::memcopy(this->mBytes.begin() + this->mOffset + 1, this->mBytes.begin() + this->mOffset + 1 + Size, static_cast<char*>(Dest));

		this->mOffset += Size;
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
			for (int i = 0; i < Size - 1; i++)
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
}