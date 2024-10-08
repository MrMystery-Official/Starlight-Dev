#include "Byml.h"

#include <fstream>
#include <algorithm>
#include <chrono>
#include "Logger.h"
#include "StarlightData.h"

/* --- Node --- */
BymlFile::Node::Node(BymlFile::Type Type, std::string Key) : m_Type(Type), m_Key(Key)
{
}

BymlFile::Type& BymlFile::Node::GetType()
{
    return this->m_Type;
}

std::string& BymlFile::Node::GetKey()
{
    return this->m_Key;
}

template<> void BymlFile::Node::SetValue(std::string Value) //Value is a string
{
    this->m_Value.clear();
    this->m_Value.resize(Value.length());
    memcpy(this->m_Value.data(), Value.data(), Value.length());
}

template<class T> void BymlFile::Node::SetValue(T Value) //Value is a default C++ type
{
    this->m_Value.clear();
    this->m_Value.resize(sizeof(Value));
    memcpy(this->m_Value.data(), &Value, sizeof(Value));
}

std::vector<BymlFile::Node>& BymlFile::Node::GetChildren()
{
    return this->m_Children;
}

void BymlFile::Node::AddChild(BymlFile::Node Node)
{
    this->m_Children.push_back(Node);
}

bool BymlFile::Node::HasChild(std::string Key)
{
    for (BymlFile::Node& Node : this->m_Children)
    {
        if (Node.m_Key == Key)
        {
            return true;
        }
    }
    return false;
}

BymlFile::Node* BymlFile::Node::GetChild(std::string Key)
{
    for (BymlFile::Node& Node : this->m_Children)
    {
        if (Node.m_Key == Key)
        {
            return &Node;
        }
    }

    return nullptr;
}

BymlFile::Node* BymlFile::Node::GetChild(int Index)
{
    return &this->m_Children[Index];
}

bool BymlFile::Node::operator==(const BymlFile::Node& Comp) const
{
    return m_Key == Comp.m_Key &&
        m_Value == Comp.m_Value &&
        m_Type == Comp.m_Type &&
        m_Children == Comp.m_Children;
}

/* --- Node --- */

bool BymlFile::HasChild(std::string Key)
{
    for (BymlFile::Node& Node : this->m_Nodes)
    {
        if (Node.GetKey() == Key)
        {
            return true;
        }
    }
    return false;
}

void BymlFile::ParseTable(BinaryVectorReader& Reader, std::vector<std::string>* Dest, int TableOffset)
{
    Reader.Seek(TableOffset, BinaryVectorReader::Position::Begin);
    if (Reader.ReadUInt8() != (uint8_t)BymlFile::Type::KeyTable)
    {
        Logger::Error("BymlDecoder", "Wrong node type, could not parse key table");
        return;
    }
    uint32_t TableSize = Reader.ReadUInt24(mBigEndian);
    Dest->resize(TableSize);
    for (uint32_t i = 0; i < TableSize; i++)
    {
        Reader.Seek(TableOffset + 4 + 4 * i, BinaryVectorReader::Position::Begin);
        Reader.Seek(TableOffset + Reader.ReadUInt32(mBigEndian), BinaryVectorReader::Position::Begin);
        char CurrentCharacter = Reader.ReadInt8();
        Dest->at(i) += CurrentCharacter;
        while (CurrentCharacter != 0x00)
        {
            CurrentCharacter = Reader.ReadInt8();
            Dest->at(i) += CurrentCharacter;
        }
        Dest->at(i).pop_back();
    }
}

int BymlFile::AlignUp(int Value, int Size)
{
    return Value + (Size - Value % Size) % Size;
}

