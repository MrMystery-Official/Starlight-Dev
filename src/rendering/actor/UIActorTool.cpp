#include "UIActorTool.h"

#include <manager/UIMgr.h>
#include <manager/ActorPackMgr.h>
#include <rendering/popup/PopUpCommon.h>
#include <rendering/ImGuiExt.h>
#include <functional>
#include <util/FileUtil.h>
#include <util/General.h>
#include <file/game/zstd/ZStdBackend.h>
#include <rendering/ainb/UIAINBEditor.h>
#include <manager/ActorPackMgr.h>
#include "imgui.h"
#include "imgui_stdlib.h"
#include "imgui_internal.h"

namespace application::rendering::actor
{
	void UIActorTool::InitializeGlobal()
	{

	}

	void UIActorTool::InitializeImpl()
	{

	}

	bool DisplayBymlNode(application::file::game::byml::BymlFile::Node* Node, int& Id)
	{
		bool Modified = false;

		ImGui::Text((Node->GetKey() + ((Node->GetType() == application::file::game::byml::BymlFile::Type::Dictionary || Node->GetType() == application::file::game::byml::BymlFile::Type::Array) ? ":" : "")).c_str());

		ImGui::NextColumn();

		switch (Node->GetType())
		{
		case application::file::game::byml::BymlFile::Type::Array:
		case application::file::game::byml::BymlFile::Type::Dictionary:
		{
			break;
		}
		case application::file::game::byml::BymlFile::Type::Bool:
		{
			Modified |= ImGui::Checkbox(("##" + Node->GetKey() + "_" + std::to_string(Id++)).c_str(), reinterpret_cast<bool*>(&Node->mValue[0]));
			break;
		}
		case application::file::game::byml::BymlFile::Type::Float:
		{
			ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().FramePadding.x);
			Modified |= ImGui::InputScalar(("##" + Node->GetKey() + "_" + std::to_string(Id++)).c_str(), ImGuiDataType_::ImGuiDataType_Float, Node->mValue.data());
			ImGui::PopItemWidth();
			break;
		}
		case application::file::game::byml::BymlFile::Type::Double:
		{
			ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().FramePadding.x);
			Modified |= ImGui::InputScalar(("##" + Node->GetKey() + "_" + std::to_string(Id++)).c_str(), ImGuiDataType_::ImGuiDataType_Double, Node->mValue.data());
			ImGui::PopItemWidth();
			break;
		}
		case application::file::game::byml::BymlFile::Type::Int32:
		{
			ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().FramePadding.x);
			Modified |= ImGui::InputScalar(("##" + Node->GetKey() + "_" + std::to_string(Id++)).c_str(), ImGuiDataType_::ImGuiDataType_S32, Node->mValue.data());
			ImGui::PopItemWidth();
			break;
		}
		case application::file::game::byml::BymlFile::Type::UInt32:
		{
			ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().FramePadding.x);
			Modified |= ImGui::InputScalar(("##" + Node->GetKey() + "_" + std::to_string(Id++)).c_str(), ImGuiDataType_::ImGuiDataType_U32, Node->mValue.data());
			ImGui::PopItemWidth();
			break;
		}
		case application::file::game::byml::BymlFile::Type::Int64:
		{
			ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().FramePadding.x);
			Modified |= ImGui::InputScalar(("##" + Node->GetKey() + "_" + std::to_string(Id++)).c_str(), ImGuiDataType_::ImGuiDataType_S64, Node->mValue.data());
			ImGui::PopItemWidth();
			break;
		}
		case application::file::game::byml::BymlFile::Type::UInt64:
		{
			ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().FramePadding.x);
			Modified |= ImGui::InputScalar(("##" + Node->GetKey() + "_" + std::to_string(Id++)).c_str(), ImGuiDataType_::ImGuiDataType_U64, Node->mValue.data());
			ImGui::PopItemWidth();
			break;
		}
		case application::file::game::byml::BymlFile::Type::StringIndex:
		{
			std::string Data = Node->GetValue<std::string>();
			ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().FramePadding.x);
			Modified |= ImGui::InputText(("##" + Node->GetKey() + "_" + std::to_string(Id++)).c_str(), &Data);
			ImGui::PopItemWidth();
			Node->SetValue<std::string>(Data);
			break;
		}
		default:
		{
			application::util::Logger::Warning("UIActorTool", "Unknown node type %u", (uint32_t)Node->GetType());
			break;
		}
		}

	DisplayChilds:

		ImGui::Indent();
		for (application::file::game::byml::BymlFile::Node& Child : Node->GetChildren())
		{
			ImGui::NextColumn();
			Modified |= DisplayBymlNode(&Child, Id);
		}
		ImGui::Unindent();

		return Modified;
	}

	void ReplaceStringInBymlNodes(application::file::game::byml::BymlFile::Node* Node, const std::string& Search, const std::string& Replace) 
	{
		application::util::General::ReplaceString(Node->GetKey(), Search, Replace);
		if (Node->GetType() == application::file::game::byml::BymlFile::Type::StringIndex)
		{
			std::string Value = Node->GetValue<std::string>();
			application::util::General::ReplaceString(Value, Search, Replace);
			Node->SetValue<std::string>(Value);
		}

		for (application::file::game::byml::BymlFile::Node& Child : Node->GetChildren()) 
		{
			ReplaceStringInBymlNodes(&Child, Search, Replace);
		}
	}

	void DrawTableProperty(const std::string& DisplayName, int& Id, const std::function<void()>& Func, const std::optional<std::function<void()>>& DelFunction = std::nullopt)
	{
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::AlignTextToFramePadding();
		ImGui::Text(DisplayName.c_str());
		if (DelFunction.has_value())
		{
			const float Height = ImGui::GetCurrentContext()->FontSize + ImGui::GetStyle().FramePadding.y * 2;
			float Size = Height - ImGui::GetStyle().FramePadding.y * 2.0f;

			ImGui::SameLine();
			if (ImGui::ImageButton(("##" + DisplayName + "_DelButton_" + std::to_string(Id++)).c_str(), (ImTextureID)ImGuiExt::gDeleteButtonTexture->mID, ImVec2(Size, Size), ImVec2(1, 1), ImVec2(0, 0)))
			{
				DelFunction.value()();
				return;
			}
		}
		ImGui::TableNextColumn();
		ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
		Func();
		ImGui::PopItemWidth();
	}

	void UIActorTool::DrawImpl()
	{
        if (mFirstFrame)
            ImGui::SetNextWindowDockID(application::manager::UIMgr::gDockMain);

        bool Focused = ImGui::Begin(GetWindowTitle().c_str(), &mOpen, /*ImGuiWindowFlags_NoCollapse |*/ ImGuiWindowFlags_MenuBar );

        if (!Focused)
        {
            ImGui::End();
            return;
        }

        if (ImGui::BeginMenuBar())
        {
            if (ImGui::MenuItem(CreateID("Load").c_str()))
            {
				application::rendering::popup::PopUpCommon::gLoadActorPopUp.Open([this](application::rendering::popup::PopUpBuilder& Builder)
					{
						const std::string& Gyml = *static_cast<std::string*>(Builder.GetDataStorage(0).mPtr);

						mActorGyml = Gyml;
						mActorInfo = *application::manager::ActorInfoMgr::GetActorInfo(Gyml);

						mActor.reset();
						mActor.emplace();
						ActorPackStruct& ActorPack = mActor.value();

						ActorPack.mActorPack.Initialize(application::file::game::ZStdBackend::Decompress(application::util::FileUtil::GetRomFSFilePath("Pack/Actor/" + Gyml + ".pack.zs")));
						ActorPack.mName = Gyml;
						ActorPack.mOriginalName = Gyml;

						std::sort(ActorPack.mActorPack.GetEntries().begin(), ActorPack.mActorPack.GetEntries().end(), [](const application::file::game::SarcFile::Entry& a, const application::file::game::SarcFile::Entry& b)
							{
								return a.mName < b.mName;
							});

						for (application::file::game::SarcFile::Entry& Entry : ActorPack.mActorPack.GetEntries())
						{
							if ((Entry.mBytes[0] == 'Y' && Entry.mBytes[1] == 'B') || (Entry.mBytes[0] == 'B' && Entry.mBytes[1] == 'Y')) //Is Byml?
							{
								ActorPack.mBymls.insert({ Entry.mName, {Entry.mBytes, false} });
							}
							if (Entry.mBytes[0] == 'A' && Entry.mBytes[1] == 'I' && Entry.mBytes[2] == 'B')
							{
								ActorPack.mAINBs.insert({ Entry.mName, {Entry.mBytes, false} });
							}
						}

						//mActorPack = application::manager::ActorPackMgr::GetActorPack(Gyml);
				});
            }

			ImGui::BeginDisabled(!(mActorGyml.has_value() && mActorInfo.has_value()));
			if (ImGui::MenuItem("Save"))
			{
				*application::manager::ActorInfoMgr::GetActorInfo(mActorGyml.value()) = mActorInfo.value();
				application::manager::ActorInfoMgr::Save();

				if (mActor.has_value())
				{
					bool Modified = false;

					for (auto Iter = mActor.value().mActorPack.GetEntries().begin(); Iter != mActor.value().mActorPack.GetEntries().end(); )
					{
						if (mActor.value().mBymls.count(Iter->mName))
						{
							if (mActor.value().mBymls[Iter->mName].second)
							{
								mActor.value().mBymls[Iter->mName].second = false;
								Iter->mBytes = mActor.value().mBymls[Iter->mName].first.ToBinary(application::file::game::byml::BymlFile::TableGeneration::AUTO);
								Modified = true;
							}

							Iter++;
							continue;
						}

						if (mActor.value().mAINBs.count(Iter->mName))
						{
							if (mActor.value().mAINBs[Iter->mName].second)
							{
								mActor.value().mAINBs[Iter->mName].second = false;
								Iter->mBytes = mActor.value().mAINBs[Iter->mName].first;
								Modified = true;
							}

							Iter++;
							continue;
						}

						Iter = mActor.value().mActorPack.GetEntries().erase(Iter);
					}

					for (auto& [Key, Value] : mActor.value().mBymls)
					{
						if (Value.second)
						{
							bool Found = false;
							for(application::file::game::SarcFile::Entry& Entry : mActor.value().mActorPack.GetEntries())
							{
								if (Entry.mName == Key)
								{
									Found = true;
									break;
								}
							}

							if (Found)
								continue;

							application::file::game::SarcFile::Entry NewEntry;
							NewEntry.mName = Key;
							NewEntry.mBytes = Value.first.ToBinary(application::file::game::byml::BymlFile::TableGeneration::AUTO);
							mActor.value().mActorPack.GetEntries().emplace_back(NewEntry);
							Value.second = false;
							Modified = true;
						}
					}

					for (auto& [Key, Value] : mActor.value().mAINBs)
					{
						if (Value.second)
						{
							bool Found = false;
							for (application::file::game::SarcFile::Entry& Entry : mActor.value().mActorPack.GetEntries())
							{
								if (Entry.mName == Key)
								{
									Found = true;
									break;
								}
							}

							if (Found)
								continue;

							application::file::game::SarcFile::Entry NewEntry;
							NewEntry.mName = Key;
							NewEntry.mBytes = Value.first;
							mActor.value().mActorPack.GetEntries().emplace_back(NewEntry);
							Value.second = false;
							Modified = true;
						}
					}

					if (Modified)
					{
						std::filesystem::create_directories(application::util::FileUtil::GetSaveFilePath("Pack/Actor"));
						application::util::FileUtil::WriteFile(application::util::FileUtil::GetSaveFilePath("Pack/Actor/" + mActorGyml.value() + ".pack.zs"), application::file::game::ZStdBackend::Compress(mActor.value().mActorPack.ToBinary(), application::file::game::ZStdBackend::Dictionary::Pack));
						
						application::manager::ActorPackMgr::LoadActorList();

						application::util::Logger::Info("UIActorTool", "Saved Actor Pack %s", mActorGyml.value().c_str());
					}
				}
			}
			ImGui::EndDisabled();

            ImGui::EndMenuBar();
        }

		int PropertyId = 0;

		const float Height = ImGui::GetCurrentContext()->FontSize + ImGui::GetStyle().FramePadding.y * 2;
		float Size = Height - ImGui::GetStyle().FramePadding.y * 2.0f;

		bool OpenAnimationResourcesPopUp = false;
		bool OpenActorInfoPopUp = false;
		bool OpenASInfoAutoPlayMaterialPopUp = false;
		bool OpenASInfoAutoPlaySkeletalPopUp = false;
		if (mActorInfo.has_value())
		{
			ImGui::Text("ActorInfo");
			ImGui::SameLine();
			if (ImGui::ImageButton("##ActorInfo_AddButton", (ImTextureID)ImGuiExt::gAddButtonTexture->mID, ImVec2(Size, Size), ImVec2(0, 0)))
			{
				OpenActorInfoPopUp = true;
			}
			ImGui::Separator();
			if (ImGui::BeginTable("ActorInfoTable", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_BordersInnerH))
			{
				application::manager::ActorInfoMgr::ActorInfoEntry& ActorInfo = mActorInfo.value();

				ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("Clip To Max Terrain Heightssssssssssssssssssssssssssssss").x);

				if (ActorInfo.mActorName.has_value()) DrawTableProperty("Actor Name", PropertyId, [&ActorInfo]() { ImGui::InputText("##ActorNameActorInfo", &ActorInfo.mActorName.value()); }, [&ActorInfo]() { ActorInfo.mActorName = std::nullopt; });

				if (ActorInfo.mAnimationResources.has_value())
				{
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::AlignTextToFramePadding();
					ImGui::Text("Animation Resources");
					ImGui::SameLine();
					if (ImGui::ImageButton("##AnimationResources_DelButton", (ImTextureID)ImGuiExt::gDeleteButtonTexture->mID, ImVec2(Size, Size), ImVec2(1, 1), ImVec2(0, 0)))
					{
						ActorInfo.mAnimationResources = std::nullopt;
						goto AnimationResourcesSkip;
					}
					{
						ImGui::SameLine();
						if (ImGui::ImageButton("##AnimationResources_AddButton", (ImTextureID)ImGuiExt::gAddButtonTexture->mID, ImVec2(Size, Size), ImVec2(0, 0)))
						{
							ActorInfo.mAnimationResources.value().emplace_back(application::manager::ActorInfoMgr::ActorInfoEntry::AnimationResource{
								.mModelProjectName = ""
								});
						}

						ImGui::Indent();
						for (auto Iter = ActorInfo.mAnimationResources.value().begin(); Iter != ActorInfo.mAnimationResources.value().end();)
						{
							size_t i = std::distance(ActorInfo.mAnimationResources.value().begin(), Iter);

							ImGui::TableNextRow();
							ImGui::TableSetColumnIndex(0);
							ImGui::AlignTextToFramePadding();
							ImGui::Text("Animation Resource %u -", static_cast<unsigned int>(i + 1));
							ImGui::SameLine();
							if (ImGui::ImageButton(("##AnimationResource_" + std::to_string(i) + "_DelButton").c_str(), (ImTextureID)ImGuiExt::gDeleteButtonTexture->mID, ImVec2(Size, Size), ImVec2(1, 1), ImVec2(0, 0)))
							{
								Iter = ActorInfo.mAnimationResources.value().erase(Iter);
								continue;
							}

							ImGui::SameLine();
							if (ImGui::ImageButton(("##AnimationResource_" + std::to_string(i) + "_AddButton").c_str(), (ImTextureID)ImGuiExt::gAddButtonTexture->mID, ImVec2(Size, Size), ImVec2(0, 0)))
							{
								mPopUpDataStructPtr = &(*Iter);
								OpenAnimationResourcesPopUp = true;
							}

							ImGui::Indent();
							if (Iter->mModelProjectName.has_value())
							{
								DrawTableProperty("Model Project Name", PropertyId, [&Iter, i]() { ImGui::InputText(("##ModelProjectNameActorInfo_" + std::to_string(i)).c_str(), &Iter->mModelProjectName.value()); }, [&Iter]() { Iter->mModelProjectName = std::nullopt; });
							}
							if (Iter->mIsRetarget.has_value())
							{
								DrawTableProperty("Is Retarget", PropertyId, [&Iter, i]() { ImGui::Checkbox(("##IsRetargetActorInfo_" + std::to_string(i)).c_str(), &Iter->mIsRetarget.value()); }, [&Iter]() { Iter->mIsRetarget = std::nullopt; });
							}
							if (Iter->mIgnoreRetargetRate.has_value())
							{
								DrawTableProperty("Ignore Retarget Rate", PropertyId, [&Iter, i]() { ImGui::Checkbox(("##IgnoreRetargetRateActorInfo_" + std::to_string(i)).c_str(), &Iter->mIgnoreRetargetRate.value()); }, [&Iter]() { Iter->mIgnoreRetargetRate = std::nullopt; });
							}
							ImGui::Unindent();

							Iter++;
						}
						ImGui::Unindent();
					}
				AnimationResourcesSkip:;
				}

				if (ActorInfo.mApplyScaleType.has_value()) DrawTableProperty("Apply Scale Type", PropertyId, [&ActorInfo]() { ImGui::Combo("##ApplyScaleTypeActorInfo", reinterpret_cast<int*>(&ActorInfo.mApplyScaleType.value()), [](void* data, int idx, const char** out_text) { *out_text = ((const std::vector<std::string>*)data)->at(idx).c_str(); return true; }, &application::manager::ActorInfoMgr::ActorInfoEntry::gApplyScaleTypeNames, application::manager::ActorInfoMgr::ActorInfoEntry::gApplyScaleTypeNames.size()); }, [&ActorInfo]() { ActorInfo.mApplyScaleType = std::nullopt; });

				if (ActorInfo.mASInfoAutoPlayMaterials.has_value())
				{
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::AlignTextToFramePadding();
					ImGui::Text("ASInfo AutoPlay Materials");
					ImGui::SameLine();
					if (ImGui::ImageButton("##ASInfoAutoPlayMaterials_DelButton", (ImTextureID)ImGuiExt::gDeleteButtonTexture->mID, ImVec2(Size, Size), ImVec2(1, 1), ImVec2(0, 0)))
					{
						ActorInfo.mASInfoAutoPlayMaterials = std::nullopt;
						goto ASInfoAutoPlayMaterialsSkip;
					}
					{
						ImGui::SameLine();
						if (ImGui::ImageButton("##ASInfoAutoPlayMaterials_AddButton", (ImTextureID)ImGuiExt::gAddButtonTexture->mID, ImVec2(Size, Size), ImVec2(0, 0)))
						{
							ActorInfo.mASInfoAutoPlayMaterials.value().emplace_back(application::manager::ActorInfoMgr::ActorInfoEntry::ASInfoAutoPlayMaterial{
								.mFileName = ""
								});
						}

						ImGui::Indent();
						for (auto Iter = ActorInfo.mASInfoAutoPlayMaterials.value().begin(); Iter != ActorInfo.mASInfoAutoPlayMaterials.value().end();)
						{
							size_t i = std::distance(ActorInfo.mASInfoAutoPlayMaterials.value().begin(), Iter);

							ImGui::TableNextRow();
							ImGui::TableSetColumnIndex(0);
							ImGui::AlignTextToFramePadding();
							ImGui::Text("AutoPlay Material %u -", static_cast<unsigned int>(i + 1));
							ImGui::SameLine();
							if (ImGui::ImageButton(("##AutoPlayMaterial_" + std::to_string(i) + "_DelButton").c_str(), (ImTextureID)ImGuiExt::gDeleteButtonTexture->mID, ImVec2(Size, Size), ImVec2(1, 1), ImVec2(0, 0)))
							{
								Iter = ActorInfo.mASInfoAutoPlayMaterials.value().erase(Iter);
								continue;
							}

							ImGui::SameLine();
							if (ImGui::ImageButton(("##AutoPlayMaterial_" + std::to_string(i) + "_AddButton").c_str(), (ImTextureID)ImGuiExt::gAddButtonTexture->mID, ImVec2(Size, Size), ImVec2(0, 0)))
							{
								mPopUpDataStructPtr = &(*Iter);
								OpenASInfoAutoPlayMaterialPopUp = true;
							}

							ImGui::Indent();
							if (Iter->mFileName.has_value())
							{
								DrawTableProperty("File Name", PropertyId, [&Iter, i]() { ImGui::InputText(("##ASInfoAutoPlayMaterialFileNameActorInfo_" + std::to_string(i)).c_str(), &Iter->mFileName.value()); }, [&Iter]() { Iter->mFileName = std::nullopt; });
							}
							if (Iter->mIsRandom.has_value())
							{
								DrawTableProperty("Is Random", PropertyId, [&Iter, i]() { ImGui::Checkbox(("##ASInfoAutoPlayMaterialIsRandomActorInfo_" + std::to_string(i)).c_str(), &Iter->mIsRandom.value()); }, [&Iter]() { Iter->mIsRandom = std::nullopt; });
							}
							ImGui::Unindent();

							Iter++;
						}
						ImGui::Unindent();
					}
				ASInfoAutoPlayMaterialsSkip:;
				}

				if (ActorInfo.mASInfoAutoPlaySkeletals.has_value())
				{
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::AlignTextToFramePadding();
					ImGui::Text("ASInfo AutoPlay Skeletals");
					ImGui::SameLine();
					if (ImGui::ImageButton("##ASInfoAutoPlaySkeletals_DelButton", (ImTextureID)ImGuiExt::gDeleteButtonTexture->mID, ImVec2(Size, Size), ImVec2(1, 1), ImVec2(0, 0)))
					{
						ActorInfo.mASInfoAutoPlaySkeletals = std::nullopt;
						goto ASInfoAutoPlaySkeletalsSkip;
					}
					{
						ImGui::SameLine();
						if (ImGui::ImageButton("##ASInfoAutoPlaySkeletals_AddButton", (ImTextureID)ImGuiExt::gAddButtonTexture->mID, ImVec2(Size, Size), ImVec2(0, 0)))
						{
							ActorInfo.mASInfoAutoPlaySkeletals.value().emplace_back(application::manager::ActorInfoMgr::ActorInfoEntry::ASInfoAutoPlaySkeletal{
								.mFileName = ""
								});
						}

						ImGui::Indent();
						for (auto Iter = ActorInfo.mASInfoAutoPlaySkeletals.value().begin(); Iter != ActorInfo.mASInfoAutoPlaySkeletals.value().end();)
						{
							size_t i = std::distance(ActorInfo.mASInfoAutoPlaySkeletals.value().begin(), Iter);

							ImGui::TableNextRow();
							ImGui::TableSetColumnIndex(0);
							ImGui::AlignTextToFramePadding();
							ImGui::Text("AutoPlay Skeletal %u -", static_cast<unsigned int>(i + 1));
							ImGui::SameLine();
							if (ImGui::ImageButton(("##AutoPlaySkeletal_" + std::to_string(i) + "_DelButton").c_str(), (ImTextureID)ImGuiExt::gDeleteButtonTexture->mID, ImVec2(Size, Size), ImVec2(1, 1), ImVec2(0, 0)))
							{
								Iter = ActorInfo.mASInfoAutoPlaySkeletals.value().erase(Iter);
								continue;
							}

							ImGui::SameLine();
							if (ImGui::ImageButton(("##AutoPlaySkeletal_" + std::to_string(i) + "_AddButton").c_str(), (ImTextureID)ImGuiExt::gAddButtonTexture->mID, ImVec2(Size, Size), ImVec2(0, 0)))
							{
								mPopUpDataStructPtr = &(*Iter);
								OpenASInfoAutoPlaySkeletalPopUp = true;
							}

							ImGui::Indent();
							if (Iter->mFileName.has_value())
							{
								DrawTableProperty("File Name", PropertyId, [&Iter, i]() { ImGui::InputText(("##ASInfoAutoPlaySkeletalFileNameActorInfo_" + std::to_string(i)).c_str(), &Iter->mFileName.value()); }, [&Iter]() { Iter->mFileName = std::nullopt; });
							}
							ImGui::Unindent();

							Iter++;
						}
						ImGui::Unindent();
					}
				ASInfoAutoPlaySkeletalsSkip:;
				}

				if (ActorInfo.mASInfoAutoPlayBoneVisibilities.has_value())
				{
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::AlignTextToFramePadding();
					ImGui::Text("ASInfo AutoPlay Bone Visibilities");
					ImGui::SameLine();
					if (ImGui::ImageButton("##ASInfoAutoPlayBoneVisibilities_DelButton", (ImTextureID)ImGuiExt::gDeleteButtonTexture->mID, ImVec2(Size, Size), ImVec2(1, 1), ImVec2(0, 0)))
					{
						ActorInfo.mASInfoAutoPlayBoneVisibilities = std::nullopt;
						goto ASInfoAutoPlayBoneVisibilitiesSkip;
					}
					{
						ImGui::SameLine();
						if (ImGui::ImageButton("##ASInfoAutoPlayBoneVisibilities_AddButton", (ImTextureID)ImGuiExt::gAddButtonTexture->mID, ImVec2(Size, Size), ImVec2(0, 0)))
						{
							ActorInfo.mASInfoAutoPlayBoneVisibilities.value().emplace_back(application::manager::ActorInfoMgr::ActorInfoEntry::ASInfoAutoPlayBoneVisibility{
								.mFileName = ""
								});
						}

						ImGui::Indent();
						for (auto Iter = ActorInfo.mASInfoAutoPlayBoneVisibilities.value().begin(); Iter != ActorInfo.mASInfoAutoPlayBoneVisibilities.value().end();)
						{
							size_t i = std::distance(ActorInfo.mASInfoAutoPlayBoneVisibilities.value().begin(), Iter);

							ImGui::TableNextRow();
							ImGui::TableSetColumnIndex(0);
							ImGui::AlignTextToFramePadding();
							ImGui::Text("AutoPlay Bone Visibilitiy %u -", static_cast<unsigned int>(i + 1));
							ImGui::SameLine();
							if (ImGui::ImageButton(("##AutoPlayBoneVisibilitiy_" + std::to_string(i) + "_DelButton").c_str(), (ImTextureID)ImGuiExt::gDeleteButtonTexture->mID, ImVec2(Size, Size), ImVec2(1, 1), ImVec2(0, 0)))
							{
								Iter = ActorInfo.mASInfoAutoPlayBoneVisibilities.value().erase(Iter);
								continue;
							}

							ImGui::SameLine();
							if (ImGui::ImageButton(("##AutoPlayBoneVisibilitiy_" + std::to_string(i) + "_AddButton").c_str(), (ImTextureID)ImGuiExt::gAddButtonTexture->mID, ImVec2(Size, Size), ImVec2(0, 0)))
							{
								mPopUpDataStructPtr = &(*Iter);
								OpenASInfoAutoPlaySkeletalPopUp = true;
							}

							ImGui::Indent();
							if (Iter->mFileName.has_value())
							{
								DrawTableProperty("File Name", PropertyId, [&Iter, i]() { ImGui::InputText(("##ASInfoAutoPlayBoneVisibilitiyFileNameActorInfo_" + std::to_string(i)).c_str(), &Iter->mFileName.value()); }, [&Iter]() { Iter->mFileName = std::nullopt; });
							}
							ImGui::Unindent();

							Iter++;
						}
						ImGui::Unindent();
					}
				ASInfoAutoPlayBoneVisibilitiesSkip:;
				}

				if (ActorInfo.mCalcRadius.has_value()) DrawTableProperty("Calc Radius", PropertyId, [&ActorInfo]() { ImGui::InputScalar("##CalcRadiusActorInfo", ImGuiDataType_Float, &ActorInfo.mCalcRadius.value()); }, [&ActorInfo]() { ActorInfo.mCalcRadius = std::nullopt; });
				if (ActorInfo.mCalcRadiusDistanceType.has_value()) DrawTableProperty("Calc Radius Distance Type", PropertyId, [&ActorInfo]() { ImGui::Combo("##CalcRadiusDistanceTyoeActorInfo", reinterpret_cast<int*>(&ActorInfo.mCalcRadiusDistanceType.value()), [](void* data, int idx, const char** out_text) { *out_text = ((const std::vector<std::string>*)data)->at(idx).c_str(); return true; }, &application::manager::ActorInfoMgr::ActorInfoEntry::gCalcRadiusDistanceTypeNames, application::manager::ActorInfoMgr::ActorInfoEntry::gCalcRadiusDistanceTypeNames.size()); }, [&ActorInfo]() { ActorInfo.mCalcRadiusDistanceType = std::nullopt; });
				if (ActorInfo.mCalcPriority.has_value()) DrawTableProperty("Calc Priority", PropertyId, [&ActorInfo]() { ImGui::Combo("##CalcPriorityActorInfo", reinterpret_cast<int*>(&ActorInfo.mCalcPriority.value()), [](void* data, int idx, const char** out_text) { *out_text = ((const std::vector<std::string>*)data)->at(idx).c_str(); return true; }, &application::manager::ActorInfoMgr::ActorInfoEntry::gCalcPriorityNames, application::manager::ActorInfoMgr::ActorInfoEntry::gCalcPriorityNames.size()); }, [&ActorInfo]() { ActorInfo.mCalcPriority = std::nullopt; });

				DrawTableProperty("Category", PropertyId, [&ActorInfo]() { ImGui::Combo("##CategoryActorInfo", reinterpret_cast<int*>(&ActorInfo.mCategory), [](void* data, int idx, const char** out_text) { *out_text = ((const std::vector<std::string>*)data)->at(idx).c_str(); return true; }, &application::manager::ActorInfoMgr::ActorInfoEntry::gCategoryNames, application::manager::ActorInfoMgr::ActorInfoEntry::gCategoryNames.size()); });
				DrawTableProperty("Class Name", PropertyId, [&ActorInfo]() { ImGui::Combo("##ClassNameActorInfo", reinterpret_cast<int*>(&ActorInfo.mClassName), [](void* data, int idx, const char** out_text) { *out_text = ((const std::vector<std::string>*)data)->at(idx).c_str(); return true; }, &application::manager::ActorInfoMgr::ActorInfoEntry::gClassNameNames, application::manager::ActorInfoMgr::ActorInfoEntry::gClassNameNames.size()); });
				if (ActorInfo.mCreateRadiusOffset.has_value()) DrawTableProperty("Create Radius Offset", PropertyId, [&ActorInfo]() { ImGui::InputScalar("##CreateRadiusOffsetActorInfo", ImGuiDataType_Float, &ActorInfo.mCreateRadiusOffset.value()); }, [&ActorInfo]() { ActorInfo.mCreateRadiusOffset = std::nullopt; });
				if (ActorInfo.mDebugModelScale.has_value()) DrawTableProperty("Debug Model Scale", PropertyId, [&ActorInfo]() { ImGui::InputFloat3("##DebugModelScaleActorInfo", &ActorInfo.mDebugModelScale.value().x); }, [&ActorInfo]() { ActorInfo.mDebugModelScale = std::nullopt; });
				DrawTableProperty("Delete Radius Offset", PropertyId, [&ActorInfo]() { ImGui::InputScalar("##DeleteRadiusOffsetActorInfo", ImGuiDataType_Float, &ActorInfo.mDeleteRadiusOffset); });
				if (ActorInfo.mDisplayRadius.has_value()) DrawTableProperty("Display Radius", PropertyId, [&ActorInfo]() { ImGui::InputScalar("##DisplayRadiusActorInfo", ImGuiDataType_Float, &ActorInfo.mDisplayRadius.value()); }, [&ActorInfo]() { ActorInfo.mDisplayRadius = std::nullopt; });
				if (ActorInfo.mDisplayRadiusDistanceType.has_value()) DrawTableProperty("Display Radius Distance Type", PropertyId, [&ActorInfo]() { ImGui::Combo("##DisplayRadiusDistanceTyoeActorInfo", reinterpret_cast<int*>(&ActorInfo.mDisplayRadiusDistanceType.value()), [](void* data, int idx, const char** out_text) { *out_text = ((const std::vector<std::string>*)data)->at(idx).c_str(); return true; }, &application::manager::ActorInfoMgr::ActorInfoEntry::gDisplayRadiusDistanceTypeNames, application::manager::ActorInfoMgr::ActorInfoEntry::gDisplayRadiusDistanceTypeNames.size()); }, [&ActorInfo]() { ActorInfo.mDisplayRadiusDistanceType = std::nullopt; });
				if (ActorInfo.mELinkUserName.has_value()) DrawTableProperty("ELink UserName", PropertyId, [&ActorInfo]() { ImGui::InputText("##ELinkUserNameActorInfo", &ActorInfo.mELinkUserName.value()); }, [&ActorInfo]() { ActorInfo.mELinkUserName = std::nullopt; });
				if (ActorInfo.mFmdbName.has_value()) DrawTableProperty("FMDB Name", PropertyId, [&ActorInfo]() { ImGui::InputText("##FmdbNameActorInfo", &ActorInfo.mFmdbName.value()); }, [&ActorInfo]() { ActorInfo.mFmdbName = std::nullopt; });
				DrawTableProperty("Instance Heap Size", PropertyId, [&ActorInfo]() { ImGui::InputScalar("##InstanceHeapSizeActorInfo", ImGuiDataType_S32, &ActorInfo.mInstanceHeapSize); });
				if (ActorInfo.mIsCalcNodePushBack.has_value()) DrawTableProperty("Is Calc Node Push Back", PropertyId, [&ActorInfo]() { ImGui::Checkbox("##IsClacNodePushBackActorInfo", &ActorInfo.mIsCalcNodePushBack.value()); }, [&ActorInfo]() { ActorInfo.mIsCalcNodePushBack = std::nullopt; });
				if (ActorInfo.mIsEventHideCharacter.has_value()) DrawTableProperty("Is Event Hide Character", PropertyId, [&ActorInfo]() { ImGui::Checkbox("##IsEventHideCharacterActorInfo", &ActorInfo.mIsEventHideCharacter.value()); }, [&ActorInfo]() { ActorInfo.mIsEventHideCharacter = std::nullopt; });
				if (ActorInfo.mIsFarActor.has_value()) DrawTableProperty("Is Far Actor", PropertyId, [&ActorInfo]() { ImGui::Checkbox("##IsFarActorActorInfo", &ActorInfo.mIsFarActor.value()); }, [&ActorInfo]() { ActorInfo.mIsFarActor = std::nullopt; });
				if (ActorInfo.mIsNotTurnToActor.has_value()) DrawTableProperty("Is Not Turn To Actor", PropertyId, [&ActorInfo]() { ImGui::Checkbox("##IsNotTurnToActorActorInfo", &ActorInfo.mIsNotTurnToActor.value()); }, [&ActorInfo]() { ActorInfo.mIsNotTurnToActor = std::nullopt; });
				if (ActorInfo.mIsSinglePreActorRender.has_value()) DrawTableProperty("Is Single PreActor Render", PropertyId, [&ActorInfo]() { ImGui::Checkbox("##IsSinglePreActorRenderActorInfo", &ActorInfo.mIsSinglePreActorRender.value()); }, [&ActorInfo]() { ActorInfo.mIsSinglePreActorRender = std::nullopt; });
				if (ActorInfo.mLoadRadius.has_value()) DrawTableProperty("Load Radius", PropertyId, [&ActorInfo]() { ImGui::InputScalar("##LoadRadiusActorInfo", ImGuiDataType_Float, &ActorInfo.mLoadRadius.value()); }, [&ActorInfo]() { ActorInfo.mLoadRadius = std::nullopt; });
				if (ActorInfo.mLoadRadiusDistanceType.has_value()) DrawTableProperty("Load Radius Distance Type", PropertyId, [&ActorInfo]() { ImGui::Combo("##LoadRadiusDistanceTyoeActorInfo", reinterpret_cast<int*>(&ActorInfo.mLoadRadiusDistanceType.value()), [](void* data, int idx, const char** out_text) { *out_text = ((const std::vector<std::string>*)data)->at(idx).c_str(); return true; }, &application::manager::ActorInfoMgr::ActorInfoEntry::gLoadRadiusDistanceTypeNames, application::manager::ActorInfoMgr::ActorInfoEntry::gLoadRadiusDistanceTypeNames.size()); }, [&ActorInfo]() { ActorInfo.mLoadRadiusDistanceType = std::nullopt; });

				if (ActorInfo.mModelAabbMax.has_value()) DrawTableProperty("Model AABB Max", PropertyId, [&ActorInfo]() { ImGui::InputFloat3("##ModelAabbMaxActorInfo", &ActorInfo.mModelAabbMax.value().x); }, [&ActorInfo]() { ActorInfo.mModelAabbMax = std::nullopt; });
				if (ActorInfo.mModelAabbMin.has_value()) DrawTableProperty("Model AABB Min", PropertyId, [&ActorInfo]() { ImGui::InputFloat3("##ModelAabbMinActorInfo", &ActorInfo.mModelAabbMin.value().x); }, [&ActorInfo]() { ActorInfo.mModelAabbMin = std::nullopt; });

				if (ActorInfo.mModelProjectName.has_value()) DrawTableProperty("Model Project Name", PropertyId, [&ActorInfo]() { ImGui::InputText("##ModelProjectNameActorInfo", &ActorInfo.mModelProjectName.value()); }, [&ActorInfo]() { ActorInfo.mModelProjectName = std::nullopt; });
				if (ActorInfo.mModelVariationFmabFrame.has_value()) DrawTableProperty("Model Variation FMAB Frame", PropertyId, [&ActorInfo]() { ImGui::InputScalar("##ModelVariationFmabFrameActorInfo", ImGuiDataType_Float, &ActorInfo.mModelVariationFmabFrame.value()); }, [&ActorInfo]() { ActorInfo.mModelVariationFmabFrame = std::nullopt; });
				if (ActorInfo.mModelVariationFmabName.has_value()) DrawTableProperty("Model Variation Fmab Name", PropertyId, [&ActorInfo]() { ImGui::InputText("##ModelVariationFmabNameActorInfo", &ActorInfo.mModelVariationFmabName.value()); }, [&ActorInfo]() { ActorInfo.mModelVariationFmabName = std::nullopt; });
				if (ActorInfo.mNoUseIActorPresenceJudgeIfBanc.has_value()) DrawTableProperty("No Use IActor Presence Judge If Banc", PropertyId, [&ActorInfo]() { ImGui::Checkbox("##NoUseIActorPresenceJudgeIfBancActorInfo", &ActorInfo.mNoUseIActorPresenceJudgeIfBanc.value()); }, [&ActorInfo]() { ActorInfo.mNoUseIActorPresenceJudgeIfBanc = std::nullopt; });
				DrawTableProperty("PreActor RenderInfo Heap Size", PropertyId, [&ActorInfo]() { ImGui::InputScalar("##PreActorRenderInfoHeapSizeActorInfo", ImGuiDataType_S32, &ActorInfo.mPreActorRenderInfoHeapSize); });
				if (ActorInfo.mSLinkUserName.has_value()) DrawTableProperty("SLink UserName", PropertyId, [&ActorInfo]() { ImGui::InputText("##SLinkUserNameActorInfo", &ActorInfo.mSLinkUserName.value()); }, [&ActorInfo]() { ActorInfo.mSLinkUserName = std::nullopt; });
				DrawTableProperty("Unload Radius Offset", PropertyId, [&ActorInfo]() { ImGui::InputScalar("##UnloadRadiusOffsetActorInfo", ImGuiDataType_Float, &ActorInfo.mUnloadRadiusOffset); });
				if (ActorInfo.mUsePreActorRenderer.has_value()) DrawTableProperty("Use PreActor Renderer", PropertyId, [&ActorInfo]() { ImGui::Checkbox("##UsePreActorRendererActorInfo", &ActorInfo.mUsePreActorRenderer.value()); }, [&ActorInfo]() { ActorInfo.mUsePreActorRenderer = std::nullopt; });
				DrawTableProperty("__RowId", PropertyId, [&ActorInfo]() { ImGui::InputText("##RowIdActorInfo", &ActorInfo.mRowId); });


				ImGui::EndTable();
			}
		}

		if (OpenAnimationResourcesPopUp || OpenActorInfoPopUp || OpenASInfoAutoPlayMaterialPopUp || OpenASInfoAutoPlaySkeletalPopUp)
		{
			ImGui::SetNextWindowPos(ImGui::GetMousePos(), ImGuiCond_Appearing);
		}

		if (OpenAnimationResourcesPopUp) ImGui::OpenPopup("AddAnimationResourceChild");
		if (OpenActorInfoPopUp) ImGui::OpenPopup("AddActorInfoChild");
		if (OpenASInfoAutoPlayMaterialPopUp) ImGui::OpenPopup("AddASInfoAutoPlayMaterialChild");
		if (OpenASInfoAutoPlaySkeletalPopUp) ImGui::OpenPopup("AddASInfoAutoPlaySkeletalChild");

		if (ImGui::BeginPopup("AddActorInfoChild"))
		{
			ImGui::BeginDisabled(mActorInfo.value().mActorName.has_value());
			if (ImGui::MenuItem("Actor Name")) mActorInfo.value().mActorName = "";
			ImGui::EndDisabled();

			ImGui::BeginDisabled(mActorInfo.value().mAnimationResources.has_value());
			if (ImGui::MenuItem("Animation Resources")) mActorInfo.value().mAnimationResources.emplace();
			ImGui::EndDisabled();

			ImGui::BeginDisabled(mActorInfo.value().mApplyScaleType.has_value());
			if (ImGui::MenuItem("Apply Scale Type")) mActorInfo.value().mApplyScaleType = application::manager::ActorInfoMgr::ActorInfoEntry::ApplyScaleType::ScaleMax;
			ImGui::EndDisabled();

			ImGui::BeginDisabled(mActorInfo.value().mASInfoAutoPlayMaterials.has_value());
			if (ImGui::MenuItem("ASInfo AutoPlay Materials")) mActorInfo.value().mASInfoAutoPlayMaterials.emplace();
			ImGui::EndDisabled();

			ImGui::BeginDisabled(mActorInfo.value().mASInfoAutoPlaySkeletals.has_value());
			if (ImGui::MenuItem("ASInfo AutoPlay Skeletal")) mActorInfo.value().mASInfoAutoPlaySkeletals.emplace();
			ImGui::EndDisabled();

			ImGui::BeginDisabled(mActorInfo.value().mASInfoAutoPlayBoneVisibilities.has_value());
			if (ImGui::MenuItem("ASInfo AutoPlay Bone Visibility")) mActorInfo.value().mASInfoAutoPlayBoneVisibilities.emplace();
			ImGui::EndDisabled();

			ImGui::BeginDisabled(mActorInfo.value().mCalcRadius.has_value());
			if (ImGui::MenuItem("Calc Radius")) mActorInfo.value().mCalcRadius = 0.0f;
			ImGui::EndDisabled();

			ImGui::BeginDisabled(mActorInfo.value().mCalcRadiusDistanceType.has_value());
			if (ImGui::MenuItem("Calc Radius Distance Type")) mActorInfo.value().mCalcRadiusDistanceType = application::manager::ActorInfoMgr::ActorInfoEntry::CalcRadiusDistanceType::EuclideanXYZ;
			ImGui::EndDisabled();

			ImGui::BeginDisabled(mActorInfo.value().mCalcPriority.has_value());
			if (ImGui::MenuItem("Calc Priority")) mActorInfo.value().mCalcPriority = application::manager::ActorInfoMgr::ActorInfoEntry::CalcPriority::Before;
			ImGui::EndDisabled();

			ImGui::BeginDisabled(mActorInfo.value().mCreateRadiusOffset.has_value());
			if (ImGui::MenuItem("Create Radius Offset")) mActorInfo.value().mCreateRadiusOffset = 0.0f;
			ImGui::EndDisabled();

			ImGui::BeginDisabled(mActorInfo.value().mDebugModelScale.has_value());
			if (ImGui::MenuItem("Debug Model Scale")) mActorInfo.value().mDebugModelScale = glm::vec3(0.0f);
			ImGui::EndDisabled();

			ImGui::BeginDisabled(mActorInfo.value().mDisplayRadius.has_value());
			if (ImGui::MenuItem("Display Radius")) mActorInfo.value().mDisplayRadius = 0.0f;
			ImGui::EndDisabled();

			ImGui::BeginDisabled(mActorInfo.value().mDisplayRadiusDistanceType.has_value());
			if (ImGui::MenuItem("Display Radius Distance Type")) mActorInfo.value().mDisplayRadiusDistanceType = application::manager::ActorInfoMgr::ActorInfoEntry::DisplayRadiusDistanceType::EuclideanXYZ;
			ImGui::EndDisabled();

			ImGui::BeginDisabled(mActorInfo.value().mELinkUserName.has_value());
			if (ImGui::MenuItem("ELink UserName")) mActorInfo.value().mELinkUserName = "";
			ImGui::EndDisabled();

			ImGui::BeginDisabled(mActorInfo.value().mFmdbName.has_value());
			if (ImGui::MenuItem("FMDB Name")) mActorInfo.value().mFmdbName = "";
			ImGui::EndDisabled();

			ImGui::BeginDisabled(mActorInfo.value().mIsCalcNodePushBack.has_value());
			if (ImGui::MenuItem("Is Calc Node Push Back")) mActorInfo.value().mIsCalcNodePushBack = true;
			ImGui::EndDisabled();

			ImGui::BeginDisabled(mActorInfo.value().mIsEventHideCharacter.has_value());
			if (ImGui::MenuItem("Is Event Hide Character")) mActorInfo.value().mIsEventHideCharacter = true;
			ImGui::EndDisabled();

			ImGui::BeginDisabled(mActorInfo.value().mIsFarActor.has_value());
			if (ImGui::MenuItem("Is Far Actor")) mActorInfo.value().mIsFarActor = true;
			ImGui::EndDisabled();

			ImGui::BeginDisabled(mActorInfo.value().mIsNotTurnToActor.has_value());
			if (ImGui::MenuItem("Is Not Turn To Actor")) mActorInfo.value().mIsNotTurnToActor = true;
			ImGui::EndDisabled();

			ImGui::BeginDisabled(mActorInfo.value().mIsSinglePreActorRender.has_value());
			if (ImGui::MenuItem("Is Single PreActor Render")) mActorInfo.value().mIsSinglePreActorRender = true;
			ImGui::EndDisabled();

			ImGui::BeginDisabled(mActorInfo.value().mLoadRadius.has_value());
			if (ImGui::MenuItem("Load Radius")) mActorInfo.value().mLoadRadius = 0.0f;
			ImGui::EndDisabled();

			ImGui::BeginDisabled(mActorInfo.value().mLoadRadiusDistanceType.has_value());
			if (ImGui::MenuItem("Load Radius Distance Type")) mActorInfo.value().mLoadRadiusDistanceType = application::manager::ActorInfoMgr::ActorInfoEntry::LoadRadiusDistanceType::EuclideanXYZ;
			ImGui::EndDisabled();

			ImGui::BeginDisabled(mActorInfo.value().mModelAabbMax.has_value());
			if (ImGui::MenuItem("Model AABB Max")) mActorInfo.value().mModelAabbMax = glm::vec3(0.0f);
			ImGui::EndDisabled();

			ImGui::BeginDisabled(mActorInfo.value().mModelAabbMin.has_value());
			if (ImGui::MenuItem("Model AABB Min")) mActorInfo.value().mModelAabbMin = glm::vec3(0.0f);
			ImGui::EndDisabled();

			ImGui::BeginDisabled(mActorInfo.value().mModelProjectName.has_value());
			if (ImGui::MenuItem("Model Project Name")) mActorInfo.value().mModelProjectName = "";
			ImGui::EndDisabled();

			ImGui::BeginDisabled(mActorInfo.value().mModelVariationFmabFrame.has_value());
			if (ImGui::MenuItem("Model Variation FMAB Frame")) mActorInfo.value().mModelVariationFmabFrame = 0.0f;
			ImGui::EndDisabled();

			ImGui::BeginDisabled(mActorInfo.value().mModelVariationFmabName.has_value());
			if (ImGui::MenuItem("Model Variation FMAB Name")) mActorInfo.value().mModelVariationFmabName = "";
			ImGui::EndDisabled();

			ImGui::BeginDisabled(mActorInfo.value().mNoUseIActorPresenceJudgeIfBanc.has_value());
			if (ImGui::MenuItem("No Use IActor Presence Judge If Banc")) mActorInfo.value().mNoUseIActorPresenceJudgeIfBanc = true;
			ImGui::EndDisabled();

			ImGui::BeginDisabled(mActorInfo.value().mSLinkUserName.has_value());
			if (ImGui::MenuItem("SLink UserName")) mActorInfo.value().mSLinkUserName = "";
			ImGui::EndDisabled();

			ImGui::BeginDisabled(mActorInfo.value().mUsePreActorRenderer.has_value());
			if (ImGui::MenuItem("Use PreActor Renderer")) mActorInfo.value().mUsePreActorRenderer = true;
			ImGui::EndDisabled();

			ImGui::EndPopup();
		}

		if (ImGui::BeginPopup("AddAnimationResourceChild"))
		{
			application::manager::ActorInfoMgr::ActorInfoEntry::AnimationResource* Resource = static_cast<application::manager::ActorInfoMgr::ActorInfoEntry::AnimationResource*>(mPopUpDataStructPtr);
			
			ImGui::BeginDisabled(Resource->mModelProjectName.has_value());
			if (ImGui::MenuItem("Model Project Name")) Resource->mModelProjectName = "";
			ImGui::EndDisabled();

			ImGui::BeginDisabled(Resource->mIsRetarget.has_value());
			if (ImGui::MenuItem("Is Retarget")) Resource->mIsRetarget = true;
			ImGui::EndDisabled();

			ImGui::BeginDisabled(Resource->mIgnoreRetargetRate.has_value());
			if (ImGui::MenuItem("Ignore Retarget Rate")) Resource->mIgnoreRetargetRate = true;
			ImGui::EndDisabled();

			ImGui::EndPopup();
		}

		if (ImGui::BeginPopup("AddASInfoAutoPlayMaterialChild"))
		{
			application::manager::ActorInfoMgr::ActorInfoEntry::ASInfoAutoPlayMaterial* Resource = static_cast<application::manager::ActorInfoMgr::ActorInfoEntry::ASInfoAutoPlayMaterial*>(mPopUpDataStructPtr);

			ImGui::BeginDisabled(Resource->mFileName.has_value());
			if (ImGui::MenuItem("File Name")) Resource->mFileName = "";
			ImGui::EndDisabled();

			ImGui::BeginDisabled(Resource->mIsRandom.has_value());
			if (ImGui::MenuItem("Is Random")) Resource->mIsRandom = true;
			ImGui::EndDisabled();

			ImGui::EndPopup();
		}

		if (ImGui::BeginPopup("AddASInfoAutoPlaySkeletalChild"))
		{
			application::manager::ActorInfoMgr::ActorInfoEntry::ASInfoAutoPlaySkeletal* Resource = static_cast<application::manager::ActorInfoMgr::ActorInfoEntry::ASInfoAutoPlaySkeletal*>(mPopUpDataStructPtr);

			ImGui::BeginDisabled(Resource->mFileName.has_value());
			if (ImGui::MenuItem("File Name")) Resource->mFileName = "";
			ImGui::EndDisabled();

			ImGui::EndPopup();
		}

		if (mActor.has_value())
		{
			ImGui::NewLine();
			ImGui::Text("Actor Pack");
			ImGui::Separator();

			ImGui::Columns(2, "NameColumn");
			ImGui::Text("Name");
			ImGui::NextColumn();
			ImGui::PushItemWidth((ImGui::GetColumnWidth() - ImGui::GetStyle().ItemSpacing.x * 2) * 0.75f);
			ImGui::InputText("##ActorNameInput", &mActor.value().mName);
			ImGui::PopItemWidth();
			ImGui::SameLine();
			if (ImGui::Button("Rename##ActorNameRename", ImVec2((ImGui::GetColumnWidth() - ImGui::GetStyle().ItemSpacing.x * 2 - ImGui::GetStyle().FramePadding.x * 2) * 0.25f, 0)))
			{
				if (mActor.value().mName.length() > 1)
				{
					//Rename native files
					for (application::file::game::SarcFile::Entry& Entry : mActor.value().mActorPack.GetEntries())
					{
						if (mActor.value().mBymls.count(Entry.mName) && Entry.mName.find(mActor.value().mOriginalName) != std::string::npos)
						{
							auto NodeHandler = mActor.value().mBymls.extract(Entry.mName);
							application::util::General::ReplaceString(NodeHandler.key(), mActor.value().mOriginalName, mActor.value().mName);

							for (application::file::game::byml::BymlFile::Node& Node : NodeHandler.mapped().first.GetNodes())
							{
								ReplaceStringInBymlNodes(&Node, mActor.value().mOriginalName, mActor.value().mName);
							}

							NodeHandler.mapped().second = true;

							mActor.value().mBymls.insert(std::move(NodeHandler));
						}

						if (mActor.value().mAINBs.count(Entry.mName) && Entry.mName.find(mActor.value().mOriginalName) != std::string::npos)
						{
							auto NodeHandler = mActor.value().mAINBs.extract(Entry.mName);
							application::util::General::ReplaceString(NodeHandler.key(), mActor.value().mOriginalName, mActor.value().mName);

							application::file::game::ainb::AINBFile AINB;
							AINB.Initialize(NodeHandler.mapped().first);
							application::util::General::ReplaceString(AINB.Header.FileName, mActor.value().mOriginalName, mActor.value().mName);

							NodeHandler.mapped().first = AINB.ToBinary();
							NodeHandler.mapped().second = true;

							mActor.value().mAINBs.insert(std::move(NodeHandler));
						}

						application::util::General::ReplaceString(Entry.mName, mActor.value().mOriginalName, mActor.value().mName);
					}
					uint32_t Count = 0;
					//Rename shared files
					for (application::file::game::SarcFile::Entry& Entry : mActor.value().mActorPack.GetEntries())
					{
						if (Entry.mName.find(mActor.value().mName) == std::string::npos)
						{
							std::string Extension = Entry.mName.substr(Entry.mName.rfind('.') + 1);
							std::string UnprefixedName = mActor.value().mName + "_" + std::to_string(Count) + Entry.mName.substr(Entry.mName.find('.'));
							std::string UnprefixedOldName = Entry.mName.substr(Entry.mName.find_last_of("/") + 1);
							UnprefixedName = UnprefixedName.substr(0, UnprefixedName.find_last_of('.'));
							UnprefixedOldName = UnprefixedOldName.substr(0, UnprefixedOldName.find_last_of('.'));
							std::string NewName = Entry.mName.substr(0, Entry.mName.find_last_of('/') + 1) + UnprefixedName;

							std::size_t CategoryStart = UnprefixedOldName.find("__");

							if (CategoryStart != std::string::npos)
							{
								CategoryStart += 2;
								std::size_t DotStart = UnprefixedOldName.find('.', CategoryStart);
								if (DotStart != std::string::npos)
								{
									std::string Category = UnprefixedOldName.substr(CategoryStart, DotStart - CategoryStart);
									UnprefixedName = mActor.value().mName + "_" + std::to_string(Count) + "__" + Category + Entry.mName.substr(Entry.mName.find('.'));
									UnprefixedName = UnprefixedName.substr(0, UnprefixedName.find_last_of('.'));
									NewName = Entry.mName.substr(0, Entry.mName.find_last_of('/') + 1) + UnprefixedName;
								}
							}
							NewName += "." + Extension;

							if (mActor.value().mBymls.count(Entry.mName))
							{
								auto NodeHandler = mActor.value().mBymls.extract(Entry.mName);
								NodeHandler.key() = NewName;

								for (application::file::game::byml::BymlFile::Node& Node : NodeHandler.mapped().first.GetNodes())
								{
									ReplaceStringInBymlNodes(&Node, UnprefixedOldName, UnprefixedName);
								}

								NodeHandler.mapped().second = true;

								mActor.value().mBymls.insert(std::move(NodeHandler));
							}

							if (mActor.value().mAINBs.count(Entry.mName))
							{
								auto NodeHandler = mActor.value().mAINBs.extract(Entry.mName);
								NodeHandler.key() = NewName;

								application::file::game::ainb::AINBFile AINB;
								AINB.Initialize(NodeHandler.mapped().first);
								AINB.Header.FileName = UnprefixedName;

								NodeHandler.mapped().first = AINB.ToBinary();
								NodeHandler.mapped().second = true;

								mActor.value().mAINBs.insert(std::move(NodeHandler));
							}

							//Fix references
							for (application::file::game::SarcFile::Entry& RefEntry : mActor.value().mActorPack.GetEntries())
							{
								if (mActor.value().mBymls.count(RefEntry.mName))
								{
									auto NodeHandler = mActor.value().mBymls.extract(RefEntry.mName);
									for (application::file::game::byml::BymlFile::Node& Node : NodeHandler.mapped().first.GetNodes())
									{
										ReplaceStringInBymlNodes(&Node, UnprefixedOldName, UnprefixedName);
									}

									NodeHandler.mapped().second = true;

									mActor.value().mBymls.insert(std::move(NodeHandler));
								}
							}
							Entry.mName = NewName;
							Count++;
						}
					}

					application::util::General::ReplaceString(mActorInfo.value().mRowId, mActor.value().mOriginalName, mActor.value().mName);
					if(mActorInfo.value().mActorName.has_value()) application::util::General::ReplaceString(mActorInfo.value().mActorName.value(), mActor.value().mOriginalName, mActor.value().mName);
					if(mActorInfo.value().mELinkUserName.has_value()) application::util::General::ReplaceString(mActorInfo.value().mELinkUserName.value(), mActor.value().mOriginalName, mActor.value().mName);
					if(mActorInfo.value().mFmdbName.has_value()) application::util::General::ReplaceString(mActorInfo.value().mFmdbName.value(), mActor.value().mOriginalName, mActor.value().mName);
					if(mActorInfo.value().mModelProjectName.has_value()) application::util::General::ReplaceString(mActorInfo.value().mModelProjectName.value(), mActor.value().mOriginalName, mActor.value().mName);
					if(mActorInfo.value().mModelVariationFmabName.has_value()) application::util::General::ReplaceString(mActorInfo.value().mModelVariationFmabName.value(), mActor.value().mOriginalName, mActor.value().mName);
					if(mActorInfo.value().mSLinkUserName.has_value()) application::util::General::ReplaceString(mActorInfo.value().mSLinkUserName.value(), mActor.value().mOriginalName, mActor.value().mName);

					mActor.value().mOriginalName = mActor.value().mName;
					mActorGyml.value() = mActor.value().mName;
				}
			}
			ImGui::Columns();

			ImGui::SeparatorText("BYML");
			int Id = 0;

			for (int i = 0; i < mActor.value().mBymls.size(); i++)
			{
				auto Iter = mActor.value().mBymls.begin();
				std::advance(Iter, i);

				if (ImGui::CollapsingHeader(Iter->first.c_str()))
				{
					bool Modified = false;

					ImGui::Indent();

					ImGui::Columns(2, Iter->first.c_str());
					ImGui::AlignTextToFramePadding();
					for (application::file::game::byml::BymlFile::Node& Node : Iter->second.first.GetNodes())
					{
						Modified |= DisplayBymlNode(&Node, Id);
						ImGui::NextColumn();
					}
					ImGui::Columns();
					ImGui::Unindent();

					Iter->second.second |= Modified;
				}
			}
			ImGui::NewLine();

			ImGui::SeparatorText("AINB");
			ImGui::Columns(2, "AINB");

			for (int i = 0; i < mActor.value().mAINBs.size(); i++)
			{
				auto Iter = mActor.value().mAINBs.begin();
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
					std::unique_ptr<application::rendering::UIWindowBase>& Window = application::manager::UIMgr::OpenWindow(std::make_unique<application::rendering::ainb::UIAINBEditor>());
					application::rendering::ainb::UIAINBEditor* EditorPtr = static_cast<application::rendering::ainb::UIAINBEditor*>(Window.get());
					std::vector<unsigned char>& AINBBinary = Iter->second.first;
					EditorPtr->mInitCallback = [&AINBBinary](application::rendering::ainb::UIAINBEditor* Editor)
						{
							Editor->LoadAINB(AINBBinary);
							Editor->mEnableOpen = false;
							Editor->mEnableSaveAs = true;
							Editor->mEnableSaveOverwrite = true;
							Editor->mEnableSaveInProject = false;
						};

					std::string AINBName = Iter->first;
					EditorPtr->mSaveCallback = [this, AINBName](application::file::game::ainb::AINBFile& AINB)
						{
							application::util::Logger::Info("UIActorTool", "Injecting AINB file called %s", AINB.Header.FileName.c_str());
							mActor.value().mAINBs[AINBName].first = AINB.ToBinary();
							mActor.value().mAINBs[AINBName].second = true;
						};
					ImGui::SetWindowFocus("AINB Editor");
				}
				ImGui::SameLine();
			}

			ImGui::Columns();

			/*
			for (const std::unique_ptr<application::game::actor_component::ActorComponentBase>& ComponentBase : mActorPack->mComponents)
			{
				if (ImGui::CollapsingHeader(ComponentBase->GetDisplayName().c_str()))
				{
					ImGui::Indent();
					ComponentBase->DrawEditingMenu();
					ImGui::Unindent();
				}
			}
			*/


		}

        ImGui::End();
	}

	void UIActorTool::DeleteImpl()
	{

	}

	std::string UIActorTool::GetWindowTitle() const
	{
		if (mSameWindowTypeCount > 0)
		{
			return "Actor Editor (" + std::to_string(mSameWindowTypeCount) + ")###" + std::to_string(mWindowId);
		}

		return "Actor Editor###" + std::to_string(mWindowId);
	}

	UIWindowBase::WindowType UIActorTool::GetWindowType()
	{
		return UIWindowBase::WindowType::EDITOR_ACTOR;
	}
}