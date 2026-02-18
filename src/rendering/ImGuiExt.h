#pragma once

#include "imgui.h"
#include <functional>
#include <manager/TextureMgr.h>
#include <unordered_map>
#include <string>
#include <functional>
#include <optional>

namespace ImGuiExt
{
	extern application::gl::Texture* gAddButtonTexture;
	extern application::gl::Texture* gDeleteButtonTexture;
	extern std::unordered_map<std::string, bool> mInputFloatSliderCombos;

	bool Initialize();
	
	IMGUI_API void DrawButtonArray(const char* Names[], uint8_t NameSize, uint8_t MaxColumnCount, int* Index, const std::optional<std::function<void()>>& OnButtonPress = std::nullopt);
	IMGUI_API bool InputFloat3Colored(const char* Label, float Values[3], ImVec4 ColorA, ImVec4 ColorB, ImVec4 ColorC, bool CopyPastePopUp = false);
	IMGUI_API void CheckVec3fInputRightClick(const std::string& Id, float Values[3], bool ConvertAngles = false);
	IMGUI_API bool SelectableTextOffset(const char* label, float offsetX, bool selected = false, ImGuiSelectableFlags flags = 0, const ImVec2& size = ImVec2(0, 0));
	IMGUI_API bool SelectableTextOffsetWithDeleteButton(const char* label, float offsetX, std::function<void()> onDeletePressed, bool selected = false, ImGuiSelectableFlags flags = 0, const ImVec2& size = ImVec2(0, 0));
	IMGUI_API bool InputScalarNWidth(const char* label, ImGuiDataType data_type, void* p_data, int components, float ItemWidth, const void* p_step = NULL, const void* p_step_fast = NULL, const char* format = NULL, ImGuiInputTextFlags flags = 0);
	IMGUI_API bool CollapsingHeaderWithAddButton(const char* label, ImGuiTreeNodeFlags flags, std::function<void()> onButtonPressed);
	IMGUI_API bool ButtonDelete(const char* Label, const ImVec2& Size);
	IMGUI_API bool InputFloatSliderCombo(const char* label, const std::string& id, float* v, float v_min, float v_max, const char* format = "%.3f", ImGuiSliderFlags flags = 0);
	IMGUI_API bool InputIntSliderCombo(const char* label, const std::string& id, int* v, int v_min, int v_max);
}