#include "NavMeshImplementationFieldScene.h"

#include <util/Logger.h>
#include <util/FileUtil.h>
#include <file/game/zstd/ZStdBackend.h>
#include <imgui.h>
#include <manager/ShaderMgr.h>
#include <rendering/ImGuiExt.h>
#include <util/MeshOptimizer.h>
#include <util/Math.h>
#include <meshoptimizer.h>

namespace application::tool::scene::navmesh
{
	NavMeshImplementationFieldScene::NavMeshImplementationFieldScene(application::game::Scene* Scene) : application::tool::scene::NavMeshImplementationBase(), mScene(Scene)
	{

	}

	NavMeshImplementationFieldScene::~NavMeshImplementationFieldScene()
	{
		for(auto& Mesh : mMeshes)
		{
			Mesh.Delete();
		}
	}

	bool NavMeshImplementationFieldScene::Initialize()
	{
		int XIndex = mScene->mBcettName[0] - 65;
		int ZIndex = ((char)mScene->mBcettName[2]) - '0' - 1;

		std::array<int, 4> xIndices;
		for (int i = 0; i < 4; ++i) 
		{
			xIndices[i] = i + (XIndex * 1000) / 250;
		}

		// Calculate z indices
		std::array<int, 4> zIndices;
		for (int i = 0; i < 4; ++i) {
			zIndices[i] = i + (ZIndex * 1000) / 250;
		}

		// Generate navmesh sections
		std::vector<FieldNavMeshSectionInfo> NavMeshSections;
		NavMeshSections.reserve(16); // Pre-allocate for efficiency

		for (int zIndex : zIndices)
		{
			for (int xIndex : xIndices)
			{
				NavMeshSections.emplace_back(xIndex, zIndex);

				std::cout << "Navmesh Section: " << xIndex << ", " << zIndex << std::endl;
			}
		}

		std::string FieldName = mScene->GetSceneTypeName(mScene->mSceneType);

		for (const FieldNavMeshSectionInfo& Info : NavMeshSections)
		{
			mNavMeshes.emplace_back(std::make_pair(Info, application::file::game::ZStdBackend::Decompress(application::util::FileUtil::GetRomFSFilePath("Phive/StitchedNavMesh/" + FieldName + "/" + Info.GetSectionName() + ".Nin_NX_NVN.bphnm.zs"))));
		}

		/*
		std::vector<unsigned char> NavMeshBinary;
		application::file::game::phive::navmesh::PhiveNavMesh NavMesh;
		NavMesh.Initialize(application::file::game::ZStdBackend::Decompress(application::util::FileUtil::GetRomFSFilePath("Phive/StitchedNavMesh/" + FieldName + "/X16_Z12.Nin_NX_NVN.bphnm.zs")));
		NavMeshBinary = NavMesh.ToBinary();
		application::util::FileUtil::WriteFile(application::util::FileUtil::GetSaveFilePath("X16_Z12.Nin_NX_NVN.bphnm"), NavMeshBinary);

		std::filesystem::create_directories(application::util::FileUtil::GetSaveFilePath("Phive/StitchedNavMesh/" + FieldName));
		application::util::FileUtil::WriteFile(application::util::FileUtil::GetSaveFilePath("Phive/StitchedNavMesh/" + FieldName + "/X16_Z12.Nin_NX_NVN.bphnm.zs"), application::file::game::ZStdBackend::Compress(NavMeshBinary, application::file::game::ZStdBackend::Dictionary::Base));
		*/

		//std::pair<std::vector<glm::vec3>, std::vector<uint32_t>> ModelData = NavMesh.ToVerticesIndices();
		//mMeshes.push_back(application::gl::SimpleMesh(ModelData.first, ModelData.second));

		ReloadMeshes();

		mShader = application::manager::ShaderMgr::GetShader("NavMesh");

		return true;
	}

	void NavMeshImplementationFieldScene::ReloadMeshes()
	{
		mMeshes.clear();

		for (auto& NavMeshPair : mNavMeshes)
		{
			std::pair<std::vector<glm::vec3>, std::vector<uint32_t>> ModelData = NavMeshPair.second.ToVerticesIndices();

			if (mScene->GetSceneTypeName(mScene->mSceneType) == "MinusField")
			{
				for (glm::vec3& Vertex : ModelData.first)
				{
					Vertex.y -= 200.0f;
				}
			}

			mMeshes.push_back(application::gl::SimpleMesh(ModelData.first, ModelData.second));
		}
	}

