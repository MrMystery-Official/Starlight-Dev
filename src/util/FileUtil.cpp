#include <util/FileUtil.h>

#include <filesystem>
#include <fstream>
#include <util/Logger.h>
#include <manager/ProjectMgr.h>

namespace application::util
{
	std::string FileUtil::gCurrentDirectory = std::filesystem::current_path().string();
	//std::string FileUtil::gCurrentDirectory = "/Users/X/Desktop/AINBTool/Build/BfresTool";

	std::string FileUtil::gRomFSPath = "";
	std::string FileUtil::gBfresPath = "";
	bool FileUtil::gPathsValid = false;

	void FileUtil::ValidatePaths()
	{
		bool RomFSValid = !gRomFSPath.empty();
		if (RomFSValid)
			RomFSValid = FileExists(gRomFSPath + "/Pack/Bootup.Nin_NX_NVN.pack.zs");

		bool ModelValid = !gBfresPath.empty();
		if (ModelValid)
			ModelValid = FileExists(gBfresPath + "/Weapon_Sword_020.Weapon_Sword_020.bfres");

		gPathsValid = RomFSValid && ModelValid;
	}

	std::string FileUtil::GetRomFSFilePath(const std::string& LocalPath, bool Replaceable)
	{
		if (Replaceable && application::manager::ProjectMgr::IsAnyProjectSelected())
		{
			const std::string ReplaceablePath = gCurrentDirectory + "/WorkingDir/Projects/" + application::manager::ProjectMgr::gProject + "/" + LocalPath;
			if (FileExists(ReplaceablePath))
				return ReplaceablePath;
		}

		return gRomFSPath + "/" + LocalPath;
	}

	std::string FileUtil::GetBfresFilePath(const std::string& Name)
	{
		const std::string ReplaceablePath = gCurrentDirectory + "/WorkingDir/Projects/" + application::manager::ProjectMgr::gProject + "/Model/" + Name + ".mc";
		if (FileExists(ReplaceablePath))
			return ReplaceablePath;

		return gBfresPath + "/" + Name;
	}

	std::string FileUtil::GetAssetFilePath(const std::string& Path)
	{
		return gCurrentDirectory + "/Assets/" + Path;
	}

	std::string FileUtil::GetWorkingDirFilePath(const std::string& Path)
	{
		return gCurrentDirectory + "/WorkingDir/" + Path;
	}

	std::string FileUtil::GetSaveFilePath(const std::string& Path)
	{
		if (!application::manager::ProjectMgr::IsAnyProjectSelected())
			application::util::Logger::Error("FileUtil", "Tried to get save file path for %s while no project was selected", Path.c_str());

		if(Path.empty())
			return gCurrentDirectory + "/WorkingDir/Projects/" + application::manager::ProjectMgr::gProject;

		return gCurrentDirectory + "/WorkingDir/Projects/" + application::manager::ProjectMgr::gProject + "/" + Path;
	}

	bool FileUtil::FileExists(const std::string& Path)
	{
		return std::filesystem::exists(Path);
	}

	std::vector<unsigned char> FileUtil::ReadFile(const std::string& Path)
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

			return Bytes;
		}
		else
		{
			application::util::Logger::Error("FileUtil", "Could not open file \"%s\"", Path.c_str());
		}

		return std::vector<unsigned char>();
	}

	void FileUtil::WriteFile(const std::string& Path, std::vector<unsigned char> Bytes)
	{
		std::ofstream FileOut(Path, std::ios::binary);
		std::copy(Bytes.cbegin(), Bytes.cend(),
			std::ostream_iterator<unsigned char>(FileOut));
		FileOut.close();
	}

	void FileUtil::CopyDirectoryRecursively(const std::string& SourcePath, const std::string& DestinationPath)
	{
		for (const auto& Entry : std::filesystem::directory_iterator(SourcePath))
		{
			if (Entry.is_directory() && !std::filesystem::is_empty(Entry) && Entry.path().filename().string() != ".git")
			{
				std::filesystem::copy(Entry, DestinationPath + "/" + Entry.path().filename().string(), std::filesystem::copy_options::overwrite_existing | std::filesystem::copy_options::recursive);
			}
		}
	}
}