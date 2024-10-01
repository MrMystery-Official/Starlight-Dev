#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include "BFEVFL.h"
#include "BinaryVectorReader.h"

namespace EventNodeDefMgr
{
	struct NodeDef
	{
		std::string mActorName;
		std::string mEventName;
		bool mIsAction = true;
		std::vector<std::pair<BFEVFLFile::ContainerDataType, std::string>> mParameters;
	};

	extern std::vector<NodeDef> mEventNodes;
	extern std::unordered_map<std::string, std::vector<std::string>> mActorNodes;

	void Initialize();
	std::string ReadString(BinaryVectorReader& Reader);
	void DecodeEvent(BFEVFLFile& Event);
	void Generate();
}