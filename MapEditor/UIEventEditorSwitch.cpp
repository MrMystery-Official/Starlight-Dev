#include "UIEventEditorSwitch.h"

#include "imgui_node_editor.h"
#include "imgui.h"
#include "imgui_stdlib.h"
#include <deque>
#include "UIEventEditor.h"
#include "PopupModifyNodeActionQuery.h"

#include "Editor.h"

UIEventEditorSwitch::UIEventEditorSwitch(BFEVFLFile::SwitchEvent& Switch, uint32_t EditorId) : UIEventEditorNodeBase(EditorId), mSwitch(&Switch)
{
}

ImColor UIEventEditorSwitch::GetHeaderColor()
{
    return ImColor(70, 100, 130);
}

void UIEventEditorSwitch::Render()
{
    uint32_t CurrentId = mEditorId;

    // Node body
    ed::PushStyleVar(ed::StyleVar_NodePadding, ImVec4(8, 8, 8, 8));
    ed::BeginNode(CurrentId++);

    std::string DisplayName = "Undefined::Undefined";

    if (mSwitch->mActorIndex >= 0)
    {
        DisplayName = UIEventEditor::mEventFile.mFlowcharts[0].mActors[mSwitch->mActorIndex].mName +
            "::" +
            UIEventEditor::mEventFile.mFlowcharts[0].mActors[mSwitch->mActorIndex].mQueries[mSwitch->mActorQueryIndex];
    }

    const char* Title = DisplayName.c_str();

    uint32_t FrameWidth = ImGui::CalcTextSize(Title).x + 2 * 8 + 48 + ImGui::GetStyle().ItemSpacing.x;

    ImGui::Dummy(ImVec2(24, 8));
    ImGui::SameLine();
    ImGui::Text(Title);
    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
    {
        PopupModifyNodeActionQuery::Open(mSwitch->mActorIndex, mSwitch->mActorQueryIndex, mSwitch->mParameters, false, UIEventEditor::mEventFile);
    }
    mNodeShapeInfo.HeaderMin = ImVec2(ImGui::GetItemRectMin().x - 1 + ((Editor::UIScale - 1) * 10) - ImGui::GetStyle().ItemSpacing.x - 24 - 8, ImGui::GetItemRectMin().y - 9);
    mNodeShapeInfo.HeaderMax = ImVec2(mNodeShapeInfo.HeaderMin.x + FrameWidth + 4 + 8 + ((Editor::UIScale - 1) * 14), ImGui::GetItemRectMax().y + 8);

    ImGui::Dummy(ImVec2(0, 8));
    //ImGui::Combo(("Actor Name##" + std::to_string(mEditorId)).c_str(), reinterpret_cast<int*>(&mAction->mActorIndex), UIEventEditor::mActorNames.data(), UIEventEditor::mActorNames.size());

    //ImGui::Dummy(ImVec2(0, ImGui::GetStyle().ItemSpacing.y * 2 + 24 * mSwitch->mSwitchCases.size() + ));

    if (ImGui::Button("Add case"))
    {
        mSwitch->mSwitchCases.push_back(BFEVFLFile::SwitchEvent::Case{
            .mValue = 0,
            .mEventIndex = -1
            });
    }

    for (auto ItemIter = mSwitch->mParameters.mItems.begin(); ItemIter != mSwitch->mParameters.mItems.end(); )
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
                ItemIter = mSwitch->mParameters.mItems.erase(ItemIter);
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
                ItemIter = mSwitch->mParameters.mItems.erase(ItemIter);
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
                ItemIter = mSwitch->mParameters.mItems.erase(ItemIter);
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
                ItemIter = mSwitch->mParameters.mItems.erase(ItemIter);
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
                ItemIter = mSwitch->mParameters.mItems.erase(ItemIter);
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
                ItemIter = mSwitch->mParameters.mItems.erase(ItemIter);
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

    mNodeShapeInfo.HeaderMax.x = mNodeShapeInfo.HeaderMin.x + FrameWidth + 2 + 8 + ((Editor::UIScale - 1) * 14);

    uint32_t CaseIndex = 0;
    uint32_t CurrentLinkId = mEditorId + 500;
    mOutputCasePinIds.clear();
    for (auto Iter = mSwitch->mSwitchCases.begin(); Iter != mSwitch->mSwitchCases.end(); )
    {
        std::string CaseText = std::to_string(Iter->mValue);
        ImGui::SetCursorPosX(mNodeShapeInfo.HeaderMax.x - 32 - 8 - ImGui::GetStyle().ItemSpacing.x * 2 - ImGui::CalcTextSize(CaseText.c_str()).x - ImGui::GetStyle().ItemInnerSpacing.x * 4 - ImGui::CalcTextSize("-").x);
        if (ImGui::Button(("-##" + std::to_string(mEditorId) + "_" + std::to_string(CaseIndex)).c_str()))
        {
            Iter = mSwitch->mSwitchCases.erase(Iter);
            continue;
        }
        ImGui::SameLine();
        ImGui::PushItemWidth(ImGui::CalcTextSize(CaseText.c_str()).x + ImGui::GetStyle().ItemInnerSpacing.x * 2);
        ImGui::InputScalar(("##" + std::to_string(mEditorId) + "_" + std::to_string(CaseIndex)).c_str(), ImGuiDataType_S32, &Iter->mValue);
        ImGui::PopItemWidth();
        ImGui::SameLine();
        DrawPin(CurrentLinkId++, Iter->mEventIndex >= 0, 255, ed::PinKind::Output);
        mOutputCasePinIds.push_back(CurrentLinkId - 1);
        CaseIndex++;
        Iter++;
    }

    ImGui::SetCursorPos(ImVec2(mNodeShapeInfo.HeaderMin.x + 8, (mNodeShapeInfo.HeaderMax.y - mNodeShapeInfo.HeaderMin.y) / 2 - 12 + mNodeShapeInfo.HeaderMin.y));
    DrawPin(CurrentId++, mHasParentNode, 255, ed::PinKind::Input);
    mInputPinId = CurrentId - 1;

    ed::EndNode();
    ed::PopStyleVar();

    DrawHeader();

    mHasParentNode = false;
}

