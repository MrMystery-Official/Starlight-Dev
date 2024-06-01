#include "PopupGeneralInputPair.h"

#include "UIMapView.h"
#include "imgui.h"
#include "imgui_stdlib.h"

bool PopupGeneralInputPair::IsOpen = false;
std::string PopupGeneralInputPair::Key = "";
std::string PopupGeneralInputPair::Value = "";
void (*PopupGeneralInputPair::Func)(std::string, std::string) = nullptr;
std::string PopupGeneralInputPair::PopupTitle = "";

void PopupGeneralInputPair::Render()
{
    if (IsOpen) {
        UIMapView::RenderSettings.AllowSelectingActor = false;
        ImGui::SetNextWindowSize(ImVec2(334, 88));
        ImGui::OpenPopup(PopupTitle.c_str());
        if (ImGui::BeginPopupModal(PopupTitle.c_str(), NULL, ImGuiWindowFlags_NoResize)) {
            ImGui::InputText("Key", &Key);
            ImGui::InputText("Value", &Value);
            if (ImGui::Button("Add")) {
                Func(Key, Value);
                IsOpen = false;
                Func = nullptr;
                Key = "";
                Value = "";
            }
            ImGui::SameLine();
            if (ImGui::Button("Return")) {
                IsOpen = false;
                Func = nullptr;
                Key = "";
                Value = "";
            }
        }
        // ImGui::SameLine();
        // ImGui::Text(std::string("Size: " + std::to_string(ImGui::GetWindowSize().x) + "x" + std::to_string(ImGui::GetWindowSize().y)).c_str());
        ImGui::EndPopup();
    }
}

void PopupGeneralInputPair::Open(std::string Title, void (*Callback)(std::string, std::string))
{
    Func = Callback;
    Key = "";
    Value = "";
    IsOpen = true;
    PopupTitle = Title;
}