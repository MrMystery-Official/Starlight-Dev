#include <rendering/mapeditor/TerrainEditor.h>

#include <rendering/mapeditor/UIMapEditor.h>
#include <util/Math.h>
#include <manager/ShaderMgr.h>
#include <manager/TexToGoFileMgr.h>
#include <rendering/ImGuiExt.h>
#include <file/game/texture/FormatHelper.h>
#include "imgui_internal.h"

namespace application::rendering::map_editor
{
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
    const char* TerrainEditor::gBrushNames[] = { "Sculpt", "Smooth", "Plateau" };
    const char* TerrainEditor::gFallOffFunctionNames[] = { "None", "Gaussian", "Linear" };
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
                glTextureView(gTerrainTextureArrayViews[i], GL_TEXTURE_2D, application::gl::TerrainRenderer::gMaterialAlbID, MaterialAlbFormat, 0, 1, i, 1);
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

                if (CurrentGridX >= 0 && CurrentGridX < (256 * 16) &&
                    CurrentGridZ >= 0 && CurrentGridZ < (256 * 16))
                {
                    const float Distance = std::sqrt(static_cast<float>(OffsetX * OffsetX + OffsetZ * OffsetZ));

                    if (Distance <= CorrectedRadius)
                    {
                        float HeightDelta = Editor->mStrength;
                        switch (Editor->mFallOffFunction)
                        {
                        case TerrainEditor::FallOffFunction::GAUSSIAN:
                            HeightDelta *= TerrainMath::GaussianFallOff(Distance, CorrectedRadius, Editor->mFallOffRate);
                            break;
                        case TerrainEditor::FallOffFunction::LINEAR:
                            HeightDelta *= TerrainMath::LinearFallOff(Distance, CorrectedRadius, Editor->mFallOffRate);
                            break;
                        case TerrainEditor::FallOffFunction::NONE:
                        default:
                            break;
                        }

                        const size_t Index = CurrentGridX + (256 * 16) * CurrentGridZ;

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

                if (CurrentGridX >= 0 && CurrentGridX < (256 * 16) &&
                    CurrentGridZ >= 0 && CurrentGridZ < (256 * 16))
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

                                if (NbrGridX >= 0 && NbrGridX < (256 * 16) &&
                                    NbrGridZ >= 0 && NbrGridZ < (256 * 16))
                                {
                                    const float NbrDistance = std::sqrt(static_cast<float>(NbrOffsetX * NbrOffsetX + NbrOffsetZ * NbrOffsetZ));

                                    if (NbrDistance <= NeighborhoodSize)
                                    {
                                        float Weight = 1.0f / (1.0f + NbrDistance);

                                        const size_t NbrIndex = NbrGridX + (256 * 16) * NbrGridZ;
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

                        float BrushWeight = 1.0f;

                        switch (Editor->mFallOffFunction)
                        {
                        case TerrainEditor::FallOffFunction::GAUSSIAN:
                            BrushWeight = TerrainMath::GaussianFallOff(Distance, CorrectedRadius, Editor->mFallOffRate);
                            break;
                        case TerrainEditor::FallOffFunction::LINEAR:
                            BrushWeight = TerrainMath::LinearFallOff(Distance, CorrectedRadius, Editor->mFallOffRate);
                            break;
                        case TerrainEditor::FallOffFunction::NONE:
                        default:
                            break;
                        }

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

                if (CurrentGridX >= 0 && CurrentGridX < (256 * 16) &&
                    CurrentGridZ >= 0 && CurrentGridZ < (256 * 16))
                {
                    const float Distance = std::sqrt(static_cast<float>(OffsetX * OffsetX + OffsetZ * OffsetZ));

                    if (Distance <= CorrectedRadius)
                    {
                        float HeightDelta = Editor->mStrength;
                        switch (Editor->mFallOffFunction)
                        {
                        case TerrainEditor::FallOffFunction::GAUSSIAN:
                            HeightDelta *= TerrainMath::GaussianFallOff(Distance, CorrectedRadius, Editor->mFallOffRate);
                            break;
                        case TerrainEditor::FallOffFunction::LINEAR:
                            HeightDelta *= TerrainMath::LinearFallOff(Distance, CorrectedRadius, Editor->mFallOffRate);
                            break;
                        case TerrainEditor::FallOffFunction::NONE:
                        default:
                            break;
                        }

                        const size_t Index = CurrentGridX + (256 * 16) * CurrentGridZ;

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

        // Define falloff - how much of the radius should be solid vs fading
        const float FalloffStart = 0.6f; // Inner 60% is solid, outer 40% fades

        for (int OffsetX = -CorrectedRadius; OffsetX <= CorrectedRadius; OffsetX++)
        {
            for (int OffsetZ = -CorrectedRadius; OffsetZ <= CorrectedRadius; OffsetZ++)
            {
                int16_t CurrentGridX = CenterGridX + OffsetX;
                int16_t CurrentGridZ = CenterGridZ + OffsetZ;

                if (CurrentGridX >= 0 && CurrentGridX < (256 * 16) &&
                    CurrentGridZ >= 0 && CurrentGridZ < (256 * 16))
                {
                    const float Distance = std::sqrt(static_cast<float>(OffsetX * OffsetX + OffsetZ * OffsetZ));

                    if (Distance <= CorrectedRadius)
                    {
                        // Get existing terrain data at this position
                        const size_t Index = CurrentGridX + (256 * 16) * CurrentGridZ;
                        uint8_t ExistingBlend = TerrainRenderer->mLayersTextureData[Index * 3 + 2];

                        TerrainEditor::TerrainTextureEdit Edit;
                        Edit.mGridX = CurrentGridX;
                        Edit.mGridZ = CurrentGridZ;

                        // Calculate blend strength based on distance
                        float BlendStrength = 1.0f;
                        const float SolidRadius = CorrectedRadius * FalloffStart;

                        if (Distance > SolidRadius)
                        {
                            // Smooth falloff from solid to edge
                            float FalloffDistance = CorrectedRadius - SolidRadius;
                            float NormalizedDistance = (Distance - SolidRadius) / FalloffDistance;
                            BlendStrength = 1.0f - NormalizedDistance;
                        }

                        if (Editor->mCurrentTextureLayer >= 120)
                        {
                            Edit.mTextureLayerA = 120;
                            Edit.mTextureLayerB = 120;
                            Edit.mBlend = 0;
                        }
                        else if (Editor->mSelectedLayer == 0)
                        {
                            // Update Layer A
                            Edit.mTextureLayerA = Editor->mCurrentTextureLayer;
                            Edit.mTextureLayerB = TerrainRenderer->mLayersTextureData[(Edit.mGridX + Edit.mGridZ * (256 * 16)) * 3 + 1];

                            // Check if Layer A already has content (blend closer to 0 = more Layer A)
                            // If blend is low, Layer A is already visible
                            bool LayerAHasContent = (ExistingBlend < 128);

                            uint8_t NewBlend = static_cast<uint8_t>(255.0f * (1.0f - BlendStrength));

                            if (LayerAHasContent)
                            {
                                // Layer A already exists, only strengthen it (move blend towards 0)
                                Edit.mBlend = std::min(ExistingBlend, NewBlend);
                            }
                            else
                            {
                                // Layer A is weak/empty, apply normal blend
                                Edit.mBlend = NewBlend;
                            }
                        }
                        else // mSelectedLayer == 1
                        {
                            // Update Layer B
                            Edit.mTextureLayerB = Editor->mCurrentTextureLayer;
                            Edit.mTextureLayerA = TerrainRenderer->mLayersTextureData[(Edit.mGridX + Edit.mGridZ * (256 * 16)) * 3 + 0];

                            // Check if Layer B already has content (blend closer to 255 = more Layer B)
                            bool LayerBHasContent = (ExistingBlend > 128);

                            uint8_t NewBlend = static_cast<uint8_t>(255.0f * BlendStrength);

                            if (LayerBHasContent)
                            {
                                // Layer B already exists, only strengthen it (move blend towards 255)
                                Edit.mBlend = std::max(ExistingBlend, NewBlend);
                            }
                            else
                            {
                                // Layer B is weak/empty, apply normal blend
                                Edit.mBlend = NewBlend;
                            }
                        }

                        Edits.push_back(Edit);
                    }
                }
            }
        }

        return Edits;
    }

	void TerrainEditor::DrawMenu(void* MapEditorPtr)
	{
        application::rendering::map_editor::UIMapEditor* MapEditor = reinterpret_cast<application::rendering::map_editor::UIMapEditor*>(MapEditorPtr);

        ImGui::SeparatorText("Terrain Editor");

        ImGui::Text("Editing Mode");
        ImGuiExt::DrawButtonArray(gEditorModeNames, IM_ARRAYSIZE(gEditorModeNames), 2, reinterpret_cast<int*>(&mEditorMode));

        if (mEditorMode == EditorMode::HEIGHT)
        {
            ImGui::NewLine();
            ImGui::Text("Brush Type");
            ImGuiExt::DrawButtonArray(gBrushNames, IM_ARRAYSIZE(gBrushNames), 3, reinterpret_cast<int*>(&mBrushType));

            ImGui::NewLine();
            ImGui::Text("Fall Off Function");
            ImGuiExt::DrawButtonArray(gFallOffFunctionNames, IM_ARRAYSIZE(gFallOffFunctionNames), 3, reinterpret_cast<int*>(&mFallOffFunction));

            if (ImGui::BeginTable("TerrainEditorTable", 2, ImGuiTableFlags_BordersInnerV))
            {
                ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("Clip To Max Terrain Height").x);

                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Fall Off Rate");
                ImGui::TableNextColumn();
                ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
                ImGui::InputScalar("##FallOffRateTerrainEditor", ImGuiDataType_Float, &mFallOffRate);
                ImGui::PopItemWidth();
                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Brush Strength");
                ImGui::TableNextColumn();
                ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
                ImGui::InputScalar("##StrengthTerrainEditor", ImGuiDataType_Float, &mStrength);
                ImGui::PopItemWidth();
                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Brush Radius");
                ImGui::TableNextColumn();
                ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
                ImGui::InputScalar("##RadiusTerrainEditor", ImGuiDataType_Float, &mRadius);
                ImGui::PopItemWidth();

                ImGui::EndTable();
            }
        }
        else
        {
            if (ImGui::BeginTable("TerrainEditorTable", 2, ImGuiTableFlags_BordersInnerV))
            {
                ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("Clip To Max Terrain Height").x);

                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Render Layer A");
                ImGui::TableNextColumn();
                ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
                if (ImGui::Checkbox("##RenderLayerATerrainEditor", &mRenderLayerA))
                {
                    MapEditor->mScene.mTerrainRenderer->mRenderLayerA = mRenderLayerA;
                }
                ImGui::PopItemWidth();
                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Render Layer B");
                ImGui::TableNextColumn();
                ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
                if (ImGui::Checkbox("##RenderLayerBTerrainEditor", &mRenderLayerB))
                {
                    MapEditor->mScene.mTerrainRenderer->mRenderLayerB = mRenderLayerB;
                }
                ImGui::PopItemWidth();
                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Fall Off Rate");
                ImGui::TableNextColumn();
                ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
                ImGui::InputScalar("##FallOffRateTerrainEditor", ImGuiDataType_Float, &mFallOffRate);
                ImGui::PopItemWidth();
                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Brush Radius");
                ImGui::TableNextColumn();
                ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
                ImGui::InputScalar("##RadiusTerrainEditor", ImGuiDataType_Float, &mRadius);
                ImGui::PopItemWidth();
                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Brush Layer");
                ImGui::TableNextColumn();
                ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
                static const char* Layers[] = { "Layer A", "Layer B" };
                ImGui::Combo("##RenderBrushLayer", &mSelectedLayer, Layers, IM_ARRAYSIZE(Layers));
                ImGui::PopItemWidth();


                ImGui::EndTable();
            }

            constexpr static int Columns = 4;

            ImGui::Text("Brush Texture:");
            ImGui::SameLine();

            ImGui::PushID("TextureSelector");

            // Display currently selected texture as a clickable button
            ImVec2 uv0(0.0f, 0.0f);
            ImVec2 uv1(1.0f, 1.0f);
            
            bool clicked = false;

            if (mCurrentTextureLayer == 120)
            {
                clicked = ImGui::ImageButton("CurrentTexture",
                    (ImTextureID)(intptr_t)gTerrainHoleTexture,
                    ImVec2(256, 256),
                    ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f),
                    ImVec4(0.2f, 0.2f, 0.2f, 1.0f),  // background
                    ImVec4(1.0f, 1.0f, 1.0f, 1.0f)   // tint
                );
            }
            else
            {
                clicked = ImGui::ImageButton("CurrentTexture",
                    (ImTextureID)(intptr_t)gTerrainTextureArrayViews[mCurrentTextureLayer],
                    ImVec2(256, 256),
                    uv0, uv1,
                    ImVec4(0.2f, 0.2f, 0.2f, 1.0f),  // background
                    ImVec4(1.0f, 1.0f, 1.0f, 1.0f)   // tint
                );
            }

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

                    if (i == 120)
                    {
                        clickedNewTexture = ImGui::ImageButton(("TextureTerrainSelector_" + std::to_string(i)).c_str(),
                            (ImTextureID)(intptr_t)gTerrainHoleTexture,
                            ImVec2(256, 256),
                            ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f),
                            ImVec4(0.15f, 0.15f, 0.15f, 1.0f),  // background
                            ImVec4(1.0f, 1.0f, 1.0f, 1.0f)      // tint
                        );
                    }
                    else
                    {
                        clickedNewTexture = ImGui::ImageButton(("TextureTerrainSelector_" + std::to_string(i)).c_str(),
                            (ImTextureID)(intptr_t)gTerrainTextureArrayViews[i],
                            ImVec2(256, 256),
                            uv0, uv1,
                            ImVec4(0.15f, 0.15f, 0.15f, 1.0f),  // background
                            ImVec4(1.0f, 1.0f, 1.0f, 1.0f)      // tint
                        );
                    }

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
        }
	}

    void TerrainEditor::ApplyBrush(int TexX, int TexY, void* MapEditorPtr)
    {
        constexpr int TextureSize = 256 * 16;

        application::rendering::map_editor::UIMapEditor* MapEditor = reinterpret_cast<application::rendering::map_editor::UIMapEditor*>(MapEditorPtr);

        const uint16_t CorrectedRadius = static_cast<uint16_t>(std::round((256 * 16) / 1000.0f * mRadius));

        if (mEditorMode == EditorMode::HEIGHT)
        {
            std::vector<TerrainEditor::TerrainHeightEdit> Edits;

            switch (mBrushType)
            {
            case BrushType::SCULPT:
                Edits = TerrainBrushes::HeightBrushSculpt(CorrectedRadius, TexX, TexY, this, MapEditor->mScene.mTerrainRenderer);
                break;
            case BrushType::SMOOTH:
                Edits = TerrainBrushes::HeightBrushSmooth(CorrectedRadius, TexX, TexY, this, MapEditor->mScene.mTerrainRenderer);
                break;
            case BrushType::PLATEAU:
                Edits = TerrainBrushes::HeightBrushPlateau(CorrectedRadius, TexX, TexY, this, MapEditor->mScene.mTerrainRenderer);
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

                MapEditor->mScene.mTerrainRenderer->mHeightTextureData[Edit.mGridX + Edit.mGridZ * TextureSize] = Edit.mNewHeight;
            }

            glBindTexture(GL_TEXTURE_2D, MapEditor->mScene.mTerrainRenderer->mHeightTextureID);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256 * 16, 256 * 16, GL_RED, GL_FLOAT, MapEditor->mScene.mTerrainRenderer->mHeightTextureData.data());
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

                MapEditor->mScene.mTerrainRenderer->mLayersTextureData[(Edit.mGridX + Edit.mGridZ * TextureSize) * 3] = Edit.mTextureLayerA;
                MapEditor->mScene.mTerrainRenderer->mLayersTextureData[(Edit.mGridX + Edit.mGridZ * TextureSize) * 3 + 1] = Edit.mTextureLayerB;
                MapEditor->mScene.mTerrainRenderer->mLayersTextureData[(Edit.mGridX + Edit.mGridZ * TextureSize) * 3 + 2] = Edit.mBlend;
            }

            glBindTexture(GL_TEXTURE_2D, MapEditor->mScene.mTerrainRenderer->mLayersTextureID);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256 * 16, 256 * 16, GL_RGB_INTEGER, GL_UNSIGNED_BYTE, MapEditor->mScene.mTerrainRenderer->mLayersTextureData.data());
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
            MapEditor->gInstancedShader->Bind();


