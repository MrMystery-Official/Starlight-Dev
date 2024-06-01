#pragma once

#include "AINBNodeDefMgr.h"
#include <string>

namespace PopupAddAINBNode {

extern bool IsOpen;
extern bool ShowUnallowedNodes;
extern std::string Name;
extern void (*Func)(std::string);
extern AINBNodeDefMgr::NodeDef::CategoryEnum AINBCategory;

void Render();
void Open(void (*Callback)(std::string));

}
