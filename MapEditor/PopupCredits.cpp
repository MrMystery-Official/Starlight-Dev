#include "PopupCredits.h"
/*
namespace PopupCredits
{
	extern bool IsOpen;
	extern std::string Message;

	void Render();
	void Open(std::string Msg);
};
*/

#include "imgui.h"
#include "imgui_stdlib.h"
#include "UIMapView.h"

bool PopupCredits::IsOpen = false;

void PopupCredits::Render()
{
	if (IsOpen)
	{
		UIMapView::RenderSettings.AllowSelectingActor = false;
		ImGui::SetNextWindowSize(ImVec2(341, 112));
		ImGui::OpenPopup("Credits");
		//, NULL, ImGuiWindowFlags_NoResize
		if (ImGui::BeginPopupModal("Credits", NULL, ImGuiWindowFlags_NoResize))
		{
			ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - ImGui::CalcTextSize("Credits").x / 2);
			ImGui::Text("Credits");
			ImGui::NewLine();

			ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - ImGui::CalcTextSize("Developer: MrMystery").x / 2);
			ImGui::Text("Developer: MrMystery");
			ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - ImGui::CalcTextSize("Discord tag: mrmystery0778").x / 2);
			ImGui::Text("Discord tag: mrmystery0778");

			if (ImGui::Button("Close", ImVec2(ImGui::GetWindowWidth() - ImGui::GetStyle().FramePadding.x, 0)))
			{
				IsOpen = false;
			}
		}
		ImGui::EndPopup();
	}
}

void PopupCredits::Open()
{
	IsOpen = true;
}