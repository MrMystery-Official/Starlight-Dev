#include "PopupAINBElementSelector.h"

#include "UIMapView.h"
#include "imgui.h"
#include "imgui_stdlib.h"

bool PopupAINBElementSelector::IsOpen = false;
std::string PopupAINBElementSelector::Key = "";
std::string PopupAINBElementSelector::Value = "";
void* PopupAINBElementSelector::ThisPtr = nullptr;
void (*PopupAINBElementSelector::Func)(void*, std::string) = nullptr;
std::string PopupAINBElementSelector::PopupTitle = "";
std::string PopupAINBElementSelector::ConfirmButtonText = "";

float PopupAINBElementSelector::SizeX = 432.0f;
float PopupAINBElementSelector::SizeY = 66.0f;
const float PopupAINBElementSelector::OriginalSizeX = 432.0f;
const float PopupAINBElementSelector::OriginalSizeY = 66.0f;

void PopupAINBElementSelector::UpdateSize(float Scale)
{
    SizeX = OriginalSizeX * Scale;
    SizeY = OriginalSizeY * Scale;
}

void PopupAINBElementSelector::Render()
{
    if (IsOpen) {
        UIMapView::RenderSettings.AllowSelectingActor = false;
        ImGui::SetNextWindowSize(ImVec2(SizeX, SizeY));
        ImGui::OpenPopup(PopupTitle.c_str());
        if (ImGui::BeginPopupModal(PopupTitle.c_str(), NULL, ImGuiWindowFlags_NoResize)) {
            ImGui::InputText(Key.c_str(), &Value);
            if (ImGui::Button(ConfirmButtonText.c_str())) {
                Func(ThisPtr, Value);
                IsOpen = false;
                Func = nullptr;
                Key = "";
                Value = "";
                ThisPtr = nullptr;
                PopupTitle = "";
                ConfirmButtonText = "";
            }
            ImGui::SameLine();
            if (ImGui::Button("Return")) {
                IsOpen = false;
                Func = nullptr;
                Key = "";
                ThisPtr = nullptr;
                Value = "";
                PopupTitle = "";
                ConfirmButtonText = "";
            }
        }
        // ImGui::SameLine();
        // ImGui::Text(std::string("Size: " + std::to_string(ImGui::GetWindowSize().x) + "x" + std::to_string(ImGui::GetWindowSize().y)).c_str());
        ImGui::EndPopup();
    }
}

void PopupAINBElementSelector::Open(std::string Title, std::string TextFieldName, std::string ButtonText, void* ThisPtr, void (*Callback)(void*, std::string))
{
    Func = Callback;
    Key = TextFieldName;
    Value = "";
    IsOpen = true;
    PopupTitle = Title;
    ConfirmButtonText = ButtonText;
    PopupAINBElementSelector::ThisPtr = ThisPtr;
}