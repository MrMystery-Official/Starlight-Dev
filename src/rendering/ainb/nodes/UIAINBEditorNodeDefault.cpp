#include "UIAINBEditorNodeDefault.h"

#include <util/Logger.h>

namespace application::rendering::ainb::nodes
{
	UIAINBEditorNodeDefault::UIAINBEditorNodeDefault(int UniqueId, application::file::game::ainb::AINBFile::Node& Node) : UIAINBEditorNodeBase(UniqueId, Node)
	{
        if (Node.Name.find(".module") != std::string::npos)
        {
            mColor = ImColor(128, 255, 128);
        }
        else
        {
            mColor = ImColor(128, 195, 248);
        }

        for(application::file::game::ainb::AINBFile::LinkedNodeInfo& Info : mNode->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink])
        {
            if(Info.ConnectionName == "" || std::ranges::find(mFixedFlowOutputParameters, Info.ConnectionName) != mFixedFlowOutputParameters.end())
                continue;
            
            mFixedFlowOutputParameters.push_back(Info.ConnectionName);
        }
	}

	void UIAINBEditorNodeDefault::DrawImpl()
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

        for(size_t i = 0; i < mFixedFlowOutputParameters.size(); i++)
        {
            bool Linked = false;
            for(application::file::game::ainb::AINBFile::LinkedNodeInfo& Info : mNode->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink])
            {
                if(Info.ConnectionName == mFixedFlowOutputParameters[i])
                {
                    Linked = true;
                    break;
                }
            }
            DrawOutputFlowParameter(mFixedFlowOutputParameters[i], Linked, i);
        }

		for (uint8_t Type = 0; Type < application::file::game::ainb::AINBFile::ValueTypeCount; Type++)
		{
			for (uint8_t Index = 0; Index < mNode->OutputParameters[Type].size(); Index++)
			{
				DrawOutputParameter(Type, Index);
			}
		}

        /*
        for(application::file::game::ainb::AINBFile::LinkedNodeInfo& Info : mNode->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink])
        {
            DrawOutputFlowParameter(Info.Condition, false);
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

    void UIAINBEditorNodeDefault::PostProcessLinkedNodeInfo(Pin& StartPin, application::file::game::ainb::AINBFile::LinkedNodeInfo& Info)
    {
        Info.ConnectionName = mFixedFlowOutputParameters[StartPin.mParameterIndex];
    }

	void UIAINBEditorNodeDefault::RenderLinks(std::vector<std::unique_ptr<UIAINBEditorNodeBase>>& Nodes)
	{
        uint32_t CurrentLinkId = mNodeId + 500; //Link start at +500
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

        for (int i = 0; i < mNode->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink].size(); i++) {
            if (mNode->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink][i].NodeIndex == -1 || mNode->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink][i].NodeIndex >= Nodes.size())
                continue;

            application::file::game::ainb::AINBFile::LinkedNodeInfo& Info = mNode->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink][i];

            uint32_t StartPinId = mOutputFlowParameters[Info.ConnectionName];

            ed::Link(CurrentLinkId++, StartPinId, Nodes[Info.NodeIndex]->mNodeId + 1, GetValueTypeColor(6));
            mLinks.insert({ CurrentLinkId - 1, Link {.mObjectPtr = mNode, .mType = LinkType::Flow, .mParameterIndex = (uint16_t)i } });
            Nodes[Info.NodeIndex]->mFlowLinked = true;
        }
	}

	ImColor UIAINBEditorNodeDefault::GetNodeColor()
	{
		return mColor;
	}

    bool UIAINBEditorNodeDefault::HasInternalParameters()
    {
        for (uint8_t i = 0; i < application::file::game::ainb::AINBFile::ValueTypeCount; i++)
        {
            if (!mNode->ImmediateParameters[i].empty())
            {
                return true;
            }
        }
        return false;
    }

    void UIAINBEditorNodeDefault::InjectFixedFlowParameters(application::manager::AINBNodeMgr::NodeDef* Def)
    {
        if(Def == nullptr)
        {
            application::util::Logger::Error("UIAINBEditorNodeDefault", "Could not inject fixed flow parameters, node def was a nullptr");
            return;
        }

        mFixedFlowOutputParameters = Def->mFlowOutputParameters;
    }

	int UIAINBEditorNodeDefault::GetNodeIndex()
	{
		return mNode->NodeIndex;
	}

    bool UIAINBEditorNodeDefault::HasFlowOutputParameters()
    {
        return !mFixedFlowOutputParameters.empty();
    }
}