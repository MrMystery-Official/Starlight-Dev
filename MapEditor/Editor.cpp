#include "Editor.h"

#include "EditorConfig.h"
#include "Logger.h"
#include "UIActorTool.h"
#include "Util.h"
#include "ZStdFile.h"
#include <filesystem>
#if defined(__APPLE__)
    #include <mach-o/dyld.h>
#endif

std::string Editor::BaseDir = GetBaseDirectory();
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
        if (FileName.find("ResourceSizeTable.Product.") != 0 || FileName.find(".rsizetable.zs") == std::string::npos) {
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

std::string Editor::GetBaseDirectory()
{
    #if defined(__APPLE__)
        if(std::filesystem::current_path().string().ends_with("/"))
            return std::filesystem::current_path().string().substr(0, std::filesystem::current_path().string().length() - 1);
        
        BaseDir.resize(512);
        uint32_t BufferSize = 0;
        if(_NSGetExecutablePath(BaseDir.data(), &BufferSize)) {
            BaseDir.resize(BufferSize);
            _NSGetExecutablePath(BaseDir.data(), &BufferSize);
            size_t Pos = BaseDir.find_last_of('/');
            BaseDir = BaseDir.substr(0, Pos);
            return BaseDir;
        }
        Logger::Error("Editor", "Could not get base directory");
        return "";
    #else
        return std::filesystem::current_path().string();
    #endif
}

std::string Editor::GetRomFSFile(std::string LocalPath, bool Replaceable)
{
    if (Util::FileExists(Editor::GetWorkingDirFile("Save/" + LocalPath)) && Replaceable) {
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
    return BaseDir + "/WorkingDir/" + File;
}

std::string Editor::GetBfresFile(std::string Name)
{
    if (Util::FileExists(Editor::GetWorkingDirFile("EditorModels/" + Name))) {
        return Editor::GetWorkingDirFile("EditorModels/" + Name);
    }
    return Editor::BfresDir + "/" + Name;
}

std::string Editor::GetAssetFile(std::string File)
{
    return BaseDir + "/Assets/" + File;
}

std::string Editor::GetInternalGameVersion()
{
    return Editor::InternalGameVersion;
}

void Editor::InitializeWithEdtc()
{
    EditorConfig::Load();

    if (Editor::RomFSDir.empty()) {
        Logger::Error("Editor", "RomFS path invalid");
        return;
    }

    Editor::DetectInternalGameVersion();
    ZStdFile::Initialize(Editor::GetRomFSFile("Pack/ZsDic.pack.zs"));
    UIActorTool::UpdateActorList();
    Logger::Info("Editor", "Initialized");
}