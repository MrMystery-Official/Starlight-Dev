#include "PopupAddActor.h"

#include "imgui.h"
#include "imgui_stdlib.h"
#include "UIMapView.h"
#include "UIActorTool.h"
#include <algorithm>

bool PopupAddActor::IsOpen = false;
std::string PopupAddActor::Gyml = "";
void (*PopupAddActor::Func)(std::string) = nullptr;

float PopupAddActor::SizeX = 450.0f;
float PopupAddActor::SizeY = 195.0f;
const float PopupAddActor::OriginalSizeX = 450.0f;
const float PopupAddActor::OriginalSizeY = 195.0f;

void PopupAddActor::UpdateSize(float Scale)
{
	SizeX = OriginalSizeX * Scale;
	SizeY = OriginalSizeY * Scale;
}

void PopupAddActor::Render()
{
	if (IsOpen)
	{
		UIMapView::RenderSettings.AllowSelectingActor = false;
		ImGui::SetNextWindowSize(ImVec2(SizeX, SizeY));
		ImGui::OpenPopup("Add actor");
		if (ImGui::BeginPopupModal("Add actor", NULL, ImGuiWindowFlags_NoResize))
		{
			ImGui::InputText("Gyml", &Gyml);

			ImGui::PushItemWidth(ImGui::GetWindowWidth() - ImGui::GetStyle().FramePadding.x);
			if (ImGui::BeginListBox("##ActorPackFilesAddActor"))
			{
				std::string ActorPackFilterLower = Gyml;
				std::transform(ActorPackFilterLower.begin(), ActorPackFilterLower.end(), ActorPackFilterLower.begin(),
					[](unsigned char c) { return std::tolower(c); });

				auto DisplayActorFile = [&ActorPackFilterLower](const std::string& Name, const std::string& LowerName) {
					if (ActorPackFilterLower.length() > 0)
					{
						if (LowerName.find(ActorPackFilterLower) == std::string::npos)
							return;
					}

					if (ImGui::Selectable(Name.c_str()))
					{
						Func(Name);
						IsOpen = false;
						Func = nullptr;
						Gyml = "";
					}
					};

				for (size_t i = 0; i < UIActorTool::ActorList.size(); i++)
				{
					DisplayActorFile(UIActorTool::ActorList[i], UIActorTool::ActorListLowerCase[i]);
				}

				ImGui::EndListBox();
			}
			ImGui::PopItemWidth();

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