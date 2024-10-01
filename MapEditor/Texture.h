#pragma once

#include <glad/glad.h>
#include <vector>
#include <stb/stb_image.h>
#include <string>

#include "TextureToGo.h"
#include "Shader.h"

class Texture
{
public:
	GLuint ID;
	GLenum type;
	std::string Sampler;
	GLenum Slot;
	Texture(const char* image, GLenum texType, GLenum slot, GLenum format, GLenum pixelType);
	Texture(std::vector<unsigned char> Pixels, int Width, int Height, GLenum ValueType, GLenum texType, GLenum slot, GLenum format, GLenum pixelType, std::string TexSampler, bool GenMipMaps = true);
	Texture(TextureToGo::Surface* Surface, GLenum Slot, bool GenMipMaps = true, GLenum TextureFilter = GL_LINEAR);
	Texture() {};

	// Assigns a texture unit to a texture
	void texUnit(Shader& shader, const char* uniform, GLuint unit);
	// Binds a texture
	void Bind();
	void ActivateTextureUnit();
	// Unbinds a texture
	void Unbind();
	// Deletes a texture
	void Delete();
};