#include "UITools.h"

#include "imgui.h"
#include "imgui_stdlib.h"
#include "Exporter.h"
#include "Editor.h"
#include "UIMapView.h"
#include "PopupExportMod.h"
#include "EditorConfig.h"
#include "PopupEditorAINBActorLinks.h"
#include "SceneMgr.h"
#include "ZStdFile.h"
#include "Byml.h"
#include "Logger.h"
#include "Util.h"
#include "UIOutliner.h"
#include "PopupStackActors.h"
#include "HashMgr.h"
#include "StarImGui.h"
#include "StarlightData.h"
#include "ObjWriter.h"
#include "SceneExporter.h"

#include <glm/glm.hpp>

bool UITools::Open = true;

void UITools::DrawToolsWindow()
{
	if (!Open) return;

	ImGui::Begin("Tools", &Open);

	if (ImGui::Button("Save", ImVec2(ImGui::GetWindowSize().x * 0.5f - ImGui::GetStyle().ItemSpacing.x, 0)))
	{
		Exporter::Export(Editor::GetWorkingDirFile("Save"), Exporter::Operation::Save);
	}
	ImGui::SameLine();
	if (ImGui::Button("Export", ImVec2(ImGui::GetWindowSize().x * 0.5f - ImGui::GetStyle().FramePadding.x, 0)))
	{
		PopupExportMod::Open([](std::string Path)
			{
				Exporter::Export(Path, Exporter::Operation::Export);
			});
	}

	ImGui::Separator();

	if (UIMapView::Focused)
	{
		if (ImGui::CollapsingHeader("Render", ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Indent();
			ImGui::Checkbox("Visible actors", &UIMapView::RenderSettings.RenderVisibleActors);
			ImGui::Checkbox("Invisible actors", &UIMapView::RenderSettings.RenderInvisibleActors);
			ImGui::Indent();
			if (!UIMapView::RenderSettings.RenderInvisibleActors) ImGui::BeginDisabled();
			ImGui::Checkbox("Areas", &UIMapView::RenderSettings.RenderAreas);
			if (!UIMapView::RenderSettings.RenderInvisibleActors) ImGui::EndDisabled();
			ImGui::Unindent();
			ImGui::Checkbox("Far actors", &UIMapView::RenderSettings.RenderFarActors);

			ImGui::Checkbox("NPCs", &UIMapView::RenderSettings.RenderNPCs);
			if (SceneMgr::SceneType != SceneMgr::Type::SmallDungeon && SceneMgr::SceneType != SceneMgr::Type::LargeDungeon && SceneMgr::SceneType != SceneMgr::Type::NormalStage)
				ImGui::BeginDisabled();
			ImGui::Checkbox("NavMesh", &UIMapView::RenderSettings.RenderNavMesh);
			if (SceneMgr::SceneType != SceneMgr::Type::SmallDungeon && SceneMgr::SceneType != SceneMgr::Type::LargeDungeon && SceneMgr::SceneType != SceneMgr::Type::NormalStage)
				ImGui::EndDisabled();

			if (ImGui::Checkbox("Cull faces", &UIMapView::RenderSettings.CullFaces))
				GLBfres::mDrawFunc = UIMapView::RenderSettings.CullFaces ? GLBfres::DrawCulled : GLBfres::DrawNotCulled;

			ImGui::Unindent();
		}
		if (SceneMgr::SceneType == SceneMgr::Type::SmallDungeon && !Editor::Identifier.empty())
		{
			if (ImGui::CollapsingHeader("SmallDungeon", ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::Indent();
				if (UIOutliner::SelectedActor == nullptr)
					ImGui::BeginDisabled();
				if (ImGui::Button("StartPos to selected"))
				{
					BymlFile StartPosByml(ZStdFile::Decompress(Editor::GetRomFSFile("Banc/SmallDungeon/StartPos/SmallDungeon.startpos.byml.zs"), ZStdFile::Dictionary::Base).Data);

					if (StartPosByml.GetNode("OnElevator")->HasChild("Dungeon" + Editor::Identifier))
					{
						StartPosByml.GetNode("OnElevator")->GetChild("Dungeon" + Editor::Identifier)->GetChild("Trans")->GetChild(0)->SetValue<float>(UIOutliner::SelectedActor->Translate.GetX());
						StartPosByml.GetNode("OnElevator")->GetChild("Dungeon" + Editor::Identifier)->GetChild("Trans")->GetChild(1)->SetValue<float>(UIOutliner::SelectedActor->Translate.GetY());
						StartPosByml.GetNode("OnElevator")->GetChild("Dungeon" + Editor::Identifier)->GetChild("Trans")->GetChild(2)->SetValue<float>(UIOutliner::SelectedActor->Translate.GetZ());

						StartPosByml.GetNode("OnElevator")->GetChild("Dungeon" + Editor::Identifier)->GetChild("Rot")->GetChild(0)->SetValue<float>(Util::DegreesToRadians(UIOutliner::SelectedActor->Rotate.GetX()));
						StartPosByml.GetNode("OnElevator")->GetChild("Dungeon" + Editor::Identifier)->GetChild("Rot")->GetChild(1)->SetValue<float>(Util::DegreesToRadians(UIOutliner::SelectedActor->Rotate.GetY()));
						StartPosByml.GetNode("OnElevator")->GetChild("Dungeon" + Editor::Identifier)->GetChild("Rot")->GetChild(2)->SetValue<float>(Util::DegreesToRadians(UIOutliner::SelectedActor->Rotate.GetZ()));
					}

					Util::CreateDir(Editor::GetWorkingDirFile("Save/Banc/SmallDungeon/StartPos"));
					ZStdFile::Compress(StartPosByml.ToBinary(BymlFile::TableGeneration::Auto), ZStdFile::Dictionary::Base).WriteToFile(Editor::GetWorkingDirFile("Save/Banc/SmallDungeon/StartPos/SmallDungeon.startpos.byml.zs"));
				}
				if (UIOutliner::SelectedActor == nullptr)
					ImGui::EndDisabled();

				if (ImGui::Button("Regenerate static compound(s)"))
				{
					if (!SceneMgr::mStaticCompounds.empty())
					{
						for (PhiveStaticCompound& StaticCompound : SceneMgr::mStaticCompounds)
						{
							std::string CompoundName = "Dungeon" + Editor::Identifier;
							for (size_t i = 0; i < 128; i++)
							{
								StaticCompound.mImage.mCompoundName[i] = 0;
							}

							for (size_t i = 0; i < CompoundName.length(); i++)
							{
								StaticCompound.mImage.mCompoundName[i] = CompoundName[i];
							}

							StaticCompound.mActorHashMap.clear();
							StaticCompound.mActorLinks.clear();
							//StaticCompound.mWaterFallArray.clear();
							//StaticCompound.mWaterCylinderArray.clear();
							//StaticCompound.mWaterBoxArray.clear();
							//StaticCompound.mExternalBphshMeshes.clear();
							//StaticCompound.mCompoundTagFile.clear();
							//StaticCompound.mTeraWaterTerrain.clear();
							//StaticCompound.mTeraWaterSubTerrain0.clear();
							//StaticCompound.mTeraWaterSubTerrain1.clear();

							HashMgr::mNewHashes.clear();

							for (Actor& SceneActor : ActorMgr::GetActors())
							{
								if (ActorMgr::IsPhysicsActor(SceneActor.Gyml))
								{
									HashMgr::mNewHashes.push_back(SceneActor.Hash);
								}

								for (Actor& ChildActor : ActorMgr::GetActors())
								{
									if (ActorMgr::IsPhysicsActor(ChildActor.Gyml))
									{
										HashMgr::mNewHashes.push_back(ChildActor.Hash);
									}
								}
							}
						}
					}
				}

				ImGui::Unindent();
			}
		}

#ifdef STARLIGHT_SHARED
		if(false)
#else
		if (Editor::Identifier.length() > 0 && (SceneMgr::SceneType == SceneMgr::Type::SmallDungeon || SceneMgr::SceneType == SceneMgr::Type::LargeDungeon || SceneMgr::SceneType == SceneMgr::Type::NormalStage) && ImGui::CollapsingHeader("NavMesh##UIToolsHeader"))
#endif
		{
			StarImGui::InputFloatSliderCombo("Cell size", "NavMeshCellSize", &PhiveNavMesh::mNavMeshGeneratorConfig.mCellSize, 0.1f, 10.0f);
			StarImGui::InputFloatSliderCombo("Cell height", "NavMeshCellHeight", &PhiveNavMesh::mNavMeshGeneratorConfig.mCellHeight, 0.1f, 10.0f);
			StarImGui::InputFloatSliderCombo("Walkable slope angle", "NavMeshSlopeAngle", &PhiveNavMesh::mNavMeshGeneratorConfig.mWalkableSlopeAngle, 0.001f, 180.0f);
			StarImGui::InputFloatSliderCombo("Walkable height", "NavMeshHeight", &PhiveNavMesh::mNavMeshGeneratorConfig.mWalkableHeight, 0.001f, 10.0f);
			StarImGui::InputFloatSliderCombo("Walkable climb", "NavMeshClimb", &PhiveNavMesh::mNavMeshGeneratorConfig.mWalkableClimb, 0.001f, 10.0f);
			StarImGui::InputFloatSliderCombo("Walkable radius", "NavMeshRadius", &PhiveNavMesh::mNavMeshGeneratorConfig.mWalkableRadius, 0.001f, 10.0f);
			StarImGui::InputIntSliderCombo("Min Region Area", "NavMeshMinRegionArea", &PhiveNavMesh::mNavMeshGeneratorConfig.mMinRegionArea, 1, 256);
			ImGui::Checkbox("Only needs Bakeable", &PhiveNavMesh::mNavMeshGeneratorConfig.mEverythingBakeable);

			if (ImGui::Button("Reset##UIToolsNavMeshReset"))
			{
				PhiveNavMesh::mNavMeshGeneratorConfig.mCellSize = 0.3f;
				PhiveNavMesh::mNavMeshGeneratorConfig.mCellHeight = 0.3f;
				PhiveNavMesh::mNavMeshGeneratorConfig.mWalkableSlopeAngle = 30.0f;
				PhiveNavMesh::mNavMeshGeneratorConfig.mWalkableHeight = 2.0;
				PhiveNavMesh::mNavMeshGeneratorConfig.mWalkableClimb = 1.0f;
				PhiveNavMesh::mNavMeshGeneratorConfig.mWalkableRadius = 0.5f;
				PhiveNavMesh::mNavMeshGeneratorConfig.mMinRegionArea = 3;
				PhiveNavMesh::mNavMeshGeneratorConfig.mEverythingBakeable = false;
			}

			ImGui::SameLine();

			if (ImGui::Button("Generate NavMesh"))
			{
				std::vector<float> Vertices;
				std::vector<uint32_t> Indices;

				auto GenereateNavMeshModel = [](Actor& SceneActor, std::vector<float>& Vertices, std::vector<uint32_t>& Indices)
					{
						if (!PhiveNavMesh::mNavMeshGeneratorConfig.mEverythingBakeable)
						{
							if (ActorMgr::IsPhysicsActor(SceneActor.Gyml) || SceneActor.Gyml.find("SkyAntiZonau") != std::string::npos || SceneActor.Model->mBfres->mDefaultModel)
								return;
						}
						else
						{
							if (SceneActor.Gyml.find("SkyAntiZonau") != std::string::npos || SceneActor.Model->mBfres->mDefaultModel || !SceneActor.Bakeable)
								return;
						}

						glm::mat4 GLMModel = glm::mat4(1.0f);  // Identity matrix

						GLMModel = glm::translate(GLMModel, glm::vec3(SceneActor.Translate.GetX(), SceneActor.Translate.GetY(), SceneActor.Translate.GetZ()));

						GLMModel = glm::rotate(GLMModel, glm::radians(SceneActor.Rotate.GetZ()), glm::vec3(0.0, 0.0f, 1.0));
						GLMModel = glm::rotate(GLMModel, glm::radians(SceneActor.Rotate.GetY()), glm::vec3(0.0f, 1.0, 0.0));
						GLMModel = glm::rotate(GLMModel, glm::radians(SceneActor.Rotate.GetX()), glm::vec3(1.0, 0.0f, 0.0));

						GLMModel = glm::scale(GLMModel, glm::vec3(SceneActor.Scale.GetX(), SceneActor.Scale.GetY(), SceneActor.Scale.GetZ()));

						BfresFile::Model& Model = SceneActor.Model->mBfres->Models.GetByIndex(0).Value;

						for (auto& [Key, Val] : Model.Shapes.Nodes)
						{
							uint32_t IndexOffset = Vertices.size() / 3;
							for (glm::vec4 Pos : Val.Value.Vertices)
							{
								glm::vec4 Vertex(Pos.x, Pos.y, Pos.z, 1.0f);
								Vertex = GLMModel * Vertex;
								Vertices.push_back(Vertex.x);
								Vertices.push_back(Vertex.y);
								Vertices.push_back(Vertex.z);
							}
							for (uint32_t Index : Val.Value.Meshes[0].GetIndices())
							{
								Indices.push_back(Index + IndexOffset);
							}
						}
					};

				for (Actor& SceneActor : ActorMgr::GetActors())
				{
					GenereateNavMeshModel(SceneActor, Vertices, Indices);
					for (Actor& ChildActor : SceneActor.MergedActorContent)
					{
						GenereateNavMeshModel(ChildActor, Vertices, Indices);
					}
				}

				PhiveNavMesh NavMesh(ZStdFile::Decompress(Editor::GetRomFSFile("Phive/NavMesh/Dungeon136.Nin_NX_NVN.bphnm.zs", false), ZStdFile::Dictionary::Base).Data);
				NavMesh.SetNavMeshModel(Vertices, Indices);
				std::vector<unsigned char> Data = NavMesh.ToBinary();
				Util::CreateDir(Editor::GetWorkingDirFile("Save/Phive/NavMesh"));
				switch (SceneMgr::SceneType)
				{
				case SceneMgr::Type::SmallDungeon:
				{
					ZStdFile::Compress(Data, ZStdFile::Dictionary::Base).WriteToFile(Editor::GetWorkingDirFile("Save/Phive/NavMesh/Dungeon" + Editor::Identifier + ".Nin_NX_NVN.bphnm.zs"));
					break;
				}
				case SceneMgr::Type::LargeDungeon:
				case SceneMgr::Type::NormalStage:
				{
					std::string NavMeshIdentifier = Editor::Identifier == "LargeDungeonWater_AllinArea" ? "LargeDungeonWater" : Editor::Identifier;
					ZStdFile::Compress(Data, ZStdFile::Dictionary::Base).WriteToFile(Editor::GetWorkingDirFile("Save/Phive/NavMesh/" + NavMeshIdentifier + ".Nin_NX_NVN.bphnm.zs"));
					break;
				}
				}
				SceneMgr::NavigationMeshModel = SceneMgr::LoadNavMeshBfres(NavMesh);

				//Processing GameBancParam to use NavMesh
				BymlFile GameBancParam;
				switch (SceneMgr::SceneType)
				{
				case SceneMgr::Type::SmallDungeon:
				{
					GameBancParam = BymlFile(Editor::GetRomFSFile("Banc/GameBancParam/Dungeon" + Editor::Identifier + ".game__banc__GameBancParam.bgyml"));
					break;
				}
				case SceneMgr::Type::LargeDungeon:
				case SceneMgr::Type::NormalStage:
				{
					std::string NavMeshIdentifier = Editor::Identifier == "LargeDungeonWater_AllinArea" ? "LargeDungeonWater" : Editor::Identifier;
					GameBancParam = BymlFile(Editor::GetRomFSFile("Banc/GameBancParam/" + NavMeshIdentifier + ".game__banc__GameBancParam.bgyml"));
					break;
				}
				}

				if (GameBancParam.GetNodes().empty())
				{
					Logger::Error("UITools", "Could not find GameBancParam");
				}
				else
				{
					if (GameBancParam.HasChild("NavMeshMgrMode"))
					{
						if (GameBancParam.GetNode("NavMeshMgrMode")->GetValue<std::string>() == "None")
						{
							GameBancParam.GetNode("NavMeshMgrMode")->SetValue<std::string>("Single");
							Util::CreateDir(Editor::GetWorkingDirFile("Save/Banc/GameBancParam"));
							switch (SceneMgr::SceneType)
							{
							case SceneMgr::Type::SmallDungeon:
							{
								GameBancParam.WriteToFile(Editor::GetWorkingDirFile("Save/Banc/GameBancParam/Dungeon" + Editor::Identifier + ".game__banc__GameBancParam.bgyml"), BymlFile::TableGeneration::Auto);
								break;
							}
							case SceneMgr::Type::LargeDungeon:
							case SceneMgr::Type::NormalStage:
							{
								std::string NavMeshIdentifier = Editor::Identifier == "LargeDungeonWater_AllinArea" ? "LargeDungeonWater" : Editor::Identifier;
								GameBancParam.WriteToFile(Editor::GetWorkingDirFile("Save/Banc/GameBancParam/" + NavMeshIdentifier + ".game__banc__GameBancParam.bgyml"), BymlFile::TableGeneration::Auto);
								break;
							}
							}
						}
					}
				}
			}
		}

		if(!Editor::Identifier.empty() && ImGui::CollapsingHeader("General##UIToolsHeader"))
		{
			if (ImGui::Button("Stack actors"))
			{
				PopupStackActors::Open([](uint64_t SrcActor, float OffX, float OffY, float OffZ, uint16_t Count) {

					Actor BaseActor;

					for (Actor& Act : ActorMgr::GetActors())
					{
						if (Act.Hash == SrcActor)
						{
							BaseActor = Act;
							break;
						}
					}

					if (BaseActor.Hash == 0)
					{
						Logger::Error("ActorStacker", "Could not find base actor with hash " + std::to_string(SrcActor));
						return;
					}

					for (uint16_t i = 0; i < Count; i++)
					{
						Actor& NewActor = ActorMgr::AddActor(BaseActor);
						NewActor.Translate.SetX(BaseActor.Translate.GetX() + OffX * (i + 1));
						NewActor.Translate.SetY(BaseActor.Translate.GetY() + OffY * (i + 1));
						NewActor.Translate.SetZ(BaseActor.Translate.GetZ() + OffZ * (i + 1));

						UIOutliner::SelectedActor = &NewActor;
					}

					}, UIOutliner::SelectedActor == nullptr ? 0 : UIOutliner::SelectedActor->Hash);
			}

			ImGui::SameLine();

			if (ImGui::Button("AINB actor links"))
			{
				PopupEditorAINBActorLinks::Open();
			}
		}
		if (StarlightData::mLevel != StarlightData::Level::PATREON && !Editor::Identifier.empty() && ImGui::CollapsingHeader("Hash##UIToolsHeader"))
		{
			ImGui::InputScalar("BiggestHash", ImGuiDataType_U64, &HashMgr::BiggestHash);
			ImGui::InputScalar("BiggestSRTHash", ImGuiDataType_U32, &HashMgr::BiggestSRTHash);
		}
		if (StarlightData::mLevel != StarlightData::Level::PATREON && !Editor::Identifier.empty() && ImGui::CollapsingHeader("Export scene model"))
		{
			ImGui::Indent();

			ImGui::Checkbox("Move to 0", &SceneExporter::mSettings.mAlignToZero);
			ImGui::Checkbox("Export Far Actors", &SceneExporter::mSettings.mExportFarActors);

			ImGui::Text("Filters");
			ImGui::SameLine();
			if (ImGui::Button("+##AddSceneExporterFilter"))
			{
				SceneExporter::mSettings.mFilterActors.push_back("None");
			}
			ImGui::Separator();
			ImGui::Indent();
			for (auto Iter = SceneExporter::mSettings.mFilterActors.begin(); Iter != SceneExporter::mSettings.mFilterActors.end(); )
			{
				uint32_t i = std::distance(SceneExporter::mSettings.mFilterActors.begin(), Iter);
				ImGui::InputText(("##" + std::to_string(i)).c_str(), &SceneExporter::mSettings.mFilterActors[i]);
				ImGui::SameLine();
				if (ImGui::Button(("-##DelSceneExporterFilter" + std::to_string(i)).c_str()))
				{
					Iter = SceneExporter::mSettings.mFilterActors.erase(Iter);
					continue;
				}
				Iter++;
			}
			ImGui::Unindent();

			if (ImGui::Button("Export scene"))
			{
				SceneExporter::ExportScene();
			}

			ImGui::Unindent();
		}
	}

	ImGui::End();
}