#include "UIAINBNodeBase.h"

#include "imgui_stdlib.h"
#include "Vector3F.h"

UIAINBNodeBase::UIAINBNodeBase(AINBFile::Node* Node, uint32_t EditorId, TextureMgr::Texture& HeaderBackground, bool& EnableFlow)
    : mNode(Node)
    , mEditorId(EditorId)
    , mHeaderBackground(HeaderBackground)
    , mEnableFlow(EnableFlow)
{
    mOutputParameters.resize(6);
    mInputParameters.resize(6);

    mLinkedOutputParams.resize(6);
}

void UIAINBNodeBase::DrawNodeHeader()
{
    if (ImGui::IsItemVisible()) {
        int Alpha = ImGui::GetStyle().Alpha;
        ImColor HeaderColor = GetHeaderColor();
        HeaderColor.Value.w = Alpha;

        ImDrawList* DrawList = ed::GetNodeBackgroundDrawList(mEditorId);

        float BorderWidth = ed::GetStyle().NodeBorderWidth;

        mNodeShapeInfo.HeaderMin.x += BorderWidth;
        mNodeShapeInfo.HeaderMin.y += BorderWidth;
        mNodeShapeInfo.HeaderMax.x -= BorderWidth;
        mNodeShapeInfo.HeaderMax.y -= BorderWidth;

        /*
        float BorderWidth = ed::GetStyle().NodeBorderWidth;

        mNodeShapeInfo.HeaderMin.x += BorderWidth;
        mNodeShapeInfo.HeaderMin.y += BorderWidth;
        mNodeShapeInfo.HeaderMax.x -= BorderWidth;
        mNodeShapeInfo.HeaderMax.y -= BorderWidth;

        DrawList->AddRectFilled(mNodeShapeInfo.HeaderMin, mNodeShapeInfo.HeaderMax, HeaderColor, ed::GetStyle().NodeRounding,
            ImDrawFlags_RoundCornersTopLeft | ImDrawFlags_RoundCornersTopRight);

        ImVec2 HeaderSeparatorLeft = ImVec2(mNodeShapeInfo.HeaderMin.x - BorderWidth / 2, mNodeShapeInfo.HeaderMax.y - 0.5f);
        ImVec2 HeaderSeparatorRight = ImVec2(mNodeShapeInfo.HeaderMax.x, mNodeShapeInfo.HeaderMax.y - 0.5f);

        DrawList->AddLine(HeaderSeparatorLeft, HeaderSeparatorRight, ImColor(255, 255, 255, (int)(Alpha * 255 / 2)), BorderWidth);
        */
        
        const auto HalfBorderWidth = ed::GetStyle().NodeBorderWidth * 0.5f;

        const auto uv = ImVec2(
            (mNodeShapeInfo.HeaderMax.x - mNodeShapeInfo.HeaderMin.x) / (float)(4.0f * mHeaderBackground.Width),
            (mNodeShapeInfo.HeaderMax.y - mNodeShapeInfo.HeaderMin.y) / (float)(4.0f * mHeaderBackground.Height));

        DrawList->AddImageRounded((ImTextureID)mHeaderBackground.ID,
            mNodeShapeInfo.HeaderMin,
            mNodeShapeInfo.HeaderMax,
            ImVec2(0.0f, 0.0f), uv,
#if IMGUI_VERSION_NUM > 18101
                        HeaderColor, ed::GetStyle().NodeRounding, ImDrawFlags_RoundCornersTop);
#else
                        HeaderColor, ed::GetStyle().NodeRounding, 1 | 2);
#endif

        ImGuiTextBuffer Builder;
        Builder.appendf("#%d", mNode->NodeIndex);

        auto TextSize = ImGui::CalcTextSize(Builder.c_str());
        auto Padding = ImVec2(2.0f, 2.0f);
        auto WidgetSize = ImVec2(TextSize.x + Padding.x * 2, TextSize.y + Padding.y * 2);

        auto WidgetPosition = ImVec2(mNodeShapeInfo.HeaderMax.x + Padding.x, mNodeShapeInfo.HeaderMin.y - Padding.y - WidgetSize.y);

        DrawList->AddRectFilled(WidgetPosition, ImVec2(WidgetPosition.x + WidgetSize.x, WidgetPosition.y + WidgetSize.y), IM_COL32(100, 80, 80, 190), 3.0f, ImDrawFlags_RoundCornersAll);
        DrawList->AddRect(WidgetPosition, ImVec2(WidgetPosition.x + WidgetSize.x, WidgetPosition.y + WidgetSize.y), IM_COL32(200, 160, 160, 190), 3.0f, ImDrawFlags_RoundCornersAll);
        DrawList->AddText(ImVec2(WidgetPosition.x + Padding.x, WidgetPosition.y + Padding.y), IM_COL32(255, 255, 255, 255), Builder.c_str());
    }
}

