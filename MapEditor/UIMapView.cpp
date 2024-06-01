#include "UIMapView.h"

#include "Frustum.h"
#include "Logger.h"
#include "TextureMgr.h"
#include "UIOutliner.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_internal.h"
#include "imgui_stdlib.h"
#include <glm/gtc/matrix_transform.hpp>

bool UIMapView::Open = true;
bool UIMapView::Focused = false;

GLFWwindow* UIMapView::Window;

Camera UIMapView::CameraView = Camera(1280, 720, glm::vec3(0.0f, 0.0f, 2.0f), nullptr);

Framebuffer* UIMapView::MapViewFramebuffer = nullptr;

Shader* UIMapView::DefaultShader = nullptr;
Shader* UIMapView::SelectedShader = nullptr;
Shader* UIMapView::PickingShader = nullptr;
Shader* UIMapView::InstancedShader = nullptr;
Shader* UIMapView::GameShader = nullptr;

ImVec4 UIMapView::ClearColor = ImVec4(0.18f, 0.21f, 0.25f, 1.00f);

UIMapView::RenderingSettingsStruct UIMapView::RenderSettings;

float UIMapView::ObjectMatrix[16] = { 1.f, 0.f, 0.f, 0.f,
    0.f, 1.f, 0.f, 0.f,
    0.f, 0.f, 1.f, 0.f,
    0.f, 0.f, 0.f, 1.f };

ImGuizmo::MODE UIMapView::ImGuizmoMode = ImGuizmo::WORLD;
ImGuizmo::OPERATION UIMapView::ImGuizmoOperation = ImGuizmo::TRANSLATE;

void UIMapView::GLFWKeyCallback(GLFWwindow* Window, int Key, int Scancode, int Action, int Mods)
{
    if (ImGui::GetIO().WantTextInput || CameraView.IsInCameraMovement() || Action != GLFW_PRESS)
        return;

    // Rotation hot key
    if (Key == GLFW_KEY_R)
        ImGuizmoOperation = ImGuizmo::ROTATE;

    if (Key == GLFW_KEY_S)
        ImGuizmoOperation = ImGuizmo::SCALE;

    if (Key == GLFW_KEY_G)
        ImGuizmoOperation = ImGuizmo::TRANSLATE;
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
    GameShader = ShaderMgr::GetShader("Game");
}

void UIMapView::DrawOverlay()
{
    ImGui::Text("FPS: %.1f (%.3f ms/frame)", ImGui::GetIO().Framerate, 1000.0 / ImGui::GetIO().Framerate);

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.5f));
    if (ImGui::Button(std::string("Mode: " + (ImGuizmoMode == ImGuizmo::WORLD ? std::string("World") : std::string("Local"))).c_str())) {
        if (ImGuizmoMode == ImGuizmo::WORLD)
            ImGuizmoMode = ImGuizmo::LOCAL;
        else
            ImGuizmoMode = ImGuizmo::WORLD;
    }
    ImGui::PopStyleColor();

    ImGui::SameLine();

    // ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 0.5f));
    ImGui::PushStyleColor(ImGuiCol_Button, UIMapView::ImGuizmoOperation == ImGuizmo::TRANSLATE ? ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered] : ImVec4(0.0f, 0.0f, 0.0f, 0.5f));
    if (ImGui::ImageButton((ImTextureID)TextureMgr::GetTexture("TranslateGizmo")->ID, ImVec2(16, 16), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), 2)) {
        UIMapView::ImGuizmoOperation = ImGuizmo::TRANSLATE;
    }
    ImGui::SetItemTooltip("Translate Gizmo (G)");
    ImGui::SameLine();
    ImGui::PopStyleColor();

    ImGui::PushStyleColor(ImGuiCol_Button, UIMapView::ImGuizmoOperation == ImGuizmo::ROTATE ? ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered] : ImVec4(0.0f, 0.0f, 0.0f, 0.5f));
    if (ImGui::ImageButton((ImTextureID)TextureMgr::GetTexture("RotateGizmo")->ID, ImVec2(16, 16), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), 2)) {
        UIMapView::ImGuizmoOperation = ImGuizmo::ROTATE;
    }
    ImGui::SetItemTooltip("Rotate Gizmo (R)");
    ImGui::SameLine();
    ImGui::PopStyleColor();

    ImGui::PushStyleColor(ImGuiCol_Button, UIMapView::ImGuizmoOperation == ImGuizmo::SCALE ? ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered] : ImVec4(0.0f, 0.0f, 0.0f, 0.5f));
    if (ImGui::ImageButton((ImTextureID)TextureMgr::GetTexture("ScaleGizmo")->ID, ImVec2(16, 16), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), 2)) {
        UIMapView::ImGuizmoOperation = ImGuizmo::SCALE;
    }
    ImGui::SetItemTooltip("Scale Gizmo (S)");
    ImGui::PopStyleColor();
}

