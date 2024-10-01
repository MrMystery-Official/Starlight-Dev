#include "PreferencesConfig.h"

#include "BinaryVectorWriter.h"
#include "BinaryVectorReader.h"
#include <fstream>
#include "Logger.h"
#include "Editor.h"

#include "UIAINBEditor.h"
#include "UITools.h"
#include "UIProperties.h"
#include "UIOutliner.h"
#include "UIMSBTEditor.h"
#include "UIMapView.h"
#include "UIConsole.h"
#include "UIActorTool.h"
#include "UICollisionCreator.h"

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
	if (Version != 0x02) {
		Logger::Error("EditorPreferenceLoader", "Wrong version, expected 2, but got " + std::to_string((int)Version));
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
	UICollisionCreator::Open = (bool)Reader.ReadUInt8();

	Reader.Seek(16, BinaryVectorReader::Position::Current);

	UIMapView::CameraView.Speed = Reader.ReadFloat();
	UIMapView::CameraView.BoostMultiplier = Reader.ReadFloat();
	UIMapView::CameraView.Sensitivity = Reader.ReadFloat();

	UpdateWindowInformation();
}

void PreferencesConfig::Save()
{
	BinaryVectorWriter Writer;

	Writer.WriteBytes("EPRF");
	Writer.WriteByte(0x02); //Version

	Writer.WriteByte((uint8_t)UIAINBEditor::Open);
	Writer.WriteByte((uint8_t)UITools::Open);
	Writer.WriteByte((uint8_t)UIProperties::Open);
	Writer.WriteByte((uint8_t)UIOutliner::Open);
	Writer.WriteByte((uint8_t)UIMSBTEditor::Open);
	Writer.WriteByte((uint8_t)UIMapView::Open);
	Writer.WriteByte((uint8_t)UIConsole::Open);
	Writer.WriteByte((uint8_t)UIActorTool::Open);
	Writer.WriteByte((uint8_t)UICollisionCreator::Open);

	//Reserve
	Writer.WriteInteger(0, sizeof(uint64_t));
	Writer.WriteInteger(0, sizeof(uint64_t));

	Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&UIMapView::CameraView.Speed), sizeof(float));
	Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&UIMapView::CameraView.BoostMultiplier), sizeof(float));
	Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&UIMapView::CameraView.Sensitivity), sizeof(float));

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
	return WindowInformation.AINBEditorOpen != UIAINBEditor::Open ||
		WindowInformation.ToolsOpen != UITools::Open ||
		WindowInformation.PropertiesOpen != UIProperties::Open ||
		WindowInformation.OutlinerOpen != UIOutliner::Open ||
		WindowInformation.MSBTEditorOpen != UIMSBTEditor::Open ||
		WindowInformation.MapViewOpen != UIMapView::Open ||
		WindowInformation.ConsoleOpen != UIConsole::Open ||
		WindowInformation.ActorToolOpen != UIActorTool::Open;
}

void PreferencesConfig::Frame()
{
	FrameCount++;

	if (FrameCount > 1800) //Checking every 1800 frames (60 FPS = 30 seconds)
	{
		if (WindowInformationChanged())
		{
			UpdateWindowInformation();
			Save();
		}

		FrameCount = 0;
	}
}