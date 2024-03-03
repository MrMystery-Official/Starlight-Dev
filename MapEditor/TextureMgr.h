#pragma once

#include <string>
#include <GLFW/glfw3.h>
#include <unordered_map>

namespace TextureMgr
{
	struct Texture
	{
		GLuint ID;
		uint32_t Width;
		uint32_t Height;
	};

	extern std::unordered_map<std::string, Texture> Textures;

	TextureMgr::Texture* GetTexture(std::string Name);
	void Cleanup();
};