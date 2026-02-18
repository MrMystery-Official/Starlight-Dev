#pragma once

#include <string>
#include <glm/glm.hpp>
#include <gl/BufferObject.h>
#include <gl/VertexArrayObject.h>
#include <gl/Shader.h>

namespace application::gl
{
	struct SimpleMesh
	{
		application::gl::VertexArrayObject mVertexArrayObject;
		uint32_t mIndexCount = 0;

		SimpleMesh() = default;

		SimpleMesh(std::vector<float>& Vertices, std::vector<GLuint>& Indices);
		SimpleMesh(std::vector<glm::vec3>& Vertices, std::vector<GLuint>& Indices);

		void Initialize(std::vector<float>& Vertices, std::vector<GLuint>& Indices);

		void Draw();
		void Delete();
	};
}