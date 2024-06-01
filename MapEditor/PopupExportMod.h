#pragma once

#include <string>

namespace PopupExportMod {

extern bool IsOpen;
extern std::string Path;
extern void (*Func)(std::string);

void Render();
void Open(void (*Callback)(std::string));

}
