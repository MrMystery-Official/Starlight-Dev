#include "PopUpCommon.h"

#include <manager/ActorPackMgr.h>
#include <algorithm>
#include "imgui.h"
#include "imgui_stdlib.h"
#include "imgui_internal.h"

namespace application::rendering::popup
{
    application::rendering::popup::PopUpBuilder PopUpCommon::gLoadActorPopUp;

    void PopUpCommon::Initialize()
    {
        gLoadActorPopUp
            .Title("Load Actor")
            .Width(500.0f)
            .Flag(ImGuiWindowFlags_NoResize)
            .NeedsConfirmation(false)
            .AddDataStorageInstanced<std::string>([](void* Ptr) { *static_cast<std::string*>(Ptr) = ""; })
            .ContentDrawingFunction([](popup::PopUpBuilder& Builder)
                {
                    if (ImGui::BeginTable("ActorTable", 2, ImGuiTableFlags_BordersInnerV))
                    {
                        ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("Name").x);

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("Name");
                        ImGui::TableNextColumn();
                        ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
                        ImGui::InputText("##Name", static_cast<std::string*>(Builder.GetDataStorage(0).mPtr));
                        ImGui::PopItemWidth();

                        ImGui::EndTable();

                        ImGui::PushItemWidth(ImGui::GetWindowWidth() - ImGui::GetStyle().FramePadding.x * 2.0f);
                        if (ImGui::BeginListBox("##ActorPackFilesAddActor"))
                        {
                            std::string ActorPackFilterLower = *static_cast<std::string*>(Builder.GetDataStorage(0).mPtr);
                            std::ranges::transform(ActorPackFilterLower, ActorPackFilterLower.begin(),
                                                   [](const unsigned char c) { return std::tolower(c); });

                            auto DisplayActorFile = [&ActorPackFilterLower, &Builder](const std::string& Name, const std::string& LowerName)
                                {
                                    if (!ActorPackFilterLower.empty())
                                    {
                                        if (LowerName.find(ActorPackFilterLower) == std::string::npos)
                                            return;
                                    }

                                    if (ImGui::Selectable(Name.c_str()))
                                    {
                                        *static_cast<std::string*>(Builder.GetDataStorage(0).mPtr) = Name;
                                    }
                                };

                            for (size_t i = 0; i < application::manager::ActorPackMgr::gActorList.size(); i++)
                            {
                                DisplayActorFile(application::manager::ActorPackMgr::gActorList[i], application::manager::ActorPackMgr::gActorListLowerCase[i]);
                            }

                            ImGui::EndListBox();
                        }
                        ImGui::PopItemWidth();
                    }
                }).Register();
    }
}