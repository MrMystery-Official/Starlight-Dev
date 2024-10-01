#include "SceneExporter.h"

#include <vector>
#include "Editor.h"
#include "ActorMgr.h"
#include "BinaryVectorWriter.h"
#include "Util.h"
#include <glm/glm.hpp>

SceneExporter::Settings SceneExporter::mSettings;

void SceneExporter::ExportScene()
{
	std::vector<std::vector<glm::vec3>> Vertices;
	std::vector<std::vector<uint32_t>> Indices;
	std::vector<std::string> Names;

	auto GenereateNavMeshModel = [&Names, &Vertices, &Indices](Actor& SceneActor)
		{
			if (SceneActor.Model->mBfres->mDefaultModel)
				return;

			if (!mSettings.mExportFarActors && SceneActor.Gyml.find("_Far") != std::string::npos)
				return;

			for (const std::string& Str : mSettings.mFilterActors)
			{
				if (SceneActor.Gyml.find(Str) != std::string::npos)
				{
					return;
				}
			}

			Vertices.resize(Vertices.size() + 1);
			Indices.resize(Indices.size() + 1);

			glm::mat4 GLMModel = glm::mat4(1.0f);  // Identity matrix

			GLMModel = glm::translate(GLMModel, glm::vec3(SceneActor.Translate.GetX(), SceneActor.Translate.GetY(), SceneActor.Translate.GetZ()));

			GLMModel = glm::rotate(GLMModel, glm::radians(SceneActor.Rotate.GetZ()), glm::vec3(0.0, 0.0f, 1.0));
			GLMModel = glm::rotate(GLMModel, glm::radians(SceneActor.Rotate.GetY()), glm::vec3(0.0f, 1.0, 0.0));
			GLMModel = glm::rotate(GLMModel, glm::radians(SceneActor.Rotate.GetX()), glm::vec3(1.0, 0.0f, 0.0));

			GLMModel = glm::scale(GLMModel, glm::vec3(SceneActor.Scale.GetX(), SceneActor.Scale.GetY(), SceneActor.Scale.GetZ()));

			BfresFile::Model& Model = SceneActor.Model->mBfres->Models.GetByIndex(0).Value;

			for (auto& [Key, Val] : Model.Shapes.Nodes)
			{
				uint32_t IndexOffset = Vertices.back().size();
				for (glm::vec4 Pos : Val.Value.Vertices)
				{
					glm::vec4 Vertex(Pos.x, Pos.y, Pos.z, 1.0f);
					Vertex = GLMModel * Vertex;
					Vertices.back().push_back(glm::vec3(Vertex.x, Vertex.y, Vertex.z));
				}
				for (uint32_t Index : Val.Value.Meshes[0].GetIndices())
				{
					Indices.back().push_back(Index + IndexOffset);
				}
			}

			Names.push_back(SceneActor.Gyml);
		};

	int32_t Index = -1;
	for (Actor& SceneActor : ActorMgr::GetActors())
	{
		Index++;
		GenereateNavMeshModel(SceneActor);
		for (Actor& ChildActor : SceneActor.MergedActorContent)
		{
			Index++;
			GenereateNavMeshModel(ChildActor);
		}
	}

	glm::vec3 SmallestVertex(
		std::numeric_limits<float>::max(),
		std::numeric_limits<float>::max(),
		std::numeric_limits<float>::max()
	);

	// Iterate through each vector and compare
	for (std::vector<glm::vec3>& Vec : Vertices) {
		for (glm::vec3 Vertex : Vec)
		{
			if (Vertex.x < SmallestVertex.x) SmallestVertex.x = Vertex.x;
			if (Vertex.y < SmallestVertex.y) SmallestVertex.y = Vertex.y;
			if (Vertex.z < SmallestVertex.z) SmallestVertex.z = Vertex.z;
		}
	}

	uint32_t ModelCount = 0;
	BinaryVectorWriter Writer;
	Writer.WriteInteger(Vertices.size(), sizeof(uint32_t));
	for (std::vector<glm::vec3>& Vec : Vertices)
	{
		Writer.WriteInteger(Vec.size(), sizeof(uint32_t));
		Writer.WriteInteger(Indices[ModelCount].size() / 3, sizeof(uint32_t));
		for (glm::vec3& Vertex : Vec)
		{
			if(mSettings.mAlignToZero)
				Vertex -= SmallestVertex;

			Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&Vertex.x), sizeof(float) * 3);
		}
		for (size_t i = 0; i < Indices[ModelCount].size() / 3; i++)
		{
			Writer.WriteInteger(Indices[ModelCount][i * 3], sizeof(uint32_t));
			Writer.WriteInteger(Indices[ModelCount][i * 3 + 1], sizeof(uint32_t));
			Writer.WriteInteger(Indices[ModelCount][i * 3 + 2], sizeof(uint32_t));
		}
		Writer.WriteInteger(Names[ModelCount].length(), sizeof(uint32_t));
		Writer.WriteRawUnsafeFixed(Names[ModelCount].c_str(), Names[ModelCount].length());
		ModelCount++;
	}

	Util::CreateDir(Editor::GetWorkingDirFile("SceneExports"));
	std::ofstream File(Editor::GetWorkingDirFile("SceneExports/" + Editor::Identifier + "_StarlightSceneModel.emdl"), std::ios::binary);
	std::vector<unsigned char> Binary = Writer.GetData();
	std::copy(Binary.cbegin(), Binary.cend(),
		std::ostream_iterator<unsigned char>(File));
	File.close();
}