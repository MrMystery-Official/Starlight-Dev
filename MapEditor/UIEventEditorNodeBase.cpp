#include "UIEventEditorNodeBase.h"

#include "UIAINBEditor.h"
#include "UIEventEditor.h"

void UIEventEditorNodeBase::DrawHeader()
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

        const auto HalfBorderWidth = ed::GetStyle().NodeBorderWidth * 0.5f;

        const auto uv = ImVec2(
            (mNodeShapeInfo.HeaderMax.x - mNodeShapeInfo.HeaderMin.x) / (float)(4.0f * UIAINBEditor::HeaderTexture->Width),
            (mNodeShapeInfo.HeaderMax.y - mNodeShapeInfo.HeaderMin.y) / (float)(4.0f * UIAINBEditor::HeaderTexture->Height));

        DrawList->AddImageRounded((ImTextureID)UIAINBEditor::HeaderTexture->ID,
            mNodeShapeInfo.HeaderMin,
            mNodeShapeInfo.HeaderMax,
            ImVec2(0.0f, 0.0f), uv,
#if IMGUI_VERSION_NUM > 18101
            HeaderColor, ed::GetStyle().NodeRounding, ImDrawFlags_RoundCornersTop);
#else
            HeaderColor, ed::GetStyle().NodeRounding, 1 | 2);
#endif
    }
}

void UIEventEditorNodeBase::DrawPinIcon(bool Connected, uint32_t Alpha, bool Input)
{
    ax::Widgets::Icon(ImVec2(24.0f, 24.0f), ax::Drawing::IconType::Flow, Connected, ImColor(255, 255, 255), ImColor(32, 32, 32, Alpha));
}

void UIEventEditorNodeBase::DrawPin(uint32_t Id, bool Connected, uint32_t Alpha, ed::PinKind Kind)
{
    ImVec2 IconPos;
    ImRect InputsRect;

    if (Kind == ed::PinKind::Input) {
        DrawPinIcon(Connected, Alpha, true);
        IconPos = ImGui::GetItemRectMin();
    }
    else if (Kind == ed::PinKind::Output)
    {
        DrawPinIcon(Connected, Alpha, false);
        IconPos = ImGui::GetItemRectMin();
    }

    ImRect HeaderPos = ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());

    IconPos.y += 12;
    IconPos.x += 8;

    ImGui::SameLine();

    ed::BeginPin(Id, Kind);
    ed::PinPivotRect(IconPos, IconPos);
    ed::PinRect(HeaderPos.GetTL(), HeaderPos.GetBR());
    ed::EndPin();
}

std::unique_ptr<UIEventEditorNodeBase>& UIEventEditorNodeBase::GetNodeByLinkId(uint32_t PinId)
{
    uint32_t EditorId = ((int)((PinId - 500) / 1000)) * 1000;
    for (std::unique_ptr<UIEventEditorNodeBase>& Node : UIEventEditor::mNodes)
    {
        std::cout << Node->mEditorId << ", " << EditorId << std::endl;
        if (Node->mEditorId == EditorId)
            return Node;
    }
}

std::unique_ptr<UIEventEditorNodeBase>& UIEventEditorNodeBase::GetNodeByPinId(uint32_t PinId)
{
    uint32_t EditorId = ((int)(PinId / 1000)) * 1000;
    for (std::unique_ptr<UIEventEditorNodeBase>& Node : UIEventEditor::mNodes)
    {
        if (Node->mEditorId == EditorId)
            return Node;
    }
}