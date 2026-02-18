#include "Camera.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>
#include "imgui.h"
#include "imgui_internal.h"

namespace application::gl
{
	Camera::Camera(int Width, int Height, glm::vec3 Position, GLFWwindow* Window)
	{
		mWidth = Width;
		mHeight = Height;
		mPosition = Position;
		mWindow = Window;
	}

	void Camera::UpdateMatrix(float FOVdeg, float NearPlane, float FarPlane)
	{
		mView = glm::mat4(1.0f);
		mProjection = glm::mat4(1.0f);

		if (mMode == CameraMode::THIRD_PERSON)
		{
			UpdateThirdPersonCamera();
			mView = glm::lookAt(mPosition, mTarget, mUp);
		}
		else
		{
			mView = glm::lookAt(mPosition, mPosition + mOrientation, mUp);
		}

		mProjection = glm::perspective(glm::radians(FOVdeg), (float)mWidth / (float)mHeight, NearPlane, FarPlane);
		mCameraMatrix = mProjection * mView;
	}

	void Camera::Matrix(application::gl::Shader* Shader, const char* Uniform)
	{
		glUniformMatrix4fv(glGetUniformLocation(Shader->mID, Uniform), 1, GL_FALSE, glm::value_ptr(mCameraMatrix));
	}

	glm::mat4& Camera::GetViewMatrix() {
		return mView;
	}

	glm::mat4& Camera::GetProjectionMatrix() {
		return mProjection;
	}

	void Camera::MouseWheelCallback(GLFWwindow* Window, double xOffset, double yOffset)
	{
		if (!mWindowHovered)
			return;

		float NewSpeed = (glfwGetKey(mWindow, GLFW_KEY_LEFT_SHIFT) > 0 ? yOffset * mBoostMultiplier : yOffset);
		mPosition += (float)NewSpeed * mOrientation;
	}

	void Camera::UpdateThirdPersonCamera()
	{
		float yawRad = glm::radians(mYaw);
		float pitchRad = glm::radians(mPitch);

		glm::vec3 direction;
		direction.x = cos(pitchRad) * cos(yawRad);
		direction.y = sin(pitchRad);
		direction.z = cos(pitchRad) * sin(yawRad);

		mOrientation = glm::normalize(direction);

		glm::vec3 desiredPos = mTarget - mOrientation * mDistance;
		float smoothing = 1.0f - exp(-ImGui::GetIO().DeltaTime * 10.0f);
		mPosition = glm::mix(mPosition, desiredPos, smoothing);
	}

	void Camera::SetTarget(const glm::vec3& target)
	{
		mTarget = target;
	}

	void Camera::SetMode(CameraMode mode)
	{
		mMode = mode;
		mFirstClick = true;
		glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
	}

