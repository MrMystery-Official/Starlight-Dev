#include "ActorPainter.h"

#include <rendering/mapeditor/UIMapEditor.h>
#include <rendering/ImGuiExt.h>
#include <manager/BfresFileMgr.h>
#include <manager/BfresRendererMgr.h>
#include <manager/MergedActorMgr.h>
#include <manager/ShaderMgr.h>
#include <util/Math.h>
#include <random>
#include "imgui_stdlib.h"

namespace application::rendering::map_editor
{
	application::gl::SimpleMesh ActorPainter::gSphereMesh;
	application::gl::Shader* ActorPainter::gSphereShader = nullptr;

	void ActorPainter::Initialize()
	{
		auto [Vertices, Indices] = application::util::Math::GenerateSphere(1.0f, 32, 32);

		gSphereShader = application::manager::ShaderMgr::GetShader("ActorPainterSphere");
		gSphereMesh.Initialize(Vertices, Indices);
	}

	void ActorPainter::SetEnabled(bool Enabled, void* MapEditorPtr)
	{
		mEnabled = Enabled;

		if (mEnabled)
		{
			application::rendering::map_editor::UIMapEditor* MapEditor = reinterpret_cast<application::rendering::map_editor::UIMapEditor*>(MapEditorPtr);

			MapEditor->DeselectActor();

			if (mTerrainMode == 1 && mTerrainActorEntity != nullptr)
			{
				mTerrainVertices.clear();
				mTerrainIndices.clear();
				for (auto& [Key, Val] : mTerrainActorEntity->mBfresRenderer->mBfresFile->Models.GetByIndex(0).mValue.Shapes.mNodes)
				{
					uint32_t IndexOffset = mTerrainVertices.size();
					for (const glm::vec4& Vertex : Val.mValue.Vertices)
					{
						mTerrainVertices.push_back(glm::vec3(Vertex) + mTerrainActorEntity->mTranslate);
					}

					for (const uint32_t& Index : Val.mValue.Meshes[0].GetIndices())
					{
						mTerrainIndices.push_back(IndexOffset + Index);
					}
				}
			}
			else if (mTerrainMode == 0)
			{
				mTerrainVertices.clear();
				mTerrainIndices.clear();
			}
		}
	}