void UIAINBNodeBase::DrawImmediateSeperator(ImVec2 Pos)
{
    if (ImGui::IsItemVisible()) {
        float BorderWidth = ed::GetStyle().NodeBorderWidth;

        ImGui::SetCursorPos(Pos);

        ImGui::Text("Node settings");

        ImVec2 HeaderSeparatorLeft = ImVec2(mNodeShapeInfo.HeaderMin.x - BorderWidth / 2, ImGui::GetCursorPosY() - 2.5f);
        ImVec2 HeaderSeparatorRight = ImVec2(mNodeShapeInfo.HeaderMax.x, ImGui::GetCursorPosY() - 1.5f);

        ed::GetNodeBackgroundDrawList(mEditorId)->AddLine(HeaderSeparatorLeft, HeaderSeparatorRight, ImColor(255, 255, 255, (int)(ImGui::GetStyle().Alpha * 255 / 2)), BorderWidth);
    }
}

void UIAINBNodeBase::DrawPinIcon(bool Connected, uint32_t Alpha, PinType Type)
{
    ax::Drawing::IconType PinIconType;
    ImColor Color = GetPinTypeColor(Type);
    Color.Value.w = Alpha / 255.0f;
    switch (Type) {
    case PinType::Flow:
        PinIconType = ax::Drawing::IconType::Flow;
        break;
    case PinType::Bool:
    case PinType::Int:
    case PinType::Float:
    case PinType::String:
    case PinType::Vec3f:
    case PinType::UserDefined:
        PinIconType = ax::Drawing::IconType::Circle;
        break;
    default:
        return;
    }

    ax::Widgets::Icon(ImVec2(24.0f, 24.0f), PinIconType, Connected, Color, ImColor(32, 32, 32, Alpha));
}

ImRect UIAINBNodeBase::DrawPin(uint32_t Id, bool Connected, uint32_t Alpha, PinType Type, ed::PinKind Kind, std::string Name)
{
    ImVec2 IconPos;
    ImRect InputsRect;

    if (Kind == ed::PinKind::Input) {
        DrawPinIcon(Connected, Alpha, Type);
        IconPos = ImGui::GetItemRectMin();
        ImGui::SameLine();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5.0f);
        ImGui::TextUnformatted(Name.c_str());
    } else if (Kind == ed::PinKind::Output) {
        ImGui::TextUnformatted(Name.c_str());
        InputsRect = ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
        ImGui::SameLine();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 5.0f);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 5.0f);
        DrawPinIcon(Connected, Alpha, Type);
        IconPos = ImGui::GetItemRectMin();
    }

    ImRect HeaderPos = ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
    if (Kind == ed::PinKind::Input) {
        InputsRect = HeaderPos;

        InputsRect.Min.x -= ImGui::GetStyle().ItemSpacing.x + 24;
        InputsRect.Min.y -= 6;

        InputsRect.Max.x += 2;
        InputsRect.Max.y += 6;
    } else if (Kind == ed::PinKind::Output) {
        InputsRect.Max.x += ImGui::GetStyle().ItemSpacing.x + 24;
        InputsRect.Max.y += 6;

        InputsRect.Min.x -= 2;
        InputsRect.Min.y -= 6;
    }

    IconPos.y += 12;
    IconPos.x += 8;

    ImGui::SameLine();

    ed::BeginPin(Id, Kind);
    ed::PinPivotRect(IconPos, IconPos);
    ed::PinRect(InputsRect.GetTL(), InputsRect.GetBR());
    ed::EndPin();

    return HeaderPos;
}

