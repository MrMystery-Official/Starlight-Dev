#pragma once

// #define ADVANCED_MODE

#include <cstdint>
#include <string>

namespace Util {

float RadiansToDegrees(float Radians);
float DegreesToRadians(float Degrees);

bool FileExists(std::string File);
void CreateDir(std::string Path);
void CopyDirectoryRecursively(const std::string& SourcePath, const std::string& DestinationPath);
long GetFileSize(std::string Path);

template <typename T>
T StringToNumber(std::string Value);
std::string StringToLowerCase(std::string Text);
bool ReplaceString(std::string& str, const std::string& from, const std::string& to);
std::string WideCharToString(wchar_t WChar);
int MultiByteToWideChar(wchar_t* out, const char* str, size_t size);

uint32_t ByteswapU32(uint32_t value);
uint64_t ByteswapU64(uint64_t value);

}
