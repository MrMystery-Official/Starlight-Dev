#include "UIAINBNodeS32Selector.h"

#include "PopupAINBElementSelector.h"
#include "Editor.h"

UIAINBNodeS32Selector::UIAINBNodeS32Selector(AINBFile::Node& Node, uint32_t EditorId, TextureMgr::Texture& HeaderBackground, bool& EnableFlow)
    : UIAINBNodeBase(&Node, EditorId, HeaderBackground, EnableFlow)
{
    GenerateNodeShapeInfo();

    for (AINBFile::LinkedNodeInfo& Info : Node.LinkedNodes[(int)AINBFile::LinkedNodeMapping::StandardLink]) {
        if (Info.Condition == "Default")
            continue;

        mConditions.push_back(std::stoi(Info.Condition));
    }
}

void UIAINBNodeS32Selector::RebuildNode()
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
    Param.Value = (uint32_t)0;
    Param.ValueType = (int)AINBFile::ValueType::Int;
    Param.Name = "Input";
    Param.NodeIndex = -1;
    Param.ParameterIndex = -1;
    mNode->InputParameters[(int)AINBFile::ValueType::Int].push_back(Param);

    mConditions.clear();
}

void UIAINBNodeS32Selector::GenerateNodeShapeInfo()
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

void UIAINBNodeS32Selector::Render()
{
    mPinIds.clear();

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

    bool HasDefaultLink = false;
    std::vector<uint32_t> HasLink; //Condition -> true
    for (AINBFile::LinkedNodeInfo& Info : mNode->LinkedNodes[(int)AINBFile::LinkedNodeMapping::StandardLink]) {
        if (Info.Condition == "Default") {
            HasDefaultLink = true;
            continue;
        }
        
        HasLink.push_back(std::stoi(Info.Condition));
    }

    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + mNodeShapeInfo.FrameWidth - (8 + ImGui::CalcTextSize("Default").x + 18 + ImGui::GetStyle().ItemSpacing.x));
    float Alpha = ImGui::GetStyle().Alpha;
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, Alpha);
    DrawPin(CurrentId++, HasDefaultLink, Alpha * 255, UIAINBNodeBase::PinType::Flow, ed::PinKind::Output, "Default");
    mPins.insert({ CurrentId - 1, UIAINBNodeBase::Pin { .Kind = ed::PinKind::Output, .Type = UIAINBNodeBase::PinType::Flow, .Node = mNode, .ObjectPtr = (void*)0x1, .ParameterIndex = 0, .AllowMultipleLinks = false, .AlreadyLinked = HasDefaultLink } });
    mPinIdDefault = CurrentId - 1;

    for (auto Iter = mConditions.begin(); Iter != mConditions.end(); ) {
        bool Linked = std::find(HasLink.begin(), HasLink.end(), *Iter) != HasLink.end();
        std::string Text = "= " + std::to_string(*Iter);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + mNodeShapeInfo.FrameWidth - (8 + ImGui::CalcTextSize(Text.c_str()).x + 18 + 2 * ImGui::GetStyle().ItemSpacing.x + ImGui::GetFrameHeight()));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.47f, 0.07f, 0.07f, 1.0f));
        bool WantRemove = ImGui::Button(("-##S32Selector" + std::to_string(mNode->NodeIndex) + ";" + std::to_string(*Iter)).c_str(), ImVec2(ImGui::GetFrameHeight(), 0));
        ImGui::PopStyleColor();
        ImGui::SameLine();
        DrawPin(CurrentId++, Linked, Alpha * 255, UIAINBNodeBase::PinType::Flow, ed::PinKind::Output, Text);
        mPins.insert({ CurrentId - 1, UIAINBNodeBase::Pin { .Kind = ed::PinKind::Output, .Type = UIAINBNodeBase::PinType::Flow, .Node = mNode, .ParameterIndex = *Iter, .AllowMultipleLinks = false, .AlreadyLinked = Linked } });
        mPinIds.insert({ *Iter, CurrentId - 1 });
        if (WantRemove) {
            for (auto LinkIter = mNode->LinkedNodes[(int)AINBFile::LinkedNodeMapping::StandardLink].begin(); LinkIter != mNode->LinkedNodes[(int)AINBFile::LinkedNodeMapping::StandardLink].end();) {
                if (LinkIter->Condition == std::to_string(*Iter)) {
                    mNode->LinkedNodes[(int)AINBFile::LinkedNodeMapping::StandardLink].erase(LinkIter);
                    break;
                }
                LinkIter++;
            }

            Iter = mConditions.erase(Iter);
        } else {
            Iter++;
        }
    }

    ImGui::PopStyleVar();

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
    if (ImGui::Button(("+##S32Selector" + std::to_string(mNode->NodeIndex)).c_str(), ImVec2(ImGui::GetFrameHeight(), 0))) {
        PopupAINBElementSelector::Open("Condition: signed 32-bit number", "signed 32-bit number", "Add condition", this, [](void* ThisPtr, std::string Number) {
            uint32_t Num = std::stoi(Number);
            if (std::find(((UIAINBNodeS32Selector*)ThisPtr)->mConditions.begin(), ((UIAINBNodeS32Selector*)ThisPtr)->mConditions.end(), Num) != ((UIAINBNodeS32Selector*)ThisPtr)->mConditions.end())
                return;

            ((UIAINBNodeS32Selector*)ThisPtr)->mConditions.push_back(Num);
        });
    }
    ImGui::PopStyleColor();

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

void UIAINBNodeS32Selector::PostProcessLinkedNodeInfo(Pin& StartPin, AINBFile::LinkedNodeInfo& Info)
{
    if (StartPin.ObjectPtr == (void*)0x1) {
        Info.Condition = "Default";
    } else {
        Info.Condition = std::to_string(StartPin.ParameterIndex);
    }
}

void UIAINBNodeS32Selector::RenderLinks(std::vector<std::unique_ptr<UIAINBNodeBase>>& Nodes)
{
    uint32_t CurrentLinkId = mEditorId + 500; // Link start at +500
    for (int i = 0; i < mNode->LinkedNodes[(int)AINBFile::LinkedNodeMapping::StandardLink].size(); i++) {
        if (mNode->LinkedNodes[(int)AINBFile::LinkedNodeMapping::StandardLink][i].NodeIndex == -1 || mNode->LinkedNodes[(int)AINBFile::LinkedNodeMapping::StandardLink][i].NodeIndex >= Nodes.size())
            continue;

        uint32_t StartPinId = mNode->LinkedNodes[(int)AINBFile::LinkedNodeMapping::StandardLink][i].Condition == "Default" ? mPinIdDefault : mPinIds[std::stoi(mNode->LinkedNodes[(int)AINBFile::LinkedNodeMapping::StandardLink][i].Condition)];

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

ImColor UIAINBNodeS32Selector::GetHeaderColor()
{
    return ImColor(120, 20, 20);
}