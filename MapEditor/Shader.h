#pragma once

#include <glad/glad.h>
#include <unordered_map>
#include <string>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

class Shader
{
public:
	GLuint ID;
	Shader(const char* VertexFile, const char* FragmentFile);
	Shader() {}

	void Activate();
	void Delete();
	void Set(std::string name, int value);
	void Set(std::string name, bool value);
	void Set(std::string name, float value);
	void Set(std::string name, glm::vec2 value);
	void Set(std::string name, glm::vec3 value);
	void Set(std::string name, glm::vec4 value);
	void Set(std::string name, glm::mat4 value);
	GLint GetAttributeLocation(std::string Name);
private:
	std::unordered_map<std::string, GLint> m_Uniforms;
	void CompileErrors(unsigned int Shader, const char* Type);
};