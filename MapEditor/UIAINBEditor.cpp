#include "UIAINBEditor.h"

#include "AINBNodeDefMgr.h"
#include "Editor.h"
#include "Logger.h"
#include "PopupAINBElementSelector.h"
#include "PopupAddAINBNode.h"
#include "Util.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_stdlib.h"
#include "tinyfiledialogs.h"
#include <algorithm>
#include <filesystem>
#include <functional>
#include <iostream>
#include <unordered_map>
#include "UIAINBNodeDefault.h"
#include "UIAINBNodeSequential.h"
#include "UIAINBNodeSimultaneous.h"
#include "UIAINBNodeBoolSelector.h"
#include "UIAINBNodeSplitTiming.h"
#include "UIAINBNodeS32Selector.h"
#include "StarImGui.h"
#include <memory>

bool UIAINBEditor::Focused = false;
bool UIAINBEditor::FocusOnZero = false;
bool UIAINBEditor::Open = true;
bool UIAINBEditor::RunAutoLayout = false;
ed::EditorContext* UIAINBEditor::Context = nullptr;
AINBFile UIAINBEditor::AINB;
void (*UIAINBEditor::SaveCallback)() = nullptr;
TextureMgr::Texture* UIAINBEditor::HeaderTexture;
bool UIAINBEditor::EnableFlow = false;
ed::LinkId UIAINBEditor::ContextLinkId;
ed::NodeId UIAINBEditor::ContextNodeId;
uint32_t UIAINBEditor::Id = 1000;

std::unordered_map<uint32_t, bool> CommandHeaderOpen;
std::unordered_map<uint32_t, bool> GlobalParamHeaderOpen;

std::vector<std::unique_ptr<UIAINBNodeBase>> UIAINBEditor::Nodes;

void UIAINBEditor::UpdateLinkedParameters()
{
    for (auto& Node : Nodes) {
        Node->ClearParameterIds();
    }

    for (int i = 0; i < AINB.Nodes.size(); i++) {
        for (int j = 0; j < AINBFile::ValueTypeCount; j++) {
            for (AINBFile::InputEntry& Entry : AINB.Nodes[i].InputParameters[j]) {
                if (Entry.NodeIndex >= 0) {
                    Nodes[Entry.NodeIndex]->mLinkedOutputParams[j].push_back(Entry.ParameterIndex);
                }
                else {
                    for (AINBFile::MultiEntry& Multi : Entry.Sources) {
                        Nodes[Multi.NodeIndex]->mLinkedOutputParams[j].push_back(Multi.ParameterIndex);
                    }
                }
            }
        }
    }
}

void UIAINBEditor::AddNode(AINBFile::Node& Node, uint32_t Id)
{
    switch (Node.Type) {
    case (uint16_t)AINBFile::NodeTypes::UserDefined:
        Nodes.push_back(std::make_unique<UIAINBNodeDefault>(Node, Id, (TextureMgr::Texture&)*HeaderTexture, EnableFlow));
        break;
    case (uint16_t)AINBFile::NodeTypes::Element_Sequential:
        Nodes.push_back(std::make_unique<UIAINBNodeSequential>(Node, Id, (TextureMgr::Texture&)*HeaderTexture, EnableFlow));
        break;
    case (uint16_t)AINBFile::NodeTypes::Element_Simultaneous:
        Nodes.push_back(std::make_unique<UIAINBNodeSimultaneous>(Node, Id, (TextureMgr::Texture&)*HeaderTexture, EnableFlow));
        break;
    case (uint16_t)AINBFile::NodeTypes::Element_BoolSelector:
        Nodes.push_back(std::make_unique<UIAINBNodeBoolSelector>(Node, Id, (TextureMgr::Texture&)*HeaderTexture, EnableFlow));
        break;
    case (uint16_t)AINBFile::NodeTypes::Element_SplitTiming:
        Nodes.push_back(std::make_unique<UIAINBNodeSplitTiming>(Node, Id, (TextureMgr::Texture&)*HeaderTexture, EnableFlow));
        break;
    case (uint16_t)AINBFile::NodeTypes::Element_S32Selector:
        Nodes.push_back(std::make_unique<UIAINBNodeS32Selector>(Node, Id, (TextureMgr::Texture&)*HeaderTexture, EnableFlow));
        break;
    default:
        Nodes.push_back(std::make_unique<UIAINBNodeDefault>(Node, Id, (TextureMgr::Texture&)*HeaderTexture, EnableFlow));
        break;
    }

    /* Updating visual node pointers */
    int NodeOffset = 0;
    for (int i = 0; i < Nodes.size(); i++) {
        if (Nodes[i]->GetNodeType() == UIAINBNodeBase::NodeType::Virtual) {
            NodeOffset++;
            continue;
        }
        Nodes[i]->mNode = &AINB.Nodes[i - NodeOffset];
    }
}

void UIAINBEditor::LoadAINBFile(std::string Path, bool AbsolutePath)
{
    LoadAINBFile(Util::ReadFile(AbsolutePath ? Path : Editor::GetRomFSFile(Path)));
}

