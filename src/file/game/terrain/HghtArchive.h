#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <file/game/terrain/HghtFile.h>
#include <file/game/terrain/TerrainSceneFile.h>

namespace application::file::game::terrain
{
	struct HghtArchive
	{
		struct HeightMapModel
		{
			std::vector<glm::vec3> mVertices;
			std::vector<uint32_t> mIndices;
		};

		void Initialize(const std::string& Path);
		HeightMapModel GenerateHeightMapModel(application::file::game::terrain::HghtFile& File, float Scale, application::file::game::terrain::TerrainSceneFile::ResArea* Area, int Start, int End);
		HeightMapModel GenerateHeightMapModelQuads(application::file::game::terrain::HghtFile& File, float Scale, application::file::game::terrain::TerrainSceneFile::ResArea* Area);

		std::vector<unsigned char> ToBinary();

		HghtArchive() = default;

		std::unordered_map<std::string, application::file::game::terrain::HghtFile> mHeightMaps;
	 };
}