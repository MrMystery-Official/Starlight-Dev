#include "PopupEditorAINBActorLinks.h"

#include "UIMapView.h"
#include "imgui.h"
#include "imgui_stdlib.h"
#include "Editor.h"
#include "UIAINBEditor.h"

bool PopupEditorAINBActorLinks::IsOpen = false;
bool PopupEditorAINBActorLinks::FirstFrame = true;

void PopupEditorAINBActorLinks::DisplayInputText(std::string Key, BymlFile::Node* Node)
{
	std::string Data = Node->GetValue<std::string>();
	ImGui::InputText(Key.c_str(), &Data);
	Node->SetValue<std::string>(Data);
}

void PopupEditorAINBActorLinks::DisplayBymlAinbActorLinks(BymlFile& File)
{
	if (File.HasChild("AiGroups"))
	{
		for (int i = 0; i < File.GetNode("AiGroups")->GetChildren().size(); i++)
		{
			BymlFile::Node& Node = File.GetNode("AiGroups")->GetChildren()[i];
			std::string LogicMetaKey = Node.HasChild("Logic") ? "Logic" : "Meta";
			if (ImGui::CollapsingHeader(Node.GetChild(LogicMetaKey)->GetValue<std::string>().c_str()))
			{
				if (ImGui::Button(("Open in AINB Editor##" + std::to_string(i)).c_str()))
				{
					IsOpen = false;
					//Logic/Root/Dungeon001/Dungeon001_6a13.logic.root.ain
					std::string LogicPrefix = "";
					std::string Path = Node.GetChild(LogicMetaKey)->GetValue<std::string>();
					if (Path.starts_with("Logic/"))
					{
						LogicPrefix = "Logic/";
					}
					else if (Path.starts_with("AI/"))
					{
						LogicPrefix = "AI/";
					}
					Path = Path.substr(Path.find_last_of('/') + 1);
					UIAINBEditor::LoadAINBFile(LogicPrefix + Path + "b");
					ImGui::SetWindowFocus("AINB Editor");
				}
				ImGui::Separator();
				ImGui::Indent();
				ImGui::Columns(2, ("AiGroups" + std::to_string(i)).c_str());

				ImGui::Text("Hash");
				ImGui::NextColumn();
				ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
				ImGui::InputScalar(("##Hash" + std::to_string(i)).c_str(), ImGuiDataType_::ImGuiDataType_U64, Node.GetChild("Hash")->m_Value.data());
				ImGui::PopItemWidth();

				ImGui::NextColumn();
				ImGui::Text(LogicMetaKey.c_str());
				ImGui::NextColumn();
				ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
				DisplayInputText(("##Logic" + std::to_string(i)).c_str(), Node.GetChild(LogicMetaKey));
				ImGui::PopItemWidth();

				ImGui::Columns();

				ImGui::Text("References");
				ImGui::SameLine();
				if (ImGui::Button("Add reference"))
				{
					BymlFile::Node NewRefId(BymlFile::Type::StringIndex, "Id");
					NewRefId.SetValue<std::string>("None");
					BymlFile::Node NewPath(BymlFile::Type::StringIndex, "Path");
					NewPath.SetValue<std::string>("None");
					BymlFile::Node NewReference(BymlFile::Type::UInt64, "Reference");
					NewReference.SetValue<uint64_t>(0);
					BymlFile::Node NewInstanceName(BymlFile::Type::StringIndex, "InstanceName");
					NewInstanceName.SetValue<std::string>("None");
					BymlFile::Node NewLogic(BymlFile::Type::StringIndex, "Logic");
					NewLogic.SetValue<std::string>("None");

					BymlFile::Node NewRefParent(BymlFile::Type::Dictionary);
					NewRefParent.AddChild(NewRefId);
					NewRefParent.AddChild(NewInstanceName);
					NewRefParent.AddChild(NewLogic);
					NewRefParent.AddChild(NewPath);
					NewRefParent.AddChild(NewReference);

					Node.GetChild("References")->AddChild(NewRefParent);
				}
				ImGui::Separator();
				ImGui::Indent();
				for (int j = 0; j < Node.GetChild("References")->GetChildren().size(); j++)
				{
					BymlFile::Node& Ref = Node.GetChild("References")->GetChildren()[j];
					ImGui::Columns(3, ("References" + std::to_string(i) + "_" + std::to_string(j)).c_str());

					ImGui::Text("Id");
					ImGui::NextColumn();
					ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
					DisplayInputText(("##ReferencesId" + std::to_string(i) + "_" + std::to_string(j)).c_str(), Ref.GetChild("Id"));
					ImGui::PopItemWidth();
					ImGui::NextColumn();

					if (Ref.HasChild("Path"))
					{
						ImGui::NextColumn();
						ImGui::Text("Path");
						ImGui::NextColumn();
						ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
						DisplayInputText(("##ReferencesPath" + std::to_string(i) + "_" + std::to_string(j)).c_str(), Ref.GetChild("Path"));
						ImGui::PopItemWidth();
						ImGui::NextColumn();
						if (ImGui::Button(("Delete##ReferencePath" + std::to_string(i) + "_" + std::to_string(j)).c_str(), ImVec2(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize, 0)))
						{
							Ref.GetChildren().erase(
								std::remove_if(Ref.GetChildren().begin(), Ref.GetChildren().end(), [&](BymlFile::Node& BymlNode) {
									return BymlNode.GetKey() == "Path";
									}),
								Ref.GetChildren().end());
						}
					}
					if (Ref.HasChild("Reference"))
					{
						ImGui::NextColumn();
						ImGui::Text("Reference");
						ImGui::NextColumn();
						ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
						ImGui::InputScalar(("##ReferencesHash" + std::to_string(i) + "_" + std::to_string(j)).c_str(), ImGuiDataType_::ImGuiDataType_U64, Ref.GetChild("Reference")->m_Value.data());
						ImGui::PopItemWidth();
						ImGui::NextColumn();
						if (ImGui::Button(("Delete##ReferenceHash" + std::to_string(i) + "_" + std::to_string(j)).c_str(), ImVec2(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize, 0)))
						{
							Ref.GetChildren().erase(
								std::remove_if(Ref.GetChildren().begin(), Ref.GetChildren().end(), [&](BymlFile::Node& BymlNode) {
									return BymlNode.GetKey() == "Reference";
									}),
								Ref.GetChildren().end());
						}
					}
					if (Ref.HasChild("InstanceName"))
					{
						ImGui::NextColumn();
						ImGui::Text("InstanceName");
						ImGui::NextColumn();
						ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
						DisplayInputText("##ReferencesInstanceName" + std::to_string(i) + "_" + std::to_string(j), Ref.GetChild("InstanceName"));
						ImGui::PopItemWidth();
						ImGui::NextColumn();
						if (ImGui::Button(("Delete##ReferenceInstanceName" + std::to_string(i) + "_" + std::to_string(j)).c_str(), ImVec2(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize, 0)))
						{
							Ref.GetChildren().erase(
								std::remove_if(Ref.GetChildren().begin(), Ref.GetChildren().end(), [&](BymlFile::Node& BymlNode) {
									return BymlNode.GetKey() == "InstanceName";
									}),
								Ref.GetChildren().end());
						}
					}
					if (Ref.HasChild("Logic"))
					{
						ImGui::NextColumn();
						ImGui::Text("Logic");
						ImGui::NextColumn();
						ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
						DisplayInputText("##ReferencesLogic" + std::to_string(i) + "_" + std::to_string(j), Ref.GetChild("Logic"));
						ImGui::PopItemWidth();
						ImGui::NextColumn();
						if (ImGui::Button(("Delete##ReferenceLogic" + std::to_string(i) + "_" + std::to_string(j)).c_str(), ImVec2(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize, 0)))
						{
							Ref.GetChildren().erase(
								std::remove_if(Ref.GetChildren().begin(), Ref.GetChildren().end(), [&](BymlFile::Node& BymlNode) {
									return BymlNode.GetKey() == "Logic";
									}),
								Ref.GetChildren().end());
						}
					}
					ImGui::Columns();
					if (ImGui::Button(("Delete reference entry##" + std::to_string(i) + "_" + std::to_string(j)).c_str()))
					{
						auto Iter = Node.GetChild("References")->GetChildren().begin();
						std::advance(Iter, j);
						Node.GetChild("References")->GetChildren().erase(Iter);
					}
					ImGui::NewLine();
				}
				ImGui::Unindent();

				ImGui::Unindent();
			}
		}
	}
}

