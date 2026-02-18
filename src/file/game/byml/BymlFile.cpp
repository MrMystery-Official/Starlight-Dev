#include "BymlFile.h"

#include <fstream>
#include <algorithm>
#include <chrono>
#include <util/Logger.h>

namespace application::file::game::byml
{
    /* --- Node --- */
    BymlFile::Node::Node(BymlFile::Type Type, const std::string& Key) : mType(Type), mKey(Key)
    {
    }

    BymlFile::Type& BymlFile::Node::GetType()
    {
        return mType;
    }

    std::string& BymlFile::Node::GetKey()
    {
        return mKey;
    }

    std::vector<BymlFile::Node>& BymlFile::Node::GetChildren()
    {
        return mChildren;
    }

    void BymlFile::Node::AddChild(BymlFile::Node Node)
    {
        mChildren.push_back(Node);
    }

    bool BymlFile::Node::HasChild(const std::string& Key)
    {
        for (BymlFile::Node& Node : this->mChildren)
        {
            if (Node.mKey == Key)
            {
                return true;
            }
        }
        return false;
    }

    BymlFile::Node* BymlFile::Node::GetChild(const std::string& Key)
    {
        for (BymlFile::Node& Node : this->mChildren)
        {
            if (Node.mKey == Key)
            {
                return &Node;
            }
        }

        return nullptr;
    }

    BymlFile::Node* BymlFile::Node::GetChild(int Index)
    {
        return &mChildren[Index];
    }

    bool BymlFile::Node::operator==(const BymlFile::Node& Comp) const
    {
        return mKey == Comp.mKey &&
            mValue == Comp.mValue &&
            mType == Comp.mType &&
            mChildren == Comp.mChildren;
    }

    /* --- Node --- */

    bool BymlFile::HasChild(const std::string& Key)
    {
        for (BymlFile::Node& Node : this->mNodes)
        {
            if (Node.GetKey() == Key)
            {
                return true;
            }
        }
        return false;
    }

    void BymlFile::ParseTable(application::util::BinaryVectorReader& Reader, application::util::BiMap<std::string, uint32_t>* Dest, int TableOffset)
    {
        Reader.Seek(TableOffset, application::util::BinaryVectorReader::Position::Begin);
        if (Reader.ReadUInt8() != (uint8_t)BymlFile::Type::KeyTable)
        {
            application::util::Logger::Error("BymlFile", "Wrong node type, could not parse key table");
            return;
        }
        uint32_t TableSize = Reader.ReadUInt24(mBigEndian);
        for (uint32_t i = 0; i < TableSize; i++)
        {
            Reader.Seek(TableOffset + 4 + 4 * i, application::util::BinaryVectorReader::Position::Begin);
            Reader.Seek(TableOffset + Reader.ReadUInt32(mBigEndian), application::util::BinaryVectorReader::Position::Begin);
            char CurrentCharacter = Reader.ReadInt8();
            std::string Str;
            Str += CurrentCharacter;
            while (CurrentCharacter != 0x00)
            {
                CurrentCharacter = Reader.ReadInt8();
                Str += CurrentCharacter;
            }
            Str.pop_back();
            Dest->Insert(Str, Dest->Size());
        }
    }

    int BymlFile::AlignUp(int Value, int Size)
    {
        return Value + (Size - Value % Size) % Size;
    }

