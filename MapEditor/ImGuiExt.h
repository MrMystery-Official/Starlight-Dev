#pragma once

#include "imgui.h"

namespace ImGui {

IMGUI_API bool InputScalarNWidth(const char* label, ImGuiDataType data_type, void* p_data, int components, float ItemWidth, const void* p_step = NULL, const void* p_step_fast = NULL, const char* format = NULL, ImGuiInputTextFlags flags = 0);

} // namespace ImGui
