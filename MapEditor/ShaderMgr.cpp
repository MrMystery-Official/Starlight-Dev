#include "ShaderMgr.h"

#include "Logger.h"

std::unordered_map<std::string, Shader> ShaderMgr::Shaders;

Shader* ShaderMgr::GetShader(std::string Name)
{
	if (!Shaders.count(Name))
	{
		Shaders.insert({ Name, Shader(("Assets/Shaders/" + Name + ".vert").c_str(), ("Assets/Shaders/" + Name + ".frag").c_str()) });
		Logger::Info("ShaderMgr", "Created shader " + Name);
	}

	return &Shaders[Name];
}

void ShaderMgr::Cleanup()
{
	for (auto& [Name, Shader] : Shaders)
	{
		glDeleteProgram(Shader.ID);
		Logger::Info("ShaderMgr", "Deleted shader " + Name);
	}
	Shaders.clear();
}