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

float PopupCredits::SizeX = 341.0f;
float PopupCredits::SizeY = 212.0f;
const float PopupCredits::OriginalSizeX = 341.0f;
const float PopupCredits::OriginalSizeY = 212.0f;

void PopupCredits::UpdateSize(float Scale)
{
	SizeX = OriginalSizeX * Scale;
	SizeY = OriginalSizeY * Scale;
}

void PopupCredits::Render()
{
	if (IsOpen)
	{
		UIMapView::RenderSettings.AllowSelectingActor = false;
		ImGui::SetNextWindowSize(ImVec2(SizeX, SizeY));
		ImGui::OpenPopup("Credits");
		//, NULL, ImGuiWindowFlags_NoResize
		if (ImGui::BeginPopupModal("Credits"))
		{
			ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - ImGui::CalcTextSize("-=Credits=-").x / 2);
			ImGui::Text("-=Credits=-");

			ImGui::NewLine();
			//ImGui::Text(std::string("Size: " + std::to_string(ImGui::GetWindowSize().x) + "x" + std::to_string(ImGui::GetWindowSize().y)).c_str());

			ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - ImGui::CalcTextSize("Developer: MrMystery").x / 2);
			ImGui::Text("Developer: MrMystery");
			ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - ImGui::CalcTextSize("Discord tag: mrmystery0778").x / 2);
			ImGui::Text("Discord tag: mrmystery0778");

			/*
			ImGui::NewLine();
			ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - ImGui::CalcTextSize("Tester: MrLapinou").x / 2);
			ImGui::Text("Tester: MrLapinou");
			ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - ImGui::CalcTextSize("Discord tag: mrlapinou").x / 2);
			ImGui::Text("Discord tag: mrlapinou");
			*/

			ImGui::NewLine();
			ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - ImGui::CalcTextSize("-=Patreon supporters=-").x / 2);
			ImGui::Text("-=Patreon supporters=-");
			ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - ImGui::CalcTextSize("Waikuteru").x / 2);
			ImGui::Text("Waikuteru");
			ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - ImGui::CalcTextSize("Aster").x / 2);
			ImGui::Text("Aster");
			ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - ImGui::CalcTextSize("lethrakh").x / 2);
			ImGui::Text("lethrakh");

			/*
			ImGui::NewLine();
			ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - ImGui::CalcTextSize("Icon creator: Lintenso").x / 2);
			ImGui::Text("Icon creator: Lintenso");
			ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - ImGui::CalcTextSize("Discord tag: lintenso").x / 2);
			ImGui::Text("Discord tag: lintenso");
			*/

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