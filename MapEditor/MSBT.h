#pragma once

#include "BinaryVectorReader.h"
#include <string>
#include <unordered_map>
#include <vector>

class MSBTFile {
public:
    struct MSBTHeader {
        char Magic[8];
        uint16_t ByteOrderMask = 0;
        uint16_t Padding = 0;
        uint8_t Encoding = 0;
        uint8_t Version = 0;
        uint16_t SectionCount = 0;
        uint16_t Unknown1 = 0;
        uint32_t FileSize = 0;
    };

    enum class StringEncodingStruct : uint8_t {
        UTF_8 = 0,
        Unicode = 1
    };

    struct StringEntry {
        std::string Text;
        StringEncodingStruct Encoding;
        uint32_t Index = 0;
    };

    MSBTHeader Header;
    StringEncodingStruct StringEncoding;
    std::vector<std::vector<std::string>> Groups;
    std::unordered_map<std::string, StringEntry> Text;
    bool Loaded = false;
    std::string FileName = ""; // Not needed, just for the MSBTEditor UI

    std::string ReadString(std::vector<uint16_t> Data, uint16_t Start = 0);
    void ParseFunction(std::string& Text, uint16_t Group, uint16_t Type, std::vector<uint16_t> Args);
    void ReadLBL1(BinaryVectorReader& Reader);
    void ReadTXT2(BinaryVectorReader& Reader);

    void MessageToBinary(std::vector<uint16_t>& Bytes, std::string Message);
    std::vector<unsigned char> ToBinary();
    void WriteToFile(std::string Path);

    MSBTFile() { }
    MSBTFile(std::vector<unsigned char> Bytes);
    MSBTFile(std::string Path);

private:
    struct LabelEntry {
        uint8_t Length = 0;
        std::string Name = "";
        uint32_t Index = 0;
        uint32_t Checksum = 0;
    };

    std::vector<LabelEntry> m_LabelData;
    std::vector<StringEntry> m_StringData;
};
