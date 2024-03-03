#include "Editor.h"

#include <filesystem>
#include "Util.h"

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
	if (Util::FileExists(Editor::GetWorkingDirFile("EditorModels/" + Name)))
	{
		return Editor::GetWorkingDirFile("EditorModels/" + Name);
	}
	return Editor::BfresDir + "/" + Name;
}

std::string Editor::GetInternalGameVersion()
{
	return Editor::InternalGameVersion;
}