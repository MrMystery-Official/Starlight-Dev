#include "SarcFile.h"

#include <util/Logger.h>

namespace application::file::game
{

    void SarcFile::Entry::WriteToFile(const std::string& Path)
    {
        std::ofstream File(Path, std::ios::binary);
        std::copy(mBytes.begin(), mBytes.end(),
            std::ostream_iterator<unsigned char>(File));
        File.close();
    }

    std::vector<SarcFile::Entry>& SarcFile::GetEntries()
    {
        return mEntries;
    }

    SarcFile::Entry& SarcFile::GetEntry(const std::string& Name)
    {
        for (SarcFile::Entry& Entry : mEntries)
        {
            if (Entry.mName == Name)
            {
                return Entry;
            }
        }
        return mEntries[0];
    }

    bool SarcFile::HasEntry(const std::string& Name)
    {
        for (SarcFile::Entry& Entry : mEntries)
        {
            if (Entry.mName == Name)
            {
                return true;
            }
        }
        return false;
    }

    bool SarcFile::HasDirectory(const std::string& Path)
    {
        for (SarcFile::Entry& Entry : mEntries)
        {
            if (Entry.mName.rfind(Path, 0) == 0)
            {
                return true;
            }
        }
        return false;
    }

    void SarcFile::Initialize(std::vector<unsigned char> Bytes)
    {
        application::util::BinaryVectorReader Reader(Bytes);
        char Magic[4]; //Magic, should be SARC
        Reader.ReadStruct(Magic, 4);
        if (Magic[0] != 'S' || Magic[1] != 'A' || Magic[2] != 'R' || Magic[3] != 'C')
        {
            application::util::Logger::Error("SarcFile", "Wrong magic, expected SARC");
            return;
        }

        Reader.Seek(4, application::util::BinaryVectorReader::Position::Current); //Doesn't matter

        uint32_t FileSize = Reader.ReadUInt32();
        uint32_t DataOffset = Reader.ReadUInt32();

        Reader.Seek(10, application::util::BinaryVectorReader::Position::Current); //Padding

        uint16_t Count = Reader.ReadUInt16();

        Reader.Seek(4, application::util::BinaryVectorReader::Position::Current); //Padding

        std::vector<SarcFile::Node> Nodes(Count);
        for (int i = 0; i < Count; i++)
        {
            SarcFile::Node& Node = Nodes[i];
            Node.mHash = Reader.ReadUInt32();

            int Attributes = Reader.ReadInt32();

            Node.mDataStart = Reader.ReadUInt32();
            Node.mDataEnd = Reader.ReadUInt32();
            Node.mStringOffset = (Attributes & 0xFFFF) * 4;
        }

        Reader.Seek(8, application::util::BinaryVectorReader::Position::Current); //Padding

        mEntries.resize(Count);

        std::vector<char> StringTableBuffer(DataOffset - Reader.GetPosition()); //This vector will hold the names of the Files inside SARC
        Reader.ReadStruct(reinterpret_cast<char*>(StringTableBuffer.data()), DataOffset - Reader.GetPosition()); //Reading String Data into StringTableBuffer

        for (int i = 0; i < Count; i++)
        {
            Reader.Seek(DataOffset + Nodes[i].mDataStart, application::util::BinaryVectorReader::Position::Begin);
            std::string Name = "";
            if (i == (Count - 1))
            {
                for (uint32_t j = Nodes[i].mStringOffset; j < StringTableBuffer.size() - 1; j++)
                {
                    if (std::isprint(StringTableBuffer[j]))
                    {
                        Name += StringTableBuffer[j];
                    }
                    else
                    {
                        break;
                    }
                }

                SarcFile::Entry& Entry = mEntries[i];
                Entry.mBytes.resize(Nodes[i].mDataEnd - Nodes[i].mDataStart);
                Reader.ReadStruct(reinterpret_cast<char*>(Entry.mBytes.data()), Nodes[i].mDataEnd - Nodes[i].mDataStart);
                Entry.mName = Name;
            }
            else
            {
                for (uint32_t j = Nodes[i].mStringOffset; j < Nodes[i + 1].mStringOffset; j++)
                {
                    if (std::isprint(StringTableBuffer[j]))
                    {
                        Name += StringTableBuffer[j];
                    }
                    else
                    {
                        break;
                    }
                }

                SarcFile::Entry& Entry = mEntries[i];
                Entry.mBytes.resize(Nodes[i].mDataEnd - Nodes[i].mDataStart);
                Reader.ReadStruct(reinterpret_cast<char*>(Entry.mBytes.data()), Nodes[i].mDataEnd - Nodes[i].mDataStart);
                Entry.mName = Name;
            }
        }

        mLoaded = true;
    }

    int SarcFile::GCD(int a, int b)
    {
        while (a != 0 && b != 0)
        {
            if (a > b)
                a %= b;
            else
                b %= a;
        }
        return a | b;
    }

    int SarcFile::LCM(int a, int b)
    {
        return a / GCD(a, b) * b;
    }

