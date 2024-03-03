#include "UIAINBEditor.h"

#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_stdlib.h"
#include "imgui_internal.h"
#include "Editor.h"
#include <unordered_map>
#include "Logger.h"
#include "Util.h"
#include <functional>
#include "PopupAddAINBNode.h"
#include "AINBNodeDefMgr.h"
#include "tinyfiledialogs.h"
#include "PopupAINBElementSelector.h"
#include <filesystem>

#define MinImmTextboxWidth 150

/*
namespace UIAinbEditor
{
	extern bool Open;

	void DrawAinbEditorWindow();
};
*/

struct NodeShapeInformation
{
	uint32_t FrameWidth = 0;
	ImVec2 HeaderMin;
	ImVec2 HeaderMax;
};

struct LinkInfo
{
	bool FlowLink = false;
	AINBFile::InputEntry* Entry = nullptr;
	int32_t MultiIndex = -1;

	AINBFile::Node* Parent = nullptr;
	AINBFile::LinkedNodeMapping LinkedNodeType = AINBFile::LinkedNodeMapping::StandardLink;
	AINBFile::LinkedNodeInfo* LinkedNodeInfo = nullptr;
};

bool UIAINBEditor::Focused = false;
bool UIAINBEditor::Open = true;
bool UIAINBEditor::RunAutoLayout = false;
bool UIAINBEditor::FocusOnZero = false;
ed::EditorContext* UIAINBEditor::Context = nullptr;
AINBFile UIAINBEditor::AINB;

std::unordered_map<uint32_t, NodeShapeInformation> NodeShapeInfo;

/* Those two maps will be cleared every frame and reconstructed */
std::unordered_map<AINBFile::InputEntry*, uint32_t> InputToId;
std::unordered_map<AINBFile::OutputEntry*, uint32_t> OutputToId;
std::unordered_map<uint32_t, LinkInfo> LinkToInput;
std::unordered_map<uint32_t, std::string> IdToFlowLinkCondition;
std::unordered_map<uint32_t, AINBFile::Node*> IdToParent;

AINBFile::Node* SelectedNode = nullptr;
LinkInfo SelectedLink; //Is Flow Link | InputEntry* / 

std::unordered_map<uint32_t, bool> CommandHeaderOpen;

//Limitation: max 42.949.600 nodes, per node max. 1000 pins

void UIAINBEditor::UpdateNodeShapes()
{
	NodeShapeInfo.clear();
	int ItemSpacingX = ImGui::GetStyle().ItemSpacing.x;
	for (int Index = 0; Index < AINB.Nodes.size(); Index++)
	{
		AINBFile::Node& Node = AINB.Nodes[Index];
		uint32_t FrameWidth = 8 * 2 + ImGui::CalcTextSize(Node.GetName().c_str()).x + 10;

		for (int i = 0; i < AINBFile::ValueTypeCount; i++)
		{
			for (AINBFile::InputEntry& Param : Node.InputParameters[i])
			{
				int Size = 8 * 2; // Frame Padding
				Size += ImGui::CalcTextSize(Param.Name.c_str()).x;
				Size += ItemSpacingX + 10;
				switch (Param.ValueType) {
				case (int)AINBFile::ValueType::Int:
				case (int)AINBFile::ValueType::String:
				case (int)AINBFile::ValueType::Float:
					Size += MinImmTextboxWidth;
					break;
				case (int)AINBFile::ValueType::Bool:
					Size += ImGui::GetFrameHeight();
					break;
				case (int)AINBFile::ValueType::Vec3f:
					Size += MinImmTextboxWidth * 3 + ItemSpacingX;
					break;
				default:
					break;
				}

				FrameWidth = std::fmax(FrameWidth, Size);
			}
			for (AINBFile::OutputEntry& Param : Node.OutputParameters[i])
			{
				int Size = 8 * 2; // Frame Padding
				Size += ImGui::CalcTextSize(Param.Name.c_str()).x;
				Size += 2 * ItemSpacingX + 10;
				FrameWidth = std::fmax(FrameWidth, Size);
			}
			for (AINBFile::ImmediateParameter& Param : Node.ImmediateParameters[i])
			{
				int Size = 8 * 2; // Frame Padding
				Size += ImGui::CalcTextSize(Param.Name.c_str()).x;
				Size += ItemSpacingX;
				switch (Param.ValueType) {
				case (int)AINBFile::ValueType::Int:
				case (int)AINBFile::ValueType::String:
				case (int)AINBFile::ValueType::Float:
					Size += MinImmTextboxWidth;
					break;
				case (int)AINBFile::ValueType::Bool:
					Size += ImGui::GetFrameHeight();
					break;
				case (int)AINBFile::ValueType::Vec3f:
					Size += MinImmTextboxWidth * 3 + ItemSpacingX;
					break;
				default:
					break;
				}

				FrameWidth = std::fmax(FrameWidth, Size);
			}
		}

		NodeShapeInformation Info;
		Info.FrameWidth = FrameWidth;

		NodeShapeInfo.insert({ Node.EditorId, Info });
	}
}

void UIAINBEditor::UpdateNodsIds()
{
	uint32_t Id = 1;
	for (int Index = 0; Index < AINB.Nodes.size(); Index++)
	{
		AINBFile::Node& Node = AINB.Nodes[Index];
		Node.EditorId = Id;
		Id += 1000;
	}
}

