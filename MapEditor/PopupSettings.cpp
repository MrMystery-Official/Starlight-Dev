#include "PopupSettings.h"

#include "imgui.h"
#include "imgui_stdlib.h"
#include "UIMapView.h"
#include "Editor.h"
#include "Util.h"
#include "Bfres.h"
#include "EditorConfig.h"
#include "PreferencesConfig.h"
#include "PopupGeneralInputString.h"

bool PopupSettings::IsOpen = false;
std::string PopupSettings::ColorActorName = "";

float PopupSettings::SizeX = 660.0f;
float PopupSettings::SizeY = 300.0f;
const float PopupSettings::OriginalSizeX = 660.0f;
const float PopupSettings::OriginalSizeY = 300.0f;

void PopupSettings::UpdateSize(float Scale)
{
	SizeX = OriginalSizeX * Scale;
	SizeY = OriginalSizeY * Scale;
}

void PopupSettings::Render()
{
	if (IsOpen)
	{
		UIMapView::RenderSettings.AllowSelectingActor = false;
		ImGui::SetNextWindowSize(ImVec2(SizeX, SizeY));
		ImGui::OpenPopup("Settings");
		if (ImGui::BeginPopupModal("Settings"))
		{
			//rgba(127, 140, 141,1.0)
			//rgba(52, 73, 94,1.0)
			//rgba(34, 47, 62,1.0)
			//rgba(189, 195, 199,1.0)
			ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(189 / 255.0f, 195 / 255.0f, 199 / 255.0f, 1.0f));
			ImGui::Text("Paths");
			ImGui::Separator();
			//ImGui::NewLine();
			ImGui::Columns(2, "Paths");
			ImGui::Indent();

			ImGui::SetColumnWidth(0, ImGui::CalcTextSize("RomFS Path").x + ImGui::GetStyle().ItemSpacing.x * 2 + ImGui::GetStyle().IndentSpacing);

			ImGui::Text("RomFS Path");
			ImGui::NextColumn();
			ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
			bool RomFSValid = !Editor::RomFSDir.empty();
			if (RomFSValid)
				RomFSValid = Util::FileExists(Editor::RomFSDir + "/Pack/Bootup.Nin_NX_NVN.pack.zs");

			ImGui::PushStyleColor(ImGuiCol_FrameBg, RomFSValid ? ImVec4(0.06f, 0.26f, 0.07f, 1.0f) : ImVec4(0.26f, 0.06f, 0.07f, 1.0f));
			ImGui::InputText("##RomFSPath", &Editor::RomFSDir);
			ImGui::PopStyleColor();
			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::Text("Model Path");
			ImGui::NextColumn();
			ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
			bool ModelValid = !Editor::BfresDir.empty();
			if (ModelValid)
				ModelValid = Util::FileExists(Editor::BfresDir + "/Weapon_Sword_020.Weapon_Sword_020.bfres");

			ImGui::PushStyleColor(ImGuiCol_FrameBg, ModelValid ? ImVec4(0.06f, 0.26f, 0.07f, 1.0f) : ImVec4(0.26f, 0.06f, 0.07f, 1.0f));
			ImGui::InputText("##ModelPath", &Editor::BfresDir);
			ImGui::PopStyleColor();
			ImGui::PopItemWidth();

			ImGui::Unindent();
			ImGui::Columns();

			/*
			ImGui::NewLine();
			ImGui::Text("Colors");
			ImGui::SameLine();
			ImGui::InputTextWithHint("##AddColorName", "String in actor name...", &ColorActorName);
			ImGui::SameLine();
			if (ImGui::Button("Add##Color"))
			{
				if (!ColorActorName.empty())
				{
					BfresLibrary::Models.insert({ ColorActorName, BfresFile::CreateDefaultModel(ColorActorName, 255, 255, 255, 255) });

					for (Actor& SceneActor : ActorMgr::GetActors())
					{
						if (!SceneActor.Model->IsDefaultModel()) continue;
						if (SceneActor.Gyml.find(ColorActorName) != std::string::npos)
						{
							ActorMgr::ActorInfo.erase(SceneActor.Gyml);
							ActorMgr::ActorInfo.insert({ SceneActor.Gyml, { BfresLibrary::GetModel(ColorActorName) } });
							SceneActor.Model = ActorMgr::ActorInfo[SceneActor.Gyml].Model;
						}
					}
					ActorMgr::UpdateModelOrder();
					ColorActorName = "";
				}
			}
			ImGui::Separator();
			ImGui::Columns(3, "Colors");
			ImGui::Indent();

			int ModelIndex = -1;
			for (std::unordered_map<std::string, BfresFile>::iterator Iter = BfresLibrary::Models.begin(); Iter != BfresLibrary::Models.end(); )
			{
				if (!Iter->second.IsDefaultModel())
				{
					++Iter;
					continue;
				}
				if (!TextureToGoLibrary::Textures.count(Iter->first))
				{
					++Iter;
					continue;
				}
				ModelIndex++;
				
				if (ModelIndex > 0)
					ImGui::NextColumn();

				ImGui::Text(Iter->first.c_str());
				ImGui::NextColumn();
				float Data[4];
				Data[0] = TextureToGoLibrary::Textures[Iter->first].GetPixels()[0] / 255.0f;
				Data[1] = TextureToGoLibrary::Textures[Iter->first].GetPixels()[1] / 255.0f;
				Data[2] = TextureToGoLibrary::Textures[Iter->first].GetPixels()[2] / 255.0f;
				Data[3] = TextureToGoLibrary::Textures[Iter->first].GetPixels()[3] / 255.0f;
				if (ImGui::ColorEdit4(("##Color" + Iter->first).c_str(), (float*)&Data, ImGuiColorEditFlags_DisplayRGB))
				{
					TextureToGoLibrary::Textures[Iter->first].GetPixels()[0] = Data[0] * 255.0f;
					TextureToGoLibrary::Textures[Iter->first].GetPixels()[1] = Data[1] * 255.0f;
					TextureToGoLibrary::Textures[Iter->first].GetPixels()[2] = Data[2] * 255.0f;
					TextureToGoLibrary::Textures[Iter->first].GetPixels()[3] = Data[3] * 255.0f;
					GLTextureLibrary::Textures.erase(&TextureToGoLibrary::Textures[Iter->first]);
					Iter->second.CreateOpenGLObjects();
				}
				ImGui::NextColumn();
				if (Iter->first == "Default")
					ImGui::BeginDisabled();
				if (ImGui::Button(("Del##" + Iter->first).c_str(), ImVec2(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize, 0)))
				{
					ImGui::NextColumn();
					BfresFile* ModelPrt = &(Iter->second);
					Iter = BfresLibrary::Models.erase(Iter);

					for (Actor& SceneActor : ActorMgr::GetActors())
					{
						if (SceneActor.Model == ModelPrt)
						{
							ActorMgr::ActorInfo.erase(SceneActor.Gyml);
							ActorMgr::ActorInfo.insert({ SceneActor.Gyml, { BfresLibrary::GetModel(ActorMgr::GetDefaultModelKey(SceneActor)) } });
							SceneActor.Model = ActorMgr::ActorInfo[SceneActor.Gyml].Model;
						}
					}
					ActorMgr::UpdateModelOrder();
					continue;
				}
				if (Iter->first == "Default")
					ImGui::EndDisabled();
				++Iter;
			}

			ImGui::Unindent();
			ImGui::Columns();

			ImGui::Text(std::string("Size: " + std::to_string(ImGui::GetWindowSize().x) + "x" + std::to_string(ImGui::GetWindowSize().y)).c_str());
			*/

			if (ImGui::Button("Close"))
			{
				IsOpen = false;
				ActorMgr::UpdateModelOrder();
				EditorConfig::Save();
				PreferencesConfig::Save();
				Editor::InitializeWithEdtc();
			}

			ImGui::PopStyleColor();
		}
		ImGui::EndPopup();
	}
}

void PopupSettings::Open()
{
	IsOpen = true;
}