#include "UIAINBEditorNodeSequential.h"

#include <util/Logger.h>

namespace application::rendering::ainb::nodes
{
    UIAINBEditorNodeSequential::UIAINBEditorNodeSequential(int UniqueId, application::file::game::ainb::AINBFile::Node& Node) : UIAINBEditorNodeBase(UniqueId, Node)
    {
        mSeqCount = std::max((int)2, (int)Node.LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink].size());
    }

    void UIAINBEditorNodeSequential::DrawImpl()
    {
        ed::PushStyleVar(ed::StyleVar_NodePadding, ImVec4(8, 8, 8, 8));
        ed::BeginNode(mUniqueId++);

        DrawNodeHeader(mNode->GetName(), mNode);

        ImVec2 CursorPos = ImGui::GetCursorPos();

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
        if(ImGui::Button(("+##" + std::to_string(mNode->NodeIndex)).c_str()))
        {
            mSeqCount++;
        }
        ImGui::PopStyleColor();
        ImGui::SameLine();
        if(mSeqCount <= 2)
            ImGui::BeginDisabled();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.47f, 0.07f, 0.07f, 1.0f));
        if(ImGui::Button(("-##" + std::to_string(mNode->NodeIndex)).c_str()))
        {
            mSeqCount--;
            if (mNode->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink].size() >= mSeqCount)
                mNode->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink].resize(mSeqCount);
        }
        else if(mSeqCount <= 2)
        {
            ImGui::EndDisabled();
        }
        ImGui::PopStyleColor();

        ImGui::SetCursorPos(CursorPos);

        for(uint16_t i = 0; i < mSeqCount; i++)
        {
            DrawOutputFlowParameter("Seq " + std::to_string(i), mNode->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink].size() > i, mSeqCount);
        }

        /*
        for (uint8_t Type = 0; Type < application::file::game::ainb::AINBFile::ValueTypeCount; Type++)
        {
            for (uint8_t Index = 0; Index < mNode->InputParameters[Type].size(); Index++)
            {
                DrawInputParameter(mNode, Type, Index);
            }
        }

        for (uint8_t Type = 0; Type < application::file::game::ainb::AINBFile::ValueTypeCount; Type++)
        {
            for (uint8_t Index = 0; Index < mNode->OutputParameters[Type].size(); Index++)
            {
                DrawOutputParameter(mNode, Type, Index);
            }
        }
        */
        
        DrawInternalParameterSeparator();

        for (uint8_t Type = 0; Type < application::file::game::ainb::AINBFile::ValueTypeCount; Type++)
        {
            for (uint8_t Index = 0; Index < mNode->ImmediateParameters[Type].size(); Index++)
            {
                DrawInternalParameter(Type, Index);
            }
        }

        ed::EndNode();
        ed::PopStyleVar();
    }

    void UIAINBEditorNodeSequential::RenderLinks(std::vector<std::unique_ptr<UIAINBEditorNodeBase>>& Nodes)
    {
        uint32_t CurrentLinkId = mNodeId + 500; //Link start at +500
        for (int i = 0; i < mNode->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink].size(); i++) {
            if (mNode->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink][i].NodeIndex == -1 || mNode->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink][i].NodeIndex >= Nodes.size())
                continue;

            uint32_t StartPinId = mOutputFlowParameters["Seq " + std::to_string(i)];

            ed::Link(CurrentLinkId++, StartPinId, Nodes[mNode->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink][i].NodeIndex]->mNodeId + 1, GetValueTypeColor(6));
            mLinks.insert({ CurrentLinkId - 1, Link {.mObjectPtr = mNode, .mType = LinkType::Flow, .mParameterIndex = (uint16_t)i } });
            Nodes[mNode->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink][i].NodeIndex]->mFlowLinked = true;
        }
    }

    bool UIAINBEditorNodeSequential::HasInternalParameters()
    {
        return true;
    }

    ImColor UIAINBEditorNodeSequential::GetNodeColor()
    {
        return ImColor(255, 128, 128);
    }

    int UIAINBEditorNodeSequential::GetNodeIndex()
    {
        return mNode->NodeIndex;
    }

    //Ensure that all output params are linked
    bool UIAINBEditorNodeSequential::FinalizeNode()
    {
        if(mNode->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink].size() != mSeqCount)
        {
            application::util::Logger::Error("UIAINBEditorNodeSequential", "The Sequential Node with index %u needs %u output flow links", mNode->NodeIndex, mSeqCount);
            return false;
        }

        return true;
    }

    bool UIAINBEditorNodeSequential::HasFlowOutputParameters()
    {
        return true;
    }
}