#include "PopupGeneralConfirm.h"

#include "imgui.h"
#include "imgui_stdlib.h"
#include "UIMapView.h"

bool PopupGeneralConfirm::IsOpen = false;;
std::string PopupGeneralConfirm::Text = "";
void (*PopupGeneralConfirm::Func)() = nullptr;

float PopupGeneralConfirm::SizeX = 334.0f;
float PopupGeneralConfirm::SizeY = 60.0f;
const float PopupGeneralConfirm::OriginalSizeX = 334.0f;
const float PopupGeneralConfirm::OriginalSizeY = 60.0f;

void PopupGeneralConfirm::UpdateSize(float Scale)
{
	SizeX = OriginalSizeX * Scale;
	SizeY = OriginalSizeY * Scale;
}

void PopupGeneralConfirm::Render()
{
	if (IsOpen)
	{
		UIMapView::RenderSettings.AllowSelectingActor = false;
		ImGui::SetNextWindowSize(ImVec2(SizeX, SizeY));
		ImGui::OpenPopup("Confirm");
		if (ImGui::BeginPopupModal("Confirm", NULL, ImGuiWindowFlags_NoResize))
		{
			ImGui::Text(Text.c_str());
			if (ImGui::Button("Yes"))
			{
				ImGui::EndPopup();
				Func();
				IsOpen = false;
				Func = nullptr;
				Text = "";
				return;
			}
			ImGui::SameLine();
			if (ImGui::Button("No"))
			{
				IsOpen = false;
				Func = nullptr;
				Text = "";
			}
		}
		//ImGui::SameLine();
		//ImGui::Text(std::string("Size: " + std::to_string(ImGui::GetWindowSize().x) + "x" + std::to_string(ImGui::GetWindowSize().y)).c_str());
		ImGui::EndPopup();
		return;
	}
}

void PopupGeneralConfirm::Open(std::string Message, void (*Callback)())
{
	Func = Callback;
	IsOpen = true;
	Text = Message;
}