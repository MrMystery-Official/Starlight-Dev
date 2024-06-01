#pragma once

#include "Shader.h"
#include <unordered_map>

namespace ShaderMgr {

extern std::unordered_map<std::string, Shader> Shaders;

Shader* GetShader(std::string Name);
void Cleanup();

}
