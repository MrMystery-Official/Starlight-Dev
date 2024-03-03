#pragma once

#include "SARC.h"
#include "Byml.h"
#include <string>
#include <unordered_map>

namespace UIActorTool
{
	struct ActorPackStruct
	{
		std::string OriginalName = "";
		std::string Name = "";

		std::unordered_map<std::string, BymlFile> Bymls;

		bool Replace = false;
		SarcFile Pack;
	};

	extern bool Open;
	extern ActorPackStruct ActorPack;

	void DrawActorToolWindow();
	void Save(std::string Path);
};