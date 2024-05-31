#pragma once

#include "BinaryVectorReader.h"
#include "BinaryVectorWriter.h"
#include "Vector3F.h"
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

class BymlFile {
public:
    enum class Type : unsigned char {
        StringIndex = 0xa0,
        Array = 0xc0,
        Dictionary = 0xc1,
        KeyTable = 0xc2,
        Bool = 0xd0,
        Int32 = 0xd1,
        Float = 0xd2,
        UInt32 = 0xd3,
        Int64 = 0xd4,
        UInt64 = 0xd5,
        Double = 0xd6,
        Null = 0xff
    };

    enum class TableGeneration : unsigned char {
        Auto = 0,
        Manual = 1
    };

    class Node {
    public:
        std::vector<unsigned char> m_Value;
        BymlFile::Type m_Type;
        std::string m_Key;
        std::vector<BymlFile::Node> m_Children;

        Node(BymlFile::Type Type, std::string Key = "");
        Node() { }

        BymlFile::Type& GetType();
        std::string& GetKey();

        template <class T>
        T GetValue() // For all numbers
        {
            T Value;
            memcpy(&Value, this->m_Value.data(), sizeof(Value));
            return Value;
        }

        template <>
        std::string GetValue<std::string>() // For strings
        {
            std::vector<unsigned char> Input = this->m_Value;

            if (this->m_Type != BymlFile::Type::StringIndex) {
                std::string Number = "";
                switch (this->m_Type) {
                case BymlFile::Type::UInt32:
                    Number = std::to_string(this->GetValue<uint32_t>());
                    break;
                case BymlFile::Type::UInt64:
                    Number = std::to_string(this->GetValue<uint64_t>());
                    break;
                case BymlFile::Type::Int32:
                    Number = std::to_string(this->GetValue<int32_t>());
                    break;
                case BymlFile::Type::Int64:
                    Number = std::to_string(this->GetValue<int64_t>());
                    break;
                case BymlFile::Type::Float:
                    Number = std::to_string(this->GetValue<float>());
                    break;
                case BymlFile::Type::Bool:
                    return this->GetValue<bool>() ? "true" : "false";
                }

                Input.resize(Number.length());
                memcpy(Input.data(), Number.data(), Number.length());
            }

            std::string Result;
            for (char Character : Input) {
                Result += Character;
            }
            return Result;
        }

        template <>
        bool GetValue<bool>() // For booleans
        {
            return this->m_Value[0];
        }

        template <>
        Vector3F GetValue<Vector3F>() // For Vec3f, like Translate, Rotate, Scale, etc.
        {
            Vector3F Vec;
            Vec.SetX(this->GetChild(0)->GetValue<float>());
            Vec.SetY(this->GetChild(1)->GetValue<float>());
            Vec.SetZ(this->GetChild(2)->GetValue<float>());
            return Vec;
        }

        template <class T>
        void SetValue(T Value);

        bool HasChild(std::string Key);
        BymlFile::Node* GetChild(std::string Key);
        BymlFile::Node* GetChild(int Index);
        void AddChild(BymlFile::Node Node);
        std::vector<BymlFile::Node>& GetChildren();
        bool operator==(const BymlFile::Node& Comp) const;
    };

    BymlFile(std::string Path);
    BymlFile(std::vector<unsigned char> Bytes);
    BymlFile() { }

    std::vector<BymlFile::Node>& GetNodes();
    BymlFile::Type& GetType();
    BymlFile::Node* GetNode(std::string Key);
    bool HasChild(std::string Key);

    void AddHashKeyTableEntry(std::string Key);
    void AddStringTableEntry(std::string String);

    void GenerateHashKeyTable(BymlFile::Node* Node);
    void GenerateStringTable(BymlFile::Node* Node);

    std::vector<unsigned char> ToBinary(BymlFile::TableGeneration TableGeneration);
    void WriteToFile(std::string Path, BymlFile::TableGeneration TableGeneration);

    struct NodeHasher {
        static std::string GenerateNodeHash(const BymlFile::Node& Node)
        {
            std::string Result = (char)Node.m_Type + Node.m_Key + std::string(Node.m_Value.begin(), Node.m_Value.end());
            for (const BymlFile::Node& Child : Node.m_Children) {
                Result += GenerateNodeHash(Child);
            }
            return Result;
        }

        int operator()(const BymlFile::Node& Node) const
        {
            std::string StrHash = GenerateNodeHash(Node);
            int Hash = StrHash.size();
            for (int i = 0; i < StrHash.length(); i++) {
                Hash ^= StrHash[i] + 0x9e3779b9 + (Hash << 6) + (Hash >> 2);
            }
            return Hash;
        }
    };

private:
    struct VectorHasher {
        int operator()(const std::vector<unsigned char>& V) const
        {
            int Hash = V.size();
            for (auto& i : V) {
                Hash ^= i + 0x9e3779b9 + (Hash << 6) + (Hash >> 2);
            }
            return Hash;
        }
    };

    struct NodeEqual {
        bool operator()(const BymlFile::Node& Node1, const BymlFile::Node& Node2) const
        {
            return Node1 == Node2;
        }
    };

    std::vector<BymlFile::Node> m_Nodes;
    std::vector<std::string> m_HashKeyTable;
    std::vector<std::string> m_StringTable;
    BymlFile::Type m_Type = BymlFile::Type::Null;
    uint32_t m_WriterLastOffset = 0;
    uint32_t m_WriterReservedDataOffset = 0;
    std::unordered_map<std::vector<unsigned char>, uint32_t, VectorHasher> m_CachedValues; // Value in bytes -> Offset
    std::unordered_map<BymlFile::Node, uint32_t, NodeHasher, NodeEqual> m_CachedNodes; // Node -> Offset (only containers)

    void ParseTable(BinaryVectorReader& Reader, std::vector<std::string>* Dest, int TableOffset);
    int AlignUp(int Value, int Size);
    bool IsNumber(const std::string& str);
    uint32_t GetStringTableIndex(std::string Value);
    uint32_t GetHashKeyTableIndex(std::string Value);
    void ParseNode(BinaryVectorReader& Reader, int Offset, BymlFile::Type Type, std::string Key, BymlFile::Node* Parent, uint32_t ChildIndex);
    void WriteNode(BinaryVectorWriter& Writer, uint32_t DataOffset, uint32_t Offset, BymlFile::Node& Node);
};