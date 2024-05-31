#pragma once

#include "Camera.h"
#include "EBO.h"
#include "Texture.h"
#include "VAO.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>

class Mesh {
public:
    std::vector<float> vertices;
    std::vector<GLuint> indices;
    std::vector<Texture*> textures;
    // Store VAO in public so it can be used in the Draw function
    VAO Vao;
    VBO instanceVBO;
    VBO Vbo;
    EBO Ebo;

    // Holds number of instances (if 1 the mesh will be rendered normally)
    unsigned int instancing;

    Mesh() { }

    // Initializes the mesh
    Mesh(
        std::vector<float>& vertices,
        std::vector<GLuint>& indices,
        std::vector<Texture*> textures,
        unsigned int instancing = 1,
        std::vector<glm::mat4> instanceMatrix = {});

    void UpdateInstances(unsigned int Instances);
    void UpdateInstanceMatrix(std::vector<glm::mat4>& Matrix);

    // Draws the mesh
    void Draw(Shader* GameShader = nullptr, int AlbedoTexCount = -1);
    void DrawPicking(Shader* Shader, Camera* Camera, glm::mat4 Matrix);

    void Delete();
};