    int SarcFile::GetBinaryFileAlignment(std::vector<unsigned char> Data)
    {
        if (Data.size() <= 0x20)
        {
            return 1;
        }

        if (Data[0] == 'A' && Data[1] == 'I' && Data[2] == 'B') //AINB has alignment 8
            return 8;

        int32_t FileSize = *reinterpret_cast<const int32_t*>(&Data[0x1C]);
        if (FileSize != static_cast<int32_t>(Data.size()))
        {
            return 1;
        }

        return 1 << Data[0x0E];
    }

    int SarcFile::AlignUp(int Value, int Size)
    {
        return Value + (Size - Value % Size) % Size;
    }

    bool SarcFile::AlignWriter(application::util::BinaryVectorWriter& Writer, int Alignment)
    {
        bool Aligned = false;
        while (Writer.GetPosition() % Alignment != 0)
        {
            Writer.Seek(1, application::util::BinaryVectorWriter::Position::Current);
            Aligned = true;
        }
        return Aligned;
    }

    std::vector<unsigned char> SarcFile::ToBinary()
    {
        application::util::BinaryVectorWriter Writer;

        Writer.Seek(0x14, application::util::BinaryVectorWriter::Position::Begin);

        uint64_t Count = mEntries.size();

        Writer.WriteBytes("SFAT");
        Writer.WriteInteger(0x0C, sizeof(uint16_t));
        Writer.WriteInteger(Count, sizeof(uint16_t));
        Writer.WriteInteger(0x65, sizeof(uint32_t));

        std::vector<SarcFile::HashValue> Keys(Count);
        for (int i = 0; i < Count; i++)
        {
            SarcFile::HashValue HashValue;
            HashValue.mNode = &mEntries[i];
            int j = 0;
            while (true)
            {
                char c = mEntries[i].mName[j++];
                if (!c)
                    break;
                HashValue.mHash = HashValue.mHash * 0x65 + c;
            }
            Keys[i] = HashValue;
        }
        std::sort(Keys.begin(), Keys.end(), [](const SarcFile::HashValue& a, const SarcFile::HashValue& b) { return a.mHash < b.mHash; });

        uint32_t RelStringOffset = 0;
        uint32_t RelDataOffset = 0;
        uint32_t FileAignment = 1;
        std::vector<uint32_t> Alignments(Count);

        for (int i = 0; i < Count; i++)
        {
            std::string FileName = Keys[i].mNode->mName;
            std::vector<unsigned char> Buffer(Keys[i].mNode->mBytes.begin(), Keys[i].mNode->mBytes.end());

            uint32_t Alignment = LCM(4, GetBinaryFileAlignment(Buffer));
            uint32_t DataStart = RelDataOffset;
            DataStart = AlignUp(DataStart, Alignment);
            uint32_t DataEnd = DataStart + Buffer.size();
            Alignments[i] = Alignment;

            Writer.WriteInteger(Keys[i].mHash, sizeof(uint32_t));
            Writer.WriteInteger((0x01000000 | (RelStringOffset / 4)), sizeof(int32_t)); //Placeholder
            Writer.WriteInteger(DataStart, sizeof(uint32_t)); //Placeholder
            Writer.WriteInteger(DataEnd, sizeof(uint32_t)); //Placeholder

            RelDataOffset = DataEnd;
            RelStringOffset += (FileName.length() + 4) & -4;
            FileAignment = LCM(FileAignment, Alignment);
        }

        Writer.WriteBytes("SFNT");
        Writer.WriteInteger(0x08, sizeof(uint16_t));
        Writer.WriteInteger(0x00, sizeof(uint16_t));

        for (int i = 0; i < Count; i++)
        {
            std::string FileName = Keys[i].mNode->mName;
            Writer.WriteBytes(FileName.c_str());
            if (!AlignWriter(Writer, 4))
            {
                Writer.Seek(4, application::util::BinaryVectorWriter::Position::Current);
            }
        }

        AlignWriter(Writer, 8);

        uint32_t DataOffset = Writer.GetPosition();
        for (int i = 0; i < Count; i++)
        {
            AlignWriter(Writer, Alignments[i]);

            for (unsigned char Byte : Keys[i].mNode->mBytes)
            {
                Writer.WriteByte(Byte);
            }
        }

        uint32_t FileSize = Writer.GetData().size();

        Writer.Seek(0, application::util::BinaryVectorWriter::Position::Begin);

        Writer.WriteBytes("SARC");
        Writer.WriteByte(0x14);
        Writer.WriteByte(0x00);
        Writer.WriteByte(0xFF);
        Writer.WriteByte(0xFE);

        Writer.WriteInteger(FileSize, sizeof(uint32_t));
        Writer.WriteInteger(DataOffset, sizeof(uint32_t));
        Writer.WriteInteger(0x100, sizeof(uint16_t));

        return Writer.GetData();
    }

    void SarcFile::WriteToFile(const std::string& Path)
    {
        std::ofstream File(Path, std::ios::binary);
        std::vector<unsigned char> Binary = ToBinary();
        std::copy(Binary.begin(), Binary.end(),
            std::ostream_iterator<unsigned char>(File));
        File.close();
    }

    SarcFile::SarcFile(std::vector<unsigned char> Bytes)
    {
        Initialize(Bytes);
    }

    SarcFile::SarcFile(const std::string& Path)
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
            application::util::Logger::Error("SarcFile", "Could not open file\"%s\"", Path.c_str());
        }
    }
}