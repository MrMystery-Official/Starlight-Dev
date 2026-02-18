#pragma once

#include <glad/glad.h>
#include <unordered_map>
#include <string>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

namespace application::gl
{
	class Shader
	{
	public:
		GLuint mID = 0xFFFFFFFF;

		Shader() = default;
		Shader(const std::string& VertexFile, const std::string& FragmentFile);
		Shader(const std::string& VertexFile, const std::string& FragmentFile, const std::string& TessellationControlFile, const std::string& TessellationEvalFile);

		void Bind();
		void Delete();

		void Set(const std::string& Name, int Value);
		void Set(const std::string& Name, bool Value);
		void Set(const std::string& Name, float Value);
		void Set(const std::string& Name, glm::vec2 Value);
		void Set(const std::string& Name, glm::vec3 Value);
		void Set(const std::string& Name, glm::vec4 Value);
		void Set(const std::string& Name, glm::mat4 Value);

		GLint GetAttributeLocation(const std::string& Name);
	private:
		std::unordered_map<std::string, GLint> mUniforms;

		void CompileErrors(unsigned int Shader, const char* Type);
	};
}