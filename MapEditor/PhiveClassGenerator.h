#pragma once

#include <string>
#include <vector>
#include <utility>
#include <map>
#include "json.hpp"

using json = nlohmann::json;

namespace PhiveClassGenerator
{
	extern std::map<std::string, std::vector<std::string>> Classes;
	extern std::map<std::string, std::vector<std::string>> ClassDependencies;
	extern uint32_t PointerArrayCount;

	std::string ConvertClassName(std::string Name);
	std::string GetPointerArrayName();
	void GenerateClass(json& Data, std::string Name);
	void SortClassDependencies(std::string Name, std::vector<std::pair<std::string, std::vector<std::string>>>& Sorted);
	void Generate(std::string Path, std::vector<std::string> Goals);
}