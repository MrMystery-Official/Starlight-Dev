#include "UIAILinks.h"

#include <game/Scene.h>
#include <rendering/ImGuiExt.h>
#include <util/FileUtil.h>
#include <manager/UIMgr.h>
#include <rendering/ainb/UIAINBEditor.h>
#include "imgui.h"
#include "imgui_stdlib.h"
#include "imgui_internal.h"

namespace application::rendering::map_editor
{
	application::rendering::popup::PopUpBuilder UIAILinks::gAILinksPopUp;
    void* UIAILinks::gScenePtr = nullptr;

	void UIAILinks::Initialize()
	{
        gAILinksPopUp
            .Title("Ai Groups")
            .Flag(ImGuiWindowFlags_NoResize)
            .Width(1280.0f)
            .Height(840.0f)
            .NeedsConfirmation(false)
            .InformationalPopUp()
            .ContentDrawingFunction([](popup::PopUpBuilder& Builder)
                {
                    application::game::Scene* ScenePtr = reinterpret_cast<application::game::Scene*>(gScenePtr);

                    for (auto Iter = ScenePtr->mAiGroups.begin(); Iter != ScenePtr->mAiGroups.end(); )
                    {
                        application::game::Scene::AiGroup& AiGroup = *Iter;
                        uint32_t Index = std::distance(ScenePtr->mAiGroups.begin(), Iter);

                        if (ImGui::CollapsingHeader((AiGroup.mPath + "###AiGroup_" + std::to_string(Index)).c_str()))
                        {
                            ImGui::Indent();
                            if (ImGui::BeginTable(("AiGroupTable_" + std::to_string(Index)).c_str(), 2, ImGuiTableFlags_BordersInnerV))
                            {
                                ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("Type").x);

                                ImGui::TableNextRow();

                                ImGui::TableSetColumnIndex(0);
                                ImGui::Text("Hash");
                                ImGui::TableNextColumn();
                                ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
                                ImGui::InputScalar("##Hash", ImGuiDataType_U64, &AiGroup.mHash);
                                ImGui::PopItemWidth();
                                ImGui::TableNextRow();

                                ImGui::TableSetColumnIndex(0);
                                ImGui::Text("Type");
                                ImGui::TableNextColumn();
                                ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
                                static const char* Types[]{ "Logic", "Meta" };
                                ImGui::Combo("##Type", reinterpret_cast<int*>(&AiGroup.mType), Types, IM_ARRAYSIZE(Types));
                                ImGui::PopItemWidth();
                                ImGui::TableNextRow();

                                ImGui::TableSetColumnIndex(0);
                                ImGui::Text("Path");
                                ImGui::TableNextColumn();
                                ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
                                ImGui::InputText("##Path", &AiGroup.mPath);
                                ImGui::PopItemWidth();

                                if (AiGroup.mInstanceName.has_value())
                                {
                                    ImGui::TableNextColumn();
                                    ImGui::TableSetColumnIndex(0);
                                    ImGui::Text("Instance Name");

                                    ImGui::OpenPopupOnItemClick("RefRootInstanceNamePopUp", ImGuiPopupFlags_MouseButtonRight);

                                    bool Del = false;

                                    if (ImGui::BeginPopup("RefRootInstanceNamePopUp"))
                                    {
                                        Del = ImGui::MenuItem("Delete##DelRootInstanceName");
                                        ImGui::EndPopup();
                                    }

                                    ImGui::TableNextColumn();
                                    ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
                                    ImGui::InputText("##RootInstanceName", &AiGroup.mInstanceName.value());
                                    ImGui::PopItemWidth();

									if (Del) AiGroup.mInstanceName = std::nullopt;
                                }

                                ImGui::EndTable();

                                ImGui::Columns(2);
                                ImGui::AlignTextToFramePadding();
                                if (ImGui::Button(("Open in AINB Editor##" + std::to_string(Index)).c_str(), ImVec2(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize, 0)))
                                {
                                    //Logic/Root/Dungeon001/Dungeon001_6a13.logic.root.ain
                                    std::string LogicPrefix = "";
                                    std::string Path = AiGroup.mPath;
                                    if (Path.starts_with("Logic/"))
                                    {
                                        LogicPrefix = "Logic/";
                                    }
                                    else if (Path.starts_with("AI/"))
                                    {
                                        LogicPrefix = "AI/";
                                    }
                                    Path = Path.substr(Path.find_last_of('/') + 1);
                                    Path = LogicPrefix + Path + "b";
                                    if (std::string AbsolutePath = application::util::FileUtil::GetRomFSFilePath(Path); application::util::FileUtil::FileExists(AbsolutePath))
                                    {
                                        std::unique_ptr<application::rendering::UIWindowBase>& Window = application::manager::UIMgr::OpenWindow(std::make_unique<application::rendering::ainb::UIAINBEditor>());
                                        application::rendering::ainb::UIAINBEditor* EditorPtr = static_cast<application::rendering::ainb::UIAINBEditor*>(Window.get());
                                        EditorPtr->mInitCallback = [AbsolutePath](application::rendering::ainb::UIAINBEditor* Editor)
                                            {
                                                Editor->LoadAINB(AbsolutePath);
                                                Editor->mEnableOpen = false;
                                                Editor->mEnableSaveAs = true;
                                                Editor->mEnableSaveOverwrite = false;
                                                Editor->mEnableSaveInProject = true;
                                            };
                                        Builder.IsOpen() = false;
                                    }
                                    //ImGui::SetWindowFocus("AINB Editor");
                                }

                                ImGui::NextColumn();
                                bool DeleteGroup = false;
                                if (ImGuiExt::ButtonDelete(("Delete##AiGroup" + std::to_string(Index)).c_str(), ImVec2(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize, 0)))
                                {
                                    Iter = ScenePtr->mAiGroups.erase(Iter);
                                    DeleteGroup = true;
                                }
                                ImGui::Columns();

                                if (DeleteGroup)
                                {
                                    continue;
                                }

                                ImGui::Text("References");
                                ImGui::SameLine();
                                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.14, 0.5, 0.14, 1));
                                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.12, 0.91, 0.15, 1));
                                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 1, 0, 1));
                                if (ImGui::Button(("Add##AiGroup_" + std::to_string(Index)).c_str()))
                                {
                                    application::game::Scene::AiGroup::Reference Ref;
                                    Ref.mId = "";
                                    Ref.mPath = "";
                                    Ref.mReference = 0;

                                    AiGroup.mReferences.insert(AiGroup.mReferences.begin(), Ref);
                                }
                                ImGui::PopStyleColor(3);

                                ImGui::Separator();
                                ImGui::Indent();
                                for (auto IterRef = AiGroup.mReferences.begin(); IterRef != AiGroup.mReferences.end(); )
                                {
                                    application::game::Scene::AiGroup::Reference& Ref = *IterRef;
                                    uint32_t IndexRef = std::distance(AiGroup.mReferences.begin(), IterRef);

                                    if (ImGui::BeginTable(("AiGroupReferenceTable_" + std::to_string(IndexRef)).c_str(), 2, ImGuiTableFlags_BordersInnerV))
                                    {
                                        ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("Instance Name").x);

                                        ImGui::TableNextRow();

                                        ImGui::TableSetColumnIndex(0);
                                        ImGui::Text("Id");
                                        ImGui::TableNextColumn();
                                        ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
                                        ImGui::InputText(("##RefId" + std::to_string(IndexRef)).c_str(), &Ref.mId);
                                        ImGui::PopItemWidth();
                                        ImGui::TableNextRow();

                                        ImGui::TableSetColumnIndex(0);
                                        ImGui::Text("Path");
                                        ImGui::TableNextColumn();
                                        ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
                                        ImGui::InputText(("##RefPath" + std::to_string(IndexRef)).c_str(), &Ref.mPath);
                                        ImGui::PopItemWidth();
                                        ImGui::TableNextRow();

                                        ImGui::TableSetColumnIndex(0);
                                        ImGui::Text("Reference");
                                        ImGui::TableNextColumn();
                                        ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
                                        ImGui::InputScalar(("##RefReference" + std::to_string(IndexRef)).c_str(), ImGuiDataType_U64, &Ref.mReference);
                                        ImGui::PopItemWidth();

                                        bool DelInstanceName = false;
                                        bool DelLogic = false;
                                        bool DelMeta = false;

                                        if (Ref.mInstanceName.has_value())
                                        {
                                            ImGui::TableNextRow();
                                            ImGui::TableSetColumnIndex(0);
                                            ImGui::Text("Instance Name");

                                            ImGui::OpenPopupOnItemClick(("RefInstanceNamePopUp_" + std::to_string(IndexRef)).c_str(), ImGuiPopupFlags_MouseButtonRight);

                                            if (ImGui::BeginPopup(("RefInstanceNamePopUp_" + std::to_string(IndexRef)).c_str()))
                                            {
                                                DelInstanceName = ImGui::MenuItem("Delete##DelInstanceName");
                                                ImGui::EndPopup();
                                            }

                                            ImGui::TableNextColumn();
                                            ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
                                            ImGui::InputText(("##RefInstanceName" + std::to_string(IndexRef)).c_str(), &Ref.mInstanceName.value());
                                            ImGui::PopItemWidth();
                                        }

                                        if (Ref.mLogic.has_value())
                                        {
                                            ImGui::TableNextRow();
                                            ImGui::TableSetColumnIndex(0);
                                            ImGui::Text("Logic");

                                            ImGui::OpenPopupOnItemClick(("RefLogicPopUp_" + std::to_string(IndexRef)).c_str(), ImGuiPopupFlags_MouseButtonRight);

                                            if (ImGui::BeginPopup(("RefLogicPopUp_" + std::to_string(IndexRef)).c_str()))
                                            {
                                                DelLogic = ImGui::MenuItem("Delete##DelLogic");
                                                ImGui::EndPopup();
                                            }

                                            ImGui::TableNextColumn();
                                            ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
                                            ImGui::InputText(("##RefLogic" + std::to_string(IndexRef)).c_str(), &Ref.mLogic.value());
                                            ImGui::PopItemWidth();
                                        }

                                        if (Ref.mMeta.has_value())
                                        {
                                            ImGui::TableNextRow();
                                            ImGui::TableSetColumnIndex(0);
                                            ImGui::Text("Meta");

                                            ImGui::OpenPopupOnItemClick(("RefMetaPopUp_" + std::to_string(IndexRef)).c_str(), ImGuiPopupFlags_MouseButtonRight);

                                            if (ImGui::BeginPopup(("RefMetaPopUp_" + std::to_string(IndexRef)).c_str()))
                                            {
                                                DelMeta = ImGui::MenuItem("Delete##DelMeta");
                                                ImGui::EndPopup();
                                            }

                                            ImGui::TableNextColumn();
                                            ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
                                            ImGui::InputText(("##RefMeta" + std::to_string(IndexRef)).c_str(), &Ref.mMeta.value());
                                            ImGui::PopItemWidth();
                                        }

                                        if (DelInstanceName)
                                            Ref.mInstanceName = std::nullopt;

                                        if (DelLogic)
                                            Ref.mLogic = std::nullopt;

                                        if (DelMeta)
                                            Ref.mMeta = std::nullopt;

                                        ImGui::EndTable();
                                    }

                                    ImGui::Columns(3, std::to_string(IndexRef).c_str());
                                    ImGui::AlignTextToFramePadding();
                                    if (Ref.mInstanceName.has_value())
                                    {
                                        ImGui::BeginDisabled();
                                    }
                                    if (ImGui::Button(("Add InstanceName##" + std::to_string(IndexRef)).c_str(), ImVec2(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize, 0)))
                                    {
                                        Ref.mInstanceName = "";
                                    }
                                    else if (Ref.mInstanceName.has_value())
                                    {
                                        ImGui::EndDisabled();
                                    }

                                    ImGui::NextColumn();
                                    if (Ref.mLogic.has_value())
                                    {
                                        ImGui::BeginDisabled();
                                    }
                                    if (ImGui::Button(("Add Logic##" + std::to_string(IndexRef)).c_str(), ImVec2(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize, 0)))
                                    {
                                        Ref.mLogic = "";
                                    }
                                    else if (Ref.mLogic.has_value())
                                    {
                                        ImGui::EndDisabled();
                                    }

                                    ImGui::NextColumn();
                                    bool Delete = false;
                                    if (ImGuiExt::ButtonDelete(("Delete##" + std::to_string(IndexRef)).c_str(), ImVec2(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize, 0)))
                                    {
                                        IterRef = AiGroup.mReferences.erase(IterRef);
                                        Delete = true;
                                    }
                                    ImGui::Columns();

                                    if (IndexRef < AiGroup.mReferences.size() - 1)
                                    {
                                        ImGui::Separator();
                                    }

                                    if (!Delete)
                                        IterRef++;
                                }
                                ImGui::Unindent();
                            }
                            ImGui::Unindent();
                        }

                        Iter++;
                    }

                    ImGui::Separator();
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.14, 0.5, 0.14, 1));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.12, 0.91, 0.15, 1));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 1, 0, 1));
                    if (ImGui::Button("Add"))
                    {
                        application::game::Scene::AiGroup AiGroup;
						AiGroup.mType = application::game::Scene::AiGroup::Type::LOGIC;
                        AiGroup.mPath = "";

                        uint64_t BiggestGroupHash = 0;
                        for (application::game::Scene::AiGroup& Group : ScenePtr->mAiGroups)
                        {
                            BiggestGroupHash = std::max(BiggestGroupHash, Group.mHash);
                        }

                        AiGroup.mHash = BiggestGroupHash + 1;

                        ScenePtr->mAiGroups.push_back(AiGroup);
                    }
                    ImGui::PopStyleColor(3);
                    ImGui::SameLine();
                }).Register();
	}

    void UIAILinks::Open(void* SceneThisPtr)
    {
        gScenePtr = SceneThisPtr;
        gAILinksPopUp.Open([](popup::PopUpBuilder& Builder){});
    }
}