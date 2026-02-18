#include "Shader.h"

#include <string>
#include <fstream>
#include <sstream>
#include <cerrno>
#include <glm/gtc/type_ptr.hpp>
#include <util/Logger.h>

namespace application::gl
{
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
		application::util::Logger::Error("ShaderLoader", "Could not load shader code");
		throw(errno);
	}

	Shader::Shader(const std::string& VertexFile, const std::string& FragmentFile)
	{
		std::string VertexCode = GetFileContents(VertexFile.c_str());
		std::string FragmentCode = GetFileContents(FragmentFile.c_str());

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

		mID = glCreateProgram();
		glAttachShader(mID, VertexShader);
		glAttachShader(mID, FragmentShader);
		glLinkProgram(mID);
		CompileErrors(mID, "PROGRAM");

		// cache the uniforms location
		GLint NumUniforms;
		glGetProgramiv(mID, GL_ACTIVE_UNIFORMS, &NumUniforms);
		for (GLint i = 0; i < NumUniforms; i++)
		{
			const GLsizei BufSize = 32; // maximum name length
			GLchar Name[BufSize];       // variable name in GLSL
			GLsizei Length;             // name length
			GLint Size;                 // size of the variable
			GLenum Type;                // type of the variable (float, vec3 or mat4, etc)

			// get the name of this uniform
			glGetActiveUniform(mID, (GLuint)i, BufSize, &Length, &Size, &Type, Name);

			// cache for later use
			mUniforms.insert({ std::string(Name), i });
		}

		glDeleteShader(VertexShader);
		glDeleteShader(FragmentShader);
	}

	Shader::Shader(const std::string& VertexFile, const std::string& FragmentFile, const std::string& TessellationControlFile, const std::string& TessellationEvalFile)
	{
		std::string VertexCode = GetFileContents(VertexFile.c_str());
		std::string FragmentCode = GetFileContents(FragmentFile.c_str());
		std::string ControlCode = GetFileContents(TessellationControlFile.c_str());
		std::string EvalCode = GetFileContents(TessellationEvalFile.c_str());

		const char* VertexSource = VertexCode.c_str();
		const char* FragmentSource = FragmentCode.c_str();
		const char* ControlSource = ControlCode.c_str();
		const char* EvalSource = EvalCode.c_str();

		GLuint VertexShader = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(VertexShader, 1, &VertexSource, NULL);
		glCompileShader(VertexShader);
		CompileErrors(VertexShader, "VERTEX");

		GLuint FragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(FragmentShader, 1, &FragmentSource, NULL);
		glCompileShader(FragmentShader);
		CompileErrors(FragmentShader, "FRAGMENT");

		GLuint ControlShader = glCreateShader(GL_TESS_CONTROL_SHADER);
		glShaderSource(ControlShader, 1, &ControlSource, NULL);
		glCompileShader(ControlShader);
		CompileErrors(ControlShader, "TESS_CONTROL");

		GLuint EvalShader = glCreateShader(GL_TESS_EVALUATION_SHADER);
		glShaderSource(EvalShader, 1, &EvalSource, NULL);
		glCompileShader(EvalShader);
		CompileErrors(EvalShader, "TESS_EVAL");

		mID = glCreateProgram();
		glAttachShader(mID, VertexShader);
		glAttachShader(mID, ControlShader);
		glAttachShader(mID, EvalShader);
		glAttachShader(mID, FragmentShader);
		glLinkProgram(mID);
		CompileErrors(mID, "PROGRAM");

		// cache the uniforms location
		GLint NumUniforms;
		glGetProgramiv(mID, GL_ACTIVE_UNIFORMS, &NumUniforms);
		for (GLint i = 0; i < NumUniforms; i++)
		{
			const GLsizei BufSize = 32; // maximum name length
			GLchar Name[BufSize];       // variable name in GLSL
			GLsizei Length;             // name length
			GLint Size;                 // size of the variable
			GLenum Type;                // type of the variable (float, vec3 or mat4, etc)

			// get the name of this uniform
			glGetActiveUniform(mID, (GLuint)i, BufSize, &Length, &Size, &Type, Name);

			// cache for later use
			mUniforms.insert({ std::string(Name), i });
		}

		glDeleteShader(VertexShader);
		glDeleteShader(FragmentShader);
		glDeleteShader(ControlShader);
		glDeleteShader(EvalShader);
	}

	void Shader::Bind()
	{
		glUseProgram(mID);
	}

	void Shader::Delete()
	{
		glDeleteProgram(mID);
	}

	void Shader::CompileErrors(unsigned int Shader, const char* Type)
	{
		GLint HasCompiled;
		char InfoLog[1024];
		if (strcmp(Type, "PROGRAM") != 0)
		{
			glGetShaderiv(Shader, GL_COMPILE_STATUS, &HasCompiled);
			if (HasCompiled == GL_FALSE)
			{
				glGetShaderInfoLog(Shader, 1024, NULL, InfoLog);
				application::util::Logger::Error("ShaderLoader", "Error while compiling shader for: %s, InfoLog: %s", Type, &InfoLog[0]);
			}
		}
		else
		{
			glGetProgramiv(Shader, GL_LINK_STATUS, &HasCompiled);
			if (HasCompiled == GL_FALSE)
			{
				glGetProgramInfoLog(Shader, 1024, NULL, InfoLog);
				application::util::Logger::Error("ShaderLoader", "Error while compiling shader for: %s, InfoLog: %s", Type, &InfoLog[0]);
			}
		}
	}

	void Shader::Set(const std::string& Name, int Value)
	{
		glUniform1i(mUniforms[Name], Value);
	}

	void Shader::Set(const std::string& Name, bool Value)
	{
		Set(Name, Value ? 1 : 0);
	}

	void Shader::Set(const std::string& Name, float Value)
	{
		glUniform1f(mUniforms[Name], Value);
	}

	void Shader::Set(const std::string& Name, glm::vec2 Value)
	{
		glUniform2fv(mUniforms[Name], 1, glm::value_ptr(Value));
	}

	void Shader::Set(const std::string& Name, glm::vec3 Value)
	{
		glUniform3fv(mUniforms[Name], 1, glm::value_ptr(Value));
	}

	void Shader::Set(const std::string& Name, glm::vec4 Value)
	{
		glUniform4fv(mUniforms[Name], 1, glm::value_ptr(Value));
	}

	void Shader::Set(const std::string& Name, glm::mat4 Value)
	{
		glUniformMatrix4fv(mUniforms[Name], 1, GL_FALSE, glm::value_ptr(Value));
	}

	GLint Shader::GetAttributeLocation(const std::string& Name)
	{
		return glGetAttribLocation(mID, Name.c_str());
	}
}