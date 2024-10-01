#pragma once

#include "SARC.h"
#include "Byml.h"
#include <string>
#include <map>
#include "AINB.h"
#include <vector>

namespace UIActorTool
{
	struct ActorPackStruct
	{
		std::string OriginalName = "";
		std::string Name = "";

		std::map<std::string, BymlFile> Bymls;
		std::map<std::string, BymlFile> OriginalBymls;
		std::map<std::string, std::vector<unsigned char>> AINBs;
		std::vector<std::string> DeletedFiles;

		bool Replace = false;
		SarcFile Pack;
	};

	extern bool Open;
	extern bool Focused;
	extern std::string ActorPackFilter;
	extern ActorPackStruct ActorPack;
	extern std::vector<std::string> ActorList;
	extern std::vector<std::string> ActorListLowerCase;

	void DrawActorToolWindow();
	void UpdateActorList();
	void Save(std::string Path);
};