void UIAINBEditor::LoadAINBFile(std::vector<unsigned char> Bytes)
{
    SaveCallback = nullptr;
    Id = 1000;
    AINB = AINBFile(Bytes);

    if (AINB.Nodes.size() > 42'949'600) {
        Logger::Error("AINBEditor", "AINB has over 42.949.600 nodes, something went clearly wrong");
        return;
    }

    EnableFlow = AINB.Header.FileCategory != "Logic";

    Nodes.clear();

    for (AINBFile::Node& Node : AINB.Nodes) {
        AddNode(Node, Id);
        Id += 1000;
    }
}

void UIAINBEditor::Initialize()
{
    ed::Config Config;
    Config.SettingsFile = nullptr;
    Context = ed::CreateEditor(&Config);

    HeaderTexture = TextureMgr::GetTexture("BlueprintBackground");
}

void UIAINBEditor::Delete()
{
    ed::DestroyEditor(Context);
}

void UIAINBEditor::ManageLinkCreationDeletion()
{
    if (ed::BeginCreate(ImColor(255, 255, 255), 2.0f)) {
        auto ShowLabel = [](const char* label, ImColor color) {
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

        auto FindPin = [](uint32_t Id) {
            for (auto& Node : Nodes) {
                if (Node->mPins.contains(Id)) {
                    return &Node->mPins[Id];
                }
            }
            return (UIAINBNodeBase::Pin*)nullptr;
            };

        auto FindNodeIndex = [](AINBFile::Node* Parent) {
            for (int i = 0; i < AINB.Nodes.size(); i++) {
                if (&AINB.Nodes[i] == Parent) {
                    return i;
                }
            }
            return -1;
            };

        ed::PinId StartPinId, EndPinId = 0;
        if (ed::QueryNewLink(&StartPinId, &EndPinId)) {
            UIAINBNodeBase::Pin* StartPin = FindPin(StartPinId.Get());
            UIAINBNodeBase::Pin* EndPin = FindPin(EndPinId.Get());

            if (StartPin != nullptr && EndPin != nullptr) {
                if (StartPin->Kind == ed::PinKind::Input) {
                    std::swap(StartPin, EndPin);
                    std::swap(StartPinId, EndPinId);
                }

                if (StartPin == EndPin) {
                    ed::RejectNewItem(ImColor(255, 0, 0), 2.0f);
                }
                else if (StartPin->Kind == EndPin->Kind) {
                    ShowLabel("x Incompatible Pin Kind", ImColor(45, 32, 32, 180));
                    ed::RejectNewItem(ImColor(255, 0, 0), 2.0f);
                }
                else if (StartPin->Type != EndPin->Type) {
                    ShowLabel("x Incompatible Pin Type", ImColor(45, 32, 32, 180));
                    ed::RejectNewItem(ImColor(255, 128, 128), 1.0f);
                }
                else if (std::abs((int)StartPinId.Get() - (int)EndPinId.Get()) < 500) {
                    ShowLabel("x Can't link to self", ImColor(45, 32, 32, 180));
                    ed::RejectNewItem(ImColor(255, 0, 0), 1.0f);
                }
                else {
                    int16_t NodeIndex = FindNodeIndex(StartPin->Node);

                    if (EndPin->Type != UIAINBNodeBase::PinType::Flow) {
                        AINBFile::InputEntry& Input = *(AINBFile::InputEntry*)EndPin->ObjectPtr;
                        //AINBFile::OutputEntry& Output = *(AINBFile::OutputEntry*)StartPin->ObjectPtr;
                        if (Input.NodeIndex == NodeIndex && Input.ParameterIndex == StartPin->ParameterIndex) {
                            ShowLabel("x Already linked", ImColor(45, 32, 32, 180));
                            ed::RejectNewItem(ImColor(255, 128, 128), 1.0f);
                            ed::EndCreate();
                            return;
                        }
                        else {
                            for (AINBFile::MultiEntry& Entry : Input.Sources) {
                                if (Entry.NodeIndex == NodeIndex && Entry.ParameterIndex == StartPin->ParameterIndex) {
                                    ShowLabel("x Already linked", ImColor(45, 32, 32, 180));
                                    ed::RejectNewItem(ImColor(255, 128, 128), 1.0f);
                                    ed::EndCreate();
                                    return;
                                }
                            }
                        }
                    }
                    else { //Flow checks
                        if (StartPin->AlreadyLinked && !StartPin->AllowMultipleLinks) {
                            ShowLabel("x Flow parameter already linked", ImColor(45, 32, 32, 180));
                            ed::RejectNewItem(ImColor(255, 128, 128), 1.0f);
                            ed::EndCreate();
                            return;
                        }
                    }

                    ShowLabel("+ Create Link", ImColor(32, 45, 32, 180));
                    if (ed::AcceptNewItem(ImColor(128, 255, 128), 4.0f)) {
                        //Parameter link
                        if (EndPin->Type != UIAINBNodeBase::PinType::Flow) {
                            AINBFile::InputEntry& Input = *(AINBFile::InputEntry*)EndPin->ObjectPtr;
                            AINBFile::OutputEntry& Output = *(AINBFile::OutputEntry*)StartPin->ObjectPtr;

                            //Input param has not been linked yet
                            if (Input.NodeIndex < 0 && Input.Sources.empty()) {
                                Input.ParameterIndex = StartPin->ParameterIndex;
                                Input.NodeIndex = NodeIndex;
                            }
                            else if (Input.NodeIndex >= 0) {
                                Input.Sources.clear();
                                Input.Sources.push_back(AINBFile::MultiEntry{
                                    .NodeIndex = (uint16_t)Input.NodeIndex,
                                    .ParameterIndex = (uint16_t)Input.ParameterIndex,
                                    .Flags = Input.Flags,
                                    .GlobalParametersIndex = Input.GlobalParametersIndex,
                                    .EXBIndex = Input.EXBIndex,
                                    .Function = Input.Function
                                    });

                                Input.NodeIndex = -1;
                                Input.ParameterIndex = 0;
                                Input.Flags.clear();
                                Input.GlobalParametersIndex = 0xFFFF;
                                Input.EXBIndex = 0xFFFF;

                                Input.Sources.push_back(AINBFile::MultiEntry{
                                    .NodeIndex = (uint16_t)NodeIndex,
                                    .ParameterIndex = (uint16_t)StartPin->ParameterIndex
                                    });
                            }
                            else {
                                Input.Sources.push_back(AINBFile::MultiEntry{
                                    .NodeIndex = (uint16_t)NodeIndex,
                                    .ParameterIndex = (uint16_t)StartPin->ParameterIndex });
                            }
                        }
                        else { //Flow Link
                            AINBFile::LinkedNodeInfo Info;
                            Info.NodeIndex = EndPin->Node->NodeIndex;
                            Info.Condition = "";
                            Nodes[StartPin->Node->NodeIndex]->PostProcessLinkedNodeInfo(*StartPin, Info);
                            StartPin->Node->LinkedNodes[(int)AINBFile::LinkedNodeMapping::StandardLink].push_back(Info);
                        }

                    }
                }
            }
        }
    }

    ed::EndCreate();

    ImVec2 OpenPopupPosition = ImGui::GetMousePos();
    ed::Suspend();
    if (ed::ShowLinkContextMenu(&ContextLinkId)) {
        ImGui::OpenPopup("Link Context Menu");
    }
    if (ed::ShowNodeContextMenu(&ContextNodeId)) {
        ImGui::OpenPopup("Node Context Menu");
    }

    if (ImGui::BeginPopup("Link Context Menu")) {
        if (ImGui::MenuItem("Delete##DelLink")) {
            DeleteNodeLink(ContextLinkId);
        }
        ImGui::EndPopup();
    }
    if (ImGui::BeginPopup("Node Context Menu")) {
        if (ImGui::MenuItem("Delete##DelNode")) {
            UIAINBEditor::DeleteNode(ContextNodeId);
        }
        ImGui::EndPopup();
    }

    if (ed::GetSelectedLinks(&ContextLinkId, 1) == 0)
        ContextLinkId = 0;
    if (ed::GetSelectedNodes(&ContextNodeId, 1) == 0)
        ContextNodeId = 0;

    ed::Resume();
}

void UIAINBEditor::DeleteNodeLink(ed::LinkId LinkId)
{
    for (auto& Node : Nodes) {
        if (Node->mLinks.contains(LinkId.Get())) {
            UIAINBNodeBase::Link& Link = Node->mLinks[LinkId.Get()];

            if (Link.Type == UIAINBNodeBase::LinkType::Parameter) {
                AINBFile::InputEntry* Parameter = (AINBFile::InputEntry*)Link.ObjectPtr;
                if (Parameter->NodeIndex >= 0) {
                    Parameter->NodeIndex = -1;
                    Parameter->ParameterIndex = 0;
                }
                else
                {
                    Parameter->Sources.erase(std::remove_if(Parameter->Sources.begin(), Parameter->Sources.end(),
                        [Link](AINBFile::MultiEntry& Entry) { return Entry.NodeIndex == Link.NodeIndex && Entry.ParameterIndex == Link.ParameterIndex; }),
                        Parameter->Sources.end());

                    if (Parameter->Sources.size() == 1) {
                        Parameter->NodeIndex = Parameter->Sources[0].NodeIndex;
                        Parameter->ParameterIndex = Parameter->Sources[0].ParameterIndex;
                        Parameter->GlobalParametersIndex = Parameter->Sources[0].GlobalParametersIndex;
                        Parameter->EXBIndex = Parameter->Sources[0].EXBIndex;
                        Parameter->Flags = Parameter->Sources[0].Flags;
                        Parameter->Function = Parameter->Sources[0].Function;
                        Parameter->Sources.clear();
                    }
                }
            }
            else if (Link.Type == UIAINBNodeBase::LinkType::Flow)
            {
                AINBFile::Node* Node = (AINBFile::Node*)Link.ObjectPtr;
                Node->LinkedNodes[(int)AINBFile::LinkedNodeMapping::StandardLink].erase(Node->LinkedNodes[(int)AINBFile::LinkedNodeMapping::StandardLink].begin() + Link.ParameterIndex);
            }

            ed::DeleteLink(LinkId);

            break;
        }
    }
}

void UIAINBEditor::DeleteNode(ed::NodeId NodeId)
{
    AINBFile::Node* NodePtr = nullptr;
    Nodes.erase(std::remove_if(Nodes.begin(), Nodes.end(),
        [NodeId, &NodePtr](std::unique_ptr<UIAINBNodeBase>& NodeBase) {
            bool Delete = NodeBase->mEditorId == NodeId.Get();
            if (Delete)
            {
                NodePtr = NodeBase->mNode;
            }
            return Delete;
        }),
        Nodes.end());

    if (NodePtr == nullptr)
        return;

    uint32_t NodeIndex = NodePtr->NodeIndex;

    AINB.Nodes.erase(std::remove_if(AINB.Nodes.begin(), AINB.Nodes.end(),
        [NodeId, NodePtr](AINBFile::Node& Node) { return &Node == NodePtr; }),
        AINB.Nodes.end());
    ed::DeleteNode(NodeId);

    NodePtr = nullptr;

    for (size_t i = 0; i < Nodes.size(); i++)
    {
        Nodes[i]->mNode = &AINB.Nodes[i];
    }

    for (auto& Node : Nodes)
    {
        for (int ValueType = 0; ValueType < AINBFile::ValueTypeCount; ValueType++)
        {
            for (AINBFile::InputEntry& Entry : Node->mNode->InputParameters[ValueType])
            {
                if (Entry.NodeIndex == NodeIndex)
                {
                    Entry.ParameterIndex = 0;
                    Entry.NodeIndex = -1;
                }
                if (Entry.NodeIndex > NodeIndex && Entry.NodeIndex >= 0)
                {
                    Entry.NodeIndex--;
                }
                for (auto Iter = Entry.Sources.begin(); Iter != Entry.Sources.end(); )
                {
                    if (Iter->NodeIndex == NodeIndex)
                    {
                        Iter = Entry.Sources.erase(Iter);
                        continue;
                    }
                    if (Iter->NodeIndex > NodeIndex)
                    {
                        Iter->NodeIndex--;
                    }
                    Iter++;
                }
                if (Entry.Sources.size() == 1) {
                    Entry.NodeIndex = Entry.Sources[0].NodeIndex;
                    Entry.ParameterIndex = Entry.Sources[0].ParameterIndex;
                    Entry.GlobalParametersIndex = Entry.Sources[0].GlobalParametersIndex;
                    Entry.EXBIndex = Entry.Sources[0].EXBIndex;
                    Entry.Flags = Entry.Sources[0].Flags;
                    Entry.Function = Entry.Sources[0].Function;
                    Entry.Sources.clear();
                }
            }
        }
        for (int LinkedInfoType = 0; LinkedInfoType < AINBFile::LinkedNodeTypeCount; LinkedInfoType++)
        {
            for (auto Iter = Node->mNode->LinkedNodes[LinkedInfoType].begin(); Iter != Node->mNode->LinkedNodes[LinkedInfoType].end();)
            {
                if (Iter->NodeIndex == NodeIndex)
                {
                    Iter = Node->mNode->LinkedNodes[LinkedInfoType].erase(Iter);
                    continue;
                }
                if (Iter->NodeIndex > NodeIndex)
                {
                    Iter->NodeIndex--;
                }
                Iter++;
            }
        }
    }

    //Updating node indexes
    for (uint32_t i = 0; i < AINB.Nodes.size(); i++)
    {
        AINB.Nodes[i].NodeIndex = i;
    }
}

void UIAINBEditor::DrawAinbEditorWindow()
{
    if (!Open)
        return;

    UIAINBEditor::Focused = ImGui::Begin("AINB Editor", &Open);

    /* Button Placeholder */

    bool WantAutoLayout = ImGui::Button("Auto layout");
    ImGui::SameLine();
    if (ImGui::Button("Add node")) {
        PopupAddAINBNode::Open([](std::string Name) {
            AINBFile::Node NewNode = AINBNodeDefMgr::NodeDefToNode(AINBNodeDefMgr::GetNodeDefinition(Name));
            NewNode.NodeIndex = AINB.Nodes.size();
            AINB.Nodes.push_back(NewNode);
            AddNode(AINB.Nodes.back(), Id);
            Id += 1000;
            Nodes.back()->RebuildNode();

            //for (std::string FileName : AINBNodeDefMgr::GetNodeDefinition(Name)->FileNames) {
                //Logger::Info("AINBEditor", "AINB that uses " + NewNode.GetName() + ": " + FileName);
            //}
            });
    }

    if (ImGui::Button("Open AINB (Vanilla)")) {
        const char* Path = tinyfd_openFileDialog("Open AINB (Vanilla)", (Editor::RomFSDir + "\\Logic").c_str(), 0, nullptr, nullptr, 0);
        if (Path != nullptr) {
            LoadAINBFile(Path, true);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Open AINB (Custom)")) {
        Util::CreateDir(Editor::GetWorkingDirFile("Save/Logic"));
        const char* Path = tinyfd_openFileDialog("Open AINB (Custom)", (std::filesystem::current_path().string() + "\\WorkingDir\\Save\\Logic").c_str(), 0, nullptr, nullptr, 0);
        if (Path != nullptr) {
            LoadAINBFile(Path, true);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Import AINB (Vanilla)"))
    {
        const char* Path = tinyfd_openFileDialog("Import AINB (Vanilla)", (Editor::RomFSDir + "\\Logic").c_str(), 0, nullptr, nullptr, 0);
        if (Path != nullptr) {
            AINBFile File(Util::ReadFile(std::string(Path)));
            uint32_t NodeIndexOffset = Nodes.size();
            for (AINBFile::Node Node : File.Nodes)
            {
                for (int ValueType = 0; ValueType < AINBFile::ValueTypeCount; ValueType++)
                {
                    for (AINBFile::InputEntry& Param : Node.InputParameters[ValueType])
                    {
                        if (Param.NodeIndex >= 0)
                        {
                            Param.NodeIndex += NodeIndexOffset;
                        }
                        for (AINBFile::MultiEntry& Entry : Param.Sources)
                        {
                            Entry.NodeIndex += NodeIndexOffset;
                        }
                    }
                }

                for (int LinkedInfoType = 0; LinkedInfoType < AINBFile::LinkedNodeTypeCount; LinkedInfoType++)
                {
                    for (AINBFile::LinkedNodeInfo& Info : Node.LinkedNodes[LinkedInfoType])
                    {
                        Info.NodeIndex += NodeIndexOffset;
                        if (Info.ReplacementNodeIndex >= 0 && Info.ReplacementNodeIndex != 0xFFFF)
                        {
                            Info.ReplacementNodeIndex += NodeIndexOffset;
                        }
                    }
                }
                Node.NodeIndex += NodeIndexOffset;
                AINB.Nodes.push_back(Node);
                AddNode(Node, Id);
                Id += 1000;
            }
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Import AINB (Custom)"))
    {
        const char* Path = tinyfd_openFileDialog("Import AINB (Custom)", (std::filesystem::current_path().string() + "\\WorkingDir\\Save\\Logic").c_str(), 0, nullptr, nullptr, 0);
        if (Path != nullptr) {
            AINBFile File(Util::ReadFile(std::string(Path)));
            uint32_t NodeIndexOffset = Nodes.size();
            for (AINBFile::Node Node : File.Nodes)
            {
                for (int ValueType = 0; ValueType < AINBFile::ValueTypeCount; ValueType++)
                {
                    for (AINBFile::InputEntry& Param : Node.InputParameters[ValueType])
                    {
                        if (Param.NodeIndex >= 0)
                        {
                            Param.NodeIndex += NodeIndexOffset;
                        }
                        for (AINBFile::MultiEntry& Entry : Param.Sources)
                        {
                            Entry.NodeIndex += NodeIndexOffset;
                        }
                    }
                }

                for (int LinkedInfoType = 0; LinkedInfoType < AINBFile::LinkedNodeTypeCount; LinkedInfoType++)
                {
                    for (AINBFile::LinkedNodeInfo& Info : Node.LinkedNodes[LinkedInfoType])
                    {
                        Info.NodeIndex += NodeIndexOffset;
                        if (Info.ReplacementNodeIndex >= 0 && Info.ReplacementNodeIndex != 0xFFFF)
                        {
                            Info.ReplacementNodeIndex += NodeIndexOffset;
                        }
                    }
                }
                Node.NodeIndex += NodeIndexOffset;
                AINB.Nodes.push_back(Node);
                AddNode(Node, Id);
                Id += 1000;
            }
        }
    }

    ImGui::Separator();

    ed::SetCurrentEditor(Context);
    ed::Begin("AINBEditor", ImVec2(0.0, 0.0f));

    UpdateLinkedParameters();

    //Rendering
    for (auto& Node : Nodes) {
        Node->Render();
        Node->mFlowLinked = false;
    }
    // Drawing links
    for (auto& Node : Nodes) {
        Node->RenderLinks(Nodes);
    }

    //Selecting

    //Link creation/deletion
    ManageLinkCreationDeletion();

    if (WantAutoLayout || RunAutoLayout) {
        RunAutoLayout = false;
        std::unordered_map<AINBFile::Node*, std::pair<int, int>> NodeToPos;
        std::unordered_map<int, std::unordered_map<int, AINBFile::Node*>> PosToNode;

        std::function<void(AINBFile::Node*, uint16_t, std::pair<int, int>)> PlaceNode = [&](AINBFile::Node* Node, uint16_t NodeIndex, std::pair<int, int> Pos) {
            if (NodeToPos.contains(Node)) {
                return;
            }
            while (PosToNode[Pos.first].contains(Pos.second)) {
                Pos.second++;
            }
            NodeToPos[Node] = Pos;
            PosToNode[Pos.first][Pos.second] = Node;

            uint16_t NewNodeIndex = 0;
            for (AINBFile::Node& LinkedNode : AINB.Nodes) {
                for (int j = 0; j < AINBFile::ValueTypeCount; j++) {
                    for (AINBFile::InputEntry Param : LinkedNode.InputParameters[j]) {
                        if (Param.NodeIndex == NodeIndex) {
                            PlaceNode(&LinkedNode, NewNodeIndex, { Pos.first + 1, Pos.second });
                        }
                        for (AINBFile::MultiEntry& Entry : Param.Sources) {
                            if (Entry.NodeIndex == NodeIndex) {
                                PlaceNode(&LinkedNode, NewNodeIndex, { Pos.first + 1, Pos.second });
                            }
                        }
                    }
                }
                NewNodeIndex++;
            }

            for (int NodeLinkType = 0; NodeLinkType < AINBFile::LinkedNodeTypeCount; NodeLinkType++) {
                for (AINBFile::LinkedNodeInfo& NodeLink : Node->LinkedNodes[NodeLinkType]) {
                    if (NodeLink.Type == AINBFile::LinkedNodeMapping::StandardLink) {
                        if (NodeLink.NodeIndex == -1 || NodeLink.NodeIndex >= AINB.Nodes.size())
                            continue;

                        PlaceNode(&AINB.Nodes[NodeLink.NodeIndex], NodeLink.NodeIndex, { Pos.first + 1, Pos.second });
                    }
                }
            }

            for (int j = 0; j < AINBFile::ValueTypeCount; j++) {
                for (AINBFile::InputEntry Param : Node->InputParameters[j]) {
                    if (Param.NodeIndex >= 0) {
                        PlaceNode(&AINB.Nodes[Param.NodeIndex], Param.NodeIndex, { Pos.first - 1, Pos.second });
                    }
                    for (AINBFile::MultiEntry& Entry : Param.Sources) {
                        PlaceNode(&AINB.Nodes[Entry.NodeIndex], Entry.NodeIndex, { Pos.first - 1, Pos.second });
                    }
                }
            }

            NewNodeIndex = 0;
            for (AINBFile::Node& LinkedNode : AINB.Nodes) {
                for (int NodeLinkType = 0; NodeLinkType < AINBFile::LinkedNodeTypeCount; NodeLinkType++) {
                    for (AINBFile::LinkedNodeInfo& NodeLink : LinkedNode.LinkedNodes[NodeLinkType]) {
                        if (NodeLink.Type == AINBFile::LinkedNodeMapping::StandardLink && NodeLink.NodeIndex == NodeIndex) {
                            PlaceNode(&LinkedNode, NewNodeIndex, { Pos.first - 1, Pos.second });
                        }
                    }
                }
                NewNodeIndex++;
            }
            };

        uint16_t NodeIdx = 0;
        for (AINBFile::Node& Node : AINB.Nodes) {
            if (!NodeToPos.count(&Node)) {
                PlaceNode(&Node, NodeIdx, { 0, 0 });
            }
            NodeIdx++;
        }

        for (auto& [Node, Pos] : NodeToPos) {
            ed::SetNodePosition(Nodes[Node->NodeIndex]->mEditorId, ImVec2(Pos.first * (600 * Editor::UIScale), Pos.second * (400 * Editor::UIScale)));
        }
    }

    if (FocusOnZero) {
        ed::NavigateToContent();
        FocusOnZero = false;
    }

    ed::End();
    ed::SetCurrentEditor(nullptr);

    ImGui::End();
}

void UIAINBEditor::DrawPropertiesWindowContent()
{
    ImGui::Text("General");
    ImGui::Separator();

    ImGui::Indent();
    ImGui::Columns(2);

    ImGui::Text("File name");
    ImGui::NextColumn();
    ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
    if (ImGui::InputText("##GeneralFileName", &AINB.Header.FileName)) {
        AINB.Loaded = true;
    }
    ImGui::PopItemWidth();
    ImGui::NextColumn();

    ImGui::Text("Category");
    ImGui::NextColumn();
    ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
    const char* CategoryDropdownItems[] = { "Logic", "AI", "Sequence" };
    int CategorySelected = 0;
    if (AINB.Header.FileCategory == "AI")
        CategorySelected = 1;
    else if (AINB.Header.FileCategory == "Sequence")
        CategorySelected = 2;
    if (ImGui::Combo("##GeneralCategory", &CategorySelected, CategoryDropdownItems, IM_ARRAYSIZE(CategoryDropdownItems))) {
        if (CategorySelected == 0)
            AINB.Header.FileCategory = "Logic";
        else if (CategorySelected == 1)
            AINB.Header.FileCategory = "AI";
        else if (CategorySelected == 2)
            AINB.Header.FileCategory = "Sequence";

        EnableFlow = CategorySelected != 0;
    }
    ImGui::PopItemWidth();

    ImGui::Columns();
    ImGui::Unindent();

    ImGui::NewLine();
    ImGui::Text("Commands");
    ImGui::Separator();
    if (ImGui::Button("Add command", ImVec2(ImGui::GetWindowSize().x - ImGui::GetStyle().FramePadding.x, 0))) {
        AINBFile::Command Command;
        Command.LeftNodeIndex = 0;
        Command.RightNodeIndex = -1;
        Command.Name = "DummyName" + std::to_string(AINB.Commands.size() + 1);
        Command.GUID.Part1 = AINB.Commands.size();
        AINB.Commands.push_back(Command);
        CommandHeaderOpen.insert({ Command.GUID.Part1, false });
    }
    for (std::vector<AINBFile::Command>::iterator Iter = AINB.Commands.begin(); Iter != AINB.Commands.end();) {
        ImGui::GetStateStorage()->SetInt(ImGui::GetID(("Command: " + Iter->Name + "##" + std::to_string(Iter->GUID.Part1)).c_str()), (int)CommandHeaderOpen[Iter->GUID.Part1]);
        if (ImGui::CollapsingHeader(("Command: " + Iter->Name + "##" + std::to_string(Iter->GUID.Part1)).c_str())) {
            ImGui::Columns(2, std::to_string(Iter->GUID.Part1).c_str());
            ImGui::Indent();

            ImGui::Text("Name");
            ImGui::NextColumn();
            ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
            ImGui::InputText(("##Name" + std::to_string(Iter->GUID.Part1)).c_str(), &Iter->Name);
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::Text("Left Node Index");
            ImGui::NextColumn();
            ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
            ImGui::InputScalar(("##LNI" + std::to_string(Iter->GUID.Part1)).c_str(), ImGuiDataType_::ImGuiDataType_S16, &Iter->LeftNodeIndex);
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::Text("Right Node Index");
            ImGui::NextColumn();
            ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
            ImGui::InputScalar(("##RNI" + std::to_string(Iter->GUID.Part1)).c_str(), ImGuiDataType_::ImGuiDataType_S16, &Iter->RightNodeIndex);
            ImGui::PopItemWidth();

            ImGui::Columns();

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5, 0.14, 0.14, 1));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.91, 0.12, 0.15, 1));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1, 0, 0, 1));
            if (ImGui::Button(("Delete##" + std::to_string(Iter->GUID.Part1)).c_str(), ImVec2(ImGui::GetWindowSize().x - ImGui::GetStyle().ScrollbarSize - ImGui::GetStyle().IndentSpacing, 0))) {
                CommandHeaderOpen.erase(Iter->GUID.Part1);
                ImGui::PopStyleColor(3);
                Iter = AINB.Commands.erase(Iter);
                ImGui::Unindent();
                continue;
            }
            ImGui::PopStyleColor(3);
            ImGui::Unindent();
            CommandHeaderOpen[Iter->GUID.Part1] = true;
        }
        else {
            CommandHeaderOpen[Iter->GUID.Part1] = false;
        }
        Iter++;
    }
    ImGui::NewLine();

    ImGui::Text("Global parameters");
    ImGui::Separator();
    if (ImGui::Button("Add global parameter", ImVec2(ImGui::GetWindowSize().x - ImGui::GetStyle().FramePadding.x, 0))) {
        AINBFile::GlobalEntry Entry;
        Entry.Name = "DummyName" + std::to_string(AINB.GlobalParameters[1].size() + 1);
        Entry.GlobalValueType = (int)AINBFile::GlobalType::Int;
        Entry.GlobalValue = (uint32_t)0;
        AINB.GlobalParameters[1].push_back(Entry);
        GlobalParamHeaderOpen.insert({ 500 + AINB.GlobalParameters[1].size() - 1, false });
    }
    for (int i = 0; i < AINB.GlobalParameters.size(); i++) {
        for (std::vector<AINBFile::GlobalEntry>::iterator Iter = AINB.GlobalParameters[i].begin(); Iter != AINB.GlobalParameters[i].end();) {
            uint32_t Index = std::distance(AINB.GlobalParameters[i].begin(), Iter);
            ImGui::GetStateStorage()->SetInt(ImGui::GetID(("Global parameter: " + Iter->Name + "##" + std::to_string(Index) + ":" + std::to_string(i)).c_str()), (int)GlobalParamHeaderOpen[(i * 500) + Index]);
            if (ImGui::CollapsingHeader(("Global parameter: " + Iter->Name + "##" + std::to_string(Index) + ":" + std::to_string(i)).c_str())) {
                ImGui::Columns(2, ("GlobalParam" + std::to_string(Index) + ":" + std::to_string(i)).c_str());
                ImGui::Indent();

                ImGui::Text("Data type");
                ImGui::NextColumn();
                ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
                const char* TypeDropdownItems[] = { "String", "Int", "Float", "Bool", "Vec3f", "UserDefined" };
                if (ImGui::Combo(("##DataTypeGP" + std::to_string(Index) + ":" + std::to_string(i)).c_str(), reinterpret_cast<int*>(&Iter->GlobalValueType), TypeDropdownItems, IM_ARRAYSIZE(TypeDropdownItems))) {
                    if (Iter->GlobalValueType == (int)AINBFile::GlobalType::String)
                        Iter->GlobalValue = "None";
                    if (Iter->GlobalValueType == (int)AINBFile::GlobalType::Int || Iter->GlobalValueType == (int)AINBFile::GlobalType::UserDefined)
                        Iter->GlobalValue = (uint32_t)0;
                    if (Iter->GlobalValueType == (int)AINBFile::GlobalType::Float)
                        Iter->GlobalValue = 0.0f;
                    if (Iter->GlobalValueType == (int)AINBFile::GlobalType::Bool)
                        Iter->GlobalValue = false;
                    if (Iter->GlobalValueType == (int)AINBFile::GlobalType::Vec3f)
                        Iter->GlobalValue = Vector3F(0.0f, 0.0f, 0.0f);
                }
                ImGui::PopItemWidth();
                ImGui::NextColumn();

                ImGui::Text("Name");
                ImGui::NextColumn();
                ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
                ImGui::InputText(("##NameGP" + std::to_string(Index) + ":" + std::to_string(i)).c_str(), &Iter->Name);
                ImGui::PopItemWidth();
                ImGui::NextColumn();

                ImGui::Text("Default value");
                ImGui::NextColumn();
                ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
                switch (Iter->GlobalValueType) {
                case (int)AINBFile::GlobalType::String: {
                    ImGui::InputText(("##DefaultValGP" + std::to_string(Index) + ":" + std::to_string(i)).c_str(), reinterpret_cast<std::string*>(&Iter->GlobalValue));
                    break;
                }
                case (int)AINBFile::GlobalType::Bool: {
                    ImGui::Checkbox(("##DefaultValGP" + std::to_string(Index) + ":" + std::to_string(i)).c_str(), reinterpret_cast<bool*>(&Iter->GlobalValue));
                    break;
                }
                case (int)AINBFile::GlobalType::UserDefined:
                case (int)AINBFile::GlobalType::Int: {
                    ImGui::InputScalar(("##DefaultValGP" + std::to_string(Index) + ":" + std::to_string(i)).c_str(), ImGuiDataType_::ImGuiDataType_U32, reinterpret_cast<uint32_t*>(&Iter->GlobalValue));
                    break;
                }
                case (int)AINBFile::GlobalType::Float: {
                    ImGui::InputScalar(("##DefaultValGP" + std::to_string(Index) + ":" + std::to_string(i)).c_str(), ImGuiDataType_::ImGuiDataType_Float, reinterpret_cast<float*>(&Iter->GlobalValue));
                    break;
                }
                case (int)AINBFile::GlobalType::Vec3f: {
                    ImGui::InputFloat3(("##DefaultValGP" + std::to_string(Index) + ":" + std::to_string(i)).c_str(), reinterpret_cast<Vector3F*>(&Iter->GlobalValue)->GetRawData());
                    break;
                }
                default:
                    ImGui::Text("Unknown data type");
                    break;
                }
                ImGui::PopItemWidth();
                ImGui::NextColumn();

                ImGui::Text("Notes");
                ImGui::NextColumn();
                ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
                ImGui::InputText(("##NotesGP" + std::to_string(Index) + ":" + std::to_string(i)).c_str(), &Iter->Notes);
                ImGui::PopItemWidth();
                ImGui::NextColumn();

                ImGui::Text("File Reference");
                ImGui::NextColumn();
                ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
                ImGui::InputText(("##FileRefGP" + std::to_string(Index) + ":" + std::to_string(i)).c_str(), &Iter->FileReference.FileName);
                ImGui::PopItemWidth();

                ImGui::Columns();

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5, 0.14, 0.14, 1));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.91, 0.12, 0.15, 1));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1, 0, 0, 1));
                if (ImGui::Button(("Delete##DelGP" + std::to_string(Index) + ":" + std::to_string(i)).c_str(), ImVec2(ImGui::GetWindowSize().x - ImGui::GetStyle().ScrollbarSize - ImGui::GetStyle().IndentSpacing, 0))) {
                    GlobalParamHeaderOpen.erase((i * 500) + Index);
                    ImGui::PopStyleColor(3);
                    Iter = AINB.GlobalParameters[i].erase(Iter);
                    ImGui::Unindent();
                    continue;
                }
                ImGui::PopStyleColor(3);

                ImGui::Unindent();

                GlobalParamHeaderOpen[(i * 500) + Index] = true;
            }
            else {
                GlobalParamHeaderOpen[(i * 500) + Index] = false;
            }
            Iter++;
        }
    }
}

void UIAINBEditor::Save()
{
    if (AINB.Loaded) {
        for (std::unique_ptr<UIAINBNodeBase>& Node : Nodes)
        {
            Node->PostProcessNode();
        }
        if (SaveCallback != nullptr) {
            SaveCallback();
            return;
        }

        Util::CreateDir(Editor::GetWorkingDirFile("Save/" + AINB.Header.FileCategory));
        AINB.Write(Editor::GetWorkingDirFile("Save/" + AINB.Header.FileCategory + "/" + AINB.Header.FileName + ".ainb"));

        GlobalParamHeaderOpen.clear();
        for (uint32_t i = 0; i < AINB.GlobalParameters.size(); i++) {
            for (uint32_t j = 0; j < AINB.GlobalParameters[i].size(); j++) {
                GlobalParamHeaderOpen.insert({ (i * 500) + j, false }); // Max 500 global params per data type
            }
        }
    }
}