void UIAINBEditor::LoadAINBFile(std::string Path, bool AbsolutePath)
{
	AINB = AINBFile(AbsolutePath ? Path : Editor::GetRomFSFile(Path));

	if (AINB.Nodes.size() > 42'949'600)
	{
		Logger::Error("AINBEditor", "AINB has over 42.949.600 nodes");
		return;
	}
	SelectedNode = nullptr;
	SelectedLink = {false, nullptr, -1};
	UpdateNodsIds();
	UpdateNodeShapes();

	for (AINBFile::Node& Node : AINB.Nodes)
	{

		if (Node.Type != (int)AINBFile::NodeTypes::UserDefined && 
			Node.Type != (int)AINBFile::NodeTypes::Element_Sequential &&
			Node.Type != (int)AINBFile::NodeTypes::Element_S32Selector &&
			Node.Type != (int)AINBFile::NodeTypes::Element_F32Selector)
		{
			std::string DisplayName = Node.GetName();
			AINBNodeDefMgr::NodeDef* Def = AINBNodeDefMgr::GetNodeDefinition(DisplayName);
			if (Def != nullptr)
			{
				Node.EditorFlowLinkParams = Def->LinkedNodeParams;
			}
			continue;
		}

		if (Node.Type == (int)AINBFile::NodeTypes::Element_S32Selector)
		{
			for (AINBFile::LinkedNodeInfo& Info : Node.LinkedNodes[(int)AINBFile::LinkedNodeMapping::StandardLink])
			{
				Node.EditorFlowLinkParams.push_back(Info.Condition);
			}
			if (std::find(Node.EditorFlowLinkParams.begin(), Node.EditorFlowLinkParams.end(), "Default") == Node.EditorFlowLinkParams.end())
				Node.EditorFlowLinkParams.push_back("Default");
			continue;
		}
		if (Node.Type == (int)AINBFile::NodeTypes::Element_F32Selector)
		{
			for (AINBFile::LinkedNodeInfo& Info : Node.LinkedNodes[(int)AINBFile::LinkedNodeMapping::StandardLink])
			{
				if(Info.Condition != "Default") Node.EditorFlowLinkParams.push_back(std::to_string(Info.ConditionMin) + " <= Input <= " + std::to_string(Info.ConditionMax));
			}
			Node.EditorFlowLinkParams.push_back("Default");
			continue;
		}
		if (Node.Type == (int)AINBFile::NodeTypes::Element_Sequential)
		{
			for (int i = 0; i < Node.LinkedNodes[(int)AINBFile::LinkedNodeMapping::StandardLink].size(); i++)
			{
				Node.EditorFlowLinkParams.push_back("Seq " + std::to_string(i));
				Node.LinkedNodes[(int)AINBFile::LinkedNodeMapping::StandardLink][i].EditorName = "Seq " + std::to_string(i);
			}
		}
	}

	CommandHeaderOpen.clear();
	for (AINBFile::Command Cmd : AINB.Commands)
	{
		CommandHeaderOpen.insert({ Cmd.GUID.Part1, false });
	}

	RunAutoLayout = true;
	FocusOnZero = true;
}

void UIAINBEditor::Initialize()
{
	ed::Config Config;
	Config.SettingsFile = nullptr;
	Context = ed::CreateEditor(&Config);
}

void UIAINBEditor::Delete()
{
	ed::DestroyEditor(Context);
}

void UIAINBEditor::DrawPinIcon(uint32_t Id, bool IsOutput) {
	ed::PushStyleVar(ed::StyleVar_PivotAlignment, ImVec2(0.5f, 1.0f));
	ed::PushStyleVar(ed::StyleVar_PivotSize, ImVec2(0, 0));
	ed::BeginPin(Id, IsOutput ? ed::PinKind::Output : ed::PinKind::Input);
	//PinIcons::DrawIcon(iconSize);

	if (ImGui::IsRectVisible(ImVec2(10, 10))) {
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		ImVec2 cursorPos = ImGui::GetCursorScreenPos();

		int fontSize = ImGui::GetFontSize();

		ImVec2 trueDrawPos = cursorPos + ImVec2(0, (10 > fontSize) ? 0 : (fontSize - 10) / 2);

		ImVec2 p2 = ImVec2(trueDrawPos.x + 10, trueDrawPos.y + (10 / 2));
		ImVec2 p3 = ImVec2(trueDrawPos.x, trueDrawPos.y + 10);

		drawList->AddTriangleFilled(trueDrawPos, p2, p3, ImColor(255, 255, 255));
	}

	ImGui::Dummy(ImVec2(10, 10));

	ed::EndPin();
	ed::PopStyleVar(2);
}

ImColor UIAINBEditor::GetHeaderColor(AINBFile::Node& Node)
{
	switch (Node.Type) {
	case (int)AINBFile::NodeTypes::Element_S32Selector:
	case (int)AINBFile::NodeTypes::Element_F32Selector:
	case (int)AINBFile::NodeTypes::Element_StringSelector:
	case (int)AINBFile::NodeTypes::Element_RandomSelector:
	case (int)AINBFile::NodeTypes::Element_BoolSelector:
		return ImColor(60, 0, 60);
	case (int)AINBFile::NodeTypes::Element_Sequential:
		return ImColor(60, 0, 0);
	case (int)AINBFile::NodeTypes::Element_Simultaneous:
		return ImColor(0, 60, 0);
	case (int)AINBFile::NodeTypes::Element_ModuleIF_Input_S32:
	case (int)AINBFile::NodeTypes::Element_ModuleIF_Input_F32:
	case (int)AINBFile::NodeTypes::Element_ModuleIF_Input_Vec3f:
	case (int)AINBFile::NodeTypes::Element_ModuleIF_Input_String:
	case (int)AINBFile::NodeTypes::Element_ModuleIF_Input_Bool:
	case (int)AINBFile::NodeTypes::Element_ModuleIF_Input_Ptr:
		return ImColor(0, 60, 60);
	case (int)AINBFile::NodeTypes::Element_ModuleIF_Output_S32:
	case (int)AINBFile::NodeTypes::Element_ModuleIF_Output_F32:
	case (int)AINBFile::NodeTypes::Element_ModuleIF_Output_Vec3f:
	case (int)AINBFile::NodeTypes::Element_ModuleIF_Output_String:
	case (int)AINBFile::NodeTypes::Element_ModuleIF_Output_Bool:
	case (int)AINBFile::NodeTypes::Element_ModuleIF_Output_Ptr:
		return ImColor(0, 0, 60);
	case (int)AINBFile::NodeTypes::Element_ModuleIF_Child:
		return ImColor(0, 0, 0);
	default: return ImColor(60, 60, 0);
	}
}

