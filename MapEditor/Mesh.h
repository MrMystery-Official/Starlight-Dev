#pragma once

#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "VAO.h"
#include "EBO.h"
#include "Camera.h"
#include "Texture.h"

class Mesh
{
public:
	std::vector<float> vertices;
	std::vector<GLuint> indices;
	unsigned int mTexture;
	// Store VAO in public so it can be used in the Draw function
	VAO Vao;
	VBO instanceVBO;
	VBO Vbo;
	EBO Ebo;

	// Holds number of instances (if 1 the mesh will be rendered normally)
	unsigned int instancing;

	Mesh() {}

	// Initializes the mesh
	Mesh
	(
		std::vector<float>& vertices,
		std::vector<GLuint>& indices,
		unsigned int texture,
		unsigned int instancing = 1,
		std::vector<glm::mat4> instanceMatrix = {}
	);

	Mesh(std::vector<float>& Vertices, std::vector<GLuint>& Indices);

	void UpdateInstances(unsigned int Instances);
	void UpdateInstanceMatrix(std::vector<glm::mat4>& Matrix);

	// Draws the mesh
	void Draw();
	void DrawPicking(Shader* Shader, Camera* Camera, glm::mat4 Matrix);
	void DrawRaw();

	void Delete();
};