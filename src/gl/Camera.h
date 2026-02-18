#pragma once

#define GLM_ENABLE_EXPERIMENTAL

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

#include <gl/Shader.h>

namespace application::gl
{
	struct Camera 
	{
		enum class CameraMode : int
		{
			FREE_FLY = 0,
			THIRD_PERSON = 1
		};

		CameraMode mMode = CameraMode::FREE_FLY;

		// Third-person specific
		glm::vec3 mTarget = glm::vec3(0.0f);
		float mDistance = 6.5f;
		float mYaw = -90.0f;
		float mPitch = 0.0f;
		float mMinPitch = -60.0f;
		float mMaxPitch = 60.0f;

		glm::vec3 mPosition;
		glm::vec3 mOrientation = glm::vec3(0.0f, 0.0f, -1.0f);
		glm::vec3 mUp = glm::vec3(0.0f, 1.0f, 0.0f);
		glm::mat4 mView;
		glm::mat4 mProjection;
		glm::mat4 mCameraMatrix = glm::mat4(1.0f);
		glm::vec2 mMousePosStart;
		int mWidth = 0;
		int mHeight = 0;
		bool mFirstClick = true;
		bool mWindowHovered = false;

		// Adjust the speed of the camera and it's sensitivity when looking around
		float mSpeed = 0.5f;
		float mBoostMultiplier = 10.0f;
		float mSensitivity = 100.0f;

		GLFWwindow* mWindow = nullptr;

		Camera(int Width, int Height, glm::vec3 Position, GLFWwindow* Window);
		Camera() = default;

		void UpdateMatrix(float FOVdeg, float NearPlane, float FarPlane);
		void Matrix(application::gl::Shader* Shader, const char* Uniform);
		void UpdateThirdPersonCamera();
		void SetTarget(const glm::vec3& target);
		void SetMode(CameraMode mode);
		void Inputs(float FramesPerSecond, glm::vec2 WindowPos);
		bool IsInCameraMovement();
		void MouseWheelCallback(GLFWwindow* Window, double xOffset, double yOffset);

		glm::mat4& GetViewMatrix();
		glm::mat4& GetProjectionMatrix();
	};
}