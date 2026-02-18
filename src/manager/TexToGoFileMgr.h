#pragma once

#include <unordered_map>
#include <string>
#include <file/game/texture/TexToGoFile.h>

namespace application::manager
{
	namespace TexToGoFileMgr
	{
		enum class AccesMode : uint8_t
		{
			NAME = 0,
			PATH = 1
		};

		extern std::unordered_map<std::string, application::file::game::texture::TexToGoFile> gTexToGoFiles;

		bool IsTextureLoaded(const std::string& Name, AccesMode Mode = AccesMode::NAME);
		application::file::game::texture::TexToGoFile* GetTexture(const std::string& Name, AccesMode Mode = AccesMode::NAME);
	}
}