#include "UIActorTool.h"

#include "imgui.h"
#include "imgui_stdlib.h"

#include "PopupGeneralInputString.h"

#include "ZStdFile.h"
#include "Util.h"
#include "Editor.h"
#include "Logger.h"
#include "UIAINBEditor.h"
#include <iostream>

bool UIActorTool::Open = true;
bool UIActorTool::Focused = false;
std::string UIActorTool::ActorPackFilter = "";
UIActorTool::ActorPackStruct UIActorTool::ActorPack;
std::vector<std::string> UIActorTool::ActorList;
std::vector<std::string> UIActorTool::ActorListLowerCase;

void DisplayBymlNode(BymlFile::Node* Node, int& Id)
{
	ImGui::Text((Node->GetKey() + ((Node->GetType() == BymlFile::Type::Dictionary || Node->GetType() == BymlFile::Type::Array) ? ":" : "")).c_str());

	ImGui::NextColumn();

	switch (Node->GetType())
	{
	case BymlFile::Type::Array:
	case BymlFile::Type::Dictionary:
	{
		break;
	}
	case BymlFile::Type::Bool:
	{
		ImGui::Checkbox(("##" + Node->GetKey() + "_" + std::to_string(Id++)).c_str(), reinterpret_cast<bool*>(&Node->m_Value[0]));
		break;
	}
	case BymlFile::Type::Float:
	{
		ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().FramePadding.x);
		ImGui::InputScalar(("##" + Node->GetKey() + "_" + std::to_string(Id++)).c_str(), ImGuiDataType_::ImGuiDataType_Float, Node->m_Value.data());
		ImGui::PopItemWidth();
		break;
	}
	case BymlFile::Type::Double:
	{
		ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().FramePadding.x);
		ImGui::InputScalar(("##" + Node->GetKey() + "_" + std::to_string(Id++)).c_str(), ImGuiDataType_::ImGuiDataType_Double, Node->m_Value.data());
		ImGui::PopItemWidth();
		break;
	}
	case BymlFile::Type::Int32:
	{
		ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().FramePadding.x);
		ImGui::InputScalar(("##" + Node->GetKey() + "_" + std::to_string(Id++)).c_str(), ImGuiDataType_::ImGuiDataType_S32, Node->m_Value.data());
		ImGui::PopItemWidth();
		break;
	}
	case BymlFile::Type::UInt32:
	{
		ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().FramePadding.x);
		ImGui::InputScalar(("##" + Node->GetKey() + "_" + std::to_string(Id++)).c_str(), ImGuiDataType_::ImGuiDataType_U32, Node->m_Value.data());
		ImGui::PopItemWidth();
		break;
	}
	case BymlFile::Type::Int64:
	{
		ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().FramePadding.x);
		ImGui::InputScalar(("##" + Node->GetKey() + "_" + std::to_string(Id++)).c_str(), ImGuiDataType_::ImGuiDataType_S64, Node->m_Value.data());
		ImGui::PopItemWidth();
		break;
	}
	case BymlFile::Type::UInt64:
	{
		ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().FramePadding.x);
		ImGui::InputScalar(("##" + Node->GetKey() + "_" + std::to_string(Id++)).c_str(), ImGuiDataType_::ImGuiDataType_U64, Node->m_Value.data());
		ImGui::PopItemWidth();
		break;
	}
	case BymlFile::Type::StringIndex:
	{
		std::string Data = Node->GetValue<std::string>();
		ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().FramePadding.x);
		ImGui::InputText(("##" + Node->GetKey() + "_" + std::to_string(Id++)).c_str(), &Data);
		ImGui::PopItemWidth();
		Node->SetValue<std::string>(Data);
		break;
	}
	default:
	{
		Logger::Warning("ActorTool", "Unknown node type " + std::to_string((int)Node->GetType()));
		break;
	}
	}

DisplayChilds:

	ImGui::Indent();
	for (BymlFile::Node& Child : Node->GetChildren())
	{
		ImGui::NextColumn();
		DisplayBymlNode(&Child, Id);
	}
	ImGui::Unindent();
}

void ReplaceStringInBymlNodes(BymlFile::Node* Node, std::string Search, std::string Replace) {
	Util::ReplaceString(Node->GetKey(), Search, Replace);
	if (Node->GetType() == BymlFile::Type::StringIndex)
	{
		std::string Value = Node->GetValue<std::string>();
		Util::ReplaceString(Value, Search, Replace);
		Node->SetValue<std::string>(Value);
	}

	for (BymlFile::Node& Child : Node->GetChildren()) {
		ReplaceStringInBymlNodes(&Child, Search, Replace);
	}
}

