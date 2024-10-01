#pragma once

#include "BinaryVectorReader.h"
#include "PhiveWrapper.h"
#include "Logger.h"
#include <iostream>
#include <vector>
#include <string>

class PhiveBinaryVectorReader : public BinaryVectorReader
{
public:
	struct hkReadable {
		virtual void Read(PhiveBinaryVectorReader& Reader) = 0;
	};

	PhiveWrapper& mPhiveWrapper;

	template <typename T>
	void ReadHkArray(std::vector<T>* VectorPtr, uint32_t ElementSize, const std::string& ClassName)
	{
		Align(8);

		uint64_t DataPtr = ReadUInt64();
		uint32_t Size = ReadUInt32();
		uint32_t CapacityAndFlags = ReadUInt32();

		if (Size == 0)
			return;

		uint32_t Jumpback = GetPosition();

		if (DataPtr == 0)
		{
			DataPtr = mPhiveWrapper.mItems[Size].mDataOffset + 0x50;
			Size = mPhiveWrapper.mItems[Size].mCount;

			Seek(DataPtr, BinaryVectorReader::Position::Begin);
		}
		else
		{
			Seek(Jumpback + DataPtr - 16, BinaryVectorReader::Position::Begin);
		}

		std::cout << "POS: " << GetPosition() << std::endl;

		VectorPtr->resize(Size);

		for (uint32_t i = 0; i < Size; i++)
		{
			T Object;
			if (ClassName.starts_with("hk"))
			{
				hkReadable* BaseObj = reinterpret_cast<hkReadable*>(&Object);
				if (BaseObj)
				{
					BaseObj->Read(*this);
					(*VectorPtr)[i] = Object;
				}
				else
				{
					Logger::Error("PhiveBinaryVectorReader", "Could not read class " + ClassName);
				}
			}
			else
			{
				ReadStruct(&Object, ElementSize);
				(*VectorPtr)[i] = Object;
			}
		}

		Seek(Jumpback, BinaryVectorReader::Position::Begin);
	}

	template <typename T>
	void ReadHkArrayFromItemPointer(std::vector<T>* VectorPtr, uint32_t ElementSize, const std::string& ClassName)
	{
		Align(8);

		uint64_t DataPtr = ReadUInt64();
		uint32_t Size = mPhiveWrapper.mItems[DataPtr].mCount;
		DataPtr = mPhiveWrapper.mItems[DataPtr].mDataOffset + 0x50;

		uint32_t Jumpback = GetPosition();

		Seek(DataPtr, BinaryVectorReader::Position::Begin);

		VectorPtr->resize(Size);

		for (uint32_t i = 0; i < Size; i++)
		{
			T Object;
			if (ClassName.starts_with("hk"))
			{
				hkReadable* BaseObj = reinterpret_cast<hkReadable*>(&Object);
				if (BaseObj)
				{
					BaseObj->Read(*this);
					(*VectorPtr)[i] = Object;
				}
				else
				{
					Logger::Error("PhiveBinaryVectorReader", "Could not read class " + ClassName);
				}
			}
			else
			{
				ReadStruct(&Object, ElementSize);
				(*VectorPtr)[i] = Object;
			}
		}

		Seek(Jumpback, BinaryVectorReader::Position::Begin);
	}

	template <typename T, int N>
	void ReadHkArrayFromItemPointerAtIndex(T* ElementPtr, uint32_t ElementSize, const std::string& ClassName)
	{
		Align(8);

		uint64_t DataPtr = ReadUInt64();
		uint32_t Size = mPhiveWrapper.mItems[DataPtr].mCount;
		DataPtr = mPhiveWrapper.mItems[DataPtr].mDataOffset + 0x50;

		uint32_t Jumpback = GetPosition();

		Seek(DataPtr, BinaryVectorReader::Position::Begin);
		Seek(ElementSize * N, BinaryVectorReader::Position::Current);

		if (ClassName.starts_with("hk"))
		{
			hkReadable* BaseObj = reinterpret_cast<hkReadable*>(ElementPtr);
			if (BaseObj)
			{
				BaseObj->Read(*this);
			}
			else
			{
				Logger::Error("PhiveBinaryVectorReader", "Could not read class " + ClassName);
			}
		}
		else
		{
			ReadStruct(ElementPtr, ElementSize);
		}

		Seek(Jumpback, BinaryVectorReader::Position::Begin);
	}

	template <typename T>
	void ReadHkPointer(T* Object, uint32_t ObjectSize)
	{
		Align(8);

		uint64_t DataPtr = ReadUInt64();

		std::cout << "Index: " << DataPtr << std::endl;

		uint32_t Jumpback = GetPosition();

		if (DataPtr < mPhiveWrapper.mItems.size() && DataPtr > 0)
		{
			DataPtr = mPhiveWrapper.mItems[DataPtr].mDataOffset + 0x50;

			std::cout << "Offset: " << DataPtr << std::endl;

			Seek(DataPtr, BinaryVectorReader::Position::Begin);
		}
		else
		{
			Seek(DataPtr - 16, BinaryVectorReader::Position::Current);
		}

		std::cout << "Pos: " << GetPosition() << std::endl;

		if constexpr (std::is_base_of_v<hkReadable, T>) {
			hkReadable* BaseObj = (hkReadable*)Object;
			if (BaseObj)
			{
				BaseObj->Read(*this);
			}
			else
			{
				Logger::Error("PhiveBinaryVectorReader", "Could not read pointer");
			}
		}
		else
		{
			ReadStruct(Object, ObjectSize);
		}

		Seek(Jumpback, BinaryVectorReader::Position::Begin);
	}

	PhiveBinaryVectorReader(std::vector<unsigned char>& Bytes, PhiveWrapper& Wrapper) : BinaryVectorReader(Bytes), mPhiveWrapper(Wrapper) {}
};