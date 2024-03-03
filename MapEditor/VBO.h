#pragma once

#include <glad/glad.h>
#include <vector>
#include <glm/glm.hpp>

class VBO
{
public:
	// Reference ID of the Vertex Buffer Object
	GLuint ID;
	// Constructor that generates a Vertex Buffer Object and links it to vertices
	VBO(std::vector<float>& vertices);
	VBO(std::vector<glm::mat4> mat4s);
	VBO() {}

	// Binds the VBO
	void Bind();
	// Unbinds the VBO
	void Unbind();
	// Deletes the VBO
	void Delete();
};