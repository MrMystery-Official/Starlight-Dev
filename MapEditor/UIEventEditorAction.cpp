#include "UIEventEditorAction.h"

#include "imgui_node_editor.h"
#include "imgui.h"
#include "imgui_stdlib.h"
#include <deque>
#include "UIEventEditor.h"
#include "PopupModifyNodeActionQuery.h"

#include "Editor.h"

UIEventEditorAction::UIEventEditorAction(BFEVFLFile::ActionEvent& Action, uint32_t EditorId) : UIEventEditorNodeBase(EditorId), mAction(&Action)
{
}

ImColor UIEventEditorAction::GetHeaderColor()
{
    return ImColor(90, 130, 90);
}

void UIEventEditorAction::Render()
{
    uint32_t CurrentId = mEditorId;

    // Node body
    ed::PushStyleVar(ed::StyleVar_NodePadding, ImVec4(8, 8, 8, 8));
    ed::BeginNode(CurrentId++);

    std::string DisplayName = "Undefined::Undefined";
    
    if (mAction->mActorIndex >= 0)
    {
        DisplayName = UIEventEditor::mEventFile.mFlowcharts[0].mActors[mAction->mActorIndex].mName +
            "::" +
            UIEventEditor::mEventFile.mFlowcharts[0].mActors[mAction->mActorIndex].mActions[mAction->mActorActionIndex];
    }

    const char* Title = DisplayName.c_str();

    uint32_t FrameWidth = ImGui::CalcTextSize(Title).x + 2 * 8 + 48 + ImGui::GetStyle().ItemSpacing.x;

    ImGui::Dummy(ImVec2(24, 8));
    ImGui::SameLine();
    ImGui::Text(Title);
    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
    {
        PopupModifyNodeActionQuery::Open(mAction->mActorIndex, mAction->mActorActionIndex, mAction->mParameters, true, UIEventEditor::mEventFile);
    }
    mNodeShapeInfo.HeaderMin = ImVec2(ImGui::GetItemRectMin().x - 1 + ((Editor::UIScale - 1) * 10) - ImGui::GetStyle().ItemSpacing.x - 24 - 8, ImGui::GetItemRectMin().y - 9);
    mNodeShapeInfo.HeaderMax = ImVec2(mNodeShapeInfo.HeaderMin.x + FrameWidth + 4 + 8 + ((Editor::UIScale - 1) * 14), ImGui::GetItemRectMax().y + 8);

    ImGui::Dummy(ImVec2(0, 8));
    //ImGui::Combo(("Actor Name##" + std::to_string(mEditorId)).c_str(), reinterpret_cast<int*>(&mAction->mActorIndex), UIEventEditor::mActorNames.data(), UIEventEditor::mActorNames.size());

    for (auto ItemIter = mAction->mParameters.mItems.begin(); ItemIter != mAction->mParameters.mItems.end(); )
    {
        std::string& Key = ItemIter->first;
        auto& Val = ItemIter->second;
        ImGui::Text(Key.c_str());
        ImGui::SameLine();
        //		std::variant<RadiaxTree<ContainerItem>, std::vector<int32_t>, std::vector<bool>, std::vector<std::string>, std::pair<std::string, std::string>> mData;
        if (Val.mType == BFEVFLFile::ContainerDataType::String || Val.mType == BFEVFLFile::ContainerDataType::WString)
        {
            ImGui::PushItemWidth(ImGui::CalcTextSize(std::get<std::string>(Val.mData).c_str()).x + ImGui::GetStyle().ItemInnerSpacing.x * 2);
            ImGui::InputText(("##" + Key + "_" + std::to_string(mEditorId)).c_str(), &std::get<std::string>(Val.mData));
            ImGui::PopItemWidth();
            ImGui::SameLine();

            FrameWidth = std::max(FrameWidth, (uint32_t)(ImGui::GetStyle().ItemSpacing.x * 2 + ImGui::GetFrameHeight() + ImGui::CalcTextSize(Key.c_str()).x + ImGui::CalcTextSize(std::get<std::string>(Val.mData).c_str()).x + ImGui::GetStyle().ItemInnerSpacing.x * 2));

            if (ImGui::Button(("-##" + Key + "_" + std::to_string(mEditorId)).c_str()))
            {
                ItemIter = mAction->mParameters.mItems.erase(ItemIter);
                continue;
            }
        }
        else if (Val.mType == BFEVFLFile::ContainerDataType::Int)
        {
            ImGui::PushItemWidth(ImGui::CalcTextSize(std::to_string(std::get<int32_t>(Val.mData)).c_str()).x + ImGui::GetStyle().ItemInnerSpacing.x * 2);
            ImGui::InputScalar(("##" + Key + "_" + std::to_string(mEditorId)).c_str(), ImGuiDataType_S32, &std::get<int32_t>(Val.mData));
            ImGui::PopItemWidth();
            ImGui::SameLine();

            FrameWidth = std::max(FrameWidth, (uint32_t)(ImGui::GetStyle().ItemSpacing.x * 2 + ImGui::GetFrameHeight() + ImGui::CalcTextSize(Key.c_str()).x + ImGui::CalcTextSize(std::to_string(std::get<int32_t>(Val.mData)).c_str()).x + ImGui::GetStyle().ItemInnerSpacing.x * 2));

            if (ImGui::Button(("-##" + Key + "_" + std::to_string(mEditorId)).c_str()))
            {
                ItemIter = mAction->mParameters.mItems.erase(ItemIter);
                continue;
            }
        }
        else if (Val.mType == BFEVFLFile::ContainerDataType::Float)
        {
            ImGui::PushItemWidth(ImGui::CalcTextSize(std::to_string(std::get<float>(Val.mData)).c_str()).x + ImGui::GetStyle().ItemInnerSpacing.x * 2);
            ImGui::InputScalar(("##" + Key + "_" + std::to_string(mEditorId)).c_str(), ImGuiDataType_Float, &std::get<float>(Val.mData));
            ImGui::PopItemWidth();
            ImGui::SameLine();

            FrameWidth = std::max(FrameWidth, (uint32_t)(ImGui::GetStyle().ItemSpacing.x * 2 + ImGui::GetFrameHeight() + ImGui::CalcTextSize(Key.c_str()).x + ImGui::CalcTextSize(std::to_string(std::get<float>(Val.mData)).c_str()).x + ImGui::GetStyle().ItemInnerSpacing.x * 2));

            if (ImGui::Button(("-##" + Key + "_" + std::to_string(mEditorId)).c_str()))
            {
                ItemIter = mAction->mParameters.mItems.erase(ItemIter);
                continue;
            }
        }
        else if (Val.mType == BFEVFLFile::ContainerDataType::Bool)
        {
            ImGui::Checkbox(("##" + Key + "_" + std::to_string(mEditorId)).c_str(), &std::get<bool>(Val.mData));
            ImGui::SameLine();

            FrameWidth = std::max(FrameWidth, (uint32_t)(ImGui::GetStyle().ItemSpacing.x * 2 + ImGui::GetFrameHeight() * 2 + ImGui::CalcTextSize(Key.c_str()).x + ImGui::GetStyle().ItemInnerSpacing.x * 2));

            if (ImGui::Button(("-##" + Key + "_" + std::to_string(mEditorId)).c_str()))
            {
                ItemIter = mAction->mParameters.mItems.erase(ItemIter);
                continue;
            }
        }
        else if (Val.mType == BFEVFLFile::ContainerDataType::FloatArray)
        {
            std::vector<float>& Value = std::get<std::vector<float>>(Val.mData);
            if (ImGui::Button(("+##" + Key + "_" + std::to_string(mEditorId)).c_str()))
            {
                Value.push_back(0.0f);
            }
            ImGui::SameLine();
            if (ImGui::Button(("-##" + Key + "_" + std::to_string(mEditorId)).c_str()))
            {
                ItemIter = mAction->mParameters.mItems.erase(ItemIter);
                continue;
            }
            ImGui::Indent();
            for (auto Iter = Value.begin(); Iter != Value.end(); )
            {
                uint32_t Index = std::distance(Value.begin(), Iter);
                ImGui::PushItemWidth(ImGui::CalcTextSize(std::to_string(Value[Index]).c_str()).x + ImGui::GetStyle().ItemInnerSpacing.x * 2);
                ImGui::InputScalar(("##" + Key + "_" + std::to_string(mEditorId) + "_" + std::to_string(Index)).c_str(), ImGuiDataType_Float, &Value[Index]);
                ImGui::PopItemWidth();
                if (ImGui::GetIO().WantTextInput)
                    ImGui::BeginDisabled();
				ImGui::SameLine();

                FrameWidth = std::max(FrameWidth, (uint32_t)(ImGui::GetStyle().IndentSpacing + ImGui::CalcTextSize(std::to_string(Value[Index]).c_str()).x + ImGui::GetStyle().ItemInnerSpacing.x * 2 + ImGui::GetStyle().ItemSpacing.x + ImGui::GetFrameHeight()));
				
                if (ImGui::Button(("-##" + Key + "_" + std::to_string(mEditorId) + "_" + std::to_string(Index)).c_str()))
				{
					Iter = Value.erase(Iter);
					continue;
				}
                if (ImGui::GetIO().WantTextInput)
                    ImGui::EndDisabled();
                Iter++;
            }
            FrameWidth = std::max(FrameWidth, (uint32_t)(ImGui::CalcTextSize(Key.c_str()).x + ImGui::GetStyle().ItemSpacing.x * 2 + ImGui::GetFrameHeight() * 2));
            ImGui::Unindent();
        }
        else if (Val.mType == BFEVFLFile::ContainerDataType::BoolArray)
        {
            std::deque<bool>& Value = std::get<std::deque<bool>>(Val.mData);
            if (ImGui::Button(("+##" + Key + "_" + std::to_string(mEditorId)).c_str()))
            {
                Value.push_back(false);
            }
            ImGui::SameLine();
            if (ImGui::Button(("-##" + Key + "_" + std::to_string(mEditorId)).c_str()))
            {
                ItemIter = mAction->mParameters.mItems.erase(ItemIter);
                continue;
            }
            ImGui::Indent();
            for (auto Iter = Value.begin(); Iter != Value.end(); )
            {
                uint32_t Index = std::distance(Value.begin(), Iter);
                ImGui::Checkbox(("##" + Key + "_" + std::to_string(mEditorId) + "_" + std::to_string(Index)).c_str(), &Value[Index]);
                if (ImGui::GetIO().WantTextInput)
                    ImGui::BeginDisabled();

                FrameWidth = std::max(FrameWidth, (uint32_t)(ImGui::GetStyle().IndentSpacing + ImGui::GetFrameHeight() + ImGui::GetStyle().ItemInnerSpacing.x * 2 + ImGui::GetStyle().ItemSpacing.x + ImGui::GetFrameHeight()));

                ImGui::SameLine();
                if (ImGui::Button(("-##" + Key + "_" + std::to_string(mEditorId) + "_" + std::to_string(Index)).c_str()))
                {
                    Iter = Value.erase(Iter);
                    continue;
                }
                if (ImGui::GetIO().WantTextInput)
                    ImGui::EndDisabled();
                Iter++;
            }
            ImGui::Unindent();
            FrameWidth = std::max(FrameWidth, (uint32_t)(ImGui::CalcTextSize(Key.c_str()).x + ImGui::GetStyle().ItemSpacing.x * 2 + ImGui::GetFrameHeight() * 2));
        }
        else if (Val.mType == BFEVFLFile::ContainerDataType::IntArray)
        {
            std::vector<int32_t>& Value = std::get<std::vector<int32_t>>(Val.mData);
            if (ImGui::Button(("+##" + Key + "_" + std::to_string(mEditorId)).c_str()))
            {
                Value.push_back(0);
            }
            ImGui::Indent();
            for (auto Iter = Value.begin(); Iter != Value.end(); )
            {
                uint32_t Index = std::distance(Value.begin(), Iter);
                ImGui::PushItemWidth(ImGui::CalcTextSize(std::to_string(Value[Index]).c_str()).x + ImGui::GetStyle().ItemInnerSpacing.x * 2);
                ImGui::InputScalar(("##" + Key + "_" + std::to_string(mEditorId) + "_" + std::to_string(Index)).c_str(), ImGuiDataType_S32, &Value[Index]);
                ImGui::PopItemWidth();
                if (ImGui::GetIO().WantTextInput)
                    ImGui::BeginDisabled();
                ImGui::SameLine();

                FrameWidth = std::max(FrameWidth, (uint32_t)(ImGui::GetStyle().IndentSpacing + ImGui::CalcTextSize(std::to_string(Value[Index]).c_str()).x + ImGui::GetStyle().ItemInnerSpacing.x * 2 + ImGui::GetStyle().ItemSpacing.x + ImGui::GetFrameHeight()));

                if (ImGui::Button(("-##" + Key + "_" + std::to_string(mEditorId) + "_" + std::to_string(Index)).c_str()))
                {
                    Iter = Value.erase(Iter);
                    continue;
                }
                if (ImGui::GetIO().WantTextInput)
                    ImGui::EndDisabled();
                Iter++;
            }
            ImGui::Unindent();
            FrameWidth = std::max(FrameWidth, (uint32_t)(ImGui::CalcTextSize(Key.c_str()).x + ImGui::GetStyle().ItemSpacing.x * 2 + ImGui::GetFrameHeight() * 2));
        }
        ItemIter++;
    }

    mNodeShapeInfo.HeaderMax.x = mNodeShapeInfo.HeaderMin.x + 8 + 8 + FrameWidth + ((Editor::UIScale - 1) * 14);
    ImGui::SetCursorPos(ImVec2(mNodeShapeInfo.HeaderMax.x - 32 - 8, (mNodeShapeInfo.HeaderMax.y - mNodeShapeInfo.HeaderMin.y) / 2 - 12 + mNodeShapeInfo.HeaderMin.y));
    DrawPin(CurrentId++, mAction->mNextEventIndex >= 0, 255, ed::PinKind::Output);
    mOutputPinId = CurrentId - 1;

    ImGui::SetCursorPos(ImVec2(mNodeShapeInfo.HeaderMin.x + 8, (mNodeShapeInfo.HeaderMax.y - mNodeShapeInfo.HeaderMin.y) / 2 - 12 + mNodeShapeInfo.HeaderMin.y));
    DrawPin(CurrentId++, mHasParentNode, 255, ed::PinKind::Input);
    mInputPinId = CurrentId - 1;

    ed::EndNode();
    ed::PopStyleVar();

    DrawHeader();

    mHasParentNode = false;
}

