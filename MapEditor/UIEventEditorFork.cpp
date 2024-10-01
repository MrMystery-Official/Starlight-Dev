#include "UIEventEditorFork.h"

#include "imgui_node_editor.h"
#include "imgui.h"
#include "imgui_stdlib.h"

#include "Editor.h"
#include "UIEventEditor.h"

#include "UIEventEditorAction.h"
#include "UIEventEditorSubFlow.h"
#include "UIEventEditorJoin.h"
#include "UIEventEditorSwitch.h"

UIEventEditorFork::UIEventEditorFork(BFEVFLFile::ForkEvent& Fork, uint32_t EditorId) : UIEventEditorNodeBase(EditorId), mFork(&Fork)
{
}

ImColor UIEventEditorFork::GetHeaderColor()
{
    return ImColor(130, 130, 130);
}

void UIEventEditorFork::Render()
{
    uint32_t CurrentId = mEditorId;

    // Node body
    ed::PushStyleVar(ed::StyleVar_NodePadding, ImVec4(8, 8, 8, 8));
    ed::BeginNode(CurrentId++);

    uint32_t FrameWidth = ImGui::CalcTextSize("Fork").x + 2 * 8 + 48 + ImGui::GetStyle().ItemSpacing.x;

    ImGui::Dummy(ImVec2(24, 8));
    ImGui::SameLine();
    ImGui::Text("Fork");
    mNodeShapeInfo.HeaderMin = ImVec2(ImGui::GetItemRectMin().x - 1 + ((Editor::UIScale - 1) * 10) - ImGui::GetStyle().ItemSpacing.x - 24 - 8, ImGui::GetItemRectMin().y - 9);
    mNodeShapeInfo.HeaderMax = ImVec2(mNodeShapeInfo.HeaderMin.x + FrameWidth + 2 + 8 + ((Editor::UIScale - 1) * 14), ImGui::GetItemRectMax().y + 8);

    ImGui::Dummy(ImVec2(0, 8));

    ImGui::SetCursorPos(ImVec2(mNodeShapeInfo.HeaderMax.x - 32 - 8, (mNodeShapeInfo.HeaderMax.y - mNodeShapeInfo.HeaderMin.y) / 2 - 12 + mNodeShapeInfo.HeaderMin.y));
    DrawPin(CurrentId++, !mFork->mForkEventIndicies.empty(), 255, ed::PinKind::Output);
    mOutputPinId = CurrentId - 1;

    ImGui::SetCursorPos(ImVec2(mNodeShapeInfo.HeaderMin.x + 8, (mNodeShapeInfo.HeaderMax.y - mNodeShapeInfo.HeaderMin.y) / 2 - 12 + mNodeShapeInfo.HeaderMin.y));
    DrawPin(CurrentId++, mHasParentNode, 255, ed::PinKind::Input);
    mInputPinId = CurrentId - 1;

    ed::EndNode();
    ed::PopStyleVar();

    DrawHeader();

    UIEventEditorJoin* Event = static_cast<UIEventEditorJoin*>(UIEventEditor::mNodes[UIEventEditor::mEventFile.mFlowcharts[0].mEntryPoints.mItems.size() + mFork->mJoinEventIndex].get());
    Event->mForkParentIndex = (mEditorId / 1000) - 1 - UIEventEditor::mEventFile.mFlowcharts[0].mEntryPoints.mItems.size();

    mHasParentNode = false;
}