void UIEventEditorSwitch::DeleteOutputLink(uint32_t LinkId)
{
    for (size_t i = 0; i < mOutputCasePinIds.size(); i++)
    {
        if (mOutputCasePinIds[i] == LinkId)
        {
            mSwitch->mSwitchCases[i].mEventIndex = -1;
            break;
        }
    }
}

void UIEventEditorSwitch::CreateLink(uint32_t SourceLinkId, uint32_t DestinationLinkId)
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
        Logger::Error("UIEventEditorSwitch", "Destination node index was -1");
        return;
    }

    NodeIndex -= UIEventEditor::mEventFile.mFlowcharts[0].mEntryPoints.mItems.size();

    for (size_t i = 0; i < mOutputCasePinIds.size(); i++)
    {
        if (mOutputCasePinIds[i] == SourceLinkId)
        {
            mSwitch->mSwitchCases[i].mEventIndex = NodeIndex;
            break;
        }
    }
}

bool UIEventEditorSwitch::IsPinLinked(uint32_t PinId)
{
    for (size_t i = 0; i < mOutputCasePinIds.size(); i++)
    {
        if (mOutputCasePinIds[i] == PinId)
        {
            return mSwitch->mSwitchCases[i].mEventIndex >= 0;
        }
    }

    if (mInputPinId == PinId)
        return mHasParentNode;

    return false;
}

void UIEventEditorSwitch::DrawLinks()
{
    uint32_t CurrentLinkId = mEditorId + 500;
    
    for (size_t i = 0; i < mSwitch->mSwitchCases.size(); i++)
    {
        if (mSwitch->mSwitchCases[i].mValue < 0)
            continue;
        
        std::unique_ptr<UIEventEditorNodeBase>& Node = UIEventEditor::mNodes[UIEventEditor::mEventFile.mFlowcharts[0].mEntryPoints.mItems.size() + mSwitch->mSwitchCases[i].mEventIndex];
        ed::Link(CurrentLinkId++, mOutputCasePinIds[i], Node->mInputPinId, ImColor(1.0f, 1.0f, 1.0f, 1.0f));
        Node->mHasParentNode = true;
    }
}

UIEventEditorNodeBase::NodeType UIEventEditorSwitch::GetNodeType()
{
    return UIEventEditorNodeBase::NodeType::Switch;
}