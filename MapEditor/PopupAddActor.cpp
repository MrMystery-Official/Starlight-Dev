#include "PopupAddActor.h"

#include "imgui.h"
#include "imgui_stdlib.h"
#include "UIMapView.h"

bool PopupAddActor::IsOpen = false;
std::string PopupAddActor::Gyml = "";
void (*PopupAddActor::Func)(std::string) = nullptr;

void PopupAddActor::Render()
{
	if (IsOpen)
	{
		UIMapView::RenderSettings.AllowSelectingActor = false;
		ImGui::SetNextWindowSize(ImVec2(334, 67));
		ImGui::OpenPopup("Add actor");
		if (ImGui::BeginPopupModal("Add actor", NULL, ImGuiWindowFlags_NoResize))
		{
			ImGui::InputText("Gyml", &Gyml);
			if (ImGui::Button("Add"))
			{
				Func(Gyml);
				IsOpen = false;
				Func = nullptr;
				Gyml = "";
			}
			ImGui::SameLine();
			if (ImGui::Button("Return"))
			{
				IsOpen = false;
				Func = nullptr;
				Gyml = "";
			}
		}
		//ImGui::SameLine();
		//ImGui::Text(std::string("Size: " + std::to_string(ImGui::GetWindowSize().x) + "x" + std::to_string(ImGui::GetWindowSize().y)).c_str());
		ImGui::EndPopup();
		return;
	}
}

void PopupAddActor::Open(void (*Callback)(std::string)) 
{
	Func = Callback;
	Gyml = "";
	IsOpen = true;
}