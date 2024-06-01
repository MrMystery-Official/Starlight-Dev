#include "UITools.h"

#include "Byml.h"
#include "Editor.h"
#include "EditorConfig.h"
#include "Exporter.h"
#include "Logger.h"
#include "PopupEditorAINBActorLinks.h"
#include "PopupExportMod.h"
#include "SceneMgr.h"
#include "UIMapView.h"
#include "Util.h"
#include "ZStdFile.h"
#include "imgui.h"

bool UITools::Open = true;

void UITools::DrawToolsWindow()
{
    if (!Open)
        return;

    ImGui::Begin("Tools", &Open);

    if (ImGui::Button("Save", ImVec2(ImGui::GetWindowSize().x * 0.5f - ImGui::GetStyle().ItemSpacing.x, 0))) {
        Exporter::Export(Editor::GetWorkingDirFile("Save"), Exporter::Operation::Save);
    }
    ImGui::SameLine();
    if (ImGui::Button("Export", ImVec2(ImGui::GetWindowSize().x * 0.5f - ImGui::GetStyle().FramePadding.x, 0))) {
        PopupExportMod::Open([](std::string Path) {
            Exporter::Export(Path, Exporter::Operation::Export);
        });
    }

    ImGui::Separator();

    if (UIMapView::Focused) {
        if (ImGui::CollapsingHeader("Render", ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Indent();
            ImGui::Checkbox("Visible actors", &UIMapView::RenderSettings.RenderVisibleActors);
            ImGui::Checkbox("Invisible actors", &UIMapView::RenderSettings.RenderInvisibleActors);
            ImGui::Indent();
            if (!UIMapView::RenderSettings.RenderInvisibleActors)
                ImGui::BeginDisabled();
            ImGui::Checkbox("Areas", &UIMapView::RenderSettings.RenderAreas);
            if (!UIMapView::RenderSettings.RenderInvisibleActors)
                ImGui::EndDisabled();
            ImGui::Unindent();
            ImGui::Checkbox("Far actors", &UIMapView::RenderSettings.RenderFarActors);
            ImGui::Checkbox("NPCs", &UIMapView::RenderSettings.RenderNPCs);
            ImGui::Unindent();
        }
        if (SceneMgr::SceneType == SceneMgr::Type::SmallDungeon) {
            if (ImGui::CollapsingHeader("SmallDungeon", ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Indent();
                if (ImGui::Button("StartPos to WarpIn")) {
                    BymlFile StartPosByml(ZStdFile::Decompress(Editor::GetRomFSFile("Banc/SmallDungeon/StartPos/SmallDungeon.startpos.byml.zs"), ZStdFile::Dictionary::Base).Data);
                    Actor* WarpInActor = ActorMgr::GetActor("DgnObj_Small_Warpin_B_02");
                    if (WarpInActor != nullptr) {
                        StartPosByml.GetNode("OnElevator")->GetChild("Dungeon" + Editor::Identifier)->GetChild("Trans")->GetChild(0)->SetValue<float>(WarpInActor->Translate.GetX());
                        StartPosByml.GetNode("OnElevator")->GetChild("Dungeon" + Editor::Identifier)->GetChild("Trans")->GetChild(1)->SetValue<float>(WarpInActor->Translate.GetY());
                        StartPosByml.GetNode("OnElevator")->GetChild("Dungeon" + Editor::Identifier)->GetChild("Trans")->GetChild(2)->SetValue<float>(WarpInActor->Translate.GetZ());

                        Util::CreateDir(Editor::GetWorkingDirFile("Save/Banc/SmallDungeon/StartPos"));
                        ZStdFile::Compress(StartPosByml.ToBinary(BymlFile::TableGeneration::Auto), ZStdFile::Dictionary::Base).WriteToFile(Editor::GetWorkingDirFile("Save/Banc/SmallDungeon/StartPos/SmallDungeon.startpos.byml.zs"));
                    } else {
                        Logger::Error("Tools", "Could not find DgnObj_Small_Warpin_B_02 actor");
                    }
                }
                ImGui::Unindent();
            }
        }
        if (Editor::Identifier.length() > 0) {
            if (ImGui::Button("AINB actor links", ImVec2(ImGui::GetWindowSize().x - ImGui::GetStyle().FramePadding.x, 0))) {
                PopupEditorAINBActorLinks::Open();
            }
        }
    }

    ImGui::End();
}