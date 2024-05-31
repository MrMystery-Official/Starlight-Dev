#pragma once

#include "SceneMgr.h"
#include <string>

namespace PopupLoadScene {

extern bool IsOpen;
extern SceneMgr::Type SceneType;
extern std::string SceneIdentifier;
extern void (*Func)(SceneMgr::Type, std::string);

void Render();
void Open(void (*Callback)(SceneMgr::Type, std::string));

}
