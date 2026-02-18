#pragma once

#include <unordered_map>
#include <string>
#include <gl/Shader.h>

namespace application::manager
{
	namespace ShaderMgr
	{
		extern std::unordered_map<std::string, application::gl::Shader> gShaders;

		application::gl::Shader* GetShader(const std::string& Name, bool Tessellation = false);
		void Cleanup();
	}
}