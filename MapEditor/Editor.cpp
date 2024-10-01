#include "Editor.h"

#include <filesystem>
#include "Util.h"
#include "EditorConfig.h"
#include "ZStdFile.h"
#include "UIActorTool.h"
#include "Logger.h"
#include "GameDataListMgr.h"
#include "Bfres.h"
#include "GLBfres.h"

std::string Editor::WorkingDir = std::filesystem::current_path().string() + "/WorkingDir";
std::string Editor::RomFSDir = "";
std::string Editor::BfresDir = "";
std::string Editor::ExportDir = "";
BymlFile Editor::StaticActorsByml;
BymlFile Editor::DynamicActorsByml;
std::string Editor::BancDir = "";
std::string Editor::BancPrefix = "";
std::string Editor::Identifier = "";
std::string Editor::InternalGameVersion = "100";
std::string Editor::GameDataListVersion = "100";
std::string Editor::StaticCompoundDirectory = "";
std::string Editor::StaticCompoundPrefix = "";
std::map<float, ImFont*> Editor::Fonts;
float Editor::UIScale = 0.0f;
 
void Editor::DetectInternalGameVersion()
{
	using recursive_directory_iterator = std::filesystem::recursive_directory_iterator;
	for (const auto& dirEntry : recursive_directory_iterator(Editor::GetRomFSFile("System/Resource"))) {
		std::string FileName = dirEntry.path().string().substr(dirEntry.path().string().find_last_of("/\\") + 1);
		if (FileName.find("ResourceSizeTable.Product.") != 0 ||
			FileName.find(".rsizetable.zs") == std::string::npos) {
			continue;
		}

		size_t start = FileName.find("ResourceSizeTable.Product.") + strlen("ResourceSizeTable.Product.");
		size_t end = FileName.find(".", start);

		std::string Version = FileName.substr(start, end - start);
		if (stoi(Editor::InternalGameVersion) <= stoi(Version)) {
			Editor::InternalGameVersion = Version;
		}
	}

	for (const auto& dirEntry : recursive_directory_iterator(Editor::GetRomFSFile("GameData"))) {
		std::string FileName = dirEntry.path().string().substr(dirEntry.path().string().find_last_of("/\\") + 1);
		if (FileName.find("GameDataList.Product.") != 0 ||
			FileName.find(".byml.zs") == std::string::npos) {
			continue;
		}

		size_t start = FileName.find("GameDataList.Product.") + strlen("GameDataList.Product.");
		size_t end = FileName.find(".", start);

		std::string Version = FileName.substr(start, end - start);
		if (stoi(Editor::GameDataListVersion) <= stoi(Version)) {
			Editor::GameDataListVersion = Version;
		}
	}
}

std::string Editor::GetRomFSFile(std::string LocalPath, bool Replaceable)
{
	if (Util::FileExists(Editor::GetWorkingDirFile("Save/" + LocalPath)) && Replaceable)
	{
		return Editor::GetWorkingDirFile("Save/" + LocalPath);
	}

	return Editor::RomFSDir + "/" + LocalPath;
}

bool Editor::RomFSFileExists(std::string LocalPath)
{
	return Util::FileExists(Editor::GetWorkingDirFile("Save/" + LocalPath)) || Util::FileExists(Editor::RomFSDir + "/" + LocalPath);
}

std::string Editor::GetWorkingDirFile(std::string File)
{
	return WorkingDir + "/" + File;
}

std::string Editor::GetBfresFile(std::string Name)
{
	if (Util::FileExists(Editor::GetWorkingDirFile("EditorModels/" + Name + ".bfres")))
	{
		return Editor::GetWorkingDirFile("EditorModels/" + Name + ".bfres");
	}
	if (Util::FileExists(Editor::GetWorkingDirFile("Save/Model/" + Name + ".bfres.mc")))
	{
		return Editor::GetWorkingDirFile("Save/Model/" + Name + ".bfres.mc");
	}
	return Editor::BfresDir + "/" + Name + ".bfres";
}

std::string Editor::GetInternalGameVersion()
{
	return Editor::InternalGameVersion;
}

void Editor::InitializeDefaultModels()
{
	BfresFile DefaultModel("Default", Util::ReadFile("Assets/Models/TexturedCube.bfres"), "Assets/Models");
	BfresFile AreaModel("Area", Util::ReadFile("Assets/Models/TexturedArea.bfres"), "Assets/Models");
	DefaultModel.mDefaultModel = true;
	AreaModel.mDefaultModel = true;

	BfresLibrary::Models.insert({ "Default", DefaultModel });
	BfresLibrary::Models.insert({ "Area", AreaModel });

	BfresFile* DefaultModelPtr = &BfresLibrary::Models["Default"];
	BfresFile* AreaModelPtr = &BfresLibrary::Models["Area"];

	GLBfresLibrary::mModels.insert({ DefaultModelPtr, GLBfres(DefaultModelPtr, GL_NEAREST) });
	GLBfresLibrary::mModels.insert({ AreaModelPtr, GLBfres(AreaModelPtr, GL_NEAREST) });
}

void Editor::InitializeWithEdtc()
{
	EditorConfig::Load();

	if (Editor::RomFSDir.empty())
	{
		Logger::Error("Editor", "RomFS path invalid");
		return;
	}

	Editor::DetectInternalGameVersion();
	ZStdFile::Initialize(Editor::GetRomFSFile("Pack/ZsDic.pack.zs"));
	UIActorTool::UpdateActorList();
	GameDataListMgr::Initialize();
	BfresFile::Initialize();
	InitializeDefaultModels();
	Logger::Info("Editor", "Initialized");
}