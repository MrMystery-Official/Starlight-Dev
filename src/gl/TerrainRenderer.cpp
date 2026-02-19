#include <gl/TerrainRenderer.h>

#include <glad/glad.h>
#include <manager/ShaderMgr.h>
#include <manager/TerrainMgr.h>
#include <manager/TexToGoFileMgr.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/glm.hpp>
#include <file/game/texture/TexToGoFile.h>
#include <file/game/texture/FormatHelper.h>
#include <file/game/zstd/ZStdBackend.h>
#include <util/FileUtil.h>
#include <filesystem>
#include <util/Logger.h>
#include <map>
#include <sstream>
#include <tool/scene/static_compound/StaticCompoundImplementationFieldScene.h>
#include <meshoptimizer.h>

namespace application::gl
{
    //Default material is stone (7), this only holds everything else
    std::unordered_map<uint16_t, uint16_t> TerrainRenderer::gMaterialAlbToPhiveMaterialIndex =
    {
        {0, 2}, //Grass
        {5, 1}, //Soil
        {6, 3}, //Sand
        {7, 3}, //Sand
        {8, 3}, //Sand
        {10, 1}, //Soil
        {11, 2}, //Grass
        {14, 1}, //Soil
        {29, 2}, //Grass
        {33, 5}, //Snow
        {34, 1}, //Soil
        {35, 1}, //Soil
        {36, 2}, //Grass
        {37, 2}, //Grass
        {38, 2}, //Grass
        {39, 2}, //Grass
        {54, 3}, //Sand
        {55, 2}, //Grass
        {56, 3}, //Sand
        {117, 0}, //Undefined
        {120, 120}, //Hole, no terrain is rendered here
    };

    unsigned int TerrainRenderer::gMaterialAlbID = 0xFFFFFFFF;
    application::gl::Shader* TerrainRenderer::gShader = nullptr;
    unsigned int TerrainRenderer::gTerrainVAO = 0;
    unsigned int TerrainRenderer::gTerrainVBO = 0;


    glm::vec2 TerrainRenderer::SectionNameToCoords(const std::string& SectionName)
    {
        glm::vec2 Result;
        Result.x = SectionName[0] - 65;
        Result.y = ((char)SectionName[2]) - '0' - 1;
        return Result;
    }

    glm::vec2 TerrainRenderer::GetSectionMidpoint(const std::string& SectionName)
    {
        const glm::vec2 Pos = SectionNameToCoords(SectionName);
        glm::vec2 Origin = glm::vec2((Pos.x - 5) * 1000, (Pos.y - 3) * 1000);
        Origin += glm::vec2(500.0f, -500.0f);
        return Origin;
    }

    glm::vec2 GetNearestAreaPosition(float Scale, const glm::vec2& Pos, const glm::vec2& SectionStartPoint)
    {
        glm::vec2 Nearest(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest());

        int AreaCount = 16 / (Scale / 0.125f);
        float TileSize = 1000.0f / AreaCount;
        for (int x = 0; x < AreaCount; x++)
        {
            for (int y = 0; y < AreaCount; y++)
            {
                glm::vec2 AreaPos(SectionStartPoint.x + x * TileSize + 0.5f * TileSize, SectionStartPoint.y + y * TileSize + 0.5f * TileSize);

                if (glm::distance(AreaPos, Pos) < glm::distance(Nearest, Pos)) Nearest = AreaPos;
            }
        }
        
        return Nearest;
    }