void PopupEditorAINBActorLinks::Render()
{
	if (IsOpen)
	{
		UIMapView::RenderSettings.AllowSelectingActor = false;
		if (FirstFrame)
		{
			ImGui::SetNextWindowSize(ImVec2(1000, 700));
			FirstFrame = false;
		}
		ImGui::OpenPopup("AINB actor links");
		if (ImGui::BeginPopupModal("AINB actor links"))
		{
			DisplayBymlAinbActorLinks(Editor::StaticActorsByml);
			if (ImGui::Button("Add AINB link"))
			{
				if (Editor::StaticActorsByml.HasChild("AiGroups"))
				{
					BymlFile::Node Child(BymlFile::Type::Dictionary);
					BymlFile::Node Hash(BymlFile::Type::UInt64, "Hash");
					Hash.SetValue<uint64_t>(0);
					BymlFile::Node Logic(BymlFile::Type::StringIndex, "Logic");
					Logic.SetValue<std::string>("None");
					BymlFile::Node References(BymlFile::Type::Array, "References");
					Child.AddChild(Hash);
					Child.AddChild(Logic);
					Child.AddChild(References);
					Editor::StaticActorsByml.GetNode("AiGroups")->AddChild(Child);
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Return"))
			{
				IsOpen = false;
			}
		}
		ImGui::SameLine();
		ImGui::Text(std::string("Size: " + std::to_string(ImGui::GetWindowSize().x) + "x" + std::to_string(ImGui::GetWindowSize().y)).c_str());
		ImGui::EndPopup();
	}
}

void PopupEditorAINBActorLinks::Open()
{
	IsOpen = true;
}