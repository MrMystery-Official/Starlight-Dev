#include "PopupStackActors.h"

#include "UIMapView.h"
#include <imgui.h>

bool PopupStackActors::IsOpen = false;
uint64_t PopupStackActors::SrcActor = 0;
float PopupStackActors::OffsetX = 0;
float PopupStackActors::OffsetY = 0;
float PopupStackActors::OffsetZ = 0;
uint16_t PopupStackActors::Count = 0;
void (*PopupStackActors::Func)(uint64_t, float, float, float, uint16_t) = nullptr;

float PopupStackActors::SizeX = 334.0f;
float PopupStackActors::SizeY = 400.0f;
const float PopupStackActors::OriginalSizeX = 334.0f;
const float PopupStackActors::OriginalSizeY = 400.0f;

void PopupStackActors::UpdateSize(float Scale)
{
	SizeX = OriginalSizeX * Scale;
	SizeY = OriginalSizeY * Scale;
}

void PopupStackActors::Render()
{
	if (IsOpen)
	{
		UIMapView::RenderSettings.AllowSelectingActor = false;
		ImGui::SetNextWindowSize(ImVec2(SizeX, SizeY));
		ImGui::OpenPopup("Stack actors");
		if (ImGui::BeginPopupModal("Stack actors", NULL))
		{
			ImGui::InputScalar("Source", ImGuiDataType_::ImGuiDataType_U64, &SrcActor);
			ImGui::InputScalar("Count", ImGuiDataType_::ImGuiDataType_U16, &Count);
			ImGui::InputScalar("OffX", ImGuiDataType_::ImGuiDataType_Float, &OffsetX);
			ImGui::InputScalar("OffY", ImGuiDataType_::ImGuiDataType_Float, &OffsetY);
			ImGui::InputScalar("OffZ", ImGuiDataType_::ImGuiDataType_Float, &OffsetZ);
			if (ImGui::Button("Add"))
			{
				Func(SrcActor, OffsetX, OffsetY, OffsetZ, Count);
				IsOpen = false;
				Func = nullptr;
				SrcActor = 0;
			}
			ImGui::SameLine();
			if (ImGui::Button("Return"))
			{
				IsOpen = false;
				Func = nullptr;
				SrcActor = 0;
				OffsetX = 0;
				OffsetY = 0;
				OffsetZ = 0;
				Count = 0;
			}
		}
		ImGui::SameLine();
		ImGui::Text(std::string("Size: " + std::to_string(ImGui::GetWindowSize().x) + "x" + std::to_string(ImGui::GetWindowSize().y)).c_str());
		ImGui::EndPopup();
	}
}

void PopupStackActors::Open(void (*Callback)(uint64_t, float, float, float, uint16_t), uint64_t Src)
{
	Func = Callback;
	SrcActor = Src;

	IsOpen = true;
}