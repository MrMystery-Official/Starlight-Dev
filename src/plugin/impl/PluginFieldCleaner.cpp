#include <plugin/impl/PluginFieldCleaner.h>

#include <rendering/mapeditor/TerrainEditor.h>
#include <util/FileUtil.h>
#include <file/game/terrain/MateArchive.h>
#include <file/game/zstd/ZStdBackend.h>
#include <file/game/phive/PhiveStaticCompoundFile.h>
#include <file/game/byml/BymlFile.h>
#include <file/game/phive/navmesh/PhiveNavMesh.h>
#include <manager/TerrainMgr.h>

#include <imgui.h>
#include <imgui_stdlib.h>
#include <filesystem>
#include <meshoptimizer.h>

namespace application::plugin::impl
{
    void PluginFieldCleaner::DrawOptions()
    {
        ImGui::InputText("Field Scene Name (MainField or MinusField)", &mFieldSceneName);

        ImGui::NewLine();
        if (ImGui::Button("Execute"))
        {
            auto TerrainScene = application::manager::TerrainMgr::GetTerrainScene(mFieldSceneName);
            float HeightScale = TerrainScene->mTerrainSceneFile.mTerrainScene.mHeightScale / 0xFFFF;
            const float TerrainHeight = 120.0f;

            /*
            float MinMaxScale = (TerrainHeight / HeightScale) / 65535.0f;

            for (auto& Area : TerrainScene->mTerrainSceneFile.mTerrainScene.mAreas)
            {
                Area.mObj.mFileResources.erase(
                    std::remove_if(
                        Area.mObj.mFileResources.begin(),
                        Area.mObj.mFileResources.end(),
                        [](const application::file::game::terrain::TerrainSceneFile::RelPtr<application::file::game::terrain::TerrainSceneFile::ResFile>& FilePtr)
                        {
                            return FilePtr.mObj.mType == application::file::game::terrain::TerrainSceneFile::ResFileType::Grass ||
                                FilePtr.mObj.mType == application::file::game::terrain::TerrainSceneFile::ResFileType::Bake ||
                                FilePtr.mObj.mType == application::file::game::terrain::TerrainSceneFile::ResFileType::Water;
                        }
                    ),
                    Area.mObj.mFileResources.end()
                );

                Area.mObj.mFileCount = Area.mObj.mFileResources.size();

                for (auto& FileResource : Area.mObj.mFileResources)
                {
                    if (FileResource.mObj.mType == application::file::game::terrain::TerrainSceneFile::ResFileType::HeightMap)
                    {
                        FileResource.mObj.mMinHeight = MinMaxScale;
                        FileResource.mObj.mMaxHeight = MinMaxScale;
					}
                }
            }

            std::filesystem::create_directories(application::util::FileUtil::GetSaveFilePath("TerrainArc"));
            application::util::FileUtil::WriteFile(application::util::FileUtil::GetSaveFilePath("TerrainArc/" + mFieldSceneName + ".tscb"), TerrainScene->mTerrainSceneFile.ToBinary());
            */

            /*
            const std::string SuffixMate = ".mate.ta.zs";

            for (const auto& Entry : std::filesystem::directory_iterator(application::util::FileUtil::GetRomFSFilePath("TerrainArc/" + mFieldSceneName)))
            {
                if (Entry.is_regular_file())
                {
                    std::string Filename = Entry.path().filename().string();

                    if (Filename.size() >= SuffixMate.size() &&
                        Filename.substr(Filename.size() - SuffixMate.size()) == SuffixMate)
                    {
                        application::file::game::terrain::MateArchive MateArchive;
                        MateArchive.Initialize(Entry.path().string(), &TerrainScene->mTerrainSceneFile);

                        for (auto& [Name, Mate] : MateArchive.mMateFiles)
                        {
                            for (auto& Material : Mate.mMaterials)
                            {
                                Material.mBlendFactor = 0;
                                Material.mIndexA = static_cast<uint8_t>(0);
                                Material.mIndexB = static_cast<uint8_t>(0);
                            }
                        }

                        std::filesystem::create_directories(application::util::FileUtil::GetSaveFilePath("TerrainArc/" + mFieldSceneName));
                        application::util::FileUtil::WriteFile(application::util::FileUtil::GetSaveFilePath("TerrainArc/" + mFieldSceneName + "/" + Filename), application::file::game::ZStdBackend::Compress(MateArchive.ToBinary(), application::file::game::ZStdBackend::Dictionary::None));
                    }
                }
            }

            const std::string SuffixHght = ".hght.ta.zs";

            for (const auto& Entry : std::filesystem::directory_iterator(application::util::FileUtil::GetRomFSFilePath("TerrainArc/" + mFieldSceneName)))
            {
                if (Entry.is_regular_file())
                {
                    std::string Filename = Entry.path().filename().string();

                    if (Filename.size() >= SuffixHght.size() &&
                        Filename.substr(Filename.size() - SuffixHght.size()) == SuffixHght)
                    {
                        application::file::game::terrain::HghtArchive HghtArchive;
                        HghtArchive.Initialize(Entry.path().string());

                        for (auto& [Name, Hght] : HghtArchive.mHeightMaps)
                        {
                            Hght.mModified = true;
                            for (int x = 0; x < Hght.GetHeight(); x++)
                            {
                                for (int y = 0; y < Hght.GetWidth(); y++)
                                {
                                    Hght.mHeightMap[x + y * Hght.GetWidth()] = TerrainHeight / HeightScale;
                                }
                            }
                        }

                        std::filesystem::create_directories(application::util::FileUtil::GetSaveFilePath("TerrainArc/" + mFieldSceneName));
                        application::util::FileUtil::WriteFile(application::util::FileUtil::GetSaveFilePath("TerrainArc/" + mFieldSceneName + "/" + Filename), application::file::game::ZStdBackend::Compress(HghtArchive.ToBinary(), application::file::game::ZStdBackend::Dictionary::None));
                    }
                }
            }
            */

            /*
			application::file::game::byml::BymlFile TemplateBymlFileDynamic(application::file::game::ZStdBackend::Decompress(application::util::FileUtil::GetRomFSFilePath("Banc/MainField/A-1_Dynamic.bcett.byml.zs", false)));
			application::file::game::byml::BymlFile TemplateBymlFileStatic(application::file::game::ZStdBackend::Decompress(application::util::FileUtil::GetRomFSFilePath("Banc/MainField/A-1_Static.bcett.byml.zs", false)));
            TemplateBymlFileDynamic.GetNode("Actors")->GetChildren().clear();

            TemplateBymlFileStatic.GetNode("Actors")->GetChildren().clear();
            TemplateBymlFileStatic.GetNode("AiGroups")->GetChildren().clear();
            TemplateBymlFileStatic.GetNode("FarDeleteGroups")->GetChildren().clear();
            TemplateBymlFileStatic.GetNode("PhiveHeader")->GetChildren().clear();
            TemplateBymlFileStatic.GetNode("SimultaneousGroups")->GetChildren().clear();

            std::filesystem::create_directories(application::util::FileUtil::GetSaveFilePath("Banc/" + mFieldSceneName));

            for (int i = 0; i < 8; i++)
            {
                for (int k = 0; k < 10; k++)
                {
					std::string SectionName = std::string(1, static_cast<char>('A' + k)) + "-" + std::to_string(i + 1);

                    TemplateBymlFileDynamic.GetNode("FilePath")->SetValue<std::string>("Work/Banc/Scene/" + mFieldSceneName + "/" + SectionName + ".bcett.json");
                    application::util::FileUtil::WriteFile(application::util::FileUtil::GetSaveFilePath("Banc/" + mFieldSceneName + "/" + SectionName + "_Dynamic.bcett.byml.zs"), application::file::game::ZStdBackend::Compress(TemplateBymlFileDynamic.ToBinary(application::file::game::byml::BymlFile::TableGeneration::AUTO), application::file::game::ZStdBackend::Dictionary::BcettByaml));

                    TemplateBymlFileStatic.GetNode("FilePath")->SetValue<std::string>("Work/Banc/Scene/" + mFieldSceneName + "/" + SectionName + ".bcett.json");
                    application::util::FileUtil::WriteFile(application::util::FileUtil::GetSaveFilePath("Banc/" + mFieldSceneName + "/" + SectionName + "_Static.bcett.byml.zs"), application::file::game::ZStdBackend::Compress(TemplateBymlFileStatic.ToBinary(application::file::game::byml::BymlFile::TableGeneration::AUTO), application::file::game::ZStdBackend::Dictionary::BcettByaml));
                }
            }
            */

/*
            application::file::game::phive::shape::PhiveShape::GeneratorData Generator;

            std::vector<glm::vec3> ModelVertices;
            std::vector<std::pair<std::tuple<uint32_t, uint32_t, uint32_t>, uint32_t>> ModelIndices;

            const float QuadSize = 5.02f;
            const int Dimensions = 251;
            int QuadCount = Dimensions / QuadSize;
            const int MarginQuads = 1;

            QuadCount += MarginQuads * 2;
            ModelVertices.resize(QuadCount * QuadCount);

            for (int z = 0; z < QuadCount; z++)
            {
                for (int x = 0; x < QuadCount; x++)
                {
                    glm::vec3& Vertex = ModelVertices[x + z * QuadCount];
                    Vertex.x = x * QuadSize - MarginQuads * QuadSize;
                    Vertex.y = TerrainHeight;
                    Vertex.z = z * QuadSize - MarginQuads * QuadSize;
                }
            }

            ModelIndices.reserve(((QuadCount - 1) * (QuadCount - 1) * 6) / 3);
            for (uint16_t z = 0; z < QuadCount - 1; z++)
            {
                for (uint16_t x = 0; x < QuadCount - 1; x++)
                {
                    uint32_t topLeft = z * QuadCount + x;
                    uint32_t topRight = topLeft + 1;
                    uint32_t bottomLeft = (z + 1) * QuadCount + x;
                    uint32_t bottomRight = bottomLeft + 1;

                    ModelIndices.push_back(std::make_pair(std::make_tuple(topLeft, bottomLeft, topRight), 0));
                    ModelIndices.push_back(std::make_pair(std::make_tuple(topRight, bottomLeft, bottomRight), 0));
                }
            }

            std::vector<uint32_t> SimplifiedIndices;
			std::vector<uint32_t> ModelIndicesFlat;
            for (auto& [Face, Material] : ModelIndices)
            {
				ModelIndicesFlat.push_back(std::get<0>(Face));
				ModelIndicesFlat.push_back(std::get<1>(Face));
				ModelIndicesFlat.push_back(std::get<2>(Face));
            }

                float ResultError = 0.0f;
                SimplifiedIndices.resize(ModelIndicesFlat.size());

                size_t original_triangle_count = ModelIndicesFlat.size() / 3;

                // Target: reduce to ~50% of original triangles, but never below 6 indices (2 triangles)
                size_t target_index_count = std::max(ModelIndicesFlat.size() / 2, static_cast<size_t>(6));

                // Use a reasonable error threshold (1% of bounding box diagonal)
                glm::vec3 bboxMin = ModelVertices[0];
                glm::vec3 bboxMax = ModelVertices[0];
                for (const auto& vertex : ModelVertices) 
                {
                    bboxMin = glm::min(bboxMin, vertex);
                    bboxMax = glm::max(bboxMax, vertex);
                }
                float bboxDiagonal = glm::length(bboxMax - bboxMin);
                float target_error = bboxDiagonal * 0.01f; // 1% of diagonal

                size_t simplified_count = meshopt_simplify(
                    &SimplifiedIndices[0],
                    &ModelIndicesFlat[0],
                    ModelIndicesFlat.size(),
                    &ModelVertices[0].x,
                    ModelVertices.size(),
                    sizeof(glm::vec3),
                    target_index_count,
                    target_error,
                    meshopt_SimplifyLockBorder,
                    &ResultError
                );

                // Check if simplification failed or removed too much
                if (simplified_count < 6) 
                {
                    // Simplification was too aggressive or failed, keep original
                    SimplifiedIndices = ModelIndicesFlat;
                }
                else 
                {
                    SimplifiedIndices.resize(simplified_count);
                }

                application::util::Logger::Info("TerrainMgr", "Reduced collision mesh from %u to %u with an error of %f", ModelIndicesFlat.size(), SimplifiedIndices.size(), ResultError);

            ModelIndices.clear();
            for (size_t i = 0; i < SimplifiedIndices.size(); i += 3)
            {
                ModelIndices.push_back(std::make_pair(std::make_tuple(SimplifiedIndices[i], SimplifiedIndices[i + 1], SimplifiedIndices[i + 2]), 0));
            }

			application::file::game::phive::util::PhiveMaterialData::PhiveMaterial DefaultMaterial;
            DefaultMaterial.mMaterialId = 2; //Grass
            DefaultMaterial.mCollisionFlags[0] = true;
            Generator.mMaterials.push_back(DefaultMaterial);

            Generator.mVertices = ModelVertices;
            Generator.mIndices = ModelIndices;

            Generator.mRunMeshOptimizer = false;

            std::optional<application::file::game::phive::shape::PhiveShape> TerrainShape = std::nullopt;

            std::filesystem::create_directories(application::util::FileUtil::GetSaveFilePath("Phive/StaticCompoundBody/" + mFieldSceneName));

            application::file::game::phive::PhiveStaticCompoundFile TemplateCompound(application::file::game::ZStdBackend::Decompress(application::util::FileUtil::GetRomFSFilePath("Phive/StaticCompoundBody/" + mFieldSceneName + "/" + mFieldSceneName + "_E-4_X3_Z3.Nin_NX_NVN.bphsc.zs")));
            TemplateCompound.mExternalMeshArray.resize(1);
            TemplateCompound.mExternalBphshMeshes.resize(1);

            for (auto& Mesh : TemplateCompound.mExternalBphshMeshes)
            {
                Mesh.mReserializeCollision = true;

                if (!TerrainShape.has_value())
                {
                    TerrainShape = Mesh.mPhiveShape;

                    TerrainShape->InjectModel(Generator);
                }

                Mesh.mPhiveShape = TerrainShape.value();
            }

            TemplateCompound.mActorLinks.resize(1);

            TemplateCompound.mActorLinks[0].mBaseIndex = 0xFFFFFFFF;
            TemplateCompound.mActorLinks[0].mEndIndex = 0xFFFFFFFF;
            TemplateCompound.mActorLinks[0].mCompoundRigidBodyIndex = 0;
            TemplateCompound.mActorLinks[0].mIsValid = 0;
            TemplateCompound.mActorLinks[0].mReserve0 = 0x12345678; //Some random SRT Hash :-)
            TemplateCompound.mActorLinks[0].mReserve1 = 0;
            TemplateCompound.mActorLinks[0].mReserve2 = 0;
            TemplateCompound.mActorLinks[0].mReserve3 = 0;

			TemplateCompound.mActorHashMap.clear();

            TemplateCompound.mWaterBoxArray.clear();
            TemplateCompound.mWaterCylinderArray.clear();
            TemplateCompound.mWaterFallArray.clear();

            TemplateCompound.mCompoundTagFile.clear();
            TemplateCompound.mImage.mRigidBodyEntryEntityCount = 0;
            TemplateCompound.mImage.mRigidBodyEntrySensorCount = 0;

            for (auto& Num : TemplateCompound.mTeraWaterSubTerrain0)
            {
                Num = 0;
            }

            for (auto& Num : TemplateCompound.mTeraWaterSubTerrain1)
            {
                Num = 0;
            }

            for (auto& Num : TemplateCompound.mTeraWaterTerrain)
            {
                Num.mReserve0 = 140739635838977;
            }

            TemplateCompound.mFixedOffset0Array.resize(1);
            TemplateCompound.mFixedOffset0Array[0].mReserve0 = 0;
            TemplateCompound.mFixedOffset0Array[0].mReserve1 = 0;
            TemplateCompound.mFixedOffset0Array[0].mReserve2 = 8;
            TemplateCompound.mFixedOffset0Array[0].mReserve3 = 12;
            TemplateCompound.mFixedOffset0Array[0].mStackIndex = 0;
            TemplateCompound.mFixedOffset0Array[0].mReserve4 = 0;
            TemplateCompound.mFixedOffset0Array[0].mInstanceIndex = 0;
            TemplateCompound.mFixedOffset0Array[0].mReserve5 = 0;
            TemplateCompound.mFixedOffset0Array[0].mReserve6 = 1;

            TemplateCompound.mFixedOffset1Array.resize(1);
            TemplateCompound.mFixedOffset1Array[0].mReserve0 = 35;
            TemplateCompound.mFixedOffset1Array[0].mReserve1 = 0;
            TemplateCompound.mFixedOffset1Array[0].mReserve2 = 32769;
            TemplateCompound.mFixedOffset1Array[0].mReserve3 = 128;
            TemplateCompound.mFixedOffset1Array[0].mReserve4 = 0;

            TemplateCompound.mFixedOffset2Array.resize(1);
			TemplateCompound.mFixedOffset2Array[0].mReserve0 = 46546943;
			TemplateCompound.mFixedOffset2Array[0].mReserve0 = 4294958591;

            uint64_t BaseHash = 100000000;

            for (int i = 0; i < 8; i++)
            {
                for (int k = 0; k < 10; k++)
                {
                    float StartX = -5000.0f + k * 1000.0f;
                    float StartZ = -4000.0f + i * 1000.0f;

                    std::string SectionName = std::string(1, static_cast<char>('A' + k)) + "-" + std::to_string(i + 1);

                    for (int x = 0; x < 4; x++)
                    {
                        for (int z = 0; z < 4; z++)
                        {
                            uint64_t Hash = BaseHash;
                            uint32_t OptimalIndex = 0;

                            uint64_t Modulus = (0 + 1) * 2;
                            uint64_t Remainder = Hash % Modulus;
                            uint64_t Difference = (OptimalIndex >= Remainder) ? (OptimalIndex - Remainder) : (Modulus - Remainder + OptimalIndex);
                            Hash += Difference;

							TemplateCompound.mActorLinks[0].mActorHash = Hash;

							TemplateCompound.mExternalMeshArray[0].mPositionX = StartX + x * 250.0f;
							TemplateCompound.mExternalMeshArray[0].mPositionY = 0.0f;
							TemplateCompound.mExternalMeshArray[0].mPositionZ = StartZ + z * 250.0f;

                            application::util::FileUtil::WriteFile(application::util::FileUtil::GetSaveFilePath("Phive/StaticCompoundBody/" + mFieldSceneName + "/" + mFieldSceneName + "_" + SectionName + "_X" + std::to_string(x) + "_Z" + std::to_string(z) + ".Nin_NX_NVN.bphsc.zs"), application::file::game::ZStdBackend::Compress(TemplateCompound.ToBinary(), application::file::game::ZStdBackend::Dictionary::Base));
                            
                            BaseHash += 100000000;
                        }
                    }
                }
            }
            */
        }
    }

    std::string PluginFieldCleaner::GetName() const
    {
        return "Field Cleaner";
    }
    std::string PluginFieldCleaner::GetAuthor() const
    {
        return "MrMystery";
    }
    std::string PluginFieldCleaner::GetDescription() const
    {
        return "A utility plugin to reset entire field scenes. (Use with caution, ! DISCONTINUED ! )";
    }
    uint32_t PluginFieldCleaner::GetVersion() const
    {
        return 1;
    }
}