void BymlFile::ParseNode(BinaryVectorReader& Reader, int Offset, BymlFile::Type Type, std::string Key, BymlFile::Node* Parent, uint32_t ChildIndex)
{
    if (Type == BymlFile::Type::Null)
    {
        Logger::Error("BymlDecoder", "Got BYML node with null type");
        return;
    }

    BymlFile::Node& Node = Parent->GetChildren()[ChildIndex];
    Node.m_Type = Type;
    Node.m_Key = Key;

    Reader.Seek(Offset, BinaryVectorReader::Position::Begin);

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
        Reader.Seek(Reader.ReadUInt32(mBigEndian), BinaryVectorReader::Position::Begin);
        Node.SetValue<uint64_t>(Reader.ReadUInt64(mBigEndian));
        break;
    }
    case BymlFile::Type::Int64:
    {
        Reader.Seek(Reader.ReadUInt32(mBigEndian), BinaryVectorReader::Position::Begin);
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
        Node.SetValue<std::string>(this->m_StringTable[Reader.ReadUInt32(mBigEndian)]);
        break;
    }
    case BymlFile::Type::Array:
    {
        uint32_t LocalOffset = Reader.ReadUInt32(mBigEndian);
        Reader.Seek(LocalOffset + 1, BinaryVectorReader::Position::Begin);
        uint32_t Size = Reader.ReadUInt24(mBigEndian);
        uint32_t ValueArrayOffset = LocalOffset + 4 + this->AlignUp(Size, 4);
        Node.GetChildren().resize(Size);
        for (uint32_t i = 0; i < Size; i++)
        {
            Reader.Seek(LocalOffset + 4 + i, BinaryVectorReader::Position::Begin);
            this->ParseNode(Reader, ValueArrayOffset + 4 * i, static_cast<BymlFile::Type>(Reader.ReadUInt8()), std::to_string(i), &Node, i);
        }
        break;
    }
    case BymlFile::Type::Dictionary:
    {
        uint32_t LocalOffset = Reader.ReadUInt32(mBigEndian);
        Reader.Seek(LocalOffset + 1, BinaryVectorReader::Position::Begin);
        uint32_t Size = Reader.ReadUInt24(mBigEndian);
        Node.GetChildren().resize(Size);
        uint32_t EntryOffset = LocalOffset + 4;
        for (uint32_t i = 0; i < Size; i++)
        {
            Reader.Seek(EntryOffset, BinaryVectorReader::Position::Begin);
            uint32_t StringOffset = Reader.ReadUInt24(mBigEndian);
            //Reader.Seek(EntryOffset + 3, BinaryVectorReader::Position::Begin);
            this->ParseNode(Reader, EntryOffset + 4, static_cast<BymlFile::Type>(Reader.ReadUInt8()), this->m_HashKeyTable[StringOffset], &Node, i);
            EntryOffset += 8;
        }
        break;
    }
    default:
        Logger::Error("BymlDecoder", "Unsupported node type: " + std::to_string((int)Type));
    }
}

std::vector<BymlFile::Node>& BymlFile::GetNodes()
{
    return this->m_Nodes;
}

BymlFile::Type& BymlFile::GetType()
{
    return this->m_Type;
}

BymlFile::Node* BymlFile::GetNode(std::string Key)
{
    for (BymlFile::Node& Node : this->m_Nodes)
    {
        if (Node.GetKey() == Key)
        {
            return &Node;
        }
    }

    return nullptr;
}

void BymlFile::AddHashKeyTableEntry(std::string Key)
{
    this->m_HashKeyTable.push_back(Key);
}

void BymlFile::AddStringTableEntry(std::string String)
{
    this->m_StringTable.push_back(String);
}

bool BymlFile::IsNumber(const std::string& str)
{
    if (str.empty())
    {
        return false;
    }

    bool hasDigit = false;
    bool hasDecimal = false;
    bool hasSign = false;

    for (char c : str)
    {
        if (isdigit(c))
        {
            hasDigit = true;
        }
        else if (c == '.' && !hasDecimal)
        {
            hasDecimal = true;
        }
        else if ((c == '-' || c == '+') && !hasSign && !hasDigit)
        {
            hasSign = true;
        }
        else
        {
            return false;
        }
    }

    return hasDigit;
}

