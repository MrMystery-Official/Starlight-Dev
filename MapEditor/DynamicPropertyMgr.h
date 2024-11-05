#pragma once

#include <string>
#include <unordered_map>
#include <utility>
#include "BinaryVectorReader.h"

namespace DynamicPropertyMgr
{
	struct MultiCaseString
	{
		std::string mString = "";
		std::string mLowerCaseString = "";

		MultiCaseString(std::string Str);
		MultiCaseString() {}

		bool operator==(const MultiCaseString& B);
	};

	extern std::unordered_map<std::string, std::vector<MultiCaseString>> mActorProperties; //Actor Name -> Properties[]
	extern std::unordered_map<std::string, std::pair<int8_t, MultiCaseString>> mProperties; //Key -> {Data Type, Multi case key}

	std::string ReadString(BinaryVectorReader& Reader);
	void Initialize();
	void Generate();
}