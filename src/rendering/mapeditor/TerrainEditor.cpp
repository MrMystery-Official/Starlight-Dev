#include <rendering/mapeditor/TerrainEditor.h>

#include <rendering/mapeditor/UIMapEditor.h>
#include <util/Math.h>
#include <manager/ShaderMgr.h>
#include <manager/TexToGoFileMgr.h>
#include <rendering/ImGuiExt.h>
#include <file/game/texture/FormatHelper.h>
#include "imgui_internal.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace application::rendering::map_editor
{
    namespace
    {
        constexpr int kTerrainTextureSize = 256 * 16;

        float GetBrushWeight(TerrainEditor* Editor, float Distance, float CorrectedRadius)
        {
            float Weight = 1.0f;
            switch (Editor->mFallOffFunction)
            {
            case TerrainEditor::FallOffFunction::GAUSSIAN:
                Weight = TerrainMath::GaussianFallOff(Distance, CorrectedRadius, Editor->mFallOffRate);
                break;
            case TerrainEditor::FallOffFunction::LINEAR:
                Weight = TerrainMath::LinearFallOff(Distance, CorrectedRadius, Editor->mFallOffRate);
                break;
            case TerrainEditor::FallOffFunction::NONE:
            default:
                break;
            }
            return std::clamp(Weight, 0.0f, 1.0f);
        }

        uint32_t HashCoord(const int x, const int y)
        {
            uint32_t v = static_cast<uint32_t>(x) * 374761393u + static_cast<uint32_t>(y) * 668265263u;
            v = (v ^ (v >> 13)) * 1274126177u;
            return v ^ (v >> 16);
        }

        float SolveBrushHalfStrengthRadius(TerrainEditor* Editor, float Radius)
        {
            if (Radius <= 0.0f)
                return 0.0f;

            if (Editor->mFallOffFunction == TerrainEditor::FallOffFunction::NONE)
                return Radius;

            float bestDistance = Radius;
            float bestError = std::numeric_limits<float>::max();

            for (int i = 0; i <= 64; ++i)
            {
                const float d = Radius * (static_cast<float>(i) / 64.0f);
                const float w = GetBrushWeight(Editor, d, Radius);
                const float err = std::abs(w - 0.5f);
                if (err < bestError)
                {
                    bestError = err;
                    bestDistance = d;
                }
            }

            return bestDistance;
        }

        bool ComputeTexturePreviewAtCenter(
            TerrainEditor* Editor,
            application::gl::TerrainRenderer* TerrainRenderer,
            uint16_t CorrectedRadius,
            int TexX, int TexY,
            TerrainEditor::TerrainTextureEdit& OutEdit)
        {
            if (TexX < 0 || TexX >= kTerrainTextureSize || TexY < 0 || TexY >= kTerrainTextureSize)
                return false;

            constexpr float kBlendScale = 255.0f;
            const size_t index = static_cast<size_t>(TexX) + static_cast<size_t>(kTerrainTextureSize) * static_cast<size_t>(TexY);
            const size_t layerIndex = index * 3;

            uint8_t layerA = TerrainRenderer->mLayersTextureData[layerIndex];
            uint8_t layerB = TerrainRenderer->mLayersTextureData[layerIndex + 1];
            uint8_t blend = TerrainRenderer->mLayersTextureData[layerIndex + 2];

            const float paintAlpha = std::clamp(Editor->mStrength, 0.0f, 1.0f);
            if (paintAlpha <= 0.0f)
                return false;

            if (Editor->mCurrentTextureLayer >= 120)
            {
                OutEdit.mGridX = static_cast<int16_t>(TexX);
                OutEdit.mGridZ = static_cast<int16_t>(TexY);
                OutEdit.mTextureLayerA = 120;
                OutEdit.mTextureLayerB = 120;
                OutEdit.mBlend = 0;
                return true;
            }

            auto chooseOutputLayers = [Editor](std::pair<uint8_t, float> p0, std::pair<uint8_t, float> p1) -> std::pair<std::pair<uint8_t, float>, std::pair<uint8_t, float>>
            {
                if (Editor->mTextureLayerMode == TerrainEditor::TextureLayerMode::LAYER_A && p1.first == Editor->mCurrentTextureLayer)
                    std::swap(p0, p1);
                if (Editor->mTextureLayerMode == TerrainEditor::TextureLayerMode::LAYER_B && p0.first == Editor->mCurrentTextureLayer)
                    std::swap(p0, p1);
                return { p0, p1 };
            };

            float weightA = (kBlendScale - static_cast<float>(blend)) / kBlendScale;
            float weightB = static_cast<float>(blend) / kBlendScale;
            weightA = std::clamp(weightA, 0.0f, 1.0f);
            weightB = std::clamp(weightB, 0.0f, 1.0f);

            std::array<std::pair<uint8_t, float>, 3> weights = {
                std::make_pair(layerA, weightA),
                std::make_pair(layerB, weightB),
                std::make_pair(static_cast<uint8_t>(Editor->mCurrentTextureLayer), 0.0f)
            };

            if (weights[0].first == weights[2].first) weights[2].second += weights[0].second;
            if (weights[1].first == weights[2].first) weights[2].second += weights[1].second;

            const float totalExisting = weights[0].second + weights[1].second;
            if (totalExisting <= 0.0f)
            {
                weights[0] = { static_cast<uint8_t>(Editor->mCurrentTextureLayer), 1.0f };
                weights[1] = weights[0];
                weights[2] = weights[0];
            }

            if (Editor->mTextureBrushType == TerrainEditor::TextureBrushType::REPLACE)
            {
                weights = {
                    std::make_pair(static_cast<uint8_t>(Editor->mCurrentTextureLayer), 1.0f),
                    std::make_pair(static_cast<uint8_t>(Editor->mCurrentTextureLayer), 0.0f),
                    std::make_pair(static_cast<uint8_t>(Editor->mCurrentTextureLayer), 1.0f)
                };
            }
            else if (Editor->mTextureBrushType == TerrainEditor::TextureBrushType::SMOOTH)
            {
                const int neighborhood = std::max(1, static_cast<int>(CorrectedRadius / 3));
                std::array<float, 121> histogram{};
                float sum = 0.0f;

                for (int nz = -neighborhood; nz <= neighborhood; ++nz)
                {
                    for (int nx = -neighborhood; nx <= neighborhood; ++nx)
                    {
                        const int sx = TexX + nx;
                        const int sz = TexY + nz;
                        if (sx < 0 || sx >= kTerrainTextureSize || sz < 0 || sz >= kTerrainTextureSize)
                            continue;

                        const size_t sampleIndex = (static_cast<size_t>(sx) + static_cast<size_t>(kTerrainTextureSize) * static_cast<size_t>(sz)) * 3;
                        const uint8_t sa = TerrainRenderer->mLayersTextureData[sampleIndex];
                        const uint8_t sb = TerrainRenderer->mLayersTextureData[sampleIndex + 1];
                        const float sblend = static_cast<float>(TerrainRenderer->mLayersTextureData[sampleIndex + 2]) / kBlendScale;
                        const float wa = 1.0f - sblend;
                        const float wb = sblend;
                        if (sa < histogram.size()) histogram[sa] += wa;
                        if (sb < histogram.size()) histogram[sb] += wb;
                        sum += wa + wb;
                    }
                }

                if (sum > 0.0f)
                {
                    for (auto& h : histogram) h /= sum;
                    const float target = histogram[Editor->mCurrentTextureLayer];
                    weights[2].second = std::clamp(weights[2].second + (target - weights[2].second) * paintAlpha, 0.0f, 1.0f);
                }
            }
            else
            {
                weights[2].second = std::clamp(weights[2].second + (1.0f - weights[2].second) * paintAlpha, 0.0f, 1.0f);
            }

            const float remaining = std::max(0.0f, 1.0f - weights[2].second);
            float existingNoSelected = 0.0f;
            if (weights[0].first != weights[2].first) existingNoSelected += weights[0].second;
            if (weights[1].first != weights[2].first) existingNoSelected += weights[1].second;

            if (existingNoSelected > 0.0f)
            {
                if (weights[0].first != weights[2].first) weights[0].second = (weights[0].second / existingNoSelected) * remaining;
                else weights[0].second = 0.0f;
                if (weights[1].first != weights[2].first) weights[1].second = (weights[1].second / existingNoSelected) * remaining;
                else weights[1].second = 0.0f;
            }
            else
            {
                weights[0] = weights[2];
                weights[1] = weights[2];
                weights[0].second = remaining;
                weights[1].second = 0.0f;
            }

            std::array<std::pair<uint8_t, float>, 3> ranked = weights;
            std::sort(ranked.begin(), ranked.end(), [](const auto& a, const auto& b) { return a.second > b.second; });
            auto [primary, secondary] = chooseOutputLayers(ranked[0], ranked[1]);

            float norm = primary.second + secondary.second;
            if (norm <= 1e-6f)
            {
                primary = { static_cast<uint8_t>(Editor->mCurrentTextureLayer), 1.0f };
                secondary = primary;
                norm = 1.0f;
            }

            OutEdit.mGridX = static_cast<int16_t>(TexX);
            OutEdit.mGridZ = static_cast<int16_t>(TexY);
            OutEdit.mTextureLayerA = primary.first;
            OutEdit.mTextureLayerB = secondary.first;
            OutEdit.mBlend = static_cast<uint8_t>(std::clamp((secondary.second / norm) * kBlendScale, 0.0f, 255.0f));
            return true;
        }
    }

	TerrainRaycast::TerrainRaycast(application::gl::TerrainRenderer* Renderer) : mTerrainRenderer(Renderer)
    {

    }

	float TerrainRaycast::SampleHeightAtWorldPos(const glm::vec2& worldPos) const
	{
        // Convert world position to terrain local coordinates [0,1]
        // Terrain spans from (center - 500) to (center + 500)
        glm::vec2 terrainMin = glm::vec2(mTerrainRenderer->mTerrainPosition.x - 500.0f, mTerrainRenderer->mTerrainPosition.z - 500.0f);
        glm::vec2 terrainMax = glm::vec2(mTerrainRenderer->mTerrainPosition.x + 500.0f, mTerrainRenderer->mTerrainPosition.z + 500.0f);

        glm::vec2 localPos = (worldPos - terrainMin) / (terrainMax - terrainMin);

        // Check bounds
        if (localPos.x < 0.0f || localPos.x > 1.0f || localPos.y < 0.0f || localPos.y > 1.0f) {
            return -1000.0f; // Return very low value for outside bounds
        }

        // Convert to texture coordinates
        const int textureSize = 256 * 16; // 4096
        float texX = localPos.x * (textureSize - 1);
        float texY = localPos.y * (textureSize - 1);

        // Get integer coordinates
        int x0 = static_cast<int>(std::floor(texX));
        int y0 = static_cast<int>(std::floor(texY));
        int x1 = std::min(x0 + 1, textureSize - 1);
        int y1 = std::min(y0 + 1, textureSize - 1);

        // Get interpolation weights
        float fx = texX - x0;
        float fy = texY - y0;

        // Sample four heights
        float h00 = mTerrainRenderer->mHeightTextureData[y0 * textureSize + x0];
        float h10 = mTerrainRenderer->mHeightTextureData[y0 * textureSize + x1];
        float h01 = mTerrainRenderer->mHeightTextureData[y1 * textureSize + x0];
        float h11 = mTerrainRenderer->mHeightTextureData[y1 * textureSize + x1];

        // Bilinear interpolation
        float h0 = h00 * (1.0f - fx) + h10 * fx;
        float h1 = h01 * (1.0f - fx) + h11 * fx;
        return h0 * (1.0f - fy) + h1 * fy;
    }

    glm::vec3 TerrainRaycast::CalculateNormalAtWorldPos(const glm::vec2& worldPos) const 
    {
        const float epsilon = 1.0f; // Small offset for derivative calculation

        float heightCenter = SampleHeightAtWorldPos(worldPos);
        float heightRight = SampleHeightAtWorldPos(worldPos + glm::vec2(epsilon, 0));
        float heightUp = SampleHeightAtWorldPos(worldPos + glm::vec2(0, epsilon));

        glm::vec3 tangentRight(epsilon, heightRight - heightCenter, 0);
        glm::vec3 tangentUp(0, heightUp - heightCenter, epsilon);

        return glm::normalize(glm::cross(tangentRight, tangentUp));
    }

    bool TerrainRaycast::RayAABBIntersect(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
        const glm::vec3& boxMin, const glm::vec3& boxMax,
        float& tMin, float& tMax) const 
    {
        tMin = 0.0f;
        tMax = std::numeric_limits<float>::max();

        for (int i = 0; i < 3; i++) {
            if (std::abs(rayDir[i]) < 1e-6f) {
                // Ray is parallel to slab
                if (rayOrigin[i] < boxMin[i] || rayOrigin[i] > boxMax[i]) {
                    return false;
                }
            }
            else {
                float t1 = (boxMin[i] - rayOrigin[i]) / rayDir[i];
                float t2 = (boxMax[i] - rayOrigin[i]) / rayDir[i];

                if (t1 > t2) std::swap(t1, t2);

                tMin = std::max(tMin, t1);
                tMax = std::min(tMax, t2);

                if (tMin > tMax) return false;
            }
        }

        return true;
    }

    TerrainRaycast::RaycastHit TerrainRaycast::RaycastTerrain(const glm::vec3& rayOrigin, const glm::vec3& rayDirection) const
    {
        RaycastHit result;
        result.mHit = false;

        // Terrain bounds: center +/- 500 in X and Z, 0 to reasonable max height in Y
        glm::vec3 terrainMin = glm::vec3(mTerrainRenderer->mTerrainPosition.x - 500.0f, -100.0f, mTerrainRenderer->mTerrainPosition.z - 500.0f);
        glm::vec3 terrainMax = glm::vec3(mTerrainRenderer->mTerrainPosition.x + 500.0f, 1000.0f, mTerrainRenderer->mTerrainPosition.z + 500.0f);

        float tMin, tMax;
        if (!RayAABBIntersect(rayOrigin, rayDirection, terrainMin, terrainMax, tMin, tMax)) {
            return result;
        }

        // Ensure we start at a valid position
        tMin = std::max(0.0f, tMin);

        // Step along the ray with adaptive step size
        float stepSize = 1.0f; // Start with smaller steps
        float currentT = tMin;

        // Take an initial step to avoid starting exactly on a boundary
        currentT += stepSize;

        while (currentT <= tMax) {
            glm::vec3 currentPos = rayOrigin + rayDirection * currentT;

            // Get terrain height at current XZ position
            float terrainHeight = SampleHeightAtWorldPos(glm::vec2(currentPos.x, currentPos.z));

            // Skip if outside terrain bounds
            if (terrainHeight <= -999.0f) {
                currentT += stepSize;
                continue;
            }

            // Check if ray point is below terrain surface
            if (currentPos.y <= terrainHeight + 0.1f) { // Small epsilon for floating point precision
                // Found intersection, refine with binary search
                float t0 = currentT - stepSize;
                float t1 = currentT;

                // Binary search for precise intersection point
                for (int i = 0; i < 10; i++) {
                    float tMid = (t0 + t1) * 0.5f;
                    glm::vec3 midPos = rayOrigin + rayDirection * tMid;
                    float midTerrainHeight = SampleHeightAtWorldPos(glm::vec2(midPos.x, midPos.z));

                    if (midTerrainHeight <= -999.0f) {
                        // Outside terrain, use t1
                        t0 = tMid;
                        continue;
                    }

                    if (midPos.y > midTerrainHeight + 0.1f) {
                        // Above terrain
                        t0 = tMid;
                    }
                    else {
                        // Below or at terrain
                        t1 = tMid;
                    }
                }

                // Final intersection point
                glm::vec3 intersectionPos = rayOrigin + rayDirection * t1;
                float finalTerrainHeight = SampleHeightAtWorldPos(glm::vec2(intersectionPos.x, intersectionPos.z));

                result.mHit = true;
                result.mPosition = glm::vec3(intersectionPos.x, finalTerrainHeight, intersectionPos.z);
                result.mNormal = CalculateNormalAtWorldPos(glm::vec2(intersectionPos.x, intersectionPos.z));
                result.mDistance = t1;
                return result;
            }

            currentT += stepSize;
        }

        return result;
    }

    application::gl::SimpleMesh TerrainEditor::gSphereMesh;
    application::gl::Shader* TerrainEditor::gSphereShader = nullptr;
    const char* TerrainEditor::gEditorModeNames[] = { "Height", "Texture" };
    const char* TerrainEditor::gHeightBrushNames[] = { "Raise", "Lower", "Smooth", "Flatten", "Noise" };
    const char* TerrainEditor::gTextureBrushNames[] = { "Paint", "Replace", "Soften" };
    const char* TerrainEditor::gFallOffFunctionNames[] = { "None", "Gaussian", "Linear" };
    const char* TerrainEditor::gTextureLayerModeNames[] = { "Auto", "Layer A", "Layer B" };
    std::vector<GLuint> TerrainEditor::gTerrainTextureArrayViews;
    GLuint TerrainEditor::gTerrainHoleTexture;

    void TerrainEditor::Initialize()
    {
        auto [Vertices, Indices] = application::util::Math::GenerateSphere(1.0f, 32, 32);

        gSphereShader = application::manager::ShaderMgr::GetShader("ActorPainterSphere");
        gSphereMesh.Initialize(Vertices, Indices);
    }

    void TerrainEditor::InitializeTerrainTextures()
    {
        if (gTerrainTextureArrayViews.empty())
        {
            application::file::game::texture::TexToGoFile* MaterialAlbTexture = application::manager::TexToGoFileMgr::GetTexture("MaterialAlb.txtg");
            gTerrainTextureArrayViews.resize(MaterialAlbTexture->GetDepth());

            const GLenum MaterialAlbFormat = MaterialAlbTexture->GetPolishedFormat() != application::file::game::texture::TextureFormat::Format::CPU_DECODED ? application::file::game::texture::FormatHelper::gInternalFormatList[MaterialAlbTexture->GetPolishedFormat()] : GL_RGBA8;

            application::gl::TerrainRenderer::InitializeMaterialAlbTexture();

            glGenTextures(gTerrainTextureArrayViews.size(), gTerrainTextureArrayViews.data());
            for (int32_t i = 0; i < gTerrainTextureArrayViews.size(); ++i)
            {
                application::file::game::texture::TexToGoFile::Surface& Surface = 
                    application::manager::TexToGoFileMgr::GetTexture("MaterialAlb.txtg")->GetSurface(i);

                glBindTexture(GL_TEXTURE_2D, gTerrainTextureArrayViews[i]);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

                if (Surface.mPolishedFormat != application::file::game::texture::TextureFormat::Format::CPU_DECODED)
                {
                    glCompressedTexImage2D(GL_TEXTURE_2D, 0,
                        application::file::game::texture::FormatHelper::gInternalFormatList[Surface.mPolishedFormat],
                        1024, 1024, 0,
                        Surface.mData.size(), Surface.mData.data());
                }
                else
                {
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                        1024, 1024, 0,
                        GL_RGBA, GL_UNSIGNED_BYTE, Surface.mData.data());
                }
            }

            gTerrainHoleTexture = application::manager::TextureMgr::GetAssetTexture("TerrainHole")->mID;
        }
    }

	void TerrainEditor::SetEnabled(bool Enabled, void* MapEditorPtr)
	{
		mEnabled = Enabled;

		if (mEnabled)
		{
			application::rendering::map_editor::UIMapEditor* MapEditor = reinterpret_cast<application::rendering::map_editor::UIMapEditor*>(MapEditorPtr);

			MapEditor->DeselectActor();

            InitializeTerrainTextures();
		}
        else
        {
            mHasLivePreview = false;
            mIsMouseDragging = false;
        }
	}

    float TerrainMath::GaussianFallOff(float Distance, float Radius, float FallOffRate)
    {
        const float Sigma = Radius / (FallOffRate * 5.0f);
        return std::exp(-(Distance * Distance) / (2.0f * Sigma * Sigma));
    }

    float TerrainMath::LinearFallOff(float Distance, float Radius, float FallOffRate)
    {
        float NormalizedDistance = Distance / Radius;
        NormalizedDistance = std::pow(NormalizedDistance, FallOffRate);
        return 1.0f - NormalizedDistance;
    }

    std::vector<TerrainEditor::TerrainHeightEdit> TerrainBrushes::HeightBrushSculpt(const uint16_t& CorrectedRadius, const int16_t& CenterGridX, const int16_t& CenterGridZ, TerrainEditor* Editor, application::gl::TerrainRenderer* TerrainRenderer)
    {
        std::vector<TerrainEditor::TerrainHeightEdit> Edits;

        for (int OffsetX = -CorrectedRadius; OffsetX <= CorrectedRadius; OffsetX++)
        {
            for (int OffsetZ = -CorrectedRadius; OffsetZ <= CorrectedRadius; OffsetZ++)
            {
                int16_t CurrentGridX = CenterGridX + OffsetX;
                int16_t CurrentGridZ = CenterGridZ + OffsetZ;

                if (CurrentGridX >= 0 && CurrentGridX < kTerrainTextureSize &&
                    CurrentGridZ >= 0 && CurrentGridZ < kTerrainTextureSize)
                {
                    const float Distance = std::sqrt(static_cast<float>(OffsetX * OffsetX + OffsetZ * OffsetZ));

                    if (Distance <= CorrectedRadius)
                    {
                        float HeightDelta = Editor->mStrength * GetBrushWeight(Editor, Distance, static_cast<float>(CorrectedRadius));

                        const size_t Index = CurrentGridX + kTerrainTextureSize * CurrentGridZ;

                        float CurrentHeight = TerrainRenderer->mHeightTextureData[Index];
                        float NewHeight = CurrentHeight + HeightDelta;

                        TerrainEditor::TerrainHeightEdit Edit;
                        Edit.mGridX = CurrentGridX;
                        Edit.mGridZ = CurrentGridZ;
                        Edit.mNewHeight = NewHeight;
                        Edits.push_back(Edit);
                    }
                }
            }
        }

        return Edits;
    }

    std::vector<TerrainEditor::TerrainHeightEdit> TerrainBrushes::HeightBrushSmooth(const uint16_t& CorrectedRadius, const int16_t& CenterGridX, const int16_t& CenterGridZ, TerrainEditor* Editor, application::gl::TerrainRenderer* TerrainRenderer)
    {
        std::vector<TerrainEditor::TerrainHeightEdit> Edits;

        for (int OffsetX = -CorrectedRadius; OffsetX <= CorrectedRadius; OffsetX++)
        {
            for (int OffsetZ = -CorrectedRadius; OffsetZ <= CorrectedRadius; OffsetZ++)
            {
                int16_t CurrentGridX = CenterGridX + OffsetX;
                int16_t CurrentGridZ = CenterGridZ + OffsetZ;

                if (CurrentGridX >= 0 && CurrentGridX < kTerrainTextureSize &&
                    CurrentGridZ >= 0 && CurrentGridZ < kTerrainTextureSize)
                {
                    const float Distance = std::sqrt(static_cast<float>(OffsetX * OffsetX + OffsetZ * OffsetZ));

                    if (Distance <= CorrectedRadius)
                    {
                        const size_t CurrentIndex = CurrentGridX + (256 * 16) * CurrentGridZ;
                        float CurrentHeight = TerrainRenderer->mHeightTextureData[CurrentIndex];

                        float TotalWeight = 0.0f;
                        float WeightedHeightSum = 0.0f;

                        const int NeighborhoodSize = std::max(2, static_cast<int>(CorrectedRadius / 2));

                        for (int NbrOffsetX = -NeighborhoodSize; NbrOffsetX <= NeighborhoodSize; NbrOffsetX++)
                        {
                            for (int NbrOffsetZ = -NeighborhoodSize; NbrOffsetZ <= NeighborhoodSize; NbrOffsetZ++)
                            {
                                if (NbrOffsetX == 0 && NbrOffsetZ == 0)
                                    continue;

                                int16_t NbrGridX = CurrentGridX + NbrOffsetX;
                                int16_t NbrGridZ = CurrentGridZ + NbrOffsetZ;

                                if (NbrGridX >= 0 && NbrGridX < kTerrainTextureSize &&
                                    NbrGridZ >= 0 && NbrGridZ < kTerrainTextureSize)
                                {
                                    const float NbrDistance = std::sqrt(static_cast<float>(NbrOffsetX * NbrOffsetX + NbrOffsetZ * NbrOffsetZ));

                                    if (NbrDistance <= NeighborhoodSize)
                                    {
                                        float Weight = 1.0f / (1.0f + NbrDistance);

                                        const size_t NbrIndex = NbrGridX + kTerrainTextureSize * NbrGridZ;
                                        float NbrHeight = TerrainRenderer->mHeightTextureData[NbrIndex];

                                        WeightedHeightSum += Weight * NbrHeight;
                                        TotalWeight += Weight;
                                    }
                                }
                            }
                        }

                        float TargetHeight;
                        if (TotalWeight > 0.0f)
                        {
                            TargetHeight = WeightedHeightSum / TotalWeight;
                        }
                        else
                        {
                            TargetHeight = CurrentHeight;
                        }

                        float BrushWeight = GetBrushWeight(Editor, Distance, static_cast<float>(CorrectedRadius));

                        float NewHeight = (1.0f - BrushWeight) * CurrentHeight + BrushWeight * TargetHeight;

                        TerrainEditor::TerrainHeightEdit Edit;
                        Edit.mGridX = CurrentGridX;
                        Edit.mGridZ = CurrentGridZ;
                        Edit.mNewHeight = NewHeight;
                        Edits.push_back(Edit);
                    }
                }
            }
        }

        return Edits;
    }

    std::vector<TerrainEditor::TerrainHeightEdit> TerrainBrushes::HeightBrushPlateau(const uint16_t& CorrectedRadius, const int16_t& CenterGridX, const int16_t& CenterGridZ, TerrainEditor* Editor, application::gl::TerrainRenderer* TerrainRenderer)
    {
        std::vector<TerrainEditor::TerrainHeightEdit> Edits;

        for (int OffsetX = -CorrectedRadius; OffsetX <= CorrectedRadius; OffsetX++)
        {
            for (int OffsetZ = -CorrectedRadius; OffsetZ <= CorrectedRadius; OffsetZ++)
            {
                int16_t CurrentGridX = CenterGridX + OffsetX;
                int16_t CurrentGridZ = CenterGridZ + OffsetZ;

                if (CurrentGridX >= 0 && CurrentGridX < kTerrainTextureSize &&
                    CurrentGridZ >= 0 && CurrentGridZ < kTerrainTextureSize)
                {
                    const float Distance = std::sqrt(static_cast<float>(OffsetX * OffsetX + OffsetZ * OffsetZ));

                    if (Distance <= CorrectedRadius)
                    {
                        const float HeightDelta = Editor->mStrength * GetBrushWeight(Editor, Distance, static_cast<float>(CorrectedRadius));
                        if (HeightDelta <= 0.0f)
                            continue;

                        TerrainEditor::TerrainHeightEdit Edit;
                        Edit.mGridX = CurrentGridX;
                        Edit.mGridZ = CurrentGridZ;
                        Edit.mNewHeight = Editor->mPlateauHeight;
                        Edits.push_back(Edit);
                    }
                }
            }
        }

        return Edits;
    }

    std::vector<TerrainEditor::TerrainTextureEdit> TerrainBrushes::TextureBrush(const uint16_t& CorrectedRadius, const int16_t& CenterGridX, const int16_t& CenterGridZ, TerrainEditor* Editor, application::gl::TerrainRenderer* TerrainRenderer)
    {
        std::vector<TerrainEditor::TerrainTextureEdit> Edits;
        constexpr float kBlendScale = 255.0f;

        auto chooseOutputLayers = [Editor](std::pair<uint8_t, float> p0, std::pair<uint8_t, float> p1) -> std::pair<std::pair<uint8_t, float>, std::pair<uint8_t, float>>
        {
            if (Editor->mTextureLayerMode == TerrainEditor::TextureLayerMode::LAYER_A && p1.first == Editor->mCurrentTextureLayer)
                std::swap(p0, p1);
            if (Editor->mTextureLayerMode == TerrainEditor::TextureLayerMode::LAYER_B && p0.first == Editor->mCurrentTextureLayer)
                std::swap(p0, p1);
            return { p0, p1 };
        };

        for (int OffsetX = -CorrectedRadius; OffsetX <= CorrectedRadius; OffsetX++)
        {
            for (int OffsetZ = -CorrectedRadius; OffsetZ <= CorrectedRadius; OffsetZ++)
            {
                int16_t CurrentGridX = CenterGridX + OffsetX;
                int16_t CurrentGridZ = CenterGridZ + OffsetZ;

                if (CurrentGridX < 0 || CurrentGridX >= kTerrainTextureSize ||
                    CurrentGridZ < 0 || CurrentGridZ >= kTerrainTextureSize)
                {
                    continue;
                }

                const float Distance = std::sqrt(static_cast<float>(OffsetX * OffsetX + OffsetZ * OffsetZ));
                if (Distance > CorrectedRadius)
                    continue;

                const float BrushWeight = GetBrushWeight(Editor, Distance, static_cast<float>(CorrectedRadius));
                const float PaintAlpha = std::clamp(Editor->mStrength * BrushWeight, 0.0f, 1.0f);
                if (PaintAlpha <= 0.0f)
                    continue;

                const size_t Index = static_cast<size_t>(CurrentGridX) + static_cast<size_t>(kTerrainTextureSize) * static_cast<size_t>(CurrentGridZ);
                const size_t LayerIndex = Index * 3;

                uint8_t LayerA = TerrainRenderer->mLayersTextureData[LayerIndex];
                uint8_t LayerB = TerrainRenderer->mLayersTextureData[LayerIndex + 1];
                uint8_t Blend = TerrainRenderer->mLayersTextureData[LayerIndex + 2];

                if (Editor->mCurrentTextureLayer >= 120)
                {
                    if (Editor->mTextureBrushType == TerrainEditor::TextureBrushType::REPLACE || PaintAlpha > 0.95f)
                    {
                        TerrainEditor::TerrainTextureEdit Edit;
                        Edit.mGridX = CurrentGridX;
                        Edit.mGridZ = CurrentGridZ;
                        Edit.mTextureLayerA = 120;
                        Edit.mTextureLayerB = 120;
                        Edit.mBlend = 0;
                        Edits.push_back(Edit);
                    }
                    continue;
                }

                float WeightA = (kBlendScale - static_cast<float>(Blend)) / kBlendScale;
                float WeightB = static_cast<float>(Blend) / kBlendScale;
                WeightA = std::clamp(WeightA, 0.0f, 1.0f);
                WeightB = std::clamp(WeightB, 0.0f, 1.0f);

                std::array<std::pair<uint8_t, float>, 3> Weights = {
                    std::make_pair(LayerA, WeightA),
                    std::make_pair(LayerB, WeightB),
                    std::make_pair(static_cast<uint8_t>(Editor->mCurrentTextureLayer), 0.0f)
                };

                if (Weights[0].first == Weights[2].first) Weights[2].second += Weights[0].second;
                if (Weights[1].first == Weights[2].first) Weights[2].second += Weights[1].second;

                float TotalExisting = Weights[0].second + Weights[1].second;
                if (TotalExisting <= 0.0f)
                {
                    Weights[0] = { static_cast<uint8_t>(Editor->mCurrentTextureLayer), 1.0f };
                    Weights[1] = Weights[0];
                    Weights[2] = Weights[0];
                }

                if (Editor->mTextureBrushType == TerrainEditor::TextureBrushType::REPLACE)
                {
                    Weights = {
                        std::make_pair(static_cast<uint8_t>(Editor->mCurrentTextureLayer), 1.0f),
                        std::make_pair(static_cast<uint8_t>(Editor->mCurrentTextureLayer), 0.0f),
                        std::make_pair(static_cast<uint8_t>(Editor->mCurrentTextureLayer), 1.0f)
                    };
                }
                else if (Editor->mTextureBrushType == TerrainEditor::TextureBrushType::SMOOTH)
                {
                    const int Neighborhood = std::max(1, static_cast<int>(CorrectedRadius / 3));
                    std::array<float, 121> Histogram{};
                    float Sum = 0.0f;

                    for (int nz = -Neighborhood; nz <= Neighborhood; ++nz)
                    {
                        for (int nx = -Neighborhood; nx <= Neighborhood; ++nx)
                        {
                            const int sx = CurrentGridX + nx;
                            const int sz = CurrentGridZ + nz;
                            if (sx < 0 || sx >= kTerrainTextureSize || sz < 0 || sz >= kTerrainTextureSize)
                                continue;

                            const size_t sampleIndex = (static_cast<size_t>(sx) + static_cast<size_t>(kTerrainTextureSize) * static_cast<size_t>(sz)) * 3;
                            const uint8_t sa = TerrainRenderer->mLayersTextureData[sampleIndex];
                            const uint8_t sb = TerrainRenderer->mLayersTextureData[sampleIndex + 1];
                            const float sblend = static_cast<float>(TerrainRenderer->mLayersTextureData[sampleIndex + 2]) / kBlendScale;
                            const float wa = 1.0f - sblend;
                            const float wb = sblend;
                            if (sa < Histogram.size()) Histogram[sa] += wa;
                            if (sb < Histogram.size()) Histogram[sb] += wb;
                            Sum += wa + wb;
                        }
                    }

                    if (Sum > 0.0f)
                    {
                        for (auto& h : Histogram) h /= Sum;
                        const float target = Histogram[Editor->mCurrentTextureLayer];
                        Weights[2].second = std::clamp(Weights[2].second + (target - Weights[2].second) * PaintAlpha, 0.0f, 1.0f);
                    }
                }
                else
                {
                    Weights[2].second = std::clamp(Weights[2].second + (1.0f - Weights[2].second) * PaintAlpha, 0.0f, 1.0f);
                }

                const float Remaining = std::max(0.0f, 1.0f - Weights[2].second);
                float ExistingNoSelected = 0.0f;
                if (Weights[0].first != Weights[2].first) ExistingNoSelected += Weights[0].second;
                if (Weights[1].first != Weights[2].first) ExistingNoSelected += Weights[1].second;

                if (ExistingNoSelected > 0.0f)
                {
                    if (Weights[0].first != Weights[2].first) Weights[0].second = (Weights[0].second / ExistingNoSelected) * Remaining;
                    else Weights[0].second = 0.0f;
                    if (Weights[1].first != Weights[2].first) Weights[1].second = (Weights[1].second / ExistingNoSelected) * Remaining;
                    else Weights[1].second = 0.0f;
                }
                else
                {
                    Weights[0] = Weights[2];
                    Weights[1] = Weights[2];
                    Weights[0].second = Remaining;
                    Weights[1].second = 0.0f;
                }

                std::array<std::pair<uint8_t, float>, 3> Ranked = Weights;
                std::sort(Ranked.begin(), Ranked.end(), [](const auto& a, const auto& b) { return a.second > b.second; });

                auto [Primary, Secondary] = chooseOutputLayers(Ranked[0], Ranked[1]);
                float norm = Primary.second + Secondary.second;
                if (norm <= 1e-6f)
                {
                    Primary = { static_cast<uint8_t>(Editor->mCurrentTextureLayer), 1.0f };
                    Secondary = Primary;
                    norm = 1.0f;
                }

                TerrainEditor::TerrainTextureEdit Edit;
                Edit.mGridX = CurrentGridX;
                Edit.mGridZ = CurrentGridZ;
                Edit.mTextureLayerA = Primary.first;
                Edit.mTextureLayerB = Secondary.first;
                Edit.mBlend = static_cast<uint8_t>(std::clamp((Secondary.second / norm) * kBlendScale, 0.0f, 255.0f));
                Edits.push_back(Edit);
            }
        }

        return Edits;
    }

    std::vector<TerrainEditor::TerrainHeightEdit> TerrainBrushes::HeightBrushNoise(const uint16_t& CorrectedRadius, const int16_t& CenterGridX, const int16_t& CenterGridZ, TerrainEditor* Editor, application::gl::TerrainRenderer* TerrainRenderer)
    {
        std::vector<TerrainEditor::TerrainHeightEdit> HeightEdits;

        for (int OffsetX = -CorrectedRadius; OffsetX <= CorrectedRadius; OffsetX++)
        {
            for (int OffsetZ = -CorrectedRadius; OffsetZ <= CorrectedRadius; OffsetZ++)
            {
                int16_t CurrentGridX = CenterGridX + OffsetX;
                int16_t CurrentGridZ = CenterGridZ + OffsetZ;

                if (CurrentGridX < 0 || CurrentGridX >= kTerrainTextureSize ||
                    CurrentGridZ < 0 || CurrentGridZ >= kTerrainTextureSize)
                {
                    continue;
                }

                const float Distance = std::sqrt(static_cast<float>(OffsetX * OffsetX + OffsetZ * OffsetZ));
                if (Distance > CorrectedRadius)
                    continue;

                const float BrushWeight = GetBrushWeight(Editor, Distance, static_cast<float>(CorrectedRadius));
                if (BrushWeight <= 0.0f)
                    continue;

                const uint32_t Hash = HashCoord(CurrentGridX, CurrentGridZ);
                const float Noise = (static_cast<float>(Hash & 0xFFFF) / 65535.0f) * 2.0f - 1.0f;
                const float Delta = Noise * Editor->mStrength * BrushWeight;

                const size_t Index = static_cast<size_t>(CurrentGridX) + static_cast<size_t>(kTerrainTextureSize) * static_cast<size_t>(CurrentGridZ);
                TerrainEditor::TerrainHeightEdit Edit;
                Edit.mGridX = CurrentGridX;
                Edit.mGridZ = CurrentGridZ;
                Edit.mNewHeight = TerrainRenderer->mHeightTextureData[Index] + Delta;
                HeightEdits.push_back(Edit);
            }
        }

        return HeightEdits;
    }

	void TerrainEditor::DrawMenu(void* MapEditorPtr)
	{
        application::rendering::map_editor::UIMapEditor* MapEditor = reinterpret_cast<application::rendering::map_editor::UIMapEditor*>(MapEditorPtr);

        ImGui::SeparatorText("Terrain Editor");
        ImGuiExt::DrawButtonArray(gEditorModeNames, IM_ARRAYSIZE(gEditorModeNames), 2, reinterpret_cast<int*>(&mEditorMode));
        ImGui::Spacing();
        ImGui::Checkbox("Show Brush Preview", &mShowBrushPreview);
        ImGui::Checkbox("Show Live Edit Preview", &mShowLiveEditPreview);
        ImGui::SliderFloat("Preview Opacity", &mBrushPreviewOpacity, 0.1f, 1.0f, "%.2f");
        mBrushPreviewOpacity = std::clamp(mBrushPreviewOpacity, 0.1f, 1.0f);
        ImGui::Spacing();

        mRadius = std::clamp(mRadius, 0.1f, 128.0f);
        mFallOffRate = std::clamp(mFallOffRate, 0.1f, 4.0f);

        if (mEditorMode == EditorMode::HEIGHT)
        {
            mStrength = std::clamp(mStrength, 0.0f, 10.0f);
            ImGui::TextUnformatted("Height Brush");
            ImGuiExt::DrawButtonArray(gHeightBrushNames, IM_ARRAYSIZE(gHeightBrushNames), 3, reinterpret_cast<int*>(&mBrushType));
            ImGui::SliderFloat("Radius (m)", &mRadius, 0.1f, 64.0f, "%.2f");
            ImGui::SliderFloat("Strength", &mStrength, 0.01f, 10.0f, "%.2f");
            ImGui::TextUnformatted("Falloff");
            ImGuiExt::DrawButtonArray(gFallOffFunctionNames, IM_ARRAYSIZE(gFallOffFunctionNames), 3, reinterpret_cast<int*>(&mFallOffFunction));
            if (mFallOffFunction != FallOffFunction::NONE)
            {
                ImGui::SliderFloat("Falloff Rate", &mFallOffRate, 0.1f, 4.0f, "%.2f");
            }
            if (mBrushType == BrushType::FLATTEN)
            {
                ImGui::InputFloat("Flatten Height", &mPlateauHeight, 0.1f, 1.0f, "%.3f");
                ImGui::TextUnformatted("Tip: first click samples current height.");
            }

            if (mShowLiveEditPreview && mHasLivePreview)
            {
                ImGui::SeparatorText("Live Preview");
                ImGui::Text("Grid: (%d, %d)", mPreviewTexX, mPreviewTexY);
                ImGui::Text("Affected Samples: ~%d", mPreviewAffectedSamples);
                ImGui::Text("Current Height: %.3f", mPreviewCurrentHeight);
                ImGui::Text("Preview Height: %.3f", mPreviewTargetHeight);
                ImVec4 deltaCol = mPreviewDeltaHeight >= 0.0f ? ImVec4(0.35f, 0.9f, 0.45f, 1.0f) : ImVec4(0.95f, 0.4f, 0.4f, 1.0f);
                ImGui::TextColored(deltaCol, "Delta: %+0.3f", mPreviewDeltaHeight);
            }
        }
        else
        {
            mStrength = std::clamp(mStrength, 0.0f, 1.0f);
            if (ImGui::Checkbox("Render Layer A", &mRenderLayerA))
                MapEditor->mScene.mTerrainRenderer->mRenderLayerA = mRenderLayerA;
            ImGui::SameLine();
            if (ImGui::Checkbox("Render Layer B", &mRenderLayerB))
                MapEditor->mScene.mTerrainRenderer->mRenderLayerB = mRenderLayerB;

            ImGui::TextUnformatted("Texture Brush");
            ImGuiExt::DrawButtonArray(gTextureBrushNames, IM_ARRAYSIZE(gTextureBrushNames), 3, reinterpret_cast<int*>(&mTextureBrushType));
            ImGui::SliderFloat("Radius (m)", &mRadius, 0.1f, 64.0f, "%.2f");
            ImGui::SliderFloat("Opacity", &mStrength, 0.01f, 1.0f, "%.2f");
            ImGui::TextUnformatted("Falloff");
            ImGuiExt::DrawButtonArray(gFallOffFunctionNames, IM_ARRAYSIZE(gFallOffFunctionNames), 3, reinterpret_cast<int*>(&mFallOffFunction));
            if (mFallOffFunction != FallOffFunction::NONE)
            {
                ImGui::SliderFloat("Falloff Rate", &mFallOffRate, 0.1f, 4.0f, "%.2f");
            }

            ImGui::Combo("Layer Placement", reinterpret_cast<int*>(&mTextureLayerMode), gTextureLayerModeNames, IM_ARRAYSIZE(gTextureLayerModeNames));

            ImGui::PushID("TextureSelector");
            ImGui::Text("Paint Texture: %d", mCurrentTextureLayer);
            const ImVec2 uv0(0.0f, 0.0f);
            const ImVec2 uv1(1.0f, 1.0f);
            const ImVec2 previewSize(96.0f, 96.0f);
            bool clicked = false;

            if (mCurrentTextureLayer == 120)
            {
                clicked = ImGui::ImageButton("CurrentTexture", (ImTextureID)(intptr_t)gTerrainHoleTexture, previewSize,
                    ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f), ImVec4(0.15f, 0.15f, 0.15f, 1.0f), ImVec4(1, 1, 1, 1));
            }
            else
            {
                clicked = ImGui::ImageButton("CurrentTexture", (ImTextureID)(intptr_t)gTerrainTextureArrayViews[mCurrentTextureLayer], previewSize,
                    uv0, uv1, ImVec4(0.15f, 0.15f, 0.15f, 1.0f), ImVec4(1, 1, 1, 1));
            }

            if (clicked)
                ImGui::OpenPopup("TextureSelectionPopup");

            if (ImGui::BeginPopup("TextureSelectionPopup"))
            {
                ImGui::TextUnformatted("Select Brush Texture");
                ImGui::Separator();
                constexpr int Columns = 6;
                constexpr float Thumb = 72.0f;
                ImGui::BeginChild("TextureGrid", ImVec2(Columns * (Thumb + 8.0f), 360.0f), true);
                for (int i = 0; i < 121; ++i)
                {
                    ImGui::PushID(i);
                    if (i > 0 && i % Columns != 0)
                        ImGui::SameLine();

                    const bool selected = (i == mCurrentTextureLayer);
                    if (selected) ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.2f, 0.7f, 1.0f, 1.0f));
                    if (selected) ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);

                    bool pick = false;
                    if (i == 120)
                    {
                        pick = ImGui::ImageButton(("TextureTile_" + std::to_string(i)).c_str(),
                            (ImTextureID)(intptr_t)gTerrainHoleTexture, ImVec2(Thumb, Thumb),
                            ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f), ImVec4(0.12f, 0.12f, 0.12f, 1.0f), ImVec4(1, 1, 1, 1));
                    }
                    else
                    {
                        pick = ImGui::ImageButton(("TextureTile_" + std::to_string(i)).c_str(),
                            (ImTextureID)(intptr_t)gTerrainTextureArrayViews[i], ImVec2(Thumb, Thumb),
                            uv0, uv1, ImVec4(0.12f, 0.12f, 0.12f, 1.0f), ImVec4(1, 1, 1, 1));
                    }
                    if (pick)
                    {
                        mCurrentTextureLayer = i;
                        ImGui::CloseCurrentPopup();
                    }
                    if (ImGui::IsItemHovered())
                    {
                        ImGui::SetTooltip("Texture %d", i);
                    }

                    if (selected) ImGui::PopStyleVar();
                    if (selected) ImGui::PopStyleColor();
                    ImGui::PopID();
                }
                ImGui::EndChild();
                ImGui::EndPopup();
            }
            ImGui::PopID();

            if (mShowLiveEditPreview && mHasLivePreview)
            {
                ImGui::SeparatorText("Live Preview");
                ImGui::Text("Grid: (%d, %d)", mPreviewTexX, mPreviewTexY);
                ImGui::Text("Affected Samples: ~%d", mPreviewAffectedSamples);
                ImGui::Text("Layer A: %u", static_cast<unsigned>(mPreviewLayerA));
                ImGui::Text("Layer B: %u", static_cast<unsigned>(mPreviewLayerB));
                ImGui::Text("Blend: %u", static_cast<unsigned>(mPreviewBlend));
            }
        }
	}

    void TerrainEditor::DrawBrushGizmo(const glm::vec3& HitPos, const glm::vec3& HitNormal, void* MapEditorPtr)
    {
        if (!mShowBrushPreview || gSphereShader == nullptr)
            return;

        application::rendering::map_editor::UIMapEditor* MapEditor = reinterpret_cast<application::rendering::map_editor::UIMapEditor*>(MapEditorPtr);

        const glm::vec3 baseColor = (mEditorMode == EditorMode::HEIGHT)
            ? glm::vec3(1.0f, 0.62f, 0.18f)
            : glm::vec3(0.2f, 0.75f, 1.0f);
        const glm::vec3 accentColor = (mEditorMode == EditorMode::HEIGHT)
            ? glm::vec3(1.0f, 0.85f, 0.35f)
            : glm::vec3(0.7f, 0.9f, 1.0f);

        const float halfWeightRadius = SolveBrushHalfStrengthRadius(this, std::max(0.001f, mRadius));
        const float pulse = 0.85f + 0.15f * std::sin(static_cast<float>(ImGui::GetTime()) * 6.0f);
        const glm::vec3 normal = glm::length(HitNormal) > 0.0f ? glm::normalize(HitNormal) : glm::vec3(0.0f, 1.0f, 0.0f);
        const glm::vec3 center = HitPos + normal * 0.08f;

        const GLboolean blendEnabled = glIsEnabled(GL_BLEND);
        const GLboolean cullEnabled = glIsEnabled(GL_CULL_FACE);
        GLboolean depthMask = GL_TRUE;
        glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_CULL_FACE);
        glDepthMask(GL_FALSE);

        gSphereShader->Bind();
        MapEditor->mCamera.Matrix(gSphereShader, "camMatrix");

        auto drawSphere = [&](float radius, const glm::vec3& tint, float opacity)
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, center);
            model = glm::scale(model, glm::vec3(radius));
            glUniformMatrix4fv(glGetUniformLocation(gSphereShader->mID, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(model));
            glUniform3f(glGetUniformLocation(gSphereShader->mID, "tintColor"), tint.x, tint.y, tint.z);
            glUniform1f(glGetUniformLocation(gSphereShader->mID, "opacity"), std::clamp(opacity, 0.0f, 1.0f));
            gSphereMesh.Draw();
        };

        drawSphere(std::max(0.05f, mRadius), baseColor, mBrushPreviewOpacity * 0.35f * pulse);
        drawSphere(std::max(0.04f, halfWeightRadius), accentColor, mBrushPreviewOpacity * 0.55f);
        drawSphere(std::max(0.02f, mRadius * 0.08f), glm::vec3(1.0f), mBrushPreviewOpacity * 0.95f);

        glDepthMask(depthMask);
        if (!blendEnabled) glDisable(GL_BLEND);
        if (cullEnabled) glEnable(GL_CULL_FACE);
    }

    void TerrainEditor::UpdateLivePreview(int TexX, int TexY, void* MapEditorPtr)
    {
        mHasLivePreview = false;
        mPreviewTexX = TexX;
        mPreviewTexY = TexY;

        if (!mShowLiveEditPreview)
            return;

        application::rendering::map_editor::UIMapEditor* MapEditor = reinterpret_cast<application::rendering::map_editor::UIMapEditor*>(MapEditorPtr);
        application::gl::TerrainRenderer* renderer = MapEditor->mScene.mTerrainRenderer;
        if (renderer == nullptr)
            return;

        if (TexX < 0 || TexX >= kTerrainTextureSize || TexY < 0 || TexY >= kTerrainTextureSize)
            return;

        const uint16_t correctedRadius = static_cast<uint16_t>(std::max(1.0f, std::round(kTerrainTextureSize / 1000.0f * mRadius)));
        mPreviewAffectedSamples = static_cast<int>(3.14159265f * correctedRadius * correctedRadius);

        if (mEditorMode == EditorMode::HEIGHT)
        {
            const size_t index = static_cast<size_t>(TexX) + static_cast<size_t>(TexY) * static_cast<size_t>(kTerrainTextureSize);
            const float currentHeight = renderer->mHeightTextureData[index];
            float targetHeight = currentHeight;

            switch (mBrushType)
            {
            case BrushType::RAISE:
                targetHeight = currentHeight + mStrength;
                break;
            case BrushType::LOWER:
                targetHeight = currentHeight - mStrength;
                break;
            case BrushType::SMOOTH:
            {
                float totalWeight = 0.0f;
                float weightedHeightSum = 0.0f;
                const int neighborhoodSize = std::max(2, static_cast<int>(correctedRadius / 2));
                for (int nz = -neighborhoodSize; nz <= neighborhoodSize; ++nz)
                {
                    for (int nx = -neighborhoodSize; nx <= neighborhoodSize; ++nx)
                    {
                        if (nx == 0 && nz == 0)
                            continue;

                        const int sx = TexX + nx;
                        const int sz = TexY + nz;
                        if (sx < 0 || sx >= kTerrainTextureSize || sz < 0 || sz >= kTerrainTextureSize)
                            continue;

                        const float dist = std::sqrt(static_cast<float>(nx * nx + nz * nz));
                        if (dist > neighborhoodSize)
                            continue;

                        const float weight = 1.0f / (1.0f + dist);
                        const size_t sampleIndex = static_cast<size_t>(sx) + static_cast<size_t>(sz) * static_cast<size_t>(kTerrainTextureSize);
                        weightedHeightSum += renderer->mHeightTextureData[sampleIndex] * weight;
                        totalWeight += weight;
                    }
                }

                const float smoothTarget = totalWeight > 0.0f ? (weightedHeightSum / totalWeight) : currentHeight;
                targetHeight = smoothTarget;
                break;
            }
            case BrushType::FLATTEN:
                targetHeight = mIsMouseDragging ? mPlateauHeight : currentHeight;
                break;
            case BrushType::NOISE:
            {
                const uint32_t hash = HashCoord(TexX, TexY);
                const float noise = (static_cast<float>(hash & 0xFFFF) / 65535.0f) * 2.0f - 1.0f;
                targetHeight = currentHeight + noise * mStrength;
                break;
            }
            default:
                break;
            }

            mPreviewCurrentHeight = currentHeight;
            mPreviewTargetHeight = targetHeight;
            mPreviewDeltaHeight = targetHeight - currentHeight;
            mHasLivePreview = true;
        }
        else
        {
            TerrainTextureEdit previewEdit;
            if (ComputeTexturePreviewAtCenter(this, renderer, correctedRadius, TexX, TexY, previewEdit))
            {
                mPreviewLayerA = previewEdit.mTextureLayerA;
                mPreviewLayerB = previewEdit.mTextureLayerB;
                mPreviewBlend = previewEdit.mBlend;
                mHasLivePreview = true;
            }
        }
    }

    void TerrainEditor::ApplyBrush(int TexX, int TexY, void* MapEditorPtr)
    {
        application::rendering::map_editor::UIMapEditor* MapEditor = reinterpret_cast<application::rendering::map_editor::UIMapEditor*>(MapEditorPtr);

        const uint16_t CorrectedRadius = static_cast<uint16_t>(std::round(kTerrainTextureSize / 1000.0f * mRadius));

        if (mEditorMode == EditorMode::HEIGHT)
        {
            std::vector<TerrainEditor::TerrainHeightEdit> Edits;

            switch (mBrushType)
            {
            case BrushType::RAISE:
                Edits = TerrainBrushes::HeightBrushSculpt(CorrectedRadius, TexX, TexY, this, MapEditor->mScene.mTerrainRenderer);
                break;
            case BrushType::LOWER:
            {
                const float OriginalStrength = mStrength;
                mStrength = -std::abs(mStrength);
                Edits = TerrainBrushes::HeightBrushSculpt(CorrectedRadius, TexX, TexY, this, MapEditor->mScene.mTerrainRenderer);
                mStrength = OriginalStrength;
                break;
            }
            case BrushType::SMOOTH:
                Edits = TerrainBrushes::HeightBrushSmooth(CorrectedRadius, TexX, TexY, this, MapEditor->mScene.mTerrainRenderer);
                break;
            case BrushType::FLATTEN:
                Edits = TerrainBrushes::HeightBrushPlateau(CorrectedRadius, TexX, TexY, this, MapEditor->mScene.mTerrainRenderer);
                break;
            case BrushType::NOISE:
                Edits = TerrainBrushes::HeightBrushNoise(CorrectedRadius, TexX, TexY, this, MapEditor->mScene.mTerrainRenderer);
                break;
            default:
                application::util::Logger::Warning("TerrainEditor", "Unknown brush type");
                break;
            }

            for (TerrainEditor::TerrainHeightEdit& Edit : Edits)
            {
                uint8_t IndexX = static_cast<uint8_t>(Edit.mGridX / 256);
                uint8_t IndexZ = static_cast<uint8_t>(Edit.mGridZ / 256);

                MapEditor->mScene.mTerrainRenderer->mModifiedTiles.insert(IndexX + 16 * IndexZ);

                MapEditor->mScene.mTerrainRenderer->mHeightTextureData[Edit.mGridX + Edit.mGridZ * kTerrainTextureSize] = Edit.mNewHeight;
            }

            glBindTexture(GL_TEXTURE_2D, MapEditor->mScene.mTerrainRenderer->mHeightTextureID);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, kTerrainTextureSize, kTerrainTextureSize, GL_RED, GL_FLOAT, MapEditor->mScene.mTerrainRenderer->mHeightTextureData.data());
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        else
        {
            std::vector<TerrainEditor::TerrainTextureEdit> Edits = TerrainBrushes::TextureBrush(CorrectedRadius, TexX, TexY, this, MapEditor->mScene.mTerrainRenderer);

            for (TerrainEditor::TerrainTextureEdit& Edit : Edits)
            {
                uint8_t IndexX = static_cast<uint8_t>(Edit.mGridX / 256);
                uint8_t IndexZ = static_cast<uint8_t>(Edit.mGridZ / 256);

                MapEditor->mScene.mTerrainRenderer->mModifiedTextureTiles.insert(IndexX + 16 * IndexZ);

                MapEditor->mScene.mTerrainRenderer->mLayersTextureData[(Edit.mGridX + Edit.mGridZ * kTerrainTextureSize) * 3] = Edit.mTextureLayerA;
                MapEditor->mScene.mTerrainRenderer->mLayersTextureData[(Edit.mGridX + Edit.mGridZ * kTerrainTextureSize) * 3 + 1] = Edit.mTextureLayerB;
                MapEditor->mScene.mTerrainRenderer->mLayersTextureData[(Edit.mGridX + Edit.mGridZ * kTerrainTextureSize) * 3 + 2] = Edit.mBlend;
            }

            glBindTexture(GL_TEXTURE_2D, MapEditor->mScene.mTerrainRenderer->mLayersTextureID);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, kTerrainTextureSize, kTerrainTextureSize, GL_RGB_INTEGER, GL_UNSIGNED_BYTE, MapEditor->mScene.mTerrainRenderer->mLayersTextureData.data());
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }

    void TerrainEditor::RenderTerrainEditor(float XPos, float YPos, int ScreenWidth, int ScreenHeight, void* MapEditorPtr)
    {
        if (!mEnabled || ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopup))
            return;

        application::rendering::map_editor::UIMapEditor* MapEditor = reinterpret_cast<application::rendering::map_editor::UIMapEditor*>(MapEditorPtr);

        if (MapEditor->mCamera.IsInCameraMovement() || ImGui::IsAnyItemHovered())
            return;

        glm::vec3 RayDir = application::util::Math::GetRayFromMouse(XPos, YPos, MapEditor->mCamera.mProjection, MapEditor->mCamera.mView, ScreenWidth, ScreenHeight);

        if (glm::any(glm::isnan(RayDir)) && !glm::any(glm::isinf(RayDir)))
            return;

        TerrainRaycast Raycast(MapEditor->mScene.mTerrainRenderer);
        TerrainRaycast::RaycastHit Hit = Raycast.RaycastTerrain(MapEditor->mCamera.mPosition, RayDir);
        if (Hit.mHit)
        {
            DrawBrushGizmo(Hit.mPosition, Hit.mNormal, MapEditorPtr);

            const float CurrentTime = ImGui::GetTime();

            glm::vec2 TerrainMin = glm::vec2(MapEditor->mScene.mTerrainRenderer->mTerrainPosition.x - 500.0f,
                MapEditor->mScene.mTerrainRenderer->mTerrainPosition.z - 500.0f);
            glm::vec2 TerrainMax = glm::vec2(MapEditor->mScene.mTerrainRenderer->mTerrainPosition.x + 500.0f,
                MapEditor->mScene.mTerrainRenderer->mTerrainPosition.z + 500.0f);
            glm::vec2 LocalPos = (glm::vec2(Hit.mPosition.x, Hit.mPosition.z) - TerrainMin) / (TerrainMax - TerrainMin);

            constexpr int TextureSize = kTerrainTextureSize;
            int TexX = static_cast<int>(LocalPos.x * (TextureSize - 1));
            int TexY = static_cast<int>(LocalPos.y * (TextureSize - 1));
            TexX = std::clamp(TexX, 0, TextureSize - 1);
            TexY = std::clamp(TexY, 0, TextureSize - 1);

            UpdateLivePreview(TexX, TexY, MapEditorPtr);

            if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                if (!mIsMouseDragging)
                {
                    mPlateauHeight = MapEditor->mScene.mTerrainRenderer->mHeightTextureData[TexX + TextureSize * TexY];

                    ApplyBrush(TexX, TexY, MapEditorPtr);

                    mIsMouseDragging = true;
                    mLastBrushApplyTime = CurrentTime;
                    mLastBrushPosition = glm::vec2(TexX, TexY);
                }
                else
                {
                    const glm::vec2 Intersection2d = glm::vec2(TexX, TexY);
                    const float DistanceMoved = glm::distance(mLastBrushPosition, Intersection2d);

                    const bool TimeThresholdMet = (CurrentTime - mLastBrushApplyTime) >= mBrushUpdateInterval;
                    const bool DistanceThresholdMet = DistanceMoved >= std::max(1.0f, mRadius * 0.3f);

                    if (TimeThresholdMet || DistanceThresholdMet)
                    {
                        ApplyBrush(TexX, TexY, MapEditorPtr);

                        mLastBrushApplyTime = CurrentTime;
                        mLastBrushPosition = Intersection2d;
                    }
                }
            }
            else
            {
                mIsMouseDragging = false;
                mPlateauHeight = 0.0f;
            }
        }
        else
        {
            mIsMouseDragging = false;
            mHasLivePreview = false;
        }
    }
}
