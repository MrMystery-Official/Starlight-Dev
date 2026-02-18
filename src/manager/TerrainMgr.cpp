#include <manager/TerrainMgr.h>

#include <util/FileUtil.h>
#include <sstream>
#include <iomanip>

namespace application::manager
{
	std::unordered_map<std::string, TerrainMgr::TerrainScene> TerrainMgr::gTerrainScenes;
    std::unordered_map<std::string, application::gl::TerrainRenderer> TerrainMgr::gTerrainRenderers;

	TerrainMgr::TerrainScene* TerrainMgr::GetTerrainScene(const std::string& TerrainSceneName)
	{
		if (!gTerrainScenes.contains(TerrainSceneName))
		{
			gTerrainScenes[TerrainSceneName].mTerrainSceneFile.Initialize(application::util::FileUtil::ReadFile(application::util::FileUtil::GetRomFSFilePath("TerrainArc/" + TerrainSceneName + ".tscb")));
		}

		return &gTerrainScenes[TerrainSceneName];
	}

    TerrainMgr::TerrainScene::ArchivePack* TerrainMgr::GetArchivePack(std::string Name, TerrainMgr::TerrainScene& Scene)
    {
        std::string TerrainSceneName = "";
        for (auto& [Name, TerrainScene] : gTerrainScenes)
        {
            if (&TerrainScene == &Scene)
            {
                TerrainSceneName = Name;
                break;
            }
        }

        const uint64_t NameInt = std::floor(std::stoull(Name, nullptr, 16) / 4) * 4;

        std::stringstream Stream;
        Stream << std::setfill('0') << std::setw(16) << std::uppercase
            << std::hex << NameInt;
        Name = Stream.str();
        Name = Name.substr(Name.length() - 9);

        if (!Scene.mArchives.contains(Name))
        {
            Scene.mArchives[Name].mHghtArchive.Initialize(application::util::FileUtil::GetRomFSFilePath("TerrainArc/" + TerrainSceneName + "/" + Name + ".hght.ta.zs"));
            Scene.mArchives[Name].mMateArchive.Initialize(application::util::FileUtil::GetRomFSFilePath("TerrainArc/" + TerrainSceneName + "/" + Name + ".mate.ta.zs"), &Scene.mTerrainSceneFile);
        }
        return &Scene.mArchives[Name];
    }

	application::gl::TerrainRenderer* TerrainMgr::GetTerrainRenderer(const std::string& SceneName, const std::string& SectionName)
    {
        std::string Key = SceneName + "_" + SectionName;
        if (!gTerrainRenderers.contains(Key))
        {
            gTerrainRenderers[Key].LoadData(SceneName, SectionName);
        }
        return &gTerrainRenderers[Key];
	}

    std::string TerrainMgr::GenerateHghtNameForArea(application::manager::TerrainMgr::TerrainScene* Scene, application::file::game::terrain::TerrainSceneFile::ResArea* Area)
    {
        float LODLevel = Scene->mTerrainSceneFile.mTerrainScene.mAreas[0].mObj.mScale;
        int LODCount = 0;
        while (LODLevel != Area->mScale)
        {
            LODLevel = LODLevel / 2.0f;
            LODCount++;
        }

        unsigned int Mask = 0;
        unsigned int Shift = 0;

        application::file::game::terrain::TerrainSceneFile::ResArea* CurrentArea = Area;
        while (CurrentArea != nullptr)
        {
            float HigherLOD = CurrentArea->mScale * 2.0f;
            if (HigherLOD != Scene->mTerrainSceneFile.mTerrainScene.mAreas[0].mObj.mScale)
            {
                glm::vec2 MiddlePointCurrent(CurrentArea->mX * 1000.0f * 0.5f, CurrentArea->mZ * 1000.0f * 0.5f);

                application::file::game::terrain::TerrainSceneFile::ResArea* ParentArea = nullptr;

                for (auto& PossibleParentAreaPtr : Scene->mTerrainSceneFile.mTerrainScene.mAreas)
                {
                    application::file::game::terrain::TerrainSceneFile::ResArea& PossibleParentArea = PossibleParentAreaPtr.mObj;

                    if (PossibleParentArea.mScale != HigherLOD)
                        continue;

                    const float TileSectionWidthHalf = 250.0f * PossibleParentArea.mScale;
                    glm::vec2 MiddlePointParent(PossibleParentArea.mX * 1000.0f * 0.5f, PossibleParentArea.mZ * 1000.0f * 0.5f);

                    if (MiddlePointCurrent.x > MiddlePointParent.x - TileSectionWidthHalf && MiddlePointCurrent.x < MiddlePointParent.x + TileSectionWidthHalf &&
                        MiddlePointCurrent.y > MiddlePointParent.y - TileSectionWidthHalf && MiddlePointCurrent.y < MiddlePointParent.y + TileSectionWidthHalf)
                    {
                        ParentArea = &PossibleParentArea;
                        break;
                    }
                }

                if (ParentArea == nullptr)
                {
                    application::util::Logger::Error("TerrainMgr", "Failed to generate HGHT name, ParentArea was a nullptr");
                    break;
                }

                glm::vec2 MiddlePointParent(ParentArea->mX * 1000.0f * 0.5f, ParentArea->mZ * 1000.0f * 0.5f);

                enum AreaPosType
                {
                    TopLeft = 0,
                    TopRight = 1,
                    BottomLeft = 2,
                    BottomRight = 3
                };

                AreaPosType Type = AreaPosType::TopLeft;
                bool Left = MiddlePointCurrent.x < MiddlePointParent.x;
                bool Top = MiddlePointCurrent.y < MiddlePointParent.y;

                if (Top && Left) Type = AreaPosType::TopLeft;
                else if (!Top && Left) Type = AreaPosType::BottomLeft;
                else if (!Top && !Left) Type = AreaPosType::BottomRight;
                else if (Top && !Left) Type = AreaPosType::TopRight;

                Mask |= (Type << Shift);
                Shift += 2;

                CurrentArea = ParentArea;
            }
            else
            {
                Mask |= (0 << Shift);
                break;
            }
        }

        std::string Filename;
        Filename.resize(10);
        snprintf(&Filename[0], 11, "%X%X%08X", (int)log2f(Scene->mTerrainSceneFile.mTerrainScene.mAreaScale), LODCount, Mask);

        return Filename;
    }
}