#pragma once

#include "BinaryVectorReader.h"
#include "Logger.h"
#include <vector>
#include <string>
#include <map>
#include <iostream>
#include <glm/glm.hpp>
#include <functional>
#include <unordered_map>

class BFEVFLBinaryVectorReader : public BinaryVectorReader
{
public:
	
	template <typename T>
	void ReadObjectOffsetsPtr(std::vector<T>& Objects, std::function<T()> Func, uint64_t OffsetsPtr)
	{
		uint32_t Jumpback = GetPosition();

		for (size_t i = 0; i < Objects.size(); i++)
		{
			uint64_t Offset = ReadUInt64();
			uint32_t OffsetJumpback = GetPosition();
			Seek(Offset, BFEVFLBinaryVectorReader::Position::Begin);
			Objects[i] = Func();
			Seek(OffsetJumpback, BFEVFLBinaryVectorReader::Position::Begin);
		}

		Seek(Jumpback, BFEVFLBinaryVectorReader::Position::Begin);
	}

	template <typename T>
	T ReadObjectPtr(std::function<T()> Func, uint64_t Offset)
	{
		if (Offset > 0)
		{
			uint32_t Jumpback = GetPosition();
			Seek(Offset, BFEVFLBinaryVectorReader::Position::Begin);
			T Object = Func();
			Seek(Jumpback, BFEVFLBinaryVectorReader::Position::Begin);

			return Object;
		}
		Logger::Error("BFEVFLBinaryVectorReader", "Offset was a nullptr");
		return T();
	}

	template <typename T>
	void ReadObjectsPtr(std::vector<T>& Data, std::function<T()> Func, uint64_t OffsetsPtr)
	{
		if (OffsetsPtr > 0)
		{
			uint32_t Jumpback = GetPosition();
			Seek(OffsetsPtr, BFEVFLBinaryVectorReader::Position::Begin);

			for (size_t i = 0; i < Data.size(); i++)
			{
				Data[i] = Func();
			}

			Seek(Jumpback, BFEVFLBinaryVectorReader::Position::Begin);
		}
		else
		{
			Logger::Error("BFEVFLBinaryVectorReader", "OffsetsPtr was a nullptr");
		}
	}

	std::string ReadStringPtr();
};