#include "Util.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <sstream>
#include <sys/stat.h>

#define PI 3.14159265358979323846

float Util::RadiansToDegrees(float Radians)
{
    return Radians * (180 / PI);
}

float Util::DegreesToRadians(float Degrees)
{
    return Degrees * (PI / 180);
}

bool Util::FileExists(std::string File)
{
    struct stat buffer;
    return (stat(File.c_str(), &buffer) == 0);
}

void Util::CreateDir(std::string Path)
{
    std::filesystem::create_directories(Path);
}

long Util::GetFileSize(std::string Path)
{
    struct stat stat_buf;
    int rc = stat(Path.c_str(), &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
}

template <>
uint32_t Util::StringToNumber(std::string Value)
{
    uint32_t Number;
    std::istringstream StringStream(Value);
    StringStream >> Number;
    return Number;
}

template <>
uint64_t Util::StringToNumber(std::string Value)
{
    uint64_t Number;
    std::istringstream StringStream(Value);
    StringStream >> Number;
    return Number;
}

template <>
int32_t Util::StringToNumber(std::string Value)
{
    int32_t Number;
    std::istringstream StringStream(Value);
    StringStream >> Number;
    return Number;
}

template <>
int64_t Util::StringToNumber(std::string Value)
{
    int64_t Number;
    std::istringstream StringStream(Value);
    StringStream >> Number;
    return Number;
}

template <>
float Util::StringToNumber(std::string Value)
{
    float Number;
    std::istringstream StringStream(Value);
    StringStream >> Number;
    return Number;
}

void Util::CopyDirectoryRecursively(const std::string& SourcePath, const std::string& DestinationPath)
{
    for (const auto& Entry : std::filesystem::directory_iterator(SourcePath)) {
        if (Entry.is_directory() && !std::filesystem::is_empty(Entry)) {
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
    while ((startPos = str.find(from, startPos)) != std::string::npos) {
        str.replace(startPos, from.length(), to);
        startPos += to.length(); // In case the replacement string contains the target substring
    }
    return true;
}

#if defined(_MSC_VER)
#include <intrin.h>
#include <windows.h>
#endif

std::string Util::WideCharToString(wchar_t WChar)
{
    std::string Result(2, 0);
#ifdef _MSC_VER
    WideCharToMultiByte(CP_UTF8, 0, &WChar, 1, &Result[0], 2, NULL, NULL);
#else
    wctomb(Result.data(), WChar);
#endif
    return Result;
}

uint64_t Util::ByteswapU64(uint64_t value)
{
#if defined(_MSC_VER)
    return _byteswap_ulong(value);
#else
    return __builtin_bswap64(value);
#endif
}

uint32_t Util::ByteswapU32(uint32_t value)
{
#if defined(_MSC_VER)
    return _byteswap_ulong(value);
#else
    return __builtin_bswap32(value);
#endif
}

int Util::MultiByteToWideChar(wchar_t* out, const char* str, size_t size)
{
#if defined(_MSC_VER)
    return ::MultiByteToWideChar(CP_ACP, 0, str, -1, out, int(size));
#else
    return mbtowc(out, str, size);
#endif
}