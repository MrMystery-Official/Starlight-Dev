#include "PopupAddLink.h"

#include "imgui.h"
#include "imgui_stdlib.h"
#include "UIMapView.h"

bool PopupAddLink::IsOpen = false;
uint64_t PopupAddLink::Dest = 0;
uint64_t PopupAddLink::Src = 0;
std::string PopupAddLink::Gyml = "";
std::string PopupAddLink::Name = "";
void (*PopupAddLink::Func)(uint64_t, uint64_t, std::string, std::string) = nullptr;

float PopupAddLink::SizeX = 334.0f;
float PopupAddLink::SizeY = 135.0f;
const float PopupAddLink::OriginalSizeX = 334.0f;
const float PopupAddLink::OriginalSizeY = 135.0f;

void PopupAddLink::UpdateSize(float Scale)
{
	SizeX = OriginalSizeX * Scale;
	SizeY = OriginalSizeY * Scale;
}

void PopupAddLink::Render()
{
	if (IsOpen)
	{
		UIMapView::RenderSettings.AllowSelectingActor = false;
		ImGui::SetNextWindowSize(ImVec2(SizeX, SizeY));
		ImGui::OpenPopup("Add link");
		if (ImGui::BeginPopupModal("Add link", NULL, ImGuiWindowFlags_NoResize))
		{
			ImGui::InputScalar("Source", ImGuiDataType_::ImGuiDataType_U64, &Src);
			ImGui::InputScalar("Destination", ImGuiDataType_::ImGuiDataType_U64, &Dest);
			ImGui::InputText("Gyml", &Gyml);
			ImGui::InputText("Name", &Name);
			if (ImGui::Button("Add"))
			{
				Func(Dest, Src, Gyml, Name);
				IsOpen = false;
				Func = nullptr;
				Name = "";
				Gyml = "";
				Dest = 0;
				Src = 0;
			}
			ImGui::SameLine();
			if (ImGui::Button("Return"))
			{
				IsOpen = false;
				Func = nullptr;
				Name = "";
				Gyml = "";
				Dest = 0;
				Src = 0;
			}
		}
		//ImGui::SameLine();
		//ImGui::Text(std::string("Size: " + std::to_string(ImGui::GetWindowSize().x) + "x" + std::to_string(ImGui::GetWindowSize().y)).c_str());
		ImGui::EndPopup();
	}
}

void PopupAddLink::Open(void (*Callback)(uint64_t, uint64_t, std::string, std::string))
{
	Func = Callback;
	Name = "";
	Gyml = "";
	Dest = 0;
	Src = 0;
	IsOpen = true;
}