#include "UIAINBNodeDefault.h"

#include "Editor.h"

UIAINBNodeDefault::UIAINBNodeDefault(AINBFile::Node& Node, uint32_t EditorId, TextureMgr::Texture& HeaderBackground, bool& EnableFlow)
    : UIAINBNodeBase(&Node, EditorId, HeaderBackground, EnableFlow)
{
    GenerateNodeShapeInfo();
    if (Node.Name.find(".module") != std::string::npos) {
        mHeaderColor = ImColor(90, 130, 90);
    } else {
        mHeaderColor = ImColor(70, 100, 130);
    }
}

void UIAINBNodeDefault::GenerateNodeShapeInfo()
{
    mNodeShapeInfo.FrameWidth = 8 * 2 + ImGui::CalcTextSize(mNode->GetName().c_str()).x + 32 + ImGui::GetStyle().ItemSpacing.x;

    for (uint8_t i = 0; i < AINBFile::ValueTypeCount; i++) {
        for (AINBFile::InputEntry& Input : mNode->InputParameters[i]) {
            float Width = 8 * 2;
            Width += ImGui::CalcTextSize((Input.Name + " (" + (i == (int)AINBFile::ValueType::UserDefined ? Input.Class : GetValueTypeName(i)) + ")").c_str()).x;
            Width += ImGui::GetStyle().ItemSpacing.x + 32;
            Width += GetParamValueWidth(i);
            if (i == (int)AINBFile::ValueType::Vec3f) {
                Width += ImGui::GetStyle().ItemSpacing.x;
            }
            mNodeShapeInfo.FrameWidth = std::fmax(mNodeShapeInfo.FrameWidth, Width);
        }

        for (AINBFile::OutputEntry& Output : mNode->OutputParameters[i]) { 
            float Width = 8 * 2;
            Width += ImGui::CalcTextSize((Output.Name + " (" + (i == (int)AINBFile::ValueType::UserDefined ? Output.Class : GetValueTypeName(i)) + ")").c_str()).x;
            Width += 2 * ImGui::GetStyle().ItemSpacing.x + 32;
            mNodeShapeInfo.FrameWidth = std::fmax(mNodeShapeInfo.FrameWidth, Width);
        }

        for (AINBFile::ImmediateParameter& Immediate : mNode->ImmediateParameters[i]) {
            float Width = 8 * 2;
            Width += ImGui::CalcTextSize(Immediate.Name.c_str()).x;
            Width += ImGui::GetStyle().ItemSpacing.x;
            Width += GetParamValueWidth(i);
            mNodeShapeInfo.FrameWidth = std::fmax(mNodeShapeInfo.FrameWidth, Width);
        }
    }
}

