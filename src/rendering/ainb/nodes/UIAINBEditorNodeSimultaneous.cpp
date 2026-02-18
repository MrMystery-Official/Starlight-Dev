#include "UIAINBEditorNodeSimultaneous.h"

#include <util/Logger.h>

namespace application::rendering::ainb::nodes
{
	UIAINBEditorNodeSimultaneous::UIAINBEditorNodeSimultaneous(int UniqueId, application::file::game::ainb::AINBFile::Node& Node) : UIAINBEditorNodeBase(UniqueId, Node)
    {

    }

    void UIAINBEditorNodeSimultaneous::DrawImpl()
    {
        ed::PushStyleVar(ed::StyleVar_NodePadding, ImVec4(8, 8, 8, 8));
        ed::BeginNode(mUniqueId++);

        DrawNodeHeader(mNode->GetName(), mNode);

        DrawOutputFlowParameter("Control", !mNode->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink].empty(), 0, true);

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

    void UIAINBEditorNodeSimultaneous::RenderLinks(std::vector<std::unique_ptr<UIAINBEditorNodeBase>>& Nodes)
    {
        uint32_t CurrentLinkId = mNodeId + 500; //Link start at +500
        for (int i = 0; i < mNode->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink].size(); i++) {
            if (mNode->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink][i].NodeIndex == -1 || mNode->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink][i].NodeIndex >= Nodes.size())
                continue;

            uint32_t StartPinId = mOutputFlowParameters["Control"];

            ed::Link(CurrentLinkId++, StartPinId, Nodes[mNode->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink][i].NodeIndex]->mNodeId + 1, GetValueTypeColor(6));
            mLinks.insert({ CurrentLinkId - 1, Link {.mObjectPtr = mNode, .mType = LinkType::Flow, .mParameterIndex = (uint16_t)i } });
            Nodes[mNode->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink][i].NodeIndex]->mFlowLinked = true;
        }
    }

    bool UIAINBEditorNodeSimultaneous::HasInternalParameters()
    {
        return true;
    }

    ImColor UIAINBEditorNodeSimultaneous::GetNodeColor()
    {
        return ImColor(255, 128, 128);
    }

    int UIAINBEditorNodeSimultaneous::GetNodeIndex()
    {
        return mNode->NodeIndex;
    }

    //Ensure that at least two nodes are linked
    bool UIAINBEditorNodeSimultaneous::FinalizeNode()
    {
        if(mNode->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink].size() < 2)
        {
            application::util::Logger::Error("UIAINBEditorNodeSimultaneous", "The Simultanous Node with index %u needs at least two output flow links", mNode->NodeIndex);
            return false;
        }

        return true;
    }

    bool UIAINBEditorNodeSimultaneous::HasFlowOutputParameters()
    {
        return true;
    }
}