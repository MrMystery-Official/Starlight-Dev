#include "UIAINBNodeBoolSelector.h"

#include "Editor.h"

UIAINBNodeBoolSelector::UIAINBNodeBoolSelector(AINBFile::Node& Node, uint32_t EditorId, TextureMgr::Texture& HeaderBackground, bool& EnableFlow)
    : UIAINBNodeBase(&Node, EditorId, HeaderBackground, EnableFlow)
{
    GenerateNodeShapeInfo();
}

void UIAINBNodeBoolSelector::RebuildNode()
{
    mNode->Flags.clear();

    for (int i = 0; i < AINBFile::ValueTypeCount; i++) {
        mNode->InputParameters[i].clear();
        mNode->OutputParameters[i].clear();
    }

    for (int i = 0; i < 10; i++) {
        mNode->LinkedNodes[i].clear();
    }

    AINBFile::InputEntry Param;
    Param.Value = false;
    Param.ValueType = (int)AINBFile::ValueType::Bool;
    Param.Name = "Input";
    Param.NodeIndex = -1;
    Param.ParameterIndex = -1;
    mNode->InputParameters[(int)AINBFile::ValueType::Bool].push_back(Param);
}

void UIAINBNodeBoolSelector::GenerateNodeShapeInfo()
{
    mNodeShapeInfo.FrameWidth = 8 * 2 + ImGui::CalcTextSize(mNode->GetName().c_str()).x + 24 + ImGui::GetStyle().ItemSpacing.x;

    for (uint8_t i = 0; i < AINBFile::ValueTypeCount; i++) {
        for (AINBFile::InputEntry& Input : mNode->InputParameters[i]) {
            float Width = 8 * 2;
            Width += ImGui::CalcTextSize((Input.Name + " (" + (i == (int)AINBFile::ValueType::UserDefined ? Input.Class : GetValueTypeName(i)) + ")").c_str()).x;
            Width += ImGui::GetStyle().ItemSpacing.x + 24;
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

void UIAINBNodeBoolSelector::Render()
{
    uint32_t CurrentId = mEditorId;

    // Node body
    ed::PushStyleVar(ed::StyleVar_NodePadding, ImVec4(8, 8, 8, 8));
    ed::BeginNode(CurrentId++);

    // Node header (-> NodeName)
    // DrawPinIcon(CurrentId++, false);
    if (mEnableFlow) {
        float Alpha = ImGui::GetStyle().Alpha;
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, Alpha);
        ImRect HeaderRect = DrawPin(CurrentId++, mFlowLinked, Alpha * 255, UIAINBNodeBase::PinType::Flow, ed::PinKind::Input, mNode->GetName());
        ImGui::PopStyleVar();
        mPins.insert({ CurrentId - 1, Pin { .Kind = ed::PinKind::Input, .Type = UIAINBNodeBase::PinType::Flow, .Node = mNode } });

        mNodeShapeInfo.HeaderMin = ImVec2(HeaderRect.GetTL().x - 32 - ImGui::GetStyle().ItemSpacing.x - 1, HeaderRect.GetTL().y - 14);
        mNodeShapeInfo.HeaderMax = ImVec2(mNodeShapeInfo.HeaderMin.x + mNodeShapeInfo.FrameWidth + 10 + ((Editor::UIScale - 1) * 10), HeaderRect.GetBR().y + 12);
    } else {
        ImGui::Text(mNode->GetName().c_str());
        mNodeShapeInfo.HeaderMin = ImVec2(ImGui::GetItemRectMin().x - 1 + ((Editor::UIScale - 1) * 10) - ImGui::GetStyle().ItemSpacing.x, ImGui::GetItemRectMin().y - 9);
        mNodeShapeInfo.HeaderMax = ImVec2(mNodeShapeInfo.HeaderMin.x + mNodeShapeInfo.FrameWidth + 2 + ((Editor::UIScale - 1) * 24), ImGui::GetItemRectMax().y + 8);
    }

    ImGui::Dummy(ImVec2(0, 8));

    mNodeShapeInfo.HeaderMax.x += 8;

    for (uint8_t i = 0; i < AINBFile::ValueTypeCount; i++) {
        for (AINBFile::InputEntry& Input : mNode->InputParameters[i]) {

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
                } else {
                    ImGui::Dummy(ImVec2(GetParamValueWidth(i), 0));
                }
            }
        }
    }

    bool HasTrueLink = false;
    bool HasFalseLink = false;
    for (AINBFile::LinkedNodeInfo& Info : mNode->LinkedNodes[(int)AINBFile::LinkedNodeMapping::StandardLink]) {
        if (Info.Condition == "True")
            HasTrueLink = true;
        if (Info.Condition == "False")
            HasFalseLink = true;
    }

    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + mNodeShapeInfo.FrameWidth - (8 + ImGui::CalcTextSize("True").x + 18 + ImGui::GetStyle().ItemSpacing.x));
    float Alpha = ImGui::GetStyle().Alpha;
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, Alpha);
    DrawPin(CurrentId++, HasTrueLink, Alpha * 255, UIAINBNodeBase::PinType::Flow, ed::PinKind::Output, "True");
    mPins.insert({ CurrentId - 1, UIAINBNodeBase::Pin { .Kind = ed::PinKind::Output, .Type = UIAINBNodeBase::PinType::Flow, .Node = mNode, .ParameterIndex = 0, .AllowMultipleLinks = false, .AlreadyLinked = HasTrueLink } });
    mPinIdTrue = CurrentId - 1;

    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + mNodeShapeInfo.FrameWidth - (8 + ImGui::CalcTextSize("False").x + 18 + ImGui::GetStyle().ItemSpacing.x));
    DrawPin(CurrentId++, HasFalseLink, Alpha * 255, UIAINBNodeBase::PinType::Flow, ed::PinKind::Output, "False");
    ImGui::PopStyleVar();
    mPins.insert({ CurrentId - 1, UIAINBNodeBase::Pin { .Kind = ed::PinKind::Output, .Type = UIAINBNodeBase::PinType::Flow, .Node = mNode, .ParameterIndex = 1, .AllowMultipleLinks = false, .AlreadyLinked = HasFalseLink } });
    mPinIdFalse = CurrentId - 1;

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
            // Immediate parameter (ParamName Value)
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
    if (HasImmediate)
        DrawImmediateSeperator(CursorPos);
}

