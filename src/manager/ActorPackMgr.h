#pragma once

#include <string>
#include <unordered_map>
#include <game/ActorPack.h>

namespace application::manager
{
	namespace ActorPackMgr
	{
		extern std::unordered_map<std::string, application::game::ActorPack> gActorPacks;
		extern std::vector<std::string> gActorList;
		extern std::vector<std::string> gActorListLowerCase;

		application::game::ActorPack* GetActorPack(const std::string& Gyml);
		void LoadActorList();
	}
}