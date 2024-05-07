#pragma once

#include "imgui.h"
#include <string>

namespace StarImGui
{
	bool InputFloat3Colored(const char* Label, float Values[3], ImVec4 ColorA, ImVec4 ColorB, ImVec4 ColorC);
	bool ButtonDelete(const char* Label, const ImVec2& Size);
};