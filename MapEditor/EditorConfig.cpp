#include "EditorConfig.h"

#include "BinaryVectorReader.h"
#include "BinaryVectorWriter.h"
#include "Editor.h"
#include "Logger.h"
#include <fstream>
#include <iterator>

void EditorConfig::Save()
{
    BinaryVectorWriter Writer;

    Writer.WriteBytes("EDTC");
    Writer.WriteByte(0x02); // Version
    Writer.WriteInteger(Editor::RomFSDir.length(), sizeof(uint32_t));
    Writer.WriteInteger(Editor::BfresDir.length(), sizeof(uint32_t));
    Writer.WriteInteger(Editor::ExportDir.length(), sizeof(uint32_t));

    Writer.WriteBytes(Editor::RomFSDir.c_str());
    Writer.WriteBytes(Editor::BfresDir.c_str());
    Writer.WriteBytes(Editor::ExportDir.c_str());

    std::ofstream File(Editor::GetWorkingDirFile("Editor.edtc"), std::ios::binary);
    std::copy(Writer.GetData().cbegin(), Writer.GetData().cend(),
        std::ostream_iterator<unsigned char>(File));
    File.close();
}

void EditorConfig::Load()
{
    std::ifstream File(Editor::GetWorkingDirFile("Editor.edtc"), std::ios::binary);
    File.seekg(0, std::ios_base::end);
    std::streampos FileSize = File.tellg();
    std::vector<unsigned char> Bytes(FileSize);
    File.seekg(0, std::ios_base::beg);
    File.read(reinterpret_cast<char*>(Bytes.data()), FileSize);
    File.close();

    BinaryVectorReader Reader(Bytes);

    char magic[5];
    Reader.Read(magic, 4);
    magic[4] = '\0';
    if (strcmp(magic, "EDTC") != 0) {
        Logger::Error("EditorConfigLoader", "Wrong magic, expected EDTC");
        return;
    }

    uint8_t Version = Reader.ReadUInt8();
    if (Version != 0x02) {
        Logger::Error("EditorConfigLoader", "Wrong version, expected 2, but got " + std::to_string((int)Version));
        return;
    }

    uint32_t RomFSLength = Reader.ReadUInt32();
    uint32_t BFRESLength = Reader.ReadUInt32();
    uint32_t ExportPathLength = Reader.ReadUInt32();

    Editor::RomFSDir = "";
    Editor::BfresDir = "";
    Editor::ExportDir = "";

    for (int i = 0; i < RomFSLength; i++) {
        Editor::RomFSDir += Reader.ReadInt8();
    }
    for (int i = 0; i < BFRESLength; i++) {
        Editor::BfresDir += Reader.ReadInt8();
    }
    for (int i = 0; i < ExportPathLength; i++) {
        Editor::ExportDir += Reader.ReadInt8();
    }
}