void UIActorTool::UpdateActorList()
{
	ActorList.clear();
	ActorListLowerCase.clear();
	for (const auto& DirEntry : std::filesystem::directory_iterator(Editor::GetRomFSFile("Pack/Actor", false)))
	{
		if (!DirEntry.is_regular_file()) continue;
		if (!DirEntry.path().filename().string().ends_with(".pack.zs")) continue;

		std::string Name = DirEntry.path().filename().string();
		Name = Name.substr(0, Name.length() - 8);
		ActorList.push_back(Name);

		std::transform(Name.begin(), Name.end(), Name.begin(),
			[](unsigned char c) { return std::tolower(c); });
		ActorListLowerCase.push_back(Name);
	}

	if (Util::FileExists(Editor::GetWorkingDirFile("Save/Pack/Actor")))
	{
		for (const auto& DirEntry : std::filesystem::directory_iterator(Editor::GetWorkingDirFile("Save/Pack/Actor")))
		{
			if (!DirEntry.is_regular_file()) continue;
			if (!DirEntry.path().filename().string().ends_with(".pack.zs")) continue;
			if (std::find(ActorList.begin(), ActorList.end(), DirEntry.path().filename().string()) != ActorList.end()) continue;

			std::string Name = DirEntry.path().filename().string();
			Name = Name.substr(0, Name.length() - 8);
			ActorList.push_back(Name);

			std::transform(Name.begin(), Name.end(), Name.begin(),
				[](unsigned char c) { return std::tolower(c); });
			ActorListLowerCase.push_back(Name);
		}
	}
}

