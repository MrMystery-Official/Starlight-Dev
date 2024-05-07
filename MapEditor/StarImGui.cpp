#include "StarImGui.h"

#include "imgui_stdlib.h"
#include "imgui_internal.h"

//const char* label, ImGuiDataType data_type, void* p_data, int components, const void* p_step, const void* p_step_fast, const char* format, ImGuiInputTextFlags flags
//label, ImGuiDataType_Float, v, 3, NULL, NULL, format, flags
bool StarImGui::InputFloat3Colored(const char* Label, float Values[3], ImVec4 ColorA, ImVec4 ColorB, ImVec4 ColorC)
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

        value_changed |= ImGui::InputScalar("", ImGuiDataType_Float, Data, NULL, NULL, "%.3f", 0);
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
    }

    ImGui::EndGroup();
    return value_changed;
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