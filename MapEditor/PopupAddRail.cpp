#include "PopupAddRail.h"

#include "imgui.h"
#include "imgui_stdlib.h"
#include "UIMapView.h"

bool PopupAddRail::IsOpen = false;
uint64_t PopupAddRail::Dest = 0;
std::string PopupAddRail::Gyml = "";
std::string PopupAddRail::Name = "";
void (*PopupAddRail::Func)(uint64_t, std::string, std::string) = nullptr;

float PopupAddRail::SizeX = 334.0f;
float PopupAddRail::SizeY = 113.0f;
const float PopupAddRail::OriginalSizeX = 334.0f;
const float PopupAddRail::OriginalSizeY = 113.0f;

void PopupAddRail::UpdateSize(float Scale)
{
	SizeX = OriginalSizeX * Scale;
	SizeY = OriginalSizeY * Scale;
}

void PopupAddRail::Render()
{
	if (IsOpen)
	{
		UIMapView::RenderSettings.AllowSelectingActor = false;
		ImGui::SetNextWindowSize(ImVec2(SizeX, SizeY));
		ImGui::OpenPopup("Add rail");
		if (ImGui::BeginPopupModal("Add rail", NULL, ImGuiWindowFlags_NoResize))
		{
			ImGui::InputScalar("Destination", ImGuiDataType_::ImGuiDataType_U64, &Dest);
			ImGui::InputText("Gyml", &Gyml);
			ImGui::InputText("Name", &Name);
			if (ImGui::Button("Add"))
			{
				Func(Dest, Gyml, Name);
				IsOpen = false;
				Func = nullptr;
				Name = "";
				Gyml = "";
				Dest = 0;
			}
			ImGui::SameLine();
			if (ImGui::Button("Return"))
			{
				IsOpen = false;
				Func = nullptr;
				Name = "";
				Gyml = "";
				Dest = 0;
			}
		}
		//ImGui::SameLine();
		//ImGui::Text(std::string("Size: " + std::to_string(ImGui::GetWindowSize().x) + "x" + std::to_string(ImGui::GetWindowSize().y)).c_str());
		ImGui::EndPopup();
	}
}

void PopupAddRail::Open(void (*Callback)(uint64_t, std::string, std::string))
{
	Func = Callback;
	Name = "";
	Gyml = "";
	Dest = 0;
	IsOpen = true;
}