	void ActorPainter::DrawMenu(const float& w, const ImVec2& Pos, void* MapEditorPtr)
	{
		ImGui::SetNextWindowPos(ImVec2(Pos.x + w, Pos.y));
		if (ImGui::BeginPopup("ActorPainter"))
		{
			application::rendering::map_editor::UIMapEditor* MapEditor = reinterpret_cast<application::rendering::map_editor::UIMapEditor*>(MapEditorPtr);

			static const char* TERRAIN_MODES[] {"Terrain", "Actor"};
			if (!MapEditor->mScene.IsTerrainScene())
			{
				ImGui::BeginDisabled();
				mTerrainMode = 1;
			}
			if (ImGui::Combo("Terrain Mode", &mTerrainMode, TERRAIN_MODES, IM_ARRAYSIZE(TERRAIN_MODES)))
			{
				if (mTerrainMode == 1 && mTerrainActorEntity != nullptr)
				{
					mTerrainVertices.clear();
					mTerrainIndices.clear();
					for (auto& [Key, Val] : mTerrainActorEntity->mBfresRenderer->mBfresFile->Models.GetByIndex(0).mValue.Shapes.mNodes)
					{
						uint32_t IndexOffset = mTerrainVertices.size();
						for (const glm::vec4& Vertex : Val.mValue.Vertices)
						{
							mTerrainVertices.push_back(glm::vec3(Vertex) + mTerrainActorEntity->mTranslate);
						}

						for (const uint32_t& Index : Val.mValue.Meshes[0].GetIndices())
						{
							mTerrainIndices.push_back(IndexOffset + Index);
						}
					}
				}
				else if (mTerrainMode == 0)
				{
					mTerrainVertices.clear();
					mTerrainIndices.clear();
				}
			}
			if (!MapEditor->mScene.IsTerrainScene())
			{
				ImGui::EndDisabled();
			}

			if (mTerrainMode == 1)
			{
				ImGui::PushStyleColor(ImGuiCol_FrameBg, mTerrainActorEntity != nullptr ? ImVec4(0.06f, 0.26f, 0.07f, 1.0f) : ImVec4(0.26f, 0.06f, 0.07f, 1.0f));
				if (ImGui::InputScalar("Terrain Hash##ActorPainter", ImGuiDataType_U64, &mTerrainActorHash))
				{
					mTerrainActorEntity = nullptr;
					mTerrainVertices.clear();
					mTerrainIndices.clear();
					for (application::game::Scene::BancEntityRenderInfo* RenderInfo : MapEditor->mScene.mDrawListRenderInfoIndices)
					{
						if (RenderInfo->mEntity->mHash == mTerrainActorHash)
						{
							mTerrainActorEntity = RenderInfo->mEntity;

							if (mEnabled)
							{
								for (auto& [Key, Val] : mTerrainActorEntity->mBfresRenderer->mBfresFile->Models.GetByIndex(0).mValue.Shapes.mNodes)
								{
									uint32_t IndexOffset = mTerrainVertices.size();
									for (const glm::vec4& Vertex : Val.mValue.Vertices)
									{
										mTerrainVertices.push_back(glm::vec3(Vertex) + mTerrainActorEntity->mTranslate);
									}

									for (const uint32_t& Index : Val.mValue.Meshes[0].GetIndices())
									{
										mTerrainIndices.push_back(IndexOffset + Index);
									}
								}
							}

							break;
						}
					}
				}
				ImGui::PopStyleColor();
			}

			static const char* DataTypes[]{ "Static", "Dynamic", "Merged" };
			ImGui::Combo("Target BancType", reinterpret_cast<int*>(&mBancType), DataTypes, IM_ARRAYSIZE(DataTypes));

			if (mBancType != application::game::BancEntity::BancType::MERGED)
				ImGui::BeginDisabled();

			ImGui::InputText("BancPath", &mBancPath);

			ImGui::PushStyleColor(ImGuiCol_FrameBg, mMergedActorEntity != nullptr ? ImVec4(0.06f, 0.26f, 0.07f, 1.0f) : ImVec4(0.26f, 0.06f, 0.07f, 1.0f));
			if (ImGui::InputScalar("Parent Hash##ActorPainter", ImGuiDataType_U64, &mMergedActorHash))
			{
				mMergedActorEntity = nullptr;
				for (application::game::Scene::BancEntityRenderInfo* RenderInfo : MapEditor->mScene.mDrawListRenderInfoIndices)
				{
					if (RenderInfo->mEntity->mHash == mMergedActorHash)
					{
						mMergedActorEntity = RenderInfo->mEntity;
						break;
					}
				}
			}
			ImGui::PopStyleColor();

			if (mBancType != application::game::BancEntity::BancType::MERGED)
				ImGui::EndDisabled();

			ImGui::InputScalar("Radius##ActorPainter", ImGuiDataType_Float, &mRadius);
			ImGui::InputScalar("Actors to place##ActorPainter", ImGuiDataType_U32, &mIterations);

			ImGui::InputFloat3("Scale Min", &mScaleMin.x);
			ImGui::InputFloat3("Scale Max", &mScaleMax.x);
			ImGui::InputFloat3("Rotation Min", &mRotMin.x);
			ImGui::InputFloat3("Rotation Max", &mRotMax.x);

			ImGui::NewLine();
			ImGui::Text("Actors");
			ImGui::SameLine();
			if (ImGui::Button("+##AddActorPainterActor"))
			{
				mEntries.push_back(application::rendering::map_editor::ActorPainter::ActorEntry
					{
						.mGyml = "None",
						.mYModelOffset = 0.0f,
						.mProbability = 1.0f
					});
			}
			int Index = 0;
			for (application::rendering::map_editor::ActorPainter::ActorEntry& Entry : mEntries)
			{
				ImGui::PushItemWidth(ImGui::CalcTextSize(Entry.mGyml.c_str()).x + 2 * ImGui::GetStyle().ItemInnerSpacing.x);
				ImGui::InputText(("Gyml##" + std::to_string(Index)).c_str(), &Entry.mGyml);
				ImGui::PopItemWidth();

				ImGui::SameLine();

				ImGui::PushItemWidth(ImGui::CalcTextSize(std::to_string(Entry.mProbability).c_str()).x + 2 * ImGui::GetStyle().ItemInnerSpacing.x);
				ImGui::InputFloat(("Probability##" + std::to_string(Index)).c_str(), &Entry.mProbability);
				ImGui::PopItemWidth();

				ImGui::SameLine();

				ImGui::PushItemWidth(ImGui::CalcTextSize(std::to_string(Entry.mYModelOffset).c_str()).x + 2 * ImGui::GetStyle().ItemInnerSpacing.x);
				ImGui::InputFloat(("Y-Offset##" + std::to_string(Index)).c_str(), &Entry.mYModelOffset);
				ImGui::PopItemWidth();

				Index++;
			}

			ImGui::EndPopup();
		}
	}

