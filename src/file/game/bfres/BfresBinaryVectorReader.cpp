#include "BfresBinaryVectorReader.h"

#include "BfresFile.h"

namespace application::file::game::bfres
{
	BfresBinaryVectorReader::BfresBinaryVectorReader(std::vector<unsigned char> Bytes, std::unordered_map<uint64_t, std::string>& StringCache, bool BigEndian) : BinaryVectorReader(Bytes, BigEndian), mStringCache(StringCache)
	{

	}

	glm::vec4 BfresBinaryVectorReader::Read_10_10_10_2_SNorm()
	{
		int32_t Value = ReadInt32();
		return glm::vec4((Value << 22 >> 22) / 511.0f, (Value << 12 >> 22) / 511.0f, (Value << 2 >> 22) / 511.0f, Value >> 30);
	}

	float BfresBinaryVectorReader::Read_8_UNorm()
	{
		return ReadUInt8() / 255.0f;
	}
	float BfresBinaryVectorReader::Read_8_UInt()
	{
		return ReadUInt8();
	}
	float BfresBinaryVectorReader::Read_8_SNorm()
	{
		return ReadInt8() / 127.0f;
	}
	float BfresBinaryVectorReader::Read_8_SInt()
	{
		return ReadInt8();
	}

	float BfresBinaryVectorReader::Read_16_UNorm()
	{
		return ReadUInt16() / 65535.0f;
	}
	float BfresBinaryVectorReader::Read_16_UInt()
	{
		return ReadUInt16();
	}
	float BfresBinaryVectorReader::Read_16_SNorm()
	{
		return ReadInt16() / 32767.0f;
	}
	float BfresBinaryVectorReader::Read_16_SInt()
	{
		return ReadInt16();
	}

	float BfresBinaryVectorReader::ReadHalfFloat()
	{
		uint8_t Byte1 = ReadUInt8();
		uint8_t Byte2 = ReadUInt8();
		uint16_t CombinedValue = (static_cast<uint16_t>(Byte2) << 8) | static_cast<uint16_t>(Byte1);
		if (CombinedValue == 0)
		{
			return 0;
		}
		int32_t BiasedExponent = (CombinedValue >> 10) & 0x1F;
		int32_t Mantissa = CombinedValue & 0x3FF;

		// Reconstruct the half-float value with proper exponent bias
		int32_t RealExponent = BiasedExponent - 15 + 127;
		uint32_t HalfFloatBits = ((CombinedValue & 0x8000) << 16) | (RealExponent << 23) | (Mantissa << 13);
		float Result;
		std::memcpy(&Result, &HalfFloatBits, sizeof(float));
		return Result;
	}