    void TerrainRenderer::InitializeMaterialAlbTexture()
    {
        if (gMaterialAlbID == 0xFFFFFFFF)
        {
            application::file::game::texture::TexToGoFile* MaterialAlbTexture = application::manager::TexToGoFileMgr::GetTexture("MaterialAlb.txtg");

            glGenTextures(1, &gMaterialAlbID);
            glBindTexture(GL_TEXTURE_2D_ARRAY, gMaterialAlbID);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);

            // Use glTexStorage3D for immutable storage
            if (MaterialAlbTexture->GetPolishedFormat() != application::file::game::texture::TextureFormat::Format::CPU_DECODED)
            {
                glTexStorage3D(GL_TEXTURE_2D_ARRAY,
                    1,  // Mip levels
                    application::file::game::texture::FormatHelper::gInternalFormatList[MaterialAlbTexture->GetPolishedFormat()],  // Internal format
                    1024, 1024, MaterialAlbTexture->GetDepth()); // Width, Height, Depth
            }
            else
            {
                glTexStorage3D(GL_TEXTURE_2D_ARRAY,
                    1,  // Mip levels
                    GL_RGBA8,  // Internal format
                    1024, 1024, MaterialAlbTexture->GetDepth()); // Width, Height, Depth
            }

            // Upload data to the immutable texture
            for (uint16_t i = 0; i < MaterialAlbTexture->GetDepth(); i++)
            {
                application::file::game::texture::TexToGoFile::Surface& Surface = MaterialAlbTexture->GetSurface(i);
                if (Surface.mPolishedFormat != application::file::game::texture::TextureFormat::Format::CPU_DECODED)
                {
                    glCompressedTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i,
                        1024, 1024, 1, application::file::game::texture::FormatHelper::gInternalFormatList[MaterialAlbTexture->GetPolishedFormat()],
                        Surface.mData.size(), Surface.mData.data());
                }
                else
                {
                    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i,
                        1024, 1024, 1, GL_RGBA, GL_UNSIGNED_BYTE, Surface.mData.data());
                }
            }
        }
    }

    void TerrainRenderer::LoadData(const std::string& SceneName, const std::string& SectionName)
    {
        //Init first time
        if (gShader == nullptr)
        {
            gShader = application::manager::ShaderMgr::GetShader("Terrain", true);

            std::vector<float> Vertices = GenerateTerrainVertices(1000.0f, 1000.0f);
            mVertexCount = Vertices.size() / 5;

            glGenVertexArrays(1, &gTerrainVAO);
            glBindVertexArray(gTerrainVAO);

            glGenBuffers(1, &gTerrainVBO);
            glBindBuffer(GL_ARRAY_BUFFER, gTerrainVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(float) * Vertices.size(), &Vertices[0], GL_STATIC_DRAW);

            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(sizeof(float) * 3));
            glEnableVertexAttribArray(1);
        }

		application::manager::TerrainMgr::TerrainScene* TerrainScene = application::manager::TerrainMgr::GetTerrainScene(SceneName);
        if(TerrainScene == nullptr)
        {
            application::util::Logger::Error("TerrainRenderer", "Failed to load terrain data for scene: %s, section: %s", SceneName.c_str(), SectionName.c_str());
            return;
		}

        mHeightScale = TerrainScene->mTerrainSceneFile.mTerrainScene.mHeightScale / 0xFFFF;
		mSceneName = SceneName;
		mSectionName = SectionName;
        mTerrainYOffset = mSceneName == "MinusField" ? -1300.0f : 0.0f;

		const glm::vec2 SectionMidPoint = GetSectionMidpoint(SectionName);
		const glm::vec2 SectionStartPoint = SectionMidPoint - glm::vec2(500.0f, 500.0f);

        mHeightTextureData.resize((256 * 16) * (256 * 16));
        mLayersTextureData.resize((256 * 16) * (256 * 16) * 3);
        mTexCoordsTextureData.resize((256 * 16) * (256 * 16) * 4);

        mTerrainPosition = glm::vec3(SectionMidPoint.x, 0, SectionMidPoint.y);
        mModelMatrix = glm::translate(mModelMatrix, mTerrainPosition);

        std::vector<application::file::game::terrain::TerrainSceneFile::ResArea*> Tiles = TerrainScene->mTerrainSceneFile.GetSectionTilesByPos(32.0f, SectionMidPoint, 1000.0f);
        
        constexpr float TileWidth = 1000.0f / 16.0f;

        std::vector<int> MateReverseMap(application::file::game::terrain::MateFile::TEXTURE_INDEX_MAP.size(), 0);
        for (int b = 0; b < application::file::game::terrain::MateFile::TEXTURE_INDEX_MAP.size(); ++b) {
            MateReverseMap[application::file::game::terrain::MateFile::TEXTURE_INDEX_MAP[b]] = b;
        }

        for (int x = 0; x < 16; x++)
        {
            for (int y = 0; y < 16; y++)
            {
                const glm::vec2 TilePos = SectionStartPoint + glm::vec2(x * TileWidth, y * TileWidth) + glm::vec2(0.5f * TileWidth, 0.5f * TileWidth);
                application::file::game::terrain::TerrainSceneFile::ResArea* TileArea = nullptr;

				float CurrentScale = 0.125f; // Default scale for tiles
                int BeginX = 0;
                int EndX = 256;
                int BeginY = 0;
                int EndY = 256;

                while (TileArea == nullptr && CurrentScale <= 32.0f)
                {
                    glm::vec2 MatchingAreaPos = GetNearestAreaPosition(CurrentScale, TilePos, SectionStartPoint);

                    for (application::file::game::terrain::TerrainSceneFile::ResArea* Area : Tiles)
                    {
                        if (Area->mScale != CurrentScale)
                            continue;

                        const glm::vec2 AreaPos = glm::vec2(Area->mX * 1000.0f * 0.5f, Area->mZ * 1000.0f * 0.5f);

                        glm::bvec2 Result = glm::epsilonEqual(MatchingAreaPos, AreaPos, glm::vec2(1.0f));
                        if (Result.x && Result.y)
                        {
                            TileArea = Area;

							//When I thought of those calculations only god and I knew what that code does, now only god knows.
							int SubSectionSize = 256 / (CurrentScale / 0.125f);

                            int AreaTileScale = TileWidth * (CurrentScale / 0.125f);
                            glm::vec2 StartTilePos = TilePos - glm::vec2(0.5f * TileWidth, 0.5f * TileWidth);
                            glm::vec2 StartAreaPos = AreaPos - glm::vec2(0.5f * AreaTileScale, 0.5f * AreaTileScale);

                            BeginX = (StartTilePos.x - StartAreaPos.x) / TileWidth * SubSectionSize;
                            BeginY = (StartTilePos.y - StartAreaPos.y) / TileWidth * SubSectionSize;
                            EndX = BeginX + SubSectionSize;
                            EndY = BeginY + SubSectionSize;

                            break;
                        }
                    }

                    CurrentScale *= 2.0f;
                }

                if (TileArea == nullptr)
                {
                    continue;
                }

                application::manager::TerrainMgr::TerrainScene::ArchivePack* ArchivePack = application::manager::TerrainMgr::GetArchivePack(TileArea->mFilenameOffset.mResString.mString, *TerrainScene);

                if(ArchivePack == nullptr)
                {
                    application::util::Logger::Error("TerrainRenderer", "Failed to get archive pack for tile: %s", TileArea->mFilenameOffset.mResString.mString.c_str());
                    continue;
				}

                application::file::game::terrain::HghtFile& Hght = ArchivePack->mHghtArchive.mHeightMaps[TileArea->mFilenameOffset.mResString.mString];
                application::file::game::terrain::MateFile& Mate = ArchivePack->mMateArchive.mMateFiles[TileArea->mFilenameOffset.mResString.mString];

                int LODScale = static_cast<int>(TileArea->mScale / 0.125f);
                int SourceResolution = 256 / LODScale;

                for (int heightX = 0; heightX < 256; heightX++)
                {
                    for (int heightY = 0; heightY < 256; heightY++)
                    {
                        int TextureX = x * 256 + heightX;
                        int TextureY = y * 256 + heightY;
                        int TextureIndex = TextureY * (256 * 16) + TextureX;

                        if (TextureIndex >= 0 && TextureIndex < mHeightTextureData.size())
                        {
                            float Height = 0.0f;

                            unsigned char LayerA = 0;
                            unsigned char LayerB = 0;
                            unsigned char BlendFactor = 0;

                            if (LODScale == 1) // Full resolution
                            {
                                Height = Hght.GetHeightAtGridPos(heightX + 2, heightY + 2) * mHeightScale;

                                LayerA = Mate.GetMaterialAtGridPos(heightX + 2, heightY + 2)->mIndexA;
                                LayerB = Mate.GetMaterialAtGridPos(heightX + 2, heightY + 2)->mIndexB;
                                BlendFactor = Mate.GetMaterialAtGridPos(heightX + 2, heightY + 2)->mBlendFactor;
                            }
                            else
                            {
                                float SourceX = (float(heightX) / 256.0f) * SourceResolution;
                                float SourceY = (float(heightY) / 256.0f) * SourceResolution;

                                int x0 = static_cast<int>(std::floor(SourceX));
                                int x1 = std::min(x0 + 1, SourceResolution - 1);
                                int y0 = static_cast<int>(std::floor(SourceY));
                                int y1 = std::min(y0 + 1, SourceResolution - 1);

                                float fx = SourceX - x0;
                                float fy = SourceY - y0;

                                float h00 = Hght.GetHeightAtGridPos(BeginX + x0 + 2, BeginY + y0 + 2) * mHeightScale;
                                float h10 = Hght.GetHeightAtGridPos(BeginX + x1 + 2, BeginY + y0 + 2) * mHeightScale;
                                float h01 = Hght.GetHeightAtGridPos(BeginX + x0 + 2, BeginY + y1 + 2) * mHeightScale;
                                float h11 = Hght.GetHeightAtGridPos(BeginX + x1 + 2, BeginY + y1 + 2) * mHeightScale;

                                float h0 = h00 * (1.0f - fx) + h10 * fx;
                                float h1 = h01 * (1.0f - fx) + h11 * fx;
                                Height = h0 * (1.0f - fy) + h1 * fy;



                                application::file::game::terrain::MateFile::MaterialIndexInfo Material00 = Mate.mMaterials[(BeginX + x0 + 2) + (BeginY + y0 + 2) * Mate.mWidth];

                                application::file::game::terrain::MateFile::MaterialIndexInfo Material10 = (BeginX + x0 + 2) < (Mate.mWidth - 1) ?
                                    Mate.mMaterials[(BeginX + x0 + 2) + 1 + (BeginY + y0 + 2) * Mate.mWidth] : Material00;

                                application::file::game::terrain::MateFile::MaterialIndexInfo Material01 = (BeginY + y0 + 2) < (Mate.mHeight - 1) ?
                                    Mate.mMaterials[(BeginX + x0 + 2) + ((BeginY + y0 + 2) + 1) * Mate.mWidth] : Material00;

                                application::file::game::terrain::MateFile::MaterialIndexInfo Material11 = (((BeginY + y0 + 2) < (Mate.mHeight - 1)) && ((BeginX + x0 + 2) < (Mate.mWidth - 1))) ?
                                    Mate.mMaterials[(BeginX + x0 + 2) + 1 + ((BeginY + y0 + 2) + 1) * Mate.mWidth] : Material00;

                                std::map<uint8_t, float> textureWeights;

                                float weight00 = (1 - fx) * (1 - fy);
                                textureWeights[Material00.mIndexA] += weight00 * (255 - Material00.mBlendFactor) / 255.0f;
                                textureWeights[Material00.mIndexB] += weight00 * Material00.mBlendFactor / 255.0f;

                                float weight10 = fx * (1 - fy);
                                textureWeights[Material10.mIndexA] += weight10 * (255 - Material10.mBlendFactor) / 255.0f;
                                textureWeights[Material10.mIndexB] += weight10 * Material10.mBlendFactor / 255.0f;

                                float weight01 = (1 - fx) * fy;
                                textureWeights[Material01.mIndexA] += weight01 * (255 - Material01.mBlendFactor) / 255.0f;
                                textureWeights[Material01.mIndexB] += weight01 * Material01.mBlendFactor / 255.0f;

                                float weight11 = fx * fy;
                                textureWeights[Material11.mIndexA] += weight11 * (255 - Material11.mBlendFactor) / 255.0f;
                                textureWeights[Material11.mIndexB] += weight11 * Material11.mBlendFactor / 255.0f;

                                uint8_t dominantTextureA = 0;
                                uint8_t dominantTextureB = 0;
                                float maxWeightA = 0.0f;
                                float maxWeightB = 0.0f;

                                for (const auto& pair : textureWeights) 
                                {
                                    if (pair.second > maxWeightA) 
                                    {
                                        maxWeightB = maxWeightA;
                                        dominantTextureB = dominantTextureA;
                                        maxWeightA = pair.second;
                                        dominantTextureA = pair.first;
                                    }
                                    else if (pair.second > maxWeightB) 
                                    {
                                        maxWeightB = pair.second;
                                        dominantTextureB = pair.first;
                                    }
                                }

                                // Calculate blend factor between the two most dominant textures
                                uint8_t blendFactor = 0;
                                if (maxWeightA + maxWeightB > 0) 
                                {
                                    blendFactor = static_cast<uint8_t>(std::round((maxWeightB / (maxWeightA + maxWeightB)) * 255.0f));
                                }

                                LayerA = dominantTextureA;
                                LayerB = dominantTextureB;
                                BlendFactor = blendFactor;
                            }

                            mHeightTextureData[TextureIndex] = Height;

							mLayersTextureData[TextureIndex * 3] = LayerA;
							mLayersTextureData[TextureIndex * 3 + 1] = LayerB;
							mLayersTextureData[TextureIndex * 3 + 2] = BlendFactor;

                            mTexCoordsTextureData[TextureIndex * 4] = static_cast<float>(TextureX) / (256 * 16) * 1000.0 * TerrainScene->mTerrainSceneFile.mTerrainScene.mMaterialInfo[MateReverseMap[LayerA]].mObj.mUScale;
                            mTexCoordsTextureData[TextureIndex * 4 + 1] = static_cast<float>(TextureY) / (256 * 16) * 1000.0 * TerrainScene->mTerrainSceneFile.mTerrainScene.mMaterialInfo[MateReverseMap[LayerA]].mObj.mVScale;
                            mTexCoordsTextureData[TextureIndex * 4 + 2] = static_cast<float>(TextureX) / (256 * 16) * 1000.0 * TerrainScene->mTerrainSceneFile.mTerrainScene.mMaterialInfo[MateReverseMap[LayerB]].mObj.mUScale;
                            mTexCoordsTextureData[TextureIndex * 4 + 3] = static_cast<float>(TextureY) / (256 * 16) * 1000.0 * TerrainScene->mTerrainSceneFile.mTerrainScene.mMaterialInfo[MateReverseMap[LayerB]].mObj.mVScale;
                        }
                    }
                }
            }
        }

        glGenTextures(1, &mHeightTextureID);
        glBindTexture(GL_TEXTURE_2D, mHeightTextureID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, 256 * 16, 256 * 16, 0, GL_RED, GL_FLOAT, mHeightTextureData.data());

        glGenTextures(1, &mLayersTextureID);
        glBindTexture(GL_TEXTURE_2D, mLayersTextureID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8UI, 256 * 16, 256 * 16, 0, GL_RGB_INTEGER, GL_UNSIGNED_BYTE, mLayersTextureData.data());

        glGenTextures(1, &mTexCoordsTextureID);
        glBindTexture(GL_TEXTURE_2D, mTexCoordsTextureID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 256 * 16, 256 * 16, 0, GL_RGBA, GL_FLOAT, mTexCoordsTextureData.data());

        InitializeMaterialAlbTexture();
    }

    void TerrainRenderer::Draw(application::gl::Camera& Camera)
    {
        if (gMaterialAlbID == 0xFFFFFFFF)
            return;

        gShader->Bind();
        Camera.Matrix(gShader, "uCamMatrix");
        glUniformMatrix4fv(glGetUniformLocation(gShader->mID, "uModelMatrix"), 1, GL_FALSE, glm::value_ptr(mModelMatrix));
        glUniformMatrix4fv(glGetUniformLocation(gShader->mID, "uView"), 1, GL_FALSE, glm::value_ptr(Camera.GetViewMatrix()));
		glUniform1i(glGetUniformLocation(gShader->mID, "uHeightMap"), 0);
		glUniform1i(glGetUniformLocation(gShader->mID, "uMaterialAlb"), 1);
		glUniform1i(glGetUniformLocation(gShader->mID, "uLayers"), 2);
		glUniform1i(glGetUniformLocation(gShader->mID, "uTexCoords"), 3);

        glUniform1i(glGetUniformLocation(gShader->mID, "uRenderLayerA"), mRenderLayerA ? 1 : 0);
        glUniform1i(glGetUniformLocation(gShader->mID, "uRenderLayerB"), mRenderLayerB ? 1 : 0);
        glUniform1i(glGetUniformLocation(gShader->mID, "uEnableTessellation"), mEnableTessellation ? 1 : 0);

        glUniform1f(glGetUniformLocation(gShader->mID, "uVertexYOffset"), mTerrainYOffset);

		glBindVertexArray(gTerrainVAO);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D_ARRAY, gMaterialAlbID);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, mLayersTextureID);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, mTexCoordsTextureID);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mHeightTextureID);

        //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDrawArrays(GL_PATCHES, 0, mVertexCount);
        //glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    float TerrainRenderer::CalculateErrorForScale(int tileX, int tileY, float targetScale,
        const glm::vec2& TilePos, const glm::vec2& SectionStartPoint)
    {
        int targetLOD = targetScale / 0.125f;
        int targetSubSectionSize = 256 / targetLOD;
        float maxError = 0.0f;

        for (int px = 0; px < 256; px += targetLOD)
        {
            for (int py = 0; py < 256; py += targetLOD)
            {
                // Get actual height from our high-resolution data
                int textureX = tileX * 256 + px;
                int textureY = tileY * 256 + py;
                int textureIndex = textureY * (256 * 16) + textureX;

                if (textureIndex < 0 || textureIndex >= mHeightTextureData.size()) continue;

                float actualHeight = mHeightTextureData[textureIndex];

                // Calculate bilinear interpolated height at target LOD
                int lodX = px / targetLOD;
                int lodY = py / targetLOD;

                auto getHeightAtTargetLOD = [&](int offsetX, int offsetY) -> float {
                    int texX = tileX * 256 + offsetX * targetLOD;
                    int texY = tileY * 256 + offsetY * targetLOD;
                    int texIdx = texY * (256 * 16) + texX;

                    if (texIdx >= 0 && texIdx < mHeightTextureData.size()) {
                        return mHeightTextureData[texIdx];
                    }
                    return actualHeight;
                    };

                int x0 = lodX;
                int x1 = std::min(lodX + 1, targetSubSectionSize - 1);
                int y0 = lodY;
                int y1 = std::min(lodY + 1, targetSubSectionSize - 1);

                float fx = (float(px % targetLOD)) / targetLOD;
                float fy = (float(py % targetLOD)) / targetLOD;

                float h00 = getHeightAtTargetLOD(x0, y0);
                float h10 = getHeightAtTargetLOD(x1, y0);
                float h01 = getHeightAtTargetLOD(x0, y1);
                float h11 = getHeightAtTargetLOD(x1, y1);

                float h0 = h00 * (1.0f - fx) + h10 * fx;
                float h1 = h01 * (1.0f - fx) + h11 * fx;
                float interpolatedHeight = h0 * (1.0f - fy) + h1 * fy;

                float error = std::abs(actualHeight - interpolatedHeight);
                maxError = std::max(maxError, error);
            }
        }

        return maxError;
    }

    std::pair<std::vector<glm::vec3>, std::vector<std::pair<std::tuple<uint32_t, uint32_t, uint32_t>, uint32_t>>> TerrainRenderer::GenerateTerrainTileCollisionModel(const glm::vec2& WorldOffset, std::vector<application::file::game::phive::util::PhiveMaterialData::PhiveMaterial>* Materials)
    {
        const float QuadSize = 1.0f;
        const int Dimensions = 251;
        const int TerrainTextureSize = 256 * 16; // 4096

        // Results
        const int QuadCount = Dimensions / QuadSize;

        std::vector<std::pair<glm::vec3, uint32_t>> Vertices; //{Vertex, MaterialAlbIndex}

        std::vector<glm::vec3> RealVertices;
        std::vector<std::pair<std::tuple<uint32_t, uint32_t, uint32_t>, uint32_t>> Indices;

        Vertices.resize(QuadCount * QuadCount);
        RealVertices.reserve(QuadCount * QuadCount);

		application::util::Logger::Info("TerrainRenderer", "Generating collision model at world offset " + std::to_string(WorldOffset.x) + ", " + std::to_string(WorldOffset.y));

        for (int z = 0; z < QuadCount; z++)
        {
            for (int x = 0; x < QuadCount; x++)
            {
                glm::vec3& Vertex = Vertices[x + z * QuadCount].first;
                Vertex.x = x * QuadSize;
                Vertex.z = z * QuadSize;

				//Calculate height from heightmap

                // Convert local model coordinates to world coordinates
                float worldX = Vertex.x + WorldOffset.x + (1000.0f / 4096.0f);
                float worldZ = Vertex.z + WorldOffset.y + (1000.0f / 4096.0f);

                // Your terrain texture represents 1000x1000 world units with 4096x4096 pixels
                // Each tile is 250x250 world units
                // So each tile should map to 1024x1024 pixels (4096/4 = 1024)

                glm::vec2 TerrainMin = glm::vec2(mTerrainPosition.x - 500.0f,
                    mTerrainPosition.z - 500.0f);
                glm::vec2 TerrainMax = glm::vec2(mTerrainPosition.x + 500.0f,
                    mTerrainPosition.z + 500.0f);
                glm::vec2 LocalPos = (glm::vec2(worldX, worldZ) - TerrainMin) / (TerrainMax - TerrainMin);

                // The key fix: account for the fact that your 251 vertices represent 250 units
                // but should map to the texture such that vertex 0 = texture 0 and vertex 250 = texture end
                float TexX = LocalPos.x * (TerrainTextureSize - 1.0f);
                float TexY = LocalPos.y * (TerrainTextureSize - 1.0f);

                // Clamp to valid range
                int x0 = std::max(0, (int)std::floor(TexX));
                int z0 = std::max(0, (int)std::floor(TexY));
                int x1 = std::min(x0 + 1, TerrainTextureSize - 1);
                int z1 = std::min(z0 + 1, TerrainTextureSize - 1);

                float fx = TexX - x0;
                float fz = TexY - z0;

                // Clamp fractional parts
                fx = std::max(0.0f, std::min(1.0f, fx));
                fz = std::max(0.0f, std::min(1.0f, fz));


                // Sample the four corner heights (assuming you have access to the texture data)
                // Replace this with your actual texture sampling method
                float h00 = mHeightTextureData[z0 * TerrainTextureSize + x0];
                float h10 = mHeightTextureData[z0 * TerrainTextureSize + x1];
                float h01 = mHeightTextureData[z1 * TerrainTextureSize + x0];
                float h11 = mHeightTextureData[z1 * TerrainTextureSize + x1];

                // Bilinear interpolation
                float h0 = h00 * (1.0f - fx) + h10 * fx; // bottom edge
                float h1 = h01 * (1.0f - fx) + h11 * fx; // top edge

                Vertex.y = h0 * (1.0f - fz) + h1 * fz;

				//Calculate material

                uint8_t ExactMaterialIndexInfoIndexA = mLayersTextureData[(static_cast<int>(TexY) * TerrainTextureSize + static_cast<int>(TexX)) * 3];
                uint8_t ExactMaterialIndexInfoIndexB = mLayersTextureData[(static_cast<int>(TexY) * TerrainTextureSize + static_cast<int>(TexX)) * 3 + 1];

                application::file::game::terrain::MateFile::MaterialIndexInfo m00 = application::file::game::terrain::MateFile::MaterialIndexInfo
                {
                    .mIndexA = mLayersTextureData[(z0 * TerrainTextureSize + x0) * 3],
                    .mIndexB = mLayersTextureData[(z0 * TerrainTextureSize + x0) * 3 + 1],
                    .mBlendFactor = mLayersTextureData[(z0 * TerrainTextureSize + x0) * 3 + 2]
                };

                application::file::game::terrain::MateFile::MaterialIndexInfo m10 = application::file::game::terrain::MateFile::MaterialIndexInfo
                {
                    .mIndexA = mLayersTextureData[(z0 * TerrainTextureSize + x1) * 3],
                    .mIndexB = mLayersTextureData[(z0 * TerrainTextureSize + x1) * 3 + 1],
                    .mBlendFactor = mLayersTextureData[(z0 * TerrainTextureSize + x1) * 3 + 2]
                };

                application::file::game::terrain::MateFile::MaterialIndexInfo m01 = application::file::game::terrain::MateFile::MaterialIndexInfo
                {
                    .mIndexA = mLayersTextureData[(z1 * TerrainTextureSize + x0) * 3],
                    .mIndexB = mLayersTextureData[(z1 * TerrainTextureSize + x0) * 3 + 1],
                    .mBlendFactor = mLayersTextureData[(z1 * TerrainTextureSize + x0) * 3 + 2]
                };

                application::file::game::terrain::MateFile::MaterialIndexInfo m11 = application::file::game::terrain::MateFile::MaterialIndexInfo
                {
                    .mIndexA = mLayersTextureData[(z1 * TerrainTextureSize + x1) * 3],
                    .mIndexB = mLayersTextureData[(z1 * TerrainTextureSize + x1) * 3 + 1],
                    .mBlendFactor = mLayersTextureData[(z1 * TerrainTextureSize + x1) * 3 + 2]
                };

                uint16_t rm00 = m00.mBlendFactor > 0.5f ? m00.mIndexB : m00.mIndexA;
                uint16_t rm10 = m10.mBlendFactor > 0.5f ? m10.mIndexB : m10.mIndexA;
                uint16_t rm01 = m01.mBlendFactor > 0.5f ? m01.mIndexB : m01.mIndexA;
                uint16_t rm11 = m11.mBlendFactor > 0.5f ? m11.mIndexB : m11.mIndexA;

                std::map<uint16_t, uint8_t> IndexCount;
                IndexCount[rm00]++;
                IndexCount[rm10]++;
                IndexCount[rm01]++;
                IndexCount[rm11]++;
                std::vector<std::pair<uint16_t, uint8_t>> sortedPairs(
                    IndexCount.begin(), IndexCount.end()
                );

                std::sort(sortedPairs.begin(), sortedPairs.end(),
                    [](const auto& a, const auto& b) {
                        return a.second > b.second;
                    }
                );

                Vertices[x + z * QuadCount].second = sortedPairs[0].first;

                if (ExactMaterialIndexInfoIndexA == 120 || ExactMaterialIndexInfoIndexB == 120)
                {
                    Vertices[x + z * QuadCount].second = 120;
                }

                RealVertices.push_back(Vertex);
            }
        }

        Indices.reserve(((QuadCount - 1) * (QuadCount - 1) * 6) / 3);
        for (uint16_t z = 0; z < QuadCount - 1; z++)
        {
            for (uint16_t x = 0; x < QuadCount - 1; x++)
            {
                uint32_t topLeft = z * QuadCount + x;
                uint32_t topRight = topLeft + 1;
                uint32_t bottomLeft = (z + 1) * QuadCount + x;
                uint32_t bottomRight = bottomLeft + 1;

                uint16_t m00 = Vertices[topLeft].second;
                uint16_t m10 = Vertices[topRight].second;
                uint16_t m01 = Vertices[bottomLeft].second;
                uint16_t m11 = Vertices[bottomRight].second;
                std::map<uint16_t, uint8_t> IndexCount;
                IndexCount[m00]++;
                IndexCount[m10]++;
                IndexCount[m01]++;
                IndexCount[m11]++;
                std::vector<std::pair<uint16_t, uint8_t>> sortedPairs(
                    IndexCount.begin(), IndexCount.end()
                );

                std::sort(sortedPairs.begin(), sortedPairs.end(),
                    [](const auto& a, const auto& b) {
                        return a.second > b.second;
                    }
                );

                uint32_t MaterialId = gMaterialAlbToPhiveMaterialIndex.contains(sortedPairs[0].first) ? gMaterialAlbToPhiveMaterialIndex[sortedPairs[0].first] : 7;
                int32_t MaterialIndex = -1;

                for (auto& [Index, Freq] : sortedPairs)
                {
                    if (Index == 120)
                    {
                        MaterialId = 120;
                        break;
                    }
                }

                if (Materials != nullptr)
                {
                    for (size_t i = 0; i < Materials->size(); i++)
                    {
                        if ((*Materials)[i].mMaterialId == MaterialId)
                        {
                            MaterialIndex = i;
                            break;
                        }
                    }
                    if (MaterialIndex == -1 && MaterialId != 120)
                    {
                        MaterialIndex = Materials->size();

                        application::file::game::phive::util::PhiveMaterialData::PhiveMaterial Material;
                        Material.mMaterialId = MaterialId;
                        Material.mCollisionFlags[0] = true;
                        Materials->push_back(Material);
                    }
                }

                if (MaterialId == 120)
                {
                    MaterialIndex = 120;
                }

                Indices.push_back(std::make_pair(std::make_tuple(topLeft, bottomLeft, topRight), MaterialIndex));
                Indices.push_back(std::make_pair(std::make_tuple(topRight, bottomLeft, bottomRight), MaterialIndex));
            }
        }

        for (auto Iter = Indices.begin(); Iter != Indices.end(); )
        {
            auto& [Face, Material] = *Iter;

            if (Material == 120)
            {
                Iter = Indices.erase(Iter);
                continue;
            }

            Iter++;
        }

        return { RealVertices, Indices };
    }

    void TerrainRenderer::Save(void* FieldSceneImpl)
    {
        if (mSceneName.empty() || mSectionName.empty()) 
        {
            application::util::Logger::Error("TerrainRenderer", "Cannot save: SceneName or SectionName not set");
            return;
        }

        application::manager::TerrainMgr::TerrainScene* TerrainScene = application::manager::TerrainMgr::GetTerrainScene(mSceneName);
        if (TerrainScene == nullptr) 
        {
            application::util::Logger::Error("TerrainRenderer", "Failed to get terrain scene for saving: %s", mSceneName.c_str());
            return;
        }

        const glm::vec2 SectionMidPoint = GetSectionMidpoint(mSectionName);
        const glm::vec2 SectionStartPoint = SectionMidPoint - glm::vec2(500.0f, 500.0f);

        std::vector<application::file::game::terrain::TerrainSceneFile::ResArea*> Tiles =
            TerrainScene->mTerrainSceneFile.GetSectionTilesByPos(32.0f, SectionMidPoint, 1000.0f);

        constexpr float BaseTileWidth = 1000.0f / 16.0f;

        constexpr float ERROR_THRESHOLD = 0.1f;

        //Applying mate edits
        for (int x = 0; x < 16; x++)
        {
            for (int y = 0; y < 16; y++)
            {
                if (!mModifiedTextureTiles.contains(x + y * 16))
                    continue;

                const glm::vec2 TilePos = SectionStartPoint + glm::vec2(x * BaseTileWidth, y * BaseTileWidth) + glm::vec2(0.5f * BaseTileWidth, 0.5f * BaseTileWidth);

                float CurrentScale = 0.125f;

                while (CurrentScale <= 32.0f)
                {
                    float TileWidth = BaseTileWidth * (CurrentScale / 0.125f);

                    for (application::file::game::terrain::TerrainSceneFile::ResArea* Area : Tiles)
                    {
                        if (Area->mScale != CurrentScale)
                            continue;

                        const glm::vec2 AreaPos = glm::vec2(Area->mX * 1000.0f * 0.5f, Area->mZ * 1000.0f * 0.5f);
                        const glm::vec2 AreaStartPos = AreaPos - glm::vec2(0.5f * TileWidth, 0.5f * TileWidth);
                        const glm::vec2 AreaEndPos = AreaPos + glm::vec2(0.5f * TileWidth, 0.5f * TileWidth);

                        //Found area that covers tile
                        if (TilePos.x > AreaStartPos.x && TilePos.x < AreaEndPos.x &&
                            TilePos.y > AreaStartPos.y && TilePos.y < AreaEndPos.y)
                        {
                            // Get the archive pack for this tile
                            application::manager::TerrainMgr::TerrainScene::ArchivePack* ArchivePack =
                                application::manager::TerrainMgr::GetArchivePack(Area->mFilenameOffset.mResString.mString, *TerrainScene);

                            if (ArchivePack == nullptr) {
                                application::util::Logger::Error("TerrainRenderer", "Failed to get archive pack for saving tile: %s",
                                    Area->mFilenameOffset.mResString.mString.c_str());
                                continue;
                            }

                            application::file::game::terrain::MateFile& Mate = ArchivePack->mMateArchive.mMateFiles[Area->mFilenameOffset.mResString.mString];

                            const glm::vec2 StartTilePos = TilePos - glm::vec2(0.5f * BaseTileWidth, 0.5f * BaseTileWidth);
                            const int SubSectionSize = 256 / (CurrentScale / 0.125f);

                            int BeginX = (StartTilePos.x - AreaStartPos.x) / TileWidth * 256;
                            int BeginY = (StartTilePos.y - AreaStartPos.y) / TileWidth * 256;
                            int EndX = BeginX + SubSectionSize;
                            int EndY = BeginY + SubSectionSize;

                            for (int AreaX = BeginX - 2; AreaX < EndX + 2; AreaX++)
                            {
                                for (int AreaY = BeginY - 2; AreaY < EndY + 2; AreaY++)
                                {
                                    // Map from ResArea coordinates back to world coordinates, then to texture coordinates
                                    float worldX = AreaStartPos.x + (AreaX / 256.0f) * TileWidth;
                                    float worldY = AreaStartPos.y + (AreaY / 256.0f) * TileWidth;

                                    // Convert world coordinates to texture coordinates
                                    int TextureX = (worldX - SectionStartPoint.x) / 1000.0f * (256 * 16);
                                    int TextureY = (worldY - SectionStartPoint.y) / 1000.0f * (256 * 16);

                                    // Skip if the texture coordinates are outside our texture bounds
                                    // (This happens for margin areas on edge tiles)
                                    if (TextureX < 0 || TextureX >= (256 * 16) ||
                                        TextureY < 0 || TextureY >= (256 * 16))
                                    {
                                        continue; // Skip margin areas that have no data
                                    }

                                    int TextureIndex = (TextureY * (256 * 16) + TextureX) * 3;

                                    if (TextureIndex >= 0 && TextureIndex < mLayersTextureData.size())
                                    {
                                        uint8_t TextureLayerA = mLayersTextureData[TextureIndex];
                                        uint8_t TextureLayerB = mLayersTextureData[TextureIndex + 1];
                                        uint8_t TextureBlend = mLayersTextureData[TextureIndex + 2];

                                        // Ensure we don't write outside the height map bounds
                                        int hghtX = AreaX + 2;
                                        int hghtY = AreaY + 2;

                                        if (hghtX >= 0 && hghtX < Mate.GetWidth() &&
                                            hghtY >= 0 && hghtY < Mate.GetHeight())
                                        {
                                            Mate.mMaterials[hghtX + hghtY * Mate.GetWidth()].mIndexA = TextureLayerA;
                                            Mate.mMaterials[hghtX + hghtY * Mate.GetWidth()].mIndexB = TextureLayerB;
                                            Mate.mMaterials[hghtX + hghtY * Mate.GetWidth()].mBlendFactor = TextureBlend;
                                            Mate.mModified = true;
                                        }
                                    }
                                }
                            }

                            application::util::Logger::Info("TerrainRenderer", "Saved height data for additional ResArea: %s (Scale: %.3f)",
                                Area->mFilenameOffset.mResString.mString.c_str(), CurrentScale);

                            break;
                        }
                    }

                    CurrentScale *= 2.0f;
                }
            }
        }

        // 1st iteration, calculating the required LOD per tile
        for (int x = 0; x < 16; x++)
        {
            for (int y = 0; y < 16; y++)
            {
                if (!mModifiedTiles.contains(x + y * 16))
                    continue;

                const glm::vec2 TilePos = SectionStartPoint + glm::vec2(x * BaseTileWidth, y * BaseTileWidth) + glm::vec2(0.5f * BaseTileWidth, 0.5f * BaseTileWidth);
                application::file::game::terrain::TerrainSceneFile::ResArea* TileArea = nullptr;
                float CurrentScale = 0.125f; // Default scale for tiles

                while (TileArea == nullptr && CurrentScale <= 32.0f)
                {
                    glm::vec2 MatchingAreaPos = GetNearestAreaPosition(CurrentScale, TilePos, SectionStartPoint);
                    for (application::file::game::terrain::TerrainSceneFile::ResArea* Area : Tiles)
                    {
                        if (Area->mScale != CurrentScale)
                            continue;
                        const glm::vec2 AreaPos = glm::vec2(Area->mX * 1000.0f * 0.5f, Area->mZ * 1000.0f * 0.5f);
                        glm::bvec2 Result = glm::epsilonEqual(MatchingAreaPos, AreaPos, glm::vec2(1.0f));
                        if (Result.x && Result.y)
                        {
                            TileArea = Area;
                            break;
                        }
                    }
                    if (TileArea == nullptr)
                    {
                        CurrentScale *= 2.0f;
                    }
                }

                if (TileArea == nullptr)
                {
                    application::util::Logger::Warning("TerrainRenderer", "No ResArea found for tile at (%d, %d) in section %s, thrown while calculating required LOD", x, y, mSectionName.c_str());
                    continue;
                }

                int AreaLOD = TileArea->mScale / 0.125f;
                if (AreaLOD == 1)
                {
                    continue; // Full resolution, no need to calculate further
                }

                int SubSectionSize = 256 / AreaLOD;
                float maxTileError = 0.0f;

                // Calculate the required LOD for this tile by checking interpolation error
                // We need to check every pixel in the tile, not just the LOD sample points
                for (int pixelX = 0; pixelX < 256; pixelX++)
                {
                    for (int pixelY = 0; pixelY < 256; pixelY++)
                    {
                        // Get the actual high-resolution height
                        int actualTexX = x * 256 + pixelX;
                        int actualTexY = y * 256 + pixelY;
                        int actualTexIndex = actualTexY * (256 * 16) + actualTexX;

                        if (actualTexIndex < 0 || actualTexIndex >= mHeightTextureData.size())
                            continue;

                        float actualHeight = mHeightTextureData[actualTexIndex];

                        // Calculate what this pixel's height would be if interpolated from the current LOD
                        // Find the LOD grid position for this pixel
                        float lodX = (float)pixelX / AreaLOD;
                        float lodY = (float)pixelY / AreaLOD;

                        int x0 = (int)std::floor(lodX);
                        int x1 = std::min(x0 + 1, SubSectionSize - 1);
                        int y0 = (int)std::floor(lodY);
                        int y1 = std::min(y0 + 1, SubSectionSize - 1);

                        float fx = lodX - x0;
                        float fy = lodY - y0;

                        // Get the heights at the 4 LOD sample points
                        auto getHeightAtLODPoint = [&](int lodPtX, int lodPtY) -> float {
                            int sampleTexX = x * 256 + lodPtX * AreaLOD;
                            int sampleTexY = y * 256 + lodPtY * AreaLOD;
                            int sampleTexIndex = sampleTexY * (256 * 16) + sampleTexX;

                            if (sampleTexIndex >= 0 && sampleTexIndex < mHeightTextureData.size()) {
                                return mHeightTextureData[sampleTexIndex];
                            }
                            return actualHeight; // Fallback
                            };

                        float h00 = getHeightAtLODPoint(x0, y0);
                        float h10 = getHeightAtLODPoint(x1, y0);
                        float h01 = getHeightAtLODPoint(x0, y1);
                        float h11 = getHeightAtLODPoint(x1, y1);

                        // Bilinear interpolation
                        float h0 = h00 * (1.0f - fx) + h10 * fx;
                        float h1 = h01 * (1.0f - fx) + h11 * fx;
                        float interpolatedHeight = h0 * (1.0f - fy) + h1 * fy;

                        // Calculate error
                        float error = std::abs(actualHeight - interpolatedHeight);
                        maxTileError = std::max(maxTileError, error);
                    }
                }

                // Check if we need a higher LOD ResArea for this tile
                if (maxTileError > ERROR_THRESHOLD)
                {
                    // Calculate what LOD we need
                    float requiredScale = TileArea->mScale;
                    float testError = maxTileError;
					application::file::game::terrain::TerrainSceneFile::ResArea* ParentArea = TileArea;

                    while (testError > ERROR_THRESHOLD && requiredScale > 0.125f)
                    {
                        requiredScale /= 2.0f; // Next higher LOD level

                        // Calculate error for this potential LOD level
                        testError = CalculateErrorForScale(x, y, requiredScale, TilePos, SectionStartPoint);



                        if (requiredScale < ParentArea->mScale)
                        {
                            // We need a higher LOD - check if ResArea already exists
                            glm::vec2 RequiredAreaPos = GetNearestAreaPosition(requiredScale, TilePos, SectionStartPoint);
                            bool areaExists = false;

                            for (application::file::game::terrain::TerrainSceneFile::ResArea* Area : Tiles)
                            {
                                if (Area->mScale != requiredScale) continue;

                                const glm::vec2 AreaPos = glm::vec2(Area->mX * 1000.0f * 0.5f, Area->mZ * 1000.0f * 0.5f);
                                glm::bvec2 Result = glm::epsilonEqual(RequiredAreaPos, AreaPos, glm::vec2(1.0f));

                                if (Result.x && Result.y)
                                {
                                    areaExists = true;
                                    break;
                                }
                            }

                            if (!areaExists)
                            {
                                application::file::game::terrain::TerrainSceneFile::ResArea NewArea;
                                NewArea.mScale = requiredScale;
                                NewArea.mX = RequiredAreaPos.x / (1000.0f * 0.5f);
                                NewArea.mZ = RequiredAreaPos.y / (1000.0f * 0.5f);

                                NewArea.mFilenameOffset.mResString.mString = application::manager::TerrainMgr::GenerateHghtNameForArea(TerrainScene, &NewArea); //Using an existing HGHT for now instead of creating a new one
                                NewArea.mFileCount = 3;
                                NewArea._40 = 1; //No idea what that is, but most areas have it set to 1 (dt said a visibility flag, thanks dt <3)

                                //Creating the file resources
                                application::file::game::terrain::TerrainSceneFile::ResFile HeightMapFile;
                                HeightMapFile.mType = application::file::game::terrain::TerrainSceneFile::ResFileType::HeightMap;

                                application::file::game::terrain::TerrainSceneFile::ResFile NormalMapFile;
                                NormalMapFile.mType = application::file::game::terrain::TerrainSceneFile::ResFileType::NormalMap;

                                application::file::game::terrain::TerrainSceneFile::ResFile MaterialMapFile;
                                MaterialMapFile.mType = application::file::game::terrain::TerrainSceneFile::ResFileType::Material;

                                //Generating the files (hght & mate)
                                {
                                    SubSectionSize = 260 / AreaLOD;

                                    application::manager::TerrainMgr::TerrainScene::ArchivePack* ArchivePack = application::manager::TerrainMgr::GetArchivePack(ParentArea->mFilenameOffset.mResString.mString, *TerrainScene);

                                    if (ArchivePack == nullptr)
                                    {
                                        application::util::Logger::Error("TerrainRenderer", "Failed to get archive pack for tile: %s", ParentArea->mFilenameOffset.mResString.mString.c_str());
                                        continue;
                                    }

                                    application::file::game::terrain::HghtFile& Hght = ArchivePack->mHghtArchive.mHeightMaps[ParentArea->mFilenameOffset.mResString.mString];
                                    application::file::game::terrain::MateFile& Mate = ArchivePack->mMateArchive.mMateFiles[ParentArea->mFilenameOffset.mResString.mString];

                                    int AreaTileScale = BaseTileWidth * (ParentArea->mScale / 0.125f);
                                    glm::vec2 StartTilePos = TilePos - glm::vec2(0.5f * BaseTileWidth, 0.5f * BaseTileWidth);
                                    glm::vec2 AreaPos = glm::vec2(ParentArea->mX * 1000.0f * 0.5f, ParentArea->mZ * 1000.0f * 0.5f);
                                    glm::vec2 StartAreaPos = AreaPos - glm::vec2(0.5f * AreaTileScale, 0.5f * AreaTileScale);

                                    int BeginX = (StartTilePos.x - StartAreaPos.x) / BaseTileWidth * 128;
                                    int BeginY = (StartTilePos.y - StartAreaPos.y) / BaseTileWidth * 128;
                                    int EndX = BeginX + SubSectionSize;
                                    int EndY = BeginY + SubSectionSize;

                                    application::file::game::terrain::HghtFile CopyHght;
                                    application::file::game::terrain::MateFile CopyMate;
                                    CopyHght.mWidth = 260;
                                    CopyHght.mHeight = 260;
                                    CopyHght.mHeightMap.resize(CopyHght.mWidth * CopyHght.mHeight);

                                    CopyMate.mWidth = 260;
                                    CopyMate.mHeight = 260;
                                    CopyMate.mMaterials.resize(CopyMate.mWidth * CopyMate.mHeight);

                                    const float RealFactor = (500 * requiredScale) / (500 * ParentArea->mScale);

                                    for (uint16_t heightX = 0; heightX < 260; heightX++)
                                    {
                                        for (uint16_t heightY = 0; heightY < 260; heightY++)
                                        {
                                            float SourceX = heightX * RealFactor;
                                            float SourceY = heightY * RealFactor;

                                            int x0 = static_cast<int>(std::floor(SourceX));
                                            int x1 = std::min(x0 + 1, SubSectionSize - 1);
                                            int y0 = static_cast<int>(std::floor(SourceY));
                                            int y1 = std::min(y0 + 1, SubSectionSize - 1);

                                            float fx = SourceX - x0;
                                            float fy = SourceY - y0;

                                            float h00 = Hght.GetHeightAtGridPos(BeginX + x0, BeginY + y0);
                                            float h10 = Hght.GetHeightAtGridPos(BeginX + x1, BeginY + y0);
                                            float h01 = Hght.GetHeightAtGridPos(BeginX + x0, BeginY + y1);
                                            float h11 = Hght.GetHeightAtGridPos(BeginX + x1, BeginY + y1);

                                            float h0 = h00 * (1.0f - fx) + h10 * fx;
                                            float h1 = h01 * (1.0f - fx) + h11 * fx;

                                            const size_t Index = heightX + CopyHght.GetWidth() * heightY;
                                            CopyHght.mHeightMap[Index] = static_cast<uint16_t>(std::round(h0 * (1.0f - fy) + h1 * fy));



                                            application::file::game::terrain::MateFile::MaterialIndexInfo Material00 = Mate.mMaterials[(BeginX + x0) + (BeginY + y0) * Mate.mWidth];

                                            application::file::game::terrain::MateFile::MaterialIndexInfo Material10 = (BeginX + x0) < (Mate.mWidth - 1) ?
                                                Mate.mMaterials[(BeginX + x0) + 1 + (BeginY + y0) * Mate.mWidth] : Material00;

                                            application::file::game::terrain::MateFile::MaterialIndexInfo Material01 = (BeginY + y0) < (Mate.mHeight - 1) ?
                                                Mate.mMaterials[(BeginX + x0) + ((BeginY + y0) + 1) * Mate.mWidth] : Material00;

                                            application::file::game::terrain::MateFile::MaterialIndexInfo Material11 = (((BeginY + y0) < (Mate.mHeight - 1)) && ((BeginX + x0) < (Mate.mWidth - 1))) ?
                                                Mate.mMaterials[(BeginX + x0) + 1 + ((BeginY + y0) + 1) * Mate.mWidth] : Material00;

                                            std::map<uint8_t, float> textureWeights;

                                            float weight00 = (1 - fx) * (1 - fy);
                                            textureWeights[Material00.mIndexA] += weight00 * (255 - Material00.mBlendFactor) / 255.0f;
                                            textureWeights[Material00.mIndexB] += weight00 * Material00.mBlendFactor / 255.0f;

                                            float weight10 = fx * (1 - fy);
                                            textureWeights[Material10.mIndexA] += weight10 * (255 - Material10.mBlendFactor) / 255.0f;
                                            textureWeights[Material10.mIndexB] += weight10 * Material10.mBlendFactor / 255.0f;

                                            float weight01 = (1 - fx) * fy;
                                            textureWeights[Material01.mIndexA] += weight01 * (255 - Material01.mBlendFactor) / 255.0f;
                                            textureWeights[Material01.mIndexB] += weight01 * Material01.mBlendFactor / 255.0f;

                                            float weight11 = fx * fy;
                                            textureWeights[Material11.mIndexA] += weight11 * (255 - Material11.mBlendFactor) / 255.0f;
                                            textureWeights[Material11.mIndexB] += weight11 * Material11.mBlendFactor / 255.0f;

                                            uint8_t dominantTextureA = 0;
                                            uint8_t dominantTextureB = 0;
                                            float maxWeightA = 0.0f;
                                            float maxWeightB = 0.0f;

                                            for (const auto& pair : textureWeights)
                                            {
                                                if (pair.second > maxWeightA)
                                                {
                                                    maxWeightB = maxWeightA;
                                                    dominantTextureB = dominantTextureA;
                                                    maxWeightA = pair.second;
                                                    dominantTextureA = pair.first;
                                                }
                                                else if (pair.second > maxWeightB)
                                                {
                                                    maxWeightB = pair.second;
                                                    dominantTextureB = pair.first;
                                                }
                                            }

                                            // Calculate blend factor between the two most dominant textures
                                            uint8_t blendFactor = 0;
                                            if (maxWeightA + maxWeightB > 0)
                                            {
                                                blendFactor = static_cast<uint8_t>(std::round((maxWeightB / (maxWeightA + maxWeightB)) * 255.0f));
                                            }

                                            // Create the new material info for this point
                                            application::file::game::terrain::MateFile::MaterialIndexInfo newMaterial;
                                            newMaterial.mIndexA = dominantTextureA;
                                            newMaterial.mIndexB = dominantTextureB;
                                            newMaterial.mBlendFactor = blendFactor;

                                            CopyMate.mMaterials[Index] = newMaterial;
                                        }
                                    }

                                    //application::file::game::terrain::HghtFile CopyHght = CopyArchivePack.mHghtArchive.mHeightMaps["56000003F0"];
                                    //application::file::game::terrain::MateFile CopyMate = CopyArchivePack.mMateArchive.mMateFiles["56000003F0"];

                                    CopyHght.mModified = true;
                                    CopyMate.mModified = true;

                                    std::string Name = NewArea.mFilenameOffset.mResString.mString;
                                    const uint64_t NameInt = std::floor(std::stoull(Name, nullptr, 16) / 4) * 4;

                                    std::stringstream Stream;
                                    Stream << std::setfill('0') << std::setw(16) << std::uppercase
                                        << std::hex << NameInt;
                                    Name = Stream.str();
                                    Name = Name.substr(Name.length() - 9);

                                    TerrainScene->mArchives[Name].mHghtArchive.mHeightMaps[NewArea.mFilenameOffset.mResString.mString] = CopyHght;
                                    TerrainScene->mArchives[Name].mMateArchive.mMateFiles[NewArea.mFilenameOffset.mResString.mString] = CopyMate;
                                }

                                //Calculating the min and max values of the HGHT and setting it in the height map resource
                                application::manager::TerrainMgr::TerrainScene::ArchivePack* ArchivePack = application::manager::TerrainMgr::GetArchivePack(NewArea.mFilenameOffset.mResString.mString, *TerrainScene);
                                application::file::game::terrain::HghtFile& Hght = ArchivePack->mHghtArchive.mHeightMaps[NewArea.mFilenameOffset.mResString.mString];

                                uint16_t Min = 0xFFFF;
                                uint16_t Max = 0x0000;
                                for (uint16_t Data : Hght.mHeightMap)
                                {
                                    Min = std::min(Min, Data);
                                    Max = std::max(Max, Data);
                                }

                                HeightMapFile.mMinHeight = Min / 65535.0f;
                                HeightMapFile.mMaxHeight = Max / 65535.0f;

                                //Creating the file resource pointers
                                application::file::game::terrain::TerrainSceneFile::RelPtr<application::file::game::terrain::TerrainSceneFile::ResFile> HeightMapFilePtr;
                                HeightMapFilePtr.mObj = HeightMapFile;

                                application::file::game::terrain::TerrainSceneFile::RelPtr<application::file::game::terrain::TerrainSceneFile::ResFile> NormalMapFilePtr;
                                NormalMapFilePtr.mObj = NormalMapFile;

                                application::file::game::terrain::TerrainSceneFile::RelPtr<application::file::game::terrain::TerrainSceneFile::ResFile> MaterialMapFilePtr;
                                MaterialMapFilePtr.mObj = MaterialMapFile;

                                //Adding the file resources
                                NewArea.mFileResources.push_back(HeightMapFilePtr);
                                NewArea.mFileResources.push_back(NormalMapFilePtr);
                                NewArea.mFileResources.push_back(MaterialMapFilePtr);


                                //Creating the new area pointer
                                application::file::game::terrain::TerrainSceneFile::RelPtr<application::file::game::terrain::TerrainSceneFile::ResArea> NewAreaPtr;
                                NewAreaPtr.mObj = NewArea;

                                //Adding the new area pointer to the TSCB
                                TerrainScene->mTerrainSceneFile.mTerrainScene.mAreas.push_back(NewAreaPtr);
                                TerrainScene->mTerrainSceneFile.mTerrainScene.mAreaCount += 1;
                                TerrainScene->mTerrainSceneFile.mModified = true;

								ParentArea = &TerrainScene->mTerrainSceneFile.mTerrainScene.mAreas.back().mObj;

                                Tiles = TerrainScene->mTerrainSceneFile.GetSectionTilesByPos(32.0f, SectionMidPoint, 1000.0f);

                                application::util::Logger::Info("TerrainRenderer",
                                    "Tile (%d, %d) requires new ResArea (Scale: %.3f, Error: %.2f)",
                                    x, y, requiredScale, maxTileError);
                            }
                        }
                    }
                }

				std::cout << "Tile (" << x << ", " << y << ") - Max Error: " << maxTileError << std::endl;
            }
        }

		//2nd iteration, applying height data to all tiles in the section
        for (int x = 0; x < 16; x++)
        {
            for (int y = 0; y < 16; y++)
            {
                if (!mModifiedTiles.contains(x + y * 16))
                    continue;

                const glm::vec2 TilePos = SectionStartPoint + glm::vec2(x * BaseTileWidth, y * BaseTileWidth) + glm::vec2(0.5f * BaseTileWidth, 0.5f * BaseTileWidth);

				float CurrentScale = 0.125f;

                while (CurrentScale <= 32.0f)
                {
					float TileWidth = BaseTileWidth * (CurrentScale / 0.125f);

                    for (application::file::game::terrain::TerrainSceneFile::ResArea* Area : Tiles)
                    {
                        if (Area->mScale != CurrentScale)
                            continue;

						const glm::vec2 AreaPos = glm::vec2(Area->mX * 1000.0f * 0.5f, Area->mZ * 1000.0f * 0.5f);
						const glm::vec2 AreaStartPos = AreaPos - glm::vec2(0.5f * TileWidth, 0.5f * TileWidth);
						const glm::vec2 AreaEndPos = AreaPos + glm::vec2(0.5f * TileWidth, 0.5f * TileWidth);

                        //Found area that covers tile
                        if (TilePos.x > AreaStartPos.x && TilePos.x < AreaEndPos.x &&
                            TilePos.y > AreaStartPos.y && TilePos.y < AreaEndPos.y)
                        {
                            // Get the archive pack for this tile
                            application::manager::TerrainMgr::TerrainScene::ArchivePack* ArchivePack =
                                application::manager::TerrainMgr::GetArchivePack(Area->mFilenameOffset.mResString.mString, *TerrainScene);

                            if (ArchivePack == nullptr) {
                                application::util::Logger::Error("TerrainRenderer", "Failed to get archive pack for saving tile: %s",
                                    Area->mFilenameOffset.mResString.mString.c_str());
                                continue;
                            }

                            application::file::game::terrain::HghtFile& Hght = ArchivePack->mHghtArchive.mHeightMaps[Area->mFilenameOffset.mResString.mString];

                            const glm::vec2 StartTilePos = TilePos - glm::vec2(0.5f * BaseTileWidth, 0.5f * BaseTileWidth);
                            const int SubSectionSize = 256 / (CurrentScale / 0.125f);

                            int BeginX = (StartTilePos.x - AreaStartPos.x) / TileWidth * 256;
                            int BeginY = (StartTilePos.y - AreaStartPos.y) / TileWidth * 256;
                            int EndX = BeginX + SubSectionSize;
                            int EndY = BeginY + SubSectionSize;
                            
                            for (int AreaX = BeginX - 2; AreaX < EndX + 2; AreaX++)
                            {
                                for (int AreaY = BeginY - 2; AreaY < EndY + 2; AreaY++)
                                {
                                    // Map from ResArea coordinates back to world coordinates, then to texture coordinates
                                    float worldX = AreaStartPos.x + (AreaX / 256.0f) * TileWidth;
                                    float worldY = AreaStartPos.y + (AreaY / 256.0f) * TileWidth;

                                    // Convert world coordinates to texture coordinates
                                    int TextureX = (worldX - SectionStartPoint.x) / 1000.0f * (256 * 16);
                                    int TextureY = (worldY - SectionStartPoint.y) / 1000.0f * (256 * 16);

                                    // Skip if the texture coordinates are outside our texture bounds
                                    // (This happens for margin areas on edge tiles)
                                    if (TextureX < 0 || TextureX >= (256 * 16) ||
                                        TextureY < 0 || TextureY >= (256 * 16))
                                    {
                                        continue; // Skip margin areas that have no data
                                    }

                                    int TextureIndex = TextureY * (256 * 16) + TextureX;

                                    if (TextureIndex >= 0 && TextureIndex < mHeightTextureData.size())
                                    {
                                        uint16_t HeightValue = static_cast<uint16_t>(mHeightTextureData[TextureIndex] / mHeightScale);

                                        // Ensure we don't write outside the height map bounds
                                        int hghtX = AreaX + 2;
                                        int hghtY = AreaY + 2;

                                        if (hghtX >= 0 && hghtX < Hght.GetWidth() &&
                                            hghtY >= 0 && hghtY < Hght.GetHeight())
                                        {
                                            Hght.mHeightMap[hghtX + hghtY * Hght.GetWidth()] = HeightValue;
                                            Hght.mModified = true;
                                        }
                                    }
                                }
                            }

                            uint16_t Min = 0xFFFF;
                            uint16_t Max = 0x0000;
                            for (uint16_t Data : Hght.mHeightMap)
                            {
                                Min = std::min(Min, Data);
                                Max = std::max(Max, Data);
                            }

                            for (auto& FileRes : Area->mFileResources)
                            {
                                if (FileRes.mObj.mType == application::file::game::terrain::TerrainSceneFile::ResFileType::HeightMap)
                                {
                                    FileRes.mObj.mMinHeight = Min / 65535.0f;
                                    FileRes.mObj.mMaxHeight = Max / 65535.0f;
                                    TerrainScene->mTerrainSceneFile.mModified = true;
                                }
                            }

                            application::util::Logger::Info("TerrainRenderer", "Saved height data for additional ResArea: %s (Scale: %.3f)",
                                Area->mFilenameOffset.mResString.mString.c_str(), CurrentScale);

                            break;
                        }
                    }

					CurrentScale *= 2.0f;
                }
            }
        }

		std::unordered_set<int> ModifiedCompounds;
		application::tool::scene::static_compound::StaticCompoundImplementationFieldScene* FieldSceneImplCast = static_cast<application::tool::scene::static_compound::StaticCompoundImplementationFieldScene*>(FieldSceneImpl);

		//3rd iteration, remaking collision for modified tiles
        for (int x = 0; x < 16; x++)
        {
            for (int y = 0; y < 16; y++)
            {
                if (!mModifiedTiles.contains(x + y * 16) && !mModifiedTextureTiles.contains(x + y * 16))
                    continue;

                // Calculate which 4x4 compound this tile belongs to
                int CompoundX = x / 4;  // 0, 1, 2, 3 for x values 0-15
                int CompoundY = y / 4;  // 0, 1, 2, 3 for y values 0-15

                if (ModifiedCompounds.contains(CompoundX + CompoundY * 4))
                    continue;

                ModifiedCompounds.insert(CompoundX + CompoundY * 4);

                glm::vec3 StartPoint = FieldSceneImplCast->GetStaticCompoundMiddlePoint(CompoundX, CompoundY);

                application::file::game::phive::shape::PhiveShape::GeneratorData Generator;

                std::pair<std::vector<glm::vec3>, std::vector<std::pair<std::tuple<uint32_t, uint32_t, uint32_t>, uint32_t>>> Model = GenerateTerrainTileCollisionModel(glm::vec2(StartPoint.x, StartPoint.z), &Generator.mMaterials);

                std::unordered_map<uint32_t, std::vector<uint32_t>> Indices;
                for (auto& [Face, Material] : Model.second)
                {
                    Indices[Material].push_back(std::get<0>(Face));
                    Indices[Material].push_back(std::get<1>(Face));
                    Indices[Material].push_back(std::get<2>(Face));
                }

                std::unordered_map<uint32_t, std::vector<uint32_t>> SimplifiedIndices;
                for (auto& [Material, Faces] : Indices)
                {
                    float ResultError = 0.0f;
                    SimplifiedIndices[Material].resize(Faces.size());

                    size_t original_triangle_count = Faces.size() / 3;

                    // Don't simplify meshes that are already very simple (< 10 triangles)
                    if (original_triangle_count < 10) 
                    {
                        SimplifiedIndices[Material] = Faces;
                    }
                    else 
                    {
                        // Target: reduce to ~50% of original triangles, but never below 6 indices (2 triangles)
                        size_t target_index_count = std::max(Faces.size() / 2, static_cast<size_t>(6));

                        // Use a reasonable error threshold (1% of bounding box diagonal)
                        glm::vec3 bboxMin = Model.first[0];
                        glm::vec3 bboxMax = Model.first[0];
                        for (const auto& vertex : Model.first) {
                            bboxMin = glm::min(bboxMin, vertex);
                            bboxMax = glm::max(bboxMax, vertex);
                        }
                        float bboxDiagonal = glm::length(bboxMax - bboxMin);
                        float target_error = bboxDiagonal * 0.01f; // 1% of diagonal

                        // Pre-allocate output buffer
                        SimplifiedIndices[Material].resize(Faces.size());

                        size_t simplified_count = meshopt_simplify(
                            &SimplifiedIndices[Material][0],
                            &Faces[0],
                            Faces.size(),
                            &Model.first[0].x,
                            Model.first.size(),
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
                            SimplifiedIndices[Material] = Faces;
                        }
                        else 
                        {
                            SimplifiedIndices[Material].resize(simplified_count);
                        }
                    }

                    application::util::Logger::Info("TerrainMgr", "Reduced collision mesh from %u to %u with an error of %f", Faces.size(), SimplifiedIndices[Material].size(), ResultError);
                }

                Model.second.clear();
                for (auto& [Material, Faces] : SimplifiedIndices)
                {
                    for (size_t i = 0; i < Faces.size(); i += 3)
                    {
                        Model.second.push_back(std::make_pair(std::make_tuple(Faces[i], Faces[i + 1], Faces[i + 2]), Material));
                    }
                }

                Generator.mVertices = Model.first;
                Generator.mIndices = Model.second;

                Generator.mRunMeshOptimizer = false;

                application::file::game::phive::PhiveStaticCompoundFile* Compound = FieldSceneImplCast->GetStaticCompoundAtIndex(CompoundX, CompoundY);

                application::file::game::phive::shape::PhiveShape& PhiveShape = Compound->mExternalBphshMeshes[0].mPhiveShape;
                Compound->mExternalBphshMeshes[0].mReserializeCollision = true;

                //application::util::FileUtil::WriteFile(application::util::FileUtil::GetWorkingDirFilePath("TerrainMgrOriginalMesh.bphsh"), PhiveShape.ToBinary());
                PhiveShape.InjectModel(Generator);
                //application::util::FileUtil::WriteFile(application::util::FileUtil::GetWorkingDirFilePath("TerrainMgrNewMesh.bphsh"), PhiveShape.ToBinary());

                FieldSceneImplCast->mModified[CompoundX + CompoundY * 4] = true;
            }
        }

        mModifiedTiles.clear();
        mModifiedTextureTiles.clear();

        for(auto& [Name, ArchivePack] : TerrainScene->mArchives)
        {
            if(ArchivePack.mHghtArchive.mHeightMaps.empty())
                continue;

            bool SaveHght = false;
            bool SaveMate = false;

            for(auto& [Name, File] : ArchivePack.mHghtArchive.mHeightMaps)
            {
                if(File.mModified)
                {
                    SaveHght = true;
                    File.mModified = false;
				}
            }

            for (auto& [Name, File] : ArchivePack.mMateArchive.mMateFiles)
            {
                if (File.mModified)
                {
                    SaveMate = true;
                    File.mModified = false;
                }
            }

            if (SaveHght)
            {
                std::filesystem::create_directories(application::util::FileUtil::GetSaveFilePath("TerrainArc/" + mSceneName));
                application::util::FileUtil::WriteFile(application::util::FileUtil::GetSaveFilePath("TerrainArc/" + mSceneName + "/" + Name + ".hght.ta.zs"), application::file::game::ZStdBackend::Compress(ArchivePack.mHghtArchive.ToBinary(), application::file::game::ZStdBackend::Dictionary::None));
            }

            if (SaveMate)
            {
                std::filesystem::create_directories(application::util::FileUtil::GetSaveFilePath("TerrainArc/" + mSceneName));
                application::util::FileUtil::WriteFile(application::util::FileUtil::GetSaveFilePath("TerrainArc/" + mSceneName + "/" + Name + ".mate.ta.zs"), application::file::game::ZStdBackend::Compress(ArchivePack.mMateArchive.ToBinary(), application::file::game::ZStdBackend::Dictionary::None));
            }
		}

        if (TerrainScene->mTerrainSceneFile.mModified)
        {
            std::filesystem::create_directories(application::util::FileUtil::GetSaveFilePath("TerrainArc"));
            application::util::FileUtil::WriteFile(application::util::FileUtil::GetSaveFilePath("TerrainArc/" + mSceneName + ".tscb"), TerrainScene->mTerrainSceneFile.ToBinary());
            TerrainScene->mTerrainSceneFile.mModified = false;
        }

        FieldSceneImplCast->ReloadMeshes();

        application::util::Logger::Info("TerrainRenderer", "Terrain height data saved for section: %s", mSectionName.c_str());
    }

	std::vector<float> TerrainRenderer::GenerateTerrainVertices(const float Width, const float Height)
	{
        const unsigned int Patches = 16;

        std::vector<float> Vertices;
        Vertices.resize(Patches * Patches * 20);

        for (unsigned i = 0; i < Patches; i++)
        {
            for (unsigned j = 0; j < Patches; j++)
            {
                Vertices.push_back(-Width / 2.0f + Width * i / (float)Patches); // v.x
                Vertices.push_back(0.0f); // v.y
                Vertices.push_back(-Height / 2.0f + Height * j / (float)Patches); // v.z
                Vertices.push_back(i / (float)Patches); // u
                Vertices.push_back(j / (float)Patches); // v

                Vertices.push_back(-Width / 2.0f + Width * (i + 1) / (float)Patches); // v.x
                Vertices.push_back(0.0f); // v.y
                Vertices.push_back(-Height / 2.0f + Height * j / (float)Patches); // v.z
                Vertices.push_back((i + 1) / (float)Patches); // u
                Vertices.push_back(j / (float)Patches); // v

                Vertices.push_back(-Width / 2.0f + Width * i / (float)Patches); // v.x
                Vertices.push_back(0.0f); // v.y
                Vertices.push_back(-Height / 2.0f + Height * (j + 1) / (float)Patches); // v.z
                Vertices.push_back(i / (float)Patches); // u
                Vertices.push_back((j + 1) / (float)Patches); // v

                Vertices.push_back(-Width / 2.0f + Width * (i + 1) / (float)Patches); // v.x
                Vertices.push_back(0.0f); // v.y
                Vertices.push_back(-Height / 2.0f + Height * (j + 1) / (float)Patches); // v.z
                Vertices.push_back((i + 1) / (float)Patches); // u
                Vertices.push_back((j + 1) / (float)Patches); // v
            }
        }

        Vertices.shrink_to_fit();
        return Vertices;
	}
}