void UIAINBEditor::DrawNode(AINBFile::Node& Node)
{
	uint32_t CurrentId = Node.EditorId;

	int FrameWidth = 0;

	ed::PushStyleVar(ed::StyleVar_NodePadding, ImVec4(8, 8, 8, 8));
	ed::BeginNode(CurrentId++);

	DrawPinIcon(CurrentId++, false);

	ImGui::SameLine();

	ImGui::Text(Node.GetName().c_str());

	NodeShapeInfo[Node.EditorId].HeaderMin = ImGui::GetItemRectMin() - ImVec2(18 + ImGui::GetStyle().ItemSpacing.x, 8);
	NodeShapeInfo[Node.EditorId].HeaderMax = ImVec2(NodeShapeInfo[Node.EditorId].HeaderMin.x + NodeShapeInfo[Node.EditorId].FrameWidth + 8, ImGui::GetItemRectMax().y + 8);

	ImGui::Dummy(ImVec2(0, 8));
	
	for (int i = 0; i < AINBFile::ValueTypeCount; i++)
	{
		for (AINBFile::InputEntry& Input : Node.InputParameters[i])
		{
			DrawPinIcon(CurrentId++, false);
			ImGui::SameLine();
			ImGui::TextUnformatted(Input.Name.c_str());

			InputToId.insert({ &Input, CurrentId - 1 });

			ImGui::SameLine();
			if (Input.NodeIndex == -1 && Input.Sources.empty())
			{
				switch (Input.ValueType)
				{
				case (int)AINBFile::ValueType::Int:
				{
					ImGui::PushItemWidth(MinImmTextboxWidth);
					ImGui::InputScalar(("##" + Input.Name + std::to_string(CurrentId)).c_str(), ImGuiDataType_U32, reinterpret_cast<uint32_t*>(&Input.Value));
					ImGui::PopItemWidth();
					break;
				}
				case (int)AINBFile::ValueType::Float:
				{
					ImGui::PushItemWidth(MinImmTextboxWidth);
					ImGui::InputScalar(("##" + Input.Name + std::to_string(CurrentId)).c_str(), ImGuiDataType_Float, reinterpret_cast<float*>(&Input.Value));
					ImGui::PopItemWidth();
					break;
				}
				case (int)AINBFile::ValueType::Bool:
					ImGui::Checkbox(("##" + Input.Name + std::to_string(CurrentId)).c_str(), reinterpret_cast<bool*>(&Input.Value));
					break;
				case (int)AINBFile::ValueType::String:
				{
					ImGui::PushItemWidth(MinImmTextboxWidth);
					ImGui::InputText(("##" + Input.Name + std::to_string(CurrentId)).c_str(), reinterpret_cast<std::string*>(&Input.Value));
					ImGui::PopItemWidth();
					break;
				}
				case (int)AINBFile::ValueType::Vec3f:
				{
					ImGui::InputScalarNWidth(("##" + Input.Name + std::to_string(CurrentId)).c_str(), ImGuiDataType_Float, reinterpret_cast<Vector3F*>(&Input.Value)->GetRawData(), 3, MinImmTextboxWidth * 3);
					break;
				}
				case (int)AINBFile::ValueType::UserDefined: //Don't know what the fuck this is
				{
					ImGui::NewLine();
					break;
				}
				default:
				{
					Logger::Warning("AINBEditor", "Input parameter has unknown value type " + std::to_string(Input.ValueType));
					break;
				}
				}
			}
			else
			{
				switch (Input.ValueType)
				{
				case (int)AINBFile::ValueType::Int:
				{
					ImGui::Dummy(ImVec2(MinImmTextboxWidth, 0));
					break;
				}
				case (int)AINBFile::ValueType::Float:
				{
					ImGui::Dummy(ImVec2(MinImmTextboxWidth, 0));
					break;
				}
				case (int)AINBFile::ValueType::Bool:
					ImGui::Dummy(ImVec2(ImGui::GetFrameHeight(), 0));
					break;
				case (int)AINBFile::ValueType::String:
				{
					ImGui::Dummy(ImVec2(MinImmTextboxWidth, 0));
					break;
				}
				case (int)AINBFile::ValueType::Vec3f:
				{
					ImGui::Dummy(ImVec2(MinImmTextboxWidth * 3 + 2 * ImGui::GetStyle().ItemSpacing.x, 0));
					break;
				}
				case (int)AINBFile::ValueType::UserDefined:
				{
					ImGui::NewLine();
					break;
				}
				}
			}
		}
	}

	for (int i = 0; i < AINBFile::ValueTypeCount; i++)
	{
		for (AINBFile::OutputEntry& Output : Node.OutputParameters[i])
		{
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + NodeShapeInfo[Node.EditorId].FrameWidth - (8 + ImGui::CalcTextSize(Output.Name.c_str()).x + 10 + ImGui::GetStyle().ItemSpacing.x));
			ImGui::TextUnformatted(Output.Name.c_str());

			ImGui::SameLine();

			DrawPinIcon(CurrentId++, true);

			OutputToId.insert({ &Output, CurrentId - 1 });
		}
	}

	/*
	int FlowIdx = 0;
	int FlowCount = 0;
	for (int NodeLinkType = 0; NodeLinkType < AINBFile::LinkedNodeTypeCount; NodeLinkType++)
	{
		for (AINBFile::LinkedNodeInfo& NodeLink : Node.LinkedNodes[NodeLinkType])
		{
			if (NodeLink.Type == AINBFile::LinkedNodeMapping::StandardLink)
			{
				FlowCount++;
			}
		}
	}

	std::map<std::string, uint32_t> LinkedNodeParams;

	for (int NodeLinkType = 0; NodeLinkType < AINBFile::LinkedNodeTypeCount; NodeLinkType++)
	{
		for (AINBFile::LinkedNodeInfo& NodeLink : Node.LinkedNodes[NodeLinkType])
		{
			if (NodeLink.Type == AINBFile::LinkedNodeMapping::StandardLink)
			{
				std::string LinkName = "";
				switch (Node.Type) {
				case (int)AINBFile::NodeTypes::UserDefined:
				case (int)AINBFile::NodeTypes::Element_BoolSelector:
				case (int)AINBFile::NodeTypes::Element_SplitTiming:
					LinkName = NodeLink.Condition;
					break;
				case (int)AINBFile::NodeTypes::Element_Simultaneous:
					LinkName = "Control";
					break;
				case (int)AINBFile::NodeTypes::Element_Sequential:
					LinkName = "Seq " + std::to_string(FlowIdx);
					break;
				case (int)AINBFile::NodeTypes::Element_S32Selector:
				case (int)AINBFile::NodeTypes::Element_F32Selector:
					if (FlowIdx == FlowCount - 1) {
						LinkName = "Default";
						break;
					}
					LinkName = "= " + NodeLink.Condition;
					break;
				case (int)AINBFile::NodeTypes::Element_Fork:
					LinkName = "Fork";
					break;
				default:
					LinkName = "<name unavailable>";
					break;
				}

				FlowIdx++;

				if (LinkedNodeParams.count(LinkName))
				{
					NodeLink.EditorId = LinkedNodeParams[LinkName];
					continue;
				}

				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + NodeShapeInfo[Node.EditorId].FrameWidth - (8 + ImGui::CalcTextSize(LinkName.c_str()).x + 10 + ImGui::GetStyle().ItemSpacing.x));
				ImGui::TextUnformatted(LinkName.c_str());
				ImGui::SameLine();
				DrawPinIcon(CurrentId++, true);
				NodeLink.EditorId = CurrentId - 1;
				IdToFlowLinkCondition.insert({ CurrentId - 1, LinkName });
				LinkedNodeParams.insert({ LinkName, CurrentId - 1 });
				IdToParent.insert({ CurrentId - 1, &Node });
			}
		}
	}
	*/

	for (auto Iter = Node.EditorFlowLinkParams.begin(); Iter != Node.EditorFlowLinkParams.end(); )
	{
		if (Node.Type == (int)AINBFile::NodeTypes::Element_S32Selector && *Iter != "Default")
		{
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + NodeShapeInfo[Node.EditorId].FrameWidth - (8 + ImGui::CalcTextSize(Iter->c_str()).x + 10 + ImGui::GetStyle().ItemSpacing.x) - ImGui::GetStyle().ItemSpacing.x - ImGui::CalcTextSize("Del").x - ImGui::GetStyle().FramePadding.x);
			if (ImGui::Button(("Del##LinkedNodeDel" + *Iter).c_str()))
			{
				std::size_t Index = std::distance(Node.EditorFlowLinkParams.begin(), Iter);
				Iter = Node.EditorFlowLinkParams.erase(Iter);

				if(Node.LinkedNodes[(int)AINBFile::LinkedNodeMapping::StandardLink].size() > Index) Node.LinkedNodes[(int)AINBFile::LinkedNodeMapping::StandardLink].erase(Node.LinkedNodes[(int)AINBFile::LinkedNodeMapping::StandardLink].begin() + Index);

				continue;
			}
			ImGui::SameLine();
		}
		else
		{
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + NodeShapeInfo[Node.EditorId].FrameWidth - (8 + ImGui::CalcTextSize(Iter->c_str()).x + 10 + ImGui::GetStyle().ItemSpacing.x));
		}
		ImGui::TextUnformatted(Iter->c_str());
		ImGui::SameLine();
		DrawPinIcon(CurrentId++, true);
		IdToFlowLinkCondition.insert({ CurrentId - 1, *Iter });
		IdToParent.insert({ CurrentId - 1, &Node });

		++Iter;
	}



	if (Node.Type == (int)AINBFile::NodeTypes::Element_Sequential)
	{
		//ImGui::SetCursorPosX(ImGui::GetCursorPosX() + NodeShapeInfo[Node.EditorId].FrameWidth - (8 + ImGui::CalcTextSize("Add sequential output").x + 10 + ImGui::GetStyle().ItemSpacing.x));
		if (ImGui::Button("Delete seq output"))
		{
			Node.EditorFlowLinkParams.pop_back();
			Node.LinkedNodes[(int)AINBFile::LinkedNodeMapping::StandardLink].pop_back();
		}
		ImGui::SameLine();
		if (ImGui::Button("Add seq output"))
			Node.EditorFlowLinkParams.push_back("Seq " + std::to_string(Util::StringToNumber<int>(Node.EditorFlowLinkParams[Node.EditorFlowLinkParams.size() - 1].substr(4, Node.EditorFlowLinkParams[Node.EditorFlowLinkParams.size() - 1].length() - 4)) + 1));
	}
	else if (Node.Type == (int)AINBFile::NodeTypes::Element_S32Selector)
	{
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + NodeShapeInfo[Node.EditorId].FrameWidth - (8 + ImGui::CalcTextSize("Add condition").x + 10 + ImGui::GetStyle().ItemSpacing.x));
		if (ImGui::Button("Add condition"))
		{
			PopupAINBElementSelector::Open("Condition: signed 32-bit number", "signed 32-bit number", "Add condition", &Node, [](AINBFile::Node* NodePtr, std::string Number) {
				NodePtr->EditorFlowLinkParams.insert(NodePtr->EditorFlowLinkParams.begin(), Number);
				});
		}
	}
	/*
	else if (Node.Type == (int)AINBFile::NodeTypes::Element_F32Selector)
	{
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + NodeShapeInfo[Node.EditorId].FrameWidth - (8 + ImGui::CalcTextSize("Add condition").x + 10 + ImGui::GetStyle().ItemSpacing.x));
		if (ImGui::Button("Add condition"))
		{
			PopupAINBElementSelector::Open("Condition: signed 32-bit float", "signed 32-bit float", "Add condition", &Node, [](AINBFile::Node* NodePtr, std::string Number) {
				NodePtr->EditorFlowLinkParams.insert(NodePtr->EditorFlowLinkParams.begin(), Number);
				});
		}
	}
	*/

	for (int i = 0; i < AINBFile::ValueTypeCount; i++)
	{
		for (AINBFile::ImmediateParameter& Immediate : Node.ImmediateParameters[i])
		{
			ImGui::TextUnformatted(Immediate.Name.c_str());
			ImGui::SameLine();

			bool ValueTypeMismatch = Immediate.ValueType != Immediate.Value.index(); //Fuck, that's a problem, hopefully the following code removes the problem, if not, crash :D
			if (ValueTypeMismatch)
			{
				Logger::Warning("AINBEditor", "Value type mismatch found, auto repair initiated");
			}

			switch (Immediate.ValueType)
			{
			case (int)AINBFile::ValueType::Int:
			{
				if (ValueTypeMismatch) Immediate.Value = (uint32_t)0;
				ImGui::PushItemWidth(MinImmTextboxWidth);
				ImGui::InputScalar(("##" + Immediate.Name + std::to_string(CurrentId)).c_str(), ImGuiDataType_U32, reinterpret_cast<uint32_t*>(&Immediate.Value));
				ImGui::PopItemWidth();
				break;
			}
			case (int)AINBFile::ValueType::Float:
			{
				if (ValueTypeMismatch) Immediate.Value = (float)0.0f;
				ImGui::PushItemWidth(MinImmTextboxWidth);
				ImGui::InputScalar(("##" + Immediate.Name + std::to_string(CurrentId)).c_str(), ImGuiDataType_Float, reinterpret_cast<float*>(&Immediate.Value));
				ImGui::PopItemWidth();
				break;
			}
			case (int)AINBFile::ValueType::Bool:
				if (ValueTypeMismatch) Immediate.Value = (bool)false;
				ImGui::Checkbox(("##" + Immediate.Name + std::to_string(CurrentId)).c_str(), reinterpret_cast<bool*>(&Immediate.Value));
				break;
			case (int)AINBFile::ValueType::String:
			{
				if (ValueTypeMismatch) Immediate.Value = "";
				ImGui::PushItemWidth(MinImmTextboxWidth);
				ImGui::InputText(("##" + Immediate.Name + std::to_string(CurrentId)).c_str(), reinterpret_cast<std::string*>(&Immediate.Value));
				ImGui::PopItemWidth();
				break;
			}
			case (int)AINBFile::ValueType::Vec3f:
			{
				if (ValueTypeMismatch) Immediate.Value = Vector3F(0, 0, 0);
				ImGui::InputScalarNWidth(("##" + Immediate.Name + std::to_string(CurrentId)).c_str(), ImGuiDataType_Float, reinterpret_cast<Vector3F*>(&Immediate.Value)->GetRawData(), 3, MinImmTextboxWidth * 3);
				break;
			}
			}
		}
	}

	ed::EndNode();
	ed::PopStyleVar();

	if (ImGui::IsItemVisible()) {
		int alpha = ImGui::GetStyle().Alpha;
		ImColor headerColor = GetHeaderColor(Node);
		headerColor.Value.w = alpha;

		ImDrawList* drawList = ed::GetNodeBackgroundDrawList(Node.EditorId);

		auto borderWidth = ed::GetStyle().NodeBorderWidth;

		NodeShapeInfo[Node.EditorId].HeaderMin.x += borderWidth;
		NodeShapeInfo[Node.EditorId].HeaderMin.y += borderWidth;
		NodeShapeInfo[Node.EditorId].HeaderMax.x -= borderWidth;
		NodeShapeInfo[Node.EditorId].HeaderMax.y -= borderWidth;

		drawList->AddRectFilled(NodeShapeInfo[Node.EditorId].HeaderMin, NodeShapeInfo[Node.EditorId].HeaderMax, headerColor, ed::GetStyle().NodeRounding,
			ImDrawFlags_RoundCornersTopLeft | ImDrawFlags_RoundCornersTopRight);

		ImVec2 headerSeparatorLeft = ImVec2(NodeShapeInfo[Node.EditorId].HeaderMin.x - borderWidth / 2, NodeShapeInfo[Node.EditorId].HeaderMax.y - 0.5f);
		ImVec2 headerSeparatorRight = ImVec2(NodeShapeInfo[Node.EditorId].HeaderMax.x, NodeShapeInfo[Node.EditorId].HeaderMax.y - 0.5f);

		drawList->AddLine(headerSeparatorLeft, headerSeparatorRight, ImColor(255, 255, 255, (int)(alpha * 255 / 2)), borderWidth);
	}
}

