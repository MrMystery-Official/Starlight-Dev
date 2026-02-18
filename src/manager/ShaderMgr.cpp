#include "ShaderMgr.h"

#include <util/Logger.h>

namespace application::manager
{
	std::unordered_map<std::string, application::gl::Shader> ShaderMgr::gShaders;

	application::gl::Shader* ShaderMgr::GetShader(const std::string& Name, bool Tessellation)
	{
		if (!gShaders.contains(Name))
		{
			if(!Tessellation)
				gShaders.insert({ Name, application::gl::Shader("Assets/Shaders/" + Name + ".vert", "Assets/Shaders/" + Name + ".frag") });
			else
				gShaders.insert({ Name, application::gl::Shader("Assets/Shaders/" + Name + ".vert", "Assets/Shaders/" + Name + ".frag", "Assets/Shaders/" + Name + ".tcs", "Assets/Shaders/" + Name + ".tes" ) });
			application::util::Logger::Info("ShaderMgr", "Created shader %s", Name.c_str());
		}

		return &gShaders[Name];
	}

	void ShaderMgr::Cleanup()
	{
		for (auto& [Name, Shader] : gShaders)
		{
			Shader.Delete();
			application::util::Logger::Info("ShaderMgr", "Deleted shader %s", Name.c_str());
		}
		gShaders.clear();
	}
}