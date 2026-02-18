#include "UIPlugins.h"

#include "imgui.h"
#include "imgui_internal.h"
#include <manager/UIMgr.h>
#include <util/Logger.h>
#include <manager/PluginMgr.h>
#include <manager/ProjectMgr.h>
#include <rendering/ainb/UIAINBEditor.h>

namespace application::rendering::plugin
{
    void UIPlugins::InitializeImpl()
    {

    }

    void UIPlugins::DrawImpl()
    {
        if (mFirstFrame)
            ImGui::SetNextWindowDockID(application::manager::UIMgr::gDockMain);

        bool Focused = ImGui::Begin(GetWindowTitle().c_str(), &mOpen, /*ImGuiWindowFlags_NoCollapse |*/ ImGuiWindowFlags_MenuBar);

        if (!Focused)
        {
            ImGui::End();
            return;
        }

        const auto& plugins = application::manager::PluginMgr::GetPlugins();

        // Left panel - Plugin list
        ImGui::BeginChild("PluginList", ImVec2(250, 0), true);
        {
            ImGui::Text("Plugins (%zu)", plugins.size());
            ImGui::Separator();

            for (size_t i = 0; i < plugins.size(); ++i)
            {
                const auto& plugin = plugins[i];
                bool isSelected = (mSelectedPluginIndex == static_cast<int>(i));

                if (ImGui::Selectable(plugin->GetName().c_str(), isSelected))
                {
                    mSelectedPluginIndex = static_cast<int>(i);
                }
            }
        }
        ImGui::EndChild();

        ImGui::SameLine();

        // Right panel - Plugin details
        ImGui::BeginChild("PluginDetails", ImVec2(0, 0), true);
        {
            if (mSelectedPluginIndex >= 0 && mSelectedPluginIndex < static_cast<int>(plugins.size()))
            {
                const auto& selectedPlugin = plugins[mSelectedPluginIndex];

                // Plugin information section
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.6f, 1.0f));
                ImGui::TextWrapped("%s", selectedPlugin->GetName().c_str());
                ImGui::PopStyleColor();

                ImGui::Separator();

                ImGui::Text("Author:");
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", selectedPlugin->GetAuthor().c_str());

                ImGui::Text("Version:");
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%u", selectedPlugin->GetVersion());

                ImGui::Spacing();
                ImGui::Text("Description:");
                ImGui::TextWrapped("%s", selectedPlugin->GetDescription().c_str());

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // Plugin options section
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 1.0f, 0.8f, 1.0f));
                ImGui::Text("Plugin Options");
                ImGui::PopStyleColor();
                ImGui::Separator();

                selectedPlugin->DrawOptions();
            }
            else
            {
                // No plugin selected
                ImGui::TextDisabled("No plugin selected");
                ImGui::Spacing();
                ImGui::TextWrapped("Select a plugin from the list on the left to view its details and options.");
            }
        }
        ImGui::EndChild();

        ImGui::End();
    }

    void UIPlugins::DeleteImpl()
    {

    }

    UIWindowBase::WindowType UIPlugins::GetWindowType()
    {
        return UIWindowBase::WindowType::EDITOR_PLUGINS;
    }

    bool UIPlugins::SupportsProjectChange()
    {
        return true;
    }

    std::string UIPlugins::GetWindowTitle() const
    {
        if (mSameWindowTypeCount > 0)
        {
            return "Plugins (" + std::to_string(mSameWindowTypeCount) + ")###" + std::to_string(mWindowId);
        }

        return "Plugins###" + std::to_string(mWindowId);
    }
}