#include "StarImGui.h"

#include "imgui_stdlib.h"
#include "imgui_internal.h"
#include "Logger.h"
#include "ActionMgr.h"
#include "Util.h"
#include <sstream>
#include <iostream>
#include <vector>

GLFWwindow* StarImGui::Window = nullptr;
std::unordered_map<std::string, bool> StarImGui::mInputFloatSliderCombos;

//const char* label, ImGuiDataType data_type, void* p_data, int components, const void* p_step, const void* p_step_fast, const char* format, ImGuiInputTextFlags flags
//label, ImGuiDataType_Float, v, 3, NULL, NULL, format, flags
bool StarImGui::InputFloat3Colored(const char* Label, float Values[3], ImVec4 ColorA, ImVec4 ColorB, ImVec4 ColorC, bool CopyPastePopUp)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    void* Data = Values;

    ImGuiContext& g = *GImGui;
    bool value_changed = false;
    ImGui::BeginGroup();
    ImGui::PushID(Label);
    ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
    size_t type_size = sizeof(float);
    for (int i = 0; i < 3; i++)
    {
        ImGui::PushID(i);
        if (i > 0)
            ImGui::SameLine(0, g.Style.ItemInnerSpacing.x);

        switch (i)
        {
        case 0:
        {
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ColorA);
            break;
        }
        case 1:
        {
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ColorB);
            break;
        }
        case 2:
        {
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ColorC);
            break;
        }
        default:
        {
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ColorA);
            break;
        }
        }

        value_changed |= StarImGui::InputScalarAction("", ImGuiDataType_Float, Data, NULL, NULL, "%.3f", 0);
        ImGui::PopID();
        ImGui::PopItemWidth();
        ImGui::PopStyleColor();
        Data = (void*)((char*)Data + type_size);
    }
    ImGui::PopID();

    const char* label_end = ImGui::FindRenderedTextEnd(Label);
    if (Label != label_end)
    {
        ImGui::SameLine(0.0f, g.Style.ItemInnerSpacing.x);
        ImGui::TextEx(Label, label_end);
        if (CopyPastePopUp)
            CheckVec3fInputRightClick(std::string(Label), Values);
    }

    ImGui::EndGroup();
    return value_changed;
}

void StarImGui::CheckVec3fInputRightClick(std::string Id, float Values[3], bool ConvertAngles)
{
    ImGui::OpenPopupOnItemClick(("CopyPasteVec3f" + Id).c_str(), ImGuiPopupFlags_MouseButtonRight);

	if (ImGui::BeginPopup(("CopyPasteVec3f" + Id).c_str()))
	{
        if (!ConvertAngles)
        {
            if (ImGui::MenuItem("Copy"))
            {
                std::string Clipboard = std::to_string(Values[0]) + "," + std::to_string(Values[1]) + "," + std::to_string(Values[2]);
                glfwSetClipboardString(Window, Clipboard.c_str());
            }
        }
        else
        {
            if (ImGui::MenuItem("Copy (to degrees)"))
            {
                std::string Clipboard = std::to_string(Util::RadiansToDegrees(Values[0])) + "," + std::to_string(Util::RadiansToDegrees(Values[1])) + "," + std::to_string(Util::RadiansToDegrees(Values[2]));
                glfwSetClipboardString(Window, Clipboard.c_str());
            }
            if (ImGui::MenuItem("Copy (to radians)"))
            {
                std::string Clipboard = std::to_string(Util::DegreesToRadians(Values[0])) + "," + std::to_string(Util::DegreesToRadians(Values[1])) + "," + std::to_string(Util::DegreesToRadians(Values[2]));
                glfwSetClipboardString(Window, Clipboard.c_str());
            }
        }
		if (ImGui::MenuItem("Paste"))
		{
			std::vector<std::string> Result;
			std::stringstream Data(glfwGetClipboardString(Window));
			std::string Line;
			while (std::getline(Data, Line, ','))
			{
				Result.push_back(Line);
			}

			if (Result.size() != 3)
			{
				Logger::Error("StarImGui", "Can't import vector data from clipboard, wrong length");
				goto EndPopUp;
			}

			Values[0] = Util::StringToNumber<float>(Result[0]);
			Values[1] = Util::StringToNumber<float>(Result[1]);
			Values[2] = Util::StringToNumber<float>(Result[2]);
		}
	EndPopUp:
		ImGui::EndPopup();
	}
}

