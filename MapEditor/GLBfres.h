#pragma once

#include <glm/glm.hpp>
#include <vector>
#include "Bfres.h"
#include "BufferObject.h"
#include "Shader.h"
#include "GLBfresVAO.h"
#include <unordered_map>

class GLBfres
{
public:
	struct FormatInfo
	{
		int32_t m_Count;
		bool m_Normalized;
		GLenum m_Type;
	};

	enum class RenderStateMode : uint8_t
	{
		OPAQUE = 0,
		TRANSLUCENT = 1,
		MASK = 2
	};

	struct GLMaterial
	{
		bool mEnableAlphaTest = false;
		RenderStateMode mRenderStateMode = RenderStateMode::OPAQUE;
		uint16_t mTextureArrayIndex = 0;

		GLenum mRenderStateDisplayFace = GL_NONE;

		Texture* mAlbedoTexture = nullptr;

		GLenum mIndexFormat = GL_UNSIGNED_INT;
	};

	static std::unordered_map<BfresFile::BfresAttribFormat, FormatInfo> mFormatList;
	static std::unordered_map<std::string, uint32_t> mAttributeLocations;
	static void (*mDrawFunc)(GLBfres*, std::vector<glm::mat4>&, Shader*);

	std::vector<std::pair<BufferObject, uint32_t>> mIndexBuffers; //Buffer, count
	std::vector<std::vector<BufferObject>> mShapeBuffers;
	std::vector<GLBfresVAO> mShapeVAOs;
	std::vector<GLMaterial> mMaterials;
	std::vector<uint16_t> mOpaqueObjects;
	std::vector<uint16_t> mTransparentObjects;
	BufferObject mInstanceMatrix;
	BfresFile* mBfres;
	bool mIsDiscard;

	GLBfres(BfresFile* Bfres, GLenum TextureFilter);
	GLBfres() = default;

	void LoadFallbackTexture(GLMaterial& Material);
	static void DrawCulled(GLBfres* Parent, std::vector<glm::mat4>& ModelMatrices, Shader* Shader);
	static void DrawNotCulled(GLBfres* Parent, std::vector<glm::mat4>& ModelMatrices, Shader* Shader);
	void Draw(std::vector<glm::mat4>& ModelMatrices, Shader* Shader);
	void Delete();
};

namespace GLTextureLibrary
{
	extern std::unordered_map<TextureToGo::Surface*, Texture> mTextures;

	bool IsTextureLoaded(TextureToGo::Surface* TexToGo);
	Texture* GetTexture(TextureToGo::Surface* TexToGo, GLenum Slot = GL_TEXTURE0, bool GenMipMaps = true, GLenum TextureFilter = GL_LINEAR);
	std::unordered_map<TextureToGo::Surface*, Texture>& GetTextures();
	void Cleanup();
};

namespace GLBfresLibrary
{
	extern std::unordered_map<BfresFile*, GLBfres> mModels;

	bool IsModelLoaded(BfresFile* Model);
	GLBfres* GetModel(BfresFile* Model, GLenum TextureFilter = GL_LINEAR);
	void Cleanup();
};