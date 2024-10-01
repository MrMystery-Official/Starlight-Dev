#include "UIEventEditorEntryPoint.h"

#include "imgui_node_editor.h"
#include "imgui.h"
#include "imgui_stdlib.h"

#include "Editor.h"
#include "UIEventEditor.h"

UIEventEditorEntryPoint::UIEventEditorEntryPoint(std::pair<std::string, BFEVFLFile::EntryPoint>& EntryPoint, uint32_t EditorId) : UIEventEditorNodeBase(EditorId), mEntryPoint(&EntryPoint)
{
}

ImColor UIEventEditorEntryPoint::GetHeaderColor()
{
    return ImColor(80, 15, 80);
}

void UIEventEditorEntryPoint::Render()
{
    uint32_t CurrentId = mEditorId;

    // Node body
    ed::PushStyleVar(ed::StyleVar_NodePadding, ImVec4(8, 8, 8, 8));
    ed::BeginNode(CurrentId++);

    // Node header (-> NodeName)
    //DrawPinIcon(CurrentId++, false);
    /*
	float Alpha = ImGui::GetStyle().Alpha;
	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, Alpha);
	ImRect HeaderRect = DrawPin(CurrentId++, false, Alpha * 255, ed::PinKind::Output, mDisplayName);
	ImGui::PopStyleVar();
    */

    uint32_t FrameWidth = ImGui::CalcTextSize("EntryPoint").x + 2 * 8 + 24;

    ImGui::Text("EntryPoint");
    mNodeShapeInfo.HeaderMin = ImVec2(ImGui::GetItemRectMin().x - 1 + ((Editor::UIScale - 1) * 10) - ImGui::GetStyle().ItemSpacing.x, ImGui::GetItemRectMin().y - 9);
    mNodeShapeInfo.HeaderMax = ImVec2(mNodeShapeInfo.HeaderMin.x + FrameWidth + 2 + ((Editor::UIScale - 1) * 14), ImGui::GetItemRectMax().y + 8);

    ImGui::Dummy(ImVec2(0, 8));

    ImGui::Text("Name");
    ImGui::SameLine();
    ImGui::PushItemWidth(ImGui::CalcTextSize(mEntryPoint->first.c_str()).x + 2 * ImGui::GetStyle().ItemInnerSpacing.x);
    ImGui::InputText(("##EntryPointName" + std::to_string(CurrentId)).c_str(), &mEntryPoint->first);
    ImGui::PopItemWidth();
    FrameWidth = std::max(FrameWidth, (uint32_t)(16 + ImGui::CalcTextSize("Name").x + ImGui::GetStyle().ItemSpacing.x + ImGui::CalcTextSize(mEntryPoint->first.c_str()).x + 2 * ImGui::GetStyle().ItemInnerSpacing.x));
    mNodeShapeInfo.HeaderMax.x = mNodeShapeInfo.HeaderMin.x + FrameWidth + 2 + ((Editor::UIScale - 1) * 14);

    ImGui::SetCursorPos(ImVec2(mNodeShapeInfo.HeaderMax.x - 24 - 8, (mNodeShapeInfo.HeaderMax.y - mNodeShapeInfo.HeaderMin.y) / 2 - 12 + mNodeShapeInfo.HeaderMin.y));
    DrawPin(CurrentId++, mEntryPoint->second.mEventIndex >= 0, 255, ed::PinKind::Output);
    mOutputPinId = CurrentId - 1;

    ed::EndNode();
    ed::PopStyleVar();

    mNodeShapeInfo.HeaderMax.x += 8;

    DrawHeader();

    mHasParentNode = false;
}

void UIEventEditorEntryPoint::DrawLinks()
{
    uint32_t CurrentLinkId = mEditorId + 500;
    if (mEntryPoint->second.mEventIndex >= 0)
    {
        std::unique_ptr<UIEventEditorNodeBase>& Node = UIEventEditor::mNodes[UIEventEditor::mEventFile.mFlowcharts[0].mEntryPoints.mItems.size() + mEntryPoint->second.mEventIndex];
        ed::Link(CurrentLinkId++, mOutputPinId, Node->mInputPinId, ImColor(1.0f, 1.0f, 1.0f, 1.0f));
        Node->mHasParentNode = true;
    }
}

void UIEventEditorEntryPoint::DeleteOutputLink(uint32_t LinkId)
{
    mEntryPoint->second.mEventIndex = -1;
}

void UIEventEditorEntryPoint::CreateLink(uint32_t SourceLinkId, uint32_t DestinationLinkId)
{
    uint32_t EditorId = DestinationLinkId;
    EditorId /= 1000;
    EditorId *= 1000;

    int32_t NodeIndex = -1;
    for (size_t i = 0; i < UIEventEditor::mNodes.size(); i++)
    {
        if (UIEventEditor::mNodes[i]->mEditorId == EditorId)
        {
            NodeIndex = i;
            break;
        }
    }

    if (NodeIndex == -1)
    {
        Logger::Error("UIEventEditorEntryPoint", "Destination node index was -1");
        return;
    }

    NodeIndex -= UIEventEditor::mEventFile.mFlowcharts[0].mEntryPoints.mItems.size();

    mEntryPoint->second.mEventIndex = NodeIndex;
}

bool UIEventEditorEntryPoint::IsPinLinked(uint32_t PinId)
{
    return mEntryPoint->second.mEventIndex >= 0;
}

UIEventEditorNodeBase::NodeType UIEventEditorEntryPoint::GetNodeType()
{
	return UIEventEditorNodeBase::NodeType::EntryPoint;
}