bool StarImGui::ButtonDelete(const char* Label, const ImVec2& Size)
{
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5, 0.14, 0.14, 1));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.91, 0.12, 0.15, 1));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1, 0, 0, 1));
    bool Changed = ImGui::Button(Label, Size);
    ImGui::PopStyleColor(3);
    return Changed;
}

void StarImGui::RenderPerFrame()
{
}

inline ImGuiInputTextFlags StarImGui::InputScalar_DefaultCharsFilter(ImGuiDataType data_type, const char* format)
{
    if (data_type == ImGuiDataType_Float || data_type == ImGuiDataType_Double)
        return ImGuiInputTextFlags_CharsScientific;
    const char format_last_char = format[0] ? format[strlen(format) - 1] : 0;
    return (format_last_char == 'x' || format_last_char == 'X') ? ImGuiInputTextFlags_CharsHexadecimal : ImGuiInputTextFlags_CharsDecimal;
}

bool StarImGui::DataTypeApplyFromText(const char* buf, ImGuiDataType data_type, void* p_data, const char* format)
{
    while (ImCharIsBlankA(*buf))
        buf++;
    if (!buf[0])
        return false;

    // Copy the value in an opaque buffer so we can compare at the end of the function if it changed at all.
    const ImGuiDataTypeInfo* type_info = ImGui::DataTypeGetInfo(data_type);
    ImGuiDataTypeTempStorage data_backup;
    memcpy(&data_backup, p_data, type_info->Size);

    // Sanitize format
    // - For float/double we have to ignore format with precision (e.g. "%.2f") because sscanf doesn't take them in, so force them into %f and %lf
    // - In theory could treat empty format as using default, but this would only cover rare/bizarre case of using InputScalar() + integer + format string without %.
    char format_sanitized[32];
    if (data_type == ImGuiDataType_Float || data_type == ImGuiDataType_Double)
        format = type_info->ScanFmt;
    else
        format = ImParseFormatSanitizeForScanning(format, format_sanitized, IM_ARRAYSIZE(format_sanitized));

    // Small types need a 32-bit buffer to receive the result from scanf()
    int v32 = 0;
    if (sscanf(buf, format, type_info->Size >= 4 ? p_data : &v32) < 1)
        return false;

    if (type_info->Size < 4)
    {
        if (data_type == ImGuiDataType_S8)
            *(ImS8*)p_data = (ImS8)ImClamp(v32, (int)-128, (int)127);
        else if (data_type == ImGuiDataType_U8)
            *(ImU8*)p_data = (ImU8)ImClamp(v32, (int)0, (int)0xFF);
        else if (data_type == ImGuiDataType_S16)
            *(ImS16*)p_data = (ImS16)ImClamp(v32, (int)-32768, (int)32767);
        else if (data_type == ImGuiDataType_U16)
            *(ImU16*)p_data = (ImU16)ImClamp(v32, (int)0, (int)0xFFFF);
        else
            IM_ASSERT(0);
    }

    bool value_changed = memcmp(&data_backup, p_data, type_info->Size) != 0;

    if (value_changed)
    {
        ActionMgr::AddAction(p_data, data_type, &data_backup);
    }

    return value_changed;
}