void BymlFile::GenerateHashKeyTable(BymlFile::Node* Node)
{
    std::string str = Node->GetKey();
    if (!str.empty())
    {
        if (std::find(this->m_HashKeyTable.begin(), this->m_HashKeyTable.end(), str) == this->m_HashKeyTable.end() && !this->IsNumber(str))
        {
            this->m_HashKeyTable.push_back(str);
        }
    }

    for (BymlFile::Node& Nodes : Node->GetChildren())
    {
        this->GenerateHashKeyTable(&Nodes);
    }
}

void BymlFile::GenerateStringTable(BymlFile::Node* Node)
{
    if (Node->GetType() == BymlFile::Type::StringIndex)
    {
        std::string str = Node->GetValue<std::string>();
        if (str.empty())
        {
            this->m_StringTable.push_back(str);
        }
        else if (std::find(this->m_StringTable.begin(), this->m_StringTable.end(), str) == this->m_StringTable.end())
        {
            this->m_StringTable.push_back(str);
        }
    }

    for (BymlFile::Node& Nodes : Node->GetChildren())
    {
        this->GenerateStringTable(&Nodes);
    }
}

uint32_t BymlFile::GetStringTableIndex(std::string Value)
{
    for (int i = 0; i < this->m_StringTable.size(); i++)
    {
        if (this->m_StringTable[i] == Value)
        {
            return i;
        }
    }
    return 0;
}

uint32_t BymlFile::GetHashKeyTableIndex(std::string Value)
{
    for (int i = 0; i < this->m_HashKeyTable.size(); i++)
    {
        if (this->m_HashKeyTable[i] == Value)
        {
            return i;
        }
    }
    return 0;
}

