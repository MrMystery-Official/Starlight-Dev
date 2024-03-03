#include "Shader.h"

#include <string>
#include <fstream>
#include <sstream>
#include <cerrno>
#include <glm/gtc/type_ptr.hpp>
#include "Logger.h"

std::string GetFileContents(const char* FileName)
{
	std::ifstream File(FileName, std::ios::binary);
	if (File)
	{
		std::string Contents;
		File.seekg(0, std::ios::end);
		Contents.resize(File.tellg());
		File.seekg(0, std::ios::beg);
		File.read(&Contents[0], Contents.size());
		File.close();
		return Contents;
	}
	Logger::Error("ShaderLoader", "Could not load shader code");
	throw(errno);
}

Shader::Shader(const char* vertexFile, const char* fragmentFile)
{
	std::string VertexCode = GetFileContents(vertexFile);
	std::string FragmentCode = GetFileContents(fragmentFile);

	const char* VertexSource = VertexCode.c_str();
	const char* FragmentSource = FragmentCode.c_str();

	GLuint VertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(VertexShader, 1, &VertexSource, NULL);
	glCompileShader(VertexShader);
	CompileErrors(VertexShader, "VERTEX");

	GLuint FragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(FragmentShader, 1, &FragmentSource, NULL);
	glCompileShader(FragmentShader);
	CompileErrors(FragmentShader, "FRAGMENT");

	ID = glCreateProgram();
	glAttachShader(ID, VertexShader);
	glAttachShader(ID, FragmentShader);
	glLinkProgram(ID);
	CompileErrors(ID, "PROGRAM");

	// cache the uniforms location
	GLint NumUniforms;
	glGetProgramiv(ID, GL_ACTIVE_UNIFORMS, &NumUniforms);
	for (GLint i = 0; i < NumUniforms; i++)
	{
		const GLsizei BufSize = 32; // maximum name length
		GLchar Name[BufSize];       // variable name in GLSL
		GLsizei Length;             // name length
		GLint Size;                 // size of the variable
		GLenum Type;                // type of the variable (float, vec3 or mat4, etc)

		// get the name of this uniform
		glGetActiveUniform(ID, (GLuint)i, BufSize, &Length, &Size, &Type, Name);

		// cache for later use
		this->m_Uniforms.insert({ std::string(Name), i });
	}

	glDeleteShader(VertexShader);
	glDeleteShader(FragmentShader);
}

void Shader::Activate()
{
	glUseProgram(ID);
}

void Shader::Delete()
{
	glDeleteProgram(ID);
}

void Shader::CompileErrors(unsigned int Shader, const char* Type)
{
	GLint HasCompiled;
	char InfoLog[1024];
	if (Type != "PROGRAM")
	{
		glGetShaderiv(Shader, GL_COMPILE_STATUS, &HasCompiled);
		if (HasCompiled == GL_FALSE)
		{
			glGetShaderInfoLog(Shader, 1024, NULL, InfoLog);
			Logger::Error("ShaderLoader", "Error while compiling shader for: " + std::string(Type) + ", InfoLog: " + std::string(InfoLog));
		}
	}
	else
	{
		glGetProgramiv(Shader, GL_LINK_STATUS, &HasCompiled);
		if (HasCompiled == GL_FALSE)
		{
			glGetProgramInfoLog(Shader, 1024, NULL, InfoLog);
			Logger::Error("ShaderLoader", "Error while compiling shader for: " + std::string(Type) + ", InfoLog: " + std::string(InfoLog));
		}
	}
}


void Shader::Set(std::string name, int value)
{
	Activate();
	glUniform1i(m_Uniforms[name], value);
}

void Shader::Set(std::string name, bool value)
{
	Set(name, value ? 1 : 0);
}

void Shader::Set(std::string name, float value)
{
	Activate();
	glUniform1f(m_Uniforms[name], value);
}

void Shader::Set(std::string name, glm::vec2 value)
{
	Activate();
	glUniform2fv(m_Uniforms[name], 1, glm::value_ptr(value));
}

void Shader::Set(std::string name, glm::vec3 value)
{
	Activate();
	glUniform3fv(m_Uniforms[name], 1, glm::value_ptr(value));
}

void Shader::Set(std::string name, glm::vec4 value)
{
	Activate();
	glUniform4fv(m_Uniforms[name], 1, glm::value_ptr(value));
}

void Shader::Set(std::string name, glm::mat4 value)
{
	Activate();
	glUniformMatrix4fv(m_Uniforms[name], 1, GL_FALSE, glm::value_ptr(value));
}

GLint Shader::GetAttributeLocation(std::string Name)
{
	return glGetAttribLocation(this->ID, Name.c_str());
}