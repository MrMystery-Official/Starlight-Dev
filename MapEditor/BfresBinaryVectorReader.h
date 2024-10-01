#pragma once

#include "BinaryVectorReader.h"
#include <vector>
#include <string>
#include <map>
#include <iostream>
#include <glm/glm.hpp>
#include <functional>
#include <unordered_map>

class BfresBinaryVectorReader : public BinaryVectorReader
{
public:
	std::unordered_map<uint64_t, std::string> StringCache;

	struct ReadableObject
	{
		virtual void Read(BfresBinaryVectorReader& Reader) = 0;
	};

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

	std::vector<bool> ReadRawBoolArray(uint32_t Count)
	{
		std::vector<bool> Result;

		for (int i = 0; i < Count; i++)
		{
			Result.push_back((bool)ReadUInt8());
		}

		return Result;
	}

	template <typename T>
	std::vector<T> ReadRawArray(uint32_t Count)
	{
		std::vector<T> Result(Count);

		ReadStruct(Result.data(), Count * sizeof(T));

		return Result;
	}

	struct ResString : public ReadableObject
	{
		std::string String;

		void Read(BfresBinaryVectorReader& Reader) override
		{
		}
	};

	struct ResUInt32 : public ReadableObject
	{
		uint32_t Value;

		void Read(BfresBinaryVectorReader& Reader) override
		{
			Value = Reader.ReadUInt32();
		}
	};

	template <typename T>
	struct ResDict : public ReadableObject
	{
		struct Node
		{
			uint32_t Index = 0;

			uint32_t Reference = 0;
			uint16_t IdxLeft = 0;
			uint16_t IdxRight = 0;
			std::string Key;
			T Value;
		};

		std::map<std::string, Node> Nodes;

		void Read(BfresBinaryVectorReader& Reader) override
		{
			Reader.ReadUInt32(); //Padding?
			uint32_t NumNodes = Reader.ReadUInt32();
			Reader.Seek(16, BfresBinaryVectorReader::Position::Current); //This node is always there, but not included in the num nodes. It does not contain relevant data, maybe it is a binding node?
			uint32_t Index = 0;
			//IndexedNodes.resize(NumNodes);
			while (NumNodes > 0)
			{
				ResDict::Node NewNode;
				NewNode.Reference = Reader.ReadUInt32();
				NewNode.IdxLeft = Reader.ReadUInt16();
				NewNode.IdxRight = Reader.ReadUInt16();
				NewNode.Key = Reader.ReadStringOffset(Reader.ReadUInt64());
				NewNode.Index = Index;

				this->Nodes[NewNode.Key] = NewNode;

				//IndexedNodes[Index] = NewNode;

				NumNodes--;
				Index++;
			}
		}

		Node& GetByIndex(uint32_t Index)
		{
			return Nodes[GetKey(Index)];
		}

		std::string GetKey(uint32_t Index)
		{
			for (auto& [Key, Val] : Nodes)
			{
				if (Val.Index == Index)
					return Key;
			}

			return "";
		}

		void Add(std::string Key, T Value)
		{
			ResDict::Node NewNode;
			NewNode.Key = Key;
			NewNode.Value = Value;
			Nodes[Key] = NewNode;
		}
	};

	template <typename T>
	T ReadResObject()
	{
		T Object;
		BfresBinaryVectorReader::ReadableObject* BaseObj = reinterpret_cast<BfresBinaryVectorReader::ReadableObject*>(&Object);
		if (BaseObj)
		{
			BaseObj->Read(*this);
		}
		else
		{
			std::cout << "Cannot cast to BfresBinaryVectorReader::ReadableObject" << std::endl;
		}
		return *reinterpret_cast<T*>(BaseObj);
	}

	template <typename T>
	std::vector<T> ReadArray(uint32_t Offset, uint32_t Count)
	{
		uint32_t Pos = GetPosition();

		this->Seek(Offset, BfresBinaryVectorReader::Position::Begin);

		std::vector<T> Result(Count);
		for (int i = 0; i < Count; i++)
		{
			Result[i] = ReadResObject<T>();
		}

		this->Seek(Pos, BfresBinaryVectorReader::Position::Begin);

		return Result;
	}

	template <typename T>
	ResDict<T> ReadDictionary(uint32_t Offset, uint32_t ValueOffset)
	{
		if (Offset == 0) return ResDict<T>();

		this->Seek(Offset, BfresBinaryVectorReader::Position::Begin);
		ResDict<T> Dict = ReadResObject<ResDict<T>>();
		std::vector<T> List = ReadArray<T>(ValueOffset, Dict.Nodes.size());

		for (int i = 0; i < List.size(); i++)
		{
			Dict.Nodes[Dict.GetKey(i)].Value = List[i];
		}

		return Dict;
	}

	template <typename T>
	ResDict<T> ReadDictionary(uint32_t Offset)
	{
		this->Seek(Offset, BfresBinaryVectorReader::Position::Begin);
		return ReadResObject<ResDict<T>>();
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
};