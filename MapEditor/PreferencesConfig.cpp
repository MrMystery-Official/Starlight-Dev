#include "PreferencesConfig.h"

#include "BinaryVectorReader.h"
#include "BinaryVectorWriter.h"
#include "Editor.h"
#include "Logger.h"
#include <fstream>
#include <iterator>

#include "UIAINBEditor.h"
#include "UIActorTool.h"
#include "UIConsole.h"
#include "UIMSBTEditor.h"
#include "UIMapView.h"
#include "UIOutliner.h"
#include "UIProperties.h"
#include "UITools.h"

unsigned int PreferencesConfig::FrameCount = 0;
PreferencesConfig::WindowInformationStruct PreferencesConfig::WindowInformation;

void PreferencesConfig::Load()
{
    std::ifstream File(Editor::GetWorkingDirFile("Preferences.eprf"), std::ios::binary);
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
    if (strcmp(magic, "EPRF") != 0) {
        Logger::Error("EditorPreferenceLoader", "Wrong magic, expected EPRF");
        return;
    }

    uint8_t Version = Reader.ReadUInt8();
    if (Version != 0x01) {
        Logger::Error("EditorPreferenceLoader", "Wrong version, expected 1, but got " + std::to_string((int)Version));
        return;
    }

    UIAINBEditor::Open = (bool)Reader.ReadUInt8();
    UITools::Open = (bool)Reader.ReadUInt8();
    UIProperties::Open = (bool)Reader.ReadUInt8();
    UIOutliner::Open = (bool)Reader.ReadUInt8();
    UIMSBTEditor::Open = (bool)Reader.ReadUInt8();
    UIMapView::Open = (bool)Reader.ReadUInt8();
    UIConsole::Open = (bool)Reader.ReadUInt8();
    UIActorTool::Open = (bool)Reader.ReadUInt8();

    UpdateWindowInformation();

    BfresLibrary::Models.clear();
    TextureToGoLibrary::Textures.clear();
    GLTextureLibrary::Textures.clear();

    uint16_t ColorSize = Reader.ReadUInt16();
    for (int i = 0; i < ColorSize; i++) {
        std::string Name;
        Name.resize(Reader.ReadUInt8());
        Reader.Read(Name.data(), Name.length());
        Logger::Info("Name", Name);
        uint8_t R = Reader.ReadUInt8();
        uint8_t G = Reader.ReadUInt8();
        uint8_t B = Reader.ReadUInt8();
        uint8_t A = Reader.ReadUInt8();

        BfresLibrary::Models.insert({ Name, BfresFile::CreateDefaultModel(Name, R, G, B, A) });
    }

    if (!BfresLibrary::Models.count("Default")) {
        BfresLibrary::Models.insert({ "Default", BfresFile::CreateDefaultModel("Default", 0, 0, 255, 255) });
    }
}

void PreferencesConfig::Save()
{
    BinaryVectorWriter Writer;

    Writer.WriteBytes("EPRF");
    Writer.WriteByte(0x01); // Version

    Writer.WriteByte((uint8_t)UIAINBEditor::Open);
    Writer.WriteByte((uint8_t)UITools::Open);
    Writer.WriteByte((uint8_t)UIProperties::Open);
    Writer.WriteByte((uint8_t)UIOutliner::Open);
    Writer.WriteByte((uint8_t)UIMSBTEditor::Open);
    Writer.WriteByte((uint8_t)UIMapView::Open);
    Writer.WriteByte((uint8_t)UIConsole::Open);
    Writer.WriteByte((uint8_t)UIActorTool::Open);

    uint32_t Jumpback = Writer.GetPosition();
    uint16_t ColorSize = 0;
    Writer.Seek(2, BinaryVectorWriter::Position::Current);
    for (auto& [Key, Val] : BfresLibrary::Models) {
        if (!Val.IsDefaultModel())
            continue;
        if (!TextureToGoLibrary::Textures.count(Key))
            continue;

        Writer.WriteByte(Key.length());
        Writer.WriteBytes(Key.c_str());
        Writer.WriteInteger((uint8_t)TextureToGoLibrary::Textures[Key].GetPixels()[0], sizeof(uint8_t));
        Writer.WriteInteger((uint8_t)TextureToGoLibrary::Textures[Key].GetPixels()[1], sizeof(uint8_t));
        Writer.WriteInteger((uint8_t)TextureToGoLibrary::Textures[Key].GetPixels()[2], sizeof(uint8_t));
        Writer.WriteInteger((uint8_t)TextureToGoLibrary::Textures[Key].GetPixels()[3], sizeof(uint8_t));
        ColorSize++;
    }

    Writer.Seek(Jumpback, BinaryVectorWriter::Position::Begin);
    Writer.WriteInteger(ColorSize, sizeof(ColorSize));

    std::ofstream File(Editor::GetWorkingDirFile("Preferences.eprf"), std::ios::binary);
    std::copy(Writer.GetData().cbegin(), Writer.GetData().cend(),
        std::ostream_iterator<unsigned char>(File));
    File.close();

    Logger::Info("Pref", "Pref save");
    FrameCount = 0;
}

void PreferencesConfig::UpdateWindowInformation()
{
    WindowInformation.AINBEditorOpen = UIAINBEditor::Open;
    WindowInformation.ToolsOpen = UITools::Open;
    WindowInformation.PropertiesOpen = UIProperties::Open;
    WindowInformation.OutlinerOpen = UIOutliner::Open;
    WindowInformation.MSBTEditorOpen = UIMSBTEditor::Open;
    WindowInformation.MapViewOpen = UIMapView::Open;
    WindowInformation.ConsoleOpen = UIConsole::Open;
    WindowInformation.ActorToolOpen = UIActorTool::Open;
}

bool PreferencesConfig::WindowInformationChanged()
{
    return WindowInformation.AINBEditorOpen != UIAINBEditor::Open || WindowInformation.ToolsOpen != UITools::Open || WindowInformation.PropertiesOpen != UIProperties::Open || WindowInformation.OutlinerOpen != UIOutliner::Open || WindowInformation.MSBTEditorOpen != UIMSBTEditor::Open || WindowInformation.MapViewOpen != UIMapView::Open || WindowInformation.ConsoleOpen != UIConsole::Open || WindowInformation.ActorToolOpen != UIActorTool::Open;
}

void PreferencesConfig::Frame()
{
    FrameCount++;

    if (FrameCount > 1800) // Checking every 1800 frames (60 FPS = 30 seconds)
    {
        if (WindowInformationChanged()) {
            UpdateWindowInformation();
            Save();
        }

        FrameCount = 0;
    }
}