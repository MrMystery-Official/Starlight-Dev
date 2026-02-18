#include "ResTblFile.h"

#include <fstream>
#include <algorithm>
#include <util/BinaryVectorReader.h>
#include <util/BinaryVectorWriter.h>
#include <util/Logger.h>

namespace application::file::game::restbl
{
    void ResTableFile::GenerateCrcTable()
    {
        mCrcTable.resize(256);
        for (uint32_t i = 0; i < 256; i++)
        {
            uint32_t c = i;
            for (size_t j = 0; j < 8; j++)
            {
                if (c & 1)
                {
                    c = 0xEDB88320 ^ (c >> 1);
                }
                else
                {
                    c >>= 1;
                }
            }
            mCrcTable[i] = c;
        }
    }

    uint32_t ResTableFile::GenerateCrc32Hash(const std::string& Key)
    {
        uint32_t CRC = 0xFFFFFFFF;
        const uint8_t* u = reinterpret_cast<const uint8_t*>(Key.c_str());
        for (size_t i = 0; i < Key.length(); ++i)
        {
            CRC = mCrcTable[(CRC ^ u[i]) & 0xFF] ^ (CRC >> 8);
        }
        return CRC ^ 0xFFFFFFFF;
    }

    uint32_t ResTableFile::GetFileSize(uint32_t Hash)
    {
        for (ResTableFile::CrcEntry& CrcEntry : mCrcEntries)
        {
            if (CrcEntry.mHash == Hash)
            {
                return CrcEntry.mSize;
            }
        }
        return 0;
    }

    uint32_t ResTableFile::GetFileSize(const std::string& Key)
    {
        for (ResTableFile::NameEntry& NameEntry : mNameEntries)
        {
            if (NameEntry.mName == Key)
            {
                return NameEntry.mSize;
            }
        }
        return GetFileSize(GenerateCrc32Hash(Key));
    }

    void ResTableFile::SetFileSize(uint32_t Hash, uint32_t Value)
    {
        for (ResTableFile::CrcEntry& CrcEntry : mCrcEntries)
        {
            if (CrcEntry.mHash == Hash)
            {
                CrcEntry.mSize = Value;
                return;
            }
        }

        mCrcEntries.push_back({ Hash, Value });
    }

    void ResTableFile::SetFileSize(const std::string& Key, uint32_t Value)
    {
        for (ResTableFile::NameEntry& NameEntry : mNameEntries)
        {
            if (NameEntry.mName == Key)
            {
                NameEntry.mSize = Value;
                return;
            }
        }
        SetFileSize(GenerateCrc32Hash(Key), Value);
    }

    void ResTableFile::Initialize(std::vector<unsigned char> Data)
    {
        GenerateCrcTable();

        application::util::BinaryVectorReader Reader(Data);

        //Checking magic, should be RESTBL
        char Magic[6];
        Reader.ReadStruct(&Magic[0], 6);
        if (Magic[0] != 'R' || Magic[1] != 'E' || Magic[2] != 'S' || Magic[3] != 'T' || Magic[4] != 'B' || Magic[5] != 'L')
        {
            application::util::Logger::Error("ResTableFile", "Wrong magic, expected RESTBL");
            return;
        }

        mHeader.mVersion = Reader.ReadUInt32();
        mHeader.mStringBlockSize = Reader.ReadUInt32();
        mHeader.mCrcMapCount = Reader.ReadUInt32();
        mHeader.mNameMapCount = Reader.ReadUInt32();

        mCrcEntries.resize(mHeader.mCrcMapCount);
        for (uint32_t i = 0; i < mHeader.mCrcMapCount; i++)
        {
            mCrcEntries[i] = { Reader.ReadUInt32(), Reader.ReadUInt32() };
        }

        mNameEntries.resize(mHeader.mNameMapCount);
        for (uint32_t i = 0; i < mHeader.mNameMapCount; i++)
        {
            uint32_t Offset = 22 + (mHeader.mCrcMapCount * 8) + ((mHeader.mStringBlockSize + 4) * i);
            Reader.Seek(Offset, application::util::BinaryVectorReader::Position::Begin);
            ResTableFile::NameEntry NameEntry;
            for (uint32_t j = 0; j < mHeader.mStringBlockSize; j++)
            {
                char Character = Reader.ReadInt8();
                if (Character != 0x00)
                {
                    NameEntry.mName += Character;
                }
            }
            NameEntry.mSize = Reader.ReadUInt32();
            mNameEntries[i] = NameEntry;
        }
    }

    ResTableFile::ResTableFile(std::vector<unsigned char> Data)
    {
        Initialize(Data);
    }

    ResTableFile::ResTableFile(const std::string& Path)
    {
        std::ifstream File(Path, std::ios::binary);

        if (!File.eof() && !File.fail())
        {
            File.seekg(0, std::ios_base::end);
            std::streampos FileSize = File.tellg();

            std::vector<unsigned char> Bytes(FileSize);

            File.seekg(0, std::ios_base::beg);
            File.read(reinterpret_cast<char*>(Bytes.data()), FileSize);

            File.close();

            Initialize(Bytes);
        }
        else
        {
            application::util::Logger::Error("ResTableFile", "Could not open file\"%s\"", Path.c_str());
        }
    }

    std::vector<unsigned char> ResTableFile::ToBinary()
    {
        application::util::BinaryVectorWriter Writer;

        Writer.WriteBytes("RESTBL");
        Writer.WriteInteger(mHeader.mVersion, sizeof(uint32_t)); //Version
        Writer.WriteInteger(mHeader.mStringBlockSize, sizeof(uint32_t)); //StringBlockSize
        Writer.WriteInteger(mCrcEntries.size(), sizeof(uint32_t)); //CrcMapCount
        Writer.WriteInteger(mNameEntries.size(), sizeof(uint32_t)); //NameMapCount

        std::sort(mCrcEntries.begin(), mCrcEntries.end(), [](const ResTableFile::CrcEntry& A, const ResTableFile::CrcEntry& B) { return A.mHash < B.mHash; });

        for (const ResTableFile::CrcEntry& CrcEntry : mCrcEntries)
        {
            Writer.WriteInteger(CrcEntry.mHash, sizeof(uint32_t));
            Writer.WriteInteger(CrcEntry.mSize, sizeof(uint32_t));
        }

        for (const ResTableFile::NameEntry& NameEntry : mNameEntries)
        {
            Writer.WriteBytes(NameEntry.mName.c_str());
            Writer.Seek(mHeader.mStringBlockSize - NameEntry.mName.length(), application::util::BinaryVectorWriter::Position::Current);
            Writer.WriteInteger(NameEntry.mSize, sizeof(uint32_t));
        }

        return Writer.GetData();
    }

    void ResTableFile::WriteToFile(const std::string &Path)
    {
        std::ofstream File(Path, std::ios::binary);
        std::vector<unsigned char> Binary = ToBinary();
        std::copy(Binary.begin(), Binary.end(),
            std::ostream_iterator<unsigned char>(File));
        File.close();
    }
}