#include "PopupGeneralConfirm.h"

#include "imgui.h"
#include "imgui_stdlib.h"
#include "UIMapView.h"

bool PopupGeneralConfirm::IsOpen = false;;
std::string PopupGeneralConfirm::Text = "";
void (*PopupGeneralConfirm::Func)() = nullptr;

void PopupGeneralConfirm::Render()
{
	if (IsOpen)
	{
		UIMapView::RenderSettings.AllowSelectingActor = false;
		ImGui::SetNextWindowSize(ImVec2(334, 60));
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