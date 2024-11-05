#include "UIMapView.h"

#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_stdlib.h"
#include "imgui_internal.h"
#include <glm/gtc/matrix_transform.hpp>
#include "UIOutliner.h"
#include "Frustum.h"
#include "TextureMgr.h"
#include "Logger.h"
#include "SceneMgr.h"
#include "GLBfres.h"
#include "ActionMgr.h"
#include "Editor.h"
#include "PreferencesConfig.h"
#include "StarImGui.h"
#include "HGHT.h"
#include <random>
#include <chrono>

bool UIMapView::Open = true;
bool UIMapView::Focused = false;

GLFWwindow* UIMapView::Window;

Camera UIMapView::CameraView = Camera(1280, 720, glm::vec3(0.0f, 0.0f, 2.0f), nullptr);

Framebuffer* UIMapView::MapViewFramebuffer = nullptr;

Shader* UIMapView::DefaultShader = nullptr;
Shader* UIMapView::SelectedShader = nullptr;
Shader* UIMapView::PickingShader = nullptr;
Shader* UIMapView::InstancedShader = nullptr;
Shader* UIMapView::InstancedDiscardShader = nullptr;
Shader* UIMapView::GameShader = nullptr;
Shader* UIMapView::NavMeshShader = nullptr;

const ImVec4 UIMapView::ClearColor = ImVec4(0.18f, 0.21f, 0.25f, 1.00f);

UIMapView::RenderingSettingsStruct UIMapView::RenderSettings;

float UIMapView::ObjectMatrix[16] =
{ 1.f, 0.f, 0.f, 0.f,
  0.f, 1.f, 0.f, 0.f,
  0.f, 0.f, 1.f, 0.f,
  0.f, 0.f, 0.f, 1.f };

ImGuizmo::MODE UIMapView::ImGuizmoMode = ImGuizmo::WORLD;
ImGuizmo::OPERATION UIMapView::ImGuizmoOperation = ImGuizmo::TRANSLATE;

glm::vec3 UIMapView::mLightColor = glm::vec3(1.0f, 1.0f, 1.0f);

bool IsUsingGizmo = false;
Vector3F PreviousVectorData;

bool UIMapView::mControlKeyDown = false;

bool UIMapView::mEnableActorPainter = false;
Actor* UIMapView::mActorPainterTerrain = nullptr;
uint64_t UIMapView::mActorPainterTerrainHash = 0;
float UIMapView::mActorPainterRadius = 1.0f;
int UIMapView::mActorPainterIterations = 5;
glm::vec3 UIMapView::mActorPainterScaleMin(0.95f, 0.95f, 0.95f);
glm::vec3 UIMapView::mActorPainterScaleMax(1.05f, 1.05f, 1.05f);
glm::vec3 UIMapView::mActorPainterRotMin(0.0f, 0.0f, 0.0f);
glm::vec3 UIMapView::mActorPainterRotMax(5.0f, 90.0f, 5.0f);
std::vector<UIMapView::ActorPainterActorEntry> UIMapView::mActorPainterActorEntries;

std::vector<Mesh> UIMapView::mDebugMeshes;

void UIMapView::GLFWKeyCallback(GLFWwindow* Window, int Key, int Scancode, int Action, int Mods)
{
	if (ImGui::GetIO().WantTextInput || CameraView.IsInCameraMovement() || Action != GLFW_PRESS)
		return;

	//Rotation hot key
	if (Key == GLFW_KEY_R)
	{
		ImGuizmoOperation = ImGuizmo::ROTATE;
		return;
	}

	if (Key == GLFW_KEY_B)
	{
		ImGuizmoOperation = ImGuizmo::SCALE;
		return;
	}

	if (Key == GLFW_KEY_G)
	{
		ImGuizmoOperation = ImGuizmo::TRANSLATE;
		return;
	}

	if (Key == GLFW_KEY_ESCAPE)
	{
		UIOutliner::SelectedActor = nullptr;
		return;
	}

	if (Key == GLFW_KEY_DELETE)
	{
		ActorMgr::DeleteActor(UIOutliner::SelectedActor->Hash, UIOutliner::SelectedActor->SRTHash);
		UIOutliner::SelectedActor = nullptr;
		return;
	}

	//Y is Z, and Z is Y
	if (Key == GLFW_KEY_Y && glfwGetKey(Window, GLFW_KEY_LEFT_CONTROL) > 0 && glfwGetKey(Window, GLFW_KEY_LEFT_SHIFT) == 0)
	{
		ActionMgr::Undo();
		return;
	}

	if (Key == GLFW_KEY_Y && glfwGetKey(Window, GLFW_KEY_LEFT_CONTROL) > 0 && glfwGetKey(Window, GLFW_KEY_LEFT_SHIFT) > 0)
	{
		ActionMgr::Redo();
	}
}

glm::vec3 UIMapView::GetRayFromMouse(double mouseX, double mouseY, const glm::mat4& projectionMatrix, const glm::mat4& viewMatrix, int screenWidth, int screenHeight) {
	// Convert mouse coordinates to normalized device coordinates
	float x = (2.0f * mouseX) / screenWidth - 1.0f;
	float y = 1.0f - (2.0f * mouseY) / screenHeight;
	glm::vec4 rayClip(x, y, -1.0f, 1.0f);

	// Convert to eye space
	glm::vec4 rayEye = glm::inverse(projectionMatrix) * rayClip;
	rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);

	// Convert to world space
	glm::vec3 rayWorld = glm::vec3(glm::inverse(viewMatrix) * rayEye);
	return glm::normalize(rayWorld);
}

bool UIMapView::RayTriangleIntersect(const glm::vec3& rayOrigin, const glm::vec3& rayDir, glm::vec3 VertexA, glm::vec3 VertexB, glm::vec3 VertexC, float& t) {
	const float EPSILON = 0.0000001f;
	glm::vec3 edge1 = VertexB - VertexA;
	glm::vec3 edge2 = VertexC - VertexA;
	glm::vec3 h = glm::cross(rayDir, edge2);
	float a = glm::dot(edge1, h);

	if (a > -EPSILON && a < EPSILON) return false;

	float f = 1.0f / a;
	glm::vec3 s = rayOrigin - VertexA;
	float u = f * glm::dot(s, h);

	if (u < 0.0f || u > 1.0f) return false;

	glm::vec3 q = glm::cross(s, edge1);
	float v = f * glm::dot(rayDir, q);

	if (v < 0.0f || u + v > 1.0f) return false;

	t = f * glm::dot(edge2, q);
	return t > EPSILON;
}

