#include <file/game/bfres/BfresBinaryVectorWriter.h>

#include <algorithm>
#include <file/game/bfres/BfresFile.h>

namespace application::file::game::bfres
{
	BfresBinaryVectorWriter::BfresBinaryVectorWriter(std::unordered_map<uint64_t, std::string>& StringCache) : BinaryVectorWriter(), mStringCache(StringCache)
	{

	}

	void BfresBinaryVectorWriter::Write_10_10_10_2_SNorm(const glm::vec4& Vec)
	{
		
	}

	void BfresBinaryVectorWriter::Write_8_UNorm(float val)
	{
		WriteInteger(static_cast<uint8_t>(std::clamp(val, 0.0f, 1.0f) * 255), 1);
	}

	void BfresBinaryVectorWriter::Write_8_UInt(float val)
	{
		WriteInteger(static_cast<uint8_t>(val), 1);
	}

	void BfresBinaryVectorWriter::Write_8_SNorm(float val)
	{
		WriteInteger(static_cast<int8_t>(std::clamp(val, -1.0f, 1.0f) * 127), 1);
	}

	void BfresBinaryVectorWriter::Write_8_SInt(float val)
	{
		WriteInteger(static_cast<int8_t>(val), 1);
	}

	void BfresBinaryVectorWriter::Write_16_UNorm(float val)
	{
		WriteInteger(static_cast<uint16_t>(std::clamp(val, 0.0f, 1.0f) * 65535), 2);
	}

	void BfresBinaryVectorWriter::Write_16_UInt(float val)
	{
		WriteInteger(static_cast<uint16_t>(val), 2);
	}

	void BfresBinaryVectorWriter::Write_16_SNorm(float val)
	{
		WriteInteger(static_cast<uint16_t>(std::clamp(val, -1.0f, 1.0f) * 32767), 2);
	}

	void BfresBinaryVectorWriter::Write_16_SInt(float val)
	{
		WriteInteger(static_cast<int16_t>(val), 2);
	}

	void BfresBinaryVectorWriter::WriteHalfFloat(float val)
	{
		uint32_t FloatBits;
		std::memcpy(&FloatBits, &val, sizeof(float));

		uint32_t Sign = (FloatBits >> 16) & 0x8000;
		int32_t Exponent = ((FloatBits >> 23) & 0xFF) - 127 + 15;
		uint32_t Mantissa = (FloatBits >> 13) & 0x3FF;

		uint16_t CombinedValue;

		if ((FloatBits & 0x7FFFFFFF) == 0)
		{
			CombinedValue = 0;
		}
		else
		{
			if (Exponent <= 0)
			{
				CombinedValue = Sign;
			}
			else if (Exponent >= 0x1F)
			{
				CombinedValue = Sign | 0x7C00;
			}
			else
			{
				CombinedValue = Sign | (Exponent << 10) | Mantissa;
			}
		}

		uint8_t Byte1 = static_cast<uint8_t>(CombinedValue & 0xFF);
		uint8_t Byte2 = static_cast<uint8_t>((CombinedValue >> 8) & 0xFF);

		WriteInteger(Byte1, 1);
		WriteInteger(Byte2, 1);
	}

