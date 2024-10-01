#pragma once

#include <string>
#include "AINBNodeDefMgr.h"

namespace PopupAddAINBNode
{
	extern bool IsOpen;
	extern bool ShowUnallowedNodes;
	extern std::string Name;
	extern void (*Func)(std::string);
	extern AINBNodeDefMgr::NodeDef::CategoryEnum AINBCategory;
	extern float SizeX;
	extern float SizeY;
	extern const float OriginalSizeX;
	extern const float OriginalSizeY;

	void UpdateSize(float Scale);

	void Render();
	void Open(void (*Callback)(std::string));
};