void UIAINBNodeDefault::Render()
{
    uint32_t CurrentId = mEditorId;

    // Node body
    ed::PushStyleVar(ed::StyleVar_NodePadding, ImVec4(8, 8, 8, 8));
    ed::BeginNode(CurrentId++);

    // Node header (-> NodeName)
    //DrawPinIcon(CurrentId++, false);
    if (mEnableFlow) {
        float Alpha = ImGui::GetStyle().Alpha;
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, Alpha);
        ImRect HeaderRect = DrawPin(CurrentId++, mFlowLinked, Alpha * 255, UIAINBNodeBase::PinType::Flow, ed::PinKind::Input, mNode->GetName());
        ImGui::PopStyleVar();
        mPins.insert({ CurrentId - 1, Pin { .Kind = ed::PinKind::Input, .Type = UIAINBNodeBase::PinType::Flow, .Node = mNode } });

        mNodeShapeInfo.HeaderMin = ImVec2(HeaderRect.GetTL().x - 32 - ImGui::GetStyle().ItemSpacing.x - 1, HeaderRect.GetTL().y - 14);
        mNodeShapeInfo.HeaderMax = ImVec2(mNodeShapeInfo.HeaderMin.x + mNodeShapeInfo.FrameWidth + 2 + ((Editor::UIScale - 1) * 14), HeaderRect.GetBR().y + 12);
    } else {
        ImGui::Text(mNode->GetName().c_str());
        mNodeShapeInfo.HeaderMin = ImVec2(ImGui::GetItemRectMin().x - 1 + ((Editor::UIScale - 1) * 10) - ImGui::GetStyle().ItemSpacing.x, ImGui::GetItemRectMin().y - 9);
        mNodeShapeInfo.HeaderMax = ImVec2(mNodeShapeInfo.HeaderMin.x + mNodeShapeInfo.FrameWidth + 2 + ((Editor::UIScale - 1) * 14), ImGui::GetItemRectMax().y + 8);
    }

    ImGui::Dummy(ImVec2(0, 8));

    bool AddedHeader = false;
    for (uint8_t i = 0; i < AINBFile::ValueTypeCount; i++) {
        for (AINBFile::InputEntry& Input : mNode->InputParameters[i]) {
            if (!AddedHeader) {
                mNodeShapeInfo.HeaderMax.x += 8;
                AddedHeader = true;
            }

            // Input param (-> Name [Value])
            float Alpha = ImGui::GetStyle().Alpha;
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, Alpha);
            DrawPin(CurrentId++, Input.NodeIndex >= 0 || !Input.Sources.empty(), Alpha * 255, ValueTypeToPinType(i), ed::PinKind::Input, Input.Name + " (" + (i == (int)AINBFile::ValueType::UserDefined ? Input.Class : GetValueTypeName(i)) + ")");
            ImGui::PopStyleVar();
            mInputParameters[i].push_back(CurrentId - 1);
            mPins.insert({ CurrentId - 1, Pin { .Kind = ed::PinKind::Input, .Type = ValueTypeToPinType(i), .Node = mNode, .ObjectPtr = &Input } });
            ImGui::SameLine();
            if (Input.NodeIndex == -1 && Input.Sources.empty()) { // Input param is not linked to any output param, so the value has to be set directly
                DrawParameterValue(static_cast<AINBFile::ValueType>(i), Input.Name, CurrentId, (void*)&Input.Value);
            } else {
                if (i == (int)AINBFile::ValueType::UserDefined) {
                    ImGui::NewLine();
                }
                else
                {
                    ImGui::Dummy(ImVec2(GetParamValueWidth(i), 0));
                }
            }
        }
    }

    for (uint8_t i = 0; i < AINBFile::ValueTypeCount; i++) {
        for (uint16_t j = 0; j < mNode->OutputParameters[i].size(); j++) {
            if (!AddedHeader) {
                mNodeShapeInfo.HeaderMax.x += 8;
                AddedHeader = true;
            }

            AINBFile::OutputEntry& Output = mNode->OutputParameters[i][j];
            //Output param (ParamName (Type/Class) ->)
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + mNodeShapeInfo.FrameWidth - (18 + ImGui::CalcTextSize((Output.Name + " (" + (i == (int)AINBFile::ValueType::UserDefined ? Output.Class : GetValueTypeName(i)) + ")").c_str()).x + 18 + ImGui::GetStyle().ItemSpacing.x));
            float Alpha = ImGui::GetStyle().Alpha;
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, Alpha);
            DrawPin(CurrentId++, std::find(mLinkedOutputParams[i].begin(), mLinkedOutputParams[i].end(), j) != mLinkedOutputParams[i].end(), Alpha * 255, ValueTypeToPinType(i), ed::PinKind::Output, Output.Name + " (" + (i == (int)AINBFile::ValueType::UserDefined ? Output.Class : GetValueTypeName(i)) + ")");
            ImGui::PopStyleVar();
            mOutputParameters[i].push_back(CurrentId - 1);
            mPins.insert({ CurrentId - 1, Pin { .Kind = ed::PinKind::Output, .Type = ValueTypeToPinType(i), .Node = mNode, .ObjectPtr = &Output, .ParameterIndex = j } });
        }
    }

    ImVec2 CursorPos = ImGui::GetCursorPos();
    bool HasImmediate = false;
    for (uint8_t i = 0; i < AINBFile::ValueTypeCount; i++) {
        if (!mNode->ImmediateParameters[i].empty()) {
            HasImmediate = true;
            break;
        }
    }

    if (HasImmediate)
        ImGui::NewLine();

    for (uint8_t i = 0; i < AINBFile::ValueTypeCount; i++) {
        for (AINBFile::ImmediateParameter& Immediate : mNode->ImmediateParameters[i]) {
            //Immediate parameter (ParamName Value)
            ImGui::TextUnformatted(Immediate.Name.c_str());
            ImGui::SameLine();

            bool ValueTypeMismatch = Immediate.ValueType != Immediate.Value.index();
            if (ValueTypeMismatch) {
                switch (i) {
                case (int)AINBFile::ValueType::Int:
                    Immediate.Value = (uint32_t)0;
                    break;
                case (int)AINBFile::ValueType::Float:
                    Immediate.Value = 0.0f;
                    break;
                case (int)AINBFile::ValueType::Bool:
                    Immediate.Value = false;
                    break;
                case (int)AINBFile::ValueType::String:
                    Immediate.Value = "";
                    break;
                case (int)AINBFile::ValueType::Vec3f:
                    Immediate.Value = Vector3F(0, 0, 0);
                    break;
                }
            }

            DrawParameterValue(static_cast<AINBFile::ValueType>(i), Immediate.Name, CurrentId, (void*)&Immediate.Value);
        }
    }

    ed::EndNode();
    ed::PopStyleVar();

    DrawNodeHeader();
    if(HasImmediate)
        DrawImmediateSeperator(CursorPos);
}

