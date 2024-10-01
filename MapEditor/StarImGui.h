#pragma once

#include "imgui.h"
#include <GLFW/glfw3.h>
#include <string>
#include <unordered_map>

namespace StarImGui
{
	extern GLFWwindow* Window;
	extern std::unordered_map<std::string, bool> mInputFloatSliderCombos;

	bool InputFloat3Colored(const char* Label, float Values[3], ImVec4 ColorA, ImVec4 ColorB, ImVec4 ColorC, bool CopyPastePopUp = false);
	bool ButtonDelete(const char* Label, const ImVec2& Size = ImVec2(0, 0));
	void CheckVec3fInputRightClick(std::string Id, float Values[3], bool ConvertAngles = false);
	void RenderPerFrame();
	bool DataTypeApplyFromText(const char* buf, ImGuiDataType data_type, void* p_data, const char* format);
	static inline ImGuiInputTextFlags InputScalar_DefaultCharsFilter(ImGuiDataType data_type, const char* format);
	bool InputScalarAction(const char* label, ImGuiDataType data_type, void* p_data, const void* p_step = NULL, const void* p_step_fast = NULL, const char* format = NULL, ImGuiInputTextFlags flags = 0);
	bool InputFloatSliderCombo(const char* label, const std::string& id, float* v, float v_min, float v_max, const char* format = "%.3f", ImGuiSliderFlags flags = 0);
	bool InputIntSliderCombo(const char* label, const std::string& id, int* v, int v_min, int v_max);
};