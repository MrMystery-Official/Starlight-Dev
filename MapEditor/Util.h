#pragma once

//#define ADVANCED_MODE
//#define STARLIGHT_SHARED

#include <string>
#include <vector>

namespace Util
{
	float RadiansToDegrees(float Radians);
	float DegreesToRadians(float Degrees);

	bool FileExists(std::string File);
	void CreateDir(std::string Path);
	void CopyDirectoryRecursively(const std::string& SourcePath, const std::string& DestinationPath);
	long GetFileSize(std::string Path);
	void CopyFileToDest(const std::string& Source, const std::string& Dest);
	void RemoveFile(const std::string& Path);

	bool CompareFloatsWithTolerance(float a, float b, float Tolerance);
	bool IsNumber(std::string Number);

	template<typename T> T StringToNumber(std::string Value);
	std::string StringToLowerCase(std::string Text);
	bool ReplaceString(std::string& str, const std::string& from, const std::string& to);
	std::string WideCharToString(wchar_t WChar);
	std::vector<unsigned char> ReadFile(std::string Path);
	void WriteFile(std::string Path, std::vector<unsigned char> Data);
};