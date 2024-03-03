#include "UIActorTool.h"

#include "imgui.h"
#include "imgui_stdlib.h"

#include "PopupGeneralInputString.h"

#include "ZStdFile.h"
#include "Util.h"
#include "Editor.h"
#include "Logger.h"

bool UIActorTool::Open = true;
UIActorTool::ActorPackStruct UIActorTool::ActorPack;

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
		ImGui::InputScalar(("##" + Node->GetKey() + "_" + std::to_string(Id++)).c_str(), ImGuiDataType_::ImGuiDataType_Float, Node->m_Value.data());
		break;
	}
	case BymlFile::Type::Double:
	{
		ImGui::InputScalar(("##" + Node->GetKey() + "_" + std::to_string(Id++)).c_str(), ImGuiDataType_::ImGuiDataType_Double, Node->m_Value.data());
		break;
	}
	case BymlFile::Type::Int32:
	{
		ImGui::InputScalar(("##" + Node->GetKey() + "_" + std::to_string(Id++)).c_str(), ImGuiDataType_::ImGuiDataType_S32, Node->m_Value.data());
		break;
	}
	case BymlFile::Type::UInt32:
	{
		ImGui::InputScalar(("##" + Node->GetKey() + "_" + std::to_string(Id++)).c_str(), ImGuiDataType_::ImGuiDataType_U32, Node->m_Value.data());
		break;
	}
	case BymlFile::Type::Int64:
	{
		ImGui::InputScalar(("##" + Node->GetKey() + "_" + std::to_string(Id++)).c_str(), ImGuiDataType_::ImGuiDataType_S64, Node->m_Value.data());
		break;
	}
	case BymlFile::Type::UInt64:
	{
		ImGui::InputScalar(("##" + Node->GetKey() + "_" + std::to_string(Id++)).c_str(), ImGuiDataType_::ImGuiDataType_U64, Node->m_Value.data());
		break;
	}
	case BymlFile::Type::StringIndex:
	{
		std::string Data = Node->GetValue<std::string>();
		ImGui::InputText(("##" + Node->GetKey() + "_" + std::to_string(Id++)).c_str(), &Data);
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

void UIActorTool::DrawActorToolWindow()
{
	if (!Open) return;

	ImGui::Begin("Actor Tool", &Open);

	if (ImGui::Button("Load actor"))
	{
		PopupGeneralInputString::Open("Load actor", "Actor name", "Load", [](std::string Name)
			{
				if (Util::FileExists(Editor::GetRomFSFile("Pack/Actor/" + Name + ".pack.zs")))
				{
					ActorPack.Bymls.clear();
					ActorPack.Pack = SarcFile(ZStdFile::Decompress(Editor::GetRomFSFile("Pack/Actor/" + Name + ".pack.zs"), ZStdFile::Dictionary::Pack).Data);
					ActorPack.Name = Name;
					ActorPack.OriginalName = Name;
					ActorPack.Replace = false; //false is default value, still set it for reading
					
					for (SarcFile::Entry& Entry : ActorPack.Pack.GetEntries())
					{
						if (Entry.Bytes[0] == 'Y' && Entry.Bytes[1] == 'B') //Is Byml?
						{
							ActorPack.Bymls.insert({ Entry.Name, BymlFile(Entry.Bytes) });
						}
					}
					
					Logger::Info("ActorTool", "Actor " + Name + " loaded");
				}
				else
				{
					Logger::Error("ActorTool", "No actor named " + Name + " found");
				}
			});
	}

	if (ActorPack.Pack.Loaded)
	{
		ImGui::InputText("Name", &ActorPack.Name);
		ImGui::SameLine();
		if (ImGui::Button("Rename"))
		{
			if (ActorPack.Name.length() > 1)
			{
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
				ActorPack.OriginalName = ActorPack.Name;
			}
		}
	}

	int Id = 0;

	for (auto& [Name, File] : ActorPack.Bymls)
	{
		if (ImGui::CollapsingHeader(Name.c_str()))
		{
			ImGui::Indent();
			ImGui::Columns(2);
			ImGui::AlignTextToFramePadding();
			for (BymlFile::Node& Node : File.GetNodes())
			{
				DisplayBymlNode(&Node, Id);
				ImGui::NextColumn();
			}
			ImGui::Columns();
			ImGui::Unindent();
		}
	}

	ImGui::End();
}

void UIActorTool::Save(std::string Path)
{
	if (ActorPack.Pack.Loaded)
	{
		for (SarcFile::Entry& Entry : ActorPack.Pack.GetEntries())
		{
			if (ActorPack.Bymls.count(Entry.Name))
			{
				Entry.Bytes = ActorPack.Bymls[Entry.Name].ToBinary(BymlFile::TableGeneration::Auto);
			}
		}

		Util::CreateDir(Path + "/Pack/Actor");

		ZStdFile::Compress(ActorPack.Pack.ToBinary(), ZStdFile::Dictionary::Pack).WriteToFile(Path + "/Pack/Actor/" + ActorPack.Name + ".pack.zs");
	}
}