std::vector<int16_t> UIEventEditorFork::GetLastEventIndices(int16_t Index)
{
    std::unique_ptr<UIEventEditorNodeBase>& Event = UIEventEditor::mNodes[UIEventEditor::mEventFile.mFlowcharts[0].mEntryPoints.mItems.size() + Index];
    std::vector<int16_t> Result;
    switch (Event->GetNodeType())
    {
    case UIEventEditorNodeBase::NodeType::Action:
        Result.push_back(static_cast<UIEventEditorAction*>(Event.get())->mAction->mNextEventIndex);
        break;
    case UIEventEditorNodeBase::NodeType::SubFlow:
        Result.push_back(static_cast<UIEventEditorSubFlow*>(Event.get())->mSubFlow->mNextEventIndex);
        break;
    case UIEventEditorNodeBase::NodeType::Switch:
    {
        UIEventEditorSwitch* Switch = static_cast<UIEventEditorSwitch*>(Event.get());
        for (BFEVFLFile::SwitchEvent::Case& Case : Switch->mSwitch->mSwitchCases)
        {
            Result.push_back(Case.mEventIndex);
        }
        break;
    }
    default:
        break;
    }

    if (!Result.empty())
    {
        std::vector<int16_t> NewResults;
        for (int16_t NextIndex : Result)
        {
            if (NextIndex == -1)
                continue;

            std::vector<int16_t> Ret = GetLastEventIndices(NextIndex);
            NewResults.insert(NewResults.end(), Ret.begin(), Ret.end());
        }

        Result.clear();
        Result.push_back(Index);

        return NewResults.empty() ? Result : NewResults;
    }

    return Result;
}

void UIEventEditorFork::DrawLinks()
{
    uint32_t CurrentLinkId = mEditorId + 500;
    mOutputPinIds.clear();
    for (int16_t& Index : mFork->mForkEventIndicies)
    {
        std::unique_ptr<UIEventEditorNodeBase>& Node = UIEventEditor::mNodes[UIEventEditor::mEventFile.mFlowcharts[0].mEntryPoints.mItems.size() + Index];
        ed::Link(CurrentLinkId++, mOutputPinId, Node->mInputPinId, ImColor(1.0f, 1.0f, 1.0f, 1.0f));
        Node->mHasParentNode = true;
        mOutputPinIds.push_back(CurrentLinkId - 1);
    }
    std::vector<int16_t> EndNodePins;
    for (int16_t& Index : mFork->mForkEventIndicies)
    {
        std::vector<int16_t> Ret = GetLastEventIndices(Index);
        for (int16_t RetIndex : Ret)
        {
            EndNodePins.push_back(UIEventEditor::mNodes[UIEventEditor::mEventFile.mFlowcharts[0].mEntryPoints.mItems.size() + RetIndex]->mOutputPinId);
        }
    }

    for (int16_t& Index : EndNodePins)
    {
        ed::Link(CurrentLinkId++, UIEventEditor::mNodes[UIEventEditor::mEventFile.mFlowcharts[0].mEntryPoints.mItems.size() + mFork->mJoinEventIndex]->mInputPinId, Index, ImColor(1.0f, 1.0f, 1.0f, 1.0f));
    }

    if (!EndNodePins.empty())
    {
        UIEventEditor::mNodes[UIEventEditor::mEventFile.mFlowcharts[0].mEntryPoints.mItems.size() + mFork->mJoinEventIndex]->mHasParentNode = true;
    }
}

void UIEventEditorFork::DeleteOutputLink(uint32_t LinkId)
{
    for (size_t i = 0; i < mOutputPinIds.size(); i++)
    {
        if (mOutputPinIds[i] == LinkId)
        {
            mFork->mForkEventIndicies.erase(mFork->mForkEventIndicies.begin() + i);
            break;
        }
    }
}

void UIEventEditorFork::CreateLink(uint32_t SourceLinkId, uint32_t DestinationLinkId)
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
        Logger::Error("UIEventEditorFork", "Destination node index was -1");
        return;
    }

    NodeIndex -= UIEventEditor::mEventFile.mFlowcharts[0].mEntryPoints.mItems.size();

    mFork->mForkEventIndicies.push_back(NodeIndex);
}

bool UIEventEditorFork::IsPinLinked(uint32_t PinId)
{
    if (mInputPinId == PinId)
        return mHasParentNode;

    return false;
}

UIEventEditorNodeBase::NodeType UIEventEditorFork::GetNodeType()
{
    return UIEventEditorNodeBase::NodeType::Fork;
}