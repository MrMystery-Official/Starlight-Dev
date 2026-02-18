#include <plugin/impl/PluginTerrainModificationUtil.h>

#include <rendering/mapeditor/TerrainEditor.h>
#include <util/FileUtil.h>
#include <file/game/terrain/MateArchive.h>
#include <file/game/zstd/ZStdBackend.h>
#include <file/game/phive/PhiveStaticCompoundFile.h>
#include <manager/TerrainMgr.h>

#include <imgui.h>
#include <imgui_stdlib.h>
#include <filesystem>

namespace application::plugin::impl
{
	void PluginTerrainModificationUtil::DrawOptions()
	{
        application::rendering::map_editor::TerrainEditor::InitializeTerrainTextures();

		ImGui::InputText("Field Scene Name (MainField or MinusField)", &mFieldSceneName);

        ImGui::Checkbox("Purge Grass Data", &mPurgeGrass);
		ImGui::Checkbox("Modify Material Maps", &mModifyMaterialMaps);

		ImGui::BeginDisabled(!mModifyMaterialMaps);

        constexpr static int Columns = 4;

        ImGui::Text("Brush Texture:");
        ImGui::SameLine();

        ImGui::PushID("TextureSelector");

        // Display currently selected texture as a clickable button
        ImVec2 uv0(0.0f, 0.0f);
        ImVec2 uv1(1.0f, 1.0f);

        bool clicked = false;

        clicked = ImGui::ImageButton("CurrentTexture",
            (ImTextureID)(intptr_t)application::rendering::map_editor::TerrainEditor::gTerrainTextureArrayViews[mCurrentTextureLayer],
            ImVec2(256, 256),
            uv0, uv1,
            ImVec4(0.2f, 0.2f, 0.2f, 1.0f),  // background
            ImVec4(1.0f, 1.0f, 1.0f, 1.0f)   // tint
        );

        // Show texture index tooltip
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("Texture Index: %d", mCurrentTextureLayer);
            ImGui::Text("Click to select different texture");
            ImGui::EndTooltip();
        }

        // Show dropdown when clicked
        if (clicked) {
            ImGui::OpenPopup("TextureSelectionPopup");
        }

