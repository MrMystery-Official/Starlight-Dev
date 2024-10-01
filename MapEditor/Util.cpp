#include "Util.h"

#include <sstream>
#include <filesystem>
#include <windows.h>
#include <fstream>
#include "Logger.h"
#include <glm/glm.hpp>

float Util::RadiansToDegrees(float Radians)
{
	return glm::degrees(Radians);
}

float Util::DegreesToRadians(float Degrees)
{
	return glm::radians(Degrees);
}

bool Util::FileExists(std::string File)
{
	struct stat buffer;
	return (stat(File.c_str(), &buffer) == 0);
}

void Util::RemoveFile(const std::string& Path)
{
	std::filesystem::remove(Path);
}

void Util::CreateDir(std::string Path)
{
	std::filesystem::create_directories(Path);
}

void Util::CopyFileToDest(const std::string& Source, const std::string& Dest)
{
	std::filesystem::copy_file(Source, Dest);
}

long Util::GetFileSize(std::string Path)
{
	struct stat stat_buf;
	int rc = stat(Path.c_str(), &stat_buf);
	return rc == 0 ? stat_buf.st_size : -1;
}

template<> uint32_t Util::StringToNumber(std::string Value)
{
	uint32_t Number;
	std::istringstream StringStream(Value);
	StringStream >> Number;
	return Number;
}

template<> uint64_t Util::StringToNumber(std::string Value)
{
	uint64_t Number;
	std::istringstream StringStream(Value);
	StringStream >> Number;
	return Number;
}

template<> int32_t Util::StringToNumber(std::string Value)
{
	int32_t Number;
	std::istringstream StringStream(Value);
	StringStream >> Number;
	return Number;
}

template<> int64_t Util::StringToNumber(std::string Value)
{
	int64_t Number;
	std::istringstream StringStream(Value);
	StringStream >> Number;
	return Number;
}

template<> float Util::StringToNumber(std::string Value)
{
	float Number;
	std::istringstream StringStream(Value);
	StringStream >> Number;
	return Number;
}

void Util::CopyDirectoryRecursively(const std::string& SourcePath, const std::string& DestinationPath)
{
	for (const auto& Entry : std::filesystem::directory_iterator(SourcePath))
	{
		if (Entry.is_directory() && !std::filesystem::is_empty(Entry))
		{
			std::filesystem::copy(Entry, DestinationPath + "/" + Entry.path().filename().string(), std::filesystem::copy_options::overwrite_existing | std::filesystem::copy_options::recursive);
		}
	}
}

std::string Util::StringToLowerCase(std::string Text)
{
	std::transform(Text.begin(), Text.end(), Text.begin(), ::tolower);
	return Text;
}

bool Util::ReplaceString(std::string& str, const std::string& from, const std::string& to)
{
	size_t startPos = 0;
	while ((startPos = str.find(from, startPos)) != std::string::npos)
	{
		str.replace(startPos, from.length(), to);
		startPos += to.length(); // In case the replacement string contains the target substring
	}
	return true;
}

std::string Util::WideCharToString(wchar_t WChar)
{
	std::string Result(2, 0);
	WideCharToMultiByte(CP_UTF8, 0, &WChar, 1, &Result[0], 2, NULL, NULL);
	return Result;
}

std::vector<unsigned char> Util::ReadFile(std::string Path)
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
		Logger::Error("Util", "Could not open file " + Path);
	}
}

void Util::WriteFile(std::string Path, std::vector<unsigned char> Data)
{
	std::ofstream File(Path, std::ios::binary);
	std::copy(Data.cbegin(), Data.cend(),
		std::ostream_iterator<unsigned char>(File));
	File.close();
}

bool Util::CompareFloatsWithTolerance(float a, float b, float Tolerance)
{
	return std::fabs(a - b) <= Tolerance;
}

bool Util::IsNumber(std::string Number)
{
	return !Number.empty() && std::find_if(Number.begin(),
		Number.end(), [](unsigned char c) { return !std::isdigit(c); }) == Number.end();
}