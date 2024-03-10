#include "Mesh.h"

#include <glm/gtc/type_ptr.hpp>

Mesh::Mesh
(
	std::vector<float>& vertices,
	std::vector<GLuint>& indices,
	std::vector<Texture*> textures,
	unsigned int instancing,
	std::vector<glm::mat4> instanceMatrix
)
{
	Mesh::vertices = vertices;
	Mesh::indices = indices;
	Mesh::textures = textures;
	Mesh::instancing = instancing;

	Vao = VAO(true);
	Vao.Bind();
	// Generates Vertex Buffer Object and links it to vertices
	instanceVBO = VBO(instanceMatrix);
	Vbo = VBO(vertices);
	// Generates Element Buffer Object and links it to indices
	Ebo = EBO(indices);
	// Links VBO attributes such as coordinates and colors to VAO
	Vao.LinkAttrib(Vbo, 0, 3, GL_FLOAT, 5 * sizeof(float), (void*)0);
	Vao.LinkAttrib(Vbo, 1, 2, GL_FLOAT, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	if (instancing != 1)
	{
		instanceVBO.Bind();
		// Can't link to a mat4 so you need to link four vec4s
		Vao.LinkAttrib(instanceVBO, 2, 4, GL_FLOAT, sizeof(glm::mat4), (void*)0);
		Vao.LinkAttrib(instanceVBO, 3, 4, GL_FLOAT, sizeof(glm::mat4), (void*)(1 * sizeof(glm::vec4)));
		Vao.LinkAttrib(instanceVBO, 4, 4, GL_FLOAT, sizeof(glm::mat4), (void*)(2 * sizeof(glm::vec4)));
		Vao.LinkAttrib(instanceVBO, 5, 4, GL_FLOAT, sizeof(glm::mat4), (void*)(3 * sizeof(glm::vec4)));
		// Makes it so the transform is only switched when drawing the next instance
		glVertexAttribDivisor(2, 1);
		glVertexAttribDivisor(3, 1);
		glVertexAttribDivisor(4, 1);
		glVertexAttribDivisor(5, 1);
	}
	// Unbind all to prevent accidentally modifying them
	Vao.Unbind();
	Vbo.Unbind();
	instanceVBO.Unbind();
	Ebo.Unbind();
}

void Mesh::Draw(Shader* GameShader, int AlbedoTexCount)
{
	Vao.Bind();

	//texture->Bind();

	if (GameShader != nullptr)
	{
		for (unsigned int i = 0; i < textures.size(); i++)
		{
			textures[i]->ActivateTextureUnit();
			textures[i]->Bind();
			textures[i]->texUnit(*GameShader, textures[i]->Sampler.c_str(), i);
		}
	}
	else
	{
		for (unsigned int i = 0; i < textures.size(); i++)
		{
			textures[i]->Bind();
		}
	}

	// Check if instance drawing should be performed
	if (instancing == 1)
	{
		// Draw the actual mesh
		glDrawElements(GL_TRIANGLES, sizeof(int) * indices.size(), GL_UNSIGNED_INT, 0);
	}
	else
	{
		glDrawElementsInstanced(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0, instancing);
	}
}

void Mesh::DrawPicking(Shader* Shader, Camera* Camera, glm::mat4 Matrix)
{
	// Bind shader to be able to access uniforms
	Shader->Activate();
	Vao.Bind();

	Camera->Matrix(Shader, "camMatrix");

	glUniformMatrix4fv(glGetUniformLocation(Shader->ID, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(Matrix));

	// Draw the actual mesh
	glDrawElements(GL_TRIANGLES, sizeof(int) * indices.size(), GL_UNSIGNED_INT, 0);
}

void Mesh::UpdateInstances(unsigned int Instances)
{
	Mesh::instancing = Instances;

	Vao.Bind();
	instanceVBO.Bind();
	// Can't link to a mat4 so you need to link four vec4s
	Vao.LinkAttrib(instanceVBO, 2, 4, GL_FLOAT, sizeof(glm::mat4), (void*)0);
	Vao.LinkAttrib(instanceVBO, 3, 4, GL_FLOAT, sizeof(glm::mat4), (void*)(1 * sizeof(glm::vec4)));
	Vao.LinkAttrib(instanceVBO, 4, 4, GL_FLOAT, sizeof(glm::mat4), (void*)(2 * sizeof(glm::vec4)));
	Vao.LinkAttrib(instanceVBO, 5, 4, GL_FLOAT, sizeof(glm::mat4), (void*)(3 * sizeof(glm::vec4)));
	// Makes it so the transform is only switched when drawing the next instance
	glVertexAttribDivisor(2, 1);
	glVertexAttribDivisor(3, 1);
	glVertexAttribDivisor(4, 1);
	glVertexAttribDivisor(5, 1);
}

void Mesh::UpdateInstanceMatrix(std::vector<glm::mat4>& Matrix)
{
	instanceVBO.Bind();
	glBufferData(GL_ARRAY_BUFFER, Matrix.size() * sizeof(glm::mat4), Matrix.data(), GL_STATIC_DRAW);
}

void Mesh::Delete()
{
	Vao.Delete();
	instanceVBO.Delete();
	Vbo.Delete();
	Ebo.Delete();
}