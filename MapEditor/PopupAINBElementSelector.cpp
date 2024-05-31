#include "PopupAINBElementSelector.h"

#include "UIMapView.h"
#include "imgui.h"
#include "imgui_stdlib.h"

bool PopupAINBElementSelector::IsOpen = false;
std::string PopupAINBElementSelector::Key = "";
std::string PopupAINBElementSelector::Value = "";
AINBFile::Node* PopupAINBElementSelector::Node = nullptr;
void (*PopupAINBElementSelector::Func)(AINBFile::Node*, std::string) = nullptr;
std::string PopupAINBElementSelector::PopupTitle = "";
std::string PopupAINBElementSelector::ConfirmButtonText = "";

void PopupAINBElementSelector::Render()
{
    if (IsOpen) {
        UIMapView::RenderSettings.AllowSelectingActor = false;
        ImGui::SetNextWindowSize(ImVec2(432, 66));
        ImGui::OpenPopup(PopupTitle.c_str());
        if (ImGui::BeginPopupModal(PopupTitle.c_str(), NULL, ImGuiWindowFlags_NoResize)) {
            ImGui::InputText(Key.c_str(), &Value);
            if (ImGui::Button(ConfirmButtonText.c_str())) {
                Func(Node, Value);
                IsOpen = false;
                Func = nullptr;
                Key = "";
                Value = "";
                Node = nullptr;
                PopupTitle = "";
                ConfirmButtonText = "";
            }
            ImGui::SameLine();
            if (ImGui::Button("Return")) {
                IsOpen = false;
                Func = nullptr;
                Key = "";
                Node = nullptr;
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

void PopupAINBElementSelector::Open(std::string Title, std::string TextFieldName, std::string ButtonText, AINBFile::Node* NodePtr, void (*Callback)(AINBFile::Node*, std::string))
{
    Func = Callback;
    Key = TextFieldName;
    Value = "";
    IsOpen = true;
    PopupTitle = Title;
    ConfirmButtonText = ButtonText;
    Node = NodePtr;
}