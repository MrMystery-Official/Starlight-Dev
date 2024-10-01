#include "PopupGeneralInputPair.h"

#include "imgui.h"
#include "imgui_stdlib.h"
#include "UIMapView.h"

bool PopupGeneralInputPair::IsOpen = false;
std::string PopupGeneralInputPair::Key = "";
std::string PopupGeneralInputPair::Value = "";
std::function<void(std::string, std::string)> PopupGeneralInputPair::Func;
std::string PopupGeneralInputPair::PopupTitle = "";

float PopupGeneralInputPair::SizeX = 334.0f;
float PopupGeneralInputPair::SizeY = 88.0f;
const float PopupGeneralInputPair::OriginalSizeX = 334.0f;
const float PopupGeneralInputPair::OriginalSizeY = 88.0f;

void PopupGeneralInputPair::UpdateSize(float Scale)
{
	SizeX = OriginalSizeX * Scale;
	SizeY = OriginalSizeY * Scale;
}

void PopupGeneralInputPair::Render()
{
	if (IsOpen)
	{
		UIMapView::RenderSettings.AllowSelectingActor = false;
		ImGui::SetNextWindowSize(ImVec2(SizeX, SizeY));
		ImGui::OpenPopup(PopupTitle.c_str());
		if (ImGui::BeginPopupModal(PopupTitle.c_str(), NULL, ImGuiWindowFlags_NoResize))
		{
			ImGui::InputText("Key", &Key);
			ImGui::InputText("Value", &Value);
			if (ImGui::Button("Add"))
			{
				Func(Key, Value);
				IsOpen = false;
				Func = NULL;
				Key = "";
				Value = "";
			}
			ImGui::SameLine();
			if (ImGui::Button("Return"))
			{
				IsOpen = false;
				Func = NULL;
				Key = "";
				Value = "";
			}
		}
		//ImGui::SameLine();
		//ImGui::Text(std::string("Size: " + std::to_string(ImGui::GetWindowSize().x) + "x" + std::to_string(ImGui::GetWindowSize().y)).c_str());
		ImGui::EndPopup();
	}
}

void PopupGeneralInputPair::Open(std::string Title, std::function<void(std::string, std::string)> Callback)
{
	Func = Callback;
	Key = "";
	Value = "";
	IsOpen = true;
	PopupTitle = Title;
}