    void BymlFile::ParseNode(application::util::BinaryVectorReader& Reader, int Offset, BymlFile::Type Type, std::string Key, BymlFile::Node* Parent, uint32_t ChildIndex)
    {
        if (Type == BymlFile::Type::Null)
        {
            application::util::Logger::Error("BymlFile", "Got BYML node with null type");
            return;
        }

        BymlFile::Node& Node = Parent->GetChildren()[ChildIndex];
        Node.mType = Type;
        Node.mKey = Key;

        Reader.Seek(Offset, application::util::BinaryVectorReader::Position::Begin);

        switch (Type)
        {
        case BymlFile::Type::Double:
        {
            Node.SetValue<double>(Reader.ReadDouble(mBigEndian));
            break;
        }
        case BymlFile::Type::Float:
        {
            Node.SetValue<float>(Reader.ReadFloat(mBigEndian));
            break;
        }
        case BymlFile::Type::UInt32:
        {
            Node.SetValue<uint32_t>(Reader.ReadUInt32(mBigEndian));
            break;
        }
        case BymlFile::Type::Int32:
        {
            Node.SetValue<int32_t>(Reader.ReadInt32(mBigEndian));
            break;
        }
        case BymlFile::Type::UInt64:
        {
            Reader.Seek(Reader.ReadUInt32(mBigEndian), application::util::BinaryVectorReader::Position::Begin);
            Node.SetValue<uint64_t>(Reader.ReadUInt64(mBigEndian));
            break;
        }
        case BymlFile::Type::Int64:
        {
            Reader.Seek(Reader.ReadUInt32(mBigEndian), application::util::BinaryVectorReader::Position::Begin);
            Node.SetValue<int64_t>(Reader.ReadInt64(mBigEndian));
            break;
        }
        case BymlFile::Type::Bool:
        {
            Node.SetValue<bool>(Reader.ReadUInt32(mBigEndian));
            break;
        }
        case BymlFile::Type::StringIndex:
        {
            Node.SetValue<std::string>(mStringTable.GetByValue(Reader.ReadUInt32(mBigEndian)));
            break;
        }
        case BymlFile::Type::Array:
        {
            uint32_t LocalOffset = Reader.ReadUInt32(mBigEndian);
            Reader.Seek(LocalOffset + 1, application::util::BinaryVectorReader::Position::Begin);
            uint32_t Size = Reader.ReadUInt24(mBigEndian);
            uint32_t ValueArrayOffset = LocalOffset + 4 + AlignUp(Size, 4);
            Node.GetChildren().resize(Size);
            for (uint32_t i = 0; i < Size; i++)
            {
                Reader.Seek(LocalOffset + 4 + i, application::util::BinaryVectorReader::Position::Begin);
                ParseNode(Reader, ValueArrayOffset + 4 * i, static_cast<BymlFile::Type>(Reader.ReadUInt8()), std::to_string(i), &Node, i);
            }
            break;
        }
        case BymlFile::Type::Dictionary:
        {
            uint32_t LocalOffset = Reader.ReadUInt32(mBigEndian);
            Reader.Seek(LocalOffset + 1, application::util::BinaryVectorReader::Position::Begin);
            uint32_t Size = Reader.ReadUInt24(mBigEndian);
            Node.GetChildren().resize(Size);
            uint32_t EntryOffset = LocalOffset + 4;
            for (uint32_t i = 0; i < Size; i++)
            {
                Reader.Seek(EntryOffset, application::util::BinaryVectorReader::Position::Begin);
                uint32_t StringOffset = Reader.ReadUInt24(mBigEndian);
                //Reader.Seek(EntryOffset + 3, BinaryVectorReader::Position::Begin);
                ParseNode(Reader, EntryOffset + 4, static_cast<BymlFile::Type>(Reader.ReadUInt8()), mHashKeyTable.GetByValue(StringOffset), &Node, i);
                EntryOffset += 8;
            }
            break;
        }
        default:
            application::util::Logger::Error("BymlFile", "Unsupported node type: %u", (int)Type);
        }
    }

    std::vector<BymlFile::Node>& BymlFile::GetNodes()
    {
        return mNodes;
    }

    BymlFile::Type& BymlFile::GetType()
    {
        return mType;
    }

    BymlFile::Node* BymlFile::GetNode(const std::string& Key)
    {
        for (BymlFile::Node& Node : mNodes)
        {
            if (Node.GetKey() == Key)
            {
                return &Node;
            }
        }

        return nullptr;
    }

    void BymlFile::AddHashKeyTableEntry(const std::string& Key)
    {
        mHashKeyTable.Insert(Key, mHashKeyTable.Size());
    }

    void BymlFile::AddStringTableEntry(const std::string& String)
    {
        mStringTable.Insert(String, mStringTable.Size());
    }

    bool BymlFile::IsNumber(const std::string& Str)
    {
        if (Str.empty())
        {
            return false;
        }

        bool HasDigit = false;
        bool HasDecimal = false;
        bool HasSign = false;

        for (char c : Str)
        {
            if (isdigit(c))
            {
                HasDigit = true;
            }
            else if (c == '.' && !HasDecimal)
            {
                HasDecimal = true;
            }
            else if ((c == '-' || c == '+') && !HasSign && !HasDigit)
            {
                HasSign = true;
            }
            else
            {
                return false;
            }
        }

        return HasDigit;
    }

