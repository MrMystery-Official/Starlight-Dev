#include "PopupCreateScene.h"

#include "imgui.h"
#include "imgui_stdlib.h"
#include "UIMapView.h"

bool PopupCreateScene::IsOpen = false;
std::string PopupCreateScene::SceneIdentifier = "";
std::string PopupCreateScene::SceneTemplate = "";
void (*PopupCreateScene::Func)(std::string, std::string) = nullptr;

float PopupCreateScene::SizeX = 520.0f;
float PopupCreateScene::SizeY = 113.0f;
const float PopupCreateScene::OriginalSizeX = 520.0f;
const float PopupCreateScene::OriginalSizeY = 113.0f;

void PopupCreateScene::UpdateSize(float Scale)
{
	SizeX = OriginalSizeX * Scale;
	SizeY = OriginalSizeY * Scale;
}

void PopupCreateScene::Render()
{
	if (IsOpen)
	{
		UIMapView::RenderSettings.AllowSelectingActor = false;
		ImGui::SetNextWindowSize(ImVec2(SizeX, SizeY));
		ImGui::OpenPopup("Load scene");
		if (ImGui::BeginPopupModal("Load scene"))
		{
			ImGui::PushItemWidth(250);
			ImGui::InputText("Template (e.g. 152)", &SceneTemplate);
			ImGui::InputText("Identifier (e.g. 153)", &SceneIdentifier);
			ImGui::PopItemWidth();
			if (ImGui::Button("Load"))
			{
				Func(SceneIdentifier, SceneTemplate);
				IsOpen = false;
				Func = nullptr;
				SceneIdentifier = "";
				SceneTemplate = "";
			}
			ImGui::SameLine();
			if (ImGui::Button("Return"))
			{
				IsOpen = false;
				Func = nullptr;
				SceneIdentifier = "";
				SceneTemplate = "";
			}
		}
		ImGui::EndPopup();
	}
}

void PopupCreateScene::Open(void (*Callback)(std::string, std::string))
{
	Func = Callback;
	SceneIdentifier = "";
	SceneTemplate = "";
	IsOpen = true;
}