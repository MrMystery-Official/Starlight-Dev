#include "UIEventEditor.h"

#include "UIEventEditorEntryPoint.h"
#include "UIEventEditorSubFlow.h"
#include "UIEventEditorAction.h"
#include "UIEventEditorFork.h"
#include "UIEventEditorJoin.h"
#include "UIEventEditorSwitch.h"
#include "StarImGui.h"

#include "PopupAddActorActionQuery.h"

bool UIEventEditor::mOpen = true;
bool UIEventEditor::mPropertiesFirstTime = true;
bool UIEventEditor::mFocused = false;
ed::EditorContext* UIEventEditor::mContext = nullptr;
BFEVFLFile UIEventEditor::mEventFile;
std::vector<std::unique_ptr<UIEventEditorNodeBase>> UIEventEditor::mNodes;
uint32_t UIEventEditor::mId = 1000;
std::vector<const char*> UIEventEditor::mActorNames;
ed::LinkId UIEventEditor::ContextLinkId;
ed::NodeId UIEventEditor::ContextNodeId;

void UIEventEditor::Initialize()
{
    ed::Config Config;
    Config.SettingsFile = nullptr;
    mContext = ed::CreateEditor(&Config);
}

void UIEventEditor::Delete()
{
    ed::DestroyEditor(mContext);
}

void UIEventEditor::LoadEventFile(std::vector<unsigned char> Bytes)
{
    mEventFile = BFEVFLFile(Bytes);

    mActorNames.clear();

    for (BFEVFLFile::Actor& Actor : mEventFile.mFlowcharts[0].mActors)
    {
        mActorNames.push_back(Actor.mName.c_str());
    }

    for (auto& Entry : mEventFile.mFlowcharts[0].mEntryPoints.mItems)
    {
        mNodes.push_back(std::make_unique<UIEventEditorEntryPoint>(Entry, mId));
        mId += 1000;
    }

    for (auto& [Type, Event] : mEventFile.mFlowcharts[0].mEvents)
    {
        switch (Type)
        {
        case BFEVFLFile::EventType::SubFlow:
            mNodes.push_back(std::make_unique<UIEventEditorSubFlow>(std::get<BFEVFLFile::SubFlowEvent>(Event), mId));
            mId += 1000;
            break;
        case BFEVFLFile::EventType::Action:
            mNodes.push_back(std::make_unique<UIEventEditorAction>(std::get<BFEVFLFile::ActionEvent>(Event), mId));
            mId += 1000;
            break;
        case BFEVFLFile::EventType::Fork:
            mNodes.push_back(std::make_unique<UIEventEditorFork>(std::get<BFEVFLFile::ForkEvent>(Event), mId));
            mId += 1000;
            break;
        case BFEVFLFile::EventType::Join:
            mNodes.push_back(std::make_unique<UIEventEditorJoin>(std::get<BFEVFLFile::JoinEvent>(Event), mId));
            mId += 1000;
            break;
        case BFEVFLFile::EventType::Switch:
            mNodes.push_back(std::make_unique<UIEventEditorSwitch>(std::get<BFEVFLFile::SwitchEvent>(Event), mId));
            mId += 1000;
            break;
        default:
            break;
        }
    }
}