glm::vec3 UIMapView::FindTerrainIntersection(const glm::vec3& rayOrigin, const glm::vec3& rayDir, Actor* TerrainActor) {
	float closestT = std::numeric_limits<float>::max();
	glm::vec3 intersection;
	bool found = false;

	for (auto& [Key, Val] : TerrainActor->Model->mBfres->Models.GetByIndex(0).Value.Shapes.Nodes)
	{
		std::vector<uint32_t> Indices = Val.Value.Meshes[0].GetIndices();
		for (size_t i = 0; i < Indices.size() / 3; i++)
		{
			float t = 0.0f;
			glm::vec4 VertexA = Val.Value.Vertices[Indices[i * 3]] + glm::vec4(TerrainActor->Translate.GetX(), TerrainActor->Translate.GetY(), TerrainActor->Translate.GetZ(), 0.0f);
			glm::vec4 VertexB = Val.Value.Vertices[Indices[i * 3 + 1]] + glm::vec4(TerrainActor->Translate.GetX(), TerrainActor->Translate.GetY(), TerrainActor->Translate.GetZ(), 0.0f);
			glm::vec4 VertexC = Val.Value.Vertices[Indices[i * 3 + 2]] + glm::vec4(TerrainActor->Translate.GetX(), TerrainActor->Translate.GetY(), TerrainActor->Translate.GetZ(), 0.0f);
			if (RayTriangleIntersect(rayOrigin, rayDir, glm::vec3(VertexA.x, VertexA.y, VertexA.z), glm::vec3(VertexB.x, VertexB.y, VertexB.z), glm::vec3(VertexC.x, VertexC.y, VertexC.z), t))
			{
				if (t < closestT) {
					closestT = t;
					intersection = rayOrigin + rayDir * t;
					found = true;
				}
			}
		}
	}

	return found ? intersection : glm::vec3(0.0f);
}

std::pair<glm::vec3, glm::vec3> UIMapView::FindTerrainIntersectionAndNormalVec(const glm::vec3& rayOrigin, const glm::vec3& rayDir, Actor* TerrainActor) {
	float closestT = std::numeric_limits<float>::max();
	glm::vec3 intersection;
	glm::vec3 normal;
	bool found = false;

	for (auto& [Key, Val] : TerrainActor->Model->mBfres->Models.GetByIndex(0).Value.Shapes.Nodes)
	{
		std::vector<uint32_t> Indices = Val.Value.Meshes[0].GetIndices();
		for (size_t i = 0; i < Indices.size() / 3; i++)
		{
			float t = 0.0f;
			glm::vec4 VertexA = Val.Value.Vertices[Indices[i * 3]] + glm::vec4(TerrainActor->Translate.GetX(), TerrainActor->Translate.GetY(), TerrainActor->Translate.GetZ(), 0.0f);
			glm::vec4 VertexB = Val.Value.Vertices[Indices[i * 3 + 1]] + glm::vec4(TerrainActor->Translate.GetX(), TerrainActor->Translate.GetY(), TerrainActor->Translate.GetZ(), 0.0f);
			glm::vec4 VertexC = Val.Value.Vertices[Indices[i * 3 + 2]] + glm::vec4(TerrainActor->Translate.GetX(), TerrainActor->Translate.GetY(), TerrainActor->Translate.GetZ(), 0.0f);
			
			glm::vec3 NewVertexA(VertexA.x, VertexA.y, VertexA.z);
			glm::vec3 NewVertexB(VertexB.x, VertexB.y, VertexB.z);
			glm::vec3 NewVertexC(VertexC.x, VertexC.y, VertexC.z);
			
			if (RayTriangleIntersect(rayOrigin, rayDir, NewVertexA, NewVertexB, NewVertexC, t))
			{
				if (t < closestT) {
					closestT = t;
					intersection = rayOrigin + rayDir * t;
					normal = glm::cross(NewVertexB - NewVertexA, NewVertexC - NewVertexA);
					found = true;
				}
			}
		}
	}

	return found ? std::make_pair(intersection, glm::normalize(normal)) : std::make_pair(glm::vec3(0.0f), glm::vec3(0.0f));
}

float UIMapView::CalculateModelYOffset(Actor& Actor)
{
	float SmallestY = std::numeric_limits<float>::max();
	for (auto& [Key, Val] : Actor.Model->mBfres->Models.GetByIndex(0).Value.Shapes.Nodes)
	{
		std::vector<uint32_t> Indices = Val.Value.Meshes[0].GetIndices();
		for (size_t i = 0; i < Indices.size() / 3; i++)
		{
			if (Val.Value.Vertices[Indices[i * 3 + 1]].y < SmallestY) SmallestY = Val.Value.Vertices[Indices[i * 3 + 1]].y;
		}
	}

	return SmallestY;
}

glm::vec3 UIMapView::NormalToEuler(const glm::vec3& normal) {
	// Ensure the normal is normalized
	glm::vec3 n = glm::normalize(normal);

	// Define the "up" vector (this assumes Y is up in your coordinate system)
	glm::vec3 up(0.0f, 1.0f, 0.0f);

	// Compute the quaternion that rotates "up" to align with the normal
	glm::quat rotQuat = glm::rotation(up, n);

	// Convert quaternion to Euler angles
	glm::vec3 eulerAngles = glm::degrees(glm::eulerAngles(rotQuat));

	// Adjust the angles to be in the range [-180, 180]
	eulerAngles.x = fmod(eulerAngles.x + 180.0f, 360.0f) - 180.0f;
	eulerAngles.y = fmod(eulerAngles.y + 180.0f, 360.0f) - 180.0f;
	eulerAngles.z = fmod(eulerAngles.z + 180.0f, 360.0f) - 180.0f;

	return eulerAngles;
}