	float CalculateModelYOffset(application::game::BancEntity* Entity)
	{
		float SmallestY = std::numeric_limits<float>::max();
		for (auto& [Key, Val] : Entity->mBfresRenderer->mBfresFile->Models.GetByIndex(0).mValue.Shapes.mNodes)
		{
			std::vector<uint32_t> Indices = Val.mValue.Meshes[0].GetIndices();
			for (size_t i = 0; i < Indices.size() / 3; i++)
			{
				if (Val.mValue.Vertices[Indices[i * 3 + 1]].y < SmallestY) SmallestY = Val.mValue.Vertices[Indices[i * 3 + 1]].y;
			}
		}

		return SmallestY;
	}

	glm::vec3 NormalToEuler(const glm::vec3& Normal) 
	{
		glm::vec3 n = glm::normalize(Normal);
		glm::vec3 up(0.0f, 1.0f, 0.0f);
		glm::quat RotQuat = glm::rotation(up, n);
		glm::vec3 EulerAngles = glm::degrees(glm::eulerAngles(RotQuat));

		EulerAngles.x = fmod(EulerAngles.x + 180.0f, 360.0f) - 180.0f;
		EulerAngles.y = fmod(EulerAngles.y + 180.0f, 360.0f) - 180.0f;
		EulerAngles.z = fmod(EulerAngles.z + 180.0f, 360.0f) - 180.0f;

		return EulerAngles;
	}

	void ActorPainter::RenderActorPainter(float XPos, float YPos, int ScreenWidth, int ScreenHeight, void* MapEditorPtr)
	{
		if (!mEnabled || ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopup))
			return;

		application::rendering::map_editor::UIMapEditor* MapEditor = reinterpret_cast<application::rendering::map_editor::UIMapEditor*>(MapEditorPtr);

		if ((mTerrainMode == 1 && mTerrainActorEntity == nullptr) || MapEditor->mCamera.IsInCameraMovement() || ImGui::IsAnyItemHovered())
			return;

		glm::vec3 RayDir = application::util::Math::GetRayFromMouse(XPos, YPos, MapEditor->mCamera.mProjection, MapEditor->mCamera.mView, ScreenWidth, ScreenHeight);

		glm::vec3 Intersection = glm::vec3(0.0f);

		if (mTerrainMode == 1 && mTerrainActorEntity != nullptr && !mTerrainVertices.empty() && !mTerrainIndices.empty())
		{
			Intersection = application::util::Math::FindTerrainIntersectionModelData(MapEditor->mCamera.mPosition, RayDir, mTerrainVertices, mTerrainIndices);
		}
		else if (mTerrainMode == 0)
		{

		}