	void BfresBinaryVectorWriter::WriteAttribute(uint32_t RawFormat, const glm::vec4& Vec)
	{
		switch (auto Format = static_cast<BfresFile::BfresAttribFormat>(RawFormat))
		{
		case BfresFile::BfresAttribFormat::Format_10_10_10_2_SNorm:
			Write_10_10_10_2_SNorm(Vec);
			break;

		case BfresFile::BfresAttribFormat::Format_8_UNorm:
			Write_8_UNorm(Vec.x);
			break;
		case BfresFile::BfresAttribFormat::Format_8_UInt:
			Write_8_UInt(Vec.x);
			break;
		case BfresFile::BfresAttribFormat::Format_8_SNorm:
			Write_8_SNorm(Vec.x);
			break;
		case BfresFile::BfresAttribFormat::Format_8_SInt:
			Write_8_SInt(Vec.x);
			break;
		case BfresFile::BfresAttribFormat::Format_8_SIntToSingle:
			Write_8_SInt(Vec.x);
			break;
		case BfresFile::BfresAttribFormat::Format_8_UIntToSingle:
			Write_8_UInt(Vec.x);
			break;


		case BfresFile::BfresAttribFormat::Format_16_UNorm:
			Write_16_UNorm(Vec.x);
			break;
		case BfresFile::BfresAttribFormat::Format_16_UInt:
			Write_16_UInt(Vec.x);
			break;
		case BfresFile::BfresAttribFormat::Format_16_SNorm:
			Write_16_SNorm(Vec.x);
			break;
		case BfresFile::BfresAttribFormat::Format_16_SInt:
			Write_16_SInt(Vec.x);
			break;
		case BfresFile::BfresAttribFormat::Format_16_SIntToSingle:
			Write_16_SInt(Vec.x);
			break;
		case BfresFile::BfresAttribFormat::Format_16_UIntToSingle:
			Write_16_UInt(Vec.x);
			break;

		case BfresFile::BfresAttribFormat::Format_8_8_UNorm:
			Write_8_UNorm(Vec.x);
			Write_8_UNorm(Vec.y);
			break;
		case BfresFile::BfresAttribFormat::Format_8_8_UInt:
			Write_8_UInt(Vec.x);
			Write_8_UInt(Vec.y);
			break;
		case BfresFile::BfresAttribFormat::Format_8_8_SNorm:
			Write_8_SNorm(Vec.x);
			Write_8_SNorm(Vec.y);
			break;
		case BfresFile::BfresAttribFormat::Format_8_8_SInt:
			Write_8_SInt(Vec.x);
			Write_8_SInt(Vec.y);
			break;
		case BfresFile::BfresAttribFormat::Format_8_8_SIntToSingle:
			Write_8_SInt(Vec.x);
			Write_8_SInt(Vec.y);
			break;
		case BfresFile::BfresAttribFormat::Format_8_8_UIntToSingle:
			Write_8_UInt(Vec.x);
			Write_8_UInt(Vec.y);
			break;

		case BfresFile::BfresAttribFormat::Format_8_8_8_8_UNorm:
			Write_8_UNorm(Vec.x);
			Write_8_UNorm(Vec.y);
			Write_8_UNorm(Vec.z);
			Write_8_UNorm(Vec.w);
			break;
		case BfresFile::BfresAttribFormat::Format_8_8_8_8_UInt:
			Write_8_UInt(Vec.x);
			Write_8_UInt(Vec.y);
			Write_8_UInt(Vec.z);
			Write_8_UInt(Vec.w);
			break;
		case BfresFile::BfresAttribFormat::Format_8_8_8_8_SNorm:
			Write_8_SNorm(Vec.x);
			Write_8_SNorm(Vec.y);
			Write_8_SNorm(Vec.z);
			Write_8_SNorm(Vec.w);
			break;
		case BfresFile::BfresAttribFormat::Format_8_8_8_8_SInt:
			Write_8_SInt(Vec.x);
			Write_8_SInt(Vec.y);
			Write_8_SInt(Vec.z);
			Write_8_SInt(Vec.w);
			break;
		case BfresFile::BfresAttribFormat::Format_8_8_8_8_SIntToSingle:
			Write_8_SInt(Vec.x);
			Write_8_SInt(Vec.y);
			Write_8_SInt(Vec.z);
			Write_8_SInt(Vec.w);
			break;
		case BfresFile::BfresAttribFormat::Format_8_8_8_8_UIntToSingle:
			Write_8_UInt(Vec.x);
			Write_8_UInt(Vec.y);
			Write_8_UInt(Vec.z);
			Write_8_UInt(Vec.w);
			break;

		case BfresFile::BfresAttribFormat::Format_16_16_UNorm:
			Write_16_UNorm(Vec.x);
			Write_16_UNorm(Vec.y);
			break;
		case BfresFile::BfresAttribFormat::Format_16_16_UInt:
			Write_16_UInt(Vec.x);
			Write_16_UInt(Vec.y);
			break;
		case BfresFile::BfresAttribFormat::Format_16_16_SNorm:
			Write_16_SNorm(Vec.x);
			Write_16_SNorm(Vec.y);
			break;
		case BfresFile::BfresAttribFormat::Format_16_16_SInt:
			Write_16_SInt(Vec.x);
			Write_16_SInt(Vec.y);
			break;
		case BfresFile::BfresAttribFormat::Format_16_16_SIntToSingle:
			Write_16_SInt(Vec.x);
			Write_16_SInt(Vec.y);
			break;
		case BfresFile::BfresAttribFormat::Format_16_16_UIntToSingle:
			Write_16_UInt(Vec.x);
			Write_16_UInt(Vec.y);
			break;

		case BfresFile::BfresAttribFormat::Format_16_16_Single:
			WriteHalfFloat(Vec.x);
			WriteHalfFloat(Vec.y);
			break;
		case BfresFile::BfresAttribFormat::Format_16_16_16_16_Single:
			WriteHalfFloat(Vec.x);
			WriteHalfFloat(Vec.y);
			WriteHalfFloat(Vec.z);
			WriteHalfFloat(Vec.w);
			break;

		case BfresFile::BfresAttribFormat::Format_32_UInt:
			WriteInteger(static_cast<uint32_t>(Vec.x), 4);
			break;
		case BfresFile::BfresAttribFormat::Format_32_SInt:
			WriteInteger(static_cast<int32_t>(Vec.x), 4);
			break;
		case BfresFile::BfresAttribFormat::Format_32_Single:
			WriteRawUnsafeFixed(reinterpret_cast<const char*>(&Vec.x), 4);
			break;

		case BfresFile::BfresAttribFormat::Format_32_32_UInt:
			WriteInteger(static_cast<uint32_t>(Vec.x), 4);
			WriteInteger(static_cast<uint32_t>(Vec.y), 4);
			break;
		case BfresFile::BfresAttribFormat::Format_32_32_SInt:
			WriteInteger(static_cast<int32_t>(Vec.x), 4);
			WriteInteger(static_cast<int32_t>(Vec.y), 4);
			break;
		case BfresFile::BfresAttribFormat::Format_32_32_Single:
			WriteRawUnsafeFixed(reinterpret_cast<const char*>(&Vec.x), 4);
			WriteRawUnsafeFixed(reinterpret_cast<const char*>(&Vec.y), 4);
			break;

		case BfresFile::BfresAttribFormat::Format_32_32_32_UInt:
			WriteInteger(static_cast<uint32_t>(Vec.x), 4);
			WriteInteger(static_cast<uint32_t>(Vec.y), 4);
			WriteInteger(static_cast<uint32_t>(Vec.z), 4);
			break;
		case BfresFile::BfresAttribFormat::Format_32_32_32_SInt:
			WriteInteger(static_cast<int32_t>(Vec.x), 4);
			WriteInteger(static_cast<int32_t>(Vec.y), 4);
			WriteInteger(static_cast<int32_t>(Vec.z), 4);
			break;
		case BfresFile::BfresAttribFormat::Format_32_32_32_Single:
			WriteRawUnsafeFixed(reinterpret_cast<const char*>(&Vec.x), 4);
			WriteRawUnsafeFixed(reinterpret_cast<const char*>(&Vec.y), 4);
			WriteRawUnsafeFixed(reinterpret_cast<const char*>(&Vec.z), 4);
			break;

		case BfresFile::BfresAttribFormat::Format_32_32_32_32_UInt:
			WriteInteger(static_cast<uint32_t>(Vec.x), 4);
			WriteInteger(static_cast<uint32_t>(Vec.y), 4);
			WriteInteger(static_cast<uint32_t>(Vec.z), 4);
			WriteInteger(static_cast<uint32_t>(Vec.w), 4);
			break;
		case BfresFile::BfresAttribFormat::Format_32_32_32_32_SInt:
			WriteInteger(static_cast<int32_t>(Vec.x), 4);
			WriteInteger(static_cast<int32_t>(Vec.y), 4);
			WriteInteger(static_cast<int32_t>(Vec.z), 4);
			WriteInteger(static_cast<int32_t>(Vec.w), 4);
			break;
		case BfresFile::BfresAttribFormat::Format_32_32_32_32_Single:
			WriteRawUnsafeFixed(reinterpret_cast<const char*>(&Vec.x), 4);
			WriteRawUnsafeFixed(reinterpret_cast<const char*>(&Vec.y), 4);
			WriteRawUnsafeFixed(reinterpret_cast<const char*>(&Vec.z), 4);
			WriteRawUnsafeFixed(reinterpret_cast<const char*>(&Vec.w), 4);
			break;
		default:
			break;
		}
	}