void UIAINBNodeDefault::RenderLinks(std::vector<std::unique_ptr<UIAINBNodeBase>>& Nodes)
{
    uint32_t CurrentLinkId = mEditorId + 500; //Link start at +500
    for (uint8_t i = 0; i < AINBFile::ValueTypeCount; i++) {
        for (uint16_t j = 0; j < mNode->InputParameters[i].size(); j++) {
            AINBFile::InputEntry& Input = mNode->InputParameters[i][j];
            if (Input.NodeIndex >= 0) { //Single link
                //ed::Link(CurrentLinkId++, Nodes[Input.NodeIndex]->mOutputParameters[i][Input.ParameterIndex], mInputParameters[i][j], GetValueTypeColor(i));
                mLinks.insert({ CurrentLinkId++, Link { .ObjectPtr = &Input, .Type = LinkType::Parameter, .NodeIndex = (uint16_t)Input.NodeIndex, .ParameterIndex = (uint16_t)Input.ParameterIndex } });
                
                if (Input.Function.Instructions.size() > 0)
                {
                    AINBFile::ValueType DataType;
                    switch (Input.Function.InputDataType)
                    {
                    case EXB::Type::Bool:
                        DataType = AINBFile::ValueType::Bool;
                        break;
                    case EXB::Type::F32:
                        DataType = AINBFile::ValueType::Float;
                        break;
                    case EXB::Type::S32:
                        DataType = AINBFile::ValueType::Int;
                        break;
                    case EXB::Type::String:
                        DataType = AINBFile::ValueType::String;
                        break;
                    case EXB::Type::Vec3f:
                        DataType = AINBFile::ValueType::Vec3f;
                        break;
                    default:
                        DataType = (AINBFile::ValueType)i;
                    }
                    
                    ed::Link(CurrentLinkId -1, Nodes[Input.NodeIndex]->mOutputParameters[(int)DataType][Input.ParameterIndex], mInputParameters[i][j], GetValueTypeColor(i));
                }
                else
                {
                    ed::Link(CurrentLinkId - 1, Nodes[Input.NodeIndex]->mOutputParameters[i][Input.ParameterIndex], mInputParameters[i][j], GetValueTypeColor(i));
                }
            }
            for (AINBFile::MultiEntry& Entry : Input.Sources) { // Multi link
                ed::Link(CurrentLinkId++, Nodes[Entry.NodeIndex]->mOutputParameters[i][Entry.ParameterIndex], mInputParameters[i][j], GetValueTypeColor(i));
                mLinks.insert({ CurrentLinkId - 1, Link { .ObjectPtr = &Input, .Type = LinkType::Parameter, .NodeIndex = Entry.NodeIndex, .ParameterIndex = Entry.ParameterIndex } });
            }
        }
    }
}

ImColor UIAINBNodeDefault::GetHeaderColor()
{
    return mHeaderColor;
}