void UIMapView::ProcessActorPainter(double XPos, double YPos, int ScreenWidth, int ScreenHeight)
{
	if (mActorPainterTerrain == nullptr || CameraView.IsInCameraMovement() || ImGui::IsAnyItemHovered())
		return;

	glm::vec3 rayDir = GetRayFromMouse(XPos, YPos, CameraView.Projection, CameraView.View, ScreenWidth, ScreenHeight);
	//std::cout << rayDir.x << ", " << rayDir.y << ", " << rayDir.z << std::endl;
	glm::vec3 intersection = FindTerrainIntersection(CameraView.Position, rayDir, mActorPainterTerrain);

	if (intersection != glm::vec3(0.0f)) {
		// Intersection found, use the coordinates
		//printf("Intersection at: (%.2f, %.2f, %.2f)\n", intersection.x, intersection.y, intersection.z);

		InstancedShader->Activate();

		std::vector<glm::mat4> InstanceMatrix(1);

		glm::mat4& GLMModel = InstanceMatrix[0];
		GLMModel = glm::mat4(1.0f); // Identity matrix

		GLMModel = glm::translate(GLMModel, intersection);

		GLBfresLibrary::GetModel(BfresLibrary::GetModel("Default"))->Draw(InstanceMatrix, InstancedShader);

		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
		{
			rayDir = glm::vec3(0.0f, -1.0f, 0.0f);
			unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
			std::default_random_engine generator(seed);
			std::uniform_real_distribution<float> distribution(0.0f, 1.0f);
			for (int i = 0; i < mActorPainterIterations; i++)
			{
				glm::vec3 Pos = intersection;
				Pos.x += rand() * 1.0 / RAND_MAX * (mActorPainterRadius - (-mActorPainterRadius) + 1) + (-mActorPainterRadius);
				Pos.z += rand() * 1.0 / RAND_MAX * (mActorPainterRadius - (-mActorPainterRadius) + 1) + (-mActorPainterRadius);

				Pos.y += 100.0f;

				std::pair<glm::vec3, glm::vec3> ActorTerrainIntersection = FindTerrainIntersectionAndNormalVec(Pos, rayDir, mActorPainterTerrain);

				if (ActorTerrainIntersection.first != glm::vec3(0.0f))
				{
					ActorPainterActorEntry* Entry = nullptr;
					float randomValue = distribution(generator);

					float cumulativeProbability = 0.0f;
					for (auto& entry : mActorPainterActorEntries) {
						cumulativeProbability += entry.mProbability;
						if (randomValue <= cumulativeProbability) {
							Entry = &entry;
							break;
						}
					}

					if (Entry == nullptr)
						Entry = &mActorPainterActorEntries.back();

					Actor& NewActor = ActorMgr::AddActor(Entry->mGyml);

					/*
					if (Entry->mYModelOffset == 1000.0f)
					{
						Entry->mYModelOffset = -CalculateModelYOffset(NewActor);
					}
					*/

					NewActor.Translate.SetX(Pos.x);
					NewActor.Translate.SetY(ActorTerrainIntersection.first.y + Entry->mYModelOffset);
					NewActor.Translate.SetZ(Pos.z);

					NewActor.Scale.SetX(mActorPainterScaleMin.x + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (mActorPainterScaleMax.x - mActorPainterScaleMin.x))));
					NewActor.Scale.SetY(mActorPainterScaleMin.y + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (mActorPainterScaleMax.y - mActorPainterScaleMin.y))));
					NewActor.Scale.SetZ(mActorPainterScaleMin.z + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (mActorPainterScaleMax.z - mActorPainterScaleMin.z))));

					glm::vec3 TerrainNormalRot = NormalToEuler(ActorTerrainIntersection.second);

					NewActor.Rotate.SetX(TerrainNormalRot.x + mActorPainterRotMin.x + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (mActorPainterRotMax.x - mActorPainterRotMin.x))));
					NewActor.Rotate.SetY(TerrainNormalRot.y + mActorPainterRotMin.y + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (mActorPainterRotMax.y - mActorPainterRotMin.y))));
					NewActor.Rotate.SetZ(TerrainNormalRot.z + mActorPainterRotMin.z + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (mActorPainterRotMax.z - mActorPainterRotMin.z))));
					
					for (Actor& SceneActor : ActorMgr::GetActors())
					{
						if (SceneActor.Hash == mActorPainterTerrainHash)
						{
							mActorPainterTerrain = &SceneActor;
							break;
						}
					}
				}
			}
		}
	}
}

void UIMapView::Initialize(GLFWwindow* pWindow)
{
	Window = pWindow;
	CameraView.pWindow = pWindow;

	MapViewFramebuffer = FramebufferMgr::CreateFramebuffer(1280, 720, "MapView");

	DefaultShader = ShaderMgr::GetShader("Default");
	SelectedShader = ShaderMgr::GetShader("Selected");
	PickingShader = ShaderMgr::GetShader("Picking");
	InstancedShader = ShaderMgr::GetShader("Instanced");
	InstancedDiscardShader = ShaderMgr::GetShader("InstancedDiscard");
	GameShader = ShaderMgr::GetShader("Game");
	NavMeshShader = ShaderMgr::GetShader("NavMesh");

	//HeightMap = HGHTFile("/switchemulator/Zelda TotK/MapEditorV4/TerrainWorkspace/5700001C8C.hght");
	//HeightMap.InitMesh();
}