	glm::vec4 BfresBinaryVectorReader::ReadAttribute(uint32_t RawFormat)
	{
		switch (auto Format = static_cast<BfresFile::BfresAttribFormat>(RawFormat))
		{
		case BfresFile::BfresAttribFormat::Format_10_10_10_2_SNorm:
			return Read_10_10_10_2_SNorm();

		case BfresFile::BfresAttribFormat::Format_8_UNorm:
			return {Read_8_UNorm(), 0, 0, 0};
		case BfresFile::BfresAttribFormat::Format_8_UInt:
			return {Read_8_UInt(), 0, 0, 0};
		case BfresFile::BfresAttribFormat::Format_8_SNorm:
			return {Read_8_SNorm(), 0, 0, 0};
		case BfresFile::BfresAttribFormat::Format_8_SInt:
			return {Read_8_SInt(), 0, 0, 0};
		case BfresFile::BfresAttribFormat::Format_8_SIntToSingle:
			return {Read_8_SInt(), 0, 0, 0};
		case BfresFile::BfresAttribFormat::Format_8_UIntToSingle:
			return {Read_8_UInt(), 0, 0, 0};


		case BfresFile::BfresAttribFormat::Format_16_UNorm:
			return {Read_16_UNorm(), 0, 0, 0};
		case BfresFile::BfresAttribFormat::Format_16_UInt:
			return {Read_16_UInt(), 0, 0, 0};
		case BfresFile::BfresAttribFormat::Format_16_SNorm:
			return {Read_16_SNorm(), 0, 0, 0};
		case BfresFile::BfresAttribFormat::Format_16_SInt:
			return {Read_16_SInt(), 0, 0, 0};
		case BfresFile::BfresAttribFormat::Format_16_SIntToSingle:
			return {Read_16_SInt(), 0, 0, 0};
		case BfresFile::BfresAttribFormat::Format_16_UIntToSingle:
			return {Read_16_UInt(), 0, 0, 0};

		case BfresFile::BfresAttribFormat::Format_8_8_UNorm:
		{
			float X = Read_8_UNorm();
			float Y = Read_8_UNorm();
			return {X, Y, 0, 0};
		}
		case BfresFile::BfresAttribFormat::Format_8_8_UInt:
		{
			float X = Read_8_UInt();
			float Y = Read_8_UInt();
			return {X, Y, 0, 0};
		}
		case BfresFile::BfresAttribFormat::Format_8_8_SNorm:
		{
			float X = Read_8_SNorm();
			float Y = Read_8_SNorm();
			return {X, Y, 0, 0};
		}
		case BfresFile::BfresAttribFormat::Format_8_8_SInt:
		{
			float X = Read_8_SInt();
			float Y = Read_8_SInt();
			return {X, Y, 0, 0};
		}
		case BfresFile::BfresAttribFormat::Format_8_8_SIntToSingle:
		{
			float X = Read_8_SInt();
			float Y = Read_8_SInt();
			return glm::vec4(X, Y, 0, 0);
		}
		case BfresFile::BfresAttribFormat::Format_8_8_UIntToSingle:
		{
			float X = Read_8_UInt();
			float Y = Read_8_UInt();
			return glm::vec4(X, Y, 0, 0);
		}

		case BfresFile::BfresAttribFormat::Format_8_8_8_8_UNorm:
		{
			float X = Read_8_UNorm();
			float Y = Read_8_UNorm();
			float Z = Read_8_UNorm();
			float W = Read_8_UNorm();
			return glm::vec4(X, Y, Z, W);
		}
		case BfresFile::BfresAttribFormat::Format_8_8_8_8_UInt:
		{
			float X = Read_8_UInt();
			float Y = Read_8_UInt();
			float Z = Read_8_UInt();
			float W = Read_8_UInt();
			return glm::vec4(X, Y, Z, W);
		}
		case BfresFile::BfresAttribFormat::Format_8_8_8_8_SNorm:
		{
			float X = Read_8_SNorm();
			float Y = Read_8_SNorm();
			float Z = Read_8_SNorm();
			float W = Read_8_SNorm();
			return glm::vec4(X, Y, Z, W);
		}
		case BfresFile::BfresAttribFormat::Format_8_8_8_8_SInt:
		{
			float X = Read_8_SInt();
			float Y = Read_8_SInt();
			float Z = Read_8_SInt();
			float W = Read_8_SInt();
			return glm::vec4(X, Y, Z, W);
		}
		case BfresFile::BfresAttribFormat::Format_8_8_8_8_SIntToSingle:
		{
			float X = Read_8_SInt();
			float Y = Read_8_SInt();
			float Z = Read_8_SInt();
			float W = Read_8_SInt();
			return glm::vec4(X, Y, Z, W);
		}
		case BfresFile::BfresAttribFormat::Format_8_8_8_8_UIntToSingle:
		{
			float X = Read_8_UInt();
			float Y = Read_8_UInt();
			float Z = Read_8_UInt();
			float W = Read_8_UInt();
			return glm::vec4(X, Y, Z, W);
		}

		case BfresFile::BfresAttribFormat::Format_16_16_UNorm:
		{
			float X = Read_16_UNorm();
			float Y = Read_16_UNorm();
			return glm::vec4(X, Y, 0, 0);
		}
		case BfresFile::BfresAttribFormat::Format_16_16_UInt:
		{
			float X = Read_16_UInt();
			float Y = Read_16_UInt();
			return glm::vec4(X, Y, 0, 0);
		}
		case BfresFile::BfresAttribFormat::Format_16_16_SNorm:
		{
			float X = Read_16_SNorm();
			float Y = Read_16_SNorm();
			return glm::vec4(X, Y, 0, 0);
		}
		case BfresFile::BfresAttribFormat::Format_16_16_SInt:
		{
			float X = Read_16_SInt();
			float Y = Read_16_SInt();
			return glm::vec4(X, Y, 0, 0);
		}
		case BfresFile::BfresAttribFormat::Format_16_16_SIntToSingle:
		{
			float X = Read_16_SInt();
			float Y = Read_16_SInt();
			return glm::vec4(X, Y, 0, 0);
		}
		case BfresFile::BfresAttribFormat::Format_16_16_UIntToSingle:
		{
			float X = Read_16_UInt();
			float Y = Read_16_UInt();
			return glm::vec4(X, Y, 0, 0);
		}

		case BfresFile::BfresAttribFormat::Format_16_16_Single:
		{
			float X = ReadHalfFloat();
			float Y = ReadHalfFloat();
			return glm::vec4(X, Y, 0, 0);
		}
		case BfresFile::BfresAttribFormat::Format_16_16_16_16_Single:
		{
			float X = ReadHalfFloat();
			float Y = ReadHalfFloat();
			float Z = ReadHalfFloat();
			float W = ReadHalfFloat();
			return glm::vec4(X, Y, Z, W);
		}

		case BfresFile::BfresAttribFormat::Format_32_UInt:
			return glm::vec4(ReadUInt32(), 0, 0, 0);
		case BfresFile::BfresAttribFormat::Format_32_SInt:
			return glm::vec4(ReadInt32(), 0, 0, 0);
		case BfresFile::BfresAttribFormat::Format_32_Single:
			return glm::vec4(ReadFloat(), 0, 0, 0);

		case BfresFile::BfresAttribFormat::Format_32_32_UInt:
		{
			uint32_t X = ReadUInt32();
			uint32_t Y = ReadUInt32();
			return glm::vec4(X, Y, 0, 0);
		}
		case BfresFile::BfresAttribFormat::Format_32_32_SInt:
		{
			int32_t X = ReadInt32();
			int32_t Y = ReadInt32();
			return glm::vec4(X, Y, 0, 0);
		}
		case BfresFile::BfresAttribFormat::Format_32_32_Single:
		{
			float X = ReadFloat();
			float Y = ReadFloat();
			return glm::vec4(X, Y, 0, 0);
		}

		case BfresFile::BfresAttribFormat::Format_32_32_32_UInt:
		{
			uint32_t X = ReadUInt32();
			uint32_t Y = ReadUInt32();
			uint32_t Z = ReadUInt32();
			return glm::vec4(X, Y, Z, 0);
		}
		case BfresFile::BfresAttribFormat::Format_32_32_32_SInt:
		{
			int32_t X = ReadInt32();
			int32_t Y = ReadInt32();
			int32_t Z = ReadInt32();
			return glm::vec4(X, Y, Z, 0);
		}
		case BfresFile::BfresAttribFormat::Format_32_32_32_Single:
		{
			float X = ReadFloat();
			float Y = ReadFloat();
			float Z = ReadFloat();
			return glm::vec4(X, Y, Z, 0);
		}

		case BfresFile::BfresAttribFormat::Format_32_32_32_32_UInt:
		{
			float X = ReadUInt32();
			float Y = ReadUInt32();
			float Z = ReadUInt32();
			float W = ReadUInt32();
			return glm::vec4(X, Y, Z, W);
		}
		case BfresFile::BfresAttribFormat::Format_32_32_32_32_SInt:
		{
			float X = ReadInt32();
			float Y = ReadInt32();
			float Z = ReadInt32();
			float W = ReadInt32();
			return glm::vec4(X, Y, Z, W);
		}
		case BfresFile::BfresAttribFormat::Format_32_32_32_32_Single:
		{
			float X = ReadFloat();
			float Y = ReadFloat();
			float Z = ReadFloat();
			float W = ReadFloat();
			return glm::vec4(X, Y, Z, W);
		}
		default:
			return glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
		}
	}

