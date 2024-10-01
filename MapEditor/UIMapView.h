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
		bool RenderNavMesh = false;
	};

	struct ActorPainterActorEntry
	{
		std::string mGyml = "";
		float mYModelOffset = 0.0f;
		float mProbability = 1.0f;
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
	extern Shader* InstancedDiscardShader;
	extern Shader* NavMeshShader;
	extern Shader* GameShader;
	extern const ImVec4 ClearColor;
	extern float ObjectMatrix[16];
	extern ImGuizmo::MODE ImGuizmoMode;
	extern ImGuizmo::OPERATION ImGuizmoOperation;
	extern RenderingSettingsStruct RenderSettings;
	extern glm::vec3 mLightColor;
	extern bool mControlKeyDown;

	extern bool mEnableActorPainter;
	extern Actor* mActorPainterTerrain;
	extern uint64_t mActorPainterTerrainHash;
	extern float mActorPainterRadius;
	extern int mActorPainterIterations;
	extern glm::vec3 mActorPainterScaleMin;
	extern glm::vec3 mActorPainterScaleMax;
	extern glm::vec3 mActorPainterRotMin;
	extern glm::vec3 mActorPainterRotMax;
	extern std::vector<ActorPainterActorEntry> mActorPainterActorEntries;

	void GLFWKeyCallback(GLFWwindow* Window, int Key, int Scancode, int Action, int Mods);
	void ProcessActorPainter(double XPos, double YPos, int ScreenWidth, int ScreenHeight);
	void Initialize(GLFWwindow* pWindow);
	void DrawOverlay();
	void DrawCameraMenu(float w, ImVec2 Pos);
	void DrawActorPainterMenu(float w, ImVec2 Pos);
	void DrawInstancedActor(GLBfres* Model, std::vector<Actor*>& ActorPtrs, Shader* Shader);
	void DrawActor(Actor& Actor, Shader* Shader);
	void SelectActorByClicking(ImVec2 SceneWindowSize, ImVec2 MousePos);
	void DrawMapViewWindow();
	bool IsInCameraMovement();
	void GLFWScrollCallback(GLFWwindow* window, double xOffset, double yOffset);
};