void UIMapView::DrawOverlay()
{
	ImGui::Text("FPS: %.1f (%.3f ms/frame)", ImGui::GetIO().Framerate, 1000.0 / ImGui::GetIO().Framerate);

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.5f));
	if (ImGui::Button(std::string("Mode: " + (ImGuizmoMode == ImGuizmo::WORLD ? std::string("World") : std::string("Local"))).c_str()))
	{
		if (ImGuizmoMode == ImGuizmo::WORLD) ImGuizmoMode = ImGuizmo::LOCAL;
		else ImGuizmoMode = ImGuizmo::WORLD;
	}
	ImGui::PopStyleColor();

	ImGui::SameLine();

	//ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 0.5f));
	ImGui::PushStyleColor(ImGuiCol_Button, UIMapView::ImGuizmoOperation == ImGuizmo::TRANSLATE ? ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered] : ImVec4(0.0f, 0.0f, 0.0f, 0.5f));
	if (ImGui::ImageButton((ImTextureID)TextureMgr::GetTexture("TranslateGizmo")->ID, ImVec2(16 * Editor::UIScale, 16 * Editor::UIScale), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), 2))
	{
		UIMapView::ImGuizmoOperation = ImGuizmo::TRANSLATE;
	}
	ImGui::SetItemTooltip("Translate Gizmo (G)");
	ImGui::SameLine();
	ImGui::PopStyleColor();

	ImGui::PushStyleColor(ImGuiCol_Button, UIMapView::ImGuizmoOperation == ImGuizmo::ROTATE ? ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered] : ImVec4(0.0f, 0.0f, 0.0f, 0.5f));
	if (ImGui::ImageButton((ImTextureID)TextureMgr::GetTexture("RotateGizmo")->ID, ImVec2(16 * Editor::UIScale, 16 * Editor::UIScale), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), 2))
	{
		UIMapView::ImGuizmoOperation = ImGuizmo::ROTATE;
	}
	ImGui::SetItemTooltip("Rotate Gizmo (R)");
	ImGui::SameLine();
	ImGui::PopStyleColor();

	ImGui::PushStyleColor(ImGuiCol_Button, UIMapView::ImGuizmoOperation == ImGuizmo::SCALE ? ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered] : ImVec4(0.0f, 0.0f, 0.0f, 0.5f));
	if (ImGui::ImageButton((ImTextureID)TextureMgr::GetTexture("ScaleGizmo")->ID, ImVec2(16 * Editor::UIScale, 16 * Editor::UIScale), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), 2))
	{
		UIMapView::ImGuizmoOperation = ImGuizmo::SCALE;
	}
	ImGui::SetItemTooltip("Scale Gizmo (B)");
	ImGui::PopStyleColor();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.5f));

	if (ImGui::ImageButton((ImTextureID)TextureMgr::GetTexture("Undo")->ID, ImVec2(16 * Editor::UIScale, 16 * Editor::UIScale), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), 2))
	{
		ActionMgr::Undo();
	}
	ImGui::SetItemTooltip("Undo (Ctrl + Z)");

	ImGui::SameLine();

	if (ImGui::ImageButton((ImTextureID)TextureMgr::GetTexture("Redo")->ID, ImVec2(16 * Editor::UIScale, 16 * Editor::UIScale), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), 2))
	{
		ActionMgr::Redo();
	}
	ImGui::SetItemTooltip("Redo (Ctrl + Shift + Z)");

	ImGui::SameLine();
	float w = ImGui::GetCursorPosX();
	if (ImGui::Button("Camera##CameraMenuButton"))
	{
		ImGui::OpenPopup("CameraMenu");
	}
	ImVec2 Pos = ImGui::GetCursorScreenPos();
	ImGui::PopStyleColor();
	DrawCameraMenu(w, Pos);

	ImGui::SameLine();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.5f));
	w = ImGui::GetCursorPosX();
	if (ImGui::Button("Actor Painter##ActorPainterButton"))
	{
		ImGui::OpenPopup("ActorPainter");
	}
	Pos = ImGui::GetCursorScreenPos();
	ImGui::PopStyleColor();
	DrawActorPainterMenu(w, Pos);
}

void UIMapView::DrawCameraMenu(float w, ImVec2 Pos)
{
	ImGui::SetNextWindowPos(ImVec2(Pos.x + w, Pos.y));
	if (ImGui::BeginPopup("CameraMenu"))
	{
		bool UpdateSettings = false;

		if (ImGui::Button("Reset##CameraMenu"))
		{
			UpdateSettings = true;
			CameraView.Speed = 0.5f;
			CameraView.BoostMultiplier = 10.0f;
			CameraView.Sensitivity = 100.0f;
		}
		UpdateSettings |= StarImGui::InputFloatSliderCombo("Speed", "SpeedCameraMenu", &CameraView.Speed, 0.01f, 100.0f);
		UpdateSettings |= StarImGui::InputFloatSliderCombo("Boost multiplier", "BoostMultiplierCameraMenu", &CameraView.BoostMultiplier, 0.1f, 100.0f);
		UpdateSettings |= StarImGui::InputFloatSliderCombo("Sensitivity", "SensitivityCameraMenu", &CameraView.Sensitivity, 10.0f, 500.0f);

		if (UpdateSettings)
		{
			PreferencesConfig::Save();
		}
		ImGui::EndPopup();
	}
}

