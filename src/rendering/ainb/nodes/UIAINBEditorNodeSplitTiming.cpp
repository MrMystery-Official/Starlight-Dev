#include "UIAINBEditorNodeSplitTiming.h"

#include <util/Logger.h>

namespace application::rendering::ainb::nodes
{
	UIAINBEditorNodeSplitTiming::UIAINBEditorNodeSplitTiming(int UniqueId, application::file::game::ainb::AINBFile::Node& Node) : UIAINBEditorNodeBase(UniqueId, Node)
    {

    }

    void UIAINBEditorNodeSplitTiming::DrawImpl()
    {
        ed::PushStyleVar(ed::StyleVar_NodePadding, ImVec4(8, 8, 8, 8));
        ed::BeginNode(mUniqueId++);

        DrawNodeHeader(mNode->GetName(), mNode);

        bool HasEnterLink = false;
        bool HasUpdateLink = false;
        bool HasLeaveLink = false;
        for (application::file::game::ainb::AINBFile::LinkedNodeInfo& Info : mNode->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink]) 
        {
            if (Info.ConnectionName == "Enter")
                HasEnterLink = true;
            if (Info.ConnectionName == "Update")
                HasUpdateLink = true;
            if (Info.ConnectionName == "Leave")
                HasLeaveLink = true;
        }

        DrawOutputFlowParameter("Enter", HasEnterLink, 0);
        DrawOutputFlowParameter("Update", HasUpdateLink, 1);
        DrawOutputFlowParameter("Leave", HasLeaveLink, 2);

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

    void UIAINBEditorNodeSplitTiming::RenderLinks(std::vector<std::unique_ptr<UIAINBEditorNodeBase>>& Nodes)
    {
        uint32_t CurrentLinkId = mNodeId + 500; //Link start at +500
        for (int i = 0; i < mNode->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink].size(); i++) {
            if (mNode->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink][i].NodeIndex == -1 || mNode->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink][i].NodeIndex >= Nodes.size())
                continue;
            
            uint32_t StartPinId = mOutputFlowParameters[mNode->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink][i].ConnectionName];

            ed::Link(CurrentLinkId++, StartPinId, Nodes[mNode->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink][i].NodeIndex]->mNodeId + 1, GetValueTypeColor(6));
            mLinks.insert({ CurrentLinkId - 1, Link {.mObjectPtr = mNode, .mType = LinkType::Flow, .mParameterIndex = (uint16_t)i } });
            Nodes[mNode->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink][i].NodeIndex]->mFlowLinked = true;
        }
    }

    bool UIAINBEditorNodeSplitTiming::HasInternalParameters()
    {
        return true;
    }

    ImColor UIAINBEditorNodeSplitTiming::GetNodeColor()
    {
        return ImColor(255, 128, 128);
    }

    int UIAINBEditorNodeSplitTiming::GetNodeIndex()
    {
        return mNode->NodeIndex;
    }

    //Needs all three links
    bool UIAINBEditorNodeSplitTiming::FinalizeNode()
    {
        bool HasEnterLink = false;
        bool HasUpdateLink = false;
        bool HasLeaveLink = false;
        for (application::file::game::ainb::AINBFile::LinkedNodeInfo& Info : mNode->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink])
        {
            if (Info.ConnectionName == "Enter")
                HasEnterLink = true;
            if (Info.ConnectionName == "Update")
                HasUpdateLink = true;
            if (Info.ConnectionName == "Leave")
                HasLeaveLink = true;
        }

        if(!HasEnterLink || !HasUpdateLink || !HasLeaveLink)
        {
            application::util::Logger::Error("UIAINBEditorNodeSplitTiming", "The Split Timing Node with index %u is missing at least one output link. (HasEnterLink: %u, HasUpdateLink: %u, HasLeaveLink: %u)", mNode->NodeIndex, HasEnterLink, HasUpdateLink, HasLeaveLink);
            return false;
        }

        application::file::game::ainb::AINBFile::LinkedNodeInfo EnterInfo;
        application::file::game::ainb::AINBFile::LinkedNodeInfo UpdateInfo;
        application::file::game::ainb::AINBFile::LinkedNodeInfo LeaveInfo;

        for (application::file::game::ainb::AINBFile::LinkedNodeInfo& Info : mNode->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink])
        {
            if (Info.ConnectionName == "Enter")
                EnterInfo = Info;
            if (Info.ConnectionName == "Update")
                UpdateInfo = Info;
            if (Info.ConnectionName == "Leave")
                LeaveInfo = Info;
        }

        mNode->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink].resize(3);
        mNode->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink][0] = EnterInfo;
        mNode->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink][1] = UpdateInfo;
        mNode->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink][2] = LeaveInfo;

        return true;
    }

    bool UIAINBEditorNodeSplitTiming::HasFlowOutputParameters()
    {
        return true;
    }

    void UIAINBEditorNodeSplitTiming::PostProcessLinkedNodeInfo(Pin& StartPin, application::file::game::ainb::AINBFile::LinkedNodeInfo& Info)
    {
        switch (StartPin.mParameterIndex)
        {
        case 1:
            Info.ConnectionName = "Update";
            break;
        case 2:
            Info.ConnectionName = "Leave";
            break;
        case 0:
        default:
            Info.ConnectionName = "Enter";
            break;
        }
    }
}