		if (Intersection != glm::vec3(0.0f)) 
		{
			MapEditor->gInstancedShader->Bind();


			glm::mat4 GLMModel = glm::mat4(1.0f);
			GLMModel = glm::translate(GLMModel, Intersection);
			GLMModel = glm::scale(GLMModel, glm::vec3(mRadius, mRadius, mRadius));

			gSphereShader->Bind();
			MapEditor->mCamera.Matrix(gSphereShader, "camMatrix");
			glUniformMatrix4fv(glGetUniformLocation(gSphereShader->mID, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(GLMModel));
			gSphereMesh.Draw();

			//application::manager::BfresRendererMgr::GetRenderer(application::manager::BfresFileMgr::GetBfresFile("Default"))->Draw(InstanceMatrix);

			if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
			{
				RayDir = glm::vec3(0.0f, -1.0f, 0.0f);
				unsigned Seed = std::chrono::system_clock::now().time_since_epoch().count();
				std::default_random_engine Generator(Seed);
				std::uniform_real_distribution<float> Distribution(0.0f, 1.0f);

				if (mBancType == application::game::BancEntity::BancType::MERGED)
				{
					std::vector<application::game::BancEntity>* MergedActor = application::manager::MergedActorMgr::GetMergedActor(mBancPath, true);
					if (MergedActor == nullptr)
						return;

					mMergedActorEntity->mMergedActorChildren = MergedActor;
					mMergedActorEntity->mDynamic["BancPath"] = mBancPath;
				}

				for (int i = 0; i < mIterations; i++)
				{
					glm::vec3 Pos = Intersection;
					float angle = rand() * 1.0f / RAND_MAX * 2.0f * M_PI;

					// Generate random radius (0 to mRadius)
					// Note: For uniform distribution, use square root of random value
					float randomValue = rand() * 1.0f / RAND_MAX;
					float radius = mRadius * std::sqrt(randomValue);

					// Convert polar coordinates to Cartesian
					Pos.x += radius * std::cos(angle);
					Pos.z += radius * std::sin(angle);
					Pos.y += 100.0f;

					std::pair<glm::vec3, glm::vec3> ActorTerrainIntersection;

					if (mTerrainMode == 1 && mTerrainActorEntity != nullptr && !mTerrainVertices.empty() && !mTerrainIndices.empty())
					{
						ActorTerrainIntersection = application::util::Math::FindTerrainIntersectionAndNormalVecModelData(Pos, RayDir, mTerrainVertices, mTerrainIndices);
					}
					else if (mTerrainMode == 0)
					{

					}

					if (ActorTerrainIntersection.first != glm::vec3(0.0f))
					{
						ActorEntry* SelectedEntry = nullptr;
						float RandomValue = Distribution(Generator);

						float CumulativeProbability = 0.0f;
						for (auto& Entry : mEntries) 
						{
							CumulativeProbability += Entry.mProbability;
							if (RandomValue <= CumulativeProbability) {
								SelectedEntry = &Entry;
								break;
							}
						}

						if (SelectedEntry == nullptr)
							SelectedEntry = &mEntries.back();

						std::optional<application::game::BancEntity> Entity = MapEditor->mScene.CreateBancEntity(SelectedEntry->mGyml);

						if (!Entity.has_value())
						{
							continue;
						}

						/*
						if (Entry->mYModelOffset == 1000.0f)
						{
							Entry->mYModelOffset = -CalculateModelYOffset(NewActor);
						}
						*/

						application::game::Scene::BancEntityRenderInfo RenderInfo;
						RenderInfo.mEntity = &Entity.value();
						RenderInfo.mMergedActorParent = mMergedActorEntity;

						RenderInfo.mTranslate.x = Pos.x;
						RenderInfo.mTranslate.y = ActorTerrainIntersection.first.y + SelectedEntry->mYModelOffset;
						RenderInfo.mTranslate.z = Pos.z;

						RenderInfo.mScale.x = mScaleMin.x + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (mScaleMax.x - mScaleMin.x)));
						RenderInfo.mScale.y = mScaleMin.y + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (mScaleMax.y - mScaleMin.y)));
						RenderInfo.mScale.z = mScaleMin.z + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (mScaleMax.z - mScaleMin.z)));

						glm::vec3 TerrainNormalRot = NormalToEuler(ActorTerrainIntersection.second);

						RenderInfo.mRotate.x = TerrainNormalRot.x + mRotMin.x + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (mRotMax.x - mRotMin.x)));
						RenderInfo.mRotate.y = TerrainNormalRot.y + mRotMin.y + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (mRotMax.y - mRotMin.y)));
						RenderInfo.mRotate.z = TerrainNormalRot.z + mRotMin.z + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (mRotMax.z - mRotMin.z)));

						Entity.value().mBancType = mBancType;

						Entity.value().mPhive.mPlacement["ID"] = Entity.value().mHash;

						MapEditor->mScene.SyncBancEntity(&RenderInfo);

						if (mBancType != application::game::BancEntity::BancType::MERGED)
						{
							MapEditor->mScene.mBancEntities.push_back(Entity.value());
						}
						else
						{
							std::vector<application::game::BancEntity>* MergedActor = application::manager::MergedActorMgr::GetMergedActor(mBancPath, true);
							if (MergedActor == nullptr)
								continue;

							MergedActor->push_back(Entity.value());
						}
					}
				}

				MapEditor->mScene.GenerateDrawList();

				mTerrainActorEntity = nullptr;
				for (application::game::Scene::BancEntityRenderInfo* RenderInfo : MapEditor->mScene.mDrawListRenderInfoIndices)
				{
					if (RenderInfo->mEntity->mHash == mTerrainActorHash)
					{
						mTerrainActorEntity = RenderInfo->mEntity;
						break;
					}
				}
			}
		}
	}
}