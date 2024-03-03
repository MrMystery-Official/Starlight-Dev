#include "Camera.h"

#include "imgui.h"
#include<glm/gtc/matrix_transform.hpp>
#include<glm/gtc/type_ptr.hpp>
#include<glm/gtx/rotate_vector.hpp>
#include<glm/gtx/vector_angle.hpp>
#include <iostream>

Camera::Camera(int Width, int Height, glm::vec3 Position, GLFWwindow* Window)
{
	Camera::Width = Width;
	Camera::Height = Height;
	Position = Position;
	pWindow = Window;
}

void Camera::UpdateMatrix(float FOVdeg, float nearPlane, float farPlane)
{
	// Initializes matrices since otherwise they will be the null matrix
	View = glm::mat4(1.0f);
	Projection = glm::mat4(1.0f);

	// Makes camera look in the right direction from the right position
	View = glm::lookAt(Position, Position + Orientation, Up);
	// Adds perspective to the scene
	Projection = glm::perspective(glm::radians(FOVdeg), (float)Width / Height, nearPlane, farPlane);
	CameraMatrix = Projection * View;
}

void Camera::Matrix(Shader* shader, const char* uniform)
{
	glUniformMatrix4fv(glGetUniformLocation(shader->ID, uniform), 1, GL_FALSE, glm::value_ptr(CameraMatrix));
}

glm::mat4& Camera::GetViewMatrix() {
	return this->View;
}

glm::mat4& Camera::GetProjectionMatrix() {
	return this->Projection;
}

void Camera::Inputs(float FramesPerSecond, ImVec2 WindowPos)
{
	if (WindowHovered) {
		if (glfwGetMouseButton(pWindow, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS && glfwGetMouseButton(pWindow, GLFW_MOUSE_BUTTON_LEFT) != GLFW_PRESS)
		{
			ImGui::SetWindowFocus(0);

			float NewSpeed = Speed * (60 / FramesPerSecond);

			// Handles key inputs
			if (glfwGetKey(pWindow, GLFW_KEY_W) == GLFW_PRESS)
			{
				Position += NewSpeed * Orientation;
			}
			if (glfwGetKey(pWindow, GLFW_KEY_A) == GLFW_PRESS)
			{
				Position += NewSpeed * -glm::normalize(glm::cross(Orientation, Up));
			}
			if (glfwGetKey(pWindow, GLFW_KEY_S) == GLFW_PRESS)
			{
				Position += NewSpeed * -Orientation;
			}
			if (glfwGetKey(pWindow, GLFW_KEY_D) == GLFW_PRESS)
			{
				Position += NewSpeed * glm::normalize(glm::cross(Orientation, Up));
			}
			if (glfwGetKey(pWindow, GLFW_KEY_SPACE) == GLFW_PRESS || glfwGetKey(pWindow, GLFW_KEY_E) == GLFW_PRESS)
			{
				Position += NewSpeed * Up;
			}
			if (glfwGetKey(pWindow, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || glfwGetKey(pWindow, GLFW_KEY_Q) == GLFW_PRESS)
			{
				Position += NewSpeed * -Up;
			}
			if (glfwGetKey(pWindow, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
			{
				Speed = 10.2f; //1.2f
			}
			else if (glfwGetKey(pWindow, GLFW_KEY_LEFT_SHIFT) == GLFW_RELEASE)
			{
				Speed = 0.5f; //.1f
			}

			// Handles mouse inputs
				// Hides mouse cursor
			glfwSetInputMode(pWindow, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

			// Prevents camera from jumping on the first click
			if (FirstClick)
			{
				double mouseX;
				double mouseY;
				// Fetches the coordinates of the cursor
				glfwGetCursorPos(pWindow, &mouseX, &mouseY);

				MousePosStart = glm::vec2(mouseX, mouseY);

				glfwSetCursorPos(pWindow, WindowPos.x + (Width / 2), WindowPos.y + (Height / 2));
				FirstClick = false;
			}

			// Stores the coordinates of the cursor
			double mouseX;
			double mouseY;
			// Fetches the coordinates of the cursor
			glfwGetCursorPos(pWindow, &mouseX, &mouseY);

			// Normalizes and shifts the coordinates of the cursor such that they begin in the middle of the screen
			// and then "transforms" them into degrees 
			float rotX = Sensitivity * (float)(mouseY - (WindowPos.y + Height / 2)) / Height;
			float rotY = Sensitivity * (float)(mouseX - (WindowPos.x + Width / 2)) / Width;

			// Calculates upcoming vertical change in the Orientation
			glm::vec3 newOrientation = glm::rotate(Orientation, glm::radians(-rotX), glm::normalize(glm::cross(Orientation, Up)));

			// Decides whether or not the next vertical Orientation is legal or not
			if (abs(glm::angle(newOrientation, Up) - glm::radians(90.0f)) <= glm::radians(85.0f))
			{
				Orientation = newOrientation;
			}

			// Rotates the Orientation left and right
			Orientation = glm::rotate(Orientation, glm::radians(-rotY), Up);

			// Sets mouse cursor to the middle of the screen so that it doesn't end up roaming around
			glfwSetCursorPos(pWindow, WindowPos.x + (Width / 2), WindowPos.y + (Height / 2));
		}
		else if (glfwGetMouseButton(pWindow, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_RELEASE)
		{
			if (!FirstClick) {
				glfwSetCursorPos(pWindow, MousePosStart.x, MousePosStart.y);
				// Unhides cursor since camera is not looking around anymore
				glfwSetInputMode(pWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
				// Makes sure the next time the camera looks around it doesn't jump
				FirstClick = true;
			}
		}
	}
}

bool Camera::IsInCameraMovement()
{
	if (WindowHovered)
	{
		return (glfwGetMouseButton(pWindow, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS && glfwGetMouseButton(pWindow, GLFW_MOUSE_BUTTON_LEFT) != GLFW_PRESS);
	}
	return false;
}