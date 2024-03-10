#include "PopupAddAINBNode.h"

#include "UIMapView.h"
#include "imgui.h"
#include "imgui_stdlib.h"
#include "UIAINBEditor.h"

bool PopupAddAINBNode::IsOpen = false;
bool PopupAddAINBNode::ShowUnallowedNodes = true;
std::string PopupAddAINBNode::Name = "";
void (*PopupAddAINBNode::Func)(std::string) = nullptr;
AINBNodeDefMgr::NodeDef::CategoryEnum PopupAddAINBNode::AINBCategory = AINBNodeDefMgr::NodeDef::CategoryEnum::Logic;

void PopupAddAINBNode::Render()
{
	if (IsOpen)
	{
		UIMapView::RenderSettings.AllowSelectingActor = false;
		ImGui::SetNextWindowSize(ImVec2(680, 221));
		ImGui::OpenPopup("Add AINB node");
		if (ImGui::BeginPopupModal("Add AINB node", NULL, ImGuiWindowFlags_NoResize))
		{
			ImGui::InputTextWithHint("##Name", "Search...", &Name);

			int Selected = 0;

			ImGui::BeginListBox("##ListBox");
			for (AINBNodeDefMgr::NodeDef& Def : AINBNodeDefMgr::NodeDefinitions)
			{
				if (std::find(Def.AllowedAINBCategories.begin(), Def.AllowedAINBCategories.end(), AINBCategory) == Def.AllowedAINBCategories.end() && !ShowUnallowedNodes) continue;
				if (Name.length() > 0 && Util::StringToLowerCase(Def.DisplayName).find(Util::StringToLowerCase(Name)) == std::string::npos) continue;

				if (ImGui::Selectable(Def.DisplayName.c_str()))
				{
					Name = Def.DisplayName;
				}
			}
			ImGui::EndListBox();
			ImGui::BeginDisabled();
			ImGui::Checkbox("Show all nodes", &ShowUnallowedNodes);
			ImGui::EndDisabled();

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