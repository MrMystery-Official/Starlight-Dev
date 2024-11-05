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
	std::vector<std::vector<glm::vec3>> Vertices; //Vertex & UV
	std::vector<std::vector<uint32_t>> Indices;
	std::vector<Actor*> Actors;

	auto GenereateNavMeshModel = [&Actors, &Vertices, &Indices](Actor& SceneActor)
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

			BfresFile::Model& Model = SceneActor.Model->mBfres->Models.GetByIndex(0).Value;

			for (auto& [Key, Val] : Model.Shapes.Nodes)
			{
				uint32_t IndexOffset = Vertices.back().size();
				for (glm::vec4 Pos : Val.Value.Vertices)
				{
					Vertices.back().push_back(glm::vec3(Pos.x, Pos.y, Pos.z));
				}
				for (uint32_t Index : Val.Value.Meshes[0].GetIndices())
				{
					Indices.back().push_back(Index + IndexOffset);
				}
			}

			Actors.push_back(&SceneActor);
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

	glm::vec3 SmallestPos(
		std::numeric_limits<float>::max(),
		std::numeric_limits<float>::max(),
		std::numeric_limits<float>::max()
	);

	// Iterate through each vector and compare
	for (uint32_t i = 0; i < Vertices.size(); i++) {

		if (Actors[i]->Translate.GetX() < SmallestPos.x) SmallestPos.x = Actors[i]->Translate.GetX();
		if (Actors[i]->Translate.GetY() < SmallestPos.y) SmallestPos.y = Actors[i]->Translate.GetY();
		if (Actors[i]->Translate.GetZ() < SmallestPos.z) SmallestPos.z = Actors[i]->Translate.GetZ();
	}

	uint32_t ModelCount = 0;
	BinaryVectorWriter Writer;
	Writer.WriteInteger(Vertices.size(), sizeof(uint32_t));
	for (std::vector<glm::vec3>& Vec : Vertices)
	{
		Writer.WriteInteger(Vec.size(), sizeof(uint32_t));
		Writer.WriteInteger(Indices[ModelCount].size() / 3, sizeof(uint32_t));
		float Pos[] = { 
			Actors[ModelCount]->Translate.GetX() - SmallestPos.x, 
			Actors[ModelCount]->Translate.GetY() - SmallestPos.y, 
			Actors[ModelCount]->Translate.GetZ() - SmallestPos.z
		};
		Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(Pos), sizeof(float) * 3);
		Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(Actors[ModelCount]->Rotate.GetRawData()), sizeof(float) * 3);
		Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(Actors[ModelCount]->Scale.GetRawData()), sizeof(float) * 3);
		for (glm::vec3& Vertex : Vec)
		{
			Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&Vertex.x), sizeof(float) * 3);
		}
		for (size_t i = 0; i < Indices[ModelCount].size() / 3; i++)
		{
			Writer.WriteInteger(Indices[ModelCount][i * 3], sizeof(uint32_t));
			Writer.WriteInteger(Indices[ModelCount][i * 3 + 1], sizeof(uint32_t));
			Writer.WriteInteger(Indices[ModelCount][i * 3 + 2], sizeof(uint32_t));
		}
		Writer.WriteInteger(Actors[ModelCount]->Gyml.length(), sizeof(uint32_t));
		Writer.WriteRawUnsafeFixed(Actors[ModelCount]->Gyml.c_str(), Actors[ModelCount]->Gyml.length());
		ModelCount++;
	}

	Util::CreateDir(Editor::GetWorkingDirFile("SceneExports"));
	std::ofstream File(Editor::GetWorkingDirFile("SceneExports/" + Editor::Identifier + "_StarlightSceneModel.emdl"), std::ios::binary);
	std::vector<unsigned char> Binary = Writer.GetData();
	std::copy(Binary.cbegin(), Binary.cend(),
		std::ostream_iterator<unsigned char>(File));
	File.close();
}