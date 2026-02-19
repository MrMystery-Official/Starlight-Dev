#include "UIAINBEditor.h"

#include <util/Logger.h>
#include <util/FileUtil.h>
#include <util/Math.h>
#include <manager/UIMgr.h>
#include <manager/ProjectMgr.h>
#include <file/tool/AdditionalAINBNodes.h>
#include <unordered_map>
#include <util/portable-file-dialogs.h>
#include <algorithm>
#include <filesystem>
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_stdlib.h"
#include <rendering/ImGuiExt.h>
#include <rendering/ainb/nodes/UIAINBEditorNodeDefault.h>
#include <rendering/ainb/nodes/UIAINBEditorNodeSplitTiming.h>
#include <rendering/ainb/nodes/UIAINBEditorNodeSimultaneous.h>
#include <rendering/ainb/nodes/UIAINBEditorNodeSequential.h>
#include <rendering/ainb/nodes/UIAINBEditorNodeBoolSelector.h>
#include <rendering/ainb/nodes/UIAINBEditorNodeS32Selector.h>
#include <rendering/ainb/nodes/UIAINBEditorNodeF32Selector.h>

namespace application::rendering::ainb
{
	const char* UIAINBEditor::gCategoryDropdownItems[3] = { "Logic", "AI", "Sequence" };
	const char* UIAINBEditor::gValueTypeDropdownItems[6] = { "String", "Int", "Float", "Bool", "Vec3f", "UserDefined" };
	const char* UIAINBEditor::gParamValueTypeDropdownItems[6] = { "Int", "Bool", "Float", "String", "Vec3f", "UserDefined" };
	application::gl::Texture* UIAINBEditor::gHeaderTexture = nullptr;

	application::rendering::popup::PopUpBuilder UIAINBEditor::gCreateNewNodePopUp;

	bool UIAINBEditor::NodeEditorSaveSettings(const char* data, size_t size, ed::SaveReasonFlags reason, void* userPointer)
	{
		return false;
	}