void UIEventEditor::ManageLinkCreationDeletion()
{
    if (ed::BeginCreate(ImColor(255, 255, 255), 2.0f))
    {
        auto ShowLabel = [](const char* label, ImColor color)
            {
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ImGui::GetTextLineHeight());
                auto size = ImGui::CalcTextSize(label);

                auto padding = ImGui::GetStyle().FramePadding;
                auto spacing = ImGui::GetStyle().ItemSpacing;

                ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPos().x + spacing.x, ImGui::GetCursorPos().y - spacing.y));

                auto rectMin = ImVec2(ImGui::GetCursorScreenPos().x - padding.x, ImGui::GetCursorScreenPos().y - padding.y);
                auto rectMax = ImVec2(ImGui::GetCursorScreenPos().x + size.x + padding.x, ImGui::GetCursorScreenPos().y + size.y + padding.y);

                auto drawList = ImGui::GetWindowDrawList();
                drawList->AddRectFilled(rectMin, rectMax, color, size.y * 0.15f);
                ImGui::TextUnformatted(label);
            };

        auto GetNodeFromPinId = [](ed::PinId Id)
            {
                return &UIEventEditorNodeBase::GetNodeByPinId(Id.Get());
            };

        ed::PinId StartPinId, EndPinId = 0;
        if (ed::QueryNewLink(&StartPinId, &EndPinId))
        {
            std::unique_ptr<UIEventEditorNodeBase>* SourceNode = GetNodeFromPinId(StartPinId);
            std::unique_ptr<UIEventEditorNodeBase>* DestinationNode = GetNodeFromPinId(EndPinId);

            if (SourceNode != nullptr && DestinationNode != nullptr)
            {
                if(SourceNode->get()->mInputPinId == StartPinId.Get())
                {
                    std::swap(SourceNode, DestinationNode);
                    std::swap(StartPinId, EndPinId);
                }

                if (SourceNode == DestinationNode)
                {
                    ShowLabel("x Can't link to self", ImColor(45, 32, 32, 180));
                    ed::RejectNewItem(ImColor(255, 0, 0), 1.0f);
                }
                else if (SourceNode->get()->IsPinLinked(StartPinId.Get()))
                {
                    ShowLabel("x Source pin already linked", ImColor(45, 32, 32, 180));
                    ed::RejectNewItem(ImColor(255, 0, 0), 1.0f);
                }
                else if (DestinationNode->get()->mInputPinId != EndPinId.Get())
                {
                    ShowLabel("x Incompatible Pin Kind", ImColor(45, 32, 32, 180));
                    ed::RejectNewItem(ImColor(255, 0, 0), 2.0f);
                }
                else if (DestinationNode->get()->GetNodeType() == UIEventEditorNodeBase::NodeType::Join)
                {
                    ShowLabel("x Manual linking with join nodes is not permitted", ImColor(45, 32, 32, 180));
                    ed::RejectNewItem(ImColor(255, 0, 0), 2.0f);
                }
                else
                {
                    ShowLabel("+ Create Link", ImColor(32, 45, 32, 180));
                    if (ed::AcceptNewItem(ImColor(128, 255, 128), 4.0f))
                    {
                        SourceNode->get()->CreateLink(StartPinId.Get(), EndPinId.Get());
                    }
                }
            }
        }
    }

    ed::EndCreate();

    ImVec2 OpenPopupPosition = ImGui::GetMousePos();
    ed::Suspend();
    if (ed::ShowLinkContextMenu(&ContextLinkId))
    {
        ImGui::OpenPopup("Link Context Menu");
    }
    if (ed::ShowNodeContextMenu(&ContextNodeId))
    {
        ImGui::OpenPopup("Node Context Menu");
    }
    if (ed::ShowBackgroundContextMenu())
    {
        ImGui::OpenPopup("Background Context Menu");
    }

    if (ImGui::BeginPopup("Link Context Menu")) {
        if (ImGui::MenuItem("Delete##DelLink")) {

            uint32_t NodeIndex = ContextLinkId.Get();
            if(NodeIndex >= 1000)
                UIEventEditorNodeBase::GetNodeByLinkId(NodeIndex)->DeleteOutputLink(NodeIndex);
        }
        ImGui::EndPopup();
    }
    if (ImGui::BeginPopup("Node Context Menu")) {
        if (ImGui::MenuItem("Delete##DelNode")) {
            uint32_t NodeIndex = ContextNodeId.Get();

            for (auto& [Key, Val] : mEventFile.mFlowcharts[0].mEntryPoints.mItems)
            {
                //UIEventEditorNodeBase::GetNodeByPinId();
            }
            mNodes.erase(mNodes.begin() + ((int)NodeIndex / 1000));
        }
        ImGui::EndPopup();
    }
    if (ImGui::BeginPopup("Background Context Menu"))
    {
        if (ImGui::MenuItem("Add Action"))
        {
            BFEVFLFile::ActionEvent Action;
            Action.mActorActionIndex = 0;
            Action.mActorIndex = -1;
            Action.mName = "";
            Action.mNextEventIndex = -1;
            Action.mType = BFEVFLFile::EventType::Action;
            mEventFile.mFlowcharts[0].mEvents.push_back(std::make_pair(BFEVFLFile::EventType::Action, Action));

            mNodes.push_back(std::make_unique<UIEventEditorAction>(std::get<BFEVFLFile::ActionEvent>(mEventFile.mFlowcharts[0].mEvents[mEventFile.mFlowcharts[0].mEvents.size() - 1].second), mId));
            mId += 1000;

            for (size_t i = 0; i < mEventFile.mFlowcharts[0].mEntryPoints.mItems.size(); i++)
            {
                static_cast<UIEventEditorEntryPoint*>(mNodes[i].get())->mEntryPoint = &mEventFile.mFlowcharts[0].mEntryPoints.mItems[i];
            }

            for (size_t i = 0; i < mEventFile.mFlowcharts[0].mEvents.size(); i++)
            {
                switch (mEventFile.mFlowcharts[0].mEvents[i].first)
                {
                case BFEVFLFile::EventType::SubFlow:
                    static_cast<UIEventEditorSubFlow*>(mNodes[mEventFile.mFlowcharts[0].mEntryPoints.mItems.size() + i].get())->mSubFlow = &std::get<BFEVFLFile::SubFlowEvent>(mEventFile.mFlowcharts[0].mEvents[i].second);
                    break;
                case BFEVFLFile::EventType::Action:
                    static_cast<UIEventEditorAction*>(mNodes[mEventFile.mFlowcharts[0].mEntryPoints.mItems.size() + i].get())->mAction = &std::get<BFEVFLFile::ActionEvent>(mEventFile.mFlowcharts[0].mEvents[i].second);
                    break;
                case BFEVFLFile::EventType::Fork:
                    static_cast<UIEventEditorFork*>(mNodes[mEventFile.mFlowcharts[0].mEntryPoints.mItems.size() + i].get())->mFork = &std::get<BFEVFLFile::ForkEvent>(mEventFile.mFlowcharts[0].mEvents[i].second);
                    break;
                case BFEVFLFile::EventType::Join:
                    static_cast<UIEventEditorJoin*>(mNodes[mEventFile.mFlowcharts[0].mEntryPoints.mItems.size() + i].get())->mJoin = &std::get<BFEVFLFile::JoinEvent>(mEventFile.mFlowcharts[0].mEvents[i].second);
                    break;
                case BFEVFLFile::EventType::Switch:
                    static_cast<UIEventEditorSwitch*>(mNodes[mEventFile.mFlowcharts[0].mEntryPoints.mItems.size() + i].get())->mSwitch = &std::get<BFEVFLFile::SwitchEvent>(mEventFile.mFlowcharts[0].mEvents[i].second);
                    break;
                default:
                    break;
                }
            }
        }
        if (ImGui::MenuItem("Add Switch"))
        {
            BFEVFLFile::SwitchEvent Switch;
            Switch.mActorIndex = -1;
            Switch.mActorQueryIndex = 0;
            Switch.mName = "";
            Switch.mType = BFEVFLFile::EventType::Switch;
            mEventFile.mFlowcharts[0].mEvents.push_back(std::make_pair(BFEVFLFile::EventType::Switch, Switch));

            mNodes.push_back(std::make_unique<UIEventEditorSwitch>(std::get<BFEVFLFile::SwitchEvent>(mEventFile.mFlowcharts[0].mEvents[mEventFile.mFlowcharts[0].mEvents.size() - 1].second), mId));
            mId += 1000;

            for (size_t i = 0; i < mEventFile.mFlowcharts[0].mEntryPoints.mItems.size(); i++)
            {
                static_cast<UIEventEditorEntryPoint*>(mNodes[i].get())->mEntryPoint = &mEventFile.mFlowcharts[0].mEntryPoints.mItems[i];
            }

            for (size_t i = 0; i < mEventFile.mFlowcharts[0].mEvents.size(); i++)
            {
                switch (mEventFile.mFlowcharts[0].mEvents[i].first)
                {
                case BFEVFLFile::EventType::SubFlow:
                    static_cast<UIEventEditorSubFlow*>(mNodes[mEventFile.mFlowcharts[0].mEntryPoints.mItems.size() + i].get())->mSubFlow = &std::get<BFEVFLFile::SubFlowEvent>(mEventFile.mFlowcharts[0].mEvents[i].second);
                    break;
                case BFEVFLFile::EventType::Action:
                    static_cast<UIEventEditorAction*>(mNodes[mEventFile.mFlowcharts[0].mEntryPoints.mItems.size() + i].get())->mAction = &std::get<BFEVFLFile::ActionEvent>(mEventFile.mFlowcharts[0].mEvents[i].second);
                    break;
                case BFEVFLFile::EventType::Fork:
                    static_cast<UIEventEditorFork*>(mNodes[mEventFile.mFlowcharts[0].mEntryPoints.mItems.size() + i].get())->mFork = &std::get<BFEVFLFile::ForkEvent>(mEventFile.mFlowcharts[0].mEvents[i].second);
                    break;
                case BFEVFLFile::EventType::Join:
                    static_cast<UIEventEditorJoin*>(mNodes[mEventFile.mFlowcharts[0].mEntryPoints.mItems.size() + i].get())->mJoin = &std::get<BFEVFLFile::JoinEvent>(mEventFile.mFlowcharts[0].mEvents[i].second);
                    break;
                case BFEVFLFile::EventType::Switch:
                    static_cast<UIEventEditorSwitch*>(mNodes[mEventFile.mFlowcharts[0].mEntryPoints.mItems.size() + i].get())->mSwitch = &std::get<BFEVFLFile::SwitchEvent>(mEventFile.mFlowcharts[0].mEvents[i].second);
                    break;
                default:
                    break;
                }
            }
        }
        if (ImGui::MenuItem("Add SubFlow"))
        {
            BFEVFLFile::SubFlowEvent SubFlow;
            SubFlow.mEntryPointName = "None";
            SubFlow.mFlowchartName = "None";
            SubFlow.mName = "";
            SubFlow.mNextEventIndex = -1;
            SubFlow.mType = BFEVFLFile::EventType::SubFlow;
            mEventFile.mFlowcharts[0].mEvents.push_back(std::make_pair(BFEVFLFile::EventType::SubFlow, SubFlow));

            mNodes.push_back(std::make_unique<UIEventEditorSubFlow>(std::get<BFEVFLFile::SubFlowEvent>(mEventFile.mFlowcharts[0].mEvents[mEventFile.mFlowcharts[0].mEvents.size() - 1].second), mId));
            mId += 1000;

            for (size_t i = 0; i < mEventFile.mFlowcharts[0].mEntryPoints.mItems.size(); i++)
            {
                static_cast<UIEventEditorEntryPoint*>(mNodes[i].get())->mEntryPoint = &mEventFile.mFlowcharts[0].mEntryPoints.mItems[i];
            }

            for (size_t i = 0; i < mEventFile.mFlowcharts[0].mEvents.size(); i++)
            {
                switch (mEventFile.mFlowcharts[0].mEvents[i].first)
                {
                case BFEVFLFile::EventType::SubFlow:
                    static_cast<UIEventEditorSubFlow*>(mNodes[mEventFile.mFlowcharts[0].mEntryPoints.mItems.size() + i].get())->mSubFlow = &std::get<BFEVFLFile::SubFlowEvent>(mEventFile.mFlowcharts[0].mEvents[i].second);
                    break;
                case BFEVFLFile::EventType::Action:
                    static_cast<UIEventEditorAction*>(mNodes[mEventFile.mFlowcharts[0].mEntryPoints.mItems.size() + i].get())->mAction = &std::get<BFEVFLFile::ActionEvent>(mEventFile.mFlowcharts[0].mEvents[i].second);
                    break;
                case BFEVFLFile::EventType::Fork:
                    static_cast<UIEventEditorFork*>(mNodes[mEventFile.mFlowcharts[0].mEntryPoints.mItems.size() + i].get())->mFork = &std::get<BFEVFLFile::ForkEvent>(mEventFile.mFlowcharts[0].mEvents[i].second);
                    break;
                case BFEVFLFile::EventType::Join:
                    static_cast<UIEventEditorJoin*>(mNodes[mEventFile.mFlowcharts[0].mEntryPoints.mItems.size() + i].get())->mJoin = &std::get<BFEVFLFile::JoinEvent>(mEventFile.mFlowcharts[0].mEvents[i].second);
                    break;
                case BFEVFLFile::EventType::Switch:
                    static_cast<UIEventEditorSwitch*>(mNodes[mEventFile.mFlowcharts[0].mEntryPoints.mItems.size() + i].get())->mSwitch = &std::get<BFEVFLFile::SwitchEvent>(mEventFile.mFlowcharts[0].mEvents[i].second);
                    break;
                default:
                    break;
                }
            }
        }
        if (ImGui::MenuItem("Add EntryPoint"))
        {
            BFEVFLFile::EntryPoint Point;
            mEventFile.mFlowcharts[0].mEntryPoints.mItems.insert(mEventFile.mFlowcharts[0].mEntryPoints.mItems.begin(), std::make_pair("None", Point));

            mNodes.insert(mNodes.begin(), std::make_unique<UIEventEditorEntryPoint>(mEventFile.mFlowcharts[0].mEntryPoints.mItems.front(), mId));
            mId += 1000;

            for (size_t i = 1; i < mEventFile.mFlowcharts[0].mEntryPoints.mItems.size(); i++)
            {
                static_cast<UIEventEditorEntryPoint*>(mNodes[i].get())->mEntryPoint = &mEventFile.mFlowcharts[0].mEntryPoints.mItems[i];
            }

            for (size_t i = 0; i < mEventFile.mFlowcharts[0].mEvents.size(); i++)
            {
                switch (mEventFile.mFlowcharts[0].mEvents[i].first)
                {
                case BFEVFLFile::EventType::SubFlow:
                    static_cast<UIEventEditorSubFlow*>(mNodes[mEventFile.mFlowcharts[0].mEntryPoints.mItems.size() + i].get())->mSubFlow = &std::get<BFEVFLFile::SubFlowEvent>(mEventFile.mFlowcharts[0].mEvents[i].second);
                    break;
                case BFEVFLFile::EventType::Action:
                    static_cast<UIEventEditorAction*>(mNodes[mEventFile.mFlowcharts[0].mEntryPoints.mItems.size() + i].get())->mAction = &std::get<BFEVFLFile::ActionEvent>(mEventFile.mFlowcharts[0].mEvents[i].second);
                    break;
                case BFEVFLFile::EventType::Fork:
                    static_cast<UIEventEditorFork*>(mNodes[mEventFile.mFlowcharts[0].mEntryPoints.mItems.size() + i].get())->mFork = &std::get<BFEVFLFile::ForkEvent>(mEventFile.mFlowcharts[0].mEvents[i].second);
                    break;
                case BFEVFLFile::EventType::Join:
                    static_cast<UIEventEditorJoin*>(mNodes[mEventFile.mFlowcharts[0].mEntryPoints.mItems.size() + i].get())->mJoin = &std::get<BFEVFLFile::JoinEvent>(mEventFile.mFlowcharts[0].mEvents[i].second);
                    break;
                case BFEVFLFile::EventType::Switch:
                    static_cast<UIEventEditorSwitch*>(mNodes[mEventFile.mFlowcharts[0].mEntryPoints.mItems.size() + i].get())->mSwitch = &std::get<BFEVFLFile::SwitchEvent>(mEventFile.mFlowcharts[0].mEvents[i].second);
                    break;
                default:
                    break;
                }
            }
        }
        if (ImGui::MenuItem("Add Fork"))
        {
            BFEVFLFile::ForkEvent Fork;
            BFEVFLFile::JoinEvent Join;
            Join.mNextEventIndex = -1;
            Fork.mJoinEventIndex = mEventFile.mFlowcharts[0].mEvents.size() + 1;
            mEventFile.mFlowcharts[0].mEvents.push_back(std::make_pair(BFEVFLFile::EventType::Fork, Fork));
            mEventFile.mFlowcharts[0].mEvents.push_back(std::make_pair(BFEVFLFile::EventType::Join, Join));

            mNodes.push_back(std::make_unique<UIEventEditorFork>(std::get<BFEVFLFile::ForkEvent>(mEventFile.mFlowcharts[0].mEvents[mEventFile.mFlowcharts[0].mEvents.size() - 2].second), mId));
            mId += 1000;
            mNodes.push_back(std::make_unique<UIEventEditorJoin>(std::get<BFEVFLFile::JoinEvent>(mEventFile.mFlowcharts[0].mEvents.back().second), mId));
            mId += 1000;

            for (size_t i = 0; i < mEventFile.mFlowcharts[0].mEntryPoints.mItems.size(); i++)
            {
                static_cast<UIEventEditorEntryPoint*>(mNodes[i].get())->mEntryPoint = &mEventFile.mFlowcharts[0].mEntryPoints.mItems[i];
            }

            for (size_t i = 0; i < mEventFile.mFlowcharts[0].mEvents.size(); i++)
            {
                switch (mEventFile.mFlowcharts[0].mEvents[i].first)
                {
                case BFEVFLFile::EventType::SubFlow:
                    static_cast<UIEventEditorSubFlow*>(mNodes[mEventFile.mFlowcharts[0].mEntryPoints.mItems.size() + i].get())->mSubFlow = &std::get<BFEVFLFile::SubFlowEvent>(mEventFile.mFlowcharts[0].mEvents[i].second);
                    break;
                case BFEVFLFile::EventType::Action:
                    static_cast<UIEventEditorAction*>(mNodes[mEventFile.mFlowcharts[0].mEntryPoints.mItems.size() + i].get())->mAction = &std::get<BFEVFLFile::ActionEvent>(mEventFile.mFlowcharts[0].mEvents[i].second);
                    break;
                case BFEVFLFile::EventType::Fork:
                    static_cast<UIEventEditorFork*>(mNodes[mEventFile.mFlowcharts[0].mEntryPoints.mItems.size() + i].get())->mFork = &std::get<BFEVFLFile::ForkEvent>(mEventFile.mFlowcharts[0].mEvents[i].second);
                    break;
                case BFEVFLFile::EventType::Join:
                    static_cast<UIEventEditorJoin*>(mNodes[mEventFile.mFlowcharts[0].mEntryPoints.mItems.size() + i].get())->mJoin = &std::get<BFEVFLFile::JoinEvent>(mEventFile.mFlowcharts[0].mEvents[i].second);
                    break;
                case BFEVFLFile::EventType::Switch:
                    static_cast<UIEventEditorSwitch*>(mNodes[mEventFile.mFlowcharts[0].mEntryPoints.mItems.size() + i].get())->mSwitch = &std::get<BFEVFLFile::SwitchEvent>(mEventFile.mFlowcharts[0].mEvents[i].second);
                    break;
                default:
                    break;
                }
            }
        }
        ImGui::EndPopup();
    }

    if (ed::GetSelectedLinks(&ContextLinkId, 1) == 0)
        ContextLinkId = 0;
    if (ed::GetSelectedNodes(&ContextNodeId, 1) == 0)
        ContextNodeId = 0;

    ed::Resume();
}

