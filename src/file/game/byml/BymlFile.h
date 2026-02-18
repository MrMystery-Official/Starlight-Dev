#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <util/BinaryVectorWriter.h>
#include <util/BinaryVectorReader.h>
#include <util/BiMap.h>
#include <glm/vec3.hpp>
#include <type_traits>
#include <unordered_set>

namespace application::file::game::byml
{
    class BymlFile
    {
    public:
        enum class Type : unsigned char
        {
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

        enum class TableGeneration : unsigned char
        {
            AUTO = 0,
            MANUAL = 1
        };

        struct Node
        {
            template <class T>
            Node(BymlFile::Type Type, const std::string& Key, const T& Value)
                : mType(Type), mKey(Key)
            {
                if constexpr (
                    std::is_array_v<T> &&
                    std::is_same_v<std::remove_cv_t<std::remove_extent_t<T>>, char>
                    )
                {
                    SetValue<std::string>(Value);
                }
                else
                {
                    SetValue<T>(Value);
                }
            }

            Node(BymlFile::Type Type, const std::string& Key = "");
            Node() = default;

            BymlFile::Type& GetType();
            std::string& GetKey();
            std::vector<BymlFile::Node>& GetChildren();

            template<class T> T GetValue() //For all numbers
            {
                T Value;
                memcpy(&Value, mValue.data(), sizeof(Value));
                return Value;
            }

            template<> std::string GetValue<std::string>() //For strings
            {
                std::vector<unsigned char> Input = mValue;

                if (mType != BymlFile::Type::StringIndex)
                {
                    std::string Number = "";
                    switch (mType)
                    {
                    case BymlFile::Type::UInt32:
                        Number = std::to_string(GetValue<uint32_t>());
                        break;
                    case BymlFile::Type::UInt64:
                        Number = std::to_string(GetValue<uint64_t>());
                        break;
                    case BymlFile::Type::Int32:
                        Number = std::to_string(GetValue<int32_t>());
                        break;
                    case BymlFile::Type::Int64:
                        Number = std::to_string(GetValue<int64_t>());
                        break;
                    case BymlFile::Type::Float:
                        Number = std::to_string(GetValue<float>());
                        break;
                    case BymlFile::Type::Bool:
                        return GetValue<bool>() ? "true" : "false";
                    }

                    Input.resize(Number.length());
                    memcpy(Input.data(), Number.data(), Number.length());
                }

                std::string Result;
                Result.reserve(Input.size());
                for (char Character : Input)
                {
                    Result += Character;
                }
                return Result;
            }

            template<> bool GetValue<bool>() //For booleans
            {
                return mValue[0];
            }

            template<> glm::vec3 GetValue<glm::vec3>() //For Vec3f, like Translate, Rotate, Scale, etc.
            {
                glm::vec3 Vec;
                Vec.x = GetChild(0)->GetValue<float>();
                Vec.y = GetChild(1)->GetValue<float>();
                Vec.z = GetChild(2)->GetValue<float>();
                return Vec;
            }
            
            template<class T> 
            void SetValue(T Value) //Value is a default C++ type
            {
                if constexpr (std::is_same_v<T, std::string>) 
                {
                    mValue.resize(Value.length());
                    memcpy(mValue.data(), Value.data(), Value.length());
                } 
                else 
                {
                    mValue.resize(sizeof(Value));
                    memcpy(mValue.data(), &Value, sizeof(Value));
                }
            }

            bool HasChild(const std::string& Key);
            BymlFile::Node* GetChild(const std::string& Key);
            BymlFile::Node* GetChild(int Index);

            void AddChild(BymlFile::Node Node);
            bool operator==(const BymlFile::Node& Comp) const;

            std::vector<unsigned char> mValue;
            BymlFile::Type mType;
            std::string mKey;
            std::vector<BymlFile::Node> mChildren;
        };

        BymlFile(const std::string& Path);
        BymlFile(const std::vector<unsigned char>& Bytes);
        BymlFile() = default;

        void Initialize(const std::vector<unsigned char> &Bytes);

        std::vector<BymlFile::Node>& GetNodes();
        BymlFile::Type& GetType();
        BymlFile::Node* GetNode(const std::string& Key);
        bool HasChild(const std::string& Key);

        void AddHashKeyTableEntry(const std::string& Key);
        void AddStringTableEntry(const std::string& String);

        void GenerateHashKeyTable(BymlFile::Node* Node);
        void GenerateStringTable(BymlFile::Node* Node);

        std::vector<unsigned char> ToBinary(BymlFile::TableGeneration TableGeneration);
        void WriteToFile(const std::string& Path, BymlFile::TableGeneration TableGeneration);

        struct NodeHasher
        {
            size_t operator()(const BymlFile::Node& Node) const
            {
                size_t hash = 0;

                // Hash the type
                HashCombine(hash, static_cast<int>(Node.mType));

                // Hash the key
                HashCombine(hash, std::hash<std::string>{}(Node.mKey));

                // Hash the value (treat as raw bytes)
                for (unsigned char byte : Node.mValue)
                {
                    HashCombine(hash, byte);
                }

                // Hash all children recursively
                for (const BymlFile::Node& Child : Node.mChildren)
                {
                    HashCombine(hash, (*this)(Child));
                }

                return hash;
            }

            // Standard hash_combine implementation
            template <typename T>
            static void HashCombine(size_t& seed, const T& value)
            {
                std::hash<T> hasher;
                seed ^= hasher(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            }
        };
    private:
        struct VectorHasher
        {
            size_t operator()(const std::vector<unsigned char>& V) const
            {
                size_t hash = V.size();
                for (unsigned char byte : V)
                {
                    hash ^= std::hash<unsigned char>{}(byte)+0x9e3779b9 + (hash << 6) + (hash >> 2);
                }
                return hash;
            }
        };

        struct NodeEqual
        {
            bool operator()(const BymlFile::Node& Node1, const BymlFile::Node& Node2) const
            {
                return Node1 == Node2;
            }
        };

        std::vector<BymlFile::Node> mNodes;
        application::util::BiMap<std::string, uint32_t> mHashKeyTable;
        application::util::BiMap<std::string, uint32_t> mStringTable;
        BymlFile::Type mType = BymlFile::Type::Null;

        uint32_t mWriterLastOffset = 0;
        uint32_t mWriterReservedDataOffset = 0;
        std::unordered_map<std::vector<unsigned char>, uint32_t, VectorHasher> mCachedValues; //Value in bytes -> Offset
        std::unordered_map<BymlFile::Node, uint32_t, NodeHasher, NodeEqual> mCachedNodes; //Node -> Offset (only containers)
        bool mBigEndian = false;

        void ParseTable(application::util::BinaryVectorReader& Reader, application::util::BiMap<std::string, uint32_t>* Dest, int TableOffset);
        int AlignUp(int Value, int Size);
        bool IsNumber(const std::string& str);
        uint32_t GetStringTableIndex(const std::string& Value);
        uint32_t GetHashKeyTableIndex(const std::string& Value);
        void ParseNode(application::util::BinaryVectorReader& Reader, int Offset, BymlFile::Type Type, std::string Key, BymlFile::Node* Parent, uint32_t ChildIndex);
        void WriteNode(application::util::BinaryVectorWriter& Writer, uint32_t DataOffset, uint32_t Offset, BymlFile::Node& Node);
    };
}