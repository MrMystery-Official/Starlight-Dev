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
#include "StarImGui.h"
#include "HashMgr.h"
#include "ActorCollisionCreator.h"
#include "Logger.h"
#include "PopupAddDynamicData.h"
#include "SceneMgr.h"
#include "UIEventEditor.h"

bool UIProperties::Open = true;
bool UIProperties::FirstFrame = true;

extern std::vector<const char*> UIProperties::mFluidShapes =
{
	"Box",
	"Cylinder"
};

extern std::vector<const char*> UIProperties::mFluidTypes =
{
	"Water",
	"Lava",
	"Driftsand"
};

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
			StarImGui::InputScalarAction("##Actor Hash", ImGuiDataType_::ImGuiDataType_U64, &UIOutliner::SelectedActor->Hash);
			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::Text("Actor SRTHash");
			ImGui::NextColumn();
			ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
			StarImGui::InputScalarAction("##Actor SRTHash", ImGuiDataType_::ImGuiDataType_U32, &UIOutliner::SelectedActor->SRTHash);
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
			const char* TypeDropdownItems[] = { "Static", "Dynamic", "Merged" };
			ImGui::Combo("##Actor Type", reinterpret_cast<int*>(&UIOutliner::SelectedActor->ActorType), TypeDropdownItems, IM_ARRAYSIZE(TypeDropdownItems));
			ImGui::PopItemWidth();
			ImGui::NextColumn();

			if (UIOutliner::SelectedActor->ActorType == Actor::Type::Merged && UIOutliner::SelectedActor->MergedActorParent != nullptr)
			{
				if (UIOutliner::SelectedActor->MergedActorParent->Dynamic.count("BancPath"))
				{
					if (UIOutliner::SelectedActor->MergedActorParent->Dynamic["BancPath"].Type == BymlFile::Type::StringIndex)
					{
						ImGui::Text("Parent actor");
						ImGui::NextColumn();
						ImGui::TextWrapped(std::get<std::string>(UIOutliner::SelectedActor->MergedActorParent->Dynamic["BancPath"].Data).c_str());
						ImGui::NextColumn();
					}
				}
			}

			ImGui::Separator();
			ImGui::Columns(3);
			ImGui::AlignTextToFramePadding();
			if (ImGui::Button("Duplicate", ImVec2(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize, 0)))
			{
				//UIOutliner::SelectedActor = &ActorMgr::AddActor(*UIOutliner::SelectedActor);

				if (UIOutliner::SelectedActor->MergedActorParent == nullptr)
				{
					Actor NewActor = *UIOutliner::SelectedActor;

					if (!NewActor.Gyml.ends_with("_Far") && !NewActor.Links.empty())
					{
						NewActor.Links.erase(std::remove_if(NewActor.Links.begin(), NewActor.Links.end(), [](const Actor::Link& Link)
							{
								for (Actor& PossibleFarActor : ActorMgr::GetActors())
								{
									if (PossibleFarActor.Hash == Link.Dest)
									{
										return PossibleFarActor.Gyml.ends_with("_Far");
									}
								}
								return false;
							}),
							NewActor.Links.end());
					}

					UIOutliner::SelectedActor = &ActorMgr::AddActor(NewActor);
				}
				else
				{
					Actor ParentActor = *UIOutliner::SelectedActor->MergedActorParent;
					if (ParentActor.Dynamic.empty()) goto DupEnd;

					Actor NewActor = *UIOutliner::SelectedActor;
					HashMgr::Hash Hash = HashMgr::GetHash();
					NewActor.Hash = Hash.Hash;
					NewActor.SRTHash = Hash.SRTHash;
					NewActor.ActorType = Actor::Type::Merged;
					UIOutliner::SelectedActor->MergedActorParent->MergedActorContent.push_back(NewActor);

					for (Actor& PossibleParent : ActorMgr::GetActors())
					{
						if (PossibleParent.Dynamic.empty()) continue;
						if (PossibleParent.Dynamic.count("BancPath"))
						{
							if (PossibleParent.Dynamic["BancPath"].Type == BymlFile::Type::StringIndex && ParentActor.Dynamic["BancPath"].Type == BymlFile::Type::StringIndex)
							{
								if (std::get<std::string>(PossibleParent.Dynamic["BancPath"].Data) == std::get<std::string>(ParentActor.Dynamic["BancPath"].Data))
								{
									UIOutliner::SelectedActor = &PossibleParent.MergedActorContent.back();
								}
							}
						}
					}

					ActorMgr::UpdateModelOrder();
				}
			DupEnd:
				ImGui::End();
				return;
			}
			ImGui::NextColumn();
			if (ImGui::Button("Deselect", ImVec2(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize, 0)))
			{
				UIOutliner::SelectedActor = nullptr;
				ImGui::End();
				return;
			}
			ImGui::NextColumn();
			if (StarImGui::ButtonDelete("Delete", ImVec2(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize, 0)))
			{
				PopupGeneralConfirm::Open("Do you really want to delete the actor?", []()
					{
						ActorMgr::DeleteActor(UIOutliner::SelectedActor->Hash, UIOutliner::SelectedActor->SRTHash);
						UIOutliner::SelectedActor = nullptr;
					});
			}
			if (UIOutliner::SelectedActor == nullptr)
			{
				ImGui::End();
				return;
			}
			ImGui::Columns();
			if (UIOutliner::SelectedActor->MergedActorParent == nullptr)
			{
				ImGui::SetCursorPosX(ImGui::GetStyle().ItemSpacing.x);
				if (ImGui::Button("Add to parent", ImVec2(ImGui::GetColumnWidth() - ImGui::GetStyle().ItemSpacing.x + 2, 0)))
				{
					PopupGeneralInputString::Open("Add to parent", "Merged actor path (e.g. Banc/MainField/Merged/xyz.bcett.byml)", "Merge", [](std::string Path)
						{
							for (Actor& Merged : ActorMgr::GetActors())
							{
								if (Merged.Dynamic.empty()) continue;
								if (Merged.Dynamic.count("BancPath"))
								{
									if (Merged.Dynamic["BancPath"].Type == BymlFile::Type::StringIndex)
									{
										if (std::get<std::string>(Merged.Dynamic["BancPath"].Data) == Path)
										{
											UIOutliner::SelectedActor->ActorType = Actor::Type::Merged;
											Merged.MergedActorContent.push_back(*UIOutliner::SelectedActor);

											Actor CompareActor = *UIOutliner::SelectedActor;

											UIOutliner::SelectedActor = &Merged.MergedActorContent.back();

											ActorMgr::GetActors().erase(
												std::remove_if(ActorMgr::GetActors().begin(), ActorMgr::GetActors().end(), [&](Actor const& Actor) {
													return Actor == CompareActor;
													}),
												ActorMgr::GetActors().end());
											ActorMgr::UpdateModelOrder();
										}
									}
								}
							}
						});
				}
			}
			else
			{
				ImGui::Columns(2);
				if (ImGui::Button("Remove from parent", ImVec2(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize, 0)))
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
				ImGui::NextColumn();
				if (ImGui::Button("Select parent", ImVec2(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize, 0)))
				{
					UIOutliner::SelectedActor = UIOutliner::SelectedActor->MergedActorParent;
					ImGui::Columns();
					ImGui::End();
					return;
				}

				ImGui::Columns();
			}

			/*
			ImGui::SetCursorPosX(ImGui::GetStyle().ItemSpacing.x);
			if (ImGui::Button("Add collision (EXPERIMENTAL)", ImVec2(ImGui::GetWindowSize().x - ImGui::GetStyle().ScrollbarSize, 0)))
			{
				ActorCollisionCreator::AddCollisionActor(*UIOutliner::SelectedActor);
			}
			*/

			if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_DefaultOpen))
			{
				Actor OldActor = *UIOutliner::SelectedActor;

				ImGui::Columns(2, "TransformCameraActor");
				ImGui::AlignTextToFramePadding();
				if (ImGui::Button("Camera to actor", ImVec2(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize, 0)))
				{
					glm::vec3 NewPos = glm::vec3(UIOutliner::SelectedActor->Translate.GetX(), UIOutliner::SelectedActor->Translate.GetY(), UIOutliner::SelectedActor->Translate.GetZ());
					NewPos += 8.0f * -UIMapView::CameraView.Orientation;

					UIMapView::CameraView.Position = NewPos;
				}
				ImGui::NextColumn();
				if (ImGui::Button("Actor to camera", ImVec2(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize, 0)))
				{
					glm::vec3 NewPos = UIMapView::CameraView.Position;
					NewPos += 4.0f * UIMapView::CameraView.Orientation;

					UIOutliner::SelectedActor->Translate.SetX(NewPos.x);
					UIOutliner::SelectedActor->Translate.SetY(NewPos.y);
					UIOutliner::SelectedActor->Translate.SetZ(NewPos.z);
				}
				ImGui::Columns();
				ImGui::Separator();

				ImGui::Indent();
				ImGui::Columns(2, "Transform");
				ImGui::AlignTextToFramePadding();

				if (FirstFrame)
					ImGui::SetColumnWidth(0, ImGui::GetColumnWidth() * 0.5f);

				ImGui::Text("Translate");
				StarImGui::CheckVec3fInputRightClick("TranslatePopUp", UIOutliner::SelectedActor->Translate.GetRawData());
				ImGui::NextColumn();
				ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
				if (StarImGui::InputFloat3Colored("##Translate", UIOutliner::SelectedActor->Translate.GetRawData(), ImVec4(0.26f, 0.06f, 0.07f, 1.0f), ImVec4(0.06f, 0.26f, 0.07f, 1.0f), ImVec4(0.06f, 0.07f, 0.26f, 1.0f)))
				{
					ActorMgr::UpdateMergedActorContent(UIOutliner::SelectedActor, OldActor);
				}
				ImGui::PopItemWidth();
				ImGui::NextColumn();

				ImGui::Text("Rotate");
				StarImGui::CheckVec3fInputRightClick("RotatePopUp", UIOutliner::SelectedActor->Rotate.GetRawData(), true);
				ImGui::NextColumn();
				ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
				if (StarImGui::InputFloat3Colored("##Rotate", UIOutliner::SelectedActor->Rotate.GetRawData(), ImVec4(0.22f, 0.02f, 0.03f, 1.0f), ImVec4(0.02f, 0.22f, 0.03f, 1.0f), ImVec4(0.02f, 0.03f, 0.22f, 1.0f)))
				{
					ActorMgr::UpdateMergedActorContent(UIOutliner::SelectedActor, OldActor);
				}
				ImGui::PopItemWidth();
				ImGui::NextColumn();

				ImGui::Text("Scale");
				StarImGui::CheckVec3fInputRightClick("ScalePopUp", UIOutliner::SelectedActor->Scale.GetRawData());
				ImGui::NextColumn();
				ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
				if (StarImGui::InputFloat3Colored("##Scale", UIOutliner::SelectedActor->Scale.GetRawData(), ImVec4(0.18f, 0.0f, 0.0f, 1.0f), ImVec4(0.0f, 0.18f, 0.0f, 1.0f), ImVec4(0.0f, 0.0f, 0.18f, 1.0f)))
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
				StarImGui::InputScalarAction("##Property MoveRadius", ImGuiDataType_Float, &UIOutliner::SelectedActor->MoveRadius);
				ImGui::PopItemWidth();
				ImGui::NextColumn();

				ImGui::Text("ExtraCreateRadius");
				ImGui::NextColumn();
				ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
				StarImGui::InputScalarAction("##Property ExtraCreateRadius", ImGuiDataType_Float, &UIOutliner::SelectedActor->ExtraCreateRadius);
				ImGui::PopItemWidth();
				ImGui::NextColumn();

				ImGui::Text("TurnActorNearEnemy");
				ImGui::NextColumn();
				ImGui::Checkbox("##Property TurnActorNearEnemy", &UIOutliner::SelectedActor->TurnActorNearEnemy);
				ImGui::NextColumn();

				ImGui::Text("InWater");
				ImGui::NextColumn();
				ImGui::Checkbox("##Property InWater", &UIOutliner::SelectedActor->InWater);
				ImGui::NextColumn();

				ImGui::Text("Version");
				ImGui::NextColumn();
				ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
				StarImGui::InputScalarAction("##Property Version", ImGuiDataType_S32, &UIOutliner::SelectedActor->Version);
				ImGui::PopItemWidth();

				ImGui::Columns();
				ImGui::Unindent();
			}

			if (ImGui::CollapsingHeader("Dynamic", ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_DefaultOpen))
			{
				if (ImGui::Button("Add##Dynamic", ImVec2(ImGui::GetWindowWidth() - ImGui::GetStyle().FramePadding.x, 0)))
				{
					PopupAddDynamicData::Open([](std::string Key, Actor::DynamicData Dynamic)
						{
							UIOutliner::SelectedActor->Dynamic.insert({ Key, Dynamic });
						});
				}

				if (!UIOutliner::SelectedActor->Dynamic.empty())
				{
					ImGui::Separator();
				}

				ImGui::Indent();
				ImGui::Columns(3, "Dynamic");

				if (FirstFrame)
				{
					float BaseWidth = ImGui::GetColumnWidth(0);
					ImGui::SetColumnWidth(1, BaseWidth * 1.6f);
					ImGui::SetColumnWidth(2, BaseWidth * 0.4f);
				}

				ImGui::AlignTextToFramePadding();
				for (int i = 0; i < UIOutliner::SelectedActor->Dynamic.size(); i++)
				{
					auto Iter = UIOutliner::SelectedActor->Dynamic.begin();
					std::advance(Iter, i);

					ImGui::Text(Iter->first.c_str());
					if(Iter->second.Type == BymlFile::Type::Array)
						StarImGui::CheckVec3fInputRightClick("DynamicPopUp" + Iter->first + std::to_string(i), reinterpret_cast<Vector3F*>(&Iter->second.Data)->GetRawData());

					ImGui::NextColumn();
					ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
					//ImGui::InputText(std::string("##Dynamic " + Iter->first + std::to_string(i)).c_str(), &Iter->second);
					switch (Iter->second.Type)
					{
					case BymlFile::Type::StringIndex:
						ImGui::InputText(std::string("##Dynamic " + Iter->first + std::to_string(i)).c_str(), reinterpret_cast<std::string*>(&Iter->second.Data));
						break;
					case BymlFile::Type::Bool:
						ImGui::Checkbox(std::string("##Dynamic " + Iter->first + std::to_string(i)).c_str(), reinterpret_cast<bool*>(&Iter->second.Data));
						break;
					case BymlFile::Type::Int32:
						StarImGui::InputScalarAction(std::string("##Dynamic " + Iter->first + std::to_string(i)).c_str(), ImGuiDataType_S32, reinterpret_cast<int32_t*>(&Iter->second.Data));
						break;
					case BymlFile::Type::Int64:
						StarImGui::InputScalarAction(std::string("##Dynamic " + Iter->first + std::to_string(i)).c_str(), ImGuiDataType_S64, reinterpret_cast<int64_t*>(&Iter->second.Data));
						break;
					case BymlFile::Type::UInt32:
						StarImGui::InputScalarAction(std::string("##Dynamic " + Iter->first + std::to_string(i)).c_str(), ImGuiDataType_U32, reinterpret_cast<uint32_t*>(&Iter->second.Data));
						break;
					case BymlFile::Type::UInt64:
						StarImGui::InputScalarAction(std::string("##Dynamic " + Iter->first + std::to_string(i)).c_str(), ImGuiDataType_U64, reinterpret_cast<uint64_t*>(&Iter->second.Data));
						break;
					case BymlFile::Type::Float:
						StarImGui::InputScalarAction(std::string("##Dynamic " + Iter->first + std::to_string(i)).c_str(), ImGuiDataType_Float, reinterpret_cast<float*>(&Iter->second.Data));
						break;
					case BymlFile::Type::Array:
						StarImGui::InputFloat3Colored(std::string("##Dynamic " + Iter->first + std::to_string(i)).c_str(), reinterpret_cast<Vector3F*>(&Iter->second.Data)->GetRawData(), ImVec4(0.26f, 0.06f, 0.07f, 1.0f), ImVec4(0.06f, 0.26f, 0.07f, 1.0f), ImVec4(0.06f, 0.07f, 0.26f, 1.0f));
						break;
					default:
						Logger::Error("UIProperties", "Invalid dynamic data type");
						break;
					}
					ImGui::PopItemWidth();
					ImGui::NextColumn();
					if (StarImGui::ButtonDelete(std::string("Del##Dynamic " + Iter->first + std::to_string(i)).c_str(), ImVec2(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize, 0)))
					{
						UIOutliner::SelectedActor->Dynamic.erase(Iter);
					}
					if (i != UIOutliner::SelectedActor->Dynamic.size() - 1)
					{
						ImGui::NextColumn();
					}
				}
				ImGui::Columns();
				ImGui::Unindent();
			}

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

				ImGui::NewLine();
				ImGui::Text("Constraint Link");
				ImGui::Separator();
				ImGui::Indent();

				ImGui::Columns(2, "PhiveConstraintLinkColumnsID");
				ImGui::Text("ID");
				ImGui::NextColumn();
				ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
				StarImGui::InputScalarAction("##PhiveConstarintLinkID", ImGuiDataType_U64, reinterpret_cast<uint64_t*>(&UIOutliner::SelectedActor->Phive.ConstraintLink.ID));
				ImGui::PopItemWidth();
				ImGui::NextColumn();

				ImGui::Columns();
				ImGui::Text("Owners");
				if (ImGui::Button("Add##Owner", ImVec2(ImGui::GetWindowWidth() - ImGui::GetStyle().FramePadding.x * 2 - ImGui::GetStyle().IndentSpacing - ImGui::GetStyle().ScrollbarSize, 0)))
				{
					Actor::PhiveData::ConstraintLinkData::OwnerData Owner;
					Owner.Type = "Fixed";
					Owner.ParamData.insert({ "EnableAngularSpringMode", false });
					Owner.ParamData.insert({ "EnableLinearSpringMode", false });
					UIOutliner::SelectedActor->Phive.ConstraintLink.Owners.push_back(Owner);
				}
				ImGui::Separator();
				ImGui::Indent();
				uint32_t OwnerIndex = 0;
				for (Actor::PhiveData::ConstraintLinkData::OwnerData& Owner : UIOutliner::SelectedActor->Phive.ConstraintLink.Owners)
				{
					ImGui::Columns(2, ("PhiveConstraintLinkColumnsType" + std::to_string(OwnerIndex)).c_str());
					ImGui::Text("Type");
					ImGui::NextColumn();
					ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
					ImGui::InputText(std::string("##PhiveConstraintLinkOwnerType" + std::to_string(OwnerIndex)).c_str(), &Owner.Type);
					ImGui::PopItemWidth();
					ImGui::NextColumn();
					ImGui::Columns();

					ImGui::Text("Breakable Data");
					if (ImGui::Button(("Add##BreakableData" + std::to_string(OwnerIndex)).c_str(), ImVec2(ImGui::GetWindowWidth() - ImGui::GetStyle().FramePadding.x * 2 - ImGui::GetStyle().IndentSpacing * 2 - ImGui::GetStyle().ScrollbarSize, 0)))
					{
						PopupGeneralInputPair::Open("Add Breakable Data", [&Owner](std::string A, std::string B)
							{
								Owner.BreakableData.insert({ A, B });
							});
					}
					ImGui::Separator();
					ImGui::Indent();
					for (auto& [Key, Val] : Owner.BreakableData)
					{
						ImGui::Columns(2, ("PhiveConstraintLinkColumnsBreakableData" + std::to_string(OwnerIndex) + Key).c_str());
						ImGui::Text(Key.c_str());
						ImGui::NextColumn();
						ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
						ImGui::InputText(std::string("##PhiveConstraintLinkOwnerBreakableData" + std::to_string(OwnerIndex) + Key).c_str(), &Val);
						ImGui::PopItemWidth();
						ImGui::NextColumn();
						ImGui::Columns();
					}
					ImGui::Unindent();

					ImGui::Text("Alias Data");
					if (ImGui::Button(("Add##AliasData" + std::to_string(OwnerIndex)).c_str(), ImVec2(ImGui::GetWindowWidth() - ImGui::GetStyle().FramePadding.x * 2 - ImGui::GetStyle().IndentSpacing * 2 - ImGui::GetStyle().ScrollbarSize, 0)))
					{
						PopupGeneralInputPair::Open("Add Alias Data", [&Owner](std::string A, std::string B)
							{
								Owner.AliasData.insert({ A, B });
							});
					}
					ImGui::Separator();
					ImGui::Indent();
					for (auto& [Key, Val] : Owner.AliasData)
					{
						ImGui::Columns(2, ("PhiveConstraintLinkColumnsAliasData" + std::to_string(OwnerIndex) + Key).c_str());
						ImGui::Text(Key.c_str());
						ImGui::NextColumn();
						ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
						ImGui::InputText(std::string("##PhiveConstraintLinkOwnerAliasData" + std::to_string(OwnerIndex) + Key).c_str(), &Val);
						ImGui::PopItemWidth();
						ImGui::NextColumn();
						ImGui::Columns();
					}
					ImGui::Unindent();

					ImGui::Text("Cluster Data");
					if (ImGui::Button(("Add##ClusterData" + std::to_string(OwnerIndex)).c_str(), ImVec2(ImGui::GetWindowWidth() - ImGui::GetStyle().FramePadding.x * 2 - ImGui::GetStyle().IndentSpacing * 2 - ImGui::GetStyle().ScrollbarSize, 0)))
					{
						PopupGeneralInputPair::Open("Add Cluster Data", [&Owner](std::string A, std::string B)
							{
								Owner.ClusterData.insert({ A, B });
							});
					}
					ImGui::Separator();
					ImGui::Indent();
					for (auto& [Key, Val] : Owner.ClusterData)
					{
						ImGui::Columns(2, ("PhiveConstraintLinkColumnsClusterData" + std::to_string(OwnerIndex) + Key).c_str());
						ImGui::Text(Key.c_str());
						ImGui::NextColumn();
						ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
						ImGui::InputText(std::string("##PhiveConstraintLinkOwnerClusterData" + std::to_string(OwnerIndex) + Key).c_str(), &Val);
						ImGui::PopItemWidth();
						ImGui::NextColumn();
						ImGui::Columns();
					}
					ImGui::Unindent();

					ImGui::Text("User Data");
					if (ImGui::Button(("Add##UserData" + std::to_string(OwnerIndex)).c_str(), ImVec2(ImGui::GetWindowWidth() - ImGui::GetStyle().FramePadding.x * 2 - ImGui::GetStyle().IndentSpacing * 2 - ImGui::GetStyle().ScrollbarSize, 0)))
					{
						PopupGeneralInputPair::Open("Add User Data", [&Owner](std::string A, std::string B)
							{
								Owner.UserData.insert({ A, B });
							});
					}
					ImGui::Separator();
					ImGui::Indent();
					for (auto& [Key, Val] : Owner.UserData)
					{
						ImGui::Columns(2, ("PhiveConstraintLinkColumnsUserData" + std::to_string(OwnerIndex) + Key).c_str());
						ImGui::Text(Key.c_str());
						ImGui::NextColumn();
						ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
						ImGui::InputText(std::string("##PhiveConstraintLinkOwnerUserData" + std::to_string(OwnerIndex) + Key).c_str(), &Val);
						ImGui::PopItemWidth();
						ImGui::NextColumn();
						ImGui::Columns();
					}
					ImGui::Unindent();

					ImGui::Text("Pose");
					ImGui::Separator();
					ImGui::Indent();
					ImGui::Columns(2, ("PhiveConstraintLinkColumnsPose" + std::to_string(OwnerIndex)).c_str());
					ImGui::Text("Translate");
					ImGui::NextColumn();
					ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
					StarImGui::InputFloat3Colored(std::string("##PhiveConstrainLinkOwnerPosTranslate" + std::to_string(OwnerIndex)).c_str(), Owner.OwnerPose.Translate.GetRawData(), ImVec4(0.26f, 0.06f, 0.07f, 1.0f), ImVec4(0.06f, 0.26f, 0.07f, 1.0f), ImVec4(0.06f, 0.07f, 0.26f, 1.0f), true);
					ImGui::PopItemWidth();
					ImGui::NextColumn();
					ImGui::Text("Rotate");
					ImGui::NextColumn();
					ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
					StarImGui::InputFloat3Colored(std::string("##PhiveConstrainLinkOwnerPosRotate" + std::to_string(OwnerIndex)).c_str(), Owner.OwnerPose.Rotate.GetRawData(), ImVec4(0.26f, 0.06f, 0.07f, 1.0f), ImVec4(0.06f, 0.26f, 0.07f, 1.0f), ImVec4(0.06f, 0.07f, 0.26f, 1.0f), true);
					ImGui::PopItemWidth();
					ImGui::NextColumn();
					ImGui::Columns();
					ImGui::Unindent();

					ImGui::Text("Refer");
					ImGui::Separator();
					ImGui::Indent();

					ImGui::Columns(2, ("PhiveConstraintLinkColumnsRefer" + std::to_string(OwnerIndex)).c_str());
					ImGui::Text("ID");
					ImGui::NextColumn();
					ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
					StarImGui::InputScalarAction(("PhiveConstraintLinkColumnsReferID" + std::to_string(OwnerIndex)).c_str(), ImGuiDataType_U64, reinterpret_cast<uint64_t*>(&Owner.Refer));
					ImGui::PopItemWidth();
					ImGui::NextColumn();
					ImGui::Columns();

					ImGui::Text("Pose");
					ImGui::Indent();
					ImGui::Columns(2, ("PhiveConstraintLinkColumnsReferPose" + std::to_string(OwnerIndex)).c_str());
					ImGui::Text("Translate");
					ImGui::NextColumn();
					ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
					StarImGui::InputFloat3Colored(std::string("##PhiveConstrainLinkOwnerReferPosTranslate" + std::to_string(OwnerIndex)).c_str(), Owner.ReferPose.Translate.GetRawData(), ImVec4(0.26f, 0.06f, 0.07f, 1.0f), ImVec4(0.06f, 0.26f, 0.07f, 1.0f), ImVec4(0.06f, 0.07f, 0.26f, 1.0f), true);
					ImGui::PopItemWidth();
					ImGui::NextColumn();
					ImGui::Text("Rotate");
					ImGui::NextColumn();
					ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
					StarImGui::InputFloat3Colored(std::string("##PhiveConstrainLinkOwnerReferPosRotate" + std::to_string(OwnerIndex)).c_str(), Owner.ReferPose.Rotate.GetRawData(), ImVec4(0.26f, 0.06f, 0.07f, 1.0f), ImVec4(0.06f, 0.26f, 0.07f, 1.0f), ImVec4(0.06f, 0.07f, 0.26f, 1.0f), true);
					ImGui::PopItemWidth();
					ImGui::NextColumn();
					ImGui::Columns();
					ImGui::Unindent();
					ImGui::Unindent();

					ImGui::Text("Pivot Data");
					ImGui::Separator();
					ImGui::Indent();
					ImGui::Columns(2, ("PhiveConstraintLinkColumnsPivotData" + std::to_string(OwnerIndex)).c_str());

					ImGui::Text("Axis");
					ImGui::NextColumn();
					ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
					StarImGui::InputScalarAction(std::string("##PhiveConstrainLinkOwnerPivotDataAxis" + std::to_string(OwnerIndex)).c_str(), ImGuiDataType_S32, &Owner.PivotData.Axis);
					ImGui::PopItemWidth();
					ImGui::NextColumn();

					ImGui::Text("AxisA");
					ImGui::NextColumn();
					ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
					StarImGui::InputScalarAction(std::string("##PhiveConstrainLinkOwnerPivotDataAxisA" + std::to_string(OwnerIndex)).c_str(), ImGuiDataType_S32, &Owner.PivotData.AxisA);
					ImGui::PopItemWidth();
					ImGui::NextColumn();

					ImGui::Text("AxisB");
					ImGui::NextColumn();
					ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
					StarImGui::InputScalarAction(std::string("##PhiveConstrainLinkOwnerPivotDataAxisB" + std::to_string(OwnerIndex)).c_str(), ImGuiDataType_S32, &Owner.PivotData.AxisB);
					ImGui::PopItemWidth();
					ImGui::NextColumn();

					ImGui::Text("Pivot");
					ImGui::NextColumn();
					ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
					StarImGui::InputFloat3Colored(std::string("##PhiveConstrainLinkOwnerPivot" + std::to_string(OwnerIndex)).c_str(), Owner.PivotData.Pivot.GetRawData(), ImVec4(0.26f, 0.06f, 0.07f, 1.0f), ImVec4(0.06f, 0.26f, 0.07f, 1.0f), ImVec4(0.06f, 0.07f, 0.26f, 1.0f), true);
					ImGui::PopItemWidth();
					ImGui::NextColumn();

					ImGui::Text("PivotA");
					ImGui::NextColumn();
					ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
					StarImGui::InputFloat3Colored(std::string("##PhiveConstrainLinkOwnerPivotA" + std::to_string(OwnerIndex)).c_str(), Owner.PivotData.PivotA.GetRawData(), ImVec4(0.26f, 0.06f, 0.07f, 1.0f), ImVec4(0.06f, 0.26f, 0.07f, 1.0f), ImVec4(0.06f, 0.07f, 0.26f, 1.0f), true);
					ImGui::PopItemWidth();
					ImGui::NextColumn();

					ImGui::Text("PivotB");
					ImGui::NextColumn();
					ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
					StarImGui::InputFloat3Colored(std::string("##PhiveConstrainLinkOwnerPivotB" + std::to_string(OwnerIndex)).c_str(), Owner.PivotData.PivotB.GetRawData(), ImVec4(0.26f, 0.06f, 0.07f, 1.0f), ImVec4(0.06f, 0.26f, 0.07f, 1.0f), ImVec4(0.06f, 0.07f, 0.26f, 1.0f), true);
					ImGui::PopItemWidth();
					ImGui::NextColumn();

					ImGui::Columns();
					ImGui::Unindent();

					ImGui::Text("Param Data");
					ImGui::Separator();
					ImGui::Indent();
					ImGui::Columns(2, std::string("##PhiveConstrainLinkColumnsParamData" + std::to_string(OwnerIndex)).c_str());
					uint32_t ParamDataIndex = 0;
					for (auto& [Key, Val] : Owner.ParamData)
					{
						ImGui::Text(Key.c_str());
						ImGui::NextColumn();
						ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
						if (std::holds_alternative<uint32_t>(Val))
						{
							StarImGui::InputScalarAction(("##PhiveConstrainLinkOwnerParamData" + std::to_string(OwnerIndex) + std::to_string(ParamDataIndex)).c_str(), ImGuiDataType_U32, &Val);
						}
						else if (std::holds_alternative<int32_t>(Val))
						{
							StarImGui::InputScalarAction(("##PhiveConstrainLinkOwnerParamData" + std::to_string(OwnerIndex) + std::to_string(ParamDataIndex)).c_str(), ImGuiDataType_S32, &Val);
						}
						else if (std::holds_alternative<int64_t>(Val))
						{
							StarImGui::InputScalarAction(("##PhiveConstrainLinkOwnerParamData" + std::to_string(OwnerIndex) + std::to_string(ParamDataIndex)).c_str(), ImGuiDataType_S64, &Val);
						}
						else if (std::holds_alternative<uint64_t>(Val))
						{
							StarImGui::InputScalarAction(("##PhiveConstrainLinkOwnerParamData" + std::to_string(OwnerIndex) + std::to_string(ParamDataIndex)).c_str(), ImGuiDataType_U64, &Val);
						}
						else if (std::holds_alternative<float>(Val))
						{
							StarImGui::InputScalarAction(("##PhiveConstrainLinkOwnerParamData" + std::to_string(OwnerIndex) + std::to_string(ParamDataIndex)).c_str(), ImGuiDataType_Float, &Val);
						}
						else if (std::holds_alternative<double>(Val))
						{
							StarImGui::InputScalarAction(("##PhiveConstrainLinkOwnerParamData" + std::to_string(OwnerIndex) + std::to_string(ParamDataIndex)).c_str(), ImGuiDataType_Double, &Val);
						}
						else if (std::holds_alternative<std::string>(Val))
						{
							ImGui::InputText(("##PhiveConstrainLinkOwnerParamData" + std::to_string(OwnerIndex) + std::to_string(ParamDataIndex)).c_str(), reinterpret_cast<std::string*>(&Val));
						}
						else if (std::holds_alternative<bool>(Val))
						{
							ImGui::Checkbox(("##PhiveConstrainLinkOwnerParamData" + std::to_string(OwnerIndex) + std::to_string(ParamDataIndex)).c_str(), reinterpret_cast<bool*>(&Val));
						}
						else if (std::holds_alternative<BymlFile::Node>(Val))
						{
							Logger::Error("UIProperties", "Unknown phive param data data type: BymlFile::Node in UIOutliner::mSelectedActor->mPhive, got decoded by ActorMgr::BymlToActor(BymlFile::Node&& ActorRootNode)");
						}
						ImGui::PopItemWidth();
						ImGui::NextColumn();
						ParamDataIndex++;
					}
					ImGui::Columns();
					ImGui::Unindent();

					OwnerIndex++;
				}
				ImGui::Unindent();

				ImGui::Text("Refers");
				if (ImGui::Button("Add##Refer", ImVec2(ImGui::GetWindowWidth() - ImGui::GetStyle().FramePadding.x * 2 - ImGui::GetStyle().IndentSpacing - ImGui::GetStyle().ScrollbarSize, 0)))
				{
					Actor::PhiveData::ConstraintLinkData::ReferData Refer;
					Refer.Owner = 0;
					Refer.Type = "Fixed";
					UIOutliner::SelectedActor->Phive.ConstraintLink.Refers.push_back(Refer);
				}
				ImGui::Separator();
				ImGui::Indent();
				uint32_t ReferIndex = 0;
				for (Actor::PhiveData::ConstraintLinkData::ReferData& Refer : UIOutliner::SelectedActor->Phive.ConstraintLink.Refers)
				{
					ImGui::Columns(2, ("##PhiveConstrainLinkRefer" + std::to_string(ReferIndex)).c_str());

					ImGui::Text("Owner");
					ImGui::NextColumn();
					ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
					StarImGui::InputScalarAction(("##PhiveConstrainLinkReferOwner" + std::to_string(ReferIndex)).c_str(), ImGuiDataType_U64, &Refer.Owner);
					ImGui::PopItemWidth();
					ImGui::NextColumn();

					ImGui::Text("Type");
					ImGui::NextColumn();
					ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
					ImGui::InputText(("##PhiveConstrainLinkReferType" + std::to_string(ReferIndex)).c_str(), &Refer.Type);
					ImGui::PopItemWidth();
					ImGui::NextColumn();

					ImGui::Columns();

					if (ReferIndex < (UIOutliner::SelectedActor->Phive.ConstraintLink.Refers.size() - 1))
						ImGui::NewLine();

					ReferIndex++;
				}

				ImGui::Unindent();
				ImGui::Unindent();

				/*
				struct PhiveData
	{
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

		RopeLinkData RopeHeadLink;
		RopeLinkData RopeTailLink;
		std::vector<RailData> Rails;
	};
				*/
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
					StarImGui::InputScalarAction(("##LinkSource" + std::to_string(i)).c_str(), ImGuiDataType_::ImGuiDataType_U64, &Link.Src);
					ImGui::PopItemWidth();
					ImGui::NextColumn();
					ImGui::Text("Destination");
					ImGui::NextColumn();
					ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
					StarImGui::InputScalarAction(("##LinkDestination" + std::to_string(i)).c_str(), ImGuiDataType_::ImGuiDataType_U64, &Link.Dest);
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
					StarImGui::InputScalarAction(("##RailDestination" + std::to_string(i)).c_str(), ImGuiDataType_::ImGuiDataType_U64, &Rail.Dest);
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
			if (ImGui::CollapsingHeader("Static compound##StaticCompoundHeader"))
			{
				ImGui::Indent();
				ImGui::Columns(2, "StaticCompoundColumns");

				ImGui::Text("Bake Fluid");
				ImGui::NextColumn();
				ImGui::Checkbox("##StaticCompoundBakeFluid", &UIOutliner::SelectedActor->mBakeFluid);
				ImGui::NextColumn();

				if (!UIOutliner::SelectedActor->mBakeFluid)
					ImGui::BeginDisabled();

				ImGui::Text("Fluid Shape");
				ImGui::NextColumn();
				ImGui::Combo("##StaticCompoundFluidShape", reinterpret_cast<int*>(&UIOutliner::SelectedActor->mWaterShapeType), mFluidShapes.data(), mFluidShapes.size());
				ImGui::NextColumn();

				ImGui::Text("Fluid Type");
				ImGui::NextColumn();
				ImGui::Combo("##StaticCompoundFluidType", reinterpret_cast<int*>(&UIOutliner::SelectedActor->mWaterFluidType), mFluidTypes.data(), mFluidTypes.size());
				ImGui::NextColumn();

				ImGui::Text("Fluid box height");
				ImGui::NextColumn();
				StarImGui::InputScalarAction("##StaticCompoundBoxHeight", ImGuiDataType_Float, &UIOutliner::SelectedActor->mWaterBoxHeight, NULL, NULL, "%.3f", 0);
				ImGui::NextColumn();

				ImGui::Text("Flow speed (z-axis)");
				ImGui::NextColumn();
				StarImGui::InputScalarAction("##StaticCompoundFlowSpeed", ImGuiDataType_Float, &UIOutliner::SelectedActor->mWaterFlowSpeed, NULL, NULL, "%.3f", 0);
				ImGui::NextColumn();

				if (!UIOutliner::SelectedActor->mBakeFluid)
					ImGui::EndDisabled();

				ImGui::Columns();
				ImGui::Unindent();
			}
			if (ImGui::CollapsingHeader("Flag Reset Conditions##FlagsHeader"))
			{
				ImGui::Indent();
				ImGui::Columns(2, "FlagColumns");

				/*
						uint8_t mExtraByte = 0;
		uint64_t mHash = 0;
		int32_t mSaveFileIndex = 0;

		bool mOnBancChange = false;
		bool mOnNewDay = false;
		bool mOnDefaultOptionRevert = false;
		bool mOnBloodMoon = false;
		bool mOnSaveLoading = false;
		bool mUnknown0 = false;
		bool mUnknown1 = false;
		bool mOnZonaiRespawnDay = false;
		bool mOnMaterialRespawn = false;
		bool mNoResetOnNewGame = false;
				*/

				ImGui::Text("OnBancChange");
				ImGui::NextColumn();
				ImGui::Checkbox("##OnBancChange", &UIOutliner::SelectedActor->mGameData.mOnBancChange);
				ImGui::NextColumn();

				ImGui::Text("OnNewDay");
				ImGui::NextColumn();
				ImGui::Checkbox("##OnNewDay", &UIOutliner::SelectedActor->mGameData.mOnNewDay);
				ImGui::NextColumn();

				ImGui::Text("OnDefaultOptionRevert");
				ImGui::NextColumn();
				ImGui::Checkbox("##OnDefaultOptionRevert", &UIOutliner::SelectedActor->mGameData.mOnDefaultOptionRevert);
				ImGui::NextColumn();

				ImGui::Text("OnBloodMoon");
				ImGui::NextColumn();
				ImGui::Checkbox("##OnBloodMoon", &UIOutliner::SelectedActor->mGameData.mOnBloodMoon);
				ImGui::NextColumn();

				ImGui::Text("OnNewGame");
				ImGui::NextColumn();
				ImGui::Checkbox("##OnNewGame", &UIOutliner::SelectedActor->mGameData.mOnNewGame);
				ImGui::NextColumn();

				ImGui::Text("Unknown0");
				ImGui::NextColumn();
				ImGui::Checkbox("##Unknown0", &UIOutliner::SelectedActor->mGameData.mUnknown0);
				ImGui::NextColumn();

				ImGui::Text("Unknown1");
				ImGui::NextColumn();
				ImGui::Checkbox("##Unknown1", &UIOutliner::SelectedActor->mGameData.mUnknown1);
				ImGui::NextColumn();

				ImGui::Text("OnZonaiRespawnDay");
				ImGui::NextColumn();
				ImGui::Checkbox("##OnZonaiRespawnDay", &UIOutliner::SelectedActor->mGameData.mOnZonaiRespawnDay);
				ImGui::NextColumn();

				ImGui::Text("OnMaterialRespawn");
				ImGui::NextColumn();
				ImGui::Checkbox("##OnMaterialRespawn", &UIOutliner::SelectedActor->mGameData.mOnMaterialRespawn);
				ImGui::NextColumn();

				ImGui::Text("NoResetOnNewGame");
				ImGui::NextColumn();
				ImGui::Checkbox("##NoResetOnNewGame", &UIOutliner::SelectedActor->mGameData.mNoResetOnNewGame);
				ImGui::NextColumn();

				ImGui::Separator();
				ImGui::Text("SaveFileIndex");
				ImGui::NextColumn();
				StarImGui::InputScalarAction("##SaveFileIndex", ImGuiDataType_S32, &UIOutliner::SelectedActor->mGameData.mSaveFileIndex, NULL, NULL, NULL, 0);
				ImGui::NextColumn();

				if (!UIOutliner::SelectedActor->mGameData.mOnMaterialRespawn)
					ImGui::BeginDisabled();

				ImGui::Text("ExtraByte");
				ImGui::NextColumn();
				StarImGui::InputScalarAction("##ExtraByte", ImGuiDataType_U8, &UIOutliner::SelectedActor->mGameData.mExtraByte, NULL, NULL, NULL, 0);
				ImGui::NextColumn();

				if (!UIOutliner::SelectedActor->mGameData.mOnMaterialRespawn)
					ImGui::EndDisabled();

				ImGui::Columns();
				ImGui::Unindent();
			}
			if (UIOutliner::SelectedActor->Gyml.find("Far") == std::string::npos)
			{
				if (ImGui::CollapsingHeader("Far##FarHeader", ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_DefaultOpen))
				{
					bool HasFarActor = false;
					Actor* LinkedFarActor = nullptr;
					Actor::Link* ActorLink = nullptr;
					for (Actor::Link& Link : UIOutliner::SelectedActor->Links)
					{
						if (Link.Gyml != "Reference" || Link.Name != "Reference") continue;

						for (Actor& PossibleFarActor : ActorMgr::GetActors())
						{
							if (PossibleFarActor.Hash == Link.Dest)
							{
								HasFarActor = PossibleFarActor.Gyml.find("Far") != std::string::npos;
								if (HasFarActor)
								{
									ActorLink = &Link;
									LinkedFarActor = &PossibleFarActor;
								}
								break;
							}
						}
					}

					bool FoundFarActor = Util::FileExists(Editor::GetRomFSFile("Pack/Actor/" + UIOutliner::SelectedActor->Gyml + "_Far.pack.zs"));
					if (!FoundFarActor || HasFarActor)
						ImGui::BeginDisabled();

					ImGui::SetCursorPosX(ImGui::GetStyle().ItemSpacing.x);
					if (ImGui::Button(FoundFarActor ? ("Add " + UIOutliner::SelectedActor->Gyml + "_Far").c_str() : "Not found##FarActor", ImVec2(ImGui::GetWindowSize().x - ImGui::GetStyle().ScrollbarSize, 0)))
					{
						Actor SelectedActorCopy = *UIOutliner::SelectedActor;

						Actor& FarActor = ActorMgr::AddActor(UIOutliner::SelectedActor->Gyml + "_Far");
						FarActor.Bakeable = true;
						FarActor.Translate = UIOutliner::SelectedActor->Translate;
						FarActor.Rotate = UIOutliner::SelectedActor->Rotate;
						FarActor.Scale = UIOutliner::SelectedActor->Scale;
						FarActor.ActorType = UIOutliner::SelectedActor->ActorType;
						FarActor.Phive.Placement.insert({ "ID", std::to_string(UIOutliner::SelectedActor->Hash) });

						/* Because the actor vector has been changed, the selected actor pointer has to be adjusted */
						for (Actor& PossibleSelectedActor : ActorMgr::GetActors())
						{
							if (PossibleSelectedActor == SelectedActorCopy)
							{
								UIOutliner::SelectedActor = &PossibleSelectedActor;
								break;
							}
						}

						UIOutliner::SelectedActor->Links.push_back(Actor::Link{ UIOutliner::SelectedActor->Hash, FarActor.Hash, "Reference", "Reference" });
					}
					if (!FoundFarActor || HasFarActor)
						ImGui::EndDisabled();

					ImGui::Columns(2, "Far");
					if (HasFarActor)
						ImGui::BeginDisabled();
					if (ImGui::Button("Add far actor", ImVec2(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize, 0)))
					{
						PopupGeneralInputString::Open("Add far actor", "Far actor gyml", "Add", [](std::string FarActorGyml)
							{
								Actor SelectedActorCopy = *UIOutliner::SelectedActor;

								Actor& FarActor = ActorMgr::AddActor(FarActorGyml);
								FarActor.Bakeable = true;
								FarActor.Translate = UIOutliner::SelectedActor->Translate;
								FarActor.Rotate = UIOutliner::SelectedActor->Rotate;
								FarActor.Scale = UIOutliner::SelectedActor->Scale;
								FarActor.ActorType = UIOutliner::SelectedActor->ActorType;
								FarActor.Phive.Placement.insert({ "ID", std::to_string(UIOutliner::SelectedActor->Hash) });

								/* Because the actor vector has been changed, the selected actor pointer has to be adjusted */
								for (Actor& PossibleSelectedActor : ActorMgr::GetActors())
								{
									if (PossibleSelectedActor == SelectedActorCopy)
									{
										UIOutliner::SelectedActor = &PossibleSelectedActor;
										break;
									}
								}

								UIOutliner::SelectedActor->Links.push_back(Actor::Link{ UIOutliner::SelectedActor->Hash, FarActor.Hash, "Reference", "Reference" });
							});
					}
					ImGui::NextColumn();
					if (HasFarActor)
						ImGui::EndDisabled();
					else
						ImGui::BeginDisabled();
					if (StarImGui::ButtonDelete("Delete far actor", ImVec2(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize, 0)))
					{
						Actor SelectedActorCopy = *UIOutliner::SelectedActor;

						HasFarActor = false;
						ActorMgr::DeleteActor(LinkedFarActor->Hash, LinkedFarActor->SRTHash);
						LinkedFarActor = nullptr;

						for (Actor& ChildActor : ActorMgr::GetActors())
						{
							if (ChildActor == SelectedActorCopy)
							{
								UIOutliner::SelectedActor = &ChildActor;
								break;
							}
						}

						UIOutliner::SelectedActor->Links.erase(
							std::remove_if(UIOutliner::SelectedActor->Links.begin(), UIOutliner::SelectedActor->Links.end(),
								[ActorLink](const Actor::Link& Link) { return &Link == ActorLink; }),
							UIOutliner::SelectedActor->Links.end());
					}
					else
					{
						if (!HasFarActor)
							ImGui::EndDisabled();
					}

					ImGui::Columns();

					ImGui::SetCursorPosX(ImGui::GetStyle().ItemSpacing.x);
					if (!HasFarActor)
						ImGui::BeginDisabled();
					if (ImGui::Button("Select far actor", ImVec2(ImGui::GetWindowSize().x - ImGui::GetStyle().ScrollbarSize, 0)))
					{
						UIOutliner::SelectedActor = LinkedFarActor;
						ImGui::End();
						return;
					}
					if (!HasFarActor)
						ImGui::EndDisabled();

					ImGui::Columns(2, "FarLinkInformation");
					ImGui::AlignTextToFramePadding();
					ImGui::Text("Far actor linked");
					ImGui::NextColumn();
					if (HasFarActor)
						ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 1, 0, 1));
					else
						ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0, 0, 1));
					ImGui::Text(HasFarActor ? "Yes" : "No");
					ImGui::PopStyleColor();
					ImGui::Columns();
				}
			}

			FirstFrame = false;
		}
	}
	else if (UIAINBEditor::Focused)
	{
		UIAINBEditor::DrawPropertiesWindowContent();
	}
	else if (UIEventEditor::mFocused)
	{
		UIEventEditor::DrawPropertiesWindowContent();
	}

	ImGui::End();
}