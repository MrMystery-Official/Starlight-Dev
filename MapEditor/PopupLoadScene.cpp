#include "PopupLoadScene.h"

#include "imgui.h"
#include "imgui_stdlib.h"
#include "UIMapView.h"

bool PopupLoadScene::IsOpen = false;
std::string PopupLoadScene::SceneIdentifier = "";
SceneMgr::Type PopupLoadScene::SceneType = SceneMgr::Type::SkyIslands;
void (*PopupLoadScene::Func)(SceneMgr::Type, std::string) = nullptr;

void PopupLoadScene::Render()
{
	if (IsOpen)
	{
		UIMapView::RenderSettings.AllowSelectingActor = false;
		ImGui::SetNextWindowSize(ImVec2(334, 113));
		ImGui::OpenPopup("Load scene");
		if (ImGui::BeginPopupModal("Load scene"))
		{
			const char* TypeDropdownItems[] = { "SkyIslands", "MainField", "MinusField", "LargeDungeon", "SmallDungeon", "NormalStage", "Custom" };
			ImGui::Combo("Type", reinterpret_cast<int*>(&SceneType), TypeDropdownItems, IM_ARRAYSIZE(TypeDropdownItems));
			ImGui::InputText("Identifier", &SceneIdentifier);
			if (ImGui::Button("Load"))
			{
				Func(SceneType, SceneIdentifier);
				IsOpen = false;
				Func = nullptr;
				SceneType = SceneMgr::Type::SkyIslands;
				SceneIdentifier = "";
			}
			ImGui::SameLine();
			if (ImGui::Button("Return"))
			{
				IsOpen = false;
				Func = nullptr;
				SceneType = SceneMgr::Type::SkyIslands;
				SceneIdentifier = "";
			}
		}
		ImGui::SameLine();
		ImGui::Text(std::string("Size: " + std::to_string(ImGui::GetWindowSize().x) + "x" + std::to_string(ImGui::GetWindowSize().y)).c_str());
		ImGui::EndPopup();
	}
}

void PopupLoadScene::Open(void (*Callback)(SceneMgr::Type, std::string))
{
	Func = Callback;
	SceneType = SceneMgr::Type::SkyIslands;
	SceneIdentifier = "";
	IsOpen = true;
}