void UIMapView::DrawActor(Actor& Actor, Shader* Shader)
{
    BfresFile::LOD* LODModel = &Actor.Model->GetModels()[0].LODs[0];

    glm::mat4 GLMModel = glm::mat4(1.0f); // Identity matrix

    GLMModel = glm::translate(GLMModel, glm::vec3(Actor.Translate.GetX(), Actor.Translate.GetY(), Actor.Translate.GetZ()));

    GLMModel = glm::rotate(GLMModel, glm::radians(Actor.Rotate.GetZ()), glm::vec3(0.0, 0.0f, 1.0));
    GLMModel = glm::rotate(GLMModel, glm::radians(Actor.Rotate.GetY()), glm::vec3(0.0f, 1.0, 0.0));
    GLMModel = glm::rotate(GLMModel, glm::radians(Actor.Rotate.GetX()), glm::vec3(1.0, 0.0f, 0.0));

    GLMModel = glm::scale(GLMModel, glm::vec3(Actor.Scale.GetX(), Actor.Scale.GetY(), Actor.Scale.GetZ()));

    for (uint32_t SubModelIndexOpaque : LODModel->OpaqueObjects) {
        LODModel->GL_Meshes[SubModelIndexOpaque].UpdateInstances(1);
        glUniformMatrix4fv(glGetUniformLocation(Shader->ID, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(GLMModel));
        LODModel->GL_Meshes[SubModelIndexOpaque].Draw();
    }

    for (uint32_t SubModelIndexTransparent : LODModel->TransparentObjects) {
        LODModel->GL_Meshes[SubModelIndexTransparent].UpdateInstances(1);
        glUniformMatrix4fv(glGetUniformLocation(Shader->ID, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(GLMModel));
        LODModel->GL_Meshes[SubModelIndexTransparent].Draw();
    }
}

void UIMapView::DrawInstancedActor(BfresFile* Model, std::vector<Actor*>& ActorPtrs)
{
    std::vector<glm::mat4> InstanceMatrices(ActorPtrs.size());

    int Shrinking = 0;
    for (int i = 0; i < ActorPtrs.size(); i++) {
        if ((ActorPtrs[i]->MergedActorParent == UIOutliner::SelectedActor && UIOutliner::SelectedActor != nullptr) || ActorPtrs[i] == UIOutliner::SelectedActor || !Frustum::SphereInFrustum(ActorPtrs[i]->Translate.GetX(), ActorPtrs[i]->Translate.GetY(), ActorPtrs[i]->Translate.GetZ(), ActorPtrs[i]->Model->GetModels()[0].BoundingBoxSphereRadius * std::fmax(std::fmax(std::fmax(1, ActorPtrs[i]->Scale.GetX()), ActorPtrs[i]->Scale.GetY()), ActorPtrs[i]->Scale.GetZ()))) {
            Shrinking++;
            continue;
        }

        glm::mat4 GLMModel = glm::mat4(1.0f); // Identity matrix

        GLMModel = glm::translate(GLMModel, glm::vec3(ActorPtrs[i]->Translate.GetX(), ActorPtrs[i]->Translate.GetY(), ActorPtrs[i]->Translate.GetZ()));

        GLMModel = glm::rotate(GLMModel, glm::radians(ActorPtrs[i]->Rotate.GetZ()), glm::vec3(0.0, 0.0f, 1.0));
        GLMModel = glm::rotate(GLMModel, glm::radians(ActorPtrs[i]->Rotate.GetY()), glm::vec3(0.0f, 1.0, 0.0));
        GLMModel = glm::rotate(GLMModel, glm::radians(ActorPtrs[i]->Rotate.GetX()), glm::vec3(1.0, 0.0f, 0.0));

        GLMModel = glm::scale(GLMModel, glm::vec3(ActorPtrs[i]->Scale.GetX(), ActorPtrs[i]->Scale.GetY(), ActorPtrs[i]->Scale.GetZ()));

        InstanceMatrices[i - Shrinking] = GLMModel;
    }

    if (Shrinking > 0)
        InstanceMatrices.resize(InstanceMatrices.size() - Shrinking);

    BfresFile::LOD* LODModel = &Model->GetModels()[0].LODs[0];

    for (uint32_t SubModelIndexOpaque : LODModel->OpaqueObjects) {
        LODModel->GL_Meshes[SubModelIndexOpaque].UpdateInstances(InstanceMatrices.size());
        LODModel->GL_Meshes[SubModelIndexOpaque].UpdateInstanceMatrix(InstanceMatrices);
        LODModel->GL_Meshes[SubModelIndexOpaque].Draw();
    }

    for (uint32_t SubModelIndexTransparent : LODModel->TransparentObjects) {
        LODModel->GL_Meshes[SubModelIndexTransparent].UpdateInstances(InstanceMatrices.size());
        LODModel->GL_Meshes[SubModelIndexTransparent].UpdateInstanceMatrix(InstanceMatrices);
        LODModel->GL_Meshes[SubModelIndexTransparent].Draw();
    }
}

void UIMapView::SelectActorByClicking(ImVec2 SceneWindowSize, ImVec2 MousePos)
{
    if (!RenderSettings.AllowSelectingActor || !Focused)
        return;
    if (glfwGetMouseButton(Window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && glfwGetMouseButton(Window, GLFW_MOUSE_BUTTON_RIGHT) != GLFW_PRESS && !ImGui::IsAnyItemHovered()) {
        if (!(SceneWindowSize.x > MousePos.x && SceneWindowSize.y > MousePos.y && MousePos.x > 0 && MousePos.y > 0))
            return;
        if (ImGuizmo::IsOver() && UIOutliner::SelectedActor != nullptr)
            return;

        MapViewFramebuffer->Bind();
        glClearColor(ClearColor.x * ClearColor.w, ClearColor.y * ClearColor.w, ClearColor.z * ClearColor.w, ClearColor.w);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        PickingShader->Activate();

        int MergedActorIndex = 0;

        for (int ActorIndex = 0; ActorIndex < ActorMgr::GetActors().size(); ActorIndex++) {
            Actor& CurrentActor = ActorMgr::GetActors()[ActorIndex];
            if (!CurrentActor.Model->IsDefaultModel() && !RenderSettings.RenderVisibleActors)
                continue;
            if (CurrentActor.Model->IsDefaultModel() && !RenderSettings.RenderInvisibleActors && !CurrentActor.IsUMii)
                continue;
            if (CurrentActor.Gyml.ends_with("_Far") && !RenderSettings.RenderFarActors)
                continue;
            if (CurrentActor.Gyml.starts_with("Npc_") && !RenderSettings.RenderNPCs)
                continue;
            if (CurrentActor.Gyml.find("Area") != std::string::npos && CurrentActor.Model->IsDefaultModel() && !RenderSettings.RenderAreas)
                continue;

            int R = (ActorIndex & 0x000000FF) >> 0;
            int G = (ActorIndex & 0x0000FF00) >> 8;
            int B = (ActorIndex & 0x00FF0000) >> 16;

            glUniform4f(glGetUniformLocation(PickingShader->ID, "PickingColor"), R / 255.0f, G / 255.0f, B / 255.0f, 1.0f);

            if (!CurrentActor.IsUMii) {
                glm::mat4 GLMModel = glm::mat4(1.0f); // Identity matrix

                GLMModel = glm::translate(GLMModel, glm::vec3(CurrentActor.Translate.GetX(), CurrentActor.Translate.GetY(), CurrentActor.Translate.GetZ()));

                GLMModel = glm::rotate(GLMModel, glm::radians(CurrentActor.Rotate.GetZ()), glm::vec3(0.0, 0.0f, 1.0));
                GLMModel = glm::rotate(GLMModel, glm::radians(CurrentActor.Rotate.GetY()), glm::vec3(0.0f, 1.0, 0.0));
                GLMModel = glm::rotate(GLMModel, glm::radians(CurrentActor.Rotate.GetX()), glm::vec3(1.0, 0.0f, 0.0));

                GLMModel = glm::scale(GLMModel, glm::vec3(CurrentActor.Scale.GetX(), CurrentActor.Scale.GetY(), CurrentActor.Scale.GetZ()));

                BfresFile::LOD* LODModel = &CurrentActor.Model->GetModels()[0].LODs[0];

                for (Mesh& SubMesh : LODModel->GL_Meshes) {
                    SubMesh.DrawPicking(PickingShader, &CameraView, GLMModel);
                }
            } else {
                CurrentActor.UMiiData.Draw(CurrentActor.Translate, CurrentActor.Rotate, CurrentActor.Scale, PickingShader, true, &CameraView);
            }
        }

        MergedActorIndex = ActorMgr::GetActors().size();

        std::unordered_map<int, Actor*> MergedActorIds;

        for (Actor& MergedActorMain : ActorMgr::GetActors()) {
            for (Actor& MergedActor : MergedActorMain.MergedActorContent) {
                if (!MergedActor.Model->IsDefaultModel() && !RenderSettings.RenderVisibleActors)
                    continue;
                if (MergedActor.Model->IsDefaultModel() && !RenderSettings.RenderInvisibleActors && !MergedActor.IsUMii)
                    continue;
                if (MergedActor.Gyml.ends_with("_Far") && !RenderSettings.RenderFarActors)
                    continue;
                if (MergedActor.Gyml.starts_with("Npc_") && !RenderSettings.RenderNPCs)
                    continue;
                if (MergedActor.Gyml.find("Area") != std::string::npos && MergedActor.Model->IsDefaultModel() && !RenderSettings.RenderAreas)
                    continue;

                int R = (MergedActorIndex & 0x000000FF) >> 0;
                int G = (MergedActorIndex & 0x0000FF00) >> 8;
                int B = (MergedActorIndex & 0x00FF0000) >> 16;

                glm::mat4 GLMModel = glm::mat4(1.0f); // Identity matrix

                GLMModel = glm::translate(GLMModel, glm::vec3(MergedActor.Translate.GetX(), MergedActor.Translate.GetY(), MergedActor.Translate.GetZ()));

                GLMModel = glm::rotate(GLMModel, glm::radians(MergedActor.Rotate.GetZ()), glm::vec3(0.0, 0.0f, 1.0));
                GLMModel = glm::rotate(GLMModel, glm::radians(MergedActor.Rotate.GetY()), glm::vec3(0.0f, 1.0, 0.0));
                GLMModel = glm::rotate(GLMModel, glm::radians(MergedActor.Rotate.GetX()), glm::vec3(1.0, 0.0f, 0.0));

                GLMModel = glm::scale(GLMModel, glm::vec3(MergedActor.Scale.GetX(), MergedActor.Scale.GetY(), MergedActor.Scale.GetZ()));

                BfresFile::LOD* LODModel = &MergedActor.Model->GetModels()[0].LODs[0];

                glUniform4f(glGetUniformLocation(PickingShader->ID, "PickingColor"), R / 255.0f, G / 255.0f, B / 255.0f, 1.0f);

                for (Mesh& SubMesh : LODModel->GL_Meshes) {
                    SubMesh.DrawPicking(PickingShader, &CameraView, GLMModel);
                }
                MergedActorIds.insert({ MergedActorIndex, &MergedActor });
                MergedActorIndex++;
            }
        }

        glReadBuffer(GL_COLOR_ATTACHMENT0);
        unsigned char Data[3];

        glReadPixels((GLint)MousePos.x, (GLint)SceneWindowSize.y - MousePos.y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, Data);

        int PickedActorId = Data[0] + Data[1] * 256 + Data[2] * 256 * 256;

        if (PickedActorId == 4208174 || PickedActorId < 0) // 3024412 = background color
        {
            UIOutliner::SelectActor(nullptr);
        } else {
            if (PickedActorId < ActorMgr::GetActors().size()) {
                UIOutliner::SelectActor(&ActorMgr::GetActors()[PickedActorId]);
            } else {
                UIOutliner::SelectActor(MergedActorIds[PickedActorId]);
            }
        }

        glReadBuffer(GL_NONE);
    }
}

void UIMapView::DrawMapViewWindow()
{
    if (!Open)
        return;

    Focused = ImGui::Begin("Map View", &Open);
    ImGuizmo::Enable(RenderSettings.AllowSelectingActor);
    ImGuizmo::SetDrawlist();

    const float WindowWidth = ImGui::GetContentRegionAvail().x;
    const float WindowHeight = ImGui::GetContentRegionAvail().y;

    CameraView.Width = WindowWidth;
    CameraView.Height = WindowHeight;

    MapViewFramebuffer->RescaleFramebuffer(WindowWidth, WindowHeight);

    ImVec2 Pos = ImGui::GetCursorScreenPos();
    CameraView.WindowHovered = (ImGui::IsWindowHovered() && !ImGui::IsItemHovered());

    SelectActorByClicking(ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y), ImVec2(ImGui::GetMousePos().x - ImGui::GetWindowPos().x - ImGui::GetWindowContentRegionMin().x, ImGui::GetMousePos().y - ImGui::GetWindowPos().y - ImGui::GetWindowContentRegionMin().y));

    MapViewFramebuffer->Bind();
    glViewport(0, 0, WindowWidth, WindowHeight);
    DefaultShader->Activate();
    glClearColor(ClearColor.x * ClearColor.w, ClearColor.y * ClearColor.w, ClearColor.z * ClearColor.w, ClearColor.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    CameraView.Inputs(ImGui::GetIO().Framerate, ImGui::GetWindowPos());
    CameraView.UpdateMatrix(45.0f, 0.1f, 30000.0f);
    CameraView.Matrix(DefaultShader, "camMatrix");
    CameraView.Matrix(PickingShader, "camMatrix");
    Frustum::CalculateFrustum(&CameraView);

    InstancedShader->Activate();
    CameraView.Matrix(InstancedShader, "camMatrix");

    for (auto& [Model, ActorPtrs] : ActorMgr::OpaqueActors) {
        if (ActorPtrs[0]->IsUMii)
            continue;
        if (!Model->IsDefaultModel() && !RenderSettings.RenderVisibleActors)
            continue;
        if (Model->IsDefaultModel() && !RenderSettings.RenderInvisibleActors)
            continue;
        if (ActorPtrs[0]->Gyml.ends_with("_Far") && !RenderSettings.RenderFarActors)
            continue;
        if (ActorPtrs[0]->Gyml.starts_with("Npc_") && !RenderSettings.RenderNPCs)
            continue;
        if (ActorPtrs[0]->Gyml.find("Area") != std::string::npos && Model->IsDefaultModel() && !RenderSettings.RenderAreas)
            continue;

        DrawInstancedActor(Model, ActorPtrs);
    }

    for (auto& [Model, ActorPtrs] : ActorMgr::TransparentActors) {
        if (ActorPtrs[0]->IsUMii)
            continue;
        if (Model->IsDefaultModel())
            continue;
        if (!Model->IsDefaultModel() && !RenderSettings.RenderVisibleActors)
            continue;
        if (ActorPtrs[0]->Gyml.starts_with("Npc_") && !RenderSettings.RenderNPCs)
            continue;
        if (ActorPtrs[0]->Gyml.ends_with("_Far") && !RenderSettings.RenderFarActors)
            continue;

        DrawInstancedActor(Model, ActorPtrs);
    }

    for (auto& [Model, ActorPtrs] : ActorMgr::TransparentActors) {
        if (ActorPtrs[0]->IsUMii)
            continue;
        if (!Model->IsDefaultModel())
            continue;
        if (Model->IsDefaultModel() && !RenderSettings.RenderInvisibleActors)
            continue;
        if (ActorPtrs[0]->Gyml.ends_with("_Far") && !RenderSettings.RenderFarActors)
            continue;
        if (ActorPtrs[0]->Gyml.starts_with("Npc_") && !RenderSettings.RenderNPCs)
            continue;
        if (ActorPtrs[0]->Gyml.find("Area") != std::string::npos && Model->IsDefaultModel() && !RenderSettings.RenderAreas)
            continue;

        DrawInstancedActor(Model, ActorPtrs);
    }

    if (RenderSettings.RenderNPCs) {
        DefaultShader->Activate();
        for (Actor* NPC : ActorMgr::UMiiActors) {
            if (NPC == UIOutliner::SelectedActor)
                continue;
            NPC->UMiiData.Draw(NPC->Translate, NPC->Rotate, NPC->Scale, DefaultShader);
        }
    }

    // Selected actor
    if (UIOutliner::SelectedActor != nullptr) {
        SelectedShader->Activate();
        CameraView.Matrix(SelectedShader, "camMatrix");
        if (!UIOutliner::SelectedActor->IsUMii) {
            DrawActor(*UIOutliner::SelectedActor, SelectedShader);

            for (Actor& Child : UIOutliner::SelectedActor->MergedActorContent) {
                DrawActor(Child, SelectedShader);
            }
        } else {
            UIOutliner::SelectedActor->UMiiData.Draw(UIOutliner::SelectedActor->Translate, UIOutliner::SelectedActor->Rotate, UIOutliner::SelectedActor->Scale, SelectedShader);
        }
    }

    // DefaultShader->Activate();
    // HeightMapFile.Draw(DefaultShader);

    ImGui::GetWindowDrawList()->AddImage(
        (void*)MapViewFramebuffer->GetFrameTexture(),
        ImVec2(Pos.x, Pos.y),
        ImVec2(Pos.x + WindowWidth, Pos.y + WindowHeight),
        ImVec2(0, 1),
        ImVec2(1, 0));

    if (UIOutliner::SelectedActor != nullptr) {
        ImGuizmo::RecomposeMatrixFromComponents(UIOutliner::SelectedActor->Translate.GetRawData(), UIOutliner::SelectedActor->Rotate.GetRawData(), UIOutliner::SelectedActor->Scale.GetRawData(), ObjectMatrix);

        ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, ImGui::GetWindowWidth(), ImGui::GetWindowHeight());

        ImGuizmo::Manipulate(glm::value_ptr(CameraView.GetViewMatrix()), glm::value_ptr(CameraView.GetProjectionMatrix()), ImGuizmoOperation, ImGuizmoMode, ObjectMatrix);
    }

    if (ImGuizmo::IsUsingAny()) {
        Actor OldActor = *UIOutliner::SelectedActor;
        ImGuizmo::DecomposeMatrixToComponents(ObjectMatrix, UIOutliner::SelectedActor->Translate.GetRawData(), UIOutliner::SelectedActor->Rotate.GetRawData(), UIOutliner::SelectedActor->Scale.GetRawData());
        ActorMgr::UpdateMergedActorContent(UIOutliner::SelectedActor, OldActor);
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