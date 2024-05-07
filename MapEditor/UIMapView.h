#pragma once

#include <glad/glad.h>
#include "Camera.h"
#include "ActorMgr.h"
#include "ShaderMgr.h"
#include "FramebufferMgr.h"
#include "imgui.h"
#include "ImGuizmo.h"
#include "Bfres.h"
#include <vector>

namespace UIMapView
{
	struct RenderingSettingsStruct
	{
		bool RenderVisibleActors = true;
		bool RenderInvisibleActors = true;
		bool RenderAreas = true;
		bool RenderFarActors = true;
		bool RenderNPCs = true;
		bool AllowSelectingActor = true;
	};

	extern bool Open;
	extern bool Focused;
	extern GLFWwindow* Window;
	extern Camera CameraView;
	extern Framebuffer* MapViewFramebuffer;
	extern Shader* DefaultShader;
	extern Shader* SelectedShader;
	extern Shader* PickingShader;
	extern Shader* InstancedShader;
	extern Shader* GameShader;
	extern ImVec4 ClearColor;
	extern float ObjectMatrix[16];
	extern ImGuizmo::MODE ImGuizmoMode;
	extern ImGuizmo::OPERATION ImGuizmoOperation;
	extern RenderingSettingsStruct RenderSettings;

	void GLFWKeyCallback(GLFWwindow* Window, int Key, int Scancode, int Action, int Mods);
	void Initialize(GLFWwindow* pWindow);
	void DrawOverlay();
	void DrawInstancedActor(BfresFile* Model, std::vector<Actor*>& ActorPtrs);
	void DrawActor(Actor& Actor, Shader* Shader);
	void SelectActorByClicking(ImVec2 SceneWindowSize, ImVec2 MousePos);
	void DrawMapViewWindow();
	bool IsInCameraMovement();
};