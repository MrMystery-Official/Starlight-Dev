#include "UIEventEditorJoin.h"

#include "imgui_node_editor.h"
#include "imgui.h"
#include "imgui_stdlib.h"

#include "Editor.h"
#include "UIEventEditor.h"

UIEventEditorJoin::UIEventEditorJoin(BFEVFLFile::JoinEvent& Join, uint32_t EditorId) : UIEventEditorNodeBase(EditorId), mJoin(&Join)
{
}

ImColor UIEventEditorJoin::GetHeaderColor()
{
    return ImColor(130, 130, 130);
}

void UIEventEditorJoin::Render()
{
    uint32_t CurrentId = mEditorId;

    // Node body
    ed::PushStyleVar(ed::StyleVar_NodePadding, ImVec4(8, 8, 8, 8));
    ed::BeginNode(CurrentId++);

    uint32_t FrameWidth = ImGui::CalcTextSize("Join").x + 2 * 8 + 48 + ImGui::GetStyle().ItemSpacing.x;

    ImGui::Dummy(ImVec2(24, 8));
    ImGui::SameLine();
    ImGui::Text("Join");
    mNodeShapeInfo.HeaderMin = ImVec2(ImGui::GetItemRectMin().x - 1 + ((Editor::UIScale - 1) * 10) - ImGui::GetStyle().ItemSpacing.x - 24 - 8, ImGui::GetItemRectMin().y - 9);
    mNodeShapeInfo.HeaderMax = ImVec2(mNodeShapeInfo.HeaderMin.x + FrameWidth + 2 + 8 + ((Editor::UIScale - 1) * 14), ImGui::GetItemRectMax().y + 8);

    ImGui::Dummy(ImVec2(0, 8));

    ImGui::SetCursorPos(ImVec2(mNodeShapeInfo.HeaderMax.x - 32 - 8, (mNodeShapeInfo.HeaderMax.y - mNodeShapeInfo.HeaderMin.y) / 2 - 12 + mNodeShapeInfo.HeaderMin.y));
    DrawPin(CurrentId++, mJoin->mNextEventIndex >= 0, 255, ed::PinKind::Output);
    mOutputPinId = CurrentId - 1;

    ImGui::SetCursorPos(ImVec2(mNodeShapeInfo.HeaderMin.x + 8, (mNodeShapeInfo.HeaderMax.y - mNodeShapeInfo.HeaderMin.y) / 2 - 12 + mNodeShapeInfo.HeaderMin.y));
    DrawPin(CurrentId++, mHasParentNode, 255, ed::PinKind::Input);
    mInputPinId = CurrentId - 1;

    ed::EndNode();
    ed::PopStyleVar();

    DrawHeader();

    mHasParentNode = false;
}

void UIEventEditorJoin::DrawLinks()
{
    uint32_t CurrentLinkId = mEditorId + 500;
    if (mJoin->mNextEventIndex >= 0)
    {
        std::unique_ptr<UIEventEditorNodeBase>& Node = UIEventEditor::mNodes[UIEventEditor::mEventFile.mFlowcharts[0].mEntryPoints.mItems.size() + mJoin->mNextEventIndex];
        ed::Link(CurrentLinkId++, mOutputPinId, Node->mInputPinId, ImColor(1.0f, 1.0f, 1.0f, 1.0f));
        Node->mHasParentNode = true;
    }
}

void UIEventEditorJoin::DeleteOutputLink(uint32_t LinkId)
{
    mJoin->mNextEventIndex = -1;
}

void UIEventEditorJoin::CreateLink(uint32_t SourceLinkId, uint32_t DestinationLinkId)
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
        Logger::Error("UIEventEditorJoin", "Destination node index was -1");
        return;
    }

    NodeIndex -= UIEventEditor::mEventFile.mFlowcharts[0].mEntryPoints.mItems.size();

    mJoin->mNextEventIndex = NodeIndex;
}

bool UIEventEditorJoin::IsPinLinked(uint32_t PinId)
{
    if (mOutputPinId == PinId)
        return mJoin->mNextEventIndex >= 0;

    if (mInputPinId == PinId)
        return mHasParentNode;

    return false;
}

UIEventEditorNodeBase::NodeType UIEventEditorJoin::GetNodeType()
{
    return UIEventEditorNodeBase::NodeType::Join;
}