	std::vector<std::string> BfresBinaryVectorReader::ReadStringOffsets(uint32_t Count)
	{
		std::vector<std::string> Result(Count);

		for (int i = 0; i < Count; i++)
		{
			uint64_t Offset = ReadUInt64();
			Result[i] = ReadStringOffset(Offset);
		}

		return Result;
	}

	glm::vec4 BfresBinaryVectorReader::ReadVector4F()
	{
		return glm::vec4(ReadFloat(), ReadFloat(), ReadFloat(), ReadFloat());
	}

	std::string BfresBinaryVectorReader::ReadStringOffset(uint64_t Offset)
	{
		if (mStringCache.count(Offset))
		{
			return mStringCache[Offset];
		}

		uint32_t Pos = this->GetPosition();

		this->Seek(Offset, BfresBinaryVectorReader::Position::Begin);

		uint16_t Size = ReadUInt16();
		std::string String;
		String.resize(Size);
		ReadStruct(String.data(), Size);

		this->Seek(Pos, BfresBinaryVectorReader::Position::Begin);
		return String;
	}

	std::vector<bool> BfresBinaryVectorReader::ReadBooleanBits(uint32_t Count)
	{
		std::vector<bool> Bools;

		int Idx = 0;
		std::vector<int64_t> BitFlags = ReadRawArray<int64_t>(1 + Count / 64);
		for (int i = 0; i < Count; i++)
		{
			if (i != 0 && i % 64 == 0)
				Idx++;

			Bools.push_back((BitFlags[Idx] & (static_cast<long>(1) << i)) != 0);
		}

		return Bools;
	}

	std::vector<bool> BfresBinaryVectorReader::ReadRawBoolArray(uint32_t Count)
	{
		std::vector<bool> Result;

		for (int i = 0; i < Count; i++)
		{
			Result.push_back(static_cast<bool>(ReadUInt8()));
		}

		return Result;
	}
}