        // Render the popup with all textures
        if (ImGui::BeginPopup("TextureSelectionPopup")) {
            ImGui::Text("Select Texture");
            ImGui::Separator();

            // Create scrollable region for the texture grid
            float windowWidth = Columns * (256 + 10.0f) + 20.0f;
            ImGui::BeginChild("TextureGrid", ImVec2(windowWidth, 400), true);

            for (int i = 0; i < 121; i++) {
                ImGui::PushID(i);

                // Calculate position in grid
                if (i > 0 && i % Columns != 0) {
                    ImGui::SameLine();
                }

                // Highlight selected texture
                bool isSelected = (i == mCurrentTextureLayer);
                if (isSelected) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.6f, 0.9f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.5f, 0.7f, 1.0f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.5f, 0.8f, 1.0f));
                }

                bool clickedNewTexture = false;

                clickedNewTexture = ImGui::ImageButton(("TextureTerrainSelector_" + std::to_string(i)).c_str(),
                    (ImTextureID)(intptr_t)application::rendering::map_editor::TerrainEditor::gTerrainTextureArrayViews[i],
                    ImVec2(256, 256),
                    uv0, uv1,
                    ImVec4(0.15f, 0.15f, 0.15f, 1.0f),  // background
                    ImVec4(1.0f, 1.0f, 1.0f, 1.0f)      // tint
                );

                // Render texture button using the texture view ID
                if (clickedNewTexture) {
                    mCurrentTextureLayer = i;
                    ImGui::CloseCurrentPopup();
                }

                if (isSelected) {
                    ImGui::PopStyleColor(3);
                }

                // Show index on hover
                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::Text("Index: %d", i);
                    ImGui::EndTooltip();
                }

                ImGui::PopID();
            }

            ImGui::EndChild();
            ImGui::EndPopup();
        }

        ImGui::PopID();

		ImGui::EndDisabled();

        ImGui::NewLine();
        if (ImGui::Button("Execute"))
        {
            auto TerrainScene = application::manager::TerrainMgr::GetTerrainScene(mFieldSceneName);

            if (mModifyMaterialMaps)
            {
                const std::string Suffix = ".mate.ta.zs";

                uint32_t MaterialId = application::gl::TerrainRenderer::gMaterialAlbToPhiveMaterialIndex.contains(mCurrentTextureLayer) ? application::gl::TerrainRenderer::gMaterialAlbToPhiveMaterialIndex[mCurrentTextureLayer] : 7;

                for (const auto& Entry : std::filesystem::directory_iterator(application::util::FileUtil::GetRomFSFilePath("Phive/StaticCompoundBody/" + mFieldSceneName, false)))
                {
                    if (Entry.is_regular_file())
                    {
                        std::string Filename = Entry.path().filename().string();

                        if (Filename.starts_with(mFieldSceneName + "_"))
                        {
                            std::vector<unsigned char> FileData = application::file::game::ZStdBackend::Decompress(application::util::FileUtil::ReadFile(application::util::FileUtil::GetRomFSFilePath("Phive/StaticCompoundBody/" + mFieldSceneName + "/" + Filename)));
                            application::util::BinaryVectorReader FileReader(FileData);


							application::file::game::phive::PhiveStaticCompoundFile StaticCompoundFile(FileData);
                            
                            uint32_t ImageIter = 0;
                            ImageIter += StaticCompoundFile.mImage.mRigidBodyEntryEntityCount + StaticCompoundFile.mImage.mRigidBodyEntrySensorCount;

                            if (StaticCompoundFile.mImage.mWaterBoxCount != 0)
                            {
                                ImageIter++;
                            }
                            if (StaticCompoundFile.mImage.mWaterCylinderCount != 0)
                            {
                                ImageIter++;
                            }
                            if (StaticCompoundFile.mImage.mWaterFallCount != 0)
                            {
                                ImageIter++;
                            }
                            if (StaticCompoundFile.mImage.mExternalMeshCount != 0)
                            {
                                ImageIter++;
                            }

                            for (uint32_t i = 0; i < StaticCompoundFile.mImage.mExternalMeshCount; i++)
                            {
                                FileReader.Seek(StaticCompoundFile.mHeader.mCompoundShapeImageOffsetArray[ImageIter], application::util::BinaryVectorReader::Position::Begin);
                                
                                application::file::game::phive::shape::PhiveShape::PhiveShapeHeader Header;

                                FileReader.ReadStruct(&Header, 48);

								uint32_t MaterialBaseOffset = StaticCompoundFile.mHeader.mCompoundShapeImageOffsetArray[ImageIter] + Header.mMaterialsOffset;

                                uint32_t MaterialCount = Header.mMaterialsSize / 16;
                                for (uint32_t i = 0; i < MaterialCount; i++)
                                {
									uint32_t* MaterialIdPtr = reinterpret_cast<uint32_t*>(&FileData[MaterialBaseOffset + i * 16]);
									uint32_t* UnkPtr = reinterpret_cast<uint32_t*>(&FileData[MaterialBaseOffset + i * 16 + 4]);

                                    *MaterialIdPtr = MaterialId;
                                    *UnkPtr = 0;
                                }

                                ImageIter++;
                            }


                            std::filesystem::create_directories(application::util::FileUtil::GetSaveFilePath("Phive/StaticCompoundBody/" + mFieldSceneName));
                            application::util::FileUtil::WriteFile(application::util::FileUtil::GetSaveFilePath("Phive/StaticCompoundBody/" + mFieldSceneName + "/" + Filename), application::file::game::ZStdBackend::Compress(FileData, application::file::game::ZStdBackend::Dictionary::Base));
                        }
                    }
                }

                for (const auto& Entry : std::filesystem::directory_iterator(application::util::FileUtil::GetRomFSFilePath("TerrainArc/" + mFieldSceneName, false)))
                {
                    if (Entry.is_regular_file())
                    {
                        std::string Filename = Entry.path().filename().string();

                        if (Filename.size() >= Suffix.size() &&
                            Filename.substr(Filename.size() - Suffix.size()) == Suffix)
                        {
                            application::file::game::terrain::MateArchive MateArchive;
                            MateArchive.Initialize(Entry.path().string(), &TerrainScene->mTerrainSceneFile);

                            for (auto& [Name, Mate] : MateArchive.mMateFiles)
                            {
                                for (auto& Material : Mate.mMaterials)
                                {
                                    if (Material.mIndexA == 120 || Material.mIndexB == 120)
                                        continue;

                                    Material.mBlendFactor = 0;
                                    Material.mIndexA = static_cast<uint8_t>(mCurrentTextureLayer);
                                    Material.mIndexB = static_cast<uint8_t>(mCurrentTextureLayer);
                                }
                            }

                            std::filesystem::create_directories(application::util::FileUtil::GetSaveFilePath("TerrainArc/" + mFieldSceneName));
                            application::util::FileUtil::WriteFile(application::util::FileUtil::GetSaveFilePath("TerrainArc/" + mFieldSceneName + "/" + Filename), application::file::game::ZStdBackend::Compress(MateArchive.ToBinary(), application::file::game::ZStdBackend::Dictionary::None));
                        }
                    }
                }
            }

            if (mPurgeGrass)
            {
                for (auto& Area : TerrainScene->mTerrainSceneFile.mTerrainScene.mAreas)
                {
                    Area.mObj.mFileResources.erase(
                        std::remove_if(
                            Area.mObj.mFileResources.begin(),
                            Area.mObj.mFileResources.end(),
                            [](const application::file::game::terrain::TerrainSceneFile::RelPtr<application::file::game::terrain::TerrainSceneFile::ResFile>& FilePtr)
                            {
                                return FilePtr.mObj.mType == application::file::game::terrain::TerrainSceneFile::ResFileType::Grass;
                            }
                        ),
                        Area.mObj.mFileResources.end()
					);

                    Area.mObj.mFileCount = Area.mObj.mFileResources.size();
               }

                std::filesystem::create_directories(application::util::FileUtil::GetSaveFilePath("TerrainArc"));
                application::util::FileUtil::WriteFile(application::util::FileUtil::GetSaveFilePath("TerrainArc/" + mFieldSceneName + ".tscb"), TerrainScene->mTerrainSceneFile.ToBinary());
            }
        }
	}

	std::string PluginTerrainModificationUtil::GetName() const
	{
		return "Terrain Modification Utility";
	}
	std::string PluginTerrainModificationUtil::GetAuthor() const
	{
		return "MrMystery";
	}
	std::string PluginTerrainModificationUtil::GetDescription() const
	{
		return "A utility plugin for modifying terrain data within the application made for Waikuteru.";
	}
	uint32_t PluginTerrainModificationUtil::GetVersion() const
	{
		return 1;
	}
}