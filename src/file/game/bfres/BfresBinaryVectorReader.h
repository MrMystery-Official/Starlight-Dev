#pragma once

#include <util/BinaryVectorReader.h>
#include <util/Logger.h>
#include <vector>
#include <string>
#include <map>
#include <iostream>
#include <glm/glm.hpp>
#include <functional>
#include <unordered_map>

namespace application::file::game::bfres
{
	class BfresBinaryVectorReader : public application::util::BinaryVectorReader
	{
	public:
		struct ReadableObject
		{
			virtual void Read(BfresBinaryVectorReader& Reader) = 0;
		};

		BfresBinaryVectorReader(std::vector<unsigned char> Bytes, std::unordered_map<uint64_t, std::string>& StringCache, bool BigEndian = false);

		glm::vec4 Read_10_10_10_2_SNorm();
		float Read_8_UNorm();
		float Read_8_UInt();
		float Read_8_SNorm();
		float Read_8_SInt();

		float Read_16_UNorm();
		float Read_16_UInt();
		float Read_16_SNorm();
		float Read_16_SInt();

		float ReadHalfFloat();

		glm::vec4 ReadAttribute(uint32_t RawFormat);
		glm::vec4 ReadVector4F();
		std::string ReadStringOffset(uint64_t Offset);

		std::vector<std::string> ReadStringOffsets(uint32_t Count);

		std::vector<bool> ReadBooleanBits(uint32_t Count);

		std::vector<bool> ReadRawBoolArray(uint32_t Count);

		template <typename T>
		std::vector<T> ReadRawArray(uint32_t Count)
		{
			std::vector<T> Result(Count);

			ReadStruct(Result.data(), Count * sizeof(T));

			return Result;
		}

		template <typename T>
		T ReadResObject()
		{
			T Object;
			BfresBinaryVectorReader::ReadableObject* BaseObj = static_cast<BfresBinaryVectorReader::ReadableObject*>(&Object);
			if (BaseObj)
			{
				BaseObj->Read(*this);
			}
			else
			{
				application::util::Logger::Error("BfresBinaryVectorReader", "Cannot cast ResObject to BfresBinaryVectorReader::ReadableObject");
			}
			return *static_cast<T*>(BaseObj);
		}

		template <typename T>
		std::vector<T> ReadArray(uint32_t Offset, uint32_t Count)
		{
			uint32_t Pos = GetPosition();

			Seek(Offset, BfresBinaryVectorReader::Position::Begin);

			std::vector<T> Result(Count);
			for (int i = 0; i < Count; i++)
			{
				Result[i] = ReadResObject<T>();
			}

			Seek(Pos, BfresBinaryVectorReader::Position::Begin);

			return Result;
		}

		template <typename T>
		T ReadCustom(std::function<T()> Func, uint32_t Offset)
		{
			uint32_t Pos = GetPosition();
			Seek(Offset, Position::Begin);
			T Result = Func();
			Seek(Pos, Position::Begin);
			return Result;
		}

		std::unordered_map<uint64_t, std::string>& mStringCache;
	};
}