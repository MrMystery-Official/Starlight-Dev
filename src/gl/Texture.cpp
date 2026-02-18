#include "Texture.h"

#include <stb/stb_image.h>
#include <file/game/texture/FormatHelper.h>

namespace application::gl
{
	Texture::Texture(const char* Image, GLenum TexType, GLenum Slot, GLenum Format, GLenum PixelType)
	{
		mType = TexType;
		mSlot = Slot;

		stbi_set_flip_vertically_on_load(true);
		unsigned char* Bytes = stbi_load(Image, &mWidth, &mHeight, &mNumColCh, 0);

		glGenTextures(1, &mID);
		glActiveTexture(Slot);
		glBindTexture(TexType, mID);

		glTexParameteri(TexType, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
		glTexParameteri(TexType, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(TexType, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(TexType, GL_TEXTURE_WRAP_T, GL_REPEAT);

		glTexImage2D(TexType, 0, GL_RGBA, mWidth, mHeight, 0, Format, PixelType, Bytes);
		glGenerateMipmap(TexType);

		stbi_image_free(Bytes);

		glBindTexture(TexType, 0);
	}

	Texture::Texture(std::vector<unsigned char> Pixels, const int& Width, const int& Height, GLenum ValueType, GLenum TexType, GLenum Slot, GLenum Format, GLenum PixelType, const std::string& Sampler, bool GenMipMaps)
	{
		mType = TexType;
		mSampler = Sampler;
		mSlot = Slot;

		glGenTextures(1, &mID);
		glActiveTexture(Slot);
		glBindTexture(TexType, mID);

		glTexParameteri(TexType, GL_TEXTURE_MIN_FILTER, GenMipMaps ? GL_NEAREST_MIPMAP_LINEAR : GL_NEAREST);
		glTexParameteri(TexType, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(TexType, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(TexType, GL_TEXTURE_WRAP_T, GL_REPEAT);

		glTexImage2D(TexType, 0, ValueType, Width, Height, 0, Format, PixelType, Pixels.data());
		if (GenMipMaps)
			glGenerateMipmap(TexType);

		glBindTexture(TexType, 0);
	}

	Texture::Texture(application::file::game::texture::TexToGoFile::Surface* Surface, GLenum Slot, bool GenMipMaps, GLenum TextureFilter)
	{
		// Assigns the type of the texture ot the texture object
		mType = GL_TEXTURE_2D;
		mSlot = Slot;

		glGenTextures(1, &mID);
		glActiveTexture(Slot);
		glBindTexture(GL_TEXTURE_2D, mID);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GenMipMaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		if (Surface->mPolishedFormat != application::file::game::texture::TextureFormat::Format::CPU_DECODED)
		{
			glCompressedTexImage2D(GL_TEXTURE_2D, 0, application::file::game::texture::FormatHelper::gInternalFormatList[Surface->mPolishedFormat], Surface->mWidth, Surface->mHeight, 0, Surface->mData.size(), Surface->mData.data());
		}
		else
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, Surface->mWidth, Surface->mHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, Surface->mData.data());
		}

		if (GenMipMaps)
			glGenerateMipmap(GL_TEXTURE_2D);

		glBindTexture(GL_TEXTURE_2D, 0);
	}

	/*
	void Texture::texUnit(Shader& shader, const char* uniform, GLuint unit)
	{
		// Gets the location of the uniform
		GLuint texUni = glGetUniformLocation(shader.ID, uniform);
		// Sets the value of the uniform
		glUniform1i(texUni, unit);
	}
	*/

	void Texture::Bind()
	{
		glBindTexture(mType, mID);
	}

	void Texture::ActivateTextureUnit()
	{
		glActiveTexture(mSlot);
	}

	void Texture::Unbind()
	{
		glBindTexture(mType, 0);
	}

	void Texture::Delete()
	{
		glDeleteTextures(1, &mID);
	}
}