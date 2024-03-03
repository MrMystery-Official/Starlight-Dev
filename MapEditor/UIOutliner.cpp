#include "UIOutliner.h"

#include "imgui.h"
#include "imgui_stdlib.h"
#include "ActorMgr.h"
#include "PopupAddActor.h"
#include "Logger.h"
#include "UIMapView.h"
#include "ImGuizmo.h"

Camera* pCamera = nullptr;

std::string FilterText = "";
Actor* UIOutliner::SelectedActor = nullptr;
bool UIOutliner::Open = true;

void UIOutliner::Initalize(Camera& Cam)
{
	pCamera = &Cam;
}

void UIOutliner::SelectActor(Actor* pActor)
{
	SelectedActor = pActor;
	
	if(pActor != nullptr) ImGuizmo::RecomposeMatrixFromComponents(pActor->Translate.GetRawData(), pActor->Rotate.GetRawData(), pActor->Scale.GetRawData(), UIMapView::ObjectMatrix);
}

void UIOutliner::DrawOutlinerWindow()
{
	if (!Open) return;

	ImGui::Begin("Outliner", &Open);

	if (ImGui::Button("Add actor", ImVec2(ImGui::GetWindowSize().x * 0.3f, 0)))
	{
		PopupAddActor::Open([](std::string Gyml)
			{
				ActorMgr::AddActor(Gyml, true, false);
				SelectActor(&ActorMgr::GetActors()[ActorMgr::GetActors().size() - 1]);
			});
	}
	ImGui::SameLine();
	ImGui::PushItemWidth(ImGui::GetWindowSize().x * 0.7f - ImGui::GetStyle().FramePadding.x * 3);
	ImGui::InputTextWithHint("##Outliner0", "Search...", &FilterText);
	ImGui::PopItemWidth();

	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGuiCol_WindowBg);

	std::vector<Actor>& ActorNames = ActorMgr::GetActors();

	if (ImGui::BeginListBox("##Outliner1", ImVec2(ImGui::GetWindowSize().x - ImGui::GetStyle().FramePadding.x, ImGui::GetWindowHeight() - 48)))
	{
		for (int i = 0; i < ActorNames.size(); i++)
		{
			if (ActorNames[i].Gyml.find(FilterText) == std::string::npos && FilterText.length() > 0) continue;

			if (ImGui::Selectable((ActorNames[i].Gyml + "##" + std::to_string(i)).c_str(), SelectedActor == &ActorNames[i]))
			{
				SelectActor(&ActorNames[i]);
			}
			/*
			ImGui::Indent();
			for (int j = 0; j < ActorNames[i].MergedActorContent.size(); j++)
			{
				if (ImGui::Selectable((ActorNames[i].MergedActorContent[j].Gyml + "##" + std::to_string(i) + "Child" + std::to_string(j)).c_str(), SelectedActor == &ActorNames[i].MergedActorContent[j]))
				{
					SelectActor(&ActorNames[i].MergedActorContent[j]);
				}
			}
			ImGui::Unindent();
			*/
		}
		ImGui::EndListBox();
	}

	ImGui::PopStyleColor();

	ImGui::End();
}