	glm::vec3 NavMeshImplementationFieldScene::GetNavMeshMiddlePoint(uint8_t X, uint8_t Z)
	{
		glm::vec2 Result;
		Result.x = mScene->mBcettName[0] - 65;
		Result.y = ((char)mScene->mBcettName[2]) - '0' - 1;

		glm::vec2 Origin = glm::vec2((Result.x - 5) * 1000, (Result.y - 3) * 1000);
		Origin.y -= 1000.0f;

		Origin.x += X * 250.0f;
		Origin.y += Z * 250.0f;

		return glm::vec3(Origin.x + 125.0f, 0, Origin.y + 125.0f);
	}

	void NavMeshImplementationFieldScene::DrawNavMesh(application::gl::Camera& Camera)
	{
		if (mMeshes.empty())
			return;

		mShader->Bind();
		Camera.Matrix(mShader, "camMatrix");

		for (size_t i = 0; i < mMeshes.size(); i++)
		{
			glm::mat4 GLMModel = glm::mat4(1.0f);

			GLMModel = glm::translate(GLMModel, GetNavMeshMiddlePoint(i % 4, i / 4));

			glUniformMatrix4fv(glGetUniformLocation(mShader->mID, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(GLMModel));

			mMeshes[i].Draw();
		}
	}

	// Custom hash function for glm::vec3
	struct Vec3Hash {
		std::size_t operator()(const glm::vec3& v) const {
			// Combine hashes of x, y, z components
			std::size_t h1 = std::hash<float>{}(v.x);
			std::size_t h2 = std::hash<float>{}(v.y);
			std::size_t h3 = std::hash<float>{}(v.z);

			// Use bit shifting and XOR for good hash distribution
			return h1 ^ (h2 << 1) ^ (h3 << 2);
		}
	};

	// Custom equality function for glm::vec3 (handles floating point comparison)
	struct Vec3Equal {
		bool operator()(const glm::vec3& a, const glm::vec3& b) const {
			const float epsilon = 1e-6f; // Adjust tolerance as needed
			return std::abs(a.x - b.x) < epsilon &&
				std::abs(a.y - b.y) < epsilon &&
				std::abs(a.z - b.z) < epsilon;
		}
	};

	void NavMeshImplementationFieldScene::RemoveDuplicateVertices(application::file::game::phive::navmesh::PhiveNavMesh::GeneratorData& GeneratorData)
	{
		// Use unordered_map for O(1) average lookup time
		std::unordered_map<glm::vec3, uint32_t, Vec3Hash, Vec3Equal> vertexMap;
		std::vector<glm::vec3> newVertices;
		std::vector<uint32_t> newIndices;

		// Reserve space to avoid reallocations
		newVertices.reserve(GeneratorData.mVertices.size());
		newIndices.reserve(GeneratorData.mIndices.size());

		// Lambda to get or create vertex index
		auto getVertexIndex = [&](const glm::vec3& vertex) -> uint32_t {
			auto it = vertexMap.find(vertex);
			if (it != vertexMap.end()) {
				return it->second; // Return existing index
			}

			// Add new vertex
			uint32_t newIndex = static_cast<uint32_t>(newVertices.size());
			newVertices.push_back(vertex);
			vertexMap[vertex] = newIndex;
			return newIndex;
			};

		// Process all triangles
		for (size_t i = 0; i < GeneratorData.mIndices.size() / 3; i++) {
			uint32_t idA = getVertexIndex(GeneratorData.mVertices[GeneratorData.mIndices[i * 3]]);
			uint32_t idB = getVertexIndex(GeneratorData.mVertices[GeneratorData.mIndices[i * 3 + 1]]);
			uint32_t idC = getVertexIndex(GeneratorData.mVertices[GeneratorData.mIndices[i * 3 + 2]]);

			newIndices.push_back(idA);
			newIndices.push_back(idB);
			newIndices.push_back(idC);
		}

		// Update the original data
		newVertices.shrink_to_fit();
		GeneratorData.mVertices = std::move(newVertices);
		GeneratorData.mIndices = std::move(newIndices);
	}

	void NavMeshImplementationFieldScene::DrawImGuiGeneratorMenu()
	{
		ImGuiExt::InputFloatSliderCombo("Cell size", "NavMeshCellSize", &mGenerator.mGeometryConfig.mCellSize, 0.1f, 10.0f);
		ImGuiExt::InputFloatSliderCombo("Cell height", "NavMeshCellHeight", &mGenerator.mGeometryConfig.mCellHeight, 0.1f, 10.0f);
		ImGuiExt::InputFloatSliderCombo("Walkable slope angle", "NavMeshSlopeAngle", &mGenerator.mGeometryConfig.mWalkableSlopeAngle, 0.001f, 180.0f);
		ImGuiExt::InputFloatSliderCombo("Walkable height", "NavMeshHeight", &mGenerator.mGeometryConfig.mWalkableHeight, 0.001f, 10.0f);
		ImGuiExt::InputFloatSliderCombo("Walkable climb", "NavMeshClimb", &mGenerator.mGeometryConfig.mWalkableClimb, 0.001f, 10.0f);
		ImGuiExt::InputFloatSliderCombo("Walkable radius", "NavMeshRadius", &mGenerator.mGeometryConfig.mWalkableRadius, 0.001f, 10.0f);
		ImGuiExt::InputIntSliderCombo("Min Region Area", "NavMeshMinRegionArea", &mGenerator.mGeometryConfig.mMinRegionArea, 1, 256);
		ImGui::NewLine();
		ImGui::InputScalar("NavMesh Index (0-15)", ImGuiDataType_U32, &mGenerationNavMeshIndex);
		ImGui::Checkbox("Merging Mode", &mMergingMode);
		ImGui::Checkbox("Include Terrain Geometry", &mIncludeTerrainGeometry);

		ImGui::BeginDisabled();
		if (ImGui::Button("Generate"))
		{
			if (mScene->mTerrainRenderer == nullptr)
			{
				application::util::Logger::Error("NavMeshImplementationFieldScene", "Terrain renderer is null, cannot generate navmesh");
				return;
			}

			std::string FieldName = mScene->GetSceneTypeName(mScene->mSceneType);

			std::filesystem::create_directories(application::util::FileUtil::GetSaveFilePath("Phive/StitchedNavMesh/" + FieldName));

			auto& NavMeshPair = mNavMeshes[mGenerationNavMeshIndex];

			uint8_t XIndex = mGenerationNavMeshIndex % 4;
			uint8_t ZIndex = mGenerationNavMeshIndex / 4;

			glm::vec3 StartPoint = GetNavMeshMiddlePoint(XIndex, ZIndex);
			StartPoint.x -= 125.0f;
			StartPoint.z -= 125.0f;

			mGenerator.mIsSingleSceneNavMesh = false;
			mGenerator.mVertices.clear();
			mGenerator.mIndices.clear();

			if (mIncludeTerrainGeometry)
			{
				std::pair<std::vector<glm::vec3>, std::vector<std::pair<std::tuple<uint32_t, uint32_t, uint32_t>, uint32_t>>> Model = mScene->mTerrainRenderer->GenerateTerrainTileCollisionModel(glm::vec2(StartPoint.x, StartPoint.z), nullptr);

				mGenerator.mVertices.reserve(Model.first.size());
				for (const glm::vec3& Vertex : Model.first)
				{
					mGenerator.mVertices.emplace_back(Vertex.x - 125.0f, Vertex.y, Vertex.z - 125.0f);
				}

				mGenerator.mIndices.resize(Model.second.size() * 3);
				for (auto& [Triangle, Material] : Model.second)
				{
					mGenerator.mIndices.push_back(std::get<0>(Triangle));
					mGenerator.mIndices.push_back(std::get<1>(Triangle));
					mGenerator.mIndices.push_back(std::get<2>(Triangle));
				}
			}

			std::pair<std::vector<glm::vec3>, std::vector<unsigned int>> SceneModelData = mScene->GetSceneModel([](application::game::Scene::BancEntityRenderInfo* Info)
				{
					return Info->mEntity->mBakeable;
				});

			std::vector<std::pair<glm::vec3, glm::vec3>> ProhibitionBoxes;
			std::vector<std::pair<glm::vec3, glm::vec3>> VolumeBoxes;

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

					if (std::get<bool>(RenderInfo->mEntity->mDynamic["Starlight_NavMeshVolume"]))
						VolumeBoxes.push_back(std::make_pair(glm::vec3(MinPoint.x, MinPoint.y, MinPoint.z), glm::vec3(MaxPoint.x, MaxPoint.y, MaxPoint.z)));
					else
						ProhibitionBoxes.push_back(std::make_pair(glm::vec3(MinPoint.x, MinPoint.y, MinPoint.z), glm::vec3(MaxPoint.x, MaxPoint.y, MaxPoint.z)));
				}
			}

			StartPoint.y = -1000.0f;
			
			glm::vec3 EndPoint = StartPoint;
			EndPoint.x += 250.0f;
			EndPoint.y = 1000.0f;
			EndPoint.z += 250.0f;

			SceneModelData = application::util::Math::ClipMeshToBoxes(SceneModelData, VolumeBoxes);
			SceneModelData = application::util::Math::ClipMeshToBox(SceneModelData, std::make_pair(StartPoint, EndPoint));
			SceneModelData = application::util::Math::ClipOutsideMeshToBoxes(SceneModelData, ProhibitionBoxes);

			//Merging scene model into navmesh generation model
			uint32_t IndexOffset = mGenerator.mVertices.size();

			mGenerator.mVertices.reserve(mGenerator.mVertices.size() + SceneModelData.first.size());
			for (const glm::vec3& Pos : SceneModelData.first)
			{
				mGenerator.mVertices.emplace_back(Pos.x - StartPoint.x - 125.0f, Pos.y, Pos.z - StartPoint.z - 125.0f);
			}

			mGenerator.mIndices.reserve(mGenerator.mIndices.size() + SceneModelData.second.size());
			for (uint32_t Index : SceneModelData.second)
			{
				mGenerator.mIndices.push_back(Index + IndexOffset);
			}

			if (mScene->GetSceneTypeName(mScene->mSceneType) == "MinusField")
			{
				for (glm::vec3& Vertex : mGenerator.mVertices)
				{
					Vertex.y += 200.0f;
				}
			}

			if (mMergingMode)
			{
				std::pair<std::vector<glm::vec3>, std::vector<uint32_t>> MergingModelData = NavMeshPair.second.ToVerticesIndices();

				uint32_t IndexOffset = mGenerator.mVertices.size();

				mGenerator.mVertices.reserve(mGenerator.mVertices.size() + MergingModelData.first.size());
				for (const glm::vec3& Pos : MergingModelData.first)
				{
					mGenerator.mVertices.emplace_back(Pos);
				}

				mGenerator.mIndices.reserve(mGenerator.mIndices.size() + MergingModelData.second.size());
				for (uint32_t Index : MergingModelData.second)
				{
					mGenerator.mIndices.push_back(Index + IndexOffset);
				}

				//RemoveDuplicateVertices(mGenerator);
			}

			auto LoadNavMeshNeighbour = [this, &FieldName](const std::string& TileName, uint8_t Index)
				{
					if (!application::util::FileUtil::FileExists(application::util::FileUtil::GetRomFSFilePath("Phive/StitchedNavMesh/" + FieldName + "/" + TileName + ".Nin_NX_NVN.bphnm.zs")))
					{
						mGenerator.mNeighbours.at(Index).clear();
						mGenerator.mNeighbourNavMeshObjs.at(Index) = nullptr;
						return;
					}

					std::vector<unsigned char> PhiveNavMeshFileData = application::file::game::ZStdBackend::Decompress(application::util::FileUtil::GetRomFSFilePath("Phive/StitchedNavMesh/" + FieldName + "/" + TileName + ".Nin_NX_NVN.bphnm.zs"));

					application::util::BinaryVectorReader Reader(PhiveNavMeshFileData);
					Reader.Seek(24, application::util::BinaryVectorReader::Position::Begin);
					uint32_t TagFileSize = Reader.ReadUInt32();

					Reader.Seek(48, application::util::BinaryVectorReader::Position::Begin);

					std::vector<unsigned char> TagFileData;
					TagFileData.resize(TagFileSize);
					Reader.ReadStruct(&TagFileData[0], TagFileSize);

					std::unique_ptr<application::file::game::phive::navmesh::PhiveNavMesh> NavMeshNeighbour = std::make_unique< application::file::game::phive::navmesh::PhiveNavMesh>();
					NavMeshNeighbour->Initialize(PhiveNavMeshFileData);

					mGenerator.mNeighbours.at(Index) = TagFileData;
					mGenerator.mNeighbourNavMeshObjs.at(Index) = std::move(NavMeshNeighbour);
				};

			//Getting neighbours for this navmesh section
			LoadNavMeshNeighbour("X" + std::to_string(NavMeshPair.first.mXIndex - 1) + "_Z" + std::to_string(NavMeshPair.first.mZIndex), 0);
			LoadNavMeshNeighbour("X" + std::to_string(NavMeshPair.first.mXIndex + 1) + "_Z" + std::to_string(NavMeshPair.first.mZIndex), 1);
			LoadNavMeshNeighbour("X" + std::to_string(NavMeshPair.first.mXIndex) + "_Z" + std::to_string(NavMeshPair.first.mZIndex - 1), 2);
			LoadNavMeshNeighbour("X" + std::to_string(NavMeshPair.first.mXIndex) + "_Z" + std::to_string(NavMeshPair.first.mZIndex + 1), 3);

			NavMeshPair.second.InjectNavMeshModel(mGenerator);

			//std::vector<unsigned char> Binary = application::file::game::ZStdBackend::Decompress(application::util::FileUtil::GetRomFSFilePath("Phive/StitchedNavMesh/" + FieldName + "/" + NavMeshPair.first.GetSectionName() + ".Nin_NX_NVN.bphnm.zs"));

			std::pair<std::vector<glm::vec3>, std::vector<uint32_t>> ModelData = NavMeshPair.second.ToVerticesIndices();
			if (mScene->GetSceneTypeName(mScene->mSceneType) == "MinusField")
			{
				for (glm::vec3& Vertex : ModelData.first)
				{
					Vertex.y -= 200.0f;
				}
			}
			mMeshes[mGenerationNavMeshIndex] = application::gl::SimpleMesh(ModelData.first, ModelData.second);
			//mMeshes[mGenerationNavMeshIndex] = application::gl::SimpleMesh(mGenerator.mVertices, mGenerator.mIndices);

			application::util::FileUtil::WriteFile(application::util::FileUtil::GetSaveFilePath("Phive/StitchedNavMesh/" + FieldName + "/" + NavMeshPair.first.GetSectionName() + ".Nin_NX_NVN.bphnm.zs"), application::file::game::ZStdBackend::Compress(NavMeshPair.second.ToBinary(), application::file::game::ZStdBackend::Dictionary::Base));
				
			application::util::FileUtil::WriteFile(application::util::FileUtil::GetSaveFilePath("Phive/StitchedNavMesh/" + FieldName + "/" + "X" + std::to_string(NavMeshPair.first.mXIndex - 1) + "_Z" + std::to_string(NavMeshPair.first.mZIndex) + ".Nin_NX_NVN.bphnm.zs"), application::file::game::ZStdBackend::Compress(mGenerator.mNeighbourNavMeshObjs[0]->ToBinary(), application::file::game::ZStdBackend::Dictionary::Base));
			application::util::FileUtil::WriteFile(application::util::FileUtil::GetSaveFilePath("Phive/StitchedNavMesh/" + FieldName + "/" + "X" + std::to_string(NavMeshPair.first.mXIndex + 1) + "_Z" + std::to_string(NavMeshPair.first.mZIndex) + ".Nin_NX_NVN.bphnm.zs"), application::file::game::ZStdBackend::Compress(mGenerator.mNeighbourNavMeshObjs[1]->ToBinary(), application::file::game::ZStdBackend::Dictionary::Base));
			application::util::FileUtil::WriteFile(application::util::FileUtil::GetSaveFilePath("Phive/StitchedNavMesh/" + FieldName + "/" + "X" + std::to_string(NavMeshPair.first.mXIndex) + "_Z" + std::to_string(NavMeshPair.first.mZIndex - 1) + ".Nin_NX_NVN.bphnm.zs"), application::file::game::ZStdBackend::Compress(mGenerator.mNeighbourNavMeshObjs[2]->ToBinary(), application::file::game::ZStdBackend::Dictionary::Base));
			application::util::FileUtil::WriteFile(application::util::FileUtil::GetSaveFilePath("Phive/StitchedNavMesh/" + FieldName + "/" + "X" + std::to_string(NavMeshPair.first.mXIndex) + "_Z" + std::to_string(NavMeshPair.first.mZIndex + 1) + ".Nin_NX_NVN.bphnm.zs"), application::file::game::ZStdBackend::Compress(mGenerator.mNeighbourNavMeshObjs[3]->ToBinary(), application::file::game::ZStdBackend::Dictionary::Base));

			/*
			for (auto& NavMeshPair : mNavMeshes)
			{
				application::util::FileUtil::WriteFile(application::util::FileUtil::GetSaveFilePath("Phive/StitchedNavMesh/" + FieldName + "/" + NavMeshPair.first.GetSectionName() + ".Nin_NX_NVN.bphnm.zs"), application::file::game::ZStdBackend::Compress(NavMeshPair.second.ToBinary(), application::file::game::ZStdBackend::Dictionary::Base));
			}
			*/

			//ReloadMeshes();
		}
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
		{
			ImGui::SetTooltip("This button is disabled by default.\nThis feature does NOT work and will break the fields NavMesh.\nTo enable it, build Starlight from source and modify the code in tool/scene/navmesh/NavMeshImplementationFieldScene.cpp");
		}
		ImGui::EndDisabled();
	}

	bool NavMeshImplementationFieldScene::SupportNavMeshGeneration()
	{
		return true;
	}

	bool NavMeshImplementationFieldScene::SupportNavMeshViewing()
	{
		return !mMeshes.empty();
	}
}