void UIActorTool::DrawActorToolWindow()
{
	if (!Open) return;

	Focused = ImGui::Begin("Actor Tool", &Open);

	if (Focused)
	{
		Focused = true;
		ImGui::PushItemWidth((ImGui::GetWindowWidth() - ImGui::GetStyle().ItemSpacing.x) * 0.75f);
		ImGui::InputTextWithHint("##ActorPackFilter", "Search...", &ActorPackFilter);
		ImGui::PopItemWidth();
		ImGui::SameLine();
		if (ImGui::Button("Load##ActorPackFile", ImVec2((ImGui::GetWindowWidth() - ImGui::GetStyle().ItemSpacing.x * 2 - ImGui::GetStyle().FramePadding.x * 2) * 0.25f, 0)))
		{
			if (Util::FileExists(Editor::GetRomFSFile("Pack/Actor/" + ActorPackFilter + ".pack.zs")))
			{
				ActorPack.DeletedFiles.clear();
				ActorPack.Bymls.clear();
				ActorPack.AINBs.clear();
				ActorPack.Pack = SarcFile(ZStdFile::Decompress(Editor::GetRomFSFile("Pack/Actor/" + ActorPackFilter + ".pack.zs"), ZStdFile::Dictionary::Pack).Data);
				ActorPack.Name = ActorPackFilter;
				ActorPack.OriginalName = ActorPackFilter;
				ActorPack.Replace = false; //false is default value, still set it for reading

				std::sort(ActorPack.Pack.GetEntries().begin(), ActorPack.Pack.GetEntries().end(), [](const SarcFile::Entry& a, const SarcFile::Entry& b)
					{
						return a.Name < b.Name;
					});

				for (SarcFile::Entry& Entry : ActorPack.Pack.GetEntries())
				{
					if ((Entry.Bytes[0] == 'Y' && Entry.Bytes[1] == 'B') || (Entry.Bytes[0] == 'B' && Entry.Bytes[1] == 'Y')) //Is Byml?
					{
						ActorPack.Bymls.insert({ Entry.Name, BymlFile(Entry.Bytes) });
					}
					if (Entry.Bytes[0] == 'A' && Entry.Bytes[1] == 'I' && Entry.Bytes[2] == 'B')
					{
						ActorPack.AINBs.insert({ Entry.Name, Entry.Bytes });
					}
				}

				ActorPack.OriginalBymls = ActorPack.Bymls;

				Logger::Info("ActorTool", "Actor " + ActorPackFilter + " loaded");
			}
			else
			{
				Logger::Error("ActorTool", "No actor named " + ActorPackFilter + " found");
			}
		}

		ImGui::PushItemWidth(ImGui::GetWindowWidth() - ImGui::GetStyle().FramePadding.x);
		if (ImGui::BeginListBox("##ActorPackFiles"))
		{
			std::string ActorPackFilterLower = ActorPackFilter;
			std::transform(ActorPackFilterLower.begin(), ActorPackFilterLower.end(), ActorPackFilterLower.begin(),
				[](unsigned char c) { return std::tolower(c); });

			auto DisplayActorFile = [&ActorPackFilterLower](const std::string& Name, const std::string& LowerName) {
				if (ActorPackFilterLower.length() > 0)
				{
					if (LowerName.find(ActorPackFilterLower) == std::string::npos)
						return;
				}

				if(ImGui::Selectable(Name.c_str()))
				{
					ActorPackFilter = Name;
				}
			};

			for (size_t i = 0; i < ActorList.size(); i++)
			{
				DisplayActorFile(ActorList[i], ActorListLowerCase[i]);
			}

			ImGui::EndListBox();
		}
		ImGui::PopItemWidth();

		if (ActorPack.Pack.Loaded)
		{
			ImGui::Separator();

			ImGui::Columns(2, "NameColumn");
			ImGui::Text("Name");
			ImGui::NextColumn();
			ImGui::PushItemWidth((ImGui::GetColumnWidth() - ImGui::GetStyle().ItemSpacing.x * 2) * 0.75f);
			ImGui::InputText("##ActorNameInput", &ActorPack.Name);
			ImGui::PopItemWidth();
			ImGui::SameLine();
			if (ImGui::Button("Rename##ActorNameRename", ImVec2((ImGui::GetColumnWidth() - ImGui::GetStyle().ItemSpacing.x * 2 - ImGui::GetStyle().FramePadding.x * 2) * 0.25f, 0)))
			{
				if (ActorPack.Name.length() > 1)
				{
					//Rename native files
					for (SarcFile::Entry& Entry : ActorPack.Pack.GetEntries())
					{
						if (ActorPack.Bymls.count(Entry.Name))
						{
							auto NodeHandler = ActorPack.Bymls.extract(Entry.Name);
							Util::ReplaceString(NodeHandler.key(), ActorPack.OriginalName, ActorPack.Name);

							for (BymlFile::Node& Node : NodeHandler.mapped().GetNodes())
							{
								ReplaceStringInBymlNodes(&Node, ActorPack.OriginalName, ActorPack.Name);
							}

							ActorPack.Bymls.insert(std::move(NodeHandler));
						}

						Util::ReplaceString(Entry.Name, ActorPack.OriginalName, ActorPack.Name);
					}
					uint32_t Count = 0;
					//Rename shared files
					for (SarcFile::Entry& Entry : ActorPack.Pack.GetEntries())
					{
						if (Entry.Name.find(ActorPack.Name) == std::string::npos)
						{
							std::string Extension = Entry.Name.substr(Entry.Name.rfind('.') + 1);
							std::string UnprefixedName = ActorPack.Name + "_" + std::to_string(Count) + Entry.Name.substr(Entry.Name.find('.'));
							std::string UnprefixedOldName = Entry.Name.substr(Entry.Name.find_last_of("/") + 1);
							UnprefixedName = UnprefixedName.substr(0, UnprefixedName.find_last_of('.'));
							UnprefixedOldName = UnprefixedOldName.substr(0, UnprefixedOldName.find_last_of('.'));
							std::string NewName = Entry.Name.substr(0, Entry.Name.find_last_of('/') + 1) + UnprefixedName;

							std::size_t CategoryStart = UnprefixedOldName.find("__");

							if (CategoryStart != std::string::npos)
							{
								CategoryStart += 2;
								std::size_t DotStart = UnprefixedOldName.find('.', CategoryStart);
								if (DotStart != std::string::npos)
								{
									std::string Category = UnprefixedOldName.substr(CategoryStart, DotStart - CategoryStart);
									UnprefixedName = ActorPack.Name + "_" + std::to_string(Count) + "__" + Category + Entry.Name.substr(Entry.Name.find('.'));
									UnprefixedName = UnprefixedName.substr(0, UnprefixedName.find_last_of('.'));
									NewName = Entry.Name.substr(0, Entry.Name.find_last_of('/') + 1) + UnprefixedName;
								}
							}
							NewName += "." + Extension;

							if (ActorPack.Bymls.count(Entry.Name))
							{
								auto NodeHandler = ActorPack.Bymls.extract(Entry.Name);
								NodeHandler.key() = NewName;

								for (BymlFile::Node& Node : NodeHandler.mapped().GetNodes())
								{
									ReplaceStringInBymlNodes(&Node, UnprefixedOldName, UnprefixedName);
								}

								ActorPack.Bymls.insert(std::move(NodeHandler));
							}
							//Fix references
							for (SarcFile::Entry& RefEntry : ActorPack.Pack.GetEntries())
							{
								if (ActorPack.Bymls.count(RefEntry.Name))
								{
									auto NodeHandler = ActorPack.Bymls.extract(RefEntry.Name);
									for (BymlFile::Node& Node : NodeHandler.mapped().GetNodes())
									{
										ReplaceStringInBymlNodes(&Node, UnprefixedOldName, UnprefixedName);
									}

									ActorPack.Bymls.insert(std::move(NodeHandler));
								}
							}
							Entry.Name = NewName;
							Count++;
						}
					}
					ActorPack.OriginalName = ActorPack.Name;
				}
			}
			ImGui::Columns();

			ImGui::SeparatorText("BYML");
			int Id = 0;

			for (int i = 0; i < ActorPack.Bymls.size(); i++)
			{
				auto Iter = ActorPack.Bymls.begin();
				std::advance(Iter, i);

				if (ImGui::CollapsingHeader(Iter->first.c_str()))
				{
					ImGui::Indent();

#ifdef ADVANCED_MODE
					bool DeleteByml = ImGui::Button(("Delete##" + Iter->first).c_str());
#endif
					ImGui::Columns(2, Iter->first.c_str());
					ImGui::AlignTextToFramePadding();
					for (BymlFile::Node& Node : Iter->second.GetNodes())
					{
						DisplayBymlNode(&Node, Id);
						ImGui::NextColumn();
					}
					ImGui::Columns();
					ImGui::Unindent();
#ifdef ADVANCED_MODE
					if (DeleteByml)
					{
						ActorPack.DeletedFiles.push_back(Iter->first);
						ActorPack.Bymls.erase(Iter);
					}
#endif
				}
			}
			ImGui::NewLine();

			ImGui::SeparatorText("AINB");
			ImGui::Columns(2, "AINB");

			for (int i = 0; i < ActorPack.AINBs.size(); i++)
			{
				auto Iter = ActorPack.AINBs.begin();
				std::advance(Iter, i);

				if (i > 0)
				{
					ImGui::Separator();
					ImGui::NextColumn();
				}

				ImGui::Text(Iter->first.c_str());
				ImGui::NextColumn();
				if (ImGui::Button(("Open##" + Iter->first).c_str()))
				{
					UIAINBEditor::LoadAINBFile(Iter->second);
					UIAINBEditor::AINB.EditorFileName = Iter->first;
					UIAINBEditor::SaveCallback = []()
						{
							std::cout << UIAINBEditor::AINB.EditorFileName << std::endl;
							ActorPack.AINBs[UIAINBEditor::AINB.EditorFileName] = UIAINBEditor::AINB.ToBinary();
						};
					ImGui::SetWindowFocus("AINB Editor");
				}
				ImGui::SameLine();
#ifdef ADVANCED_MODE
				if (ImGui::Button(("Delete##" + Iter->first).c_str()))
				{
					ActorPack.DeletedFiles.push_back(Iter->first);
					ActorPack.AINBs.erase(Iter);
				}
#endif
			}

			ImGui::Columns();
		}
	}
	ImGui::End();
}