void UIEventEditorAction::DrawLinks()
{
    uint32_t CurrentLinkId = mEditorId + 500;
    if (mAction->mNextEventIndex >= 0)
    {
        std::unique_ptr<UIEventEditorNodeBase>& Node = UIEventEditor::mNodes[UIEventEditor::mEventFile.mFlowcharts[0].mEntryPoints.mItems.size() + mAction->mNextEventIndex];
        ed::Link(CurrentLinkId++, mOutputPinId, Node->mInputPinId, ImColor(1.0f, 1.0f, 1.0f, 1.0f));
        Node->mHasParentNode = true;
    }
}

void UIEventEditorAction::CreateLink(uint32_t SourceLinkId, uint32_t DestinationLinkId)
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
        Logger::Error("UIEventEditorAction", "Destination node index was -1");
        return;
    }

    NodeIndex -= UIEventEditor::mEventFile.mFlowcharts[0].mEntryPoints.mItems.size();

    mAction->mNextEventIndex = NodeIndex;
}

bool UIEventEditorAction::IsPinLinked(uint32_t PinId)
{
    if (mOutputPinId == PinId)
        return mAction->mNextEventIndex >= 0;

    if (mInputPinId == PinId)
        return mHasParentNode;

    return false;
}

void UIEventEditorAction::DeleteOutputLink(uint32_t LinkId)
{
    mAction->mNextEventIndex = -1;
}

UIEventEditorNodeBase::NodeType UIEventEditorAction::GetNodeType()
{
    return UIEventEditorNodeBase::NodeType::Action;
}