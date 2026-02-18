#pragma once

#include <string>
#include <vector>
#include <unordered_map>

namespace application::tool
{
	namespace ResourceSizeTableGenerator
	{
		extern std::unordered_map<std::string, uint32_t> gResourceSizes;
		extern std::vector<std::string> gShaderArchives;

		uint32_t CalcSize(const std::string& FilePath);
		void Generate();
	}
}