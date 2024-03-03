#pragma once

#include <unordered_map>
#include "Shader.h"

namespace ShaderMgr
{
	extern std::unordered_map<std::string, Shader> Shaders;

	Shader* GetShader(std::string Name);
	void Cleanup();
}