#include "UIAINBEditorNodeS32Selector.h"

#include <util/Logger.h>
#include <algorithm>

namespace application::rendering::ainb::nodes
{
    application::rendering::popup::PopUpBuilder UIAINBEditorNodeS32Selector::gAddNewSelection;

    void UIAINBEditorNodeS32Selector::Initialize()
    {
        gAddNewSelection.Title("Add S32 Case").Flag(ImGuiWindowFlags_NoResize).NeedsConfirmation(false).AddDataStorage(sizeof(int32_t)).ContentDrawingFunction([](popup::PopUpBuilder& Builder)
        {
            if (ImGui::BeginTable("S32Table", 2, ImGuiTableFlags_BordersInnerV))
			{
				ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("S32 Case Value").x);

				ImGui::TableNextRow();

				ImGui::TableSetColumnIndex(0);
				ImGui::Text("S32 Case Value");
				ImGui::TableNextColumn();

				ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
                ImGui::InputScalar("##S32CaseValue", ImGuiDataType_S32, Builder.GetDataStorage(0).mPtr);
                ImGui::PopItemWidth();

                ImGui::EndTable();
            }
        }).Register();
    }

    UIAINBEditorNodeS32Selector::UIAINBEditorNodeS32Selector(int UniqueId, application::file::game::ainb::AINBFile::Node& Node) : UIAINBEditorNodeBase(UniqueId, Node)
    {
        for (application::file::game::ainb::AINBFile::LinkedNodeInfo& Info : mNode->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink]) 
        {
            if(Info.Condition != "" && Info.Condition != "Default")
            {
                mConditions.push_back(std::stoi(Info.Condition));
            }
        }
    }

    void UIAINBEditorNodeS32Selector::DrawImpl()
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

        float BasePosX = ImGui::GetCursorPosX();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
        if(ImGui::Button(("+##" + std::to_string(mNode->NodeIndex)).c_str()))
        {
            gAddNewSelection.Open([this](popup::PopUpBuilder& Builder)
            {
                int32_t Value = *reinterpret_cast<int32_t*>(Builder.GetDataStorage(0).mPtr);

                if(std::find(mConditions.begin(), mConditions.end(), Value) != mConditions.end())
                    return;

                mConditions.push_back(Value);
            });
        }
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::SetCursorPosX(BasePosX);

        bool HasDefaultLink = false;
        for (application::file::game::ainb::AINBFile::LinkedNodeInfo& Info : mNode->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink]) 
        {
            if(Info.Condition == "Default")
            {
                HasDefaultLink = true;
                break;
            }
        }
        DrawOutputFlowParameter("Default", HasDefaultLink, 0);

        uint32_t Index = 0;
        for(auto Iter = mConditions.begin(); Iter != mConditions.end(); )
        {
            bool Linked = false;
            std::string Str = std::to_string(*Iter);
            for (application::file::game::ainb::AINBFile::LinkedNodeInfo& Info : mNode->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink]) 
            {
                if(Info.Condition == Str)
                {
                    Linked = true;
                    break;
                }
            }
            std::string VisualStr = "= " + Str;
            float PosX = ImGui::GetCursorPosX();
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (mNodeShapeInfo.mHeaderMax.x - mNodeShapeInfo.mHeaderMin.x) - 8 - (18 + ImGui::CalcTextSize(VisualStr.c_str()).x + ImGui::GetStyle().ItemSpacing.x + 16) - ImGui::GetStyle().ItemSpacing.x - ImGui::CalcTextSize("-").x - ImGui::GetStyle().ItemInnerSpacing.x * 2.0f);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.47f, 0.07f, 0.07f, 1.0f));
            bool WantRemove = ImGui::Button(("-##" + Str + "_" + std::to_string(mNode->NodeIndex)).c_str());
            ImGui::PopStyleColor();
            if(WantRemove)
            {
                mNode->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink].erase(
                    std::remove_if(mNode->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink].begin(), mNode->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink].end(), [Str](const application::file::game::ainb::AINBFile::LinkedNodeInfo& Info)
                        {
                            return Info.Condition == Str;
                        }),
                    mNode->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink].end());
                
                Iter = mConditions.erase(Iter);
                continue;
            }
            ImGui::SameLine();
            ImGui::SetCursorPosX(PosX);
            DrawOutputFlowParameter(VisualStr, Linked, 1 + Index);
            Index++;
            Iter++;
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

    void UIAINBEditorNodeS32Selector::RenderLinks(std::vector<std::unique_ptr<UIAINBEditorNodeBase>>& Nodes)
    {
        uint32_t CurrentLinkId = mNodeId + 500; //Link start at +500
        for (int i = 0; i < mNode->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink].size(); i++) 
        {
            if (mNode->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink][i].NodeIndex == -1 || mNode->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink][i].NodeIndex >= Nodes.size())
                continue;

            int32_t StartPinId = -1;

            application::file::game::ainb::AINBFile::LinkedNodeInfo& Info = mNode->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink][i];

            if(Info.Condition == "Default")
            {
                StartPinId = mOutputFlowParameters["Default"];
            }
            else
            {
                StartPinId = mOutputFlowParameters["= " + Info.Condition];
            }

            ed::Link(CurrentLinkId++, StartPinId, Nodes[Info.NodeIndex]->mNodeId + 1, GetValueTypeColor(6));
            mLinks.insert({ CurrentLinkId - 1, Link {.mObjectPtr = mNode, .mType = LinkType::Flow, .mParameterIndex = (uint16_t)i } });
            Nodes[Info.NodeIndex]->mFlowLinked = true;
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

    bool UIAINBEditorNodeS32Selector::HasInternalParameters()
    {
        return true;
    }

    ImColor UIAINBEditorNodeS32Selector::GetNodeColor()
    {
        return ImColor(255, 128, 128);
    }

    int UIAINBEditorNodeS32Selector::GetNodeIndex()
    {
        return mNode->NodeIndex;
    }

    //Ensure that first node link is True
    bool UIAINBEditorNodeS32Selector::FinalizeNode()
    {
        if(mNode->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink].size() < 2)
        {
            application::util::Logger::Error("UIAINBEditorNodeS32Selector", "The S32 Selector with index %u is missing at least one output flow link", mNode->NodeIndex);
            return false;
        }

        std::sort(mNode->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink].begin(), mNode->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink].end(), [](application::file::game::ainb::AINBFile::LinkedNodeInfo& A, application::file::game::ainb::AINBFile::LinkedNodeInfo& B)
        {
            if(A.Condition == "Default") return false;  // a should go after b
            if(B.Condition == "Default") return true;   // b should go after a

            return std::stoi(A.Condition) < std::stoi(B.Condition);
        });

        return true;
    }

    void UIAINBEditorNodeS32Selector::PostProcessLinkedNodeInfo(Pin& StartPin, application::file::game::ainb::AINBFile::LinkedNodeInfo& Info)
    {
        Info.Condition = StartPin.mParameterIndex == 0 ? "Default" : std::to_string(mConditions[StartPin.mParameterIndex - 1]);
    }

    bool UIAINBEditorNodeS32Selector::HasFlowOutputParameters()
    {
        return true;
    }
}