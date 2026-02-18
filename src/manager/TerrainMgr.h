#pragma once

#include <unordered_map>
#include <string>
#include <file/game/terrain/HghtArchive.h>
#include <file/game/terrain/MateArchive.h>
#include <file/game/terrain/TerrainSceneFile.h>
#include <gl/TerrainRenderer.h>

namespace application::manager
{
	namespace TerrainMgr
	{
		struct TerrainScene
		{
			struct ArchivePack
			{
				application::file::game::terrain::HghtArchive mHghtArchive;
				application::file::game::terrain::MateArchive mMateArchive;
			};
			application::file::game::terrain::TerrainSceneFile mTerrainSceneFile;
			std::unordered_map<std::string, ArchivePack> mArchives;
		};

		extern std::unordered_map<std::string, TerrainScene> gTerrainScenes;
		extern std::unordered_map<std::string, application::gl::TerrainRenderer> gTerrainRenderers;

		TerrainScene* GetTerrainScene(const std::string& TerrainSceneName);
		TerrainMgr::TerrainScene::ArchivePack* GetArchivePack(std::string Name, TerrainMgr::TerrainScene& Scene);
		application::gl::TerrainRenderer* GetTerrainRenderer(const std::string& SceneName, const std::string& SectionName);
		std::string GenerateHghtNameForArea(application::manager::TerrainMgr::TerrainScene* Scene, application::file::game::terrain::TerrainSceneFile::ResArea* Area);
	}
};