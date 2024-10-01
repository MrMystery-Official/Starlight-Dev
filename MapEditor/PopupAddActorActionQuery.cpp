#include "PopupAddActorActionQuery.h"

#include <algorithm>
#include "EventNodeDefMgr.h"
#include "imgui.h"
#include "imgui_stdlib.h"
#include "UIMapView.h"

bool PopupAddActorActionQuery::IsOpen = false;

float PopupAddActorActionQuery::SizeX = 370.0f;
float PopupAddActorActionQuery::SizeY = 192.0f;
const float PopupAddActorActionQuery::OriginalSizeX = 370.0f;
const float PopupAddActorActionQuery::OriginalSizeY = 192.0f;

bool PopupAddActorActionQuery::mIsAction = false;
std::string PopupAddActorActionQuery::mActorName;
std::string PopupAddActorActionQuery::mActionName;
std::function<void(std::string)> PopupAddActorActionQuery::mCallback;

void PopupAddActorActionQuery::UpdateSize(float Scale)
{
	SizeX = OriginalSizeX * Scale;
	SizeY = OriginalSizeY * Scale;
}

void PopupAddActorActionQuery::Render()
{
	if (IsOpen)
	{
		UIMapView::RenderSettings.AllowSelectingActor = false;
		ImGui::SetNextWindowSize(ImVec2(SizeX, SizeY));
		ImGui::OpenPopup("Add Actor Action/Query");
		//, NULL, ImGuiWindowFlags_NoResize
		if (ImGui::BeginPopupModal("Add Actor Action/Query"))
		{
			ImGui::InputText("Action/Query", &mActionName);

			ImGui::PushItemWidth(ImGui::GetWindowWidth() - ImGui::GetStyle().FramePadding.x);
			if (ImGui::BeginListBox("##EventEditorAddActionQuery"))
			{
				std::string ActorPackFilterLower = mActionName;
				std::transform(ActorPackFilterLower.begin(), ActorPackFilterLower.end(), ActorPackFilterLower.begin(),
					[](unsigned char c) { return std::tolower(c); });

				auto DisplayActorFile = [&ActorPackFilterLower](const std::string& Name) {
					if (ActorPackFilterLower.length() > 0)
					{

						std::string LowerName = Name;
						std::transform(LowerName.begin(), LowerName.end(), LowerName.begin(),
							[](unsigned char c) { return std::tolower(c); });

						if (LowerName.find(ActorPackFilterLower) == std::string::npos)
							return;
					}

					if (ImGui::Selectable(Name.c_str()))
					{
						IsOpen = false;
						mCallback(Name);
						mActorName = "";
						mActionName = "";
					}
					};

				for (size_t i = 0; i < EventNodeDefMgr::mActorNodes[mActorName].size(); i++)
				{
					DisplayActorFile(EventNodeDefMgr::mActorNodes[mActorName][i]);
				}

				ImGui::EndListBox();
			}
			ImGui::PopItemWidth();

			if (ImGui::Button("Add"))
			{
				IsOpen = false;
				mCallback(mActionName);
				mActorName = "";
				mActionName = "";
			}

			ImGui::SameLine();

			if (ImGui::Button("Return"))
			{
				IsOpen = false;
				mActorName = "";
				mActionName = "";
			}

			ImGui::SameLine();
			ImGui::Text(std::string("Size: " + std::to_string(ImGui::GetWindowSize().x / 1.2f) + "x" + std::to_string(ImGui::GetWindowSize().y / 1.2f)).c_str());
		}
		ImGui::EndPopup();
	}
}

void PopupAddActorActionQuery::Open(std::string ActorName, bool IsAction, std::function<void(std::string)> Callback)
{
	IsOpen = true;
	mActorName = ActorName;
	mIsAction = IsAction;
	mCallback = Callback;
}