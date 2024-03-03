#pragma once

#include <stdint.h>
#include <string>

namespace SceneMgr
{
	enum class Type : int
	{
		SkyIslands = 0,
		MainField = 1,
		MinusField = 2,
		LargeDungeon = 3,
		SmallDungeon = 4,
		NormalStage = 5,
		Custom = 6
	};

	extern SceneMgr::Type SceneType;

	void LoadScene(SceneMgr::Type Type, std::string Identifier);
};