void UIAINBEditor::DrawLinks(AINBFile::Node& Node, int& CurrentId)
{
	for (int i = 0; i < AINBFile::ValueTypeCount; i++)
	{
		for (AINBFile::InputEntry& Input : Node.InputParameters[i])
		{
			if (Input.NodeIndex >= 0)
			{
				AINBFile::OutputEntry* Output = nullptr;

				for (int j = 0; j < AINBFile::ValueTypeCount; j++)
				{
					for (int k = 0; k < AINB.Nodes[Input.NodeIndex].OutputParameters[j].size(); k++)
					{
						if (k == Input.ParameterIndex)
						{
							Output = &AINB.Nodes[Input.NodeIndex].OutputParameters[j][k];
							goto LoopBreak0;
						}
					}
				}
				
			LoopBreak0:
				if (Output == nullptr)
				{
					Logger::Warning("AINBEditor", "Link had a nullptr as output");
					Input.NodeIndex = -1;
					Input.ParameterIndex = -1;
					return;
				}

				ed::Link(CurrentId++, OutputToId[Output], InputToId[&Input], ImColor(140, 140, 40));
				LinkToInput.insert({ CurrentId - 1, LinkInfo{false, &Input, -1} });
			}
			if (!Input.Sources.empty())
			{
				for (int MultiIndex = 0; MultiIndex < Input.Sources.size(); MultiIndex++)
				{
					AINBFile::OutputEntry* Output = nullptr;
					AINBFile::MultiEntry& Entry = Input.Sources[MultiIndex];

					for (int j = 0; j < AINBFile::ValueTypeCount; j++)
					{
						for (int k = 0; k < AINB.Nodes[Entry.NodeIndex].OutputParameters[j].size(); k++)
						{
							if (k == Entry.ParameterIndex)
							{
								Output = &AINB.Nodes[Entry.NodeIndex].OutputParameters[j][k];
								goto LoopBreak2;
							}
						}
					}

				LoopBreak2:
					if (Output == nullptr)
					{
						Logger::Warning("AINBEditor", "Multi Link had a nullptr as output");
						Entry.NodeIndex = -1;
						Entry.ParameterIndex = -1;
						return;
					}

					ed::Link(CurrentId++, OutputToId[Output], InputToId[&Input], ImColor(140, 140, 40));
					LinkToInput.insert({ CurrentId - 1, LinkInfo{false, &Input, MultiIndex} });
				}
			}
		}
	}

	int FlowIdx = 0;
	int FlowCount = Node.EditorFlowLinkParams.size();

	for (int NodeLinkType = 0; NodeLinkType < AINBFile::LinkedNodeTypeCount; NodeLinkType++)
	{
		for (AINBFile::LinkedNodeInfo& NodeLink : Node.LinkedNodes[NodeLinkType])
		{
			if (NodeLink.Type == AINBFile::LinkedNodeMapping::StandardLink)
			{
				std::string LinkName = "";
				switch (Node.Type) {
				case (int)AINBFile::NodeTypes::UserDefined:
				case (int)AINBFile::NodeTypes::Element_BoolSelector:
				case (int)AINBFile::NodeTypes::Element_SplitTiming:
				case (int)AINBFile::NodeTypes::Element_S32Selector:
					LinkName = NodeLink.Condition;
					break;
				case (int)AINBFile::NodeTypes::Element_F32Selector:
					LinkName = std::to_string(NodeLink.ConditionMin) + " <= Input <= " + std::to_string(NodeLink.ConditionMax);
					break;
				case (int)AINBFile::NodeTypes::Element_Simultaneous:
					LinkName = "Control";
					break;
				case (int)AINBFile::NodeTypes::Element_Sequential:
					LinkName = NodeLink.EditorName;
					break;
				case (int)AINBFile::NodeTypes::Element_Fork:
					LinkName = "Fork";
					break;
				default:
					LinkName = "<name unavailable>";
					break;
				}

				FlowIdx++;
				uint32_t FlowLinkId = 0;
				for (auto& [Id, Key] : IdToFlowLinkCondition)
				{
					if (Key == LinkName && IdToParent[Id] == &Node)
					{
						FlowLinkId = Id;
						break;
					}
				}

				ed::Link(CurrentId++, AINB.Nodes[NodeLink.NodeIndex].EditorId + 1, FlowLinkId, ImColor(255, 255, 255));
				NodeLink.EditorId = CurrentId - 1;
				LinkToInput.insert({ CurrentId - 1, LinkInfo{ true, nullptr, -1, IdToParent[FlowLinkId], AINBFile::LinkedNodeMapping::StandardLink, &NodeLink } });
			}
		}
	}
}