void BymlFile::WriteNode(BinaryVectorWriter& Writer, uint32_t DataOffset, uint32_t Offset, BymlFile::Node& Node)
{
    if (Node.GetType() == BymlFile::Type::Null)
    {
        return;
    }

    Writer.Seek(Offset, BinaryVectorWriter::Position::Begin);

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
        if (this->m_CachedValues.contains(Node.m_Value))
        {
            Writer.WriteInteger(this->m_CachedValues[Node.m_Value], sizeof(uint32_t), mBigEndian);
            break;
        }
        Writer.WriteInteger(DataOffset + this->m_WriterReservedDataOffset, sizeof(uint32_t), mBigEndian);
        Writer.Seek(DataOffset + this->m_WriterReservedDataOffset, BinaryVectorWriter::Position::Begin);
        Writer.WriteInteger(Node.GetValue<uint64_t>(), sizeof(uint64_t), mBigEndian);
        this->m_CachedValues.insert({ Node.m_Value, DataOffset + this->m_WriterReservedDataOffset });
        this->m_WriterReservedDataOffset += 8;
        break;
    }
    case BymlFile::Type::Int64:
    {
        if (this->m_CachedValues.contains(Node.m_Value))
        {
            Writer.WriteInteger(this->m_CachedValues[Node.m_Value], sizeof(uint32_t), mBigEndian);
            break;
        }
        Writer.WriteInteger(DataOffset + this->m_WriterReservedDataOffset, sizeof(uint32_t), mBigEndian);
        Writer.Seek(DataOffset + this->m_WriterReservedDataOffset, BinaryVectorWriter::Position::Begin);
        Writer.WriteInteger(Node.GetValue<int64_t>(), sizeof(int64_t), mBigEndian);
        this->m_CachedValues.insert({ Node.m_Value, DataOffset + this->m_WriterReservedDataOffset });
        this->m_WriterReservedDataOffset += 8;
        break;
    }
    case BymlFile::Type::Double:
    {
        if (this->m_CachedValues.contains(Node.m_Value))
        {
            Writer.WriteInteger(this->m_CachedValues[Node.m_Value], sizeof(uint32_t), mBigEndian);
            break;
        }
        Writer.WriteInteger(DataOffset + this->m_WriterReservedDataOffset, sizeof(uint32_t), mBigEndian);
        Writer.Seek(DataOffset + this->m_WriterReservedDataOffset, BinaryVectorWriter::Position::Begin);
        Writer.WriteInteger(Node.GetValue<double>(), sizeof(double), mBigEndian);
        this->m_CachedValues.insert({ Node.m_Value, DataOffset + this->m_WriterReservedDataOffset });
        this->m_WriterReservedDataOffset += 8;
        break;
    }
    case BymlFile::Type::Array:
    {
        if (!this->m_CachedNodes.contains(Node))
        {
            Writer.WriteInteger(DataOffset + this->m_WriterReservedDataOffset, sizeof(uint32_t), mBigEndian);
            Writer.Seek(DataOffset + this->m_WriterReservedDataOffset, BinaryVectorWriter::Position::Begin);
            uint32_t LocalOffset = DataOffset + this->m_WriterReservedDataOffset;
            Writer.WriteByte(0xC0);
            Writer.WriteInteger(Node.GetChildren().size(), 3, mBigEndian); //3 = uint24
            for (BymlFile::Node& Child : Node.GetChildren())
            {
                Writer.WriteByte((uint8_t)Child.GetType());
            }
            
            Writer.Seek(((Writer.GetPosition() + 3) & ~0x03), BinaryVectorWriter::Position::Begin);

            this->m_WriterReservedDataOffset = Writer.GetPosition() - DataOffset + 4 * Node.GetChildren().size();
            uint32_t EntryOffset = Writer.GetPosition();
            for (int i = 0; i < Node.GetChildren().size(); i++)
            {
                this->WriteNode(Writer, DataOffset, EntryOffset + 4 * i, *Node.GetChild(i));
            }
            this->m_CachedNodes.insert({ Node, LocalOffset });
        }
        else
        {
            Writer.WriteInteger(this->m_CachedNodes[Node], sizeof(uint32_t), mBigEndian);
        }
        break;
    }
    case BymlFile::Type::Dictionary:
    {
        if (!this->m_CachedNodes.contains(Node))
        {
            uint32_t LocalOffset = DataOffset + this->m_WriterReservedDataOffset;
            Writer.WriteInteger(LocalOffset, sizeof(uint32_t), mBigEndian);
            this->m_WriterReservedDataOffset += 4 + 8 * Node.GetChildren().size();
            Writer.Seek(LocalOffset, BinaryVectorWriter::Position::Begin);
            Writer.WriteByte(0xC1);
            Writer.WriteInteger(Node.GetChildren().size(), 3, mBigEndian); // 3 = uint24
            for (int i = 0; i < Node.GetChildren().size(); i++)
            {
                uint32_t EntryOffset = LocalOffset + 4 + 8 * i;
                Writer.Seek(EntryOffset, BinaryVectorWriter::Position::Begin);
                Writer.WriteInteger(GetHashKeyTableIndex(Node.GetChild(i)->GetKey()), 3, mBigEndian);
                Writer.WriteByte((char)Node.GetChild(i)->GetType());
                this->WriteNode(Writer, DataOffset, EntryOffset + 4, *Node.GetChild(i));
                
                Writer.Seek(((Writer.GetPosition() + 3) & ~0x03), BinaryVectorWriter::Position::Begin);
            }
            this->m_CachedNodes.insert({ Node, LocalOffset });
        }
        else
        {
            Writer.WriteInteger(this->m_CachedNodes[Node], sizeof(uint32_t), mBigEndian);
        }
        break;
    }
    default:
        Logger::Error("BymlEncoder", "Unsupported node type: " + std::to_string((int)Node.GetType()));
    }

    if (Writer.GetPosition() > this->m_WriterLastOffset)
    {
        this->m_WriterLastOffset = Writer.GetPosition();
    }
}

