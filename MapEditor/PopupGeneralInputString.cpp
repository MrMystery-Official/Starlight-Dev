#include "PopupGeneralInputString.h"

#include "imgui.h"
#include "imgui_stdlib.h"
#include "UIMapView.h"

bool PopupGeneralInputString::IsOpen = false;
std::string PopupGeneralInputString::Key = "";
std::string PopupGeneralInputString::Value = "";
void (*PopupGeneralInputString::Func)(std::string) = nullptr;
std::string PopupGeneralInputString::PopupTitle = "";
std::string PopupGeneralInputString::ConfirmButtonText = "";

float PopupGeneralInputString::SizeX = 334.0f;
float PopupGeneralInputString::SizeY = 88.0f;
const float PopupGeneralInputString::OriginalSizeX = 334.0f;
const float PopupGeneralInputString::OriginalSizeY = 88.0f;

void PopupGeneralInputString::UpdateSize(float Scale)
{
	SizeX = OriginalSizeX * Scale;
	SizeY = OriginalSizeY * Scale;
}

void PopupGeneralInputString::Render()
{
	if (IsOpen)
	{
		UIMapView::RenderSettings.AllowSelectingActor = false;
		ImGui::SetNextWindowSize(ImVec2(SizeX, SizeY));
		ImGui::OpenPopup(PopupTitle.c_str());
		if (ImGui::BeginPopupModal(PopupTitle.c_str()))
		{
			ImGui::InputText(Key.c_str(), &Value);
			if (ImGui::Button(ConfirmButtonText.c_str()))
			{
				Func(Value);
				IsOpen = false;
				Func = nullptr;
				Key = "";
				Value = "";
				PopupTitle = "";
				ConfirmButtonText = "";
			}
			ImGui::SameLine();
			if (ImGui::Button("Return"))
			{
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