#include "HghtArchive.h"

#include <file/game/zstd/ZStdBackend.h>
#include <file/game/sarc/SarcFile.h>
#include <util/FileUtil.h>
#include <util/General.h>

namespace application::file::game::terrain
{
	void HghtArchive::Initialize(const std::string& Path)
	{
		application::file::game::SarcFile Archive(application::file::game::ZStdBackend::Decompress(application::util::FileUtil::ReadFile(Path)));

		for (application::file::game::SarcFile::Entry& File : Archive.GetEntries())
		{
			std::string Key = File.mName;
			application::util::General::ReplaceString(Key, ".hght", "");

			mHeightMaps.insert({ Key, application::file::game::terrain::HghtFile(File.mBytes) });
		}
	}

	HghtArchive::HeightMapModel HghtArchive::GenerateHeightMapModel(application::file::game::terrain::HghtFile& File, float Scale, application::file::game::terrain::TerrainSceneFile::ResArea* Area, int Start, int End)
	{
		HghtArchive::HeightMapModel Model;

		for (float y = Start; y < End; y++)
		{
			float normY = y - (End - Start) / 2.0f;
			for (float x = Start; x < End; x++)
			{
				float HeightValue = File.GetHeightAtGridPos(x, y) * 0.0122072175174f;
				Model.mVertices.push_back(glm::vec3((x - (End - Start) / 2.0f) * (Scale / 255.0f), HeightValue, normY * (Scale / 255.0f)));
			}
		}

		for (glm::vec3& Vertex : Model.mVertices)
		{
			Vertex.x += Area->mX * 1000.0f * 0.5f;
			Vertex.z += Area->mZ * 1000.0f * 0.5f;
		}

		Model.mIndices.reserve(Model.mVertices.size() + (End - Start) * (End - Start) * 6);
		for (uint16_t z = 0; z < (End - Start) - 1; z++)
		{
			for (uint16_t x = 0; x < (End - Start) - 1; x++)
			{
				uint32_t topLeft = z * (End - Start) + x;
				uint32_t topRight = topLeft + 1;
				uint32_t bottomLeft = (z + 1) * (End - Start) + x;
				uint32_t bottomRight = bottomLeft + 1;

				Model.mIndices.push_back(topLeft);
				Model.mIndices.push_back(bottomLeft);
				Model.mIndices.push_back(topRight);
				Model.mIndices.push_back(topRight);
				Model.mIndices.push_back(bottomLeft);
				Model.mIndices.push_back(bottomRight);
			}
		}

		return Model;
	}

	HghtArchive::HeightMapModel HghtArchive::GenerateHeightMapModelQuads(application::file::game::terrain::HghtFile& File, float Scale, application::file::game::terrain::TerrainSceneFile::ResArea* Area)
	{
		HghtArchive::HeightMapModel Model;

		for (float y = 1; y < 259.0f; y++)
		{
			float normY = y - 129.5f;
			for (float x = 1; x < 259.0f; x++)
			{
				float HeightValue = File.GetHeightAtGridPos(x, y) * 0.0122072175174f;
				Model.mVertices.push_back(glm::vec3((x - 129.5f) * (Scale / 255.0f), HeightValue, normY * (Scale / 255.0f)));
			}
		}

		for (glm::vec3& Vertex : Model.mVertices)
		{
			Vertex.x += Area->mX * 1000.0f * 0.5f;
			Vertex.z += Area->mZ * 1000.0f * 0.5f;
		}

		Model.mIndices.reserve(Model.mVertices.size() + (File.GetWidth() - 2) * (File.GetHeight() - 2) * 6);
		for (uint16_t z = 0; z < File.GetHeight() - 1 - 2; z++)
		{
			for (uint16_t x = 0; x < File.GetWidth() - 1 - 2; x++)
			{
				uint32_t topLeft = z * (File.GetHeight() - 2) + x;
				uint32_t topRight = topLeft + 1;
				uint32_t bottomLeft = (z + 1) * (File.GetWidth() - 2) + x;
				uint32_t bottomRight = bottomLeft + 1;

				Model.mIndices.push_back(bottomLeft);
				Model.mIndices.push_back(topLeft);
				Model.mIndices.push_back(bottomRight);
				Model.mIndices.push_back(topRight);
			}
		}

		return Model;
	}

	std::vector<unsigned char> HghtArchive::ToBinary()
	{
		application::file::game::SarcFile Pack;
		
		for (auto& [Name, File] : mHeightMaps)
		{
			application::file::game::SarcFile::Entry Entry;
			Entry.mName = Name + ".hght";
			Entry.mBytes = File.ToBinary();
			File.mModified = false;
			Pack.GetEntries().push_back(Entry);
		}

		return Pack.ToBinary();
	}
}