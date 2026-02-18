#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <file/game/terrain/MateFile.h>
#include <file/game/terrain/TerrainSceneFile.h>

namespace application::file::game::terrain
{
	struct MateArchive
	{
		void Initialize(const std::string& Path, application::file::game::terrain::TerrainSceneFile* SceneFile);

		std::vector<unsigned char> ToBinary();

		MateArchive() = default;

		std::unordered_map<std::string, application::file::game::terrain::MateFile> mMateFiles;
	};
}