void UIMapView::DrawActorPainterMenu(float w, ImVec2 Pos)
{
	ImGui::SetNextWindowPos(ImVec2(Pos.x + w, Pos.y));
	if (ImGui::BeginPopup("ActorPainter"))
	{
		if (ImGui::Checkbox("Enable##ActorPainter", &mEnableActorPainter))
		{
			if (mEnableActorPainter)
			{
				UIOutliner::SelectedActor = nullptr;
			}
		}
		if (ImGui::InputScalar("Hash##ActorPainter", ImGuiDataType_U64, &mActorPainterTerrainHash))
		{
			for (Actor& SceneActor : ActorMgr::GetActors())
			{
				if (SceneActor.Hash == mActorPainterTerrainHash)
				{
					mActorPainterTerrain = &SceneActor;
					std::cout << "Found terrain actor\n";
					break;
				}
			}
		}
		ImGui::InputScalar("Radius##ActorPainter", ImGuiDataType_Float, &mActorPainterRadius);
		ImGui::InputScalar("Actors to place##ActorPainter", ImGuiDataType_U32, &mActorPainterIterations);
		/*
			extern glm::vec3 mActorPainterScaleMin;
	extern glm::vec3 mActorPainterScaleMax;
	extern glm::vec3 mActorPainterRotMin;
	extern glm::vec3 mActorPainterRotMax;
		*/
		StarImGui::InputFloat3Colored("Scale Min", &mActorPainterScaleMin.x, ImVec4(0.26f, 0.06f, 0.07f, 1.0f), ImVec4(0.06f, 0.26f, 0.07f, 1.0f), ImVec4(0.06f, 0.07f, 0.26f, 1.0f));
		StarImGui::InputFloat3Colored("Scale Max", &mActorPainterScaleMax.x, ImVec4(0.26f, 0.06f, 0.07f, 1.0f), ImVec4(0.06f, 0.26f, 0.07f, 1.0f), ImVec4(0.06f, 0.07f, 0.26f, 1.0f));
		StarImGui::InputFloat3Colored("Rotation Min", &mActorPainterRotMin.x, ImVec4(0.26f, 0.06f, 0.07f, 1.0f), ImVec4(0.06f, 0.26f, 0.07f, 1.0f), ImVec4(0.06f, 0.07f, 0.26f, 1.0f));
		StarImGui::InputFloat3Colored("Rotation Max", &mActorPainterRotMax.x, ImVec4(0.26f, 0.06f, 0.07f, 1.0f), ImVec4(0.06f, 0.26f, 0.07f, 1.0f), ImVec4(0.06f, 0.07f, 0.26f, 1.0f));
		
		ImGui::NewLine();
		ImGui::Text("Actors");
		ImGui::SameLine();
		if (ImGui::Button("+##AddActorPainterActor"))
		{
			mActorPainterActorEntries.push_back(ActorPainterActorEntry{
				.mGyml = "None",
				.mYModelOffset = 0.0f,
				.mProbability = 1.0f
				});
		}
		int Index = 0;
		for (ActorPainterActorEntry& Entry : mActorPainterActorEntries)
		{
			ImGui::PushItemWidth(ImGui::CalcTextSize(Entry.mGyml.c_str()).x + 2 * ImGui::GetStyle().ItemInnerSpacing.x);
			ImGui::InputText(("Gyml##" + std::to_string(Index)).c_str(), &Entry.mGyml);
			ImGui::PopItemWidth();

			ImGui::SameLine();

			ImGui::PushItemWidth(ImGui::CalcTextSize(std::to_string(Entry.mProbability).c_str()).x + 2 * ImGui::GetStyle().ItemInnerSpacing.x);
			ImGui::InputFloat(("Probability##" + std::to_string(Index)).c_str(), &Entry.mProbability);
			ImGui::PopItemWidth();

			ImGui::SameLine();

			ImGui::PushItemWidth(ImGui::CalcTextSize(std::to_string(Entry.mYModelOffset).c_str()).x + 2 * ImGui::GetStyle().ItemInnerSpacing.x);
			ImGui::InputFloat(("Y-Offset##" + std::to_string(Index)).c_str(), &Entry.mYModelOffset);
			ImGui::PopItemWidth();

			Index++;
		}

		ImGui::EndPopup();
	}
}

void UIMapView::DrawActor(Actor& Actor, Shader* Shader)
{
	std::vector<glm::mat4> InstanceMatrix(1);

	glm::mat4& GLMModel = InstanceMatrix[0];
	GLMModel = glm::mat4(1.0f); // Identity matrix

	GLMModel = glm::translate(GLMModel, glm::vec3(Actor.Translate.GetX(), Actor.Translate.GetY(), Actor.Translate.GetZ()));

	GLMModel = glm::rotate(GLMModel, glm::radians(Actor.Rotate.GetZ()), glm::vec3(0.0, 0.0f, 1.0));
	GLMModel = glm::rotate(GLMModel, glm::radians(Actor.Rotate.GetY()), glm::vec3(0.0f, 1.0, 0.0));
	GLMModel = glm::rotate(GLMModel, glm::radians(Actor.Rotate.GetX()), glm::vec3(1.0, 0.0f, 0.0));

	GLMModel = glm::scale(GLMModel, glm::vec3(Actor.Scale.GetX(), Actor.Scale.GetY(), Actor.Scale.GetZ()));

	Actor.Model->Draw(InstanceMatrix, Shader);
}

void UIMapView::DrawInstancedActor(GLBfres* Model, std::vector<Actor*>& ActorPtrs, Shader* Shader)
{
	std::vector<glm::mat4> InstanceMatrices(ActorPtrs.size());

	int Shrinking = 0;
	for (size_t i = 0; i < ActorPtrs.size(); i++)
	{
		if ((ActorPtrs[i]->MergedActorParent == UIOutliner::SelectedActor && UIOutliner::SelectedActor != nullptr) || ActorPtrs[i] == UIOutliner::SelectedActor || !Frustum::SphereInFrustum(ActorPtrs[i]->Translate.GetX(), ActorPtrs[i]->Translate.GetY(), ActorPtrs[i]->Translate.GetZ(), ActorPtrs[i]->Model->mBfres->Models.GetByIndex(0).Value.BoundingBoxSphereRadius * std::fmax(std::fmax(ActorPtrs[i]->Scale.GetX(), ActorPtrs[i]->Scale.GetY()), ActorPtrs[i]->Scale.GetZ())))
		{
			Shrinking++;
			continue;
		}

		InstanceMatrices[i - Shrinking] = glm::mat4(1.0f); // Identity matrix

		InstanceMatrices[i - Shrinking] = glm::translate(InstanceMatrices[i - Shrinking], glm::vec3(ActorPtrs[i]->Translate.GetX(), ActorPtrs[i]->Translate.GetY(), ActorPtrs[i]->Translate.GetZ()));

		InstanceMatrices[i - Shrinking] = glm::rotate(InstanceMatrices[i - Shrinking], glm::radians(ActorPtrs[i]->Rotate.GetZ()), glm::vec3(0.0, 0.0f, 1.0));
		InstanceMatrices[i - Shrinking] = glm::rotate(InstanceMatrices[i - Shrinking], glm::radians(ActorPtrs[i]->Rotate.GetY()), glm::vec3(0.0f, 1.0, 0.0));
		InstanceMatrices[i - Shrinking] = glm::rotate(InstanceMatrices[i - Shrinking], glm::radians(ActorPtrs[i]->Rotate.GetX()), glm::vec3(1.0, 0.0f, 0.0));

		InstanceMatrices[i - Shrinking] = glm::scale(InstanceMatrices[i - Shrinking], glm::vec3(ActorPtrs[i]->Scale.GetX(), ActorPtrs[i]->Scale.GetY(), ActorPtrs[i]->Scale.GetZ()));
	}
	
	if (Shrinking > 0)
		InstanceMatrices.resize(InstanceMatrices.size() - Shrinking);

	Model->Draw(InstanceMatrices, Shader);
}