void UIAINBEditor::ManageLinkCreationDeletion()
{
	if (ed::BeginCreate())
	{
		ed::PinId InputPinId, OutputPinId;
		if (ed::QueryNewLink(&InputPinId, &OutputPinId))
		{
			if (ed::AcceptNewItem())
			{
				int16_t NodeIndex = -1;
				int16_t ParameterIndex = 0;

				bool FlowLink = false;

				for (int16_t i = 0; i < AINB.Nodes.size(); i++)
				{
					for (int j = 0; j < AINBFile::ValueTypeCount; j++)
					{
						for (AINBFile::OutputEntry& Param : AINB.Nodes[i].OutputParameters[j])
						{
							if (OutputToId[&Param] == InputPinId.Get())
							{
								NodeIndex = i;
							}
						}
					}
				}

				if (NodeIndex == -1)
				{
					for (int16_t i = 0; i < AINB.Nodes.size(); i++)
					{
						if (AINB.Nodes[i].EditorId + 1 == OutputPinId.Get())
						{
							NodeIndex = i;
							FlowLink = true;
							std::cout << "FlowLink\n";
						}
					}
				}

				if (NodeIndex != -1)
				{
					if (!FlowLink)
					{
						for (int i = 0; i < AINBFile::ValueTypeCount; i++)
						{
							for (AINBFile::OutputEntry& OutputEntry : AINB.Nodes[NodeIndex].OutputParameters[i])
							{
								if (OutputToId.count(&OutputEntry))
								{
									if (OutputToId[&OutputEntry] == InputPinId.Get())
									{
										goto LoopBreak1;
									}
								}
								ParameterIndex++;
							}
						}
					LoopBreak1:
						for (auto& [Input, Id] : InputToId)
						{
							if (Id == OutputPinId.Get())
							{
								if (Input->NodeIndex < 0)
								{
									if (Input->Sources.empty())
									{
										Input->NodeIndex = NodeIndex;
										Input->ParameterIndex = ParameterIndex;
									}
									else
									{
										AINBFile::MultiEntry NewEntry;
										NewEntry.NodeIndex = NodeIndex;
										NewEntry.ParameterIndex = ParameterIndex;
										NewEntry.Flags = Input->Flags;
										NewEntry.GlobalParametersIndex = Input->GlobalParametersIndex;
										Input->MultiCount++;
										Input->Sources.push_back(NewEntry);
									}
								}
								else
								{
									AINBFile::MultiEntry BaseEntry;
									BaseEntry.NodeIndex = Input->NodeIndex;
									BaseEntry.ParameterIndex = Input->ParameterIndex;
									BaseEntry.Flags = Input->Flags;
									BaseEntry.GlobalParametersIndex = Input->GlobalParametersIndex;

									AINBFile::MultiEntry NewEntry;
									NewEntry.NodeIndex = NodeIndex;
									NewEntry.ParameterIndex = ParameterIndex;
									NewEntry.Flags = Input->Flags;
									NewEntry.GlobalParametersIndex = Input->GlobalParametersIndex;

									Input->NodeIndex = -1;
									Input->ParameterIndex = -1;
									Input->Sources.push_back(BaseEntry);
									Input->Sources.push_back(NewEntry);
									Input->MultiCount = 2;

									std::cout << "BaseMulti: " << BaseEntry.NodeIndex << ", " << BaseEntry.ParameterIndex << std::endl;
									std::cout << "NewMulti: " << NewEntry.NodeIndex << ", " << NewEntry.ParameterIndex << std::endl;
								}
								break;
							}
						}
					}
					else
					{
						AINBFile::LinkedNodeInfo Info;
						Info.Type = AINBFile::LinkedNodeMapping::StandardLink;
						Info.NodeIndex = NodeIndex;
						if (IdToParent[InputPinId.Get()]->Type == (uint16_t)AINBFile::NodeTypes::Element_S32Selector ||
							IdToParent[InputPinId.Get()]->Type == (uint16_t)AINBFile::NodeTypes::Element_StringSelector ||
							IdToParent[InputPinId.Get()]->Type == (uint16_t)AINBFile::NodeTypes::Element_RandomSelector ||
							IdToParent[InputPinId.Get()]->Type == (uint16_t)AINBFile::NodeTypes::Element_BoolSelector) Info.Condition = IdToFlowLinkCondition[InputPinId.Get()];
						else
						{
							Info.ConnectionName = "";
							Info.EditorName = IdToFlowLinkCondition[InputPinId.Get()];
						}
						IdToParent[InputPinId.Get()]->LinkedNodes[(int)AINBFile::LinkedNodeMapping::StandardLink].push_back(Info);
						std::cout << "Added link\n";
					}
				}
			}
		}
	}
	ed::EndCreate();
}

