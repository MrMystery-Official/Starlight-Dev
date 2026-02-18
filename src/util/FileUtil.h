#pragma once

#include <string>
#include <vector>

namespace application::util
{
	namespace FileUtil
	{
		extern std::string gCurrentDirectory;
		extern std::string gRomFSPath;
		extern std::string gBfresPath;
		extern bool gPathsValid;

		void ValidatePaths();

		std::string GetRomFSFilePath(const std::string& LocalPath, bool Replaceable = true);
		std::string GetBfresFilePath(const std::string& Name);
		std::string GetAssetFilePath(const std::string& Path);
		std::string GetWorkingDirFilePath(const std::string& Path);
		std::string GetSaveFilePath(const std::string& Path = "");

		bool FileExists(const std::string& Path);
		std::vector<unsigned char> ReadFile(const std::string& Path);
		void WriteFile(const std::string& Path, std::vector<unsigned char> Bytes);
		void CopyDirectoryRecursively(const std::string& SourcePath, const std::string& DestinationPath);
	}
}