	void BfresBinaryVectorWriter::WriteVector4F(const glm::vec4& Vec)
	{
		WriteRawUnsafeFixed(reinterpret_cast<const char*>(&Vec.x), 4);
		WriteRawUnsafeFixed(reinterpret_cast<const char*>(&Vec.y), 4);
		WriteRawUnsafeFixed(reinterpret_cast<const char*>(&Vec.z), 4);
		WriteRawUnsafeFixed(reinterpret_cast<const char*>(&Vec.w), 4);
	}

	uint64_t BfresBinaryVectorWriter::WriteString(const std::string& Str)
	{
		return 0;
	}

	void BfresBinaryVectorWriter::WriteStrings(const std::vector<std::string>& Strings)
	{

	}

	void BfresBinaryVectorWriter::WriteBooleanBits(const std::vector<bool>& Vec)
	{
		uint32_t Count = static_cast<uint32_t>(Vec.size());
		std::vector<int64_t> BitFlags(1 + Count / 64, 0);

		int Idx = 0;
		for (int i = 0; i < Count; i++)
		{
			if (i != 0 && i % 64 == 0)
				Idx++;

			if (Vec[i])
			{
				BitFlags[Idx] |= (static_cast<int64_t>(1) << i);
			}
		}

		WriteRawArray<int64_t>(BitFlags);
	}

	void BfresBinaryVectorWriter::WriteRawBoolArray(const std::vector<bool>& Vec)
	{
		for (bool Value : Vec)
		{
			WriteInteger(static_cast<uint8_t>(Value), 1);
		}
	}
}