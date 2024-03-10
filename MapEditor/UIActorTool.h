#pragma once

#include "SARC.h"
#include "Byml.h"
#include <string>
#include <map>
#include "AINB.h"

namespace UIActorTool
{
	struct ActorPackStruct
	{
		std::string OriginalName = "";
		std::string Name = "";

		std::map<std::string, BymlFile> Bymls;
		std::map<std::string, BymlFile> OriginalBymls;
		std::map<std::string, std::vector<unsigned char>> AINBs;
		std::map<std::string, std::vector<unsigned char>> ChangedAINBs;
		std::vector<std::string> DeletedFiles;

		bool Replace = false;
		SarcFile Pack;
	};

	extern bool Open;
	extern std::string ActorPackFilter;
	extern ActorPackStruct ActorPack;

	void DrawActorToolWindow();
	void Save(std::string Path);
};