#include "UIProperties.h"

#include "imgui.h"
#include "imgui_stdlib.h"
#include "imgui_internal.h"
#include "UIOutliner.h"
#include "ImGuizmo.h"
#include "Editor.h"
#include "UIMapView.h"
#include "Util.h"
#include "TextureMgr.h"
#include "PopupGeneralInputPair.h"
#include "PopupAddRail.h"
#include "PopupAddLink.h"
#include "PopupGeneralConfirm.h"
#include "PopupGeneralInputString.h"
#include "UIAINBEditor.h"

bool UIProperties::Open = true;
bool UIProperties::FirstFrame = true;

void UIProperties::DrawPropertiesWindow()
{
	if (!Open) return;

	ImGui::Begin("Properties", &Open);

	if (UIMapView::Focused)
	{
		if (UIOutliner::SelectedActor != nullptr)
		{
			ImGui::Columns(2, "General");
			ImGui::AlignTextToFramePadding();

			ImGui::Text("Actor Gyml");
			ImGui::NextColumn();
			ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
			if (ImGui::InputText("##Actor Gyml", &UIOutliner::SelectedActor->Gyml))
			{
				if (Util::FileExists(Editor::GetRomFSFile("Pack/Actor/" + UIOutliner::SelectedActor->Gyml + ".pack.zs")))
				{
					ActorMgr::UpdateModel(UIOutliner::SelectedActor);
					ActorMgr::UpdateModelOrder();
				}
			}
			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::Text("Actor Hash");
			ImGui::NextColumn();
			ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
			ImGui::InputScalar("##Actor Hash", ImGuiDataType_::ImGuiDataType_U64, &UIOutliner::SelectedActor->Hash);
			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::Text("Actor SRTHash");
			ImGui::NextColumn();
			ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
			ImGui::InputScalar("##Actor SRTHash", ImGuiDataType_::ImGuiDataType_U32, &UIOutliner::SelectedActor->SRTHash);
			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::Separator();
			ImGui::Columns(2);
			ImGui::AlignTextToFramePadding();

			ImGui::Text("Name");
			ImGui::NextColumn();
			ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
			ImGui::InputText("##Name", &UIOutliner::SelectedActor->Name);
			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::Text("Type");
			ImGui::NextColumn();
			ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
			const char* TypeDropdownItems[] = { "Static", "Dynamic", "Merged" }; //Merged actor adding is currently unsupported
			ImGui::Combo("##Actor Type", reinterpret_cast<int*>(&UIOutliner::SelectedActor->ActorType), TypeDropdownItems, IM_ARRAYSIZE(TypeDropdownItems));
			ImGui::PopItemWidth();
			ImGui::NextColumn();

			if (UIOutliner::SelectedActor->ActorType == Actor::Type::Merged && UIOutliner::SelectedActor->MergedActorParent != nullptr)
			{
				if (UIOutliner::SelectedActor->MergedActorParent->Dynamic.DynamicString.count("BancPath"))
				{
					ImGui::Text("Parent actor");
					ImGui::NextColumn();
					ImGui::TextWrapped(UIOutliner::SelectedActor->MergedActorParent->Dynamic.DynamicString["BancPath"].c_str());
					ImGui::NextColumn();
				}
			}

			ImGui::Separator();
			ImGui::Columns(3);
			ImGui::AlignTextToFramePadding();
			if (ImGui::Button("Duplicate", ImVec2(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize, 0)))
			{
				UIOutliner::SelectedActor = &ActorMgr::AddActor(*UIOutliner::SelectedActor);
				ImGui::End();
				return;
			}
			ImGui::NextColumn();
			if (UIOutliner::SelectedActor->MergedActorParent == nullptr)
			{
				if (ImGui::Button("Add to parent", ImVec2(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize, 0)))
				{
					PopupGeneralInputString::Open("Add to parent", "Merged actor path (e.g. Banc/MainField/Merged/xyz.bcett.byml)", "Merge", [](std::string Path)
						{
							for (Actor& Merged : ActorMgr::GetActors())
							{
								if (Merged.Dynamic.DynamicString.empty()) continue;
								if (Merged.Dynamic.DynamicString.count("BancPath"))
								{
									if (Merged.Dynamic.DynamicString["BancPath"] == Path)
									{
										UIOutliner::SelectedActor->ActorType = Actor::Type::Merged;
										Merged.MergedActorContent.push_back(*UIOutliner::SelectedActor);
										UIOutliner::SelectedActor = &Merged.MergedActorContent.back();

										ActorMgr::GetActors().erase(
											std::remove_if(ActorMgr::GetActors().begin(), ActorMgr::GetActors().end(), [&](Actor const& Actor) {
												return Actor.Gyml == Merged.MergedActorContent.back().Gyml && Actor.Hash == Merged.MergedActorContent.back().Hash && Actor.SRTHash == Merged.MergedActorContent.back().SRTHash;
												}),
											ActorMgr::GetActors().end());
										ActorMgr::UpdateModelOrder();
									}
								}
							}
						});
				}
			}
			else
			{
				if (ImGui::Button("Remove from \nparent", ImVec2(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize, 0)))
				{
					UIOutliner::SelectedActor->ActorType = Actor::Type::Static;
					ActorMgr::GetActors().push_back(*UIOutliner::SelectedActor);
					UIOutliner::SelectedActor->MergedActorParent->MergedActorContent.erase(
						std::remove_if(UIOutliner::SelectedActor->MergedActorParent->MergedActorContent.begin(), UIOutliner::SelectedActor->MergedActorParent->MergedActorContent.end(), [&](Actor const& Actor) {
							return Actor.Gyml == UIOutliner::SelectedActor->Gyml && Actor.Hash == UIOutliner::SelectedActor->Hash && Actor.SRTHash == UIOutliner::SelectedActor->SRTHash;
							}),
						UIOutliner::SelectedActor->MergedActorParent->MergedActorContent.end());
					ActorMgr::GetActors().back().MergedActorParent = nullptr;
					ActorMgr::UpdateModelOrder();
					UIOutliner::SelectedActor = &ActorMgr::GetActors().back();
				}
			}
			ImGui::NextColumn();
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5, 0.14, 0.14, 1));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.91, 0.12, 0.15, 1));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1, 0, 0, 1));
			if (ImGui::Button("Delete", ImVec2(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize, 0)))
			{
				PopupGeneralConfirm::Open("Do you really want to delete the actor?", []()
					{
						ActorMgr::DeleteActor(UIOutliner::SelectedActor->Hash, UIOutliner::SelectedActor->SRTHash);
						UIOutliner::SelectedActor = nullptr;
						ImGui::PopStyleColor(3);
						ImGui::End();
						return;
					});
			}
			ImGui::PopStyleColor(3);
			ImGui::Columns();
			if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_DefaultOpen))
			{
				Actor OldActor = *UIOutliner::SelectedActor;

				ImGui::Columns(2);
				ImGui::AlignTextToFramePadding();
				if (ImGui::Button("Camera to actor", ImVec2(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize, 0)))
				{
					UIMapView::CameraView.Position.x = UIOutliner::SelectedActor->Translate.GetX();
					UIMapView::CameraView.Position.y = UIOutliner::SelectedActor->Translate.GetY();
					UIMapView::CameraView.Position.z = UIOutliner::SelectedActor->Translate.GetZ();
				}
				ImGui::NextColumn();
				if (ImGui::Button("Actor to camera", ImVec2(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize, 0)))
				{
					UIOutliner::SelectedActor->Translate.SetX(UIMapView::CameraView.Position.x);
					UIOutliner::SelectedActor->Translate.SetY(UIMapView::CameraView.Position.y);
					UIOutliner::SelectedActor->Translate.SetZ(UIMapView::CameraView.Position.z);
				}
				ImGui::Columns();
				ImGui::Separator();

				ImGui::Indent();
				ImGui::Columns(2, "Transform");
				ImGui::AlignTextToFramePadding();

				if (FirstFrame)
					ImGui::SetColumnWidth(0, ImGui::GetColumnWidth() * 0.5f);

				ImGui::Text("Translate");
				ImGui::NextColumn();
				ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
				if (ImGui::InputFloat3("##Translate", UIOutliner::SelectedActor->Translate.GetRawData()))
				{
					ActorMgr::UpdateMergedActorContent(UIOutliner::SelectedActor, OldActor);
				}
				ImGui::PopItemWidth();
				ImGui::NextColumn();

				ImGui::Text("Rotate");
				ImGui::NextColumn();
				ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
				if (ImGui::InputFloat3("##Rotate", UIOutliner::SelectedActor->Rotate.GetRawData()))
				{
					ActorMgr::UpdateMergedActorContent(UIOutliner::SelectedActor, OldActor);
				}
				ImGui::PopItemWidth();
				ImGui::NextColumn();

				ImGui::Text("Scale");
				ImGui::NextColumn();
				ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
				if (ImGui::InputFloat3("##Scale", UIOutliner::SelectedActor->Scale.GetRawData()))
				{
					ActorMgr::UpdateMergedActorContent(UIOutliner::SelectedActor, OldActor);
				}
				ImGui::PopItemWidth();

				ImGui::Columns();
				ImGui::Unindent();
			}

			if (ImGui::CollapsingHeader("Properties", ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::Indent();
				ImGui::Columns(2, "Properties");
				ImGui::AlignTextToFramePadding();

				ImGui::Text("Bakeable");
				ImGui::NextColumn();
				ImGui::Checkbox("##Property Bakeable", &UIOutliner::SelectedActor->Bakeable);
				ImGui::NextColumn();

				ImGui::Text("PhysicsStable");
				ImGui::NextColumn();
				ImGui::Checkbox("##Property PhysicsStable", &UIOutliner::SelectedActor->PhysicsStable);
				ImGui::NextColumn();

				ImGui::Text("ForceActive");
				ImGui::NextColumn();
				ImGui::Checkbox("##Property ForceActive", &UIOutliner::SelectedActor->ForceActive);
				ImGui::NextColumn();

				ImGui::Text("MoveRadius");
				ImGui::NextColumn();
				ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
				ImGui::InputFloat("##Property MoveRadius", &UIOutliner::SelectedActor->MoveRadius);
				ImGui::PopItemWidth();
				ImGui::NextColumn();

				ImGui::Text("ExtraCreateRadius");
				ImGui::NextColumn();
				ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
				ImGui::InputFloat("##Property ExtraCreateRadius", &UIOutliner::SelectedActor->ExtraCreateRadius);
				ImGui::PopItemWidth();
				ImGui::NextColumn();

				ImGui::Text("TurnActorNearEnemy");
				ImGui::NextColumn();
				ImGui::Checkbox("##Property TurnActorNearEnemy", &UIOutliner::SelectedActor->TurnActorNearEnemy);
				ImGui::NextColumn();

				ImGui::Text("InWater");
				ImGui::NextColumn();
				ImGui::Checkbox("##Property InWater", &UIOutliner::SelectedActor->InWater);

				ImGui::Columns();
				ImGui::Unindent();
			}

			if (ImGui::CollapsingHeader("Dynamic", ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_DefaultOpen))
			{
				if (ImGui::Button("Add##Dynamic", ImVec2(ImGui::GetWindowWidth() - ImGui::GetStyle().FramePadding.x, 0)))
				{
					PopupGeneralInputPair::Open("Add dynamic data", [](std::string Key, std::string Value)
						{
							UIOutliner::SelectedActor->Dynamic.DynamicString.insert({ Key, Value });
						});
				}

				if (!UIOutliner::SelectedActor->Dynamic.DynamicString.empty())
				{
					ImGui::Separator();
				}

				ImGui::Indent();
				ImGui::Columns(3, "Dynamic");

				ImGui::AlignTextToFramePadding();
				for (int i = 0; i < UIOutliner::SelectedActor->Dynamic.DynamicString.size(); i++)
				{
					auto Iter = UIOutliner::SelectedActor->Dynamic.DynamicString.begin();
					std::advance(Iter, i);

					ImGui::Text(Iter->first.c_str());
					ImGui::NextColumn();
					ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
					ImGui::InputText(std::string("##Dynamic " + Iter->first + std::to_string(i)).c_str(), &Iter->second);
					ImGui::PopItemWidth();
					ImGui::NextColumn();
					if (ImGui::Button(std::string("Del##Dynamic " + Iter->first + std::to_string(i)).c_str(), ImVec2(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize, 0)))
					{
						UIOutliner::SelectedActor->Dynamic.DynamicString.erase(Iter);
					}
					if (i != UIOutliner::SelectedActor->Dynamic.DynamicString.size() - 1)
					{
						ImGui::NextColumn();
					}
				}
				ImGui::Columns();
				ImGui::Unindent();
			}

			/*
				struct PhiveData
		{
			struct ConstraintLinkData
			{
				struct ReferData
				{
					uint64_t Owner;
					std::string Type;
				};

				struct OwnerData
				{
					struct OwnerPoseData
					{
						Vector3F Rotate;
						Vector3F Translate;
					};

					struct PivotDataStruct
					{
						int32_t Axis = 0;
						Vector3F Pivot;
					};

					struct ReferPoseData
					{
						Vector3F Rotate;
						Vector3F Translate;
					};

					std::map<std::string, std::string> BreakableData;
					std::map<std::string, std::string> ClusterData;
					OwnerPoseData OwnerPose;
					std::map<std::string, std::string> ParamData;
					PivotDataStruct PivotData;
					uint64_t Refer;
					ReferPoseData ReferPose;
					std::string Type = "";
					std::map<std::string, std::string> UserData;
				};

				uint64_t ID = 0;
				std::vector<OwnerData> Owners;
				std::vector<ReferData> Refers;
			};

			struct RopeLinkData
			{
				uint64_t ID = 0;
				std::vector<uint64_t> Owners;
				std::vector<uint64_t> Refers;
			};

			struct RailData
			{
				struct Node
				{
					std::string Key;
					Vector3F Value;
				};

				bool IsClosed = false;
				std::vector<Node> Nodes;
				std::string Type = "";
			};

			ConstraintLinkData ConstraintLink;
			RopeLinkData RopeHeadLink;
			RopeLinkData RopeTailLink;
			std::vector<RailData> Rails;
		};
			*/

			if (ImGui::CollapsingHeader("Phive"))
			{
				ImGui::Text("Placement");
				ImGui::Separator();

				if (ImGui::Button("Add##PhivePlacement", ImVec2(ImGui::GetWindowWidth() - ImGui::GetStyle().FramePadding.x, 0)))
				{
					PopupGeneralInputPair::Open("Add placement entry", [](std::string Key, std::string Value)
						{
							UIOutliner::SelectedActor->Phive.Placement.insert({ Key, Value });
						});
				}

				ImGui::Indent();
				ImGui::Columns(3, "PhivePlacement");
				ImGui::AlignTextToFramePadding();
				for (int i = 0; i < UIOutliner::SelectedActor->Phive.Placement.size(); i++)
				{
					auto Iter = UIOutliner::SelectedActor->Phive.Placement.begin();
					std::advance(Iter, i);

					ImGui::Text(Iter->first.c_str());
					ImGui::NextColumn();
					ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
					ImGui::InputText(std::string("##PhivePlacement " + Iter->first + std::to_string(i)).c_str(), &Iter->second);
					ImGui::PopItemWidth();
					ImGui::NextColumn();
					if (ImGui::Button(std::string("Del##PhivePlacement " + Iter->first + std::to_string(i)).c_str(), ImVec2(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize, 0)))
					{
						UIOutliner::SelectedActor->Phive.Placement.erase(Iter);
					}
					if (i != UIOutliner::SelectedActor->Phive.Placement.size() - 1)
					{
						ImGui::NextColumn();
					}
				}
				ImGui::Columns();


				ImGui::Unindent();
			}

			if (ImGui::CollapsingHeader("Links##Links"))
			{
				if (ImGui::Button("Add##Link", ImVec2(ImGui::GetWindowWidth() - ImGui::GetStyle().FramePadding.x, 0)))
				{
					PopupAddLink::Open([](uint64_t Dest, uint64_t Src, std::string Gyml, std::string Name)
						{
							UIOutliner::SelectedActor->Links.push_back({ Src, Dest, Gyml, Name });
						});
				}
				if (!UIOutliner::SelectedActor->Links.empty()) ImGui::Separator();

				ImGui::Indent();
				ImGui::Columns(2, "Links");
				ImGui::AlignTextToFramePadding();

				for (int i = 0; i < UIOutliner::SelectedActor->Links.size(); i++)
				{
					Actor::Link& Link = UIOutliner::SelectedActor->Links[i];
					ImGui::Text("Source");
					ImGui::NextColumn();
					ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
					ImGui::InputScalar(("##LinkSource" + std::to_string(i)).c_str(), ImGuiDataType_::ImGuiDataType_U64, &Link.Src);
					ImGui::PopItemWidth();
					ImGui::NextColumn();
					ImGui::Text("Destination");
					ImGui::NextColumn();
					ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
					ImGui::InputScalar(("##LinkDestination" + std::to_string(i)).c_str(), ImGuiDataType_::ImGuiDataType_U64, &Link.Dest);
					ImGui::PopItemWidth();
					ImGui::NextColumn();
					ImGui::Text("Gyml");
					ImGui::NextColumn();
					ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
					ImGui::InputText(("##LinkGyml" + std::to_string(i)).c_str(), &Link.Gyml);
					ImGui::PopItemWidth();
					ImGui::NextColumn();
					ImGui::Text("Name");
					ImGui::NextColumn();
					ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
					ImGui::InputText(("##LinkName" + std::to_string(i)).c_str(), &Link.Name);
					ImGui::PopItemWidth();
					ImGui::NextColumn();
					if (ImGui::Button(("Delete##LinkDel" + std::to_string(i)).c_str(), ImVec2(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize, 0)))
					{
						UIOutliner::SelectedActor->Links.erase(UIOutliner::SelectedActor->Links.begin() + i);
					}
					ImGui::NextColumn();
					if (i != UIOutliner::SelectedActor->Links.size() - 1)
					{
						ImGui::NextColumn();
						ImGui::Separator();
					}
				}

				ImGui::Columns();
				ImGui::Unindent();
			}

			if (ImGui::CollapsingHeader("Rails##Rails"))
			{
				if (ImGui::Button("Add##Rail", ImVec2(ImGui::GetWindowWidth() - ImGui::GetStyle().FramePadding.x, 0)))
				{
					PopupAddRail::Open([](uint64_t Dest, std::string Gyml, std::string Name)
						{
							UIOutliner::SelectedActor->Rails.push_back({ Dest, Gyml, Name });
						});
				}
				if (!UIOutliner::SelectedActor->Rails.empty()) ImGui::Separator();

				ImGui::Indent();
				ImGui::Columns(2, "Rails");
				ImGui::AlignTextToFramePadding();

				for (int i = 0; i < UIOutliner::SelectedActor->Rails.size(); i++)
				{
					Actor::Rail& Rail = UIOutliner::SelectedActor->Rails[i];
					ImGui::Text("Destination");
					ImGui::NextColumn();
					ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
					ImGui::InputScalar(("##RailDestination" + std::to_string(i)).c_str(), ImGuiDataType_::ImGuiDataType_U64, &Rail.Dest);
					ImGui::PopItemWidth();
					ImGui::NextColumn();
					ImGui::Text("Gyml");
					ImGui::NextColumn();
					ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
					ImGui::InputText(("##RailGyml" + std::to_string(i)).c_str(), &Rail.Gyml);
					ImGui::PopItemWidth();
					ImGui::NextColumn();
					ImGui::Text("Name");
					ImGui::NextColumn();
					ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
					ImGui::InputText(("##RailName" + std::to_string(i)).c_str(), &Rail.Name);
					ImGui::PopItemWidth();
					ImGui::NextColumn();
					if (ImGui::Button(("Delete##RailDel" + std::to_string(i)).c_str(), ImVec2(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize, 0)))
					{
						UIOutliner::SelectedActor->Rails.erase(UIOutliner::SelectedActor->Rails.begin() + i);
					}
					ImGui::NextColumn();
					if (i != UIOutliner::SelectedActor->Rails.size() - 1)
					{
						ImGui::NextColumn();
						ImGui::Separator();
					}
				}

				ImGui::Columns();
				ImGui::Unindent();
			}

			FirstFrame = false;
		}
	}
	else if (UIAINBEditor::Focused)
	{
		UIAINBEditor::DrawPropertiesWindowContent();
	}

	ImGui::End();
}