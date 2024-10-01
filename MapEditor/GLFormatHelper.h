#pragma once

#include <unordered_map>
#include <glad/glad.h>
#include "TextureFormat.h"
#include <cinttypes>
#include <string>

namespace GLFormatHelper
{
	extern std::unordered_map<TextureFormat::Format, GLenum> InternalFormatList;
	extern std::unordered_map<GLenum, int32_t> BlockSizeByFormat;
	extern std::unordered_map<std::string, GLenum> mAlphaTestFunctions;
};