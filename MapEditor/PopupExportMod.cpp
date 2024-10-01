#include "PopupExportMod.h"

#include "imgui.h"
#include "imgui_stdlib.h"
#include "UIMapView.h"
#include "Editor.h"
#include "EditorConfig.h"

bool PopupExportMod::IsOpen = false;
std::string PopupExportMod::Path = "";
void (*PopupExportMod::Func)(std::string) = nullptr;

float PopupExportMod::SizeX = 650.0f;
float PopupExportMod::SizeY = 67.0f;
const float PopupExportMod::OriginalSizeX = 650.0f;
const float PopupExportMod::OriginalSizeY = 67.0f;

void PopupExportMod::UpdateSize(float Scale)
{
	SizeX = OriginalSizeX * Scale;
	SizeY = OriginalSizeY * Scale;
}

void PopupExportMod::Render()
{
	if (IsOpen)
	{
		UIMapView::RenderSettings.AllowSelectingActor = false;
		ImGui::SetNextWindowSize(ImVec2(SizeX, SizeY));
		ImGui::OpenPopup("Export mod");
		if (ImGui::BeginPopupModal("Export mod", NULL, ImGuiWindowFlags_NoResize))
		{
			ImGui::InputText("Directory", &Path);
			if (ImGui::Button("Export"))
			{
				Func(Path);
				Editor::ExportDir = Path;
				EditorConfig::Save();
				IsOpen = false;
				Func = nullptr;
				Path = Editor::ExportDir;
			}
			ImGui::SameLine();
			if (ImGui::Button("Return"))
			{
				IsOpen = false;
				Func = nullptr;
				Path = Editor::ExportDir;
			}
		}
		//ImGui::SameLine();
		//ImGui::Text(std::string("Size: " + std::to_string(ImGui::GetWindowSize().x) + "x" + std::to_string(ImGui::GetWindowSize().y)).c_str());
		ImGui::EndPopup();
	}
}

void PopupExportMod::Open(void (*Callback)(std::string))
{
	Func = Callback;
	IsOpen = true;
	Path = Editor::ExportDir;
}