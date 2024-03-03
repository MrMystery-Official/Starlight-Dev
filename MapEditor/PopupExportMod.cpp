#include "PopupExportMod.h"

#include "imgui.h"
#include "imgui_stdlib.h"
#include "UIMapView.h"
#include "Editor.h"
#include "EditorConfig.h"

bool PopupExportMod::IsOpen = false;
std::string PopupExportMod::Path = "";
void (*PopupExportMod::Func)(std::string) = nullptr;

void PopupExportMod::Render()
{
	if (IsOpen)
	{
		UIMapView::RenderSettings.AllowSelectingActor = false;
		ImGui::SetNextWindowSize(ImVec2(334, 88));
		ImGui::OpenPopup("Export mod");
		if (ImGui::BeginPopupModal("Export mod"))
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
		ImGui::SameLine();
		ImGui::Text(std::string("Size: " + std::to_string(ImGui::GetWindowSize().x) + "x" + std::to_string(ImGui::GetWindowSize().y)).c_str());
		ImGui::EndPopup();
	}
}

void PopupExportMod::Open(void (*Callback)(std::string))
{
	Func = Callback;
	IsOpen = true;
	Path = Editor::ExportDir;
}