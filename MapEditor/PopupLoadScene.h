#pragma once

#include <string>
#include "SceneMgr.h"

namespace PopupLoadScene
{
	extern bool IsOpen;
	extern SceneMgr::Type SceneType;
	extern std::string SceneIdentifier;
	extern void (*Func)(SceneMgr::Type, std::string);
	extern float SizeX;
	extern float SizeY;
	extern const float OriginalSizeX;
	extern const float OriginalSizeY;

	void UpdateSize(float Scale);

	void Render();
	void Open(void (*Callback)(SceneMgr::Type, std::string));
};