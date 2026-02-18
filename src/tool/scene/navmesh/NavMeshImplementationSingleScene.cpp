#include "NavMeshImplementationSingleScene.h"

#include <util/Logger.h>
#include <util/FileUtil.h>
#include <file/game/zstd/ZStdBackend.h>
#include <imgui.h>
#include <manager/ShaderMgr.h>
#include <rendering/ImGuiExt.h>
#include <util/MeshOptimizer.h>
#include <util/Math.h>

namespace application::tool::scene::navmesh
{
	NavMeshImplementationSingleScene::NavMeshImplementationSingleScene(application::game::Scene* Scene) : application::tool::scene::NavMeshImplementationBase(), mScene(Scene)
	{

	}

	NavMeshImplementationSingleScene::~NavMeshImplementationSingleScene()
	{
		if (mMesh.has_value())
		{
			mMesh.value().Delete();
			mMesh = std::nullopt;
		}
	}

	bool NavMeshImplementationSingleScene::Initialize()
	{
		if(mShader == nullptr)
			mShader = application::manager::ShaderMgr::GetShader("NavMesh");

		if (application::util::FileUtil::FileExists(application::util::FileUtil::GetRomFSFilePath("Phive/NavMesh/" + mScene->mBcettName + ".Nin_NX_NVN.bphnm.zs")))
		{
			mNavMesh = application::file::game::phive::navmesh::PhiveNavMesh(application::file::game::ZStdBackend::Decompress(application::util::FileUtil::GetRomFSFilePath("Phive/NavMesh/" + mScene->mBcettName + ".Nin_NX_NVN.bphnm.zs")));
			
			std::pair<std::vector<glm::vec3>, std::vector<uint32_t>> ModelData = mNavMesh->ToVerticesIndices();
			mMesh = application::gl::SimpleMesh(ModelData.first, ModelData.second);

			/*
			std::vector<glm::vec3> ClusterVertices;
			std::vector<uint32_t> Indices;
			for (auto& Vec : mNavMesh->mContainer.m_namedVariantClusterGraph.m_variant.m_ptr.m_positions)
			{
				ClusterVertices.emplace_back(Vec.m_subTypeArray[0], Vec.m_subTypeArray[1], Vec.m_subTypeArray[2]);
			}

			for (auto& Node : mNavMesh->mContainer.m_namedVariantClusterGraph.m_variant.m_ptr.m_nodes)
			{
				if (Node.m_numEdges == 3)
				{
					Indices.push_back(mNavMesh->mContainer.m_namedVariantClusterGraph.m_variant.m_ptr.m_edges[Node.m_startEdgeIndex].m_source.m_primitiveBase);
					Indices.push_back(mNavMesh->mContainer.m_namedVariantClusterGraph.m_variant.m_ptr.m_edges[Node.m_startEdgeIndex + 1].m_source.m_primitiveBase);
					Indices.push_back(mNavMesh->mContainer.m_namedVariantClusterGraph.m_variant.m_ptr.m_edges[Node.m_startEdgeIndex + 2].m_source.m_primitiveBase);
					continue;
				}

				if (Node.m_numEdges < 3)
				{
					if (Node.m_numEdges == 0 || Node.m_numEdges == 1)
						continue;

					Indices.push_back(mNavMesh->mContainer.m_namedVariantClusterGraph.m_variant.m_ptr.m_edges[Node.m_startEdgeIndex].m_source.m_primitiveBase);
					Indices.push_back(mNavMesh->mContainer.m_namedVariantClusterGraph.m_variant.m_ptr.m_edges[Node.m_startEdgeIndex + 1].m_source.m_primitiveBase);
					Indices.push_back(mNavMesh->mContainer.m_namedVariantClusterGraph.m_variant.m_ptr.m_edges[Node.m_startEdgeIndex + 1].m_source.m_primitiveBase);
					continue;
				}

				std::vector<std::pair<glm::vec3, int32_t>> FaceVertices;
				std::vector<application::file::game::phive::PhiveNavMesh::hkaiClusterGraph__Edge> NodeEdges(Node.m_numEdges);
				for (size_t i = 0; i < NodeEdges.size(); i++)
				{
					NodeEdges[i] = mNavMesh->mContainer.m_namedVariantClusterGraph.m_variant.m_ptr.m_edges[Node.m_startEdgeIndex + i];
				}

				for (size_t i = 0; i < NodeEdges.size(); i++)
				{
					auto& Vertex = mNavMesh->mContainer.m_namedVariantClusterGraph.m_variant.m_ptr.m_positions[NodeEdges[i].m_source.m_primitiveBase];
					FaceVertices.push_back(std::make_pair(glm::vec3(Vertex.m_subTypeArray[0], Vertex.m_subTypeArray[1], Vertex.m_subTypeArray[2]), NodeEdges[i].m_source.m_primitiveBase));
				}

				for (uint32_t i = 0; i < FaceVertices.size() - 2; i++)
				{
					Indices.push_back(FaceVertices[0].second);
					Indices.push_back(FaceVertices[i + 1].second);
					Indices.push_back(FaceVertices[i + 2].second);
				}
			}

			mClusterGraphMesh = application::gl::SimpleMesh(ClusterVertices, Indices);
			*/
		}

		return true;
	}