	void UIAINBEditor::Initialize()
	{
		application::rendering::ainb::nodes::UIAINBEditorNodeS32Selector::Initialize();

		gCreateNewNodePopUp
			.Title("Create new node")
			.Width(500.0f)
			.Height(500.0f)
			.Flag(ImGuiWindowFlags_NoResize)
			.NeedsConfirmation(false)
			.AddDataStorageInstanced<application::manager::AINBNodeMgr::NodeDef>([](void* Ptr) { *reinterpret_cast<application::manager::AINBNodeMgr::NodeDef*>(Ptr) = {}; })
			.ContentDrawingFunction([](popup::PopUpBuilder& Builder)
				{
					application::manager::AINBNodeMgr::NodeDef* Def = reinterpret_cast<application::manager::AINBNodeMgr::NodeDef*>(Builder.GetDataStorage(0).mPtr);

					if (ImGui::BeginTable("NewNodeTable", 2, ImGuiTableFlags_BordersInnerV))
					{
						//Def->mNodeType = application::file::game::ainb::AINBFile::NodeTypes::UserDefined;
						//Def->mNameHash = 0;
						//Def->mProjectName = application::manager::ProjectMgr::gProject;

						/*
						        struct NodeDef
        {
            struct ParameterDef
            {
                std::string mName;
                std::string mClass;
                application::file::game::ainb::AINBFile::ValueType mValueType;
                std::vector<application::file::game::ainb::AINBFile::FlagsStruct> mFlags;
            };

            std::vector<std::string> mFlowOutputParameters;
            std::vector<ParameterDef> mInputParameters;
            std::vector<ParameterDef> mOutputParameters;
            std::vector<ParameterDef> mImmediateParameters;
        };
						*/

						ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("Destination").x);

						ImGui::TableNextRow();

						ImGui::TableSetColumnIndex(0);
						ImGui::Text("Name");
						ImGui::TableNextColumn();
						ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
						ImGui::InputText("##NodeName", &Def->mName);
						ImGui::PopItemWidth();
						ImGui::TableNextRow();

						ImGui::TableSetColumnIndex(0);
						ImGui::Text("Categories");
						ImGui::TableNextColumn();
						ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);

						bool CatAI = std::find(Def->mCategories.begin(), Def->mCategories.end(), application::manager::AINBNodeMgr::NodeDef::Category::AI) != Def->mCategories.end();
						if (ImGui::Checkbox("AI", &CatAI))
						{
							auto It = std::find(Def->mCategories.begin(), Def->mCategories.end(), application::manager::AINBNodeMgr::NodeDef::Category::AI);
							if (It != Def->mCategories.end())
								Def->mCategories.erase(It);
							else
								Def->mCategories.push_back(application::manager::AINBNodeMgr::NodeDef::Category::AI);
						}
						ImGui::SameLine();
						bool CatSeq = std::find(Def->mCategories.begin(), Def->mCategories.end(), application::manager::AINBNodeMgr::NodeDef::Category::SEQUENCE) != Def->mCategories.end();
						if (ImGui::Checkbox("Sequence", &CatSeq))
						{
							auto It = std::find(Def->mCategories.begin(), Def->mCategories.end(), application::manager::AINBNodeMgr::NodeDef::Category::SEQUENCE);
							if (It != Def->mCategories.end())
								Def->mCategories.erase(It);
							else
								Def->mCategories.push_back(application::manager::AINBNodeMgr::NodeDef::Category::SEQUENCE);
						}
						ImGui::SameLine();
						bool CatLogic = std::find(Def->mCategories.begin(), Def->mCategories.end(), application::manager::AINBNodeMgr::NodeDef::Category::LOGIC) != Def->mCategories.end();
						if (ImGui::Checkbox("Logic", &CatLogic))
						{
							auto It = std::find(Def->mCategories.begin(), Def->mCategories.end(), application::manager::AINBNodeMgr::NodeDef::Category::LOGIC);
							if (It != Def->mCategories.end())
								Def->mCategories.erase(It);
							else
								Def->mCategories.push_back(application::manager::AINBNodeMgr::NodeDef::Category::LOGIC);
						}

						ImGui::PopItemWidth();
						ImGui::EndTable();
					}

					if (ImGuiExt::CollapsingHeaderWithAddButton("Execution Flow Output Parameters", ImGuiTreeNodeFlags_None, [&Def](){ Def->mFlowOutputParameters.push_back("Execute"); }))
					{
						ImGui::Indent();

						if (ImGui::BeginTable("NewNodeExecutionFlowTable", 2, ImGuiTableFlags_BordersInnerV))
						{
							for(auto Iter = Def->mFlowOutputParameters.begin(); Iter != Def->mFlowOutputParameters.end(); )
							{
								size_t i = std::distance(Def->mFlowOutputParameters.begin(), Iter);

								ImGui::TableNextRow();
								ImGui::TableSetColumnIndex(0);
								ImGui::InputText(("##NewExecutionParam_" + std::to_string(i)).c_str(), &Def->mFlowOutputParameters[i]);
								ImGui::TableNextColumn();

								if (ImGuiExt::ButtonDelete(("Delete##NewExecutionParam_" + std::to_string(i)).c_str(), ImVec2(0, 0)))
								{
									Iter = Def->mFlowOutputParameters.erase(Iter);
									continue;
								}

								Iter++;
							}

							ImGui::EndTable();
						}

						ImGui::Unindent();
					}

					if (ImGuiExt::CollapsingHeaderWithAddButton("Input Parameters", ImGuiTreeNodeFlags_None, [&Def]()
						{
							Def->mInputParameters.push_back(
								application::manager::AINBNodeMgr::NodeDef::ParameterDef
								{
									.mName = "NewParam",
									.mClass = "",
									.mValueType = application::file::game::ainb::AINBFile::ValueType::Int,
									.mFlags = {}
								}
							);
						}
					))
					{
						ImGui::Indent();

						if (ImGui::BeginTable("NewNodeInputParamsTable", 3, ImGuiTableFlags_BordersInnerV))
						{
							for (auto Iter = Def->mInputParameters.begin(); Iter != Def->mInputParameters.end(); )
							{
								size_t i = std::distance(Def->mInputParameters.begin(), Iter);

								ImGui::TableNextRow();
								ImGui::TableSetColumnIndex(0);
								ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
								ImGui::Combo(("##ParamDataType_" + std::to_string(i)).c_str(), reinterpret_cast<int*>(&Def->mInputParameters[i].mValueType), gParamValueTypeDropdownItems, IM_ARRAYSIZE(gParamValueTypeDropdownItems) - 1); //-1 to remove UserDefined as an option
								ImGui::PopItemWidth();

								ImGui::TableNextColumn();
								ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
								ImGui::InputText(("##ParamName_" + std::to_string(i)).c_str(), &Def->mInputParameters[i].mName);
								ImGui::PopItemWidth();

								ImGui::TableNextColumn();
								ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
								if (ImGuiExt::ButtonDelete(("Delete##ParamDelete_" + std::to_string(i)).c_str(), ImVec2(0, 0)))
								{
									Iter = Def->mInputParameters.erase(Iter);
									continue;
								}
								ImGui::PopItemWidth();

								Iter++;
							}

							ImGui::EndTable();
						}

						ImGui::Unindent();
					}

					if (ImGuiExt::CollapsingHeaderWithAddButton("Output Parameters", ImGuiTreeNodeFlags_None, [&Def]()
						{
							Def->mOutputParameters.push_back(
								application::manager::AINBNodeMgr::NodeDef::ParameterDef
								{
									.mName = "NewParam",
									.mClass = "",
									.mValueType = application::file::game::ainb::AINBFile::ValueType::Int,
									.mFlags = {}
								}
							);
						}
					))
					{
						ImGui::Indent();

						if (ImGui::BeginTable("NewNodeOutputParamsTable", 3, ImGuiTableFlags_BordersInnerV))
						{
							for (auto Iter = Def->mOutputParameters.begin(); Iter != Def->mOutputParameters.end(); )
							{
								size_t i = std::distance(Def->mOutputParameters.begin(), Iter);

								ImGui::TableNextRow();
								ImGui::TableSetColumnIndex(0);
								ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
								ImGui::Combo(("##OutParamDataType_" + std::to_string(i)).c_str(), reinterpret_cast<int*>(&Def->mOutputParameters[i].mValueType), gParamValueTypeDropdownItems, IM_ARRAYSIZE(gParamValueTypeDropdownItems) - 1); //-1 to remove UserDefined as an option
								ImGui::PopItemWidth();

								ImGui::TableNextColumn();
								ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
								ImGui::InputText(("##OutParamName_" + std::to_string(i)).c_str(), &Def->mOutputParameters[i].mName);
								ImGui::PopItemWidth();

								ImGui::TableNextColumn();
								ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
								if (ImGuiExt::ButtonDelete(("Delete##OutParamDelete_" + std::to_string(i)).c_str(), ImVec2(0, 0)))
								{
									Iter = Def->mOutputParameters.erase(Iter);
									continue;
								}
								ImGui::PopItemWidth();

								Iter++;
							}

							ImGui::EndTable();
						}

						ImGui::Unindent();
					}

					if (ImGuiExt::CollapsingHeaderWithAddButton("Internal Parameters", ImGuiTreeNodeFlags_None, [&Def]()
						{
							Def->mImmediateParameters.push_back(
								application::manager::AINBNodeMgr::NodeDef::ParameterDef
								{
									.mName = "NewParam",
									.mClass = "",
									.mValueType = application::file::game::ainb::AINBFile::ValueType::Int,
									.mFlags = {}
								}
							);
						}
					))
					{
						ImGui::Indent();

						if (ImGui::BeginTable("NewNodeInternalParamsTable", 3, ImGuiTableFlags_BordersInnerV))
						{
							for (auto Iter = Def->mImmediateParameters.begin(); Iter != Def->mImmediateParameters.end(); )
							{
								size_t i = std::distance(Def->mImmediateParameters.begin(), Iter);

								ImGui::TableNextRow();
								ImGui::TableSetColumnIndex(0);
								ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
								ImGui::Combo(("##ImmParamDataType_" + std::to_string(i)).c_str(), reinterpret_cast<int*>(&Def->mImmediateParameters[i].mValueType), gParamValueTypeDropdownItems, IM_ARRAYSIZE(gParamValueTypeDropdownItems) - 1); //-1 to remove UserDefined as an option
								ImGui::PopItemWidth();

								ImGui::TableNextColumn();
								ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
								ImGui::InputText(("##ImmParamName_" + std::to_string(i)).c_str(), &Def->mImmediateParameters[i].mName);
								ImGui::PopItemWidth();

								ImGui::TableNextColumn();
								ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
								if (ImGuiExt::ButtonDelete(("Delete##ImmParamDelete_" + std::to_string(i)).c_str(), ImVec2(0, 0)))
								{
									Iter = Def->mImmediateParameters.erase(Iter);
									continue;
								}
								ImGui::PopItemWidth();

								Iter++;
							}

							ImGui::EndTable();
						}

						ImGui::Unindent();
					}
				}).Register();
	}

	UIAINBEditor::UIAINBEditor(const std::string& AINBPath)
	{
		LoadAINB(AINBPath);
	}

	void UIAINBEditor::LoadAINB(const std::string& AINBPath)
	{
		mAINBPath = AINBPath;
		mAINBFile = application::file::game::ainb::AINBFile(AINBPath);

		if (gHeaderTexture == nullptr)
			gHeaderTexture = application::manager::TextureMgr::GetAssetTexture("HeaderBackground");
		
		mNodes.clear();
		mGraphRenderActions.clear();
		mRenderEndActions.clear();
		mContextNodeId = 0;
		mContextLinkId = 0;
		mNodeListSearchBar = "";
		mDetailsEditorContent.mContent = 0;
		mDetailsEditorContent.mType = DetailsEditorContentType::NONE;
		mNewNodeLinkState = NewNodeToLinkState::NONE;
		mNewNodeStartIndex = -1;

		for (application::file::game::ainb::AINBFile::Node& Node : mAINBFile.Nodes)
		{
			AddEditorNode(&Node);
		}
	}

	void UIAINBEditor::LoadAINB(const std::vector<unsigned char>& AINBBytes)
	{
		mAINBPath = "NONE";
		mAINBFile.Initialize(AINBBytes);

		if (gHeaderTexture == nullptr)
			gHeaderTexture = application::manager::TextureMgr::GetAssetTexture("HeaderBackground");

		mNodes.clear();
		mGraphRenderActions.clear();
		mRenderEndActions.clear();
		mContextNodeId = 0;
		mContextLinkId = 0;
		mNodeListSearchBar = "";
		mDetailsEditorContent.mContent = 0;
		mDetailsEditorContent.mType = DetailsEditorContentType::NONE;
		mNewNodeLinkState = NewNodeToLinkState::NONE;
		mNewNodeStartIndex = -1;

		for (application::file::game::ainb::AINBFile::Node& Node : mAINBFile.Nodes)
		{
			AddEditorNode(&Node);
		}
	}

	void UIAINBEditor::GraphDeselect(bool Nodes, bool Links)
	{
		std::vector<ed::NodeId> selectedNodes;
		std::vector<ed::LinkId> selectedLinks;
		selectedNodes.resize(ed::GetSelectedObjectCount());
		selectedLinks.resize(ed::GetSelectedObjectCount());

		int nodeCount = ed::GetSelectedNodes(selectedNodes.data(), static_cast<int>(selectedNodes.size()));
		int linkCount = ed::GetSelectedLinks(selectedLinks.data(), static_cast<int>(selectedLinks.size()));

		selectedNodes.resize(nodeCount);
		selectedLinks.resize(linkCount);

		if(Nodes)
		{
			for(ed::NodeId& Id : selectedNodes)
				ed::DeselectNode(Id);
		}
		if(Links)
		{
			for(ed::LinkId& Id : selectedLinks)
				ed::DeselectLink(Id);
		}
	}

	void UIAINBEditor::AddEditorNode(application::file::game::ainb::AINBFile::Node* Node, application::manager::AINBNodeMgr::NodeDef* Def)
	{
		if(Node == nullptr)
		{
			application::util::Logger::Error("UIAINBEditor", "Tried to add editor node, but AINB node was a nullptr");
			return;
		}

		switch (Node->Type)
		{
		case (uint16_t)application::file::game::ainb::AINBFile::NodeTypes::UserDefined:
			mNodes.push_back(std::make_unique<nodes::UIAINBEditorNodeDefault>(mNodeEditorUniqueId, *Node));

			if(Def != nullptr)
				static_cast<nodes::UIAINBEditorNodeDefault*>(mNodes.back().get())->InjectFixedFlowParameters(Def);
			
			mNodeEditorUniqueId += 1000;
			break;
		case (uint16_t)application::file::game::ainb::AINBFile::NodeTypes::Element_SplitTiming:
			mNodes.push_back(std::make_unique<nodes::UIAINBEditorNodeSplitTiming>(mNodeEditorUniqueId, *Node));
			mNodeEditorUniqueId += 1000;
			break;
		case (uint16_t)application::file::game::ainb::AINBFile::NodeTypes::Element_Simultaneous:
			mNodes.push_back(std::make_unique<nodes::UIAINBEditorNodeSimultaneous>(mNodeEditorUniqueId, *Node));
			mNodeEditorUniqueId += 1000;
			break;
		case (uint16_t)application::file::game::ainb::AINBFile::NodeTypes::Element_Sequential:
			mNodes.push_back(std::make_unique<nodes::UIAINBEditorNodeSequential>(mNodeEditorUniqueId, *Node));
			mNodeEditorUniqueId += 1000;
			break;
		case (uint16_t)application::file::game::ainb::AINBFile::NodeTypes::Element_BoolSelector:
			mNodes.push_back(std::make_unique<nodes::UIAINBEditorNodeBoolSelector>(mNodeEditorUniqueId, *Node));
			mNodeEditorUniqueId += 1000;
			break;
		case (uint16_t)application::file::game::ainb::AINBFile::NodeTypes::Element_S32Selector:
			mNodes.push_back(std::make_unique<nodes::UIAINBEditorNodeS32Selector>(mNodeEditorUniqueId, *Node));
			mNodeEditorUniqueId += 1000;
			break;
		/*
		case (uint16_t)application::file::game::ainb::AINBFile::NodeTypes::Element_F32Selector:
			mNodes.push_back(std::make_unique<nodes::UIAINBEditorNodeF32Selector>(mNodeEditorUniqueId, *Node));
			mNodeEditorUniqueId += 1000;
			break;
		*/
		default:
			application::util::Logger::Error("UIAINBEditor", "Unknown node type: %u", Node->Type);
			mNodes.push_back(std::make_unique<nodes::UIAINBEditorNodeDefault>(mNodeEditorUniqueId, *Node));
			mNodeEditorUniqueId += 1000;
			break;
		}
	}

	void UIAINBEditor::InitializeImpl()
	{
		application::util::Logger::Info("UIAINBEditor", "Initializing...");
		
		ed::Config Config;
		Config.SettingsFile = nullptr;
		Config.SaveSettings = UIAINBEditor::NodeEditorSaveSettings;
		mNodeEditorContext = ed::CreateEditor(&Config);
	}

	void UIAINBEditor::DrawGeneralWindow()
	{
		if(!mGeneralOpen)
			return;

		ImGui::Begin(CreateID("General").c_str(), &mGeneralOpen, ImGuiWindowFlags_NoCollapse);

		if (mAINBPath.empty())
		{
			ImGui::End();
			return;
		}

		if (ImGui::CollapsingHeader(CreateID("Settings").c_str(), ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Indent();

			if (ImGui::BeginTable(CreateID("SettingsTable").c_str(), 2, ImGuiTableFlags_BordersInnerV))
			{
				ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("File name").x);

				ImGui::TableNextRow();

				ImGui::TableSetColumnIndex(0);
				ImGui::Text("Category");
				ImGui::TableNextColumn();

				int CategorySelected = 0;
				if (mAINBFile.Header.FileCategory == "AI")
					CategorySelected = 1;
				else if (mAINBFile.Header.FileCategory == "Sequence")
					CategorySelected = 2;

				ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
				if (ImGui::Combo(CreateID("##SettingsCategory").c_str(), &CategorySelected, gCategoryDropdownItems, IM_ARRAYSIZE(gCategoryDropdownItems)))
				{
					if (CategorySelected == 0) mAINBFile.Header.FileCategory = "Logic";
					else if (CategorySelected == 1) mAINBFile.Header.FileCategory = "AI";
					else mAINBFile.Header.FileCategory = "Sequence";
				}
				ImGui::PopItemWidth();

				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::Text("File name");
				ImGui::TableNextColumn();
				ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
				ImGui::InputText(CreateID("##SettingsFileName").c_str(), &mAINBFile.Header.FileName);
				ImGui::PopItemWidth();

				ImGui::EndTable();
			}

			ImGui::Unindent();
		}

		auto OnEntryPointsAddPress = [this]()
		{
			application::file::game::ainb::AINBFile::Command Cmd;
			Cmd.IsLeftNodeResident = true;
			Cmd.IsRightNodeResident = false;
			Cmd.LeftNodeIndex = -1;
			Cmd.RightNodeIndex = -1;
			Cmd.Name = "Entry Point " + std::to_string(mAINBFile.Commands.size());

			mAINBFile.Commands.push_back(Cmd);

			DetailsEditorContentEntryPoint EntryPoint;
			EntryPoint.mCommand = &mAINBFile.Commands.back();
			
			mDetailsEditorContent.mType = DetailsEditorContentType::ENTRYPOINT;
			mDetailsEditorContent.mContent = EntryPoint;
		};

		bool Continue = false;
		if (ImGuiExt::CollapsingHeaderWithAddButton(CreateID("Entry Points").c_str(), ImGuiTreeNodeFlags_DefaultOpen, OnEntryPointsAddPress))
		{
			for (auto Iter = mAINBFile.Commands.begin(); Iter != mAINBFile.Commands.end(); )
			{
				if(ImGuiExt::SelectableTextOffsetWithDeleteButton(CreateID(Iter->Name + "##EntryPointName").c_str(), ImGui::GetStyle().IndentSpacing, [this, &Iter, &Continue]() 
					{ 
						Iter = mAINBFile.Commands.erase(Iter);
						Continue = true;
					}, mDetailsEditorContent.mType == DetailsEditorContentType::ENTRYPOINT && std::get<DetailsEditorContentEntryPoint>(mDetailsEditorContent.mContent).mCommand == &(*Iter)))
				{
					DetailsEditorContentEntryPoint EntryPoint;
					EntryPoint.mCommand = &(*Iter); //*Iter = Command&            &(Command&) = *Command
					
					mDetailsEditorContent.mType = DetailsEditorContentType::ENTRYPOINT;
					mDetailsEditorContent.mContent = EntryPoint;

					if(Iter->LeftNodeIndex >= 0 && mNodes.size() > Iter->LeftNodeIndex)
					{
						mGraphRenderActions.push_back(GraphRenderAction{
							.mFunc = [this, Iter]()
							{
								GraphDeselect();
								ed::SelectNode(mNodes[Iter->LeftNodeIndex]->mNodeId, true);
								ed::NavigateToSelection();
							}
						});
					}
				}
				if(Continue)
				{
					Continue = false;
					mDetailsEditorContent.mType = UIAINBEditor::DetailsEditorContentType::NONE;
					mDetailsEditorContent.mContent = 0;
					continue;
				}
				
				Iter++;
			}
		}

		auto OnLocalBlackboardAddPress = [this]()
		{
			uint32_t Count = 0;
			for (uint32_t Type = 0; Type < application::file::game::ainb::AINBFile::GlobalTypeCount; Type++)
			{
				for (application::file::game::ainb::AINBFile::GlobalEntry& Entry : mAINBFile.GlobalParameters[Type])
				{
					Count++;
				}
			}

			application::file::game::ainb::AINBFile::GlobalEntry Entry;
			Entry.Notes = "";
			Entry.Name = "Variable " + std::to_string(Count);
			Entry.GlobalValueType = (int)application::file::game::ainb::AINBFile::GlobalType::Int;
			Entry.GlobalValue = (uint32_t)0;
			Entry.FileReference.FileName = "";

			mAINBFile.GlobalParameters[(int)application::file::game::ainb::AINBFile::GlobalType::Int].push_back(Entry);

			DetailsEditorContentBlackBoard BlackBoard;
			BlackBoard.mVariable = &mAINBFile.GlobalParameters[(int)application::file::game::ainb::AINBFile::GlobalType::Int].back();
			
			mDetailsEditorContent.mType = DetailsEditorContentType::BLACKBOARD;
			mDetailsEditorContent.mContent = BlackBoard;
		};

		Continue = false;
		if (ImGuiExt::CollapsingHeaderWithAddButton(CreateID("Local Blackboard").c_str(), ImGuiTreeNodeFlags_DefaultOpen, OnLocalBlackboardAddPress))
		{
			for (uint32_t Type = 0; Type < application::file::game::ainb::AINBFile::GlobalTypeCount; Type++)
			{
				for (auto Iter = mAINBFile.GlobalParameters[Type].begin(); Iter != mAINBFile.GlobalParameters[Type].end(); )
				{
					if(ImGuiExt::SelectableTextOffsetWithDeleteButton(CreateID(Iter->Name).c_str(), ImGui::GetStyle().IndentSpacing, [this, &Iter, &Continue, &Type]() 
						{
							Iter = mAINBFile.GlobalParameters[Type].erase(Iter);
							Continue = true;
						}, mDetailsEditorContent.mType == DetailsEditorContentType::BLACKBOARD && std::get<DetailsEditorContentBlackBoard>(mDetailsEditorContent.mContent).mVariable == &(*Iter)))
					{
						DetailsEditorContentBlackBoard BlackBoard;
						BlackBoard.mVariable = &(*Iter);
						
						mDetailsEditorContent.mType = DetailsEditorContentType::BLACKBOARD;
						mDetailsEditorContent.mContent = BlackBoard;
					}
					if(Continue)
					{
						Continue = false;
						mDetailsEditorContent.mType = UIAINBEditor::DetailsEditorContentType::NONE;
						mDetailsEditorContent.mContent = 0;
						continue;
					}
					Iter++;
				}
			}
		}

		ImGui::End();
	}

	void UIAINBEditor::UpdateEditorNodeIndices()
	{
		for(size_t i = 0; i < mNodes.size(); i++)
		{
			mNodes[i]->mNode = &mAINBFile.Nodes[i];
		}
	}

	void UIAINBEditor::DrawNodeListWindow()
	{
		if(!mNodeListOpen)
			return;

		ImGui::Begin(CreateID("Node List").c_str(), &mNodeListOpen, ImGuiWindowFlags_NoCollapse);

		ImGui::PushItemWidth(ImGui::GetCurrentWindow()->ParentWorkRect.GetSize().x);
		ImGui::InputTextWithHint(CreateID("##NodeListSearchBar").c_str(), "Search...", &mNodeListSearchBar);
		ImGui::PopItemWidth();

		if (mAINBPath.empty())
		{
			ImGui::End();
			return;
		}

		for(size_t i = 0; i < mNodes.size(); i++)
		{
			std::string Name = mNodes[i]->mNode->GetName() + "(#" + std::to_string(i) + ")";

			if((mNodeListSearchBar.empty() || std::search(Name.begin(), Name.end(), mNodeListSearchBar.begin(),
                 mNodeListSearchBar.end(), [](char a, char b) {
					return std::tolower(a) == std::tolower(b);
				 }) != Name.end()) && ImGui::Selectable(Name.c_str(), mContextNodeId.Get() == (i * 1000 + 1)))
			{
				mGraphRenderActions.push_back(GraphRenderAction{
					.mFunc = [this, i]()
					{
						GraphDeselect();
						ed::SelectNode(mNodes[i]->mNodeId, true);
						ed::NavigateToSelection();
					}
				});
			}

			/*
			if(ImGui::IsMouseClicked(ImGuiMouseButton_Right) && ImGui::IsMouseHoveringRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax()))
			{
				application::util::Logger::Info("ad", "ad");
				ImGui::OpenPopup("Node Context Menu");
			}
			*/
		}

		if (mAINBPath.empty())
			ImGui::EndDisabled();

		ImGui::End();
	}

	void UIAINBEditor::DrawGraphWindow()
	{
		if(!mGraphOpen)
			return;

		ImGui::Begin(CreateID("Graph").c_str(), &mGraphOpen, ImGuiWindowFlags_NoCollapse);
		ed::SetCurrentEditor(mNodeEditorContext);

		//ImGui::Text("Zoom: %f", ed::GetCurrentZoom());

		ed::Begin("AINBNodeEditor", ImVec2(0.0f, 0.0f));

		if (mAINBPath.empty())
		{
			ed::End();
			ed::SetCurrentEditor(nullptr);
			ImGui::End();
			return;
		}

		ImVec2 WindowPos = ImGui::GetWindowPos();
		
		for (std::unique_ptr<UIAINBEditorNodeBase>& Node : mNodes)
		{
			Node->Reset();
			Node->mEnableFlow = mAINBFile.Header.FileCategory != "Logic";
		}

		for(application::file::game::ainb::AINBFile::Command& Cmd : mAINBFile.Commands)
		{
			if(Cmd.LeftNodeIndex < 0 || mNodes.size() <= Cmd.LeftNodeIndex)
				continue;
			
			mNodes[Cmd.LeftNodeIndex]->mIsEntryPoint = true;
		}

		for (int i = 0; i < mAINBFile.Nodes.size(); i++)
		{
			for (int j = 0; j < application::file::game::ainb::AINBFile::ValueTypeCount; j++) 
			{
				for (application::file::game::ainb::AINBFile::InputEntry& Entry : mAINBFile.Nodes[i].InputParameters[j]) 
				{
					if (Entry.NodeIndex >= 0) 
					{
						mNodes[Entry.NodeIndex]->mLinkedOutputParams[j].push_back(Entry.ParameterIndex);
					}
					else 
					{
						for (application::file::game::ainb::AINBFile::MultiEntry& Multi : Entry.Sources) 
						{
							mNodes[Multi.NodeIndex]->mLinkedOutputParams[j].push_back(Multi.ParameterIndex);
						}
					}
				}
			}
		}

		for (std::unique_ptr<UIAINBEditorNodeBase>& Node : mNodes)
		{
			Node->Draw();
			Node->mFlowLinked = false;
		}

		for (std::unique_ptr<UIAINBEditorNodeBase>& Node : mNodes)
		{
			Node->RenderLinks(mNodes);
		}

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

			auto FindPin = [this](uint32_t Id)
			{
				for (auto& Node : mNodes)
				{
					if (Node->mPins.contains(Id))
					{
						return &Node->mPins[Id];
					}
				}
				return (UIAINBEditorNodeBase::Pin*)nullptr;
			};

			auto FindNodeIndex = [this](application::file::game::ainb::AINBFile::Node* Parent)
			{
				for (int i = 0; i < mAINBFile.Nodes.size(); i++)
				{
					if (&mAINBFile.Nodes[i] == Parent)
					{
						return i;
					}
				}
				return -1;
			};

			ed::PinId StartPinId, EndPinId = 0;
			if (ed::QueryNewLink(&StartPinId, &EndPinId))
			{
				UIAINBEditorNodeBase::Pin* StartPin = FindPin(StartPinId.Get());
				UIAINBEditorNodeBase::Pin* EndPin = FindPin(EndPinId.Get());

				if (StartPin != nullptr && EndPin != nullptr)
				{
					if (StartPin->mKind == ed::PinKind::Input)
					{
						std::swap(StartPin, EndPin);
						std::swap(StartPinId, EndPinId);
					}

					if (StartPin == EndPin)
					{
						ed::RejectNewItem(ImColor(255, 0, 0), 2.0f);
					}
					else if (StartPin->mKind == EndPin->mKind)
					{
						ShowLabel("x Incompatible Pin Kind", ImColor(45, 32, 32, 180));
						ed::RejectNewItem(ImColor(255, 0, 0), 2.0f);
					}
					else if (StartPin->mType != EndPin->mType)
					{
						ShowLabel("x Incompatible Pin Type", ImColor(45, 32, 32, 180));
						ed::RejectNewItem(ImColor(255, 128, 128), 1.0f);
					}
					else if (std::abs((int)StartPinId.Get() - (int)EndPinId.Get()) < 500)
					{
						ShowLabel("x Can't link to self", ImColor(45, 32, 32, 180));
						ed::RejectNewItem(ImColor(255, 0, 0), 1.0f);
					}
					else
					{
						int16_t NodeIndex = FindNodeIndex(StartPin->mNode);

						if (EndPin->mType != UIAINBEditorNodeBase::PinType::Flow)
						{
							application::file::game::ainb::AINBFile::InputEntry& Input = *(application::file::game::ainb::AINBFile::InputEntry*)EndPin->mObjectPtr;
							//AINBFile::OutputEntry& Output = *(AINBFile::OutputEntry*)StartPin->ObjectPtr;
							if (Input.NodeIndex == NodeIndex && Input.ParameterIndex == StartPin->mParameterIndex)
							{
								ShowLabel("x Already linked", ImColor(45, 32, 32, 180));
								ed::RejectNewItem(ImColor(255, 128, 128), 1.0f);
								ed::EndCreate();
								goto AbortLinkCreation;
							}
							else
							{
								for (application::file::game::ainb::AINBFile::MultiEntry& Entry : Input.Sources)
								{
									if (Entry.NodeIndex == NodeIndex && Entry.ParameterIndex == StartPin->mParameterIndex)
									{
										ShowLabel("x Already linked", ImColor(45, 32, 32, 180));
										ed::RejectNewItem(ImColor(255, 128, 128), 1.0f);
										ed::EndCreate();
										goto AbortLinkCreation;
									}
								}
							}
						}
						else 
						{ //Flow checks
							if (StartPin->mAlreadyLinked && !StartPin->mAllowMultipleLinks) {
								ShowLabel("x Flow parameter already linked", ImColor(45, 32, 32, 180));
								ed::RejectNewItem(ImColor(255, 128, 128), 1.0f);
								ed::EndCreate();
								goto AbortLinkCreation;
							}
						}

						ShowLabel("+ Create Link", ImColor(32, 45, 32, 180));
						if (ed::AcceptNewItem(ImColor(128, 255, 128), 4.0f))
						{
							//Parameter link
							if (EndPin->mType != UIAINBEditorNodeBase::PinType::Flow)
							{
								application::file::game::ainb::AINBFile::InputEntry& Input = *(application::file::game::ainb::AINBFile::InputEntry*)EndPin->mObjectPtr;
								application::file::game::ainb::AINBFile::OutputEntry& Output = *(application::file::game::ainb::AINBFile::OutputEntry*)StartPin->mObjectPtr;

								//Input param has not been linked yet
								if (Input.NodeIndex < 0 && Input.Sources.empty())
								{
									Input.ParameterIndex = StartPin->mParameterIndex;
									Input.NodeIndex = NodeIndex;
								}
								else if (Input.NodeIndex >= 0)
								{
									Input.Sources.clear();
									Input.Sources.push_back(application::file::game::ainb::AINBFile::MultiEntry
									{
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

									Input.Sources.push_back(application::file::game::ainb::AINBFile::MultiEntry
									{
										.NodeIndex = (uint16_t)NodeIndex,
										.ParameterIndex = (uint16_t)StartPin->mParameterIndex
									});
								}
								else
								{
									Input.Sources.push_back(application::file::game::ainb::AINBFile::MultiEntry
									{
										.NodeIndex = (uint16_t)NodeIndex,
										.ParameterIndex = (uint16_t)StartPin->mParameterIndex 
									});
								}
							}
							else
							{ //Flow Link
								application::file::game::ainb::AINBFile::LinkedNodeInfo Info;
								Info.NodeIndex = EndPin->mNode->NodeIndex;
								Info.Condition = "";
								mNodes[StartPin->mNode->NodeIndex]->PostProcessLinkedNodeInfo(*StartPin, Info);
								StartPin->mNode->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink].push_back(Info);
							}
						}
					}
				}
			}

			ed::PinId PinId = 0;
			if (ed::QueryNewNode(&PinId))
			{
				UIAINBEditorNodeBase::Pin* StartPin = FindPin(PinId.Get());
				if(StartPin != nullptr)
				{
					if (!(StartPin->mAlreadyLinked && !StartPin->mAllowMultipleLinks))
					{
						ShowLabel("+ Create Node", ImColor(32, 45, 32, 180));
						if (ed::AcceptNewItem())
						{
							mNewNodeLinkPin = *StartPin;
							mNewNodeLinkState = NewNodeToLinkState::INIT;
							mNewNodeStartIndex = StartPin->mNode->NodeIndex;
							ed::Suspend();
							ImGui::OpenPopup("Add Node");
							mFocusAddNodeSearchBar = true;
							ed::Resume();
						}
					}
					else
					{
						ShowLabel("x Flow parameter already linked", ImColor(45, 32, 32, 180));
					}
				}
			}
			ed::EndCreate();
		}
	AbortLinkCreation:

		ImVec2 OpenPopupPosition = ImGui::GetMousePos();
		if(mNewNodeLinkState == NewNodeToLinkState::INIT)
		{
			mOpenPopupPosition = OpenPopupPosition;
		}
		ed::Suspend();
		if (ed::ShowLinkContextMenu(&mContextLinkId))
		{
			ImGui::OpenPopup("Link Context Menu");
			mOpenPopupPosition = OpenPopupPosition;
		}
		if (ed::ShowNodeContextMenu(&mContextNodeId))
		{
			ImGui::OpenPopup("Node Context Menu");
			mOpenPopupPosition = OpenPopupPosition;
		}
		if(ed::ShowBackgroundContextMenu())
		{
			ImGui::OpenPopup("Add Node");
			mOpenPopupPosition = OpenPopupPosition;
			mFocusAddNodeSearchBar = true;
		}

		if (ImGui::BeginPopup("Link Context Menu"))
		{
			if (ImGui::MenuItem("Delete##DelLink"))
			{
				DeleteNodeLink(mContextLinkId);
			}
			ImGui::EndPopup();
		}
		if (ImGui::BeginPopup("Node Context Menu"))
		{
			if (ImGui::MenuItem("Delete##DelNode"))
			{
				DeleteNode(mContextNodeId);
			}
			ImGui::Separator();

			std::unique_ptr<UIAINBEditorNodeBase>* NodePtr = nullptr;

			for (auto& Node : mNodes)
			{
				if (Node->mNodeId == mContextNodeId.Get())
				{
					NodePtr = &Node;
					break;
				}
			}

			if(!NodePtr || NodePtr->get()->mNode->Type != (uint16_t)application::file::game::ainb::AINBFile::NodeTypes::UserDefined)
				ImGui::BeginDisabled();

			if(ImGui::MenuItem("Print file usage to console"))
			{
				application::util::Logger::Info("UIAINBEditor", "%s is used in the following AINB files:", NodePtr->get()->mNode->GetName().c_str());
				for(application::manager::AINBNodeMgr::NodeDef& Def : application::manager::AINBNodeMgr::gNodeDefinitions)
				{
					if(Def.mName == NodePtr->get()->mNode->Name)
					{
						for(const std::string& Str : Def.mFileNames)
						{
							application::util::Logger::Info("UIAINBEditor", Str);
						}
						break;
					}
				}
			}
			if(!NodePtr || NodePtr->get()->mNode->Type != (uint16_t)application::file::game::ainb::AINBFile::NodeTypes::UserDefined)
				ImGui::EndDisabled();

			if(mAINBFile.Header.FileCategory != "Logic")
			{
				if(NodePtr)
				{
					if(NodePtr->get()->mIsEntryPoint)
						ImGui::BeginDisabled();

					if(ImGui::MenuItem("Add new Entry Point here"))
					{
						application::file::game::ainb::AINBFile::Command Cmd;
						Cmd.LeftNodeIndex = NodePtr->get()->mNode->NodeIndex;
						std::string Name = NodePtr->get()->mNode->GetName();
						uint32_t NameCount = 0;
						for(application::file::game::ainb::AINBFile::Command& Cmd : mAINBFile.Commands)
						{
							if(Cmd.Name == Name)
								NameCount++;
						}
						if(NameCount > 0)
						{
							Name += " (" + std::to_string(NameCount) + ")";
						}
						Cmd.IsLeftNodeResident = true;
						Cmd.IsRightNodeResident = false;
						Cmd.RightNodeIndex = -1;
						Cmd.Name = Name;
						mAINBFile.Commands.push_back(Cmd);

						DetailsEditorContentEntryPoint Point;
						Point.mCommand = &mAINBFile.Commands.back();

						mDetailsEditorContent.mContent = Point;
						mDetailsEditorContent.mType = DetailsEditorContentType::ENTRYPOINT;
					}
					if(NodePtr->get()->mIsEntryPoint)
						ImGui::EndDisabled();
				}
			}
			ImGui::EndPopup();
		}

		ImGui::SetNextWindowSize(ImVec2(0, ImGui::CalcTextSize("a").y * 20.0f));
		bool WantNewNode = false;
		if(ImGui::BeginPopup("Add Node", ImGuiWindowFlags_NoScrollbar))
		{
			if(mNewNodeLinkState == NewNodeToLinkState::INIT)
			{
				mNewNodeLinkState = NewNodeToLinkState::WAITING;
			}
			ImGui::Text("Add Node");
			ImGui::SameLine();
			ImGui::Checkbox("Show all nodes", &mShowAllNodes);
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
			{
				ImGui::SetTooltip("Some nodes are tied to a specific AINB file category (AI, Logic or Sequence). \nThis tool only shows the nodes allowed in the current category. \nBy enabling this option, all nodes will be shown.");
			}
			ImGui::SameLine();
			WantNewNode = ImGui::Button("Create new node");
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
			{
				ImGui::SetTooltip("Event entry nodes (or module nodes) do not have to be registered in the executable.\nBy pressing this button, you can create entirely new nodes and use them as entry points for events (or create module nodes).");
			}
			ImGui::Separator();

			ImGui::Dummy(ImVec2(application::manager::AINBNodeMgr::gMaxDisplayLength, 0));
			
			ImGui::PushItemWidth(ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x);
			if(ImGui::InputTextWithHint("##AddNodeSearchBar", "Search...", &mAddNodeSearchBar))
			{
				if(mAddNodeSearchBar.length() == 1 && mCategoriesOriginalState.empty())
				{
					for(auto& [Tag, Nodes] : application::manager::AINBNodeMgr::gNodeCategories)
					{
						std::string ID = CreateID(Tag);
						ImGuiID GuiID = ImGui::GetID(ID.c_str());
						mCategoriesOriginalState[ID] = ImGui::GetStateStorage()->GetInt(GuiID);
						ImGui::GetStateStorage()->SetInt(GuiID, 1);
					}
				}
				else if(mAddNodeSearchBar.empty())
				{
					for(auto& [Tag, Nodes] : application::manager::AINBNodeMgr::gNodeCategories)
					{
						std::string ID = CreateID(Tag);
						ImGui::GetStateStorage()->SetInt(ImGui::GetID(ID.c_str()), mCategoriesOriginalState[ID]);
					}
					mCategoriesOriginalState.clear();
				}
			}
			ImGui::PopItemWidth();
			if(mFocusAddNodeSearchBar)
			{
				ImGui::SetKeyboardFocusHere(-1);
				ImGuiInputTextState* State = ImGui::GetInputTextState(ImGui::GetID("##AddNodeSearchBar"));
				if(State && !mAddNodeSearchBar.empty())
				{
					State->SelectAll();
				}
				mFocusAddNodeSearchBar = false;
			}

			for(auto& [Tag, Nodes] : application::manager::AINBNodeMgr::gNodeCategories)
			{
				if((application::manager::AINBNodeMgr::gHasNodeCategoryNodes[mAINBFile.Header.FileCategory][Tag] || mShowAllNodes) && ImGui::CollapsingHeader(CreateID(Tag).c_str()))
				{
					ImGui::Indent();
					
					for(application::manager::AINBNodeMgr::NodeDef* Def : Nodes)
					{
						if((mAddNodeSearchBar.empty() || std::search(Def->mName.begin(), Def->mName.end(), mAddNodeSearchBar.begin(),
							mAddNodeSearchBar.end(), [](char a, char b) {
								return std::tolower(a) == std::tolower(b);
							}) != Def->mName.end()) && (std::find(Def->mCategories.begin(), Def->mCategories.end(), application::manager::AINBNodeMgr::gFileToNodeCategory[mAINBFile.Header.FileCategory]) != Def->mCategories.end() || mShowAllNodes) && ImGui::Selectable(Def->mName.c_str()))
						{
							application::manager::AINBNodeMgr::AddNode(mAINBFile, Def);
							AddEditorNode(&mAINBFile.Nodes.back(), Def);
							UpdateEditorNodeIndices();

							uint32_t LeftNodeIndex = mNewNodeStartIndex;
							uint32_t RightNodeIndex = mNodes.size() - 1;

							if(mNewNodeLinkPin.mKind == ed::PinKind::Input)
							{
								std::swap(LeftNodeIndex, RightNodeIndex);
							}

							if(mNewNodeLinkState == application::rendering::ainb::UIAINBEditor::NewNodeToLinkState::WAITING)
							{
								if(mNewNodeLinkPin.mType == UIAINBEditorNodeBase::PinType::Flow)
								{
									if(mNewNodeLinkPin.mKind == ed::PinKind::Output || (mNewNodeLinkPin.mKind == ed::PinKind::Input && mNodes[LeftNodeIndex]->HasFlowOutputParameters()))
									{
										mNewNodeLinkPin.mNode = mNodes[mNewNodeStartIndex]->mNode;
										if(mNewNodeLinkPin.mKind == ed::PinKind::Input)
										{
											mNewNodeLinkPin = UIAINBEditorNodeBase::Pin {.mKind = ed::PinKind::Output, .mType = UIAINBEditorNodeBase::PinType::Flow, .mNode = mNodes[LeftNodeIndex]->mNode, .mParameterIndex = 0, .mAllowMultipleLinks = false, .mAlreadyLinked = false };
										}

										application::file::game::ainb::AINBFile::LinkedNodeInfo Info;
										Info.NodeIndex = mNodes[RightNodeIndex]->mNode->NodeIndex;
										Info.Condition = "";
										mNodes[LeftNodeIndex]->PostProcessLinkedNodeInfo(mNewNodeLinkPin, Info);
										mNodes[LeftNodeIndex]->mNode->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink].push_back(Info);
									}
								}
								else
								{
									application::file::game::ainb::AINBFile::InputEntry* Input = mNodes[RightNodeIndex]->mNode->InputParameters[(int)mNewNodeLinkPin.mType].size() <= mNewNodeLinkPin.mParameterIndex ? nullptr : &mNodes[RightNodeIndex]->mNode->InputParameters[(int)mNewNodeLinkPin.mType][mNewNodeLinkPin.mParameterIndex];
									if(Input != nullptr)
									{
										if(mNewNodeLinkPin.mKind == ed::PinKind::Input)
										{
											mNewNodeLinkPin = UIAINBEditorNodeBase::Pin {.mKind = ed::PinKind::Output, .mType = static_cast<UIAINBEditorNodeBase::PinType>(Input->ValueType), .mNode = mNodes[LeftNodeIndex]->mNode, .mParameterIndex = 0 };

											if(mNodes[LeftNodeIndex]->mNode->OutputParameters[(int)mNewNodeLinkPin.mType].empty())
												goto NoParamFound;
										}
										//Input param has not been linked yet
										if (Input->NodeIndex < 0 && Input->Sources.empty())
										{
											Input->ParameterIndex = mNewNodeLinkPin.mParameterIndex;
											Input->NodeIndex = LeftNodeIndex;
										}
										else if (Input->NodeIndex >= 0)
										{
											Input->Sources.clear();
											Input->Sources.push_back(application::file::game::ainb::AINBFile::MultiEntry
											{
												.NodeIndex = (uint16_t)Input->NodeIndex,
												.ParameterIndex = (uint16_t)Input->ParameterIndex,
												.Flags = Input->Flags,
												.GlobalParametersIndex = Input->GlobalParametersIndex,
												.EXBIndex = Input->EXBIndex,
												.Function = Input->Function
											});

											Input->NodeIndex = -1;
											Input->ParameterIndex = 0;
											Input->Flags.clear();
											Input->GlobalParametersIndex = 0xFFFF;
											Input->EXBIndex = 0xFFFF;

											Input->Sources.push_back(application::file::game::ainb::AINBFile::MultiEntry
											{
												.NodeIndex = (uint16_t)LeftNodeIndex,
												.ParameterIndex = (uint16_t)mNewNodeLinkPin.mParameterIndex
											});
										}
										else
										{
											Input->Sources.push_back(application::file::game::ainb::AINBFile::MultiEntry
											{
												.NodeIndex = (uint16_t)LeftNodeIndex,
												.ParameterIndex = (uint16_t)mNewNodeLinkPin.mParameterIndex 
											});
										}
									}
								}
							NoParamFound:
								mNewNodeStartIndex = -1;
								mNewNodeLinkState = application::rendering::ainb::UIAINBEditor::NewNodeToLinkState::NONE;
							}

							GraphRenderAction Action;
							Action.mFrameDelay = 1;
							Action.mFunc = [this]()
							{
								ed::SetNodePosition(mNodes[mAINBFile.Nodes.size() - 1]->mNodeId, mOpenPopupPosition);
								ed::SelectNode(mNodes[mAINBFile.Nodes.size() - 1]->mNodeId, true);
							};
							mGraphRenderActions.push_back(Action);
						}
					}

					ImGui::Unindent();
				}
			}

			ImGui::EndPopup();
		}
		else if(mNewNodeLinkState == NewNodeToLinkState::WAITING)
		{
			mNewNodeLinkState = NewNodeToLinkState::NONE;
		}

		if (WantNewNode)
		{
			ImGui::CloseCurrentPopup();
			mNewNodeLinkState = NewNodeToLinkState::NONE;

			gCreateNewNodePopUp.Open([this](application::rendering::popup::PopUpBuilder& Builder)
				{
					application::manager::AINBNodeMgr::NodeDef& Def = *reinterpret_cast<application::manager::AINBNodeMgr::NodeDef*>(Builder.GetDataStorage(0).mPtr);
					Def.mNodeType = application::file::game::ainb::AINBFile::NodeTypes::UserDefined;
					Def.mNameHash = application::util::Math::StringHashMurMur3_32(Def.mName);
					Def.mProjectName = application::manager::ProjectMgr::gProject;

					application::file::tool::AdditionalAINBNodes::gAdditionalNodeDefs.push_back(Def);
					application::manager::AINBNodeMgr::ReloadAdditionalProjectNodes();
				});

			/*
			application::manager::AINBNodeMgr::NodeDef Def;
			Def.mProjectName = application::manager::ProjectMgr::gProject;
			Def.mName = "EventStarlightTestNode";
			Def.mCategories.push_back(application::manager::AINBNodeMgr::NodeDef::Category::AI);
			Def.mNodeType = application::file::game::ainb::AINBFile::NodeTypes::UserDefined;
			Def.mFlowOutputParameters.push_back("Execute");

			application::manager::AINBNodeMgr::NodeDef::ParameterDef InParam;
			InParam.mName = "TestInput";
			InParam.mValueType = application::file::game::ainb::AINBFile::ValueType::Int;

			application::manager::AINBNodeMgr::NodeDef::ParameterDef OutParam;
			OutParam.mName = "TestOutput";
			OutParam.mValueType = application::file::game::ainb::AINBFile::ValueType::String;

			application::manager::AINBNodeMgr::NodeDef::ParameterDef ImmParam;
			ImmParam.mName = "TestImmediate";
			ImmParam.mValueType = application::file::game::ainb::AINBFile::ValueType::Bool;

			Def.mInputParameters.push_back(InParam);
			Def.mOutputParameters.push_back(OutParam);
			Def.mImmediateParameters.push_back(ImmParam);

			application::file::tool::AdditionalAINBNodes::gAdditionalNodeDefs.push_back(Def);
			application::manager::AINBNodeMgr::ReloadAdditionalProjectNodes();
			*/
		}

		if (ed::GetSelectedLinks(&mContextLinkId, 1) == 0)
			mContextLinkId = 0;
		if (ed::GetSelectedNodes(&mContextNodeId, 1) == 0)
			mContextNodeId = 0;

		ed::Resume();

		for(auto Iter = mGraphRenderActions.begin(); Iter != mGraphRenderActions.end(); )
		{
			if(Iter->mFrameDelay == 0)
			{
				Iter->mFunc();
				Iter = mGraphRenderActions.erase(Iter);
				continue;
			}

			Iter->mFrameDelay--;
			Iter++;
		}

		if(mWantAutoLayout)
		{
			AutoLayout();
		}

		ed::End();

		if (mAINBPath.empty())
			ImGui::EndDisabled();

		ed::SetCurrentEditor(nullptr);
		ImGui::End();
	}

	void UIAINBEditor::DeleteNodeLink(ed::LinkId LinkId)
	{
		for (auto& Node : mNodes)
		{
			if (Node->mLinks.contains(LinkId.Get()))
			{
				UIAINBEditorNodeBase::Link& Link = Node->mLinks[LinkId.Get()];

				if (Link.mType == UIAINBEditorNodeBase::LinkType::Parameter)
				{
					application::file::game::ainb::AINBFile::InputEntry* Parameter = (application::file::game::ainb::AINBFile::InputEntry*)Link.mObjectPtr;
					if (Parameter->NodeIndex >= 0) 
					{
						Parameter->NodeIndex = -1;
						Parameter->ParameterIndex = 0;
					}
					else
					{
						Parameter->Sources.erase(std::remove_if(Parameter->Sources.begin(), Parameter->Sources.end(),
							[Link](application::file::game::ainb::AINBFile::MultiEntry& Entry) { return Entry.NodeIndex == Link.mNodeIndex && Entry.ParameterIndex == Link.mParameterIndex; }),
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
				else if (Link.mType == UIAINBEditorNodeBase::LinkType::Flow)
				{
					application::file::game::ainb::AINBFile::Node* Node = (application::file::game::ainb::AINBFile::Node*)Link.mObjectPtr;
					Node->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink].erase(Node->LinkedNodes[(int)application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink].begin() + Link.mParameterIndex);
				}

				ed::DeleteLink(LinkId);

				break;
			}
		}
	}

	void UIAINBEditor::DeleteNode(ed::NodeId NodeId)
	{
		application::file::game::ainb::AINBFile::Node* NodePtr = nullptr;
		mNodes.erase(std::remove_if(mNodes.begin(), mNodes.end(),
			[NodeId, &NodePtr](std::unique_ptr<UIAINBEditorNodeBase>& NodeBase) {
				bool Delete = NodeBase->mNodeId == NodeId.Get();
				if (Delete)
				{
					NodePtr = NodeBase->mNode;
				}
				return Delete;
			}),
			mNodes.end());

		if (NodePtr == nullptr)
			return;

		uint32_t NodeIndex = NodePtr->NodeIndex;

		mAINBFile.Nodes.erase(std::remove_if(mAINBFile.Nodes.begin(), mAINBFile.Nodes.end(),
			[NodeId, NodePtr](application::file::game::ainb::AINBFile::Node& Node) { return &Node == NodePtr; }),
			mAINBFile.Nodes.end());
		ed::DeleteNode(NodeId);

		NodePtr = nullptr;

		for (size_t i = 0; i < mNodes.size(); i++)
		{
			mNodes[i]->mNode = &mAINBFile.Nodes[i];
		}

		for (auto& Node : mNodes)
		{
			for (int ValueType = 0; ValueType < application::file::game::ainb::AINBFile::ValueTypeCount; ValueType++)
			{
				for (application::file::game::ainb::AINBFile::InputEntry& Entry : Node->mNode->InputParameters[ValueType])
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
					if (Entry.Sources.size() == 1)
					{
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
			for (int LinkedInfoType = 0; LinkedInfoType < application::file::game::ainb::AINBFile::LinkedNodeTypeCount; LinkedInfoType++)
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

		//Updating node indices
		for (uint32_t i = 0; i < mAINBFile.Nodes.size(); i++)
		{
			mAINBFile.Nodes[i].NodeIndex = i;
		}
	}

	void UIAINBEditor::DrawDetailsWindow()
	{
		if(!mDetailsOpen)
			return;

		ImGui::Begin(CreateID("Details").c_str(), &mDetailsOpen, ImGuiWindowFlags_NoCollapse);

		if (mAINBPath.empty())
		{
			ImGui::End();
			return;
		}

		if(mDetailsEditorContent.mType == DetailsEditorContentType::ENTRYPOINT)
		{
			if(ImGui::CollapsingHeader(CreateID("General##EntryPointHeader").c_str(), ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::Indent();
				if (ImGui::BeginTable(CreateID("EntryPointTable").c_str(), 2, ImGuiTableFlags_BordersInnerV))
				{
					ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("Node Index").x + ImGui::GetStyle().FramePadding.x);

					ImGui::TableNextRow();

					ImGui::TableSetColumnIndex(0);
					ImGui::Text("Name");
					ImGui::TableNextColumn();

					ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
					ImGui::InputText(CreateID("##EntryPointName").c_str(), &(std::get<DetailsEditorContentEntryPoint>(mDetailsEditorContent.mContent).mCommand->Name));
					ImGui::PopItemWidth();
					ImGui::TableNextColumn();

					ImGui::Text("Node Index");
					ImGui::TableNextColumn();

					ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
					int16_t OldNodeIndex = std::get<DetailsEditorContentEntryPoint>(mDetailsEditorContent.mContent).mCommand->LeftNodeIndex;
					if(ImGui::InputScalar(CreateID("##EntryPointLeftNodeIndex").c_str(), ImGuiDataType_S16, &(std::get<DetailsEditorContentEntryPoint>(mDetailsEditorContent.mContent).mCommand->LeftNodeIndex)))
					{
						if (mAINBFile.Nodes.size() > OldNodeIndex && OldNodeIndex >= 0)
						{
							mAINBFile.Nodes[OldNodeIndex].Flags.erase(std::remove_if(
								mAINBFile.Nodes[OldNodeIndex].Flags.begin(), mAINBFile.Nodes[OldNodeIndex].Flags.end(),
								[](const application::file::game::ainb::AINBFile::FlagsStruct& Flag) {
									return Flag == application::file::game::ainb::AINBFile::FlagsStruct::IsResidentNode;
								}), mAINBFile.Nodes[OldNodeIndex].Flags.end());
						}

						if(std::get<DetailsEditorContentEntryPoint>(mDetailsEditorContent.mContent).mCommand->LeftNodeIndex >= 0 && mNodes.size() > std::get<DetailsEditorContentEntryPoint>(mDetailsEditorContent.mContent).mCommand->LeftNodeIndex)
						{
							mGraphRenderActions.push_back(GraphRenderAction
								{
									.mFunc = [this]()
									{
										GraphDeselect();
										ed::SelectNode(mNodes[std::get<DetailsEditorContentEntryPoint>(mDetailsEditorContent.mContent).mCommand->LeftNodeIndex]->mNodeId, true);
										ed::NavigateToSelection();	
									}
							});
						}
					}
					ImGui::PopItemWidth();

					ImGui::EndTable();
				}
				ImGui::Unindent();
			}

			if(ImGui::CollapsingHeader(CreateID("Advanced##EntryPointHeader").c_str()))
			{
				ImGui::Indent();
				if (ImGui::BeginTable(CreateID("EntryPointAdvancedTable").c_str(), 2, ImGuiTableFlags_BordersInnerV))
				{
					ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, ImGui::GetWindowSize().x * 0.5f);

					ImGui::TableNextRow();

					ImGui::TableSetColumnIndex(0);
					ImGui::Text("Is Left Node Resident");
					ImGui::TableNextColumn();
					ImGui::Checkbox(CreateID("##EntryPointLeftResident").c_str(), &(std::get<DetailsEditorContentEntryPoint>(mDetailsEditorContent.mContent).mCommand->IsLeftNodeResident));
					ImGui::TableNextColumn();

					ImGui::Text("Is Right Node Resident");
					ImGui::TableNextColumn();
					ImGui::Checkbox(CreateID("##EntryPointRightResident").c_str(), &(std::get<DetailsEditorContentEntryPoint>(mDetailsEditorContent.mContent).mCommand->IsRightNodeResident));
					ImGui::TableNextColumn();

					ImGui::Text("Right Node Index");
					ImGui::TableNextColumn();
					ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
					ImGui::InputScalar(CreateID("##EntryPointRightNodeIndex").c_str(), ImGuiDataType_S16, &(std::get<DetailsEditorContentEntryPoint>(mDetailsEditorContent.mContent).mCommand->RightNodeIndex));
					ImGui::PopItemWidth();

					ImGui::EndTable();
				}
				ImGui::Unindent();
			}
		}
		else if(mDetailsEditorContent.mType == DetailsEditorContentType::BLACKBOARD)
		{
			application::file::game::ainb::AINBFile::GlobalEntry* Entry = std::get<DetailsEditorContentBlackBoard>(mDetailsEditorContent.mContent).mVariable;
			if(ImGui::CollapsingHeader(CreateID("General##BlackBoardHeader").c_str(), ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::Indent();
				if (ImGui::BeginTable(CreateID("BlackBoardTableGeneral").c_str(), 2, ImGuiTableFlags_BordersInnerV))
				{
					ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("File Reference").x + ImGui::GetStyle().FramePadding.x);

					ImGui::TableNextRow();

					ImGui::TableSetColumnIndex(0);
					ImGui::Text("Name");
					ImGui::TableNextColumn();
					ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
					ImGui::InputText(CreateID("##BlackBoardName").c_str(), &(Entry->Name));
					ImGui::PopItemWidth();
					ImGui::TableNextColumn();

					ImGui::Text("Notes");
					ImGui::TableNextColumn();
					ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
					ImGui::InputText(CreateID("##BlackBoardNotes").c_str(), &(Entry->Notes));
					ImGui::PopItemWidth();
					ImGui::TableNextColumn();

					ImGui::EndTable();
				}
				ImGui::Unindent();
			}

			if(ImGui::CollapsingHeader(CreateID("Data##BlackBoardHeader").c_str(), ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::Indent();
				if (ImGui::BeginTable(CreateID("BlackBoardTableData").c_str(), 2, ImGuiTableFlags_BordersInnerV))
				{
					ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("File Reference").x + ImGui::GetStyle().FramePadding.x);

					ImGui::TableNextRow();

					ImGui::TableSetColumnIndex(0);
					ImGui::Text("Type");
					ImGui::TableNextColumn();
					ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
					if (ImGui::Combo(CreateID("##BlackBoardDataType").c_str(), reinterpret_cast<int*>(&Entry->GlobalValueType), gValueTypeDropdownItems, IM_ARRAYSIZE(gValueTypeDropdownItems))) {
						if (Entry->GlobalValueType == (int)application::file::game::ainb::AINBFile::GlobalType::String)
							Entry->GlobalValue = "None";
						else if (Entry->GlobalValueType == (int)application::file::game::ainb::AINBFile::GlobalType::Int || Entry->GlobalValueType == (int)application::file::game::ainb::AINBFile::GlobalType::UserDefined)
							Entry->GlobalValue = (uint32_t)0;
						else if (Entry->GlobalValueType == (int)application::file::game::ainb::AINBFile::GlobalType::Float)
							Entry->GlobalValue = 0.0f;
						else if (Entry->GlobalValueType == (int)application::file::game::ainb::AINBFile::GlobalType::Bool)
							Entry->GlobalValue = false;
						else if (Entry->GlobalValueType == (int)application::file::game::ainb::AINBFile::GlobalType::Vec3f)
							Entry->GlobalValue = glm::vec3(0, 0, 0);
					}
					ImGui::PopItemWidth();
					ImGui::TableNextColumn();

					ImGui::Text("Value");
					ImGui::TableNextColumn();
					ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
					switch (Entry->GlobalValueType) 
					{
					case (int)application::file::game::ainb::AINBFile::GlobalType::String:
					{
						ImGui::InputText(CreateID("##BlackBoardValue").c_str(), reinterpret_cast<std::string*>(&Entry->GlobalValue));
						break;
					}
					case (int)application::file::game::ainb::AINBFile::GlobalType::Bool: 
					{
						ImGui::Checkbox(CreateID("##BlackBoardValue").c_str(), reinterpret_cast<bool*>(&Entry->GlobalValue));
						break;
					}
					case (int)application::file::game::ainb::AINBFile::GlobalType::UserDefined:
					case (int)application::file::game::ainb::AINBFile::GlobalType::Int: 
					{
						ImGui::InputScalar(CreateID("##BlackBoardValue").c_str(), ImGuiDataType_::ImGuiDataType_U32, reinterpret_cast<uint32_t*>(&Entry->GlobalValue));
						break;
					}
					case (int)application::file::game::ainb::AINBFile::GlobalType::Float: 
					{
						ImGui::InputScalar(CreateID("##BlackBoardValue").c_str(), ImGuiDataType_::ImGuiDataType_Float, reinterpret_cast<float*>(&Entry->GlobalValue));
						break;
					}
					case (int)application::file::game::ainb::AINBFile::GlobalType::Vec3f: 
					{
						ImGui::InputFloat3(CreateID("##BlackBoardValue").c_str(), &(reinterpret_cast<glm::vec3*>(&Entry->GlobalValue)->x));
						break;
					}
					default:
						ImGui::Text("Unknown data type");
						break;
					}
					ImGui::PopItemWidth();
					ImGui::TableNextColumn();

					ImGui::EndTable();
				}
				ImGui::Unindent();
			}

			if(ImGui::CollapsingHeader(CreateID("Advanced##BlackBoardHeader").c_str()))
			{
				ImGui::Indent();

				if (ImGui::BeginTable(CreateID("BlackBoardTableAdvanced").c_str(), 2, ImGuiTableFlags_BordersInnerV))
				{
					ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("File Reference").x + ImGui::GetStyle().FramePadding.x);

					ImGui::TableNextRow();

					ImGui::TableSetColumnIndex(0);
					ImGui::Text("File Reference");
					ImGui::TableNextColumn();

					ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
					ImGui::InputText(CreateID("##BlackBoardFileRef").c_str(), &(Entry->FileReference.FileName));
					ImGui::PopItemWidth();

					ImGui::EndTable();
				}

				ImGui::Unindent();
			}
		}

		if (mAINBPath.empty())
			ImGui::EndDisabled();

		ImGui::End();
	}

	void UIAINBEditor::AutoLayoutGreedyCollisionResolve(std::vector<VisualNode>& AllNodes)
	{
		const int32_t Margin = 20;
		const int32_t MaxIterations = 100;  // Increased from 10
		bool HadCollision = true;
		int32_t Iterations = 0;

		while (HadCollision && Iterations < MaxIterations)
		{
			HadCollision = false;
			Iterations++;

			for (size_t i = 0; i < AllNodes.size(); i++)
			{
				for (size_t j = i + 1; j < AllNodes.size(); j++)
				{
					VisualNode& a = AllNodes[i];
					VisualNode& b = AllNodes[j];

					bool OverlapsX = a.mX < b.mX + b.mWidth + Margin && a.mX + a.mWidth + Margin > b.mX;
					bool OverlapsY = a.mY < b.mY + b.mHeight + Margin && a.mY + a.mHeight + Margin > b.mY;  // FIXED

					if (OverlapsX && OverlapsY)
					{
						HadCollision = true;

						// Move the lower node down
						if (b.mY >= a.mY)
						{
							b.mY = a.mY + a.mHeight + Margin;
						}
						else
						{
							a.mY = b.mY + b.mHeight + Margin;
						}
					}
				}
			}
		}
	}

	int32_t UIAINBEditor::AutoLayoutCalcNodeWidth(VisualNode& Node)
	{
		std::string Name = Node.mPtr->GetName();
		int32_t NameWidth = ImGui::CalcTextSize(Name.c_str()).x + 80;
		return MATH_MAX((int32_t)320, MATH_MIN((int32_t)600, NameWidth));
	}

	void UIAINBEditor::AutoLayoutCalcTreeHeight(VisualNode* n, std::vector<VisualNode*>& Visited, std::vector<VisualNode>& Nodes)
	{
		if (std::find(Visited.begin(), Visited.end(), n) != Visited.end())
			return;

		Visited.push_back(n);

		const int32_t NodeMargin = 100;
		const float PinHeight = (24.0f * ImGui::GetPlatformIO().Monitors[0].DpiScale);

		// Calculate actual node height
		uint32_t h = mNodes[n->mPtr->NodeIndex]->mNodeShapeInfo.mHeaderMax.y + ed::GetStyle().NodePadding.y * 2.0f;

		for (uint32_t Type = 0; Type < application::file::game::ainb::AINBFile::ValueTypeCount; Type++)
		{
			h += (PinHeight + ImGui::GetStyle().ItemSpacing.x) * n->mPtr->ImmediateParameters[Type].size();
			h += (PinHeight + ImGui::GetStyle().ItemSpacing.x) * n->mPtr->InputParameters[Type].size();
			h += (PinHeight + ImGui::GetStyle().ItemSpacing.x) * n->mPtr->OutputParameters[Type].size();

			if (!n->mPtr->ImmediateParameters[Type].empty())
			{
				h += ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().ItemSpacing.x;
			}
		}

		for (uint32_t Type = 0; Type < application::file::game::ainb::AINBFile::LinkedNodeTypeCount; Type++)
		{
			h += (PinHeight + ImGui::GetStyle().ItemSpacing.x) * n->mPtr->LinkedNodes[Type].size();
		}

		n->mHeight = h + NodeMargin;

		// Calculate total height needed for all children
		const int32_t NodeSeparation = 80;
		uint32_t childH = 0;
		uint32_t childCount = 0;

		for (uint32_t Type = 0; Type < application::file::game::ainb::AINBFile::LinkedNodeTypeCount; Type++)
		{
			for (auto& Child : n->mPtr->LinkedNodes[Type])
			{
				if (Child.NodeIndex >= Nodes.size())
				{
					application::util::Logger::Warning("UIAINBEditor", "The node %s with index %u links to a another node with index %i, which does not exist", n->mPtr->GetName().c_str(), n->mPtr->NodeIndex, Child.NodeIndex);
					continue;
				}

				childCount++;
				if (std::find(Visited.begin(), Visited.end(), &Nodes[Child.NodeIndex]) == Visited.end())
				{
					AutoLayoutCalcTreeHeight(&Nodes[Child.NodeIndex], Visited, Nodes);
					childH += Nodes[Child.NodeIndex]._mTreeHeight;
				}
			}
		}

		if (childCount > 0)
			childH += (childCount - 1) * NodeSeparation;

		// Calculate total height needed for all data providers
		uint32_t provH = 0;
		uint32_t provCount = 0;

		for (uint32_t Type = 0; Type < application::file::game::ainb::AINBFile::ValueTypeCount; Type++)
		{
			for (application::file::game::ainb::AINBFile::InputEntry& Param : n->mPtr->InputParameters[Type])
			{
				if (Param.NodeIndex > 0 && Param.NodeIndex < Nodes.size())
				{
					provCount++;
					VisualNode* Node = &Nodes[Param.NodeIndex];
					if (std::find(Visited.begin(), Visited.end(), Node) == Visited.end())
					{
						AutoLayoutCalcTreeHeight(Node, Visited, Nodes);
						provH += Node->_mTreeHeight;
					}
				}
				else if (!Param.Sources.empty())
				{
					for (application::file::game::ainb::AINBFile::MultiEntry& Source : Param.Sources)
					{
						provCount++;
						if (Source.NodeIndex > 0 && Source.NodeIndex < Nodes.size())
						{
							VisualNode* Node = &Nodes[Source.NodeIndex];
							if (std::find(Visited.begin(), Visited.end(), Node) == Visited.end())
							{
								AutoLayoutCalcTreeHeight(Node, Visited, Nodes);
								provH += Node->_mTreeHeight;
							}
						}
					}
				}
			}
		}

		if (provCount > 0)
			provH += (provCount - 1) * NodeSeparation;

		// Tree height is the max of: own height, children height, or providers height
		n->_mTreeHeight = MATH_MAX(n->mHeight, MATH_MAX(childH, provH));

		// Now calculate relative Y positions for children (centered around parent)
		int32_t curY = -(childH / 2);

		for (uint32_t Type = 0; Type < application::file::game::ainb::AINBFile::LinkedNodeTypeCount; Type++)
		{
			for (auto& Child : n->mPtr->LinkedNodes[Type])
			{
				if (Child.NodeIndex >= Nodes.size())
				{
					application::util::Logger::Warning("UIAINBEditor", "The node %s with index %u links to a another node with index %i, which does not exist", n->mPtr->GetName().c_str(), n->mPtr->NodeIndex, Child.NodeIndex);
					continue;
				}

				VisualNode* Node = &Nodes[Child.NodeIndex];
				if (std::find(Visited.begin(), Visited.end(), Node) == Visited.end())
				{
					Node->_mRelY = curY + (Node->_mTreeHeight / 2);
					curY += Node->_mTreeHeight + NodeSeparation;
				}
			}
		}

		// Calculate relative Y positions for providers (centered around parent)
		curY = -(provH / 2);

		for (uint32_t Type = 0; Type < application::file::game::ainb::AINBFile::ValueTypeCount; Type++)
		{
			for (application::file::game::ainb::AINBFile::InputEntry& Param : n->mPtr->InputParameters[Type])
			{
				if (Param.NodeIndex > 0 && Param.NodeIndex < Nodes.size())
				{
					VisualNode* Node = &Nodes[Param.NodeIndex];
					// FIXED: Only set _mRelY if not already visited
					if (std::find(Visited.begin(), Visited.end(), Node) == Visited.end())
					{
						Node->_mRelY = curY + (Node->_mTreeHeight / 2);
						curY += Node->_mTreeHeight + NodeSeparation;
					}
				}
				else if (!Param.Sources.empty())
				{
					for (application::file::game::ainb::AINBFile::MultiEntry& Source : Param.Sources)
					{
						if (Source.NodeIndex > 0 && Source.NodeIndex < Nodes.size())
						{
							VisualNode* Node = &Nodes[Source.NodeIndex];
							// FIXED: Only set _mRelY if not already visited
							if (std::find(Visited.begin(), Visited.end(), Node) == Visited.end())
							{
								Node->_mRelY = curY + (Node->_mTreeHeight / 2);
								curY += Node->_mTreeHeight + NodeSeparation;
							}
						}
					}
				}
			}
		}
	}

	bool UIAINBEditor::AutoLayoutCheckCollision(int32_t x, int32_t y, int32_t w, int32_t h, std::vector<glm::i32vec4>& Placed)
	{
		constexpr int32_t Margin = 20;
		for (const glm::i32vec4& Other : Placed)
		{
			const bool OverlapsX = x < Other.x + Other.z + Margin && x + w + Margin > Other.x;
			const bool OverlapsY = y < Other.y + Other.w + Margin && y + h + Margin > Other.y;
			if (OverlapsX && OverlapsY)
			{
				return true;
			}
		}
		return false;
	}

	int32_t UIAINBEditor::AutoLayoutResolveCollision(int32_t x, int32_t y, int32_t w, int32_t h, std::vector<glm::i32vec4>& Placed)
	{
		constexpr int32_t StepSize = 50;  // Try smaller steps for finer placement
		constexpr int32_t MaxDistance = 5000;  // Maximum distance to search

		// If no collision at original position, use it
		if (!AutoLayoutCheckCollision(x, y, w, h, Placed))
		{
			return y;
		}

		// Try moving down and up simultaneously, taking whichever works first with smallest offset
		int32_t bestY = y;
		int32_t bestDistance = INT32_MAX;

		for (int32_t offset = StepSize; offset < MaxDistance; offset += StepSize)
		{
			// Try down
			int32_t tryDown = y + offset;
			if (!AutoLayoutCheckCollision(x, tryDown, w, h, Placed))
			{
				if (offset < bestDistance)
				{
					bestDistance = offset;
					bestY = tryDown;
					break;  // Found a spot moving down
				}
			}

			// Try up
			int32_t tryUp = y - offset;
			if (!AutoLayoutCheckCollision(x, tryUp, w, h, Placed))
			{
				if (offset < bestDistance)
				{
					bestDistance = offset;
					bestY = tryUp;
					break;  // Found a spot moving up
				}
			}
		}

		return bestY;
	}

	void UIAINBEditor::AutoLayoutPlaceSubtree(VisualNode* n, int32_t x, int32_t y, std::vector<VisualNode*>& Placed, std::vector<glm::i32vec4>& Bounds, std::vector<VisualNode>& Nodes)
	{
		if (std::find(Placed.begin(), Placed.end(), n) != Placed.end())
			return;

		// Resolve collision for this node
		int32_t finalY = AutoLayoutResolveCollision(x, y, n->mWidth, n->mHeight, Bounds);

		// Mark as placed
		Placed.push_back(n);
		n->mX = x;
		n->mY = finalY;
		Bounds.push_back(glm::vec4(n->mX, n->mY, n->mWidth, n->mHeight));

		const int32_t HorizontalSpacing = 800;

		// Place data providers first (going left)
		for (uint32_t Type = 0; Type < application::file::game::ainb::AINBFile::ValueTypeCount; Type++)
		{
			for (application::file::game::ainb::AINBFile::InputEntry& Param : n->mPtr->InputParameters[Type])
			{
				if (Param.NodeIndex > 0 && Param.NodeIndex < Nodes.size())
				{
					VisualNode* Node = &Nodes[Param.NodeIndex];
					if (std::find(Placed.begin(), Placed.end(), Node) == Placed.end())
					{
						AutoLayoutPlaceSubtree(Node, x - HorizontalSpacing, finalY + Node->_mRelY, Placed, Bounds, Nodes);
					}
				}
				else if (!Param.Sources.empty())
				{
					for (application::file::game::ainb::AINBFile::MultiEntry& Source : Param.Sources)
					{
						if (Source.NodeIndex > 0 && Source.NodeIndex < Nodes.size())
						{
							VisualNode* Node = &Nodes[Source.NodeIndex];
							if (std::find(Placed.begin(), Placed.end(), Node) == Placed.end())
							{
								AutoLayoutPlaceSubtree(Node, x - HorizontalSpacing, finalY + Node->_mRelY, Placed, Bounds, Nodes);
							}
						}
					}
				}
			}
		}

		// Then place children (going right)
		for (uint32_t Type = 0; Type < application::file::game::ainb::AINBFile::LinkedNodeTypeCount; Type++)
		{
			for (auto& Child : n->mPtr->LinkedNodes[Type])
			{
				if (Child.NodeIndex >= Nodes.size())
				{
					application::util::Logger::Warning("UIAINBEditor", "The node %s with index %u links to a another node with index %i, which does not exist", n->mPtr->GetName().c_str(), n->mPtr->NodeIndex, Child.NodeIndex);
					continue;
				}
				if (std::find(Placed.begin(), Placed.end(), &Nodes[Child.NodeIndex]) == Placed.end())
				{
					AutoLayoutPlaceSubtree(&Nodes[Child.NodeIndex], x + HorizontalSpacing, finalY + Nodes[Child.NodeIndex]._mRelY, Placed, Bounds, Nodes);
				}
			}
		}

		// Also place data consumers that aren't already placed
		for (size_t i = 0; i < Nodes.size(); i++)
		{
			VisualNode* Consumer = &Nodes[i];

			if (std::find(Placed.begin(), Placed.end(), Consumer) != Placed.end())
				continue;

			// Check if this consumer uses our node 'n' as a data provider
			bool ConsumesFromN = false;

			for (uint32_t Type = 0; Type < application::file::game::ainb::AINBFile::ValueTypeCount; Type++)
			{
				for (const auto& Param : Consumer->mPtr->InputParameters[Type])
				{
					if (Param.NodeIndex == n->mPtr->NodeIndex)
					{
						ConsumesFromN = true;
						break;
					}

					if (!Param.Sources.empty())
					{
						for (const auto& Source : Param.Sources)
						{
							if (Source.NodeIndex == n->mPtr->NodeIndex)
							{
								ConsumesFromN = true;
								break;
							}
						}
					}

					if (ConsumesFromN) break;
				}
				if (ConsumesFromN) break;
			}

			if (ConsumesFromN)
			{
				AutoLayoutPlaceSubtree(Consumer, x + HorizontalSpacing, finalY, Placed, Bounds, Nodes);
			}
		}
	}

	void UIAINBEditor::AutoLayout()
	{
		if (mAINBFile.Nodes.empty())
			return;

		std::vector<VisualNode> VisualNodes;
		VisualNodes.resize(mAINBFile.Nodes.size());
		for (size_t i = 0; i < mAINBFile.Nodes.size(); i++)
		{
			VisualNodes[i].mPtr = &mAINBFile.Nodes[i];
			VisualNodes[i].mWidth = AutoLayoutCalcNodeWidth(VisualNodes[i]);
		}

		std::vector<CommandGroup> CommandGroups;
		std::set<VisualNode*> AllProcessedNodes;

		for (const auto& Command : mAINBFile.Commands)
		{
			if (Command.LeftNodeIndex < 0 || Command.LeftNodeIndex >= VisualNodes.size())
				continue;

			CommandGroup Group;
			Group.CommandName = Command.Name;
			Group.RootNode = &VisualNodes[Command.LeftNodeIndex];

			std::set<VisualNode*> Visited;
			AutoLayoutCollectCommandNodes(Group.RootNode, Group.Nodes, Visited, VisualNodes);

			for (VisualNode* Node : Group.Nodes)
			{
				AllProcessedNodes.insert(Node);
			}

			CommandGroups.push_back(Group);
		}

		int32_t CurrentGroupY = 100;
		const int32_t GroupVerticalSpacing = 400;

		for (CommandGroup& Group : CommandGroups)
		{
			std::vector<VisualNode*> Visited;
			std::vector<VisualNode*> Placed;
			std::vector<glm::i32vec4> PlacedNodeBounds;

			for (VisualNode* Node : Group.Nodes)
			{
				Node->_mRelY = 0;
			}

			AutoLayoutCalcTreeHeight(Group.RootNode, Visited, VisualNodes);
			AutoLayoutPlaceSubtree(Group.RootNode, 100, CurrentGroupY, Placed, PlacedNodeBounds, VisualNodes);

			if (!PlacedNodeBounds.empty())
			{
				int32_t MinX = INT32_MAX, MinY = INT32_MAX;
				int32_t MaxX = INT32_MIN, MaxY = INT32_MIN;

				for (const auto& Bound : PlacedNodeBounds)
				{
					MinX = MATH_MIN(MinX, Bound.x);
					MinY = MATH_MIN(MinY, Bound.y);
					MaxX = MATH_MAX(MaxX, Bound.x + Bound.z);
					MaxY = MATH_MAX(MaxY, Bound.y + Bound.w);
				}

				Group.Bounds = glm::vec4(MinX, MinY, MaxX - MinX, MaxY - MinY);
				CurrentGroupY = MaxY + GroupVerticalSpacing;
			}
		}

		std::vector<VisualNode*> OrphanNodes;
		for (VisualNode& Node : VisualNodes)
		{
			if (AllProcessedNodes.count(&Node) == 0)
			{
				bool IsOrphan = true;

				for (const VisualNode& Other : VisualNodes)
				{
					for (uint32_t Type = 0; Type < application::file::game::ainb::AINBFile::LinkedNodeTypeCount; Type++)
					{
						for (const auto& Child : Other.mPtr->LinkedNodes[Type])
						{
							if (Child.NodeIndex == Node.mPtr->NodeIndex)
							{
								IsOrphan = false;
								break;
							}
						}
						if (!IsOrphan) break;
					}

					for (uint32_t Type = 0; Type < application::file::game::ainb::AINBFile::ValueTypeCount; Type++)
					{
						for (const auto& Param : Other.mPtr->InputParameters[Type])
						{
							if (Param.NodeIndex == Node.mPtr->NodeIndex)
							{
								IsOrphan = false;
								break;
							}
							if (!Param.Sources.empty())
							{
								for (const auto& Source : Param.Sources)
								{
									if (Source.NodeIndex == Node.mPtr->NodeIndex)
									{
										IsOrphan = false;
										break;
									}
								}
							}
							if (!IsOrphan) break;
						}
						if (!IsOrphan) break;
					}

					if (!IsOrphan) break;
				}

				if (IsOrphan)
				{
					OrphanNodes.push_back(&Node);
				}
			}
		}

		for (VisualNode* Orphan : OrphanNodes)
		{
			std::vector<VisualNode*> Visited;
			std::vector<VisualNode*> Placed;
			std::vector<glm::i32vec4> PlacedNodeBounds;

			Orphan->_mRelY = 0;
			AutoLayoutCalcTreeHeight(Orphan, Visited, VisualNodes);
			AutoLayoutPlaceSubtree(Orphan, 100, CurrentGroupY, Placed, PlacedNodeBounds, VisualNodes);

			CurrentGroupY += Orphan->_mTreeHeight + GroupVerticalSpacing;
		}

		AutoLayoutResolveGroupCollisions(CommandGroups);

		AutoLayoutGreedyCollisionResolve(VisualNodes);

		for (size_t i = 0; i < VisualNodes.size(); i++)
		{
			ed::SetNodePosition(
				mNodes[i]->mNodeId,
				ImVec2(VisualNodes[i].mX, VisualNodes[i].mY)
			);
		}
	}

	// collect all nodes connected to a root node
	void UIAINBEditor::AutoLayoutCollectCommandNodes(VisualNode* Root, std::set<VisualNode*>& OutNodes, std::set<VisualNode*>& Visited, std::vector<VisualNode>& AllNodes)
	{
		if (Visited.count(Root) > 0)
			return;

		Visited.insert(Root);
		OutNodes.insert(Root);

		// Traverse child nodes (execution flow)
		for (uint32_t Type = 0; Type < application::file::game::ainb::AINBFile::LinkedNodeTypeCount; Type++)
		{
			for (const auto& Child : Root->mPtr->LinkedNodes[Type])
			{
				if (Child.NodeIndex >= 0 && Child.NodeIndex < AllNodes.size())
				{
					AutoLayoutCollectCommandNodes(&AllNodes[Child.NodeIndex], OutNodes, Visited, AllNodes);
				}
			}
		}

		// Traverse data providers (input parameters)
		for (uint32_t Type = 0; Type < application::file::game::ainb::AINBFile::ValueTypeCount; Type++)
		{
			for (const auto& Param : Root->mPtr->InputParameters[Type])
			{
				if (Param.NodeIndex >= 0 && Param.NodeIndex < AllNodes.size())
				{
					AutoLayoutCollectCommandNodes(&AllNodes[Param.NodeIndex], OutNodes, Visited, AllNodes);
				}

				if (!Param.Sources.empty())
				{
					for (const auto& Source : Param.Sources)
					{
						if (Source.NodeIndex >= 0 && Source.NodeIndex < AllNodes.size())
						{
							AutoLayoutCollectCommandNodes(&AllNodes[Source.NodeIndex], OutNodes, Visited, AllNodes);
						}
					}
				}
			}
		}

		// Traverse data consumers (nodes that use THIS node's outputs)
		for (size_t i = 0; i < AllNodes.size(); i++)
		{
			VisualNode* PotentialConsumer = &AllNodes[i];

			if (Visited.count(PotentialConsumer) > 0)
				continue;

			bool ConsumesRoot = false;

			// Check input parameters
			for (uint32_t Type = 0; Type < application::file::game::ainb::AINBFile::ValueTypeCount; Type++)
			{
				for (const auto& Param : PotentialConsumer->mPtr->InputParameters[Type])
				{
					if (Param.NodeIndex == Root->mPtr->NodeIndex)
					{
						ConsumesRoot = true;
						break;
					}

					if (!Param.Sources.empty())
					{
						for (const auto& Source : Param.Sources)
						{
							if (Source.NodeIndex == Root->mPtr->NodeIndex)
							{
								ConsumesRoot = true;
								break;
							}
						}
					}

					if (ConsumesRoot) break;
				}
				if (ConsumesRoot) break;
			}

			// Check linked nodes (if this node has Root as a child)
			if (!ConsumesRoot)
			{
				for (uint32_t Type = 0; Type < application::file::game::ainb::AINBFile::LinkedNodeTypeCount; Type++)
				{
					for (const auto& Child : PotentialConsumer->mPtr->LinkedNodes[Type])
					{
						if (Child.NodeIndex == Root->mPtr->NodeIndex)
						{
							ConsumesRoot = true;
							break;
						}
					}
					if (ConsumesRoot) break;
				}
			}

			if (ConsumesRoot)
			{
				AutoLayoutCollectCommandNodes(PotentialConsumer, OutNodes, Visited, AllNodes);
			}
		}
	}

	// resolve collisions between command groups
	void UIAINBEditor::AutoLayoutResolveGroupCollisions(std::vector<CommandGroup>& Groups)
	{
		const int32_t GroupMargin = 450;

		for (size_t i = 0; i < Groups.size(); i++)
		{
			for (size_t j = i + 1; j < Groups.size(); j++)
			{
				const auto& A = Groups[i].Bounds;
				const auto& B = Groups[j].Bounds;

				bool OverlapsX = A.x < B.x + B.z + GroupMargin && A.x + A.z + GroupMargin > B.x;
				bool OverlapsY = A.y < B.y + B.w + GroupMargin && A.y + A.w + GroupMargin > B.y;

				if (OverlapsX && OverlapsY)
				{
					int32_t MoveDownBy = (A.y + A.w + GroupMargin) - B.y;

					for (VisualNode* Node : Groups[j].Nodes)
					{
						Node->mY += MoveDownBy;
					}

					Groups[j].Bounds.y += MoveDownBy;
				}
			}
		}
	}


	void UIAINBEditor::DrawImpl()
	{
		if(mFirstFrame)
            ImGui::SetNextWindowDockID(application::manager::UIMgr::gDockMain);

		bool Focused = ImGui::Begin(GetWindowTitle().c_str(), &mOpen, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar);

		if (!Focused && !mFirstFrame)
		{
			ImGui::End();
			return;
		}

		mWantAutoLayout = false;
		bool WantSave = false;

		if(ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu(CreateID("File").c_str()))
			{
				if (!mEnableOpen)
					ImGui::BeginDisabled();

				if (ImGui::MenuItem(CreateID("Open").c_str()))
				{
					auto Dialog = pfd::open_file("Choose AINB file to open", pfd::path::home(),
						{ "Artificial Intelligence Node Binary (.ainb)", "*.ainb" },
						pfd::opt::none);

					if (!Dialog.result().empty())
					{
						LoadAINB(Dialog.result()[0]);
					}
				}

				if (!mEnableOpen)
					ImGui::EndDisabled();

				if (!mEnableSaveOverwrite || mAINBPath.empty())
					ImGui::BeginDisabled();

				WantSave = ImGui::MenuItem(CreateID("Save (Overwrite)").c_str());

				if (!mEnableSaveOverwrite || mAINBPath.empty())
					ImGui::EndDisabled();

				if (!mEnableSaveInProject || mAINBPath.empty())
					ImGui::BeginDisabled();

				if (ImGui::MenuItem(CreateID("Save in project").c_str()))
				{
					mAINBPath = application::util::FileUtil::GetSaveFilePath(mAINBFile.Header.FileCategory + "/" + mAINBFile.Header.FileName + ".ainb");
					std::filesystem::create_directories(application::util::FileUtil::GetSaveFilePath(mAINBFile.Header.FileCategory));

					WantSave = true;
				}

				if (!mEnableSaveInProject || mAINBPath.empty())
					ImGui::EndDisabled();

				if (!mEnableSaveAs || mAINBPath.empty())
					ImGui::BeginDisabled();

				if (ImGui::MenuItem(CreateID("Save As...").c_str()))
				{
					auto Dialog = pfd::save_file("Choose AINB file path", pfd::path::home(),
						{ "Artificial Intelligence Node Binary (.ainb)", "*.ainb" },
						pfd::opt::none);

					if (!Dialog.result().empty())
					{
						mAINBPath = Dialog.result();
						if (!mAINBPath.ends_with(".ainb"))
						{
							mAINBPath += ".ainb";
						}
						std::filesystem::path Path(mAINBPath);
						mAINBFile.Header.FileName = Path.stem().string();

						WantSave = true;
					}
				}

				if (!mEnableSaveAs || mAINBPath.empty())
					ImGui::EndDisabled();

				ImGui::EndMenu();
			}

			if (mAINBPath.empty())
				ImGui::BeginDisabled();
			mWantAutoLayout = ImGui::Button(CreateID("Auto Layout").c_str());
			if (mAINBPath.empty())
				ImGui::EndDisabled();

			ImGui::EndMenuBar();
		}

		ImGuiID DockspaceId = ImGui::GetID(GetWindowTitle().c_str());
		ImGui::DockSpace(DockspaceId);

		if (mFirstFrame)
		{
			ImGui::DockBuilderRemoveNode(DockspaceId);
			ImGui::DockBuilderAddNode(DockspaceId, ImGuiDockNodeFlags_DockSpace);
			ImGui::DockBuilderSetNodeSize(DockspaceId, ImGui::GetWindowSize());

			ImGuiID DockLeft, DockMiddle, DockRight;
			ImGui::DockBuilderSplitNode(DockspaceId, ImGuiDir_Left, 0.15f, &DockLeft, &DockMiddle);
			ImGui::DockBuilderSplitNode(DockMiddle, ImGuiDir_Right, 0.25f, &DockRight, &DockMiddle);

			ImGuiID DockLeftTop, DockLeftBottom;
			ImGui::DockBuilderSplitNode(DockLeft, ImGuiDir_Up, 0.5f, &DockLeftTop, &DockLeftBottom);

			ImGui::DockBuilderDockWindow(CreateID("General").c_str(), DockLeftBottom);
			ImGui::DockBuilderDockWindow(CreateID("Node List").c_str(), DockLeftTop);

			ImGui::DockBuilderDockWindow(CreateID("Graph").c_str(), DockMiddle);
			ImGui::DockBuilderDockWindow(CreateID("Details").c_str(), DockRight);

			ImGui::DockBuilderFinish(DockspaceId);

			application::manager::AINBNodeMgr::CalculateMaxDisplayLength();
		}

		ImGui::End();

		DrawNodeListWindow();
		DrawGeneralWindow();
		DrawDetailsWindow();
		DrawGraphWindow();

		for(auto& Func : mRenderEndActions)
		{
			Func();
		}
		mRenderEndActions.clear();

		if(WantSave)
		{
			application::util::Logger::Info("UIAINBEditor", "Saving AINB file %s..., save callback overwritten: %s", mAINBFile.Header.FileName.c_str(), mSaveCallback == std::nullopt ? "false" : "true");
			bool Error = false;
			for(auto& Node : mNodes)
			{
				Error |= !Node->FinalizeNode();
			}

			if(!Error)
			{
				if (mSaveCallback == std::nullopt)
				{
					mAINBFile.Write(mAINBPath);
					application::util::Logger::Info("UIAINBEditor", "Successfully saved AINB file %s", mAINBFile.Header.FileName.c_str());
				}
				else
				{
					mSaveCallback.value()(mAINBFile);
				}
			}
			else
			{
				application::util::Logger::Error("UIAINBEditor", "The AINB graph contains issues which break the game. Please fix them and try saving again");
			}
		}

		if (mInitCallback.has_value())
		{
			mInitCallback.value()(this);
			mInitCallback = std::nullopt;
		}
	}

	void UIAINBEditor::DeleteImpl()
	{
		application::util::Logger::Info("UIAINBEditor", "Deleting...");

		ed::DestroyEditor(mNodeEditorContext);
		mNodeEditorContext = nullptr;
	}

	UIWindowBase::WindowType UIAINBEditor::GetWindowType()
	{
		return UIWindowBase::WindowType::EDITOR_AINB;
	}

	std::string UIAINBEditor::GetWindowTitle()
	{
		if (mSameWindowTypeCount > 0)
		{
			return "AINB Editor (" + std::to_string(mSameWindowTypeCount) + ")###" + std::to_string(mWindowId);
		}

		return "AINB Editor###" + std::to_string(mWindowId);
	}
}