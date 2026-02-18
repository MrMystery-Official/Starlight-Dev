#include "UIAINBEditorNodeBoolSelector.h"

#include <util/Logger.h>

namespace application::rendering::ainb::nodes
{
    UIAINBEditorNodeBoolSelector::UIAINBEditorNodeBoolSelector(int UniqueId, application::file::game::ainb::AINBFile::Node& Node) : UIAINBEditorNodeBase(UniqueId, Node)
    {

    }

    void UIAINBEditorNodeBoolSelector::DrawImpl()
    {
        ed::PushStyleVar(ed::StyleVar_NodePadding, ImVec4(8, 8, 8, 8));
        ed::BeginNode(mUniqueId++);

        DrawNodeHeader(mNode->GetName(), mNode);

        for (uint8_t Type = 0; Type < application::file::game::ainb::AINBFile::ValueTypeCount; Type++)
		{
			for (uint8_t Index = 0; Index < mNode->InputParameters[Type].size(); Index++)
			{
				DrawInputParameter(Type, Index);
			}
		}

        bool HasTrue = false;
        bool HasFalse = false;

        for(application::file::game::ainb::AINBFile::LinkedNodeInfo& Info : mNode->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink])
        {
            if(Info.Condition == "True")
                HasTrue = true;

            if(Info.Condition == "False")
                HasFalse = true;
        }

        DrawOutputFlowParameter("True", HasTrue, 0);
        DrawOutputFlowParameter("False", HasFalse, 1);

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

    void UIAINBEditorNodeBoolSelector::RenderLinks(std::vector<std::unique_ptr<UIAINBEditorNodeBase>>& Nodes)
    {
        uint32_t CurrentLinkId = mNodeId + 500; //Link start at +500
        for (int i = 0; i < mNode->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink].size(); i++) {
            if (mNode->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink][i].NodeIndex == -1 || mNode->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink][i].NodeIndex >= Nodes.size())
                continue;

            uint32_t StartPinId = mOutputFlowParameters[mNode->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink][i].Condition];

            ed::Link(CurrentLinkId++, StartPinId, Nodes[mNode->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink][i].NodeIndex]->mNodeId + 1, GetValueTypeColor(6));
            mLinks.insert({ CurrentLinkId - 1, Link {.mObjectPtr = mNode, .mType = LinkType::Flow, .mParameterIndex = (uint16_t)i } });
            Nodes[mNode->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink][i].NodeIndex]->mFlowLinked = true;
        }

        for (uint8_t i = 0; i < application::file::game::ainb::AINBFile::ValueTypeCount; i++)
        {
            for (uint16_t j = 0; j < mNode->InputParameters[i].size(); j++)
            {
                application::file::game::ainb::AINBFile::InputEntry& Input = mNode->InputParameters[i][j];
                if (Input.NodeIndex >= 0) //Single link
                {
                    //ed::Link(CurrentLinkId++, Nodes[Input.NodeIndex]->mOutputParameters[i][Input.ParameterIndex], mInputParameters[i][j], GetValueTypeColor(i));
                    mLinks.insert({ CurrentLinkId++, Link {.mObjectPtr = &Input, .mType = LinkType::Parameter, .mNodeIndex = (uint16_t)Input.NodeIndex, .mParameterIndex = (uint16_t)Input.ParameterIndex } });

                    if (!Input.Function.Instructions.empty())
                    {
                        application::file::game::ainb::AINBFile::ValueType DataType;
                        switch (Input.Function.InputDataType)
                        {
                        case application::file::game::ainb::EXB::Type::Bool:
                            DataType = application::file::game::ainb::AINBFile::ValueType::Bool;
                            break;
                        case application::file::game::ainb::EXB::Type::F32:
                            DataType = application::file::game::ainb::AINBFile::ValueType::Float;
                            break;
                        case application::file::game::ainb::EXB::Type::S32:
                            DataType = application::file::game::ainb::AINBFile::ValueType::Int;
                            break;
                        case application::file::game::ainb::EXB::Type::String:
                            DataType = application::file::game::ainb::AINBFile::ValueType::String;
                            break;
                        case application::file::game::ainb::EXB::Type::Vec3f:
                            DataType = application::file::game::ainb::AINBFile::ValueType::Vec3f;
                            break;
                        default:
                            DataType = (application::file::game::ainb::AINBFile::ValueType)i;
                        }

                        ed::Link(CurrentLinkId - 1, Nodes[Input.NodeIndex]->mOutputParameters[(int)DataType][Input.ParameterIndex], mInputParameters[i][j], GetValueTypeColor(i));
                    }
                    else
                    {
                        ed::Link(CurrentLinkId - 1, Nodes[Input.NodeIndex]->mOutputParameters[i][Input.ParameterIndex], mInputParameters[i][j], GetValueTypeColor(i));
                    }
                }
                for (application::file::game::ainb::AINBFile::MultiEntry& Entry : Input.Sources) // Multi link
                {
                    ed::Link(CurrentLinkId++, Nodes[Entry.NodeIndex]->mOutputParameters[i][Entry.ParameterIndex], mInputParameters[i][j], GetValueTypeColor(i));
                    mLinks.insert({ CurrentLinkId - 1, Link {.mObjectPtr = &Input, .mType = LinkType::Parameter, .mNodeIndex = Entry.NodeIndex, .mParameterIndex = Entry.ParameterIndex } });
                }
            }
        }
    }

    bool UIAINBEditorNodeBoolSelector::HasInternalParameters()
    {
        return true;
    }

    ImColor UIAINBEditorNodeBoolSelector::GetNodeColor()
    {
        return ImColor(255, 128, 128);
    }

    int UIAINBEditorNodeBoolSelector::GetNodeIndex()
    {
        return mNode->NodeIndex;
    }

    //Ensure that first node link is True
    bool UIAINBEditorNodeBoolSelector::FinalizeNode()
    {
        if(mNode->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink].size() != 2)
        {
            application::util::Logger::Error("UIAINBEditorNodeBoolSelector", "The Bool Selector with index %u is missing at least one output link", mNode->NodeIndex);
            return false;
        }

        //Swapping conditions
        if(mNode->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink][0].Condition == "False")
        {
            application::file::game::ainb::AINBFile::LinkedNodeInfo Info = mNode->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink][0];
            mNode->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink][0] = mNode->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink][1];
            mNode->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink][1] = Info;
        }

        return true;
    }

    void UIAINBEditorNodeBoolSelector::PostProcessLinkedNodeInfo(Pin& StartPin, application::file::game::ainb::AINBFile::LinkedNodeInfo& Info)
    {
        Info.Condition = StartPin.mParameterIndex == 0 ? "True" : "False";
    }

    bool UIAINBEditorNodeBoolSelector::HasFlowOutputParameters()
    {
        return true;
    }
}