	void NavMeshImplementationSingleScene::DrawNavMesh(application::gl::Camera& Camera)
	{
		if (!mMesh.has_value())
			return;

		mShader->Bind();
		Camera.Matrix(mShader, "camMatrix");

		glm::mat4 GLMModel = glm::mat4(1.0f);
		glUniformMatrix4fv(glGetUniformLocation(mShader->mID, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(GLMModel));

		mMesh->Draw();
	}

	void NavMeshImplementationSingleScene::DrawImGuiGeneratorMenu()
	{
		/*
					float mMinCharacterRadius = 0.2f;
			float mMaxCharacterRadius = 0.4f;
			float mMaxStepHeight = 0.3f;
			float mMaxGapWidth = 0.3f;
		*/

		ImGuiExt::InputFloatSliderCombo("Cell size", "NavMeshCellSize", &mGeneratorData.mGeometryConfig.mCellSize, 0.1f, 10.0f);
		ImGuiExt::InputFloatSliderCombo("Cell height", "NavMeshCellHeight", &mGeneratorData.mGeometryConfig.mCellHeight, 0.1f, 10.0f);
		ImGuiExt::InputFloatSliderCombo("Walkable slope angle", "NavMeshSlopeAngle", &mGeneratorData.mGeometryConfig.mWalkableSlopeAngle, 0.001f, 180.0f);
		ImGuiExt::InputFloatSliderCombo("Walkable height", "NavMeshHeight", &mGeneratorData.mGeometryConfig.mWalkableHeight, 0.001f, 10.0f);
		ImGuiExt::InputFloatSliderCombo("Walkable climb", "NavMeshClimb", &mGeneratorData.mGeometryConfig.mWalkableClimb, 0.001f, 10.0f);
		ImGuiExt::InputFloatSliderCombo("Walkable radius", "NavMeshRadius", &mGeneratorData.mGeometryConfig.mWalkableRadius, 0.001f, 10.0f);
		ImGuiExt::InputIntSliderCombo("Min Region Area", "NavMeshMinRegionArea", &mGeneratorData.mGeometryConfig.mMinRegionArea, 1, 256);

		if (ImGui::Button("Reset##UIToolsNavMeshReset"))
		{
			mGeneratorData = application::file::game::phive::navmesh::PhiveNavMesh::GeneratorData();
		}

		ImGui::SameLine();

		if (ImGui::Button("Generate"))
		{
			if (!mNavMesh.has_value())
			{
				mNavMesh = application::file::game::phive::navmesh::PhiveNavMesh(application::file::game::ZStdBackend::Decompress(application::util::FileUtil::GetRomFSFilePath("Phive/NavMesh/Dungeon136.Nin_NX_NVN.bphnm.zs")));
			}

			application::file::game::phive::navmesh::PhiveNavMesh& NavMesh = mNavMesh.value();

			std::pair<std::vector<glm::vec3>, std::vector<unsigned int>> ModelData = mScene->GetSceneModel([](application::game::Scene::BancEntityRenderInfo* Info)
				{
					return Info->mEntity->mBakeable;
				});

			std::vector<std::pair<glm::vec3, glm::vec3>> VolumeBoxes;
			std::vector<std::pair<glm::vec3, glm::vec3>> ProhibitionBoxes;

			for (application::game::Scene::BancEntityRenderInfo* RenderInfo : mScene->mDrawListRenderInfoIndices)
			{
				if (RenderInfo->mEntity->mGyml == "Area" && RenderInfo->mEntity->mDynamic.contains("Starlight_NavMeshVolume"))
				{
					glm::vec4 MinPoint(-1, 0, -1, 1);
					glm::vec4 MaxPoint(1, 2, 1, 1);

					glm::mat4 GLMModel = glm::mat4(1.0f); // Identity matrix

					GLMModel = glm::translate(GLMModel, RenderInfo->mTranslate);

					GLMModel = glm::rotate(GLMModel, glm::radians(RenderInfo->mRotate.z), glm::vec3(0.0, 0.0f, 1.0));
					GLMModel = glm::rotate(GLMModel, glm::radians(RenderInfo->mRotate.y), glm::vec3(0.0f, 1.0, 0.0));
					GLMModel = glm::rotate(GLMModel, glm::radians(RenderInfo->mRotate.x), glm::vec3(1.0, 0.0f, 0.0));

					GLMModel = glm::scale(GLMModel, RenderInfo->mScale);

					MinPoint = GLMModel * MinPoint;
					MaxPoint = GLMModel * MaxPoint;

					if(std::get<bool>(RenderInfo->mEntity->mDynamic["Starlight_NavMeshVolume"]))
						VolumeBoxes.push_back(std::make_pair(glm::vec3(MinPoint.x, MinPoint.y, MinPoint.z), glm::vec3(MaxPoint.x, MaxPoint.y, MaxPoint.z)));
					else
						ProhibitionBoxes.push_back(std::make_pair(glm::vec3(MinPoint.x, MinPoint.y, MinPoint.z), glm::vec3(MaxPoint.x, MaxPoint.y, MaxPoint.z)));
				}
			}

			ModelData = application::util::Math::ClipMeshToBoxes(ModelData, VolumeBoxes);
			ModelData = application::util::Math::ClipOutsideMeshToBoxes(ModelData, ProhibitionBoxes);

			mGeneratorData.mIsSingleSceneNavMesh = true;
			mGeneratorData.mVertices = ModelData.first;
			mGeneratorData.mIndices = ModelData.second;

			NavMesh.InjectNavMeshModel(mGeneratorData);
			std::vector<unsigned char> Data = NavMesh.ToBinary();

			std::filesystem::create_directories(application::util::FileUtil::GetSaveFilePath("Phive/NavMesh"));
			application::util::FileUtil::WriteFile(application::util::FileUtil::GetSaveFilePath("Phive/NavMesh/" + mScene->mBcettName + ".Nin_NX_NVN.bphnm.zs"), application::file::game::ZStdBackend::Compress(Data, application::file::game::ZStdBackend::Dictionary::Base));

			if (mScene->mGameBancParam.has_value())
			{
				if (mScene->mGameBancParam.value().HasChild("NavMeshMgrMode"))
				{
					if (mScene->mGameBancParam.value().GetNode("NavMeshMgrMode")->GetValue<std::string>() == "None")
					{
						mScene->mGameBancParam.value().GetNode("NavMeshMgrMode")->SetValue<std::string>("Single");
						std::filesystem::create_directories(std::filesystem::path(application::util::FileUtil::GetSaveFilePath(mScene->mGameBancParamPath)).parent_path());
						application::util::FileUtil::WriteFile(application::util::FileUtil::GetSaveFilePath(mScene->mGameBancParamPath), mScene->mGameBancParam.value().ToBinary(application::file::game::byml::BymlFile::TableGeneration::AUTO));
					}
				}
			}

			std::pair<std::vector<glm::vec3>, std::vector<uint32_t>> NavMeshModelData = mNavMesh->ToVerticesIndices();
			mMesh = application::gl::SimpleMesh(NavMeshModelData.first, NavMeshModelData.second);
		}
	}

	bool NavMeshImplementationSingleScene::SupportNavMeshGeneration()
	{
		return true;
	}

	bool NavMeshImplementationSingleScene::SupportNavMeshViewing()
	{
		return mNavMesh.has_value();
	}
}