#pragma once

#include <string>

namespace PopupAddActor {

extern bool IsOpen;
extern std::string Gyml;
extern void (*Func)(std::string);

void Render();
void Open(void (*Callback)(std::string));

}
