#pragma once

#include <glad/glad.h>
#include <file/game/bfres/BfresFile.h>
#include <unordered_map>
#include <gl/BufferObject.h>
#include <gl/Texture.h>
#include <gl/VertexArrayObject.h>

namespace application::gl
{
	class BfresRenderer
	{
	public:
		struct FormatInfo
		{
			int32_t mCount;
			bool mNormalized;
			GLenum mType;
		};

		enum class RenderStateMode : uint8_t
		{
			OPAQUE = 0,
			TRANSLUCENT = 1,
			MASK = 2
		};

		struct Material
		{
			bool mEnableAlphaTest = false;
			RenderStateMode mRenderStateMode = RenderStateMode::OPAQUE;
			uint16_t mTextureArrayIndex = 0;

			GLenum mRenderStateDisplayFace = GL_NONE;
			GLenum mIndexFormat = GL_UNSIGNED_INT;

			application::gl::Texture* mAlbedoTexture = nullptr;
		};

		struct DrawElementsIndirectCommand
		{
			uint32_t mCount;
			uint32_t mInstanceCount;
			uint32_t mFirstIndex;
			int32_t mBaseVertex;
			uint32_t mBaseInstance;
		};

		static std::unordered_map<application::file::game::bfres::BfresFile::BfresAttribFormat, application::gl::BfresRenderer::FormatInfo> gFormatList;
		static std::unordered_map<std::string, uint32_t> gAttributeLocations;

		BfresRenderer() = default;
		BfresRenderer(application::file::game::bfres::BfresFile* BfresFile);

		void LoadFallbackTexture(Material& Material);
		void Initialize(application::file::game::bfres::BfresFile* BfresFile);
		void Draw(std::vector<glm::mat4>& ModelMatrices);
		void Delete();

		application::file::game::bfres::BfresFile* mBfresFile = nullptr;
		application::gl::BufferObject mInstanceMatrix;
		std::vector<std::pair<application::gl::BufferObject, uint32_t>> mIndexBuffers;
		std::vector<std::vector<BufferObject>> mShapeBuffers;
		std::vector<application::gl::VertexArrayObject> mShapeVAOs;
		std::vector<Material> mMaterials;
		std::vector<uint16_t> mOpaqueObjects;
		std::vector<uint16_t> mTransparentObjects;
		bool mIsDiscard = false;
		bool mIsSystemModelTransparent = false;
		float mSphereBoundingBoxRadius = 0.0f;
	};
}