void UIAINBNodeBoolSelector::PostProcessLinkedNodeInfo(Pin& StartPin, AINBFile::LinkedNodeInfo& Info)
{
    Info.Condition = StartPin.ParameterIndex == 0 ? "True" : "False";
}

void UIAINBNodeBoolSelector::PostProcessNode()
{
    if (mNode->LinkedNodes[(int)AINBFile::LinkedNodeMapping::StandardLink][0].Condition == "False")
    {
        AINBFile::LinkedNodeInfo FalseInfo = mNode->LinkedNodes[(int)AINBFile::LinkedNodeMapping::StandardLink][0];
        mNode->LinkedNodes[(int)AINBFile::LinkedNodeMapping::StandardLink][0] = mNode->LinkedNodes[(int)AINBFile::LinkedNodeMapping::StandardLink][1];
        mNode->LinkedNodes[(int)AINBFile::LinkedNodeMapping::StandardLink][1] = FalseInfo;
    }
}

void UIAINBNodeBoolSelector::RenderLinks(std::vector<std::unique_ptr<UIAINBNodeBase>>& Nodes)
{
    uint32_t CurrentLinkId = mEditorId + 500; // Link start at +500
    for (int i = 0; i < mNode->LinkedNodes[(int)AINBFile::LinkedNodeMapping::StandardLink].size(); i++) {
        if (mNode->LinkedNodes[(int)AINBFile::LinkedNodeMapping::StandardLink][i].NodeIndex == -1 || mNode->LinkedNodes[(int)AINBFile::LinkedNodeMapping::StandardLink][i].NodeIndex >= Nodes.size())
            continue;
        
        uint32_t StartPinId = mNode->LinkedNodes[(int)AINBFile::LinkedNodeMapping::StandardLink][i].Condition == "True" ? mPinIdTrue : mPinIdFalse;

        ed::Link(CurrentLinkId++, StartPinId, Nodes[mNode->LinkedNodes[(int)AINBFile::LinkedNodeMapping::StandardLink][i].NodeIndex]->mEditorId + 1, GetValueTypeColor(6));
        mLinks.insert({ CurrentLinkId - 1, Link { .ObjectPtr = mNode, .Type = LinkType::Flow, .ParameterIndex = (uint16_t)i } });
        Nodes[mNode->LinkedNodes[(int)AINBFile::LinkedNodeMapping::StandardLink][i].NodeIndex]->mFlowLinked = true;
    }

    for (uint8_t i = 0; i < AINBFile::ValueTypeCount; i++) {
        for (uint16_t j = 0; j < mNode->InputParameters[i].size(); j++) {
            AINBFile::InputEntry& Input = mNode->InputParameters[i][j];
            if (Input.NodeIndex >= 0) { // Single link
                ed::Link(CurrentLinkId++, Nodes[Input.NodeIndex]->mOutputParameters[i][Input.ParameterIndex], mInputParameters[i][j], GetValueTypeColor(i));
                mLinks.insert({ CurrentLinkId - 1, Link { .ObjectPtr = &Input, .Type = LinkType::Parameter, .NodeIndex = (uint16_t)Input.NodeIndex, .ParameterIndex = (uint16_t)Input.ParameterIndex } });
            }
            for (AINBFile::MultiEntry& Entry : Input.Sources) { // Multi link
                ed::Link(CurrentLinkId++, Nodes[Entry.NodeIndex]->mOutputParameters[i][Entry.ParameterIndex], mInputParameters[i][j], GetValueTypeColor(i));
                mLinks.insert({ CurrentLinkId - 1, Link { .ObjectPtr = &Input, .Type = LinkType::Parameter, .NodeIndex = Entry.NodeIndex, .ParameterIndex = Entry.ParameterIndex } });
            }
        }
    }
}

ImColor UIAINBNodeBoolSelector::GetHeaderColor()
{
    return ImColor(120, 20, 20);
}