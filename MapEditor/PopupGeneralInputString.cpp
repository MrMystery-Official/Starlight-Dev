#include "PopupGeneralInputString.h"

#include "UIMapView.h"
#include "imgui.h"
#include "imgui_stdlib.h"

bool PopupGeneralInputString::IsOpen = false;
std::string PopupGeneralInputString::Key = "";
std::string PopupGeneralInputString::Value = "";
void (*PopupGeneralInputString::Func)(std::string) = nullptr;
std::string PopupGeneralInputString::PopupTitle = "";
std::string PopupGeneralInputString::ConfirmButtonText = "";

void PopupGeneralInputString::Render()
{
    if (IsOpen) {
        UIMapView::RenderSettings.AllowSelectingActor = false;
        ImGui::SetNextWindowSize(ImVec2(334, 88));
        ImGui::OpenPopup(PopupTitle.c_str());
        if (ImGui::BeginPopupModal(PopupTitle.c_str())) {
            ImGui::InputText(Key.c_str(), &Value);
            if (ImGui::Button(ConfirmButtonText.c_str())) {
                Func(Value);
                IsOpen = false;
                Func = nullptr;
                Key = "";
                Value = "";
                PopupTitle = "";
                ConfirmButtonText = "";
            }
            ImGui::SameLine();
            if (ImGui::Button("Return")) {
                IsOpen = false;
                Func = nullptr;
                Key = "";
                Value = "";
                PopupTitle = "";
                ConfirmButtonText = "";
            }
        }
        ImGui::SameLine();
        ImGui::Text(std::string("Size: " + std::to_string(ImGui::GetWindowSize().x) + "x" + std::to_string(ImGui::GetWindowSize().y)).c_str());
        ImGui::EndPopup();
    }
}

void PopupGeneralInputString::Open(std::string Title, std::string TextFieldName, std::string ButtonText, void (*Callback)(std::string))
{
    Func = Callback;
    Key = TextFieldName;
    Value = "";
    IsOpen = true;
    PopupTitle = Title;
    ConfirmButtonText = ButtonText;
}