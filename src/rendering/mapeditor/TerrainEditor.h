#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <gl/TerrainRenderer.h>
#include <gl/Shader.h>
#include <gl/SimpleMesh.h>
#include <limits>
#include <imgui.h>
#include <vector>

namespace application::rendering::map_editor
{
	class TerrainRaycast
	{
	public:
		struct RaycastHit 
		{
			bool mHit = false;
			glm::vec3 mPosition;
			glm::vec3 mNormal;
			float mDistance = 0.0f;
		};

		TerrainRaycast(application::gl::TerrainRenderer* Renderer);
		RaycastHit RaycastTerrain(const glm::vec3& rayOrigin, const glm::vec3& rayDirection) const;
	private:
		application::gl::TerrainRenderer* mTerrainRenderer = nullptr;

		float SampleHeightAtWorldPos(const glm::vec2& worldPos) const;
		glm::vec3 CalculateNormalAtWorldPos(const glm::vec2& worldPos) const;
		bool RayAABBIntersect(const glm::vec3& rayOrigin, const glm::vec3& rayDir, const glm::vec3& boxMin, const glm::vec3& boxMax, float& tMin, float& tMax) const;
	};

	namespace TerrainMath
	{
		float GaussianFallOff(float Distance, float Radius, float FallOffRate = 0.5f);
		float LinearFallOff(float Distance, float Radius, float FallOffRate = 0.5f);
	}

	class TerrainEditor
	{
	public:
		struct TerrainHeightEdit
		{
			int16_t mGridX = 0;
			int16_t mGridZ = 0;
			float mNewHeight = 0.0f;
		};

		struct TerrainTextureEdit
		{
			int16_t mGridX = 0;
			int16_t mGridZ = 0;

			uint8_t mTextureLayerA = 0;
			uint8_t mTextureLayerB = 0;
			uint8_t mBlend = 0;
		};

		enum class BrushType : int
		{
			RAISE = 0,
			LOWER = 1,
			SMOOTH = 2,
			FLATTEN = 3,
			NOISE = 4
		};

		enum class TextureBrushType : int
		{
			PAINT = 0,
			REPLACE = 1,
			SMOOTH = 2
		};

		enum class TextureLayerMode : int
		{
			AUTO = 0,
			LAYER_A = 1,
			LAYER_B = 2
		};

		enum class FallOffFunction : int
		{
			NONE = 0,
			GAUSSIAN = 1,
			LINEAR = 2
		};

		enum class EditorMode : int
		{
			HEIGHT = 0,
			TEXTURE = 1
		};

		static application::gl::Shader* gSphereShader;
		static application::gl::SimpleMesh gSphereMesh;
			static const char* gEditorModeNames[];
			static const char* gHeightBrushNames[];
			static const char* gTextureBrushNames[];
			static const char* gFallOffFunctionNames[];
			static const char* gTextureLayerModeNames[];
		static std::vector<GLuint> gTerrainTextureArrayViews;
		static GLuint gTerrainHoleTexture;

		static void Initialize();
		static void InitializeTerrainTextures();

		void SetEnabled(bool Enabled, void* MapEditorPtr);
		void DrawMenu(void* MapEditorPtr);
		void RenderTerrainEditor(float XPos, float YPos, int ScreenWidth, int ScreenHeight, void* MapEditorPtr);

		EditorMode mEditorMode = EditorMode::HEIGHT;

			bool mEnabled = false;
			BrushType mBrushType = BrushType::RAISE;
			TextureBrushType mTextureBrushType = TextureBrushType::PAINT;
			FallOffFunction mFallOffFunction = FallOffFunction::GAUSSIAN;
			float mFallOffRate = 0.5f;
			float mStrength = 1.0f;
			float mRadius = 4.0f;
			int mCurrentTextureLayer = 0;

		float mPlateauHeight = 0.0f;

			bool mRenderLayerA = true;
			bool mRenderLayerB = true;
			TextureLayerMode mTextureLayerMode = TextureLayerMode::AUTO;
		private:
			void ApplyBrush(int TexX, int TexY, void* MapEditorPtr);
			void DrawBrushGizmo(const glm::vec3& HitPos, const glm::vec3& HitNormal, void* MapEditorPtr);
			void UpdateLivePreview(int TexX, int TexY, void* MapEditorPtr);

			bool mIsMouseDragging = false;
			float mLastBrushApplyTime = 0.0f;
			glm::vec2 mLastBrushPosition = glm::vec2(-1.0f, -1.0f);
			float mBrushUpdateInterval = 0.05f;

			bool mShowBrushPreview = true;
			bool mShowLiveEditPreview = true;
			float mBrushPreviewOpacity = 0.6f;

			bool mHasLivePreview = false;
			int mPreviewTexX = -1;
			int mPreviewTexY = -1;
			int mPreviewAffectedSamples = 0;
			float mPreviewCurrentHeight = 0.0f;
			float mPreviewTargetHeight = 0.0f;
			float mPreviewDeltaHeight = 0.0f;
			uint8_t mPreviewLayerA = 0;
			uint8_t mPreviewLayerB = 0;
			uint8_t mPreviewBlend = 0;
		};

		namespace TerrainBrushes
		{
			std::vector<TerrainEditor::TerrainHeightEdit> HeightBrushSculpt(const uint16_t& CorrectedRadius, const int16_t& CenterGridX, const int16_t& CenterGridZ, TerrainEditor* Editor, application::gl::TerrainRenderer* TerrainRenderer);
			std::vector<TerrainEditor::TerrainHeightEdit> HeightBrushSmooth(const uint16_t& CorrectedRadius, const int16_t& CenterGridX, const int16_t& CenterGridZ, TerrainEditor* Editor, application::gl::TerrainRenderer* TerrainRenderer);
			std::vector<TerrainEditor::TerrainHeightEdit> HeightBrushPlateau(const uint16_t& CorrectedRadius, const int16_t& CenterGridX, const int16_t& CenterGridZ, TerrainEditor* Editor, application::gl::TerrainRenderer* TerrainRenderer);
			std::vector<TerrainEditor::TerrainHeightEdit> HeightBrushNoise(const uint16_t& CorrectedRadius, const int16_t& CenterGridX, const int16_t& CenterGridZ, TerrainEditor* Editor, application::gl::TerrainRenderer* TerrainRenderer);
			
			std::vector<TerrainEditor::TerrainTextureEdit> TextureBrush(const uint16_t& CorrectedRadius, const int16_t& CenterGridX, const int16_t& CenterGridZ, TerrainEditor* Editor, application::gl::TerrainRenderer* TerrainRenderer);
		};
}