void UIMapView::SelectActorByClicking(ImVec2 SceneWindowSize, ImVec2 MousePos)
{
	if (!RenderSettings.AllowSelectingActor || !Focused || mEnableActorPainter) return;
	if (glfwGetMouseButton(Window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && glfwGetMouseButton(Window, GLFW_MOUSE_BUTTON_RIGHT) != GLFW_PRESS && !ImGui::IsAnyItemHovered())
	{
		if (!(SceneWindowSize.x > MousePos.x && SceneWindowSize.y > MousePos.y && MousePos.x > 0 && MousePos.y > 0)) return;
		if (ImGuizmo::IsOver() && UIOutliner::SelectedActor != nullptr) return;

		MapViewFramebuffer->Bind();
		glClearColor(ClearColor.x * ClearColor.w, ClearColor.y * ClearColor.w, ClearColor.z * ClearColor.w, ClearColor.w);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		PickingShader->Activate();

		int MergedActorIndex = 0;
		bool Discard = false;

		for (int ActorIndex = 0; ActorIndex < ActorMgr::GetActors().size(); ActorIndex++)
		{
			Actor& CurrentActor = ActorMgr::GetActors()[ActorIndex];
			if (!CurrentActor.Model->mBfres->mDefaultModel && !RenderSettings.RenderVisibleActors) continue;
			if (CurrentActor.Model->mBfres->mDefaultModel && !RenderSettings.RenderInvisibleActors && !CurrentActor.IsUMii) continue;
			if (CurrentActor.Gyml.ends_with("_Far") && !RenderSettings.RenderFarActors) continue;
			if (CurrentActor.Gyml.starts_with("Npc_") && !RenderSettings.RenderNPCs) continue;
			if (CurrentActor.Gyml.find("Area") != std::string::npos && CurrentActor.Model->mBfres->mDefaultModel && !RenderSettings.RenderAreas) continue;

			Discard = CurrentActor.Model->mIsDiscard;

			int R = (ActorIndex & 0x000000FF) >> 0;
			int G = (ActorIndex & 0x0000FF00) >> 8;
			int B = (ActorIndex & 0x00FF0000) >> 16;

			glUniform4f(glGetUniformLocation(PickingShader->ID, "PickingColor"), R / 255.0f, G / 255.0f, B / 255.0f, 1.0f);

			if (!CurrentActor.IsUMii)
			{
				std::vector<glm::mat4> InstanceMatrix(1);

				glm::mat4& GLMModel = InstanceMatrix[0];
				GLMModel = glm::mat4(1.0f); // Identity matrix

				GLMModel = glm::translate(GLMModel, glm::vec3(CurrentActor.Translate.GetX(), CurrentActor.Translate.GetY(), CurrentActor.Translate.GetZ()));

				GLMModel = glm::rotate(GLMModel, glm::radians(CurrentActor.Rotate.GetZ()), glm::vec3(0.0, 0.0f, 1.0));
				GLMModel = glm::rotate(GLMModel, glm::radians(CurrentActor.Rotate.GetY()), glm::vec3(0.0f, 1.0, 0.0));
				GLMModel = glm::rotate(GLMModel, glm::radians(CurrentActor.Rotate.GetX()), glm::vec3(1.0, 0.0f, 0.0));

				GLMModel = glm::scale(GLMModel, glm::vec3(CurrentActor.Scale.GetX(), CurrentActor.Scale.GetY(), CurrentActor.Scale.GetZ()));

				CurrentActor.Model->Draw(InstanceMatrix, PickingShader);
			}
			else
			{
				CurrentActor.UMiiData.Draw(CurrentActor.Translate, CurrentActor.Rotate, CurrentActor.Scale, PickingShader, true, &CameraView);
			}
		}

		MergedActorIndex = ActorMgr::GetActors().size();

		std::unordered_map<int, Actor*> MergedActorIds;

		for (Actor& MergedActorMain : ActorMgr::GetActors())
		{
			for (Actor& MergedActor : MergedActorMain.MergedActorContent)
			{
				if (!MergedActor.Model->mBfres->mDefaultModel && !RenderSettings.RenderVisibleActors) continue;
				if (MergedActor.Model->mBfres->mDefaultModel && !RenderSettings.RenderInvisibleActors && !MergedActor.IsUMii) continue;
				if (MergedActor.Gyml.ends_with("_Far") && !RenderSettings.RenderFarActors) continue;
				if (MergedActor.Gyml.starts_with("Npc_") && !RenderSettings.RenderNPCs) continue;
				if (MergedActor.Gyml.find("Area") != std::string::npos && MergedActor.Model->mBfres->mDefaultModel && !RenderSettings.RenderAreas) continue;

				int R = (MergedActorIndex & 0x000000FF) >> 0;
				int G = (MergedActorIndex & 0x0000FF00) >> 8;
				int B = (MergedActorIndex & 0x00FF0000) >> 16;

				std::vector<glm::mat4> InstanceMatrix(1);

				glm::mat4& GLMModel = InstanceMatrix[0];
				GLMModel = glm::mat4(1.0f); // Identity matrix

				GLMModel = glm::translate(GLMModel, glm::vec3(MergedActor.Translate.GetX(), MergedActor.Translate.GetY(), MergedActor.Translate.GetZ()));

				GLMModel = glm::rotate(GLMModel, glm::radians(MergedActor.Rotate.GetZ()), glm::vec3(0.0, 0.0f, 1.0));
				GLMModel = glm::rotate(GLMModel, glm::radians(MergedActor.Rotate.GetY()), glm::vec3(0.0f, 1.0, 0.0));
				GLMModel = glm::rotate(GLMModel, glm::radians(MergedActor.Rotate.GetX()), glm::vec3(1.0, 0.0f, 0.0));

				GLMModel = glm::scale(GLMModel, glm::vec3(MergedActor.Scale.GetX(), MergedActor.Scale.GetY(), MergedActor.Scale.GetZ()));

				glUniform4f(glGetUniformLocation(PickingShader->ID, "PickingColor"), R / 255.0f, G / 255.0f, B / 255.0f, 1.0f);

				MergedActor.Model->Draw(InstanceMatrix, PickingShader);

				MergedActorIds.insert({ MergedActorIndex, &MergedActor });
				MergedActorIndex++;
			}
		}

		glReadBuffer(GL_COLOR_ATTACHMENT0);
		unsigned char Data[3];

		glReadPixels((GLint)MousePos.x, (GLint)SceneWindowSize.y - MousePos.y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, Data);

		int PickedActorId =
			Data[0] +
			Data[1] * 256 +
			Data[2] * 256 * 256;

		if (PickedActorId == 4208174 || PickedActorId < 0) //3024412 = background color
		{
			UIOutliner::SelectActor(nullptr);
		}
		else
		{
			if (PickedActorId < ActorMgr::GetActors().size())
			{
				UIOutliner::SelectActor(&ActorMgr::GetActors()[PickedActorId]);
			}
			else
			{
				UIOutliner::SelectActor(MergedActorIds[PickedActorId]);
			}
		}

		glReadBuffer(GL_NONE);
	}
}

void UIMapView::DrawRails()
{

}

void UIMapView::DrawMapViewWindow()
{
	if (!Open) return;

	Focused = ImGui::Begin("Map View", &Open);

	RenderSettings.AllowSelectingActor = RenderSettings.AllowSelectingActor && !ImGui::GetIO().WantTextInput;

	ImGuizmo::Enable(RenderSettings.AllowSelectingActor);
	ImGuizmo::SetDrawlist();

	const float WindowWidth = ImGui::GetContentRegionAvail().x;
	const float WindowHeight = ImGui::GetContentRegionAvail().y;

	CameraView.Width = WindowWidth;
	CameraView.Height = WindowHeight;

	MapViewFramebuffer->RescaleFramebuffer(WindowWidth, WindowHeight); //This also binds the frame buffer

	ImVec2 Pos = ImGui::GetCursorScreenPos();
	CameraView.WindowHovered = (ImGui::IsWindowHovered() && !ImGui::IsItemHovered());

	glViewport(0, 0, WindowWidth, WindowHeight);
	glClearColor(ClearColor.x * ClearColor.w, ClearColor.y * ClearColor.w, ClearColor.z * ClearColor.w, ClearColor.w);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	CameraView.Inputs(ImGui::GetIO().Framerate, ImGui::GetWindowPos());
	CameraView.UpdateMatrix(45.0f, 0.1f, 30000.0f);
	PickingShader->Activate();
	CameraView.Matrix(PickingShader, "camMatrix");

	Frustum::CalculateFrustum(&CameraView);

	ImVec2 ScreenSize = ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y);
	ImVec2 MousePos = ImVec2(ImGui::GetMousePos().x - ImGui::GetWindowPos().x - ImGui::GetWindowContentRegionMin().x,
		ImGui::GetMousePos().y - ImGui::GetWindowPos().y - ImGui::GetWindowContentRegionMin().y);

	SelectActorByClicking(ScreenSize, MousePos);

	glClearColor(ClearColor.x * ClearColor.w, ClearColor.y * ClearColor.w, ClearColor.z * ClearColor.w, ClearColor.w);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	InstancedShader->Activate();
	CameraView.Matrix(InstancedShader, "camMatrix");
	glUniform3fv(glGetUniformLocation(InstancedShader->ID, "lightColor"), 1, &mLightColor[0]);
	glUniform3fv(glGetUniformLocation(InstancedShader->ID, "lightPos"), 1, &CameraView.Position[0]);

	for (auto& [Model, ActorPtrs] : ActorMgr::OpaqueActors)
	{
		if (!Model->mBfres->mDefaultModel && !RenderSettings.RenderVisibleActors) continue;
		if (Model->mBfres->mDefaultModel && !RenderSettings.RenderInvisibleActors) continue;
		if (ActorPtrs[0]->Gyml.ends_with("_Far") && !RenderSettings.RenderFarActors) continue;
		if (ActorPtrs[0]->Gyml.starts_with("Npc_") && !RenderSettings.RenderNPCs) continue;

		DrawInstancedActor(Model, ActorPtrs, InstancedShader);
	}

	if (RenderSettings.RenderNPCs)
	{
		for (Actor* NPC : ActorMgr::UMiiActors)
		{
			if (NPC == UIOutliner::SelectedActor) continue;
			NPC->UMiiData.Draw(NPC->Translate, NPC->Rotate, NPC->Scale, InstancedShader);
		}
	}

	InstancedDiscardShader->Activate();
	CameraView.Matrix(InstancedDiscardShader, "camMatrix");
	glUniform3fv(glGetUniformLocation(InstancedDiscardShader->ID, "lightColor"), 1, &mLightColor[0]);
	glUniform3fv(glGetUniformLocation(InstancedDiscardShader->ID, "lightPos"), 1, &CameraView.Position[0]);

	for (auto& [Model, ActorPtrs] : ActorMgr::TransparentActorsDiscard)
	{
		if (!Model->mBfres->mDefaultModel && !RenderSettings.RenderVisibleActors) continue;
		if (ActorPtrs[0]->Gyml.starts_with("Npc_") && !RenderSettings.RenderNPCs) continue;
		if (ActorPtrs[0]->Gyml.ends_with("_Far") && !RenderSettings.RenderFarActors) continue;
		if (ActorPtrs[0]->Gyml.find("Area") != std::string::npos && Model->mBfres->mDefaultModel && (!RenderSettings.RenderAreas || !RenderSettings.RenderInvisibleActors)) continue;

		DrawInstancedActor(Model, ActorPtrs, InstancedDiscardShader);
	}

	//Selected actor
	if (UIOutliner::SelectedActor != nullptr)
	{
		SelectedShader->Activate();
		CameraView.Matrix(SelectedShader, "camMatrix");
		glUniform3fv(glGetUniformLocation(SelectedShader->ID, "lightColor"), 1, &mLightColor[0]);
		glUniform3fv(glGetUniformLocation(SelectedShader->ID, "lightPos"), 1, &CameraView.Position[0]);
		if (!UIOutliner::SelectedActor->IsUMii)
		{
			DrawActor(*UIOutliner::SelectedActor, SelectedShader);

			for (Actor& Child : UIOutliner::SelectedActor->MergedActorContent)
			{
				DrawActor(Child, SelectedShader);
			}
		}
		else
		{
			UIOutliner::SelectedActor->UMiiData.Draw(UIOutliner::SelectedActor->Translate, UIOutliner::SelectedActor->Rotate, UIOutliner::SelectedActor->Scale, SelectedShader);
		}
	}

	glDisable(GL_CULL_FACE);

	if (RenderSettings.RenderNavMesh)
	{
		NavMeshShader->Activate();
		CameraView.Matrix(NavMeshShader, "camMatrix");
		glm::mat4 GLMModel = glm::mat4(1.0f);
		glUniformMatrix4fv(glGetUniformLocation(NavMeshShader->ID, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(GLMModel));
		SceneMgr::NavigationMeshModel.DrawRaw();
	}

	if (mEnableActorPainter)
	{
		ProcessActorPainter(MousePos.x, MousePos.y, ScreenSize.x, ScreenSize.y);
	}

	if (!mDebugMeshes.empty())
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		NavMeshShader->Activate();
		CameraView.Matrix(NavMeshShader, "camMatrix");

		for (Mesh& DebugMesh : mDebugMeshes)
		{
			glm::mat4 GLMModel = glm::mat4(1.0f);
			glUniformMatrix4fv(glGetUniformLocation(NavMeshShader->ID, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(GLMModel));
			DebugMesh.DrawRaw();
		}
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	/*
	NavMeshShader->Activate();
	CameraView.Matrix(NavMeshShader, "camMatrix");
	glm::mat4 GLMModel = glm::mat4(1.0f);
	glUniformMatrix4fv(glGetUniformLocation(NavMeshShader->ID, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(GLMModel));
	HeightMap.mMesh.DrawRaw();

	glBindVertexArray(0);
	glUseProgram(0);
	*/

	//DefaultShader->Activate();
	//HeightMapFile.Draw(DefaultShader);

	ImGui::GetWindowDrawList()->AddImage(
		(void*)MapViewFramebuffer->GetFrameTexture(),
		ImVec2(Pos.x, Pos.y),
		ImVec2(Pos.x + WindowWidth, Pos.y + WindowHeight),
		ImVec2(0, 1),
		ImVec2(1, 0)
	);

	if (UIOutliner::SelectedActor != nullptr)
	{
		ImGuizmo::RecomposeMatrixFromComponents(UIOutliner::SelectedActor->Translate.GetRawData(), UIOutliner::SelectedActor->Rotate.GetRawData(), UIOutliner::SelectedActor->Scale.GetRawData(), ObjectMatrix);

		ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, ImGui::GetWindowWidth(), ImGui::GetWindowHeight());

		ImGuizmo::Manipulate(glm::value_ptr(CameraView.GetViewMatrix()), glm::value_ptr(CameraView.GetProjectionMatrix()), ImGuizmoOperation, ImGuizmoMode, ObjectMatrix);
	}

	if (ImGuizmo::IsUsingAny())
	{
		if (!IsUsingGizmo)
		{
			switch (ImGuizmoOperation)
			{
			case ImGuizmo::TRANSLATE:
				PreviousVectorData = UIOutliner::SelectedActor->Translate;
				break;
			case ImGuizmo::ROTATE:
				PreviousVectorData = UIOutliner::SelectedActor->Rotate;
				break;
			case ImGuizmo::SCALE:
				PreviousVectorData = UIOutliner::SelectedActor->Scale;
				break;
			default:
				Logger::Error("UIMapView", "Unknown ImGuizmo operation");
				break;
			}
			IsUsingGizmo = true;
		}

		Actor OldActor = *UIOutliner::SelectedActor;
		ImGuizmo::DecomposeMatrixToComponents(ObjectMatrix, UIOutliner::SelectedActor->Translate.GetRawData(), UIOutliner::SelectedActor->Rotate.GetRawData(), UIOutliner::SelectedActor->Scale.GetRawData());
		ActorMgr::UpdateMergedActorContent(UIOutliner::SelectedActor, OldActor);
	}
	else if (IsUsingGizmo)
	{
		IsUsingGizmo = false;
		switch (ImGuizmoOperation)
		{
		case ImGuizmo::TRANSLATE:
			ActionMgr::AddAction<Vector3F>(&UIOutliner::SelectedActor->Translate, PreviousVectorData);
			break;
		case ImGuizmo::ROTATE:
			ActionMgr::AddAction<Vector3F>(&UIOutliner::SelectedActor->Rotate, PreviousVectorData);
			break;
		case ImGuizmo::SCALE:
			ActionMgr::AddAction<Vector3F>(&UIOutliner::SelectedActor->Scale, PreviousVectorData);
			break;
		default:
			Logger::Error("UIMapView", "Unknown ImGuizmo operation");
			break;
		}
	}

	DrawOverlay();

	MapViewFramebuffer->Unbind();

	ImGui::End();

	RenderSettings.AllowSelectingActor = true;
}

bool UIMapView::IsInCameraMovement()
{
	return CameraView.IsInCameraMovement();
}

void UIMapView::GLFWScrollCallback(GLFWwindow* window, double xOffset, double yOffset)
{
	CameraView.MouseWheelCallback(window, xOffset, yOffset);
}