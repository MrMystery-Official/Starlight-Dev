#pragma once

#include<glad/glad.h>
#include<GLFW/glfw3.h>
#include<glm/glm.hpp>
#include "Shader.h"
#include "imgui.h"

class Camera {
public:
	glm::vec3 Position;
	glm::vec3 Orientation = glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 Up = glm::vec3(0.0f, 1.0f, 0.0f);

	glm::mat4 View;
	glm::mat4 Projection;
	glm::mat4 CameraMatrix = glm::mat4(1.0f);

	glm::vec2 MousePosStart;

	int Width;
	int Height;

	bool FirstClick = true;

	bool WindowHovered = false;

	// Adjust the speed of the camera and it's sensitivity when looking around
	float Speed = 0.5f;
	float BoostMultiplier = 10.0f;
	float Sensitivity = 100.0f;

	GLFWwindow* pWindow = nullptr;

	// Camera constructor to set up initial values
	Camera(int Width, int Height, glm::vec3 Position, GLFWwindow* Window);
	Camera() {}

	// Updates and exports the camera matrix to the Vertex Shader
	void UpdateMatrix(float FOVdeg, float nearPlane, float farPlane);
	void Matrix(Shader* shader, const char* uniform);
	// Handles camera inputs
	void Inputs(float FramesPerSecond, ImVec2 WindowPos);
	bool IsInCameraMovement();
	void MouseWheelCallback(GLFWwindow* window, double xOffset, double yOffset);

	glm::mat4& GetViewMatrix();
	glm::mat4& GetProjectionMatrix();
};