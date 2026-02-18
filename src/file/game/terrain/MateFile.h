#pragma once

#include <vector>
#include <glm/vec4.hpp>
#include <utility>
#include <file/game/terrain/TerrainSceneFile.h>

namespace application::file::game::terrain
{
	class MateFile
	{
	public:
		static const std::vector<uint8_t> TEXTURE_INDEX_MAP;

		struct MaterialIndexInfo
		{
			uint8_t mIndexA;
			uint8_t mIndexB;
			uint8_t mBlendFactor;
		};

		void Initialize(std::vector<unsigned char> Data, application::file::game::terrain::TerrainSceneFile* SceneFile, uint16_t Width = 260, uint16_t Height = 260);
		std::vector<unsigned char> ToBinary();

		MaterialIndexInfo* GetMaterialAtGridPos(const uint16_t& X, const uint16_t& Y);
		glm::vec4* GetTexCoordAtGridPos(const uint16_t& X, const uint16_t& Y);
		uint16_t& GetWidth();
		uint16_t& GetHeight();
		void GenerateTexCoords(application::file::game::terrain::TerrainSceneFile* SceneFile);

		MateFile(std::vector<unsigned char> Data, application::file::game::terrain::TerrainSceneFile* SceneFile, uint16_t Width = 260, uint16_t Height = 260);
		MateFile() = default;

		std::vector<MaterialIndexInfo> mMaterials;
		std::vector<glm::vec4> mTexCoords;
		bool mModified = false;
		uint16_t mWidth = 0;
		uint16_t mHeight = 0;
	};
}