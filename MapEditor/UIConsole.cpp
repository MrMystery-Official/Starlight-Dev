#include "UIConsole.h"

#include "imgui.h"
#include "imgui_stdlib.h"
#include <vector>

std::vector<std::pair<std::string, UIConsole::MessageType>> Messages;
bool UIConsole::Open = true;

bool ShowInfo = true;
bool ShowWarnings = true;
bool ShowErrors = true;

void UIConsole::DrawConsoleWindow()
{
	if (!Open) return;

	ImGui::Begin("Console", &Open);

	ImGui::Checkbox("Infos", &ShowInfo);
	ImGui::SameLine();
	ImGui::Checkbox("Warnings", &ShowWarnings);
	ImGui::SameLine();
	ImGui::Checkbox("Errors", &ShowErrors);

	ImGui::Separator();

	for (auto& [Message, Type] : Messages)
	{
		switch (Type)
		{
		case UIConsole::MessageType::Info:
			if (ShowInfo) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0, 1.0f, 1.0f, 1.0f));
			else continue;
			break;
		case UIConsole::MessageType::Warning:
			if (ShowWarnings) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0, 1.0f, 0.0f, 1.0f));
			else continue;
			break;
		case UIConsole::MessageType::Error:
			if (ShowErrors) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0, 0.0f, 0.0f, 1.0f));
			else continue;
			break;
		}

		ImGui::Text(Message.c_str());
		ImGui::PopStyleColor();
	}

	ImGui::End();
}

void UIConsole::AddMessage(std::string Message, UIConsole::MessageType Type)
{
	Messages.push_back({ Message, Type });
}