#pragma once

#include <glad/glad.h>
#include <unordered_map>
#include <file/game/texture/TextureFormat.h>
#include <string>

namespace application::file::game::texture
{
	namespace FormatHelper
	{
		extern std::unordered_map<TextureFormat::Format, GLenum> gInternalFormatList;
		extern std::unordered_map<GLenum, int32_t> gBlockSizeByFormat;
		extern std::unordered_map<std::string, GLenum> gAlphaTestFunctions;
	}
}