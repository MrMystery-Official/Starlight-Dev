#pragma once

#include <cstdint>
#include <string>

namespace PopupAddLink {

extern bool IsOpen;
extern uint64_t Dest;
extern uint64_t Src;
extern std::string Gyml;
extern std::string Name;
extern void (*Func)(uint64_t, uint64_t, std::string, std::string);

void Render();
void Open(void (*Callback)(uint64_t, uint64_t, std::string, std::string));

}
