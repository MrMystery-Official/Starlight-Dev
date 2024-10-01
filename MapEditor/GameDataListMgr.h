#pragma once

#include "Byml.h"
#include <vector>
#include <ctype.h>
#include <unordered_map>

namespace GameDataListMgr
{
	struct GameData
	{
		uint8_t mExtraByte = 0;
		uint64_t mHash = 0;
		int32_t mSaveFileIndex = 0;

		bool mOnBancChange = false;
		bool mOnNewDay = false;
		bool mOnDefaultOptionRevert = false;
		bool mOnBloodMoon = false;
		bool mOnNewGame = false;
		bool mUnknown0 = false;
		bool mUnknown1 = false;
		bool mOnZonaiRespawnDay = false;
		bool mOnMaterialRespawn = false;
		bool mNoResetOnNewGame = false;
	};

	extern BymlFile mGameDataList;
	extern std::unordered_map<uint64_t, BymlFile::Node> mBoolU64HashNodes;

	void Initialize();
	void RebuildAndSave();
	void LoadActorGameData();
}