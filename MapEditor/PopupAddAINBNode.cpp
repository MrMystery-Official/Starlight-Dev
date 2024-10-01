#include "PopupAddAINBNode.h"

#include "UIMapView.h"
#include "imgui.h"
#include "imgui_stdlib.h"
#include "UIAINBEditor.h"

bool PopupAddAINBNode::IsOpen = false;
bool PopupAddAINBNode::ShowUnallowedNodes = false;
std::string PopupAddAINBNode::Name = "";
void (*PopupAddAINBNode::Func)(std::string) = nullptr;
AINBNodeDefMgr::NodeDef::CategoryEnum PopupAddAINBNode::AINBCategory = AINBNodeDefMgr::NodeDef::CategoryEnum::Logic;

float PopupAddAINBNode::SizeX = 680.0f;
float PopupAddAINBNode::SizeY = 221.0f;
const float PopupAddAINBNode::OriginalSizeX = 680.0f;
const float PopupAddAINBNode::OriginalSizeY = 221.0f;

void PopupAddAINBNode::UpdateSize(float Scale)
{
	SizeX = OriginalSizeX * Scale;
	SizeY = OriginalSizeY * Scale;
}

void PopupAddAINBNode::Render()
{
	if (IsOpen)
	{
		UIMapView::RenderSettings.AllowSelectingActor = false;
		ImGui::SetNextWindowSize(ImVec2(SizeX, SizeY));
		ImGui::OpenPopup("Add AINB node");
		if (ImGui::BeginPopupModal("Add AINB node", NULL, ImGuiWindowFlags_NoResize))
		{
			ImGui::InputTextWithHint("##Name", "Search...", &Name);

			int Selected = 0;

			ImGui::BeginListBox("##ListBox");
			for (AINBNodeDefMgr::NodeDef& Def : AINBNodeDefMgr::NodeDefinitions)
			{
				if (std::find(Def.AllowedAINBCategories.begin(), Def.AllowedAINBCategories.end(), AINBCategory) == Def.AllowedAINBCategories.end() && !ShowUnallowedNodes)
				{
					if (!Def.DisplayName.starts_with("Element_") || AINBCategory == AINBNodeDefMgr::NodeDef::CategoryEnum::Logic)
					{
						continue;
					}
				}
				if (Name.length() > 0 && Util::StringToLowerCase(Def.DisplayName).find(Util::StringToLowerCase(Name)) == std::string::npos) continue;

				if (ImGui::Selectable(Def.DisplayName.c_str()))
				{
					Name = Def.DisplayName;
				}
			}
			ImGui::EndListBox();

			ImGui::Checkbox("Show unallowed nodes (those nodes won't work)", &ShowUnallowedNodes);

			if (ImGui::Button("Add"))
			{
				Func(Name);
				IsOpen = false;
				Func = nullptr;
				Name = "";
			}
			ImGui::SameLine();
			if (ImGui::Button("Return"))
			{
				IsOpen = false;
				Func = nullptr;
				Name = "";
			}
		}
		//ImGui::SameLine();
		//ImGui::Text(std::string("Size: " + std::to_string(ImGui::GetWindowSize().x) + "x" + std::to_string(ImGui::GetWindowSize().y)).c_str());
		ImGui::EndPopup();
	}
}

void PopupAddAINBNode::Open(void (*Callback)(std::string))
{
	IsOpen = true;
	Name = "";
	Func = Callback;
	AINBCategory = AINBNodeDefMgr::NodeDef::CategoryEnum::Logic;
	if (UIAINBEditor::AINB.Header.FileCategory == "AI") AINBCategory = AINBNodeDefMgr::NodeDef::CategoryEnum::AI;
	else if (UIAINBEditor::AINB.Header.FileCategory == "Sequence") AINBCategory = AINBNodeDefMgr::NodeDef::CategoryEnum::Sequence;
}