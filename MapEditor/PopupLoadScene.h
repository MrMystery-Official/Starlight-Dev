#pragma once

#include <string>
#include "SceneMgr.h"

namespace PopupLoadScene
{
	extern bool IsOpen;
	extern SceneMgr::Type SceneType;
	extern std::string SceneIdentifier;
	extern void (*Func)(SceneMgr::Type, std::string);

	void Render();
	void Open(void (*Callback)(SceneMgr::Type, std::string));
};