#include "UIContentBrowser.h"

#include "imgui.h"
#include "imgui_internal.h"
#include <manager/UIMgr.h>
#include <util/portable-file-dialogs.h>
#include <util/Logger.h>
#include <manager/ProjectMgr.h>
#include <rendering/ainb/UIAINBEditor.h>

namespace application::rendering::general
{
    void UIContentBrowser::InitializeImpl()
    {
        application::manager::ProjectMgr::gProjectChangeCallbacks.push_back([](const std::string& Name)
            {
                if (Name.empty())
                {
                    application::util::Logger::Info("UIContentBrowser", "No project selected");
                    return;
                }

                application::util::Logger::Info("UIContentBrowser", "Switching project to: %s", Name.c_str());
            });
    }

	void UIContentBrowser::DrawImpl()
    {
        if(mFirstFrame)
            ImGui::SetNextWindowDockID(application::manager::UIMgr::gDockBottom);
        
		ImGui::Begin("Content Browser", &mOpen, ImGuiWindowFlags_NoCollapse);
        ImGui::Text("I am an awesome content browser without purpose :D");

        /*
        if(ImGui::Button("Open AINB"))
        {
            auto Dialog = pfd::open_file("Choose AINB file(s) to open", pfd::path::home(),
                                        { "Artificial Intelligence Node Binary (.ainb)", "*.ainb" },
                                        pfd::opt::multiselect);
            
            for (auto const &FileName : Dialog.result())
            {
                application::manager::UIMgr::OpenWindow(std::make_unique<application::rendering::ainb::UIAINBEditor>(FileName));
            }
        }
        */

        ImGui::End();
    }

	void UIContentBrowser::DeleteImpl()
    {

    }

    UIWindowBase::WindowType UIContentBrowser::GetWindowType()
    {
        return UIWindowBase::WindowType::GENERAL_CONTENT_BROWSER;
    }

    bool UIContentBrowser::SupportsProjectChange()
    {
        return true;
    }
}   