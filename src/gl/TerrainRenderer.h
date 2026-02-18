#pragma once

#include <vector>
#include <gl/Camera.h>
#include <gl/Shader.h>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <unordered_set>
#include <file/game/phive/util/PhiveMaterialData.h>

namespace application::gl
{
	class TerrainRenderer
	{
	public:
		static std::unordered_map<uint16_t, uint16_t> gMaterialAlbToPhiveMaterialIndex;
		static unsigned int gMaterialAlbID;
		static application::gl::Shader* gShader;
		static unsigned int gTerrainVAO;
		static unsigned int gTerrainVBO;

		static void InitializeMaterialAlbTexture();

		std::vector<float> mHeightTextureData;
		std::vector<unsigned char> mLayersTextureData;
		std::vector<float> mTexCoordsTextureData;
		std::unordered_set<uint16_t> mModifiedTiles; //For height
		std::unordered_set<uint16_t> mModifiedTextureTiles; //For texture
		glm::vec3 mTerrainPosition;
		unsigned int mHeightTextureID = 0;
		unsigned int mTexCoordsTextureID = 0;
		unsigned int mLayersTextureID = 0;
		float mHeightScale = 0.0f;
		float mTerrainYOffset = 0.0f;

		bool mRenderLayerA = true;
		bool mRenderLayerB = true;
		bool mEnableTessellation = true;

		void LoadData(const std::string& SceneName, const std::string& SectionName);
		void Draw(application::gl::Camera& Camera);
		void Save(void* FieldSceneImpl);

		std::pair<std::vector<glm::vec3>, std::vector<std::pair<std::tuple<uint32_t, uint32_t, uint32_t>, uint32_t>>> GenerateTerrainTileCollisionModel(const glm::vec2& WorldOffset, std::vector<application::file::game::phive::util::PhiveMaterialData::PhiveMaterial>* Materials);

	private:
		unsigned int mVertexCount = 0;
		glm::mat4 mModelMatrix = glm::mat4(1.0f);
		std::string mSceneName;
		std::string mSectionName;

		float CalculateErrorForScale(int tileX, int tileY, float targetScale,
			const glm::vec2& TilePos, const glm::vec2& SectionStartPoint);
		std::vector<float> GenerateTerrainVertices(const float Width, const float Height);
		glm::vec2 SectionNameToCoords(const std::string& SectionName);
		glm::vec2 GetSectionMidpoint(const std::string& SectionName);
	};
}