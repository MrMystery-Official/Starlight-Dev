#pragma once

#include <glad/glad.h>
#include <string>
#include <vector>
#include <file/game/texture/TexToGoFile.h>

namespace application::gl
{
	class Texture
	{
	public:
		GLuint mID = 0xFFFFFFFF;
		GLenum mType = GL_NONE;
		GLenum mSlot = GL_NONE;
		std::string mSampler = "";
		int mWidth = 0;
		int mHeight = 0;
		int mNumColCh = 0;

		Texture() = default;
		Texture(const char* Image, GLenum TexType, GLenum Slot, GLenum Format, GLenum PixelType);
		Texture(std::vector<unsigned char> Pixels, const int& Width, const int& Height, GLenum ValueType, GLenum TexType, GLenum Slot, GLenum Format, GLenum PixelType, const std::string& Sampler, bool GenMipMaps = true);
		Texture(application::file::game::texture::TexToGoFile::Surface* Surface, GLenum Slot, bool GenMipMaps = true, GLenum TextureFilter = GL_LINEAR);

		void Bind();
		void Unbind();
		void Delete();
		void ActivateTextureUnit();
	};
}