void UIActorTool::Save(std::string Path)
{
	if (!Focused) return;

	if (ActorPack.Pack.Loaded)
	{
		for (SarcFile::Entry& Entry : ActorPack.Pack.GetEntries())
		{
			if (ActorPack.Bymls.count(Entry.Name))
			{
				if (ActorPack.OriginalBymls.count(Entry.Name))
				{
					bool NodesSame = ActorPack.OriginalBymls[Entry.Name].GetNodes().size() == ActorPack.Bymls[Entry.Name].GetNodes().size();
					if (NodesSame)
					{
						for (int i = 0; i < ActorPack.Bymls[Entry.Name].GetNodes().size(); i++)
						{
							if (BymlFile::NodeHasher::GenerateNodeHash(ActorPack.Bymls[Entry.Name].GetNodes()[i]) != BymlFile::NodeHasher::GenerateNodeHash(ActorPack.OriginalBymls[Entry.Name].GetNodes()[i]))
							{
								NodesSame = false;
								break;
							}
						}
					}
					if (NodesSame)
						continue;
				}
				
				Entry.Bytes = ActorPack.Bymls[Entry.Name].ToBinary(BymlFile::TableGeneration::Auto);
			}
			if (ActorPack.AINBs.count(Entry.Name))
			{
				Entry.Bytes = ActorPack.AINBs[Entry.Name];
			}
		}

		for (std::vector<SarcFile::Entry>::iterator Iter = ActorPack.Pack.GetEntries().begin(); Iter != ActorPack.Pack.GetEntries().end(); )
		{
			if (std::find(ActorPack.DeletedFiles.begin(), ActorPack.DeletedFiles.end(), Iter->Name) != ActorPack.DeletedFiles.end())
			{
				Iter = ActorPack.Pack.GetEntries().erase(Iter);
			}
			else
			{
				Iter++;
			}
		}

		Util::CreateDir(Path + "/Pack/Actor");

		ZStdFile::Compress(ActorPack.Pack.ToBinary(), ZStdFile::Dictionary::Pack).WriteToFile(Path + "/Pack/Actor/" + ActorPack.Name + ".pack.zs");
	}
}