            glm::mat4 GLMModel = glm::mat4(1.0f);
            GLMModel = glm::translate(GLMModel, Hit.mPosition);
            GLMModel = glm::scale(GLMModel, glm::vec3(mRadius, mRadius, mRadius));

            gSphereShader->Bind();
            MapEditor->mCamera.Matrix(gSphereShader, "camMatrix");
            glUniformMatrix4fv(glGetUniformLocation(gSphereShader->mID, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(GLMModel));
            gSphereMesh.Draw();

            const float CurrentTime = ImGui::GetTime();

            if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                glm::vec2 TerrainMin = glm::vec2(MapEditor->mScene.mTerrainRenderer->mTerrainPosition.x - 500.0f,
                    MapEditor->mScene.mTerrainRenderer->mTerrainPosition.z - 500.0f);
                glm::vec2 TerrainMax = glm::vec2(MapEditor->mScene.mTerrainRenderer->mTerrainPosition.x + 500.0f,
                    MapEditor->mScene.mTerrainRenderer->mTerrainPosition.z + 500.0f);
                glm::vec2 LocalPos = (glm::vec2(Hit.mPosition.x, Hit.mPosition.z) - TerrainMin) / (TerrainMax - TerrainMin);

                constexpr int TextureSize = 256 * 16;
                int TexX = static_cast<int>(LocalPos.x * (TextureSize - 1));
                int TexY = static_cast<int>(LocalPos.y * (TextureSize - 1));

                std::cout << "Height at pos " << TexX << ", " << TexY << ": " 
                          << MapEditor->mScene.mTerrainRenderer->mHeightTextureData[TexX + (256 * 16) * TexY] 
					<< std::endl;

                if (!mIsMouseDragging)
                {
                    mPlateauHeight = MapEditor->mScene.mTerrainRenderer->mHeightTextureData[TexX + (256 * 16) * TexY];

                    ApplyBrush(TexX, TexY, MapEditorPtr);

                    mIsMouseDragging = true;
                    mLastBrushApplyTime = CurrentTime;
                    mLastBrushPosition = glm::vec2(TexY);
                }
                else
                {
                    const glm::vec2 Intersection2d = glm::vec2(TexX, TexY);
                    const float DistanceMoved = glm::distance(mLastBrushPosition, Intersection2d);

                    const bool TimeThresholdMet = (CurrentTime - mLastBrushApplyTime) >= mBrushUpdateInterval;
                    const bool DistanceThresholdMet = DistanceMoved >= mRadius;

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
    }
}