void UIAINBNodeBase::DrawParameterValue(AINBFile::ValueType Type, std::string Name, uint32_t Id, void* Dest)
{
    switch (Type) {
    case AINBFile::ValueType::Int:
        ImGui::PushItemWidth(mMinTextBoxWidth);
        ImGui::InputScalar(("##" + Name + std::to_string(Id)).c_str(), ImGuiDataType_U32, Dest);
        ImGui::PopItemWidth();
        break;
    case AINBFile::ValueType::Float:
        ImGui::PushItemWidth(mMinTextBoxWidth);
        ImGui::InputScalar(("##" + Name + std::to_string(Id)).c_str(), ImGuiDataType_Float, Dest);
        ImGui::PopItemWidth();
        break;
    case AINBFile::ValueType::Bool:
        ImGui::Checkbox(("##" + Name + std::to_string(Id)).c_str(), (bool*)Dest);
        break;
    case AINBFile::ValueType::String:
        ImGui::PushItemWidth(mMinTextBoxWidth);
        ImGui::InputText(("##" + Name + std::to_string(Id)).c_str(), (std::string*)Dest);
        ImGui::PopItemWidth();
        break;
    case AINBFile::ValueType::Vec3f:
        ImGui::InputScalarNWidth(("##" + Name + std::to_string(Id)).c_str(), ImGuiDataType_Float, ((Vector3F*)Dest)->GetRawData(), 3, mMinTextBoxWidth * 3);
        break;
    case AINBFile::ValueType::UserDefined:
        ImGui::NewLine();
        break;
    }
}

std::string UIAINBNodeBase::GetValueTypeName(AINBFile::ValueType ValueType)
{
    return GetValueTypeName((int)ValueType);
}

std::string UIAINBNodeBase::GetValueTypeName(uint8_t ValueType)
{
    switch (ValueType) {
    case 0:
        return "Int";
    case 1:
        return "Bool";
    case 2:
        return "Float";
    case 3:
        return "String";
    case 4:
        return "Vec3f";
    case 5:
        return "UserDefined";
    default:
        return "Unknown";
    }
}

float UIAINBNodeBase::GetParamValueWidth(AINBFile::ValueType ValueType)
{
    return GetParamValueWidth((int)ValueType);
}

float UIAINBNodeBase::GetParamValueWidth(uint8_t ValueType)
{
    switch (ValueType) {
    case 0:
    case 2:
    case 3:
        return mMinTextBoxWidth;
    case 1:
        return ImGui::GetFrameHeight();
    case 4:
        return mMinTextBoxWidth * 3;
    case 5:
        return 0.0f;
    }
}

ImColor UIAINBNodeBase::GetValueTypeColor(AINBFile::ValueType ValueType)
{
    return GetValueTypeColor((int)ValueType);
}

ImColor UIAINBNodeBase::GetPinTypeColor(PinType Type)
{
    return GetValueTypeColor((int)Type);
}

ImColor UIAINBNodeBase::GetValueTypeColor(uint8_t ValueType)
{
    switch (ValueType) {
    case 0:
        return ImColor(34, 215, 168);
    case 1:
        return ImColor(0, 163, 234);
    case 2:
        return ImColor(162, 250, 84);
    case 3:
        return ImColor(247, 0, 206);
    case 4:
        return ImColor(247, 195, 33);
    case 5:
        return ImColor(195, 124, 243);
    case 6:
        return ImColor(255, 255, 255);
    }
}

UIAINBNodeBase::PinType UIAINBNodeBase::ValueTypeToPinType(AINBFile::ValueType Type)
{
    return ValueTypeToPinType((int)Type);
}

UIAINBNodeBase::PinType UIAINBNodeBase::ValueTypeToPinType(uint8_t Type)
{
    switch (Type) {
    case 0:
        return PinType::Int;
    case 1:
        return PinType::Bool;
    case 2:
        return PinType::Float;
    case 3:
        return PinType::String;
    case 4:
        return PinType::Vec3f;
    case 5:
        return PinType::UserDefined;
    case 6:
        return PinType::Flow; //Should never be the case
    default:
        return PinType::Int;
    }
}

void UIAINBNodeBase::ClearParameterIds()
{
    for (uint8_t i = 0; i < AINBFile::ValueTypeCount; i++) {
        mOutputParameters[i].clear();
        mInputParameters[i].clear();
        mLinkedOutputParams[i].clear();
    }
    mPins.clear();
    mLinks.clear();
}

UIAINBNodeBase::NodeType UIAINBNodeBase::GetNodeType()
{
    return NodeType::Default;
}