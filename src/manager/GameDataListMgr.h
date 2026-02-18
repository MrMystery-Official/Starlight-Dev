#pragma once

#include <cstdint>
#include <file/game/byml/BymlFile.h>
#include <game/Scene.h>

namespace application::manager
{
	namespace GameDataListMgr
	{
		extern application::file::game::byml::BymlFile gGameDataList;
		extern std::unordered_map<uint64_t, application::file::game::byml::BymlFile::Node> gBoolU64HashNodes;

		void Initialize();
		void RebuildAndSave(application::game::Scene& Scene);
		void LoadActorGameData(application::game::Scene& Scene);
	}
}