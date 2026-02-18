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
			SCULPT = 0,
			SMOOTH = 1,
			PLATEAU = 2
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
		static const char* gBrushNames[];
		static const char* gFallOffFunctionNames[];
		static std::vector<GLuint> gTerrainTextureArrayViews;
		static GLuint gTerrainHoleTexture;

		static void Initialize();
		static void InitializeTerrainTextures();

		void SetEnabled(bool Enabled, void* MapEditorPtr);
		void DrawMenu(void* MapEditorPtr);
		void RenderTerrainEditor(float XPos, float YPos, int ScreenWidth, int ScreenHeight, void* MapEditorPtr);

		EditorMode mEditorMode = EditorMode::HEIGHT;

		bool mEnabled = false;
		BrushType mBrushType = BrushType::SCULPT;
		FallOffFunction mFallOffFunction = FallOffFunction::GAUSSIAN;
		float mFallOffRate = 0.5f;
		float mStrength = 1.0f;
		float mRadius = 4.0f;
		int mCurrentTextureLayer = 0;

		float mPlateauHeight = 0.0f;

		bool mRenderLayerA = true;
		bool mRenderLayerB = true;
		int mSelectedLayer = 0;
	private:
		void ApplyBrush(int TexX, int TexY, void* MapEditorPtr);

		bool mIsMouseDragging = false;
		float mLastBrushApplyTime = 0.0f;
		glm::vec2 mLastBrushPosition = glm::vec2(-1.0f, -1.0f);
		float mBrushUpdateInterval = 0.05f;
	};

	namespace TerrainBrushes
	{
		std::vector<TerrainEditor::TerrainHeightEdit> HeightBrushSculpt(const uint16_t& CorrectedRadius, const int16_t& CenterGridX, const int16_t& CenterGridZ, TerrainEditor* Editor, application::gl::TerrainRenderer* TerrainRenderer);
		std::vector<TerrainEditor::TerrainHeightEdit> HeightBrushSmooth(const uint16_t& CorrectedRadius, const int16_t& CenterGridX, const int16_t& CenterGridZ, TerrainEditor* Editor, application::gl::TerrainRenderer* TerrainRenderer);
		std::vector<TerrainEditor::TerrainHeightEdit> HeightBrushPlateau(const uint16_t& CorrectedRadius, const int16_t& CenterGridX, const int16_t& CenterGridZ, TerrainEditor* Editor, application::gl::TerrainRenderer* TerrainRenderer);
		
		std::vector<TerrainEditor::TerrainTextureEdit> TextureBrush(const uint16_t& CorrectedRadius, const int16_t& CenterGridX, const int16_t& CenterGridZ, TerrainEditor* Editor, application::gl::TerrainRenderer* TerrainRenderer);
	};
}