    void BymlFile::GenerateHashKeyTable(BymlFile::Node* Node)
    {
        std::string str = Node->GetKey();
        if (!str.empty())
        {
            if (!mHashKeyTable.ContainsKey(str) && !IsNumber(str))
            {
                mHashKeyTable.Insert(str, mHashKeyTable.Size());
            }
        }

        for (BymlFile::Node& Nodes : Node->GetChildren())
        {
            GenerateHashKeyTable(&Nodes);
        }
    }

    void BymlFile::GenerateStringTable(BymlFile::Node* Node)
    {
        if (Node->GetType() == BymlFile::Type::StringIndex)
        {
            const std::string str = Node->GetValue<std::string>();
            if (!mStringTable.ContainsKey(str))
            {
                mStringTable.Insert(str, mStringTable.Size());
            }
        }

        for (BymlFile::Node& Nodes : Node->GetChildren())
        {
            GenerateStringTable(&Nodes);
        }
    }

    uint32_t BymlFile::GetStringTableIndex(const std::string& Value)
    {
        return mStringTable.GetByKey(Value);
    }

    uint32_t BymlFile::GetHashKeyTableIndex(const std::string& Value)
    {
        return mHashKeyTable.GetByKey(Value);
    }

    void BymlFile::WriteNode(application::util::BinaryVectorWriter& Writer, uint32_t DataOffset, uint32_t Offset, BymlFile::Node& Node)
    {
        if (Node.GetType() == BymlFile::Type::Null)
        {
            return;
        }

        Writer.Seek(Offset, application::util::BinaryVectorWriter::Position::Begin);

        switch (Node.GetType())
        {
        case BymlFile::Type::Float:
        {
            float Value = Node.GetValue<float>();
            Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&Value), sizeof(float), mBigEndian);
            break;
        }
        case BymlFile::Type::UInt32:
        {
            Writer.WriteInteger(Node.GetValue<uint32_t>(), sizeof(uint32_t), mBigEndian);
            break;
        }
        case BymlFile::Type::Int32:
        {
            Writer.WriteInteger(Node.GetValue<int32_t>(), sizeof(int32_t), mBigEndian);
            break;
        }
        case BymlFile::Type::Bool:
        {
            Writer.WriteInteger(Node.GetValue<bool>(), sizeof(uint32_t), mBigEndian);
            break;
        }
        case BymlFile::Type::StringIndex:
        {
            Writer.WriteInteger(GetStringTableIndex(Node.GetValue<std::string>()), sizeof(uint32_t), mBigEndian);
            break;
        }
        case BymlFile::Type::UInt64:
        {
            if (mCachedValues.contains(Node.mValue))
            {
                Writer.WriteInteger(mCachedValues[Node.mValue], sizeof(uint32_t), mBigEndian);
                break;
            }
            Writer.WriteInteger(DataOffset + mWriterReservedDataOffset, sizeof(uint32_t), mBigEndian);
            Writer.Seek(DataOffset + mWriterReservedDataOffset, application::util::BinaryVectorWriter::Position::Begin);
            Writer.WriteInteger(Node.GetValue<uint64_t>(), sizeof(uint64_t), mBigEndian);
            mCachedValues.insert({ Node.mValue, DataOffset + mWriterReservedDataOffset });
            mWriterReservedDataOffset += 8;
            break;
        }
        case BymlFile::Type::Int64:
        {
            if (mCachedValues.contains(Node.mValue))
            {
                Writer.WriteInteger(mCachedValues[Node.mValue], sizeof(uint32_t), mBigEndian);
                break;
            }
            Writer.WriteInteger(DataOffset + mWriterReservedDataOffset, sizeof(uint32_t), mBigEndian);
            Writer.Seek(DataOffset + mWriterReservedDataOffset, application::util::BinaryVectorWriter::Position::Begin);
            Writer.WriteInteger(Node.GetValue<int64_t>(), sizeof(int64_t), mBigEndian);
            mCachedValues.insert({ Node.mValue, DataOffset + mWriterReservedDataOffset });
            mWriterReservedDataOffset += 8;
            break;
        }
        case BymlFile::Type::Double:
        {
            if (mCachedValues.contains(Node.mValue))
            {
                Writer.WriteInteger(mCachedValues[Node.mValue], sizeof(uint32_t), mBigEndian);
                break;
            }
            Writer.WriteInteger(DataOffset + mWriterReservedDataOffset, sizeof(uint32_t), mBigEndian);
            Writer.Seek(DataOffset + mWriterReservedDataOffset, application::util::BinaryVectorWriter::Position::Begin);
            Writer.WriteInteger(Node.GetValue<double>(), sizeof(double), mBigEndian);
            mCachedValues.insert({ Node.mValue, DataOffset + this->mWriterReservedDataOffset });
            mWriterReservedDataOffset += 8;
            break;
        }
        case BymlFile::Type::Array:
        {
            if (!mCachedNodes.contains(Node))
            {
                std::sort(Node.GetChildren().begin(), Node.GetChildren().end(), [this](BymlFile::Node& a, BymlFile::Node& b) {
                    return GetHashKeyTableIndex(a.GetKey()) < GetHashKeyTableIndex(b.GetKey());
                });

                Writer.WriteInteger(DataOffset + mWriterReservedDataOffset, sizeof(uint32_t), mBigEndian);
                Writer.Seek(DataOffset + mWriterReservedDataOffset, application::util::BinaryVectorWriter::Position::Begin);
                uint32_t LocalOffset = DataOffset + mWriterReservedDataOffset;
                Writer.WriteByte(0xC0);
                Writer.WriteInteger(Node.GetChildren().size(), 3, mBigEndian); //3 = uint24
                for (BymlFile::Node& Child : Node.GetChildren())
                {
                    Writer.WriteByte((uint8_t)Child.GetType());
                }

                Writer.Seek(((Writer.GetPosition() + 3) & ~0x03), application::util::BinaryVectorWriter::Position::Begin);

                mWriterReservedDataOffset = Writer.GetPosition() - DataOffset + 4 * Node.GetChildren().size();
                uint32_t EntryOffset = Writer.GetPosition();
                for (int i = 0; i < Node.GetChildren().size(); i++)
                {
                    WriteNode(Writer, DataOffset, EntryOffset + 4 * i, *Node.GetChild(i));
                }
                mCachedNodes.insert({ Node, LocalOffset });
            }
            else
            {
                Writer.WriteInteger(mCachedNodes[Node], sizeof(uint32_t), mBigEndian);
            }
            break;
        }
        case BymlFile::Type::Dictionary:
        {
            if (!mCachedNodes.contains(Node))
            {
                std::sort(Node.GetChildren().begin(), Node.GetChildren().end(), [this](BymlFile::Node& a, BymlFile::Node& b) {
                    return GetHashKeyTableIndex(a.GetKey()) < GetHashKeyTableIndex(b.GetKey());
                });

                uint32_t LocalOffset = DataOffset + mWriterReservedDataOffset;
                Writer.WriteInteger(LocalOffset, sizeof(uint32_t), mBigEndian);
                mWriterReservedDataOffset += 4 + 8 * Node.GetChildren().size();
                Writer.Seek(LocalOffset, application::util::BinaryVectorWriter::Position::Begin);
                Writer.WriteByte(0xC1);
                Writer.WriteInteger(Node.GetChildren().size(), 3, mBigEndian); // 3 = uint24
                for (int i = 0; i < Node.GetChildren().size(); i++)
                {
                    uint32_t EntryOffset = LocalOffset + 4 + 8 * i;
                    Writer.Seek(EntryOffset, application::util::BinaryVectorWriter::Position::Begin);
                    Writer.WriteInteger(GetHashKeyTableIndex(Node.GetChild(i)->GetKey()), 3, mBigEndian);
                    Writer.WriteByte((char)Node.GetChild(i)->GetType());
                    WriteNode(Writer, DataOffset, EntryOffset + 4, *Node.GetChild(i));

                    Writer.Seek(((Writer.GetPosition() + 3) & ~0x03), application::util::BinaryVectorWriter::Position::Begin);
                }
                mCachedNodes.insert({ Node, LocalOffset });
            }
            else
            {
                Writer.WriteInteger(mCachedNodes[Node], sizeof(uint32_t), mBigEndian);
            }
            break;
        }
        default:
            application::util::Logger::Error("BymlFile", "Unsupported node type: %u", (int)Node.GetType());
        }

        if (Writer.GetPosition() > mWriterLastOffset)
        {
            mWriterLastOffset = Writer.GetPosition();
        }
    }

    std::vector<unsigned char> BymlFile::ToBinary(BymlFile::TableGeneration TableGeneration)
    {
        mWriterLastOffset = 0;
        mWriterReservedDataOffset = 0;
        mCachedValues.clear();
        mCachedNodes.clear();

        application::util::BinaryVectorWriter Writer;

        Writer.WriteBytes(mBigEndian ? "BY" : "YB"); //Magic
        Writer.WriteInteger(0x07, sizeof(uint16_t), mBigEndian); //Version
        Writer.WriteInteger(0x10, sizeof(uint32_t), mBigEndian); //Hash Key Table Offset

        Writer.Seek(8, application::util::BinaryVectorWriter::Position::Current);

        if (TableGeneration == BymlFile::TableGeneration::AUTO)
        {
            mHashKeyTable.Clear();
            mStringTable.Clear();

            for (BymlFile::Node& Node : mNodes)
            {
                GenerateHashKeyTable(&Node);
                GenerateStringTable(&Node);
            }
        }

        std::vector<std::string> OrderedHashKeyTable;
        std::vector<std::string> OrderedStringTable;
        OrderedHashKeyTable.resize(mHashKeyTable.Size());
        OrderedStringTable.resize(mStringTable.Size());

        for (auto& [Key, Index] : mHashKeyTable.mForward)
        {
            OrderedHashKeyTable[Index] = Key;
        }
        for (auto& [Key, Index] : mStringTable.mForward)
        {
            OrderedStringTable[Index] = Key;
        }

        std::sort(OrderedHashKeyTable.begin(), OrderedHashKeyTable.end(), [](const std::string& A, const std::string& B) { return A < B; });
        std::sort(OrderedStringTable.begin(), OrderedStringTable.end(), [](const std::string& A, const std::string& B) { return A < B; });

        mHashKeyTable.Clear();
        mStringTable.Clear();

        for (size_t i = 0; i < OrderedHashKeyTable.size(); i++)
        {
            mHashKeyTable.Insert(OrderedHashKeyTable[i], i);
        }
        for (size_t i = 0; i < OrderedStringTable.size(); i++)
        {
            mStringTable.Insert(OrderedStringTable[i], i);
        }

        Writer.WriteByte(0xC2); //Key Table Node
        Writer.WriteInteger(OrderedHashKeyTable.size(), 3, mBigEndian); //3 = uint24
        Writer.Seek(OrderedHashKeyTable.size() * 4 + 4, application::util::BinaryVectorWriter::Position::Current);

        std::vector<uint32_t> HashKeyOffsets;

        for (std::string& Key : OrderedHashKeyTable)
        {
            HashKeyOffsets.push_back(Writer.GetPosition() - 16);
            Writer.WriteBytes(Key.c_str());
            Writer.WriteByte(0x00);
        }
        HashKeyOffsets.push_back(Writer.GetPosition() - 16);
        while (Writer.GetPosition() % 4 != 0) //Align
        {
            Writer.WriteByte(0x00);
        }


        Writer.WriteByte(0xC2);
        Writer.WriteInteger(OrderedStringTable.size(), 3, mBigEndian); //3 = uint24
        uint32_t StringTableJumpback = Writer.GetPosition();
        Writer.Seek(OrderedStringTable.size() * 4 + 4, application::util::BinaryVectorWriter::Position::Current);
        std::vector<uint32_t> StringOffsets;
        for (std::string& String : OrderedStringTable)
        {
            StringOffsets.push_back(Writer.GetPosition() - StringTableJumpback + 4); //-16 because it is a relative offset
            Writer.WriteBytes(String.c_str());
            Writer.WriteByte(0x00);
        }
        StringOffsets.push_back(Writer.GetPosition() - StringTableJumpback + 4); //For the last string in the table
        while (Writer.GetPosition() % 4 != 0) //Align
        {
            Writer.WriteByte(0x00);
        }

        uint32_t DataOffset = Writer.GetPosition();

        //Write nodes
        BymlFile::Node RootNode(mType, "root");
        for (BymlFile::Node& Node : mNodes)
        {
            RootNode.AddChild(Node);
        }
        WriteNode(Writer, DataOffset, 12, RootNode);

        Writer.Seek(0x08, application::util::BinaryVectorWriter::Position::Begin); //Jump to the string table offset
        Writer.WriteInteger(StringTableJumpback - 4, sizeof(uint32_t), mBigEndian);

        Writer.Seek(0x14, application::util::BinaryVectorWriter::Position::Begin);
        for (uint32_t HashKeyOffset : HashKeyOffsets)
        {
            Writer.WriteInteger(HashKeyOffset, sizeof(uint32_t), mBigEndian);
        }

        Writer.Seek(StringTableJumpback, application::util::BinaryVectorWriter::Position::Begin);
        for (uint32_t StringOffset : StringOffsets)
        {
            Writer.WriteInteger(StringOffset, sizeof(uint32_t), mBigEndian);
        }

        //std::vector<unsigned char> Data = Writer.GetData();
        //Data.resize(Data.size() - 1);

        return Writer.GetData();
    }

    void BymlFile::WriteToFile(const std::string& Path, BymlFile::TableGeneration TableGeneration)
    {
        std::ofstream File(Path, std::ios::binary);
        std::vector<unsigned char> Binary = ToBinary(TableGeneration);
        std::copy(Binary.cbegin(), Binary.cend(),
            std::ostream_iterator<unsigned char>(File));
        File.close();
    }

    void BymlFile::Initialize(const std::vector<unsigned char>& Bytes)
    {
        application::util::BinaryVectorReader Reader(Bytes);

        char Magic[2]; //Magic, should be YB or BY
        Reader.ReadStruct(Magic, 2);
        if ((Magic[0] != 'Y' && Magic[0] != 'B') && (Magic[1] != 'Y' && Magic[1] != 'B'))
        {
            application::util::Logger::Error("BymlFile", "Wrong magic, expected YB or BY");
            return;
        }

        if (Magic[0] == 'B' && Magic[1] == 'Y')
        {
            mBigEndian = true;
        }

        uint16_t Version = Reader.ReadUInt16(mBigEndian); //Version should be 7 or 2
        if (Version != 0x07 && Version != 0x02)
        {
            application::util::Logger::Error("BymlFile", "Wrong version, expected v2 or v7, got v%u", Version);
            return;
        }

        uint32_t KeyTableOffset = Reader.ReadUInt32(mBigEndian);
        uint32_t StringTableOffset = Reader.ReadUInt32(mBigEndian);
        uint32_t DataOffset = Reader.ReadUInt32(mBigEndian);

        if (KeyTableOffset != 0) //If equal to 0, there is no Key Table
        {
            ParseTable(Reader, &mHashKeyTable, KeyTableOffset);
        }

        if (StringTableOffset != 0) //If equal to 0, there is no String Table
        {
            ParseTable(Reader, &mStringTable, StringTableOffset);
        }

        /* Parsing actual BYML data */
        Reader.Seek(DataOffset, application::util::BinaryVectorReader::Position::Begin);
        uint8_t RootNodeType = Reader.ReadUInt8();
        if (RootNodeType != (uint8_t)BymlFile::Type::Array && RootNodeType != (uint8_t)BymlFile::Type::Dictionary)
        {
            application::util::Logger::Error("BymlFile", "Expected array (0xC0) or dictionary (0xC1), but got %u", (int)RootNodeType);
            return;
        }

        BymlFile::Node RootNode(RootNodeType == 0xC0 ? BymlFile::Type::Array : BymlFile::Type::Dictionary, "root");
        RootNode.GetChildren().resize(1);
        ParseNode(Reader, 12, RootNode.GetType(), "root", &RootNode, 0);

        mNodes.resize(RootNode.GetChildren()[0].GetChildren().size());
        for (int i = 0; i < RootNode.GetChildren()[0].GetChildren().size(); i++)
        {
            mNodes[i] = RootNode.GetChildren()[0].GetChildren()[i];
        }

        mType = RootNode.GetType();
    }

    BymlFile::BymlFile(const std::vector<unsigned char>& Bytes)
    {
        Initialize(Bytes);
    }

    BymlFile::BymlFile(const std::string& Path)
    {
        std::ifstream File(Path, std::ios::binary);

        if (!File.eof() && !File.fail())
        {
            File.seekg(0, std::ios_base::end);
            std::streampos FileSize = File.tellg();

            std::vector<unsigned char> Bytes(FileSize);

            File.seekg(0, std::ios_base::beg);
            File.read(reinterpret_cast<char*>(Bytes.data()), FileSize);

            Initialize(Bytes);

            File.close();
        }
        else
        {
            application::util::Logger::Error("BymlFile", "Could not open file \"%s\"", Path.c_str());
        }
    }
}