void UIAINBEditor::DrawAinbEditorWindow()
{
	if (!Open) return;

	UIAINBEditor::Focused = ImGui::Begin("AINB Editor", &Open);

    /* Button Placeholder */

	bool WantAutoLayout = ImGui::Button("Auto layout");
	ImGui::SameLine();
	if (ImGui::Button("Add node"))
	{
		PopupAddAINBNode::Open([](std::string Name)
			{
				AINBFile::Node NewNode = AINBNodeDefMgr::NodeDefToNode(AINBNodeDefMgr::GetNodeDefinition(Name));
				NewNode.NodeIndex = AINB.Nodes.size();
				NewNode.EditorId = AINB.Nodes.empty() ? 1 : AINB.Nodes[AINB.Nodes.size() - 1].EditorId + 1000;

				if (NewNode.Type == (int)AINBFile::NodeTypes::Element_Sequential)
					NewNode.EditorFlowLinkParams.resize(2);
				else if (NewNode.Type == (int)AINBFile::NodeTypes::Element_S32Selector)
				{
					NewNode.EditorFlowLinkParams.clear();
					NewNode.EditorFlowLinkParams.push_back("Default");
				}

				AINB.Nodes.push_back(NewNode);
				UpdateNodeShapes();
				std::cout << NewNode.Name << std::endl;
				for (std::string FileName : AINBNodeDefMgr::GetNodeDefinition(Name)->FileNames)
				{
					std::cout << FileName << std::endl;
				}
			});
	}

	if (ImGui::Button("Open AINB (Vanilla)"))
	{
		const char* Path = tinyfd_openFileDialog("Open AINB (Vanilla)", (Editor::RomFSDir + "\\Logic").c_str(), 0, nullptr, nullptr, 0);
		if (Path != nullptr) {
			LoadAINBFile(Path, true);
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("Open AINB (Custom)"))
	{
		Util::CreateDir(Editor::GetWorkingDirFile("Save/Logic"));
		const char* Path = tinyfd_openFileDialog("Open AINB (Custom)", (std::filesystem::current_path().string() + "\\WorkingDir\\Save\\Logic").c_str(), 0, nullptr, nullptr, 0);
		if (Path != nullptr) {
			LoadAINBFile(Path, true);
		}
	}

    ImGui::Separator();

    ed::SetCurrentEditor(Context);
    ed::Begin("AINBEditor", ImVec2(0.0, 0.0f));
    
	InputToId.clear();
	OutputToId.clear();
	LinkToInput.clear();
	IdToFlowLinkCondition.clear();
	IdToParent.clear();

	for (AINBFile::Node& Node : AINB.Nodes)
	{
		DrawNode(Node);
	}

	int LinkId = AINB.Nodes.size() * 1000;

	for (AINBFile::Node& Node : AINB.Nodes)
	{
		DrawLinks(Node, LinkId);
	}

	ed::Suspend();
	ed::NodeId SelectedNodeID;
	if (ed::GetSelectedNodes(&SelectedNodeID, 1) > 0) {
		for (AINBFile::Node& Node : AINB.Nodes)
		{
			if (Node.EditorId == SelectedNodeID.Get())
			{
				SelectedNode = &Node;
				break;
			}
		}
	}
	else
	{
		SelectedNode = nullptr;
	}
	ed::LinkId SelectedLinkID;
	if (ed::GetSelectedLinks(&SelectedLinkID, 1) > 0)
	{
		SelectedLink = LinkToInput[SelectedLinkID.Get()];
	}
	else
	{
		SelectedLink = {false, nullptr, -1};
	}
	ed::Resume();

	ManageLinkCreationDeletion();

	if (WantAutoLayout || RunAutoLayout)
	{
		RunAutoLayout = false;
		std::unordered_map<AINBFile::Node*, std::pair<int, int>> NodeToPos;
		std::unordered_map<int, std::unordered_map<int, AINBFile::Node*>> PosToNode;

		std::function<void(AINBFile::Node*, uint16_t, std::pair<int, int>)> PlaceNode = [&](AINBFile::Node* Node, uint16_t NodeIndex, std::pair<int, int> Pos) {
			if (NodeToPos.contains(Node)) {
				return;
			}
			while (PosToNode[Pos.first].contains(Pos.second))
			{
				Pos.second++;
			}
			NodeToPos[Node] = Pos;
			PosToNode[Pos.first][Pos.second] = Node;

			uint16_t NewNodeIndex = 0;
			for (AINBFile::Node& LinkedNode : AINB.Nodes)
			{
				for (int j = 0; j < AINBFile::ValueTypeCount; j++)
				{
					for (AINBFile::InputEntry Param : LinkedNode.InputParameters[j]) {
						if (Param.NodeIndex == NodeIndex)
						{
							PlaceNode(&LinkedNode, NewNodeIndex, { Pos.first + 1, Pos.second });
						}
						for (AINBFile::MultiEntry& Entry : Param.Sources)
						{
							if (Entry.NodeIndex == NodeIndex)
							{
								PlaceNode(&LinkedNode, NewNodeIndex, { Pos.first + 1, Pos.second });
							}
						}
					}
				}
				NewNodeIndex++;
			}

			for (int NodeLinkType = 0; NodeLinkType < AINBFile::LinkedNodeTypeCount; NodeLinkType++)
			{
				for (AINBFile::LinkedNodeInfo& NodeLink : Node->LinkedNodes[NodeLinkType])
				{
					if (NodeLink.Type == AINBFile::LinkedNodeMapping::StandardLink)
					{
						PlaceNode(&AINB.Nodes[NodeLink.NodeIndex], NodeLink.NodeIndex, {Pos.first + 1, Pos.second});
					}
				}
			}

			for (int j = 0; j < AINBFile::ValueTypeCount; j++)
			{
				for (AINBFile::InputEntry Param : Node->InputParameters[j]) {
					if (Param.NodeIndex >= 0)
					{
						PlaceNode(&AINB.Nodes[Param.NodeIndex], Param.NodeIndex, { Pos.first - 1, Pos.second });
					}
					for (AINBFile::MultiEntry& Entry : Param.Sources)
					{
						PlaceNode(&AINB.Nodes[Entry.NodeIndex], Entry.NodeIndex, { Pos.first - 1, Pos.second });
					}
				}
			}

			NewNodeIndex = 0;
			for (AINBFile::Node& LinkedNode : AINB.Nodes)
			{
				for (int NodeLinkType = 0; NodeLinkType < AINBFile::LinkedNodeTypeCount; NodeLinkType++)
				{
					for (AINBFile::LinkedNodeInfo& NodeLink : LinkedNode.LinkedNodes[NodeLinkType])
					{
						if (NodeLink.Type == AINBFile::LinkedNodeMapping::StandardLink && NodeLink.NodeIndex == NodeIndex)
						{
							PlaceNode(&LinkedNode, NewNodeIndex, { Pos.first - 1, Pos.second });
						}
					}
				}
				NewNodeIndex++;
			}
		};

		uint16_t NodeIdx = 0;
		for (AINBFile::Node& Node : AINB.Nodes)
		{
			if (!NodeToPos.count(&Node))
			{
				PlaceNode(&Node, NodeIdx, { 0, 0 });
			}
			NodeIdx++;
		}

		for (auto& [Node, Pos] : NodeToPos)
		{
			ed::SetNodePosition(Node->EditorId, ImVec2(Pos.first * 600, Pos.second * 400));
		}
	}

	if (FocusOnZero)
	{
		ed::NavigateToContent();
		FocusOnZero = false;
	}

    ed::End();
    ed::SetCurrentEditor(nullptr);

	ImGui::End();
}

void UIAINBEditor::DrawPropertiesWindowContent()
{
	if (ImGui::Button("Add command"))
	{
		AINBFile::Command Command;
		Command.LeftNodeIndex = 0;
		Command.RightNodeIndex = -1;
		Command.Name = "DummyName";
		AINB.Commands.push_back(Command);
	}
	for (AINBFile::Command& Command : AINB.Commands)
	{
		ImGui::GetStateStorage()->SetInt(ImGui::GetID(("Command: " + Command.Name).c_str()), (int)CommandHeaderOpen[Command.GUID.Part1]);
		if (ImGui::CollapsingHeader(("Command: " + Command.Name).c_str()))
		{
			ImGui::Indent();
			ImGui::InputText("Name", &Command.Name);
			ImGui::InputScalar(("Left Node Index##" + Command.Name).c_str(), ImGuiDataType_::ImGuiDataType_S16, &Command.LeftNodeIndex);
			ImGui::InputScalar(("Right Node Index##" + Command.Name).c_str(), ImGuiDataType_::ImGuiDataType_S16, &Command.RightNodeIndex);
			ImGui::Unindent();
			CommandHeaderOpen[Command.GUID.Part1] = true;
		}
		else
		{
			CommandHeaderOpen[Command.GUID.Part1] = false;
		}
	}

	if (SelectedNode != nullptr)
	{
		ImGui::Text(SelectedNode->Name.c_str());
		std::string NodeIndex = "NodeIndex: ";
		for (int LoopNodeIndex = 0; LoopNodeIndex < AINB.Nodes.size(); LoopNodeIndex++)
		{
			if (&AINB.Nodes[LoopNodeIndex] == SelectedNode)
			{
				NodeIndex += std::to_string(LoopNodeIndex);
				break;
			}
		}
		ImGui::Text(NodeIndex.c_str());
		if (ImGui::Button("Duplicate"))
		{
			AINBFile::Node NewNode = *SelectedNode;
			uint32_t NewId = 0;
			for (AINBFile::Node& Node : AINB.Nodes)
			{
				NewId = std::max(NewId, Node.EditorId);
			}
			NewNode.EditorId = NewId + 1000;
			AINB.Nodes.push_back(NewNode);
			UpdateNodeShapes();
		}
		if (ImGui::Button("Delete"))
		{
			uint16_t NodeIndex = 0;
			for (AINBFile::Node& Node : AINB.Nodes)
			{
				if (&Node != SelectedNode)
				{
					NodeIndex++;
					continue;
				}
				break;
			}
			
			for (AINBFile::Node& Node : AINB.Nodes)
			{
				for (int i = 0; i < AINBFile::ValueTypeCount; i++)
				{
					for (AINBFile::InputEntry& Param : Node.InputParameters[i])
					{
						if (Param.NodeIndex == NodeIndex)
						{
							Param.NodeIndex = -1;
							Param.ParameterIndex = -1;
						}

						for (int j = 0; j < Param.Sources.size(); j++)
						{
							auto Iter = Param.Sources.begin();
							std::advance(Iter, j);
							if (Iter->NodeIndex == NodeIndex)
							{
								Param.MultiCount--;
								Param.Sources.erase(Iter);
							}
						}
					}
				}
			}
			for (AINBFile::Node& Node : AINB.Nodes)
			{
				for (int i = 0; i < AINBFile::ValueTypeCount; i++)
				{
					for (AINBFile::InputEntry& Param : Node.InputParameters[i])
					{
						if (Param.NodeIndex > NodeIndex)
						{
							Param.NodeIndex--;
						}

						for (int j = 0; j < Param.Sources.size(); j++)
						{
							auto Iter = Param.Sources.begin();
							std::advance(Iter, j);
							if (Iter->NodeIndex > NodeIndex)
							{
								Iter->NodeIndex--;
							}
						}
					}
				}
				for (int NodeLinkType = 0; NodeLinkType < AINBFile::LinkedNodeTypeCount; NodeLinkType++)
				{
					for (int j = 0; j < Node.LinkedNodes[NodeLinkType].size(); j++)
					{
						auto Iter = Node.LinkedNodes[NodeLinkType].begin();
						std::advance(Iter, j);
						if(Iter->NodeIndex == NodeIndex)
						{
							Node.LinkedNodes[NodeLinkType].erase(Iter);
						} 
						else if (Iter->NodeIndex > NodeIndex)
						{
							Iter->NodeIndex--;
						}
					}
				}
			}
			NodeShapeInfo.erase(SelectedNode->EditorId);
			AINB.Nodes.erase(AINB.Nodes.begin() + NodeIndex);
			SelectedNode = nullptr;
		}
		return;
	}
	if (SelectedLink.Entry != nullptr && !SelectedLink.FlowLink)
	{
		if (ImGui::Button("Delete"))
		{
			if (SelectedLink.MultiIndex == -1)
			{
				SelectedLink.Entry->NodeIndex = -1;
				SelectedLink.Entry->ParameterIndex = -1;
			}
			else
			{
				SelectedLink.Entry->MultiCount--;
				auto Iter = SelectedLink.Entry->Sources.begin();
				std::advance(Iter, SelectedLink.MultiIndex);
				SelectedLink.Entry->Sources.erase(Iter);

				if (SelectedLink.Entry->Sources.size() == 1)
				{
					SelectedLink.Entry->NodeIndex = SelectedLink.Entry->Sources[0].NodeIndex;
					SelectedLink.Entry->ParameterIndex = SelectedLink.Entry->Sources[0].ParameterIndex;
					SelectedLink.Entry->MultiCount = 0;
					SelectedLink.Entry->Sources.clear();
				}
			}
			SelectedLink = { false, nullptr, -1 };
		}
	}
	else if (SelectedLink.FlowLink && SelectedLink.LinkedNodeInfo != nullptr && SelectedLink.Parent != nullptr)
	{
		if (ImGui::Button("Delete"))
		{
			std::cout << (int)SelectedLink.LinkedNodeType << ", " << SelectedLink.Parent->LinkedNodes[(int)SelectedLink.LinkedNodeType].size() << ", " << SelectedLink.Parent->GetName() << std::endl;
			SelectedLink.Parent->LinkedNodes[(int)SelectedLink.LinkedNodeType].erase(
				std::remove_if(SelectedLink.Parent->LinkedNodes[(int)SelectedLink.LinkedNodeType].begin(), SelectedLink.Parent->LinkedNodes[(int)SelectedLink.LinkedNodeType].end(), [&](AINBFile::LinkedNodeInfo const& LinkedNode) {
					std::cout << LinkedNode.EditorId << ", " << SelectedLink.LinkedNodeInfo->EditorId << std::endl;
					return LinkedNode.EditorId == SelectedLink.LinkedNodeInfo->EditorId;
					}),
				SelectedLink.Parent->LinkedNodes[(int)SelectedLink.LinkedNodeType].end());
			SelectedLink = { false, nullptr, -1 };
		}
	}
}

void UIAINBEditor::Save()
{
	if (AINB.Loaded)
	{
		Util::CreateDir(Editor::GetWorkingDirFile("Save/" + AINB.Header.FileCategory));
		AINB.Write(Editor::GetWorkingDirFile("Save/" + AINB.Header.FileCategory + "/" + AINB.Header.FileName + ".ainb"));
	}
}