#pragma once

#include <unordered_map>
#include <string>
#include <vector>
#include <game/BancEntity.h>

namespace application::manager
{
	namespace MergedActorMgr
	{
		extern std::unordered_map<std::string, std::vector<application::game::BancEntity>> gMergedActors;

		std::vector<application::game::BancEntity>* GetMergedActor(const std::string& RomFSPath, bool AllowNew = false);
		void WriteMergedActor(const std::string& BancPath);
		bool DoesMergedActorExist(const std::string& BancPath);
	}
}