	void Camera::Inputs(float FramesPerSecond, glm::vec2 WindowPos)
	{
		if (glfwGetMouseButton(mWindow, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_RELEASE && !mFirstClick && mMode != CameraMode::THIRD_PERSON)
		{
			glfwSetCursorPos(mWindow, mMousePosStart.x, mMousePosStart.y);
			glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
			mFirstClick = true;
		}

		if (mMode == CameraMode::THIRD_PERSON)
		{
			glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
			ImGui::SetMouseCursor(ImGuiMouseCursor_None);

			double mouseX, mouseY;
			glfwGetCursorPos(mWindow, &mouseX, &mouseY);


			if (mFirstClick)
			{
				mMousePosStart = glm::vec2(mouseX, mouseY);
				mFirstClick = false;
			}

			float dx = float(mouseX - mMousePosStart.x);
			float dy = float(mouseY - mMousePosStart.y);

			mMousePosStart.x = mouseX;
			mMousePosStart.y = mouseY;

			mYaw += dx * (mSensitivity / 100.0f);
			mPitch -= dy * (mSensitivity / 100.0f);

			mPitch = glm::clamp(mPitch, mMinPitch, mMaxPitch);
		}
		else if (mMode == CameraMode::FREE_FLY && mWindowHovered)
		{
			if (glfwGetMouseButton(mWindow, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS && glfwGetMouseButton(mWindow, GLFW_MOUSE_BUTTON_LEFT) != GLFW_PRESS)
			{
				ImGui::SetWindowFocus(0);

				float NewSpeed = (glfwGetKey(mWindow, GLFW_KEY_LEFT_SHIFT) > 0 ? mSpeed * mBoostMultiplier : mSpeed) * (60 / FramesPerSecond);

				// Handles key inputs
				if (glfwGetKey(mWindow, GLFW_KEY_W) == GLFW_PRESS)
				{
					mPosition += NewSpeed * mOrientation;
				}
				if (glfwGetKey(mWindow, GLFW_KEY_A) == GLFW_PRESS)
				{
					mPosition += NewSpeed * -glm::normalize(glm::cross(mOrientation, mUp));
				}
				if (glfwGetKey(mWindow, GLFW_KEY_S) == GLFW_PRESS)
				{
					mPosition += NewSpeed * -mOrientation;
				}
				if (glfwGetKey(mWindow, GLFW_KEY_D) == GLFW_PRESS)
				{
					mPosition += NewSpeed * glm::normalize(glm::cross(mOrientation, mUp));
				}
				if (glfwGetKey(mWindow, GLFW_KEY_SPACE) == GLFW_PRESS || glfwGetKey(mWindow, GLFW_KEY_E) == GLFW_PRESS)
				{
					mPosition += NewSpeed * mUp;
				}
				if (glfwGetKey(mWindow, GLFW_KEY_Q) == GLFW_PRESS)
				{
					mPosition += NewSpeed * -mUp;
				}

				// Handles mouse inputs
					// Hides mouse cursor
				glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
				ImGui::SetMouseCursor(ImGuiMouseCursor_None);

				// Prevents camera from jumping on the first click
				if (mFirstClick)
				{
					double mouseX;
					double mouseY;
					// Fetches the coordinates of the cursor
					glfwGetCursorPos(mWindow, &mouseX, &mouseY);

					mMousePosStart = glm::vec2(mouseX, mouseY);

					glfwSetCursorPos(mWindow, WindowPos.x + (mWidth / 2), WindowPos.y + (mHeight / 2));
					mFirstClick = false;
				}

				// Stores the coordinates of the cursor
				double mouseX;
				double mouseY;
				// Fetches the coordinates of the cursor
				glfwGetCursorPos(mWindow, &mouseX, &mouseY);

				// Normalizes and shifts the coordinates of the cursor such that they begin in the middle of the screen
				// and then "transforms" them into degrees 
				float rotX = mSensitivity * (float)(mouseY - (WindowPos.y + mHeight / 2)) / mHeight;
				float rotY = mSensitivity * (float)(mouseX - (WindowPos.x + mWidth / 2)) / mWidth;

				// Calculates upcoming vertical change in the Orientation
				glm::vec3 newOrientation = glm::rotate(mOrientation, glm::radians(-rotX), glm::normalize(glm::cross(mOrientation, mUp)));

				// Decides whether or not the next vertical Orientation is legal or not
				if (abs(glm::angle(newOrientation, mUp) - glm::radians(90.0f)) <= glm::radians(85.0f))
				{
					mOrientation = newOrientation;
				}

				// Rotates the Orientation left and right
				mOrientation = glm::rotate(mOrientation, glm::radians(-rotY), mUp);

				// Sets mouse cursor to the middle of the screen so that it doesn't end up roaming around
				glfwSetCursorPos(mWindow, WindowPos.x + (mWidth / 2), WindowPos.y + (mHeight / 2));
			}
		}
	}

	bool Camera::IsInCameraMovement()
	{
		if (mWindowHovered)
		{
			return (glfwGetMouseButton(mWindow, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS && glfwGetMouseButton(mWindow, GLFW_MOUSE_BUTTON_LEFT) != GLFW_PRESS);
		}
		return false;
	}
}