void UIEventEditor::DrawEventEditorWindow()
{
    if (!mOpen)
        return;

    mFocused = ImGui::Begin("Event Editor", &mOpen);

    if (mFocused)
    {
        ImGui::Separator();

        ed::SetCurrentEditor(mContext);
        ed::Begin("EventEditor", ImVec2(0.0, 0.0f));

        for (auto& Node : mNodes)
        {
            Node->Render();
        }

        for (auto& Node : mNodes)
        {
            Node->DrawLinks();
        }

        ManageLinkCreationDeletion();

        ed::End();
        ed::SetCurrentEditor(nullptr);
    }
    ImGui::End();
}

void UIEventEditor::DrawPropertiesWindowContent()
{
    if (mEventFile.mLoaded && !mEventFile.mFlowcharts.empty())
    {
        ImGui::Text("Actors");
        ImGui::Separator();
        for (BFEVFLFile::Actor& Actor : mEventFile.mFlowcharts[0].mActors)
        {
            if (ImGui::CollapsingHeader(Actor.mName.c_str()))
            {
                ImGui::Indent();

                ImGui::Text("Actions");
                ImGui::Separator();
                ImGui::Indent();
                if (ImGui::Button(("Add##Action" + Actor.mName).c_str()))
                {
                    PopupAddActorActionQuery::Open(Actor.mName, true, [&Actor](std::string ActionName)
                        {
                            if (std::find(Actor.mActions.begin(), Actor.mActions.end(), ActionName) == Actor.mActions.end())
                            {
                                Actor.mActions.push_back(ActionName);
                            }
                        });
                }
                ImGui::Columns(2, "EventActorActionQuery");
                if (mPropertiesFirstTime)
                {
                    ImGui::SetColumnWidth(0, ImGui::GetColumnWidth(0) * 1.5f);
                    mPropertiesFirstTime = false;
                }
                for (auto Iter = Actor.mActions.begin(); Iter != Actor.mActions.end(); )
                {
                    ImGui::Text(Iter->c_str());
                    ImGui::NextColumn();
                    if (StarImGui::ButtonDelete(("Del##" + *Iter).c_str(), ImVec2(ImGui::GetColumnWidth(1) - ImGui::GetStyle().ItemSpacing.x - ImGui::GetStyle().ScrollbarSize, 0)))
                    {
                        bool Used = false;
                        for (auto& [Type, Event] : mEventFile.mFlowcharts[0].mEvents)
                        {
                            if (std::holds_alternative<BFEVFLFile::ActionEvent>(Event))
                            {
                                BFEVFLFile::ActionEvent& ActionEvent = std::get<BFEVFLFile::ActionEvent>(Event);
                                if (*Iter == mEventFile.mFlowcharts[0].mActors[ActionEvent.mActorIndex].mActions[ActionEvent.mActorActionIndex])
                                {
                                    Used = true;
                                    break;
                                }
                            }
                        }
                        if (!Used)
                        {
                            Iter = Actor.mActions.erase(Iter);
                            ImGui::NextColumn();
                            continue;
                        }
                        Logger::Warning("UIEventEditor", "Could not remove action, it is still used in an event action node");
                    }
                    ImGui::NextColumn();
                    Iter++;
                }
                ImGui::Columns();
                ImGui::Unindent();

                ImGui::Text("Queries");
                ImGui::Separator();
                ImGui::Indent();

                if (ImGui::Button(("Add##Query" + Actor.mName).c_str()))
                {
                    PopupAddActorActionQuery::Open(Actor.mName, false, [&Actor](std::string QueryName)
                        {
                            if (std::find(Actor.mQueries.begin(), Actor.mQueries.end(), QueryName) == Actor.mQueries.end())
                            {
                                Actor.mQueries.push_back(QueryName);
                            }
                        });
                }

                ImGui::Columns(2, "EventActorActionQuery");
                for (auto Iter = Actor.mQueries.begin(); Iter != Actor.mQueries.end(); )
                {
                    ImGui::Text(Iter->c_str());
                    ImGui::NextColumn();
                    if (StarImGui::ButtonDelete(("Del##" + *Iter).c_str(), ImVec2(ImGui::GetColumnWidth(1) - ImGui::GetStyle().ItemSpacing.x - ImGui::GetStyle().ScrollbarSize, 0)))
                    {
                        bool Used = false;
                        for (auto& [Type, Event] : mEventFile.mFlowcharts[0].mEvents)
                        {
                            if (std::holds_alternative<BFEVFLFile::SwitchEvent>(Event))
                            {
                                BFEVFLFile::SwitchEvent& SwitchEvent = std::get<BFEVFLFile::SwitchEvent>(Event);
                                if (*Iter == mEventFile.mFlowcharts[0].mActors[SwitchEvent.mActorIndex].mQueries[SwitchEvent.mActorQueryIndex])
                                {
                                    Used = true;
                                    break;
                                }
                            }
                        }

                        if (!Used)
                        {
                            Iter = Actor.mQueries.erase(Iter);
                            ImGui::NextColumn();
                            continue;
                        }
                        Logger::Warning("UIEventEditor", "Could not remove query, it is still used in an event switch node");
                    }
                    ImGui::NextColumn();
                    Iter++;
                }
                ImGui::Columns();
                ImGui::Unindent();

                ImGui::Unindent();
            }
        }
    }
}