bool StarImGui::InputScalarAction(const char* label, ImGuiDataType data_type, void* p_data, const void* p_step, const void* p_step_fast, const char* format, ImGuiInputTextFlags flags)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    ImGuiStyle& style = g.Style;

    if (format == NULL)
        format = ImGui::DataTypeGetInfo(data_type)->PrintFmt;

    char buf[64];
    ImGui::DataTypeFormatString(buf, IM_ARRAYSIZE(buf), data_type, p_data, format);

    // Testing ActiveId as a minor optimization as filtering is not needed until active
    if (g.ActiveId == 0 && (flags & (ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsScientific)) == 0)
        flags |= StarImGui::InputScalar_DefaultCharsFilter(data_type, format);
    flags |= ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_NoMarkEdited; // We call MarkItemEdited() ourselves by comparing the actual data rather than the string.

    bool value_changed = false;
    if (p_step == NULL)
    {
        if (ImGui::InputText(label, buf, IM_ARRAYSIZE(buf), flags))
        {
            value_changed = StarImGui::DataTypeApplyFromText(buf, data_type, p_data, format);
        }
    }
    else
    {
        const float button_size = ImGui::GetFrameHeight();

        ImGui::BeginGroup(); // The only purpose of the group here is to allow the caller to query item data e.g. IsItemActive()
        ImGui::PushID(label);
        ImGui::SetNextItemWidth(ImMax(1.0f, ImGui::CalcItemWidth() - (button_size + style.ItemInnerSpacing.x) * 2));
        if (ImGui::InputText("", buf, IM_ARRAYSIZE(buf), flags)) // PushId(label) + "" gives us the expected ID from outside point of view
            value_changed = StarImGui::DataTypeApplyFromText(buf, data_type, p_data, format);
        IMGUI_TEST_ENGINE_ITEM_INFO(g.LastItemData.ID, label, g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Inputable);

        // Step buttons
        const ImVec2 backup_frame_padding = style.FramePadding;
        style.FramePadding.x = style.FramePadding.y;
        ImGuiButtonFlags button_flags = ImGuiButtonFlags_Repeat | ImGuiButtonFlags_DontClosePopups;
        if (flags & ImGuiInputTextFlags_ReadOnly)
            ImGui::BeginDisabled();
        ImGui::SameLine(0, style.ItemInnerSpacing.x);
        if (ImGui::ButtonEx("-", ImVec2(button_size, button_size), button_flags))
        {
            ImGui::DataTypeApplyOp(data_type, '-', p_data, p_data, g.IO.KeyCtrl && p_step_fast ? p_step_fast : p_step);
            value_changed = true;
        }
        ImGui::SameLine(0, style.ItemInnerSpacing.x);
        if (ImGui::ButtonEx("+", ImVec2(button_size, button_size), button_flags))
        {
            ImGui::DataTypeApplyOp(data_type, '+', p_data, p_data, g.IO.KeyCtrl && p_step_fast ? p_step_fast : p_step);
            value_changed = true;
        }
        if (flags & ImGuiInputTextFlags_ReadOnly)
            ImGui::EndDisabled();

        const char* label_end = ImGui::FindRenderedTextEnd(label);
        if (label != label_end)
        {
            ImGui::SameLine(0, style.ItemInnerSpacing.x);
            ImGui::TextEx(label, label_end);
        }
        style.FramePadding = backup_frame_padding;

        ImGui::PopID();
        ImGui::EndGroup();
    }
    if (value_changed)
        ImGui::MarkItemEdited(g.LastItemData.ID);

    return value_changed;
}

bool StarImGui::InputFloatSliderCombo(const char* label, const std::string& id, float* v, float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
{
    bool ValueChanged = false;
    ImGui::PushID(id.c_str());

    if (mInputFloatSliderCombos[id])
    {
        ImGui::SetKeyboardFocusHere();
        if (ImGui::InputFloat(("##Field" + id).c_str(), v, 0.0f, 0.0f, format, ImGuiInputTextFlags_EnterReturnsTrue | flags))
        {
            mInputFloatSliderCombos[id] = false;
            ValueChanged = true;
        }
        if (ImGui::IsItemDeactivated())
        {
            mInputFloatSliderCombos[id] = false;
        }
    }
    else
    {
        ValueChanged |= ImGui::SliderFloat(("##Slider" + std::string(id)).c_str(), v, v_min, v_max, "%.3f");

        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
        {
            mInputFloatSliderCombos[id] = true;
        }
    }

    ImGui::SameLine();
    ImGui::Text("%s", label);
    ImGui::PopID();

    return ValueChanged;
}

bool StarImGui::InputIntSliderCombo(const char* label, const std::string& id, int* v, int v_min, int v_max)
{
    bool ValueChanged = false;
    ImGui::PushID(id.c_str());

    if (mInputFloatSliderCombos[id])
    {
        ImGui::SetKeyboardFocusHere();
        if (ImGui::InputInt(("##Field" + id).c_str(), v, 1, 100, ImGuiInputTextFlags_EnterReturnsTrue))
        {
            mInputFloatSliderCombos[id] = false;
            ValueChanged = true;
        }
        if (ImGui::IsItemDeactivated())
        {
            mInputFloatSliderCombos[id] = false;
        }
    }
    else
    {
        ValueChanged |= ImGui::SliderInt(("##Slider" + std::string(id)).c_str(), v, v_min, v_max, "%.3f");

        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
        {
            mInputFloatSliderCombos[id] = true;
        }
    }

    ImGui::SameLine();
    ImGui::Text("%s", label);
    ImGui::PopID();

    return ValueChanged;
}