std::vector<unsigned char> BymlFile::ToBinary(BymlFile::TableGeneration TableGeneration)
{
    std::chrono::milliseconds StartMS = duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    );

    this->m_WriterLastOffset = 0;
    this->m_WriterReservedDataOffset = 0;
    this->m_CachedValues.clear();
    this->m_CachedNodes.clear();

    BinaryVectorWriter Writer;

    Writer.WriteBytes(mBigEndian ? "BY" : "YB"); //Magic
    Writer.WriteInteger(0x07, sizeof(uint16_t), mBigEndian); //Version
    Writer.WriteInteger(0x10, sizeof(uint32_t), mBigEndian); //Hash Key Table Offset

    Writer.Seek(8, BinaryVectorWriter::Position::Current);

    if (TableGeneration == BymlFile::TableGeneration::Auto)
    {
        this->m_HashKeyTable.resize(0);
        this->m_StringTable.resize(0);

        for (BymlFile::Node& Node : this->m_Nodes)
        {
            this->GenerateHashKeyTable(&Node);
            this->GenerateStringTable(&Node);
        }
    }
    this->m_HashKeyTable.shrink_to_fit();
    this->m_StringTable.shrink_to_fit();
    this->m_StringTable.push_back(StarlightData::GetStringTableSignature());
    std::sort(this->m_HashKeyTable.begin(), this->m_HashKeyTable.end(), [](std::string A, std::string B) { return A < B; });
    std::sort(this->m_StringTable.begin(), this->m_StringTable.end(), [](std::string A, std::string B) { return A < B; });

    Writer.WriteByte(0xC2); //Key Table Node
    Writer.WriteInteger(this->m_HashKeyTable.size(), 3, mBigEndian); //3 = uint24
    Writer.Seek(this->m_HashKeyTable.size() * 4 + 4, BinaryVectorWriter::Position::Current);

    std::vector<uint32_t> HashKeyOffsets;

    for (std::string& HashKey : this->m_HashKeyTable)
    {
        HashKeyOffsets.push_back(Writer.GetPosition() - 16);
        Writer.WriteBytes(HashKey.c_str());
        Writer.WriteByte(0xC00);
    }
    HashKeyOffsets.push_back(Writer.GetPosition() - 16);
    while (Writer.GetPosition() % 4 != 0) //Align
    {
        Writer.WriteByte(0x00);
    }


    Writer.WriteByte(0xC2);
    Writer.WriteInteger(this->m_StringTable.size(), 3, mBigEndian); //3 = uint24
    uint32_t StringTableJumpback = Writer.GetPosition();
    Writer.Seek(this->m_StringTable.size() * 4 + 4, BinaryVectorWriter::Position::Current);
    std::vector<uint32_t> StringOffsets;
    for (std::string String : this->m_StringTable)
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
    BymlFile::Node RootNode(this->m_Type, "root");
    for (BymlFile::Node& Node : this->m_Nodes)
    {
        RootNode.AddChild(Node);
    }
    this->WriteNode(Writer, DataOffset, 12, RootNode);

    Writer.Seek(0x08, BinaryVectorWriter::Position::Begin); //Jump to the string table offset
    Writer.WriteInteger(StringTableJumpback - 4, sizeof(uint32_t), mBigEndian);

    Writer.Seek(0x14, BinaryVectorWriter::Position::Begin);
    for (uint32_t HashKeyOffset : HashKeyOffsets)
    {
        Writer.WriteInteger(HashKeyOffset, sizeof(uint32_t), mBigEndian);
    }

    Writer.Seek(StringTableJumpback, BinaryVectorWriter::Position::Begin);
    for (uint32_t StringOffset : StringOffsets)
    {
        Writer.WriteInteger(StringOffset, sizeof(uint32_t), mBigEndian);
    }

    //std::vector<unsigned char> Data = Writer.GetData();
    //Data.resize(Data.size() - 1);

    Logger::Info("BymlEncoder", "Encoded BYML data, took " + std::to_string((duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ) - StartMS).count()) + "ms");

    return Writer.GetData();
}

void BymlFile::WriteToFile(std::string Path, BymlFile::TableGeneration TableGeneration)
{
    std::ofstream File(Path, std::ios::binary);
    std::vector<unsigned char> Binary = this->ToBinary(TableGeneration);
    std::copy(Binary.cbegin(), Binary.cend(),
        std::ostream_iterator<unsigned char>(File));
    File.close();
}

BymlFile::BymlFile(std::vector<unsigned char> Bytes)
{
    BinaryVectorReader Reader(Bytes);

    char Magic[2]; //Magic, should be YB or BY
    Reader.Read(Magic, 2);
    if ((Magic[0] != 'Y' && Magic[0] != 'B') && (Magic[1] != 'Y' && Magic[1] != 'B'))
    {
        Logger::Error("BymlDecoder", "Wrong magic, expected YB or BY");
        return;
    }

    if (Magic[0] == 'B' && Magic[1] == 'Y')
    {
        mBigEndian = true;
    }

    uint16_t Version = Reader.ReadUInt16(mBigEndian); //Version should be 7 or 2
    if (Version != 0x07 && Version != 0x02)
    {
        Logger::Error("BymlDecoder", "Wrong version, expected v2 or v7, got v" + std::to_string(Version));
        return;
    }

    uint32_t KeyTableOffset = Reader.ReadUInt32(mBigEndian);
    uint32_t StringTableOffset = Reader.ReadUInt32(mBigEndian);
    uint32_t DataOffset = Reader.ReadUInt32(mBigEndian);

    if (KeyTableOffset != 0) //If equal to 0, there is no Key Table
    {
        this->ParseTable(Reader, &this->m_HashKeyTable, KeyTableOffset);
    }

    if (StringTableOffset != 0) //If equal to 0, there is no String Table
    {
        this->ParseTable(Reader, &this->m_StringTable, StringTableOffset);
    }

    /* Parsing actual BYML data */
    Reader.Seek(DataOffset, BinaryVectorReader::Position::Begin);
    uint8_t RootNodeType = Reader.ReadUInt8();
    if (RootNodeType != (uint8_t)BymlFile::Type::Array && RootNodeType != (uint8_t)BymlFile::Type::Dictionary)
    {
        Logger::Error("BymlDecoder", "Expected array (0xC0) or dictionary (0xC1), but got " + std::to_string((int)RootNodeType));
        return;
    }

    BymlFile::Node RootNode(RootNodeType == 0xC0 ? BymlFile::Type::Array : BymlFile::Type::Dictionary, "root");
    RootNode.GetChildren().resize(1);
    this->ParseNode(Reader, 12, RootNode.GetType(), "root", &RootNode, 0);

    this->m_Nodes.resize(RootNode.GetChildren()[0].GetChildren().size());
    for (int i = 0; i < RootNode.GetChildren()[0].GetChildren().size(); i++)
    {
        this->m_Nodes[i] = RootNode.GetChildren()[0].GetChildren()[i];
    }

    this->m_Type = RootNode.GetType();
}

BymlFile::BymlFile(std::string Path)
{
    std::ifstream File(Path, std::ios::binary);

    if (!File.eof() && !File.fail())
    {
        File.seekg(0, std::ios_base::end);
        std::streampos FileSize = File.tellg();

        std::vector<unsigned char> Bytes(FileSize);

        File.seekg(0, std::ios_base::beg);
        File.read(reinterpret_cast<char*>(Bytes.data()), FileSize);

        this->BymlFile::BymlFile(Bytes);

        File.close();
    }
    else
    {
        Logger::Error("BymlDecoder", "Could not open file " + Path);
    }
}