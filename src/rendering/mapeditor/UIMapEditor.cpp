#include "UIMapEditor.h"

#include "imgui_internal.h"
#include "imgui_stdlib.h"
#include <manager/UIMgr.h>
#include <util/FileUtil.h>
#include <manager/ShaderMgr.h>
#include <rendering/ImGuiExt.h>
#include <manager/BfresFileMgr.h>
#include <manager/PopUpMgr.h>
#include <manager/MergedActorMgr.h>
#include <manager/ActorPackMgr.h>
#include <manager/ProjectMgr.h>
#include <util/portable-file-dialogs.h>
#include <util/Math.h>
#include <util/GitManager.h>
#include <tool/SceneCreator.h>
#include <tool/scene/static_compound/StaticCompoundImplementationSingleScene.h>
#include <file/game/zstd/ZStdBackend.h>
#include <util/ImGuiNotify.h>

namespace application::rendering::map_editor
{
    application::gl::Shader* UIMapEditor::gInstancedShader = nullptr;
    application::gl::Shader* UIMapEditor::gPickingShader = nullptr;
    application::gl::Shader* UIMapEditor::gSelectedShader = nullptr;
    application::gl::Shader* UIMapEditor::gNavMeshShader = nullptr;

    application::rendering::popup::PopUpBuilder UIMapEditor::gAddNewDynamicTypeParameterPopUp;
    application::rendering::popup::PopUpBuilder UIMapEditor::gAddBancEntityPopUp;
    application::rendering::popup::PopUpBuilder UIMapEditor::gAddLinkPopUp;
    application::rendering::popup::PopUpBuilder UIMapEditor::gAddRailPopUp;
    application::rendering::popup::PopUpBuilder UIMapEditor::gLoadScenePopUp;
    application::rendering::popup::PopUpBuilder UIMapEditor::gNewScenePopUp;
    application::rendering::popup::PopUpBuilder UIMapEditor::gAddFarActorPopUp;
    application::rendering::popup::PopUpBuilder UIMapEditor::gAddBancEntityToParentPopUp;
    application::rendering::popup::PopUpBuilder UIMapEditor::gConfirmActorDeletePopUp;
    application::rendering::popup::PopUpBuilder UIMapEditor::gConfigureGitIdentityPopUp;

    glm::vec3 UIMapEditor::gLightColor = glm::vec3(1.0f, 1.0f, 1.0f);

    void UIMapEditor::InitializeGlobal()
    {
        gAddNewDynamicTypeParameterPopUp
            .Title("Dynamic Type Parameter")
            .Width(500.0f)
            .Flag(ImGuiWindowFlags_NoResize)
            .NeedsConfirmation(false)
            .AddDataStorage(sizeof(uint32_t))
            .AddDataStorageInstanced<std::string>([](void* Ptr) { *reinterpret_cast<std::string*>(Ptr) = ""; })
            .AddDataStorageInstanced<std::variant<std::string, bool, int32_t, int64_t, uint32_t, uint64_t, float, glm::vec3>>([](void* Ptr) { *reinterpret_cast<std::variant<std::string, bool, int32_t, int64_t, uint32_t, uint64_t, float, glm::vec3>*>(Ptr) = ""; })
            .ContentDrawingFunction([](popup::PopUpBuilder& Builder)
            {
                if (ImGui::BeginTable("DynamicTypeParameterPopUpTable", 2, ImGuiTableFlags_BordersInnerV))
                {
                    ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("Value").x);

                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Type");
                    ImGui::TableNextColumn();
                    ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
                    static const char* DataTypes[]{ "String", "Bool", "S32", "S64", "U32", "U64", "Float", "Vec3f" };
                    if (ImGui::Combo("##Data type", reinterpret_cast<int*>(Builder.GetDataStorage(0).mPtr), DataTypes, IM_ARRAYSIZE(DataTypes)))
                    {
                        std::variant<std::string, bool, int32_t, int64_t, uint32_t, uint64_t, float, glm::vec3>& Value = *reinterpret_cast<std::variant<std::string, bool, int32_t, int64_t, uint32_t, uint64_t, float, glm::vec3>*>(Builder.GetDataStorage(2).mPtr);
                        switch (*reinterpret_cast<int*>(Builder.GetDataStorage(0).mPtr))
                        {
                        case 0:
                            Value = "";
                            break;
                        case 1:
                            Value = false;
                            break;
                        case 2:
                            Value = (int32_t)0;
                            break;
                        case 3:
                            Value = (int64_t)0;
                            break;
                        case 4:
                            Value = (uint32_t)0;
                            break;
                        case 5:
                            Value = (uint64_t)0;
                            break;
                        case 6:
                            Value = (float)0;
                            break;
                        case 7:
                            Value = glm::vec3(0.0f, 0.0f, 0.0f);
                            break;
                        default:
                            application::util::Logger::Error("UIMapEditor", "Invalid dynamic type parameter data type");
                            break;
                        }
                    }
                    ImGui::PopItemWidth();
                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Name");
                    ImGui::TableNextColumn();
                    ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
                    ImGui::InputText("##Name", reinterpret_cast<std::string*>(Builder.GetDataStorage(1).mPtr));
                    ImGui::PopItemWidth();
                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Value");
                    ImGui::TableNextColumn();
                    ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
                    std::variant<std::string, bool, int32_t, int64_t, uint32_t, uint64_t, float, glm::vec3>& Value = *reinterpret_cast<std::variant<std::string, bool, int32_t, int64_t, uint32_t, uint64_t, float, glm::vec3>*>(Builder.GetDataStorage(2).mPtr);
                    switch (*reinterpret_cast<int*>(Builder.GetDataStorage(0).mPtr))
                    {
                    case 0:
                        ImGui::InputText("##Value", &std::get<std::string>(Value));
                        break;
                    case 1:
                        ImGui::Checkbox("##Value", &std::get<bool>(Value));
                        break;
                    case 2:
                        ImGui::InputScalar("##Value", ImGuiDataType_S32, &std::get<int32_t>(Value));
                        break;
                    case 3:
                        ImGui::InputScalar("##Value", ImGuiDataType_S64, &std::get<int64_t>(Value));
                        break;
                    case 4:
                        ImGui::InputScalar("##Value", ImGuiDataType_U32, &std::get<uint32_t>(Value));
                        break;
                    case 5:
                        ImGui::InputScalar("##Value", ImGuiDataType_U64, &std::get<uint64_t>(Value));
                        break;
                    case 6:
                        ImGui::InputScalar("##Value", ImGuiDataType_Float, &std::get<float>(Value));
                        break;
                    case 7:
                        ImGui::InputFloat3("##Value", &std::get<glm::vec3>(Value).x);
                        break;
                    default:
                        application::util::Logger::Error("UIMapEditor", "Invalid dynamic type parameter data type at render stage");
                        break;
                    }
                    ImGui::PopItemWidth();
                    ImGui::TableNextRow();

                    ImGui::EndTable();
                }
            }).Register();

            gAddBancEntityPopUp
                .Title("Add BancEntity")
                .Width(500.0f)
                .Flag(ImGuiWindowFlags_NoResize)
                .NeedsConfirmation(false)
                .AddDataStorage(sizeof(uint32_t))
                .AddDataStorageInstanced<std::string>([](void* Ptr) { *reinterpret_cast<std::string*>(Ptr) = ""; })
                .AddDataStorageInstanced<std::string>([](void* Ptr) { *reinterpret_cast<std::string*>(Ptr) = ""; })
                .ContentDrawingFunction([](popup::PopUpBuilder& Builder)
                    {
                        if (ImGui::BeginTable("BancEntityTable", 2, ImGuiTableFlags_BordersInnerV))
                        {
                            ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("BancPath").x);

                            ImGui::TableNextRow();

                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("Type");
                            ImGui::TableNextColumn();
                            ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
                            static const char* DataTypes[]{ "Static", "Dynamic", "Merged" };
                            ImGui::Combo("##Data type", reinterpret_cast<int*>(Builder.GetDataStorage(0).mPtr), DataTypes, IM_ARRAYSIZE(DataTypes));
                            ImGui::PopItemWidth();
                            ImGui::TableNextRow();

                            //If not Merged
                            if (*reinterpret_cast<uint32_t*>(Builder.GetDataStorage(0).mPtr) != 2)
                            {
                                ImGui::BeginDisabled();
                            }

                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("BancPath");
                            ImGui::TableNextColumn();
                            ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
                            ImGui::InputText("##BancPath", reinterpret_cast<std::string*>(Builder.GetDataStorage(2).mPtr));
                            ImGui::PopItemWidth();
                            ImGui::TableNextRow();

                            //If not Merged
                            if (*reinterpret_cast<uint32_t*>(Builder.GetDataStorage(0).mPtr) != 2)
                            {
                                ImGui::EndDisabled();
                            }

                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("Name");
                            ImGui::TableNextColumn();
                            ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
                            ImGui::InputText("##Name", reinterpret_cast<std::string*>(Builder.GetDataStorage(1).mPtr));
                            ImGui::PopItemWidth();

                            ImGui::EndTable();

                            ImGui::PushItemWidth(ImGui::GetWindowWidth() - ImGui::GetStyle().FramePadding.x * 2.0f);
                            if (ImGui::BeginListBox("##ActorPackFilesAddActor"))
                            {
                                std::string ActorPackFilterLower = *reinterpret_cast<std::string*>(Builder.GetDataStorage(1).mPtr);
                                std::transform(ActorPackFilterLower.begin(), ActorPackFilterLower.end(), ActorPackFilterLower.begin(),
                                    [](unsigned char c) { return std::tolower(c); });

                                auto DisplayActorFile = [&ActorPackFilterLower, &Builder](const std::string& Name, const std::string& LowerName) 
                                    {
                                        if (ActorPackFilterLower.length() > 0)
                                        {
                                            if (LowerName.find(ActorPackFilterLower) == std::string::npos)
                                                return;
                                        }

                                        if (ImGui::Selectable(Name.c_str()))
                                        {
                                            *reinterpret_cast<std::string*>(Builder.GetDataStorage(1).mPtr) = Name;
                                        }
                                    };

                                for (size_t i = 0; i < application::manager::ActorPackMgr::gActorList.size(); i++)
                                {
                                    DisplayActorFile(application::manager::ActorPackMgr::gActorList[i], application::manager::ActorPackMgr::gActorListLowerCase[i]);
                                }

                                ImGui::EndListBox();
                            }
                            ImGui::PopItemWidth();
                        }
                    }).Register();

            gAddLinkPopUp
                .Title("Add Link")
                .Width(500.0f)
                .Flag(ImGuiWindowFlags_NoResize)
                .NeedsConfirmation(false)
                .AddDataStorage(sizeof(uint64_t))
                .AddDataStorage(sizeof(uint64_t))
                .AddDataStorageInstanced<std::string>([](void* Ptr) { *reinterpret_cast<std::string*>(Ptr) = ""; })
                .AddDataStorageInstanced<std::string>([](void* Ptr) { *reinterpret_cast<std::string*>(Ptr) = ""; })
                .ContentDrawingFunction([](popup::PopUpBuilder& Builder)
                    {
                        if (ImGui::BeginTable("LinkTable", 2, ImGuiTableFlags_BordersInnerV))
                        {
                            ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("Destination").x);

                            ImGui::TableNextRow();

                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("Source");
                            ImGui::TableNextColumn();
                            ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
                            ImGui::InputScalar("##Source", ImGuiDataType_U64, reinterpret_cast<uint64_t*>(Builder.GetDataStorage(0).mPtr));
                            ImGui::PopItemWidth();
                            ImGui::TableNextRow();

                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("Destination");
                            ImGui::TableNextColumn();
                            ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
                            ImGui::InputScalar("##TableSetColumnIndex", ImGuiDataType_U64, reinterpret_cast<uint64_t*>(Builder.GetDataStorage(1).mPtr));
                            ImGui::PopItemWidth();
                            ImGui::TableNextRow();

                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("Gyml");
                            ImGui::TableNextColumn();
                            ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
                            ImGui::InputText("##GymlLinkTable", reinterpret_cast<std::string*>(Builder.GetDataStorage(2).mPtr));
                            ImGui::PopItemWidth();
                            ImGui::TableNextRow();

                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("Name");
                            ImGui::TableNextColumn();
                            ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
                            ImGui::InputText("##NameLinkTable", reinterpret_cast<std::string*>(Builder.GetDataStorage(3).mPtr));
                            ImGui::PopItemWidth();

                            ImGui::EndTable();
                        }
                    }).Register();

            gAddRailPopUp
                .Title("Add Rail")
                .Width(500.0f)
                .Flag(ImGuiWindowFlags_NoResize)
                .NeedsConfirmation(false)
                .AddDataStorage(sizeof(uint64_t))
                .AddDataStorageInstanced<std::string>([](void* Ptr) { *reinterpret_cast<std::string*>(Ptr) = ""; })
                .AddDataStorageInstanced<std::string>([](void* Ptr) { *reinterpret_cast<std::string*>(Ptr) = ""; })
                .ContentDrawingFunction([](popup::PopUpBuilder& Builder)
                    {
                        if (ImGui::BeginTable("RailTable", 2, ImGuiTableFlags_BordersInnerV))
                        {
                            ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("Destination").x);

                            ImGui::TableNextRow();

                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("Destination");
                            ImGui::TableNextColumn();
                            ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
                            ImGui::InputScalar("##TableSetColumnIndex", ImGuiDataType_U64, reinterpret_cast<uint64_t*>(Builder.GetDataStorage(0).mPtr));
                            ImGui::PopItemWidth();
                            ImGui::TableNextRow();

                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("Gyml");
                            ImGui::TableNextColumn();
                            ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
                            ImGui::InputText("##GymlLinkTable", reinterpret_cast<std::string*>(Builder.GetDataStorage(1).mPtr));
                            ImGui::PopItemWidth();
                            ImGui::TableNextRow();

                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("Name");
                            ImGui::TableNextColumn();
                            ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
                            ImGui::InputText("##NameLinkTable", reinterpret_cast<std::string*>(Builder.GetDataStorage(2).mPtr));
                            ImGui::PopItemWidth();

                            ImGui::EndTable();
                        }
                    }).Register();

            gLoadScenePopUp
                .Title("Load scene")
                .Width(500.0f)
                .Flag(ImGuiWindowFlags_NoResize)
                .NeedsConfirmation(false)
                .AddDataStorage(sizeof(uint32_t))
                .AddDataStorageInstanced<std::string>([](void* Ptr) { *reinterpret_cast<std::string*>(Ptr) = ""; })
                .ContentDrawingFunction([](popup::PopUpBuilder& Builder)
                    {
                        if (ImGui::BeginTable("LoadSceneTable", 2, ImGuiTableFlags_BordersInnerV))
                        {
                            ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("Identifier").x);

                            ImGui::TableNextRow();

                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("Type");
                            ImGui::TableNextColumn();
                            ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
                            if (application::manager::ProjectMgr::gIsTrialOfTheChosenHeroProject)
                            {
                                static const char* TrialTypes[] = { "SmallDungeon", "LargeDungeon", "MainField", "MinusField", "Sky", "NormalStage", "TrialField" };
                                ImGui::Combo("##Type", reinterpret_cast<int*>(Builder.GetDataStorage(0).mPtr), TrialTypes, IM_ARRAYSIZE(TrialTypes));
                            }
                            else
                            {
                                static const char* Types[] = { "SmallDungeon", "LargeDungeon", "MainField", "MinusField", "Sky", "NormalStage" };
                                ImGui::Combo("##Type", reinterpret_cast<int*>(Builder.GetDataStorage(0).mPtr), Types, IM_ARRAYSIZE(Types));
                            }
                            ImGui::PopItemWidth();
                            ImGui::TableNextRow();

                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("Identifier");
                            ImGui::TableNextColumn();
                            ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
                            ImGui::InputText("##Identifier", reinterpret_cast<std::string*>(Builder.GetDataStorage(1).mPtr));
                            ImGui::PopItemWidth();

                            ImGui::EndTable();
                        }
                    }).Register();

            gNewScenePopUp
                .Title("New scene")
                .Width(500.0f)
                .Flag(ImGuiWindowFlags_NoResize)
                .NeedsConfirmation(false)
                .AddDataStorageInstanced<std::string>([](void* Ptr) { *reinterpret_cast<std::string*>(Ptr) = ""; })
                .ContentDrawingFunction([](popup::PopUpBuilder& Builder)
                    {
                        if (ImGui::BeginTable("NewSceneTable", 2, ImGuiTableFlags_BordersInnerV))
                        {
                            ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("Identifier").x);

                            ImGui::TableNextRow();

                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("Identifier");
                            ImGui::TableNextColumn();
                            ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
                            ImGui::InputText("##Identifier", reinterpret_cast<std::string*>(Builder.GetDataStorage(0).mPtr));
                            ImGui::PopItemWidth();

                            ImGui::EndTable();
                        }
                    }).Register();

            gAddFarActorPopUp
                .Title("Add Far BancEntity")
                .Width(500.0f)
                .Flag(ImGuiWindowFlags_NoResize)
                .NeedsConfirmation(false)
                .AddDataStorageInstanced<std::string>([](void* Ptr) { *reinterpret_cast<std::string*>(Ptr) = ""; })
                .ContentDrawingFunction([](popup::PopUpBuilder& Builder)
                    {
                        if (ImGui::BeginTable("AddFarBancEntityTable", 2, ImGuiTableFlags_BordersInnerV))
                        {
                            ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("Gyml").x);

                            ImGui::TableNextRow();

                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("Gyml");
                            ImGui::TableNextColumn();
                            ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
                            ImGui::InputText("##FarActorGyml", reinterpret_cast<std::string*>(Builder.GetDataStorage(0).mPtr));
                            ImGui::PopItemWidth();

                            ImGui::EndTable();
                        }
                    }).Register();

            gAddBancEntityToParentPopUp
                .Title("Link to parent")
                .Width(500.0f)
                .Flag(ImGuiWindowFlags_NoResize)
                .NeedsConfirmation(false)
                .AddDataStorageInstanced<std::string>([](void* Ptr) { *reinterpret_cast<std::string*>(Ptr) = ""; })
                .ContentDrawingFunction([](popup::PopUpBuilder& Builder)
                    {
                        if (ImGui::BeginTable("LinkToParentTable", 2, ImGuiTableFlags_BordersInnerV))
                        {
                            ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("BancPath").x);

                            ImGui::TableNextRow();

                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("BancPath");
                            ImGui::TableNextColumn();
                            ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
                            ImGui::InputText("##LinkToParentBancPath", reinterpret_cast<std::string*>(Builder.GetDataStorage(0).mPtr));
                            ImGui::PopItemWidth();

                            ImGui::EndTable();
                        }
                    }).Register();

            gConfirmActorDeletePopUp
                .Title("Confirm BancEntity deletion")
                .Width(500.0f)
                .Flag(ImGuiWindowFlags_NoResize)
                .NeedsConfirmation(false)
                .ContentDrawingFunction([](popup::PopUpBuilder& Builder)
                    {
                        ImGui::Text("Please confirm the BancEntity deletion.");
                    }).Register();

            gConfigureGitIdentityPopUp
                .Title("Configure Git Identity")
                .Width(500.0f)
                .Flag(ImGuiWindowFlags_NoResize)
                .NeedsConfirmation(false)
                .ContentDrawingFunction([](popup::PopUpBuilder& Builder)
                    {
                        ImGui::Text("Configure your Git identity:");

                        ImGui::InputText("Name", &application::util::GitManager::gUserIdentity.mName);
                        ImGui::InputText("E-Mail", &application::util::GitManager::gUserIdentity.mEmail);
                        ImGui::InputText("Access Token", &application::util::GitManager::gUserIdentity.mAccessToken);

                    }).Register();

        application::rendering::map_editor::UIAILinks::Initialize();
        application::rendering::map_editor::ActorPainter::Initialize();
        application::rendering::map_editor::TerrainEditor::Initialize();
    }

    void UIMapEditor::InitializeImpl()
    {
        if (gInstancedShader == nullptr)
            gInstancedShader = application::manager::ShaderMgr::GetShader("Instanced");
        if (gPickingShader == nullptr)
            gPickingShader = application::manager::ShaderMgr::GetShader("Picking");
        if (gSelectedShader == nullptr)
            gSelectedShader = application::manager::ShaderMgr::GetShader("Selected");
        if (gNavMeshShader == nullptr)
            gNavMeshShader = application::manager::ShaderMgr::GetShader("NavMesh");

        mSceneWindowFramebuffer = application::manager::FramebufferMgr::CreateFramebuffer(1, 1, "SceneView_" + std::to_string(mWindowId));
        mCamera.mWindow = application::manager::UIMgr::gWindow;
    }

    void UIMapEditor::DrawOutlinerWindow()
    {
        ImGui::Begin(CreateID("Outliner").c_str());

        ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.75f - ImGui::GetStyle().ItemSpacing.x - ImGui::GetStyle().WindowPadding.x);
        ImGui::InputTextWithHint("##SearchOutliner", "Search...", &mOutlierSearchFieldText);
        ImGui::PopItemWidth();

        ImGui::SameLine();

        if (ImGui::Button("Add", ImVec2(ImGui::GetWindowWidth() * 0.25f - ImGui::GetStyle().WindowPadding.x, 0)))
        {
            gAddBancEntityPopUp.Open([this](application::rendering::popup::PopUpBuilder& Builder)
                {
                    //Static & Dynamic
                    if (*reinterpret_cast<uint32_t*>(Builder.GetDataStorage(0).mPtr) != 2)
                    {
                        application::game::Scene::BancEntityRenderInfo* EntityInfo = mScene.AddBancEntity(*reinterpret_cast<std::string*>(Builder.GetDataStorage(1).mPtr));

                        if (EntityInfo == nullptr)
                            return;

                        EntityInfo->mTranslate = mCamera.mPosition + 4.0f * mCamera.mOrientation;

                        EntityInfo->mEntity->mBancType = static_cast<application::game::BancEntity::BancType>(*reinterpret_cast<uint32_t*>(Builder.GetDataStorage(0).mPtr));

                        EntityInfo->mEntity->mPhive.mPlacement["ID"] = EntityInfo->mEntity->mHash;

                        mScene.SyncBancEntity(EntityInfo);
                        SelectActor(EntityInfo->mEntityIndex);
                        return;
                    }
                    
                    //Merged
                    std::optional<application::game::BancEntity> EntityOpt = mScene.CreateBancEntity(*reinterpret_cast<std::string*>(Builder.GetDataStorage(1).mPtr));

                    if (!EntityOpt.has_value())
                        return;

                    application::game::BancEntity& Entity = EntityOpt.value();
                    Entity.mBancType = static_cast<application::game::BancEntity::BancType>(*reinterpret_cast<uint32_t*>(Builder.GetDataStorage(0).mPtr));
                    Entity.mPhive.mPlacement["ID"] = Entity.mHash;
                    
                    std::vector<application::game::BancEntity>* MergedActor = application::manager::MergedActorMgr::GetMergedActor(*reinterpret_cast<std::string*>(Builder.GetDataStorage(2).mPtr), true);
                    if (MergedActor == nullptr)
                        return;

                    MergedActor->push_back(Entity);
                    mScene.GenerateDrawList();

                    //Select nearest instance of new merged actor relative to camera
                    SelectNearestMergedBancEntityInstance(MergedActor, Entity, mCamera.mPosition);
                });
        }
        
        std::vector<application::game::Scene::BancEntityRenderInfo*> RenderInfoCopy;
        RenderInfoCopy.reserve(mScene.mDrawListRenderInfoIndices.size());

        for (auto& RenderInfo : mScene.mDrawListRenderInfoIndices)
        {
            if (!mOutlierSearchFieldText.empty() && RenderInfo->mEntity->mGyml.find(mOutlierSearchFieldText) == std::string::npos)
                continue;

            RenderInfoCopy.push_back(RenderInfo);
        }

        ImGuiListClipper Clipper;
        Clipper.Begin(RenderInfoCopy.size());

        while (Clipper.Step())
        {
            for (int i = Clipper.DisplayStart; i < Clipper.DisplayEnd; ++i)
            {
                application::game::Scene::BancEntityRenderInfo* RenderInfo = RenderInfoCopy[i];
                if (!mOutlierSearchFieldText.empty() && RenderInfo->mEntity->mGyml.find(mOutlierSearchFieldText) == std::string::npos)
                    continue;

                if (ImGui::Selectable((RenderInfo->mEntity->mGyml + "##" + std::to_string(i)).c_str(), mSelectedActorIndex == RenderInfo->mEntityIndex))
                {
                    SelectActor(RenderInfo->mEntityIndex);
                }
            }
        }

        /*
        uint32_t Index = 0;
        for (application::game::Scene::BancEntityRenderInfo* RenderInfo : mScene.mDrawListRenderInfoIndices)
        {
            if (!mOutlierSearchFieldText.empty() && RenderInfo->mEntity->mGyml.find(mOutlierSearchFieldText) == std::string::npos)
            {
                continue;
            }

            if (ImGui::Selectable((RenderInfo->mEntity->mGyml + "##" + std::to_string(Index++)).c_str()))
            {
                SelectActor(RenderInfo->mEntityIndex);
            }
        }
        */

        ImGui::End();
    }

    void UIMapEditor::DrawToolsWindow()
    {
        ImGui::Begin(CreateID("Tools").c_str());

        if (mScene.mSceneType == application::game::Scene::SceneType::NONE)
        {
            ImGui::End();
            return;
        }

        ImGui::Checkbox("Render Visible Entities", &mRenderSettings.mRenderVisibleEntities);

        ImGui::Indent();
        if (!mRenderSettings.mRenderVisibleEntities)
            ImGui::BeginDisabled();
        ImGui::Checkbox("Render Far Entities", &mRenderSettings.mRenderFarEntities);
        if (!mRenderSettings.mRenderVisibleEntities)
            ImGui::EndDisabled();
        ImGui::Unindent();

        ImGui::Checkbox("Render Invisible Entities", &mRenderSettings.mRenderInvisibleEntities);

        ImGui::Indent();
        if (!mRenderSettings.mRenderInvisibleEntities)
            ImGui::BeginDisabled();
        ImGui::Checkbox("Render Areas", &mRenderSettings.mRenderAreas);
        if (!mRenderSettings.mRenderInvisibleEntities)
            ImGui::EndDisabled();
        ImGui::Unindent();

        ImGui::Checkbox("Render Merged Actors", &mRenderSettings.mRenderMergedActors);

        ImGui::Checkbox("Render Terrain", &mRenderSettings.mRenderTerrain);
        ImGui::Indent();
        ImGui::BeginDisabled(!mRenderSettings.mRenderTerrain);
        ImGui::Checkbox("Wireframe##Terrain", &mRenderSettings.mRenderTerrainWireframe);
        if(mScene.mTerrainRenderer) ImGui::Checkbox("Tessellation##Terrain", &mScene.mTerrainRenderer->mEnableTessellation);
        ImGui::EndDisabled();
        ImGui::Unindent();

        ImGui::Checkbox("Render Terrain Collision", &mRenderSettings.mRenderTerrainCollision);
        ImGui::Indent();
        ImGui::BeginDisabled(!mRenderSettings.mRenderTerrainCollision);
        ImGui::Checkbox("Wireframe##TerrainCollision", &mRenderSettings.mRenderTerrainCollisionWireframe);
        ImGui::EndDisabled();
        ImGui::Unindent();

        if (!mScene.mNavMeshImplementation->SupportNavMeshViewing())
        {
            ImGui::BeginDisabled();
        }

        ImGui::Checkbox("Render NavMesh", &mRenderSettings.mRenderNavMesh);
        ImGui::Indent();
        ImGui::BeginDisabled(!mRenderSettings.mRenderNavMesh);
        ImGui::Checkbox("Wireframe##NavMesh", &mRenderSettings.mRenderNavMeshWireframe);
        ImGui::EndDisabled();
        ImGui::Unindent();

        if (!mScene.mNavMeshImplementation->SupportNavMeshViewing())
        {
            ImGui::EndDisabled();
        }

        if (!mScene.mNavMeshImplementation->SupportNavMeshGeneration())
        {
            ImGui::BeginDisabled();
        }

        ImGui::NewLine();

        if (ImGui::CollapsingHeader("NavMesh"))
        {
            mScene.mNavMeshImplementation->DrawImGuiGeneratorMenu();
        }

        if (!mScene.mNavMeshImplementation->SupportNavMeshGeneration())
        {
            ImGui::EndDisabled();
        }

        ImGui::Checkbox("Generate physics hashes", &mScene.mGeneratePhysicsHashes);

        if (ImGui::Button("Regenerate scene draw list"))
            mScene.GenerateDrawList();

        ImGui::BeginDisabled(mScene.IsTerrainScene() || mSelectedActorIndex == -1);
        if (ImGui::Button("StartPos to selected BancEntity"))
        {
            application::file::game::byml::BymlFile StartPosByml(application::file::game::ZStdBackend::Decompress(application::util::FileUtil::GetRomFSFilePath("Banc/" + mScene.GetSceneTypeName(mScene.mSceneType) + "/StartPos/" + mScene.GetSceneTypeName(mScene.mSceneType) + ".startpos.byml.zs")));

            if (StartPosByml.GetNodes()[0].HasChild(mScene.mBcettName))
            {
                application::game::Scene::BancEntityRenderInfo* RenderInfo = GetSelectedActor();

                StartPosByml.GetNodes()[0].GetChild(mScene.mBcettName)->GetChild("Trans")->GetChild(0)->SetValue<float>(RenderInfo->mTranslate.x);
                StartPosByml.GetNodes()[0].GetChild(mScene.mBcettName)->GetChild("Trans")->GetChild(1)->SetValue<float>(RenderInfo->mTranslate.y);
                StartPosByml.GetNodes()[0].GetChild(mScene.mBcettName)->GetChild("Trans")->GetChild(2)->SetValue<float>(RenderInfo->mTranslate.z);

                StartPosByml.GetNodes()[0].GetChild(mScene.mBcettName)->GetChild("Rot")->GetChild(0)->SetValue<float>(glm::radians(RenderInfo->mRotate.x));
                StartPosByml.GetNodes()[0].GetChild(mScene.mBcettName)->GetChild("Rot")->GetChild(1)->SetValue<float>(glm::radians(RenderInfo->mRotate.y));
                StartPosByml.GetNodes()[0].GetChild(mScene.mBcettName)->GetChild("Rot")->GetChild(2)->SetValue<float>(glm::radians(RenderInfo->mRotate.z));
            }

            std::filesystem::create_directories(application::util::FileUtil::GetSaveFilePath("Banc/" + mScene.GetSceneTypeName(mScene.mSceneType) + "/StartPos"));
			application::util::FileUtil::WriteFile(application::util::FileUtil::GetSaveFilePath("Banc/" + mScene.GetSceneTypeName(mScene.mSceneType) + "/StartPos/" + mScene.GetSceneTypeName(mScene.mSceneType) + ".startpos.byml.zs"), application::file::game::ZStdBackend::Compress(StartPosByml.ToBinary(application::file::game::byml::BymlFile::TableGeneration::AUTO), application::file::game::ZStdBackend::Dictionary::Base));
        }
        ImGui::EndDisabled();

        //ImGui::BeginDisabled(mScene.mStaticCompoundImplementation == nullptr || mScene.IsTerrainScene());
        if (ImGui::CollapsingHeader("Baked collision"))
        {
            ImGui::Indent();
            if (ImGui::Button("Remove Baked Collision"))
            {
                application::tool::scene::static_compound::StaticCompoundImplementationSingleScene* Compound = dynamic_cast<application::tool::scene::static_compound::StaticCompoundImplementationSingleScene*>(mScene.mStaticCompoundImplementation.get());
                if (Compound != nullptr) Compound->RemoveBakedCollision();
            }
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            {
                ImGui::SetTooltip("By clicking this button all compound tag files in the static compound will be deleted. That way all of the baked collision will be removed.");
            }

            if (ImGui::Button("Remove Ghost Collision"))
            {
                mScene.mStaticCompoundImplementation.get()->CleanupCollision(&mScene);
            }
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            {
                ImGui::SetTooltip("By clicking this button Starlight will search for baked collision without any actor bound to it. The only way to do this is by\nmaking assumptions, so use this feature carefully, sometimes too much baked collision gets degenerated.");
            }

            if (ImGui::Button("Remove Ghost Volume Collision"))
            {
                mScene.mStaticCompoundImplementation.get()->CleanupCollisionVolume(&mScene);
            }
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            {
                ImGui::SetTooltip("By clicking this button Starlight will remove every baked collision inside of an Area actor with the\ndynamic bool property \"Starlight_GhostCollisionVolume\" set to true.");
            }
            ImGui::Unindent();
        }
        //ImGui::EndDisabled();

        ImGui::End();
    }

    void UIMapEditor::DrawDynamicTypePropertiesHeader(const std::string& Title, std::map<std::string, std::variant<std::string, bool, int32_t, int64_t, uint32_t, uint64_t, float, glm::vec3>>& Map, bool& SyncBancEntity, bool& UpdateColumnWidth, ImGuiTreeNodeFlags Flags, int IndentationLevel)
    {
        auto OnAddButtonPress = [&Map, &SyncBancEntity]()
            {
                gAddNewDynamicTypeParameterPopUp.Open([&Map, &SyncBancEntity](application::rendering::popup::PopUpBuilder& Builder)
                    {
                        const std::string& Key = *reinterpret_cast<std::string*>(Builder.GetDataStorage(1).mPtr);
                        if (Map.contains(Key))
                            return;

                        Map.insert({ Key, *reinterpret_cast<std::variant<std::string, bool, int32_t, int64_t, uint32_t, uint64_t, float, glm::vec3>*>(Builder.GetDataStorage(2).mPtr) });
                        SyncBancEntity = true;
                    });
            };

        if (ImGuiExt::CollapsingHeaderWithAddButton(Title.c_str(), Flags, OnAddButtonPress))
        {
            DrawDynamicTypePropertiesTable(Title, Map, SyncBancEntity, UpdateColumnWidth, IndentationLevel);
        }
    }

    void UIMapEditor::DrawDynamicTypePropertiesSeparator(const std::string& Title, std::map<std::string, std::variant<std::string, bool, int32_t, int64_t, uint32_t, uint64_t, float, glm::vec3>>& Map, bool& SyncBancEntity, bool& UpdateColumnWidth, int IndentationLevel)
    {
        auto OnAddButtonPress = [&Map, &SyncBancEntity]()
            {
                gAddNewDynamicTypeParameterPopUp.Open([&Map, &SyncBancEntity](application::rendering::popup::PopUpBuilder& Builder)
                    {
                        const std::string& Key = *reinterpret_cast<std::string*>(Builder.GetDataStorage(1).mPtr);
                        if (Map.contains(Key))
                            return;

                        Map.insert({ Key, *reinterpret_cast<std::variant<std::string, bool, int32_t, int64_t, uint32_t, uint64_t, float, glm::vec3>*>(Builder.GetDataStorage(2).mPtr) });
                        SyncBancEntity = true;
                    });
            };

        ImGui::Text(Title.c_str());
        ImGui::SameLine();
        const float Size = (ImGui::GetCurrentContext()->FontSize + ImGui::GetStyle().FramePadding.y * 2) - ImGui::GetStyle().FramePadding.y * 2.0f;
        if (ImGui::ImageButton(("+##" + Title).c_str(), (ImTextureID)ImGuiExt::gAddButtonTexture->mID, ImVec2(Size, Size), ImVec2(0, 0)))
        {
            OnAddButtonPress();
        }
        ImGui::Separator();
        DrawDynamicTypePropertiesTable(Title, Map, SyncBancEntity, UpdateColumnWidth, IndentationLevel);
    }

    void UIMapEditor::DrawDynamicTypePropertiesTable(const std::string& Title, std::map<std::string, std::variant<std::string, bool, int32_t, int64_t, uint32_t, uint64_t, float, glm::vec3>>& Map, bool& SyncBancEntity, bool& UpdateColumnWidth, int IndentationLevel)
    {
        if (Map.empty())
            return;

        ImGui::Columns(2, (Title + "Table").c_str());

        if (UpdateColumnWidth)
        {
            const std::string* MaxLength = nullptr;

            for (auto& [Key, Val] : Map)
            {
                if (MaxLength == nullptr)
                {
                    MaxLength = &Key;
                    continue;
                }

                if (Key.size() > MaxLength->size())
                    MaxLength = &Key;
            }

            ImGui::SetColumnWidth(0, ImGui::CalcTextSize(MaxLength->c_str()).x + ImGui::GetStyle().IndentSpacing * (float)IndentationLevel + ImGui::GetStyle().FramePadding.x * 2.0f + ImGui::GetStyle().ItemSpacing.x);
            UpdateColumnWidth = false;
        }

        for (auto Iter = Map.begin(); Iter != Map.end(); )
        {
            uint32_t i = std::distance(Map.begin(), Iter);
            bool Delete = false;

            ImGui::Indent();
            ImGui::Text(Iter->first.c_str());

            ImGui::OpenPopupOnItemClick(("DynamicTypeParameterPopUp_" + Iter->first).c_str(), ImGuiPopupFlags_MouseButtonRight);

            if (ImGui::BeginPopup(("DynamicTypeParameterPopUp_" + Iter->first).c_str()))
            {
                if (ImGui::MenuItem("Copy key"))
                {
                    glfwSetClipboardString(application::manager::UIMgr::gWindow, Iter->first.c_str());
                }
                ImGui::Separator();
                Delete = ImGui::MenuItem("Delete");
                ImGui::EndPopup();
            }

            ImGui::Unindent();
            ImGui::NextColumn();

            ImGui::PushItemWidth(ImGui::GetColumnWidth());

            if (std::holds_alternative<std::string>(Iter->second)) SyncBancEntity |= ImGui::InputText(("##" + Title + Iter->first).c_str(), &std::get<std::string>(Iter->second));
            if (std::holds_alternative<bool>(Iter->second)) SyncBancEntity |= ImGui::Checkbox(("##" + Title + Iter->first).c_str(), &std::get<bool>(Iter->second));
            if (std::holds_alternative<int32_t>(Iter->second)) SyncBancEntity |= ImGui::InputScalar(("##" + Title + Iter->first).c_str(), ImGuiDataType_S32, &std::get<int32_t>(Iter->second));
            if (std::holds_alternative<int64_t>(Iter->second)) SyncBancEntity |= ImGui::InputScalar(("##" + Title + Iter->first).c_str(), ImGuiDataType_S64, &std::get<int64_t>(Iter->second));
            if (std::holds_alternative<uint32_t>(Iter->second)) SyncBancEntity |= ImGui::InputScalar(("##" + Title + Iter->first).c_str(), ImGuiDataType_U32, &std::get<uint32_t>(Iter->second));
            if (std::holds_alternative<uint64_t>(Iter->second)) SyncBancEntity |= ImGui::InputScalar(("##" + Title + Iter->first).c_str(), ImGuiDataType_U64, &std::get<uint64_t>(Iter->second));
            if (std::holds_alternative<float>(Iter->second)) SyncBancEntity |= ImGui::InputScalar(("##" + Title + Iter->first).c_str(), ImGuiDataType_Float, &std::get<float>(Iter->second));
            if (std::holds_alternative<glm::vec3>(Iter->second)) SyncBancEntity |= ImGui::InputFloat3(("##" + Title + Iter->first).c_str(), &std::get<glm::vec3>(Iter->second).x);

            ImGui::PopItemWidth();

            if (i < Map.size() - 1)
                ImGui::NextColumn();

            if (Delete)
            {
                Iter = Map.erase(Iter);
                continue;
            }

            Iter++;
        }

        ImGui::Columns();
    }

    void UIMapEditor::DrawDetailsWindow()
    {
        ImGui::Begin(CreateID("Properties").c_str());

        if (IsAnyActorSelected() && mEditingMode == EditingMode::BANC_ENTITY)
        {
            application::game::Scene::BancEntityRenderInfo* RenderInfo = GetSelectedActor();
            bool SyncBancEntity = false;

            if (ImGui::BeginTable("GeneralTable", 2, ImGuiTableFlags_BordersInnerV))
            {
                ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("SRTHash").x + ImGui::GetStyle().IndentSpacing);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Gyml");
                ImGui::TableNextColumn();
                ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
                if (ImGui::InputText("##Gyml", &RenderInfo->mEntity->mGyml))
                {
                    mScene.SyncBancEntityModel(RenderInfo);
                    RenderInfo = GetSelectedActor();
                }
                ImGui::PopItemWidth();
                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Hash");
                ImGui::TableNextColumn();
                ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
                SyncBancEntity |= ImGui::InputScalar("##Hash", ImGuiDataType_U64, &RenderInfo->mEntity->mHash);
                ImGui::PopItemWidth();
                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::Text("SRTHash");
                ImGui::TableNextColumn();
                ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
                SyncBancEntity |= ImGui::InputScalar("##SRTHash", ImGuiDataType_U32, &RenderInfo->mEntity->mSRTHash);
                ImGui::PopItemWidth();
                ImGui::TableNextRow();

                ImGui::Separator();

                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Name");
                ImGui::TableNextColumn();
                ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
                SyncBancEntity |= ImGui::InputText("##Name", &RenderInfo->mEntity->mName);
                ImGui::PopItemWidth();
                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Type");
                ImGui::TableNextColumn();
                ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
                static const char* TypeDropdownItems[] = { "Static", "Dynamic", "Merged" };

                if (RenderInfo->mEntity->mBancType == application::game::BancEntity::BancType::MERGED)
                    ImGui::BeginDisabled();

                if (ImGui::BeginCombo("##Type", TypeDropdownItems[(int)RenderInfo->mEntity->mBancType])) 
                {
                    for (uint8_t n = 0; n < IM_ARRAYSIZE(TypeDropdownItems); n++)
                    {
                        bool IsSelected = ((int)RenderInfo->mEntity->mBancType == n);

                        if (n == 2) //Merged
                            ImGui::BeginDisabled();

                        if (ImGui::Selectable(TypeDropdownItems[n], IsSelected))
                            RenderInfo->mEntity->mBancType = static_cast<application::game::BancEntity::BancType>(n);

                        if (n == 2) //Merged
                            ImGui::EndDisabled();

                        if (IsSelected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }

                if (RenderInfo->mEntity->mBancType == application::game::BancEntity::BancType::MERGED)
                    ImGui::EndDisabled();

                ImGui::PopItemWidth();
                
                ImGui::EndTable();
            }

            ImGui::Separator();
            ImGui::Columns(3);
            ImGui::AlignTextToFramePadding();
            if (ImGui::Button("Duplicate", ImVec2(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize, 0)))
            {
                //Static + Dynamic
                if (RenderInfo->mEntity->mBancType != application::game::BancEntity::BancType::MERGED)
                {
                    std::optional<application::game::BancEntity> NewEntity = mScene.CreateBancEntity(*RenderInfo->mEntity);

                    if (!NewEntity.has_value())
                        goto AbortDuplication;

                    //Removing a link to a far actor
                    if (!NewEntity.value().mGyml.ends_with("_Far") && !NewEntity.value().mLinks.empty())
                    {
                        NewEntity.value().mLinks.erase(std::remove_if(NewEntity.value().mLinks.begin(), NewEntity.value().mLinks.end(), [this](const application::game::BancEntity::Link& Link)
                            {
                                for (const application::game::BancEntity& Entity : mScene.mBancEntities)
                                {
                                    if (Entity.mHash == Link.mDest)
                                    {
                                        return Entity.mGyml.ends_with("_Far");
                                    }
                                }
                                return false;
                            }),
                            NewEntity.value().mLinks.end());
                    }

                    application::game::Scene::BancEntityRenderInfo* EntityInfo = mScene.AddBancEntity(NewEntity.value());

                    if (EntityInfo == nullptr)
                        goto AbortDuplication;

                    SelectActor(EntityInfo->mEntityIndex);
                }
                else //Merged
                {
                    std::optional<application::game::BancEntity> EntityOpt = mScene.CreateBancEntity(*RenderInfo->mEntity);

                    if (!EntityOpt.has_value())
                        goto AbortDuplication;

                    application::game::BancEntity& Entity = EntityOpt.value();

                    std::vector<application::game::BancEntity>* MergedActor = RenderInfo->mMergedActorParent->mMergedActorChildren;
                    if (MergedActor == nullptr)
                        goto AbortDuplication;

                    glm::vec3 OriginalActorPos = RenderInfo->mTranslate;

                    MergedActor->push_back(Entity);
                    mScene.GenerateDrawList();

                    //Select nearest instance of new merged actor relative to camera
                    SelectNearestMergedBancEntityInstance(MergedActor, Entity, OriginalActorPos);
                }
            AbortDuplication:
                ;
            }
            ImGui::NextColumn();
            if (ImGui::Button("Deselect", ImVec2(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize, 0)))
            {
                DeselectActor();
            }
            ImGui::NextColumn();
            if (ImGuiExt::ButtonDelete("Delete", ImVec2(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize, 0)))
            {
                gConfirmActorDeletePopUp.Open([this](application::rendering::popup::PopUpBuilder& Builder)
                    {
                        application::game::Scene::BancEntityRenderInfo* RenderInfo = GetSelectedActor();

                        if (RenderInfo == nullptr)
                            return;

                        mScene.DeleteBancEntity(RenderInfo);
                        DeselectActor();
                    });
            }
            ImGui::Columns();

            if (!IsAnyActorSelected() || GetSelectedActor() != RenderInfo)
            {
                ImGui::End();
                return;
            }

            if (RenderInfo->mMergedActorParent == nullptr)
            {
                ImGui::SetCursorPosX(ImGui::GetStyle().ItemSpacing.x);
                if (ImGui::Button("Add to parent", ImVec2(ImGui::GetColumnWidth() - ImGui::GetStyle().ItemSpacing.x + 2, 0)))
                {
                    gAddBancEntityToParentPopUp.Open([this, &RenderInfo, &SyncBancEntity](application::rendering::popup::PopUpBuilder& Builder)
                        {
                            application::game::Scene::BancEntityRenderInfo NewRenderInfo = *GetSelectedActor();
                            NewRenderInfo.mEntity->mPhive.mPlacement["ID"] = NewRenderInfo.mEntity->mHash;

                            std::vector<application::game::BancEntity>* MergedActor = application::manager::MergedActorMgr::GetMergedActor(*reinterpret_cast<std::string*>(Builder.GetDataStorage(0).mPtr), true);
                            if (MergedActor == nullptr)
                                return;

                            MergedActor->push_back(*NewRenderInfo.mEntity);
                            mScene.DeleteBancEntity(GetSelectedActor());
                            MergedActor->back().mBancType = application::game::BancEntity::BancType::MERGED;

                            mScene.GenerateDrawList();

                            for (application::game::Scene::BancEntityRenderInfo* Info : mScene.mDrawListRenderInfoIndices)
                            {
                                if (Info->mEntity == &MergedActor->back())
                                {
                                    SelectActor(Info->mEntityIndex);
                                    break;
                                }
                            }

                            RenderInfo = GetSelectedActor();

                            RenderInfo->mTranslate = NewRenderInfo.mTranslate;
                            RenderInfo->mRotate = NewRenderInfo.mRotate;
                            RenderInfo->mScale = NewRenderInfo.mScale;
                            mScene.SyncBancEntity(RenderInfo);

                            SyncBancEntity = true;
                        });
                }
            }
            else
            {
                ImGui::Columns(2);
                if (ImGui::Button("Remove from parent", ImVec2(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize, 0)))
                {
                    std::optional<application::game::BancEntity> NewEntityOpt = mScene.CreateBancEntity(*RenderInfo->mEntity);

                    if (NewEntityOpt.has_value())
                    {
                        application::game::BancEntity& NewEntity = NewEntityOpt.value();
                        NewEntity.mBancType = application::game::BancEntity::BancType::STATIC;
                        NewEntity.mTranslate = RenderInfo->mTranslate;
                        NewEntity.mRotate = RenderInfo->mRotate;
                        NewEntity.mScale = RenderInfo->mScale;

                        RenderInfo->mMergedActorParent->mMergedActorChildren->erase(
                            std::remove_if(RenderInfo->mMergedActorParent->mMergedActorChildren->begin(), RenderInfo->mMergedActorParent->mMergedActorChildren->end(), [&](const application::game::BancEntity& Entity) {
                                return Entity.mGyml == RenderInfo->mEntity->mGyml && Entity.mHash == RenderInfo->mEntity->mHash && Entity.mSRTHash == RenderInfo->mEntity->mSRTHash;
                                }),
                            RenderInfo->mMergedActorParent->mMergedActorChildren->end());
                        RenderInfo->mMergedActorParent->mMergedActorChildrenModified = true;

                        mScene.mBancEntities.push_back(NewEntity);

                        mScene.GenerateDrawList();

                        for (application::game::Scene::BancEntityRenderInfo* Info : mScene.mDrawListRenderInfoIndices)
                        {
                            if (Info->mEntity == &mScene.mBancEntities.back())
                            {
                                SelectActor(Info->mEntityIndex);
                                break;
                            }
                        }

                        RenderInfo = GetSelectedActor();
                    }
                }
                ImGui::NextColumn();
                if (ImGui::Button("Select parent", ImVec2(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize, 0)))
                {
                    for (application::game::Scene::BancEntityRenderInfo* Info : mScene.mDrawListRenderInfoIndices)
                    {
                        if (Info->mEntity == RenderInfo->mMergedActorParent)
                        {
                            SelectActor(Info->mEntityIndex);
                            break;
                        }
                    }
                    ImGui::Columns();
                    ImGui::End();
                    return;
                }

                ImGui::Columns();
            }

            if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Columns(2, "TransformCameraActor");
                ImGui::AlignTextToFramePadding();
                if (ImGui::Button("Camera to actor", ImVec2(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize, 0)))
                {
                    mCamera.mPosition = RenderInfo->mTranslate + (8.0f * -mCamera.mOrientation);
                }
                ImGui::NextColumn();
                if (ImGui::Button("Actor to camera", ImVec2(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize, 0)))
                {
                    RenderInfo->mTranslate = mCamera.mPosition + (4.0f * mCamera.mOrientation);
                    SyncBancEntity = true;
                }
                ImGui::Columns();
                ImGui::Separator();

                ImGui::Indent();

                if (ImGui::BeginTable("TransformTable", 2, ImGuiTableFlags_BordersInnerV))
                {
                    ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("Translation").x + ImGui::GetStyle().IndentSpacing);
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Translation");
                    ImGuiExt::CheckVec3fInputRightClick("TranslationPopUp", &RenderInfo->mTranslate.x);
                    ImGui::TableNextColumn();
                    ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
                    SyncBancEntity |= ImGuiExt::InputFloat3Colored("##Translation", &RenderInfo->mTranslate.x, ImVec4(0.26f, 0.06f, 0.07f, 1.0f), ImVec4(0.06f, 0.26f, 0.07f, 1.0f), ImVec4(0.06f, 0.07f, 0.26f, 1.0f));
                    ImGui::PopItemWidth();
                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Rotation");
                    ImGuiExt::CheckVec3fInputRightClick("RotationPopUp", &RenderInfo->mRotate.x, true);
                    ImGui::TableNextColumn();
                    ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
                    SyncBancEntity |= ImGuiExt::InputFloat3Colored("##Rotation", &RenderInfo->mRotate.x, ImVec4(0.22f, 0.02f, 0.03f, 1.0f), ImVec4(0.02f, 0.22f, 0.03f, 1.0f), ImVec4(0.02f, 0.03f, 0.22f, 1.0f));
                    ImGui::PopItemWidth();
                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Scale");
                    ImGuiExt::CheckVec3fInputRightClick("ScalePopUp", &RenderInfo->mScale.x);
                    ImGui::TableNextColumn();
                    ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
                    SyncBancEntity |= ImGuiExt::InputFloat3Colored("##Scale", &RenderInfo->mScale.x, ImVec4(0.18f, 0.0f, 0.0f, 1.0f), ImVec4(0.0f, 0.18f, 0.0f, 1.0f), ImVec4(0.0f, 0.0f, 0.18f, 1.0f));
                    ImGui::PopItemWidth();

                    ImGui::EndTable();
                }

                ImGui::Unindent();
            }

            if (ImGui::CollapsingHeader("Properties", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Indent();

                if (ImGui::BeginTable("PropertiesTable", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable))
                {
                    ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("TurnActorNearEnemy").x + ImGui::GetStyle().IndentSpacing);
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Bakeable");
                    ImGui::TableNextColumn();
                    SyncBancEntity |= ImGui::Checkbox("##Bakeable", &RenderInfo->mEntity->mBakeable);
                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("PhysicsStable");
                    ImGui::TableNextColumn();
                    SyncBancEntity |= ImGui::Checkbox("##PhysicsStable", &RenderInfo->mEntity->mIsPhysicsStable);
                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("ForceActive");
                    ImGui::TableNextColumn();
                    SyncBancEntity |= ImGui::Checkbox("##ForceActive", &RenderInfo->mEntity->mIsForceActive);
                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("TurnActorNearEnemy");
                    ImGui::TableNextColumn();
                    SyncBancEntity |= ImGui::Checkbox("##TurnActorNearEnemy", &RenderInfo->mEntity->mTurnActorNearEnemy);
                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("InWater");
                    ImGui::TableNextColumn();
                    SyncBancEntity |= ImGui::Checkbox("##InWater", &RenderInfo->mEntity->mIsInWater);
                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("MoveRadius");
                    ImGui::TableNextColumn();
                    ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
                    SyncBancEntity |= ImGui::InputScalar("##MoveRadius", ImGuiDataType_Float, &RenderInfo->mEntity->mMoveRadius);
                    ImGui::PopItemWidth();
                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("ExtraCreateRadius");
                    ImGui::TableNextColumn();
                    ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
                    SyncBancEntity |= ImGui::InputScalar("##ExtraCreateRadius", ImGuiDataType_Float, &RenderInfo->mEntity->mExtraCreateRadius);
                    ImGui::PopItemWidth();
                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Version");
                    ImGui::TableNextColumn();
                    ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
                    SyncBancEntity |= ImGui::InputScalar("##Version", ImGuiDataType_S32, &RenderInfo->mEntity->mVersion);
                    ImGui::PopItemWidth();
                    ImGui::TableNextRow();

                    ImGui::EndTable();
                }

                ImGui::Unindent();
            }

            DrawDynamicTypePropertiesHeader("Dynamic", RenderInfo->mEntity->mDynamic, SyncBancEntity, mPropertiesWindowInfo.mSetDynamicColumnWidth, ImGuiTreeNodeFlags_DefaultOpen);

            if (ImGui::CollapsingHeader("Phive", ImGuiTreeNodeFlags_DefaultOpen))
            {
                /*

			struct RopeLink
			{
				uint64_t mID = 0;
				std::vector<uint64_t> mOwners;
				std::vector<uint64_t> mRefers;
			};

			std::optional<RopeLink> mRopeHeadLink = std::nullopt;
			std::optional<RopeLink> mRopeTailLink = std::nullopt;
		};
                */
                ImGui::Indent();

                DrawDynamicTypePropertiesSeparator("Placement", RenderInfo->mEntity->mPhive.mPlacement, SyncBancEntity, mPropertiesWindowInfo.mSetPhivePlacementColumnWidth);
               
                 ImGui::Unindent();

                 ImGui::NewLine();
                 ImGui::Text("Constraint Link");

                 if (!RenderInfo->mEntity->mPhive.mConstraintLink.has_value())
                 {
                     ImGui::SameLine();
                     if (ImGui::Button("Add"))
                     {
                         RenderInfo->mEntity->mPhive.mConstraintLink.emplace();
						 SyncBancEntity = true;
                     }
                 }

                 ImGui::Separator();
                 if (RenderInfo->mEntity->mPhive.mConstraintLink.has_value())
                 {
                     ImGui::Indent();

                     ImGui::Columns(2, "PhiveConstraintLinkColumnsID");
                     ImGui::Text("ID");
                     ImGui::NextColumn();
                     ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
                     ImGui::InputScalar("##PhiveConstarintLinkID", ImGuiDataType_U64, reinterpret_cast<uint64_t*>(&RenderInfo->mEntity->mPhive.mConstraintLink->mID));
                     ImGui::PopItemWidth();
                     ImGui::NextColumn();

                     ImGui::Columns();
                     ImGui::Text("Owners");
                     if (ImGui::Button("Add##Owner", ImVec2(ImGui::GetWindowWidth() - ImGui::GetStyle().FramePadding.x * 2 - ImGui::GetStyle().IndentSpacing - ImGui::GetStyle().ScrollbarSize, 0)))
                     {
                         application::game::BancEntity::Phive::ConstraintLink::Owner Owner;
                         Owner.mType = "Fixed";
                         Owner.mParamData.insert({ "EnableAngularSpringMode", false });
                         Owner.mParamData.insert({ "EnableLinearSpringMode", false });
                         RenderInfo->mEntity->mPhive.mConstraintLink->mOwners.push_back(Owner);
                     }
                     ImGui::Separator();
                     ImGui::Indent();
                     uint32_t OwnerIndex = 0;
                     for (application::game::BancEntity::Phive::ConstraintLink::Owner& Owner : RenderInfo->mEntity->mPhive.mConstraintLink->mOwners)
                     {
                         ImGui::Columns(2, ("PhiveConstraintLinkColumnsType" + std::to_string(OwnerIndex)).c_str());
                         ImGui::Text("Type");
                         ImGui::NextColumn();
                         ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
                         ImGui::InputText(std::string("##PhiveConstraintLinkOwnerType" + std::to_string(OwnerIndex)).c_str(), &Owner.mType);
                         ImGui::PopItemWidth();
                         ImGui::NextColumn();
                         ImGui::Columns();

                         DrawDynamicTypePropertiesSeparator("Breakable Data##ConstraintLink", Owner.mBreakableData, SyncBancEntity, mPropertiesWindowInfo.mSetPhiveColumnWidth);

                         DrawDynamicTypePropertiesSeparator("Alias Data##ConstraintLink", Owner.mAliasData, SyncBancEntity, mPropertiesWindowInfo.mSetPhiveColumnWidth);

                         DrawDynamicTypePropertiesSeparator("Cluster Data##ConstraintLink", Owner.mClusterData, SyncBancEntity, mPropertiesWindowInfo.mSetPhiveColumnWidth);

                         DrawDynamicTypePropertiesSeparator("User Data##ConstraintLink", Owner.mUserData, SyncBancEntity, mPropertiesWindowInfo.mSetPhiveColumnWidth);

                         DrawDynamicTypePropertiesSeparator("Param Data##ConstraintLink", Owner.mParamData, SyncBancEntity, mPropertiesWindowInfo.mSetPhiveColumnWidth);

                         ImGui::Text("Pose");
                         ImGui::Separator();
                         ImGui::Indent();
                         ImGui::Columns(2, ("PhiveConstraintLinkColumnsPose" + std::to_string(OwnerIndex)).c_str());
                         ImGui::Text("Translate");
                         ImGui::NextColumn();
                         ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
                         ImGuiExt::InputFloat3Colored(std::string("##PhiveConstrainLinkOwnerPosTranslate" + std::to_string(OwnerIndex)).c_str(), &Owner.mOwnerPose.mTranslate.x, ImVec4(0.26f, 0.06f, 0.07f, 1.0f), ImVec4(0.06f, 0.26f, 0.07f, 1.0f), ImVec4(0.06f, 0.07f, 0.26f, 1.0f), true);
                         ImGui::PopItemWidth();
                         ImGui::NextColumn();
                         ImGui::Text("Rotate");
                         ImGui::NextColumn();
                         ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
                         ImGuiExt::InputFloat3Colored(std::string("##PhiveConstrainLinkOwnerPosRotate" + std::to_string(OwnerIndex)).c_str(), &Owner.mOwnerPose.mRotate.x, ImVec4(0.26f, 0.06f, 0.07f, 1.0f), ImVec4(0.06f, 0.26f, 0.07f, 1.0f), ImVec4(0.06f, 0.07f, 0.26f, 1.0f), true);
                         ImGui::PopItemWidth();
                         ImGui::NextColumn();
                         ImGui::Columns();
                         ImGui::Unindent();

                         ImGui::Text("Refer");
                         ImGui::Separator();
                         ImGui::Indent();

                         ImGui::Columns(2, ("PhiveConstraintLinkColumnsRefer" + std::to_string(OwnerIndex)).c_str());
                         ImGui::Text("ID");
                         ImGui::NextColumn();
                         ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
                         ImGui::InputScalar(("PhiveConstraintLinkColumnsReferID" + std::to_string(OwnerIndex)).c_str(), ImGuiDataType_U64, reinterpret_cast<uint64_t*>(&Owner.mRefer));
                         ImGui::PopItemWidth();
                         ImGui::NextColumn();
                         ImGui::Columns();

                         ImGui::Text("Pose");
                         ImGui::Indent();
                         ImGui::Columns(2, ("PhiveConstraintLinkColumnsReferPose" + std::to_string(OwnerIndex)).c_str());
                         ImGui::Text("Translate");
                         ImGui::NextColumn();
                         ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
                         ImGuiExt::InputFloat3Colored(std::string("##PhiveConstrainLinkOwnerReferPosTranslate" + std::to_string(OwnerIndex)).c_str(), &Owner.mReferPose.mTranslate.x, ImVec4(0.26f, 0.06f, 0.07f, 1.0f), ImVec4(0.06f, 0.26f, 0.07f, 1.0f), ImVec4(0.06f, 0.07f, 0.26f, 1.0f), true);
                         ImGui::PopItemWidth();
                         ImGui::NextColumn();
                         ImGui::Text("Rotate");
                         ImGui::NextColumn();
                         ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
                         ImGuiExt::InputFloat3Colored(std::string("##PhiveConstrainLinkOwnerReferPosRotate" + std::to_string(OwnerIndex)).c_str(), &Owner.mReferPose.mRotate.x, ImVec4(0.26f, 0.06f, 0.07f, 1.0f), ImVec4(0.06f, 0.26f, 0.07f, 1.0f), ImVec4(0.06f, 0.07f, 0.26f, 1.0f), true);
                         ImGui::PopItemWidth();
                         ImGui::NextColumn();
                         ImGui::Columns();
                         ImGui::Unindent();
                         ImGui::Unindent();

                         ImGui::Text("Pivot Data");
                         ImGui::SameLine();
                         if (ImGui::Button("Add all"))
                         {
                             if (!Owner.mPivotData.mAxis.has_value()) Owner.mPivotData.mAxis = 0;
                             if (!Owner.mPivotData.mAxisA.has_value()) Owner.mPivotData.mAxisA = 0;
                             if (!Owner.mPivotData.mAxisB.has_value()) Owner.mPivotData.mAxisB = 0;

                             if (!Owner.mPivotData.mPivot.has_value()) Owner.mPivotData.mPivot.emplace();
                             if (!Owner.mPivotData.mPivotA.has_value()) Owner.mPivotData.mPivotA.emplace();
                             if (!Owner.mPivotData.mPivotB.has_value()) Owner.mPivotData.mPivotB.emplace();
                         }
                         ImGui::Separator();
                         ImGui::Indent();
                         ImGui::Columns(2, ("PhiveConstraintLinkColumnsPivotData" + std::to_string(OwnerIndex)).c_str());

                         if (Owner.mPivotData.mAxis.has_value())
                         {
                             ImGui::Text("Axis");
                             ImGui::NextColumn();
                             ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
                             ImGui::InputScalar(std::string("##PhiveConstrainLinkOwnerPivotDataAxis" + std::to_string(OwnerIndex)).c_str(), ImGuiDataType_S32, &Owner.mPivotData.mAxis.value());
                             ImGui::PopItemWidth();
                             ImGui::NextColumn();
                         }

                         if (Owner.mPivotData.mAxisA.has_value())
                         {
                             ImGui::Text("AxisA");
                             ImGui::NextColumn();
                             ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
                             ImGui::InputScalar(std::string("##PhiveConstrainLinkOwnerPivotDataAxisA" + std::to_string(OwnerIndex)).c_str(), ImGuiDataType_S32, &Owner.mPivotData.mAxisA.value());
                             ImGui::PopItemWidth();
                             ImGui::NextColumn();
                         }

                         if (Owner.mPivotData.mAxisB.has_value())
                         {
                             ImGui::Text("AxisB");
                             ImGui::NextColumn();
                             ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
                             ImGui::InputScalar(std::string("##PhiveConstrainLinkOwnerPivotDataAxisB" + std::to_string(OwnerIndex)).c_str(), ImGuiDataType_S32, &Owner.mPivotData.mAxisB.value());
                             ImGui::PopItemWidth();
                             ImGui::NextColumn();
                         }

                         if (Owner.mPivotData.mPivot.has_value())
                         {
                             ImGui::Text("Pivot");
                             ImGui::NextColumn();
                             ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
                             ImGuiExt::InputFloat3Colored(std::string("##PhiveConstrainLinkOwnerPivot" + std::to_string(OwnerIndex)).c_str(), &Owner.mPivotData.mPivot->x, ImVec4(0.26f, 0.06f, 0.07f, 1.0f), ImVec4(0.06f, 0.26f, 0.07f, 1.0f), ImVec4(0.06f, 0.07f, 0.26f, 1.0f), true);
                             ImGui::PopItemWidth();
                             ImGui::NextColumn();
                         }

                         if (Owner.mPivotData.mPivotA.has_value())
                         {
                             ImGui::Text("PivotA");
                             ImGui::NextColumn();
                             ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
                             ImGuiExt::InputFloat3Colored(std::string("##PhiveConstrainLinkOwnerPivotA" + std::to_string(OwnerIndex)).c_str(), &Owner.mPivotData.mPivotA->x, ImVec4(0.26f, 0.06f, 0.07f, 1.0f), ImVec4(0.06f, 0.26f, 0.07f, 1.0f), ImVec4(0.06f, 0.07f, 0.26f, 1.0f), true);
                             ImGui::PopItemWidth();
                             ImGui::NextColumn();
                         }

                         if (Owner.mPivotData.mPivotB.has_value())
                         {
                             ImGui::Text("PivotB");
                             ImGui::NextColumn();
                             ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
                             ImGuiExt::InputFloat3Colored(std::string("##PhiveConstrainLinkOwnerPivotB" + std::to_string(OwnerIndex)).c_str(), &Owner.mPivotData.mPivotB->x, ImVec4(0.26f, 0.06f, 0.07f, 1.0f), ImVec4(0.06f, 0.26f, 0.07f, 1.0f), ImVec4(0.06f, 0.07f, 0.26f, 1.0f), true);
                             ImGui::PopItemWidth();
                             ImGui::NextColumn();
                         }

                         ImGui::Columns();
                         ImGui::Unindent();

                         OwnerIndex++;
                     }
                     ImGui::Unindent();

                     ImGui::Text("Refers");
                     if (ImGui::Button("Add##Refer", ImVec2(ImGui::GetWindowWidth() - ImGui::GetStyle().FramePadding.x * 2 - ImGui::GetStyle().IndentSpacing - ImGui::GetStyle().ScrollbarSize, 0)))
                     {
                         application::game::BancEntity::Phive::ConstraintLink::Refer Refer;
                         Refer.mOwner = 0;
                         Refer.mType = "Fixed";
                         RenderInfo->mEntity->mPhive.mConstraintLink->mRefers.push_back(Refer);
                     }
                     ImGui::Separator();
                     ImGui::Indent();
                     uint32_t ReferIndex = 0;
                     for (application::game::BancEntity::Phive::ConstraintLink::Refer& Refer : RenderInfo->mEntity->mPhive.mConstraintLink->mRefers)
                     {
                         ImGui::Columns(2, ("##PhiveConstrainLinkRefer" + std::to_string(ReferIndex)).c_str());

                         ImGui::Text("Owner");
                         ImGui::NextColumn();
                         ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
                         ImGui::InputScalar(("##PhiveConstrainLinkReferOwner" + std::to_string(ReferIndex)).c_str(), ImGuiDataType_U64, &Refer.mOwner);
                         ImGui::PopItemWidth();
                         ImGui::NextColumn();

                         ImGui::Text("Type");
                         ImGui::NextColumn();
                         ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
                         ImGui::InputText(("##PhiveConstrainLinkReferType" + std::to_string(ReferIndex)).c_str(), &Refer.mType);
                         ImGui::PopItemWidth();
                         ImGui::NextColumn();

                         ImGui::Columns();

                         if (ReferIndex < (RenderInfo->mEntity->mPhive.mConstraintLink->mRefers.size() - 1))
                             ImGui::NewLine();

                         ReferIndex++;
                     }

                     ImGui::Unindent();
                     ImGui::Unindent();
                 }
            }

            DrawDynamicTypePropertiesHeader("Presence", RenderInfo->mEntity->mPresence, SyncBancEntity, mPropertiesWindowInfo.mSetPresenceColumnWidth, ImGuiTreeNodeFlags_None);
            DrawDynamicTypePropertiesHeader("External Parameters", RenderInfo->mEntity->mExternalParameter, SyncBancEntity, mPropertiesWindowInfo.mSetExternalParametersColumnWidth, ImGuiTreeNodeFlags_None);

            auto OnAddLinkPress = [this, &RenderInfo]()
                {
                    gAddLinkPopUp.Open([this, &RenderInfo](application::rendering::popup::PopUpBuilder& Builder)
                        {
                            application::game::BancEntity::Link Link;
                            Link.mSrc = *reinterpret_cast<uint64_t*>(Builder.GetDataStorage(0).mPtr);
                            Link.mDest = *reinterpret_cast<uint64_t*>(Builder.GetDataStorage(1).mPtr);
                            Link.mGyml = *reinterpret_cast<std::string*>(Builder.GetDataStorage(2).mPtr);
                            Link.mName = *reinterpret_cast<std::string*>(Builder.GetDataStorage(3).mPtr);

                            GetSelectedActor()->mEntity->mLinks.push_back(Link);
                        });
                };

            auto OnAddRailPress = [this, &RenderInfo]()
                {
                    gAddRailPopUp.Open([this, &RenderInfo](application::rendering::popup::PopUpBuilder& Builder)
                        {
                            application::game::BancEntity::Rail Rail;
                            Rail.mDest = *reinterpret_cast<uint64_t*>(Builder.GetDataStorage(0).mPtr);
                            Rail.mGyml = *reinterpret_cast<std::string*>(Builder.GetDataStorage(1).mPtr);
                            Rail.mName = *reinterpret_cast<std::string*>(Builder.GetDataStorage(2).mPtr);

                            GetSelectedActor()->mEntity->mRails.push_back(Rail);
                        });
                };

            if (ImGuiExt::CollapsingHeaderWithAddButton("Links", ImGuiTreeNodeFlags_DefaultOpen, OnAddLinkPress))
            {
                ImGui::Indent();
                ImGui::Columns(2, "Links");
                ImGui::AlignTextToFramePadding();

                for (auto Iter = RenderInfo->mEntity->mLinks.begin(); Iter != RenderInfo->mEntity->mLinks.end(); )
                {
                    uint32_t i = std::distance(RenderInfo->mEntity->mLinks.begin(), Iter);

                    ImGui::Text("Source");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
                    ImGui::InputScalar(("##LinkSource" + std::to_string(i)).c_str(), ImGuiDataType_::ImGuiDataType_U64, &Iter->mSrc);
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::Text("Destination");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
                    ImGui::InputScalar(("##LinkDestination" + std::to_string(i)).c_str(), ImGuiDataType_::ImGuiDataType_U64, &Iter->mDest);
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::Text("Gyml");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
                    ImGui::InputText(("##LinkGyml" + std::to_string(i)).c_str(), &Iter->mGyml);
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::Text("Name");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
                    ImGui::InputText(("##LinkName" + std::to_string(i)).c_str(), &Iter->mName);
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    bool WantDelete = ImGuiExt::ButtonDelete(("Delete##LinkDel" + std::to_string(i)).c_str(), ImVec2(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize, 0));
                    
                    ImGui::NextColumn();
                    if (i != RenderInfo->mEntity->mLinks.size()  - 1)
                    {
                        ImGui::NextColumn();
                        ImGui::Separator();
                    }
                    if (WantDelete)
                    {
                        Iter = RenderInfo->mEntity->mLinks.erase(Iter);
                        continue;
                    }
                    Iter++;
                }

                ImGui::Columns();
                ImGui::Unindent();
            }

            if (ImGuiExt::CollapsingHeaderWithAddButton("Rails", ImGuiTreeNodeFlags_None, OnAddRailPress))
            {
                ImGui::Indent();
                ImGui::Columns(2, "Rails");
                ImGui::AlignTextToFramePadding();

                for (auto Iter = RenderInfo->mEntity->mRails.begin(); Iter != RenderInfo->mEntity->mRails.end(); )
                {
                    uint32_t i = std::distance(RenderInfo->mEntity->mRails.begin(), Iter);

                    ImGui::Text("Destination");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
                    ImGui::InputScalar(("##LinkDestination" + std::to_string(i)).c_str(), ImGuiDataType_::ImGuiDataType_U64, &Iter->mDest);
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::Text("Gyml");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
                    ImGui::InputText(("##LinkGyml" + std::to_string(i)).c_str(), &Iter->mGyml);
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::Text("Name");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
                    ImGui::InputText(("##LinkName" + std::to_string(i)).c_str(), &Iter->mName);
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    bool WantDelete = ImGuiExt::ButtonDelete(("Delete##LinkDel" + std::to_string(i)).c_str(), ImVec2(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize, 0));

                    ImGui::NextColumn();
                    if (i != RenderInfo->mEntity->mRails.size() - 1)
                    {
                        ImGui::NextColumn();
                        ImGui::Separator();
                    }
                    if (WantDelete)
                    {
                        Iter = RenderInfo->mEntity->mRails.erase(Iter);
                        continue;
                    }
                    Iter++;
                }

                ImGui::Columns();
                ImGui::Unindent();
            }

            if (ImGui::CollapsingHeader("Fluid"))
            {
                ImGui::Indent();
                ImGui::Columns(2, "FluidColumns");

                ImGui::Text("Bake Fluid");
                ImGui::NextColumn();
                ImGui::Checkbox("##StaticCompoundBakeFluid", &RenderInfo->mEntity->mBakedFluid.mBakeFluid);
                ImGui::NextColumn();

                ImGui::BeginDisabled(!RenderInfo->mEntity->mBakedFluid.mBakeFluid);

                ImGui::Text("Fluid Shape");
                ImGui::NextColumn();
                ImGui::Combo("##StaticCompoundFluidShape", reinterpret_cast<int*>(&RenderInfo->mEntity->mBakedFluid.mFluidShapeType), application::game::BancEntity::BakedFluid::gFluidShapeTypes.data(), application::game::BancEntity::BakedFluid::gFluidShapeTypes.size());
                ImGui::NextColumn();

                ImGui::Text("Fluid Type");
                ImGui::NextColumn();
                ImGui::Combo("##StaticCompoundFluidType", reinterpret_cast<int*>(&RenderInfo->mEntity->mBakedFluid.mFluidMaterialType), application::game::BancEntity::BakedFluid::gFluidMaterialTypes.data(), application::game::BancEntity::BakedFluid::gFluidMaterialTypes.size());
                ImGui::NextColumn();

                ImGui::Text("Fluid box height");
                ImGui::NextColumn();
                ImGui::InputScalar("##StaticCompoundBoxHeight", ImGuiDataType_Float, &RenderInfo->mEntity->mBakedFluid.mFluidBoxHeight, NULL, NULL, "%.3f", 0);
                ImGui::NextColumn();

                ImGui::Text("Flow speed (z-axis)");
                ImGui::NextColumn();
                ImGui::InputScalar("##StaticCompoundFlowSpeed", ImGuiDataType_Float, &RenderInfo->mEntity->mBakedFluid.mFluidFlowSpeed, NULL, NULL, "%.3f", 0);
                ImGui::NextColumn();

                ImGui::EndDisabled();

                ImGui::Columns();
                ImGui::Unindent();
            }

            if (ImGui::CollapsingHeader("Flag Reset Conditions"))
            {
                ImGui::Indent();
                ImGui::Columns(2, "FlagColumns");

                ImGui::Text("OnBancChange");
                ImGui::NextColumn();
                ImGui::Checkbox("##OnBancChange", &RenderInfo->mEntity->mGameData.mOnBancChange);
                ImGui::NextColumn();

                ImGui::Text("OnNewDay");
                ImGui::NextColumn();
                ImGui::Checkbox("##OnNewDay", &RenderInfo->mEntity->mGameData.mOnNewDay);
                ImGui::NextColumn();

                ImGui::Text("OnDefaultOptionRevert");
                ImGui::NextColumn();
                ImGui::Checkbox("##OnDefaultOptionRevert", &RenderInfo->mEntity->mGameData.mOnDefaultOptionRevert);
                ImGui::NextColumn();

                ImGui::Text("OnBloodMoon");
                ImGui::NextColumn();
                ImGui::Checkbox("##OnBloodMoon", &RenderInfo->mEntity->mGameData.mOnBloodMoon);
                ImGui::NextColumn();

                ImGui::Text("OnNewGame");
                ImGui::NextColumn();
                ImGui::Checkbox("##OnNewGame", &RenderInfo->mEntity->mGameData.mOnNewGame);
                ImGui::NextColumn();

                ImGui::Text("Unknown0");
                ImGui::NextColumn();
                ImGui::Checkbox("##Unknown0", &RenderInfo->mEntity->mGameData.mUnknown0);
                ImGui::NextColumn();

                ImGui::Text("Unknown1");
                ImGui::NextColumn();
                ImGui::Checkbox("##Unknown1", &RenderInfo->mEntity->mGameData.mUnknown1);
                ImGui::NextColumn();

                ImGui::Text("OnZonaiRespawnDay");
                ImGui::NextColumn();
                ImGui::Checkbox("##OnZonaiRespawnDay", &RenderInfo->mEntity->mGameData.mOnZonaiRespawnDay);
                ImGui::NextColumn();

                ImGui::Text("OnMaterialRespawn");
                ImGui::NextColumn();
                ImGui::Checkbox("##OnMaterialRespawn", &RenderInfo->mEntity->mGameData.mOnMaterialRespawn);
                ImGui::NextColumn();

                ImGui::Text("NoResetOnNewGame");
                ImGui::NextColumn();
                ImGui::Checkbox("##NoResetOnNewGame", &RenderInfo->mEntity->mGameData.mNoResetOnNewGame);
                ImGui::NextColumn();

                ImGui::Separator();
                ImGui::Text("SaveFileIndex");
                ImGui::NextColumn();
                ImGui::InputScalar("##SaveFileIndex", ImGuiDataType_S32, &RenderInfo->mEntity->mGameData.mSaveFileIndex, NULL, NULL, NULL, 0);
                ImGui::NextColumn();

                if (!RenderInfo->mEntity->mGameData.mOnMaterialRespawn)
                    ImGui::BeginDisabled();

                ImGui::Text("ExtraByte");
                ImGui::NextColumn();
                ImGui::InputScalar("##ExtraByte", ImGuiDataType_U8, &RenderInfo->mEntity->mGameData.mExtraByte, NULL, NULL, NULL, 0);
                ImGui::NextColumn();

                if (!RenderInfo->mEntity->mGameData.mOnMaterialRespawn)
                    ImGui::EndDisabled();

                ImGui::Columns();
                ImGui::Unindent();
            }

            if (RenderInfo->mEntity->mGyml.find("Far") == std::string::npos)
            {
                if (ImGui::CollapsingHeader("Far##FarHeader", ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_DefaultOpen))
                {
                    bool HasFarActor = false;
                    application::game::Scene::BancEntityRenderInfo* LinkedFarActor = nullptr;
                    application::game::BancEntity::Link* ActorLink = nullptr;
                    for (application::game::BancEntity::Link& Link : RenderInfo->mEntity->mLinks)
                    {
                        if (Link.mGyml != "Reference" || Link.mName != "Reference") continue;

                        for (application::game::Scene::BancEntityRenderInfo* PossibleFarActor : mScene.mDrawListRenderInfoIndices)
                        {
                            if (PossibleFarActor->mEntity->mHash == Link.mDest)
                            {
                                HasFarActor = PossibleFarActor->mEntity->mGyml.find("Far") != std::string::npos;
                                if (HasFarActor)
                                {
                                    ActorLink = &Link;
                                    LinkedFarActor = PossibleFarActor;
                                }
                                break;
                            }
                        }
                    }

                    bool FoundFarActor = application::util::FileUtil::FileExists(application::util::FileUtil::GetRomFSFilePath("Pack/Actor/" + RenderInfo->mEntity->mGyml + "_Far.pack.zs"));
                    if (!FoundFarActor || HasFarActor)
                        ImGui::BeginDisabled();

                    ImGui::SetCursorPosX(ImGui::GetStyle().ItemSpacing.x);
                    if (ImGui::Button(FoundFarActor ? ("Add " + RenderInfo->mEntity->mGyml + "_Far").c_str() : "Not found##FarActor", ImVec2(ImGui::GetWindowSize().x - ImGui::GetStyle().ScrollbarSize, 0)))
                    {
                        application::game::Scene::BancEntityRenderInfo* FarActorRenderInfo = mScene.AddBancEntity(RenderInfo->mEntity->mGyml + "_Far");
                        RenderInfo = GetSelectedActor();

                        FarActorRenderInfo->mEntity->mBakeable = true;
                        FarActorRenderInfo->mTranslate = RenderInfo->mTranslate;
                        FarActorRenderInfo->mRotate = RenderInfo->mRotate;
                        FarActorRenderInfo->mScale = RenderInfo->mScale;
                        FarActorRenderInfo->mEntity->mBancType = RenderInfo->mEntity->mBancType;
                        FarActorRenderInfo->mEntity->mPhive.mPlacement["ID"] = FarActorRenderInfo->mEntity->mHash;

                        RenderInfo->mEntity->mLinks.push_back(application::game::BancEntity::Link{ RenderInfo->mEntity->mHash, FarActorRenderInfo->mEntity->mHash, "Reference", "Reference" });
                        
                        mScene.SyncBancEntity(FarActorRenderInfo);
                    }
                    if (!FoundFarActor || HasFarActor)
                        ImGui::EndDisabled();

                    ImGui::Columns(2, "Far");
                    if (HasFarActor)
                        ImGui::BeginDisabled();
                    if (ImGui::Button("Add far actor", ImVec2(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize, 0)))
                    {
                        gAddFarActorPopUp.Open([this, &RenderInfo](application::rendering::popup::PopUpBuilder& Builder)
                            {
                                application::game::Scene::BancEntityRenderInfo* FarActorRenderInfo = mScene.AddBancEntity(*reinterpret_cast<std::string*>(Builder.GetDataStorage(0).mPtr));
                                RenderInfo = GetSelectedActor();

                                FarActorRenderInfo->mEntity->mBakeable = true;
                                FarActorRenderInfo->mTranslate = RenderInfo->mTranslate;
                                FarActorRenderInfo->mRotate = RenderInfo->mRotate;
                                FarActorRenderInfo->mScale = RenderInfo->mScale;
                                FarActorRenderInfo->mEntity->mBancType = RenderInfo->mEntity->mBancType;
                                FarActorRenderInfo->mEntity->mPhive.mPlacement["ID"] = FarActorRenderInfo->mEntity->mHash;

                                RenderInfo->mEntity->mLinks.push_back(application::game::BancEntity::Link{ RenderInfo->mEntity->mHash, FarActorRenderInfo->mEntity->mHash, "Reference", "Reference" });
                                
                                mScene.SyncBancEntity(FarActorRenderInfo);
                            });
                    }
                    ImGui::NextColumn();
                    if (HasFarActor)
                        ImGui::EndDisabled();
                    else
                        ImGui::BeginDisabled();
                    if (ImGuiExt::ButtonDelete("Delete far actor", ImVec2(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize, 0)))
                    {
                        mScene.DeleteBancEntity(LinkedFarActor);
                        RenderInfo = GetSelectedActor();

                        HasFarActor = false;
                        LinkedFarActor = nullptr;
                        
                        RenderInfo->mEntity->mLinks.erase(
                            std::remove_if(RenderInfo->mEntity->mLinks.begin(), RenderInfo->mEntity->mLinks.end(),
                                [ActorLink](const application::game::BancEntity::Link& Link) { return &Link == ActorLink; }),
                            RenderInfo->mEntity->mLinks.end());
                    }
                    else
                    {
                        if (!HasFarActor)
                            ImGui::EndDisabled();
                    }

                    ImGui::Columns();

                    ImGui::SetCursorPosX(ImGui::GetStyle().ItemSpacing.x);
                    if (!HasFarActor)
                        ImGui::BeginDisabled();
                    if (ImGui::Button("Select far actor", ImVec2(ImGui::GetWindowSize().x - ImGui::GetStyle().ScrollbarSize, 0)))
                    {
                        SelectActor(LinkedFarActor->mEntityIndex);
                        ImGui::End();
                        return;
                    }
                    if (!HasFarActor)
                        ImGui::EndDisabled();

                    ImGui::Columns(2, "FarLinkInformation");
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Far actor linked");
                    ImGui::NextColumn();
                    if (HasFarActor)
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 1, 0, 1));
                    else
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0, 0, 1));
                    ImGui::Text(HasFarActor ? "Yes" : "No");
                    ImGui::PopStyleColor();
                    ImGui::Columns();
                }
            }

            if (ImGui::CollapsingHeader("AINB Info"))
            {
                ImGui::Indent();
                for (auto& AiGroup : mScene.mAiGroups)
                {
                    for (auto& Reference : AiGroup.mReferences)
                    {
                        if(Reference.mReference == RenderInfo->mEntity->mHash)
                        {
                            ImGui::Text("AINB Group: %s", AiGroup.mPath.c_str());
						}
                    }
                }
                ImGui::Unindent();
            }

            ImGui::Text("Needs physics: %s", RenderInfo->mEntity->mActorPack->mNeedsPhysicsHash ? "true" : "false");
            ImGui::Text("Check for hash generation: %s", RenderInfo->mEntity->mCheckForHashGeneration ? "true" : "false");
            ImGui::Text("Distance: %f", application::util::Math::CalculateScreenRadius(RenderInfo->mTranslate, RenderInfo->mEntity->mBfresRenderer->mSphereBoundingBoxRadius, mCamera.mPosition, 45.0f, mCamera.mHeight));

            if (SyncBancEntity)
                mScene.SyncBancEntity(RenderInfo);
        }
        else if (mEditingMode == EditingMode::TERRAIN)
        {
            mTerrainEditor.DrawMenu(this);
        }

        ImGui::End();
    }

    bool UIMapEditor::IsBancEntityRenderInfoCulled(const application::game::Scene::BancEntityRenderInfo& RenderInfo)
    {
        if (RenderInfo.mEntity->mBfresRenderer->mBfresFile->mDefaultModel && !mRenderSettings.mRenderInvisibleEntities) return true;
        if (!RenderInfo.mEntity->mBfresRenderer->mBfresFile->mDefaultModel && !mRenderSettings.mRenderVisibleEntities) return true;
        if (RenderInfo.mEntity->mBfresRenderer->mBfresFile->mDefaultModel && RenderInfo.mEntity->mBfresRenderer->mIsSystemModelTransparent && !mRenderSettings.mRenderAreas) return true;
        if (!RenderInfo.mEntity->mBfresRenderer->mBfresFile->mDefaultModel && !mRenderSettings.mRenderFarEntities && RenderInfo.mEntity->mGyml.ends_with("_Far")) return true;

        return false;
    }

    void UIMapEditor::DrawInstancedBancEntityRenderInfo(std::vector<application::game::Scene::BancEntityRenderInfo>& Entities)
    {
        if (Entities.empty()) return;

        if (IsBancEntityRenderInfoCulled(Entities[0])) return;

        std::vector<glm::mat4> InstanceMatrices(Entities.size());

        application::game::Scene::BancEntityRenderInfo* SelectedActor = GetSelectedActor();

        int Shrinking = 0;
        for (size_t i = 0; i < Entities.size(); i++)
        {
            application::game::Scene::BancEntityRenderInfo& RenderInfo = Entities[i];

            RenderInfo.mSphereScreenSize = application::util::Math::CalculateScreenRadius(RenderInfo.mTranslate, RenderInfo.mEntity->mBfresRenderer->mSphereBoundingBoxRadius, mCamera.mPosition, 45.0f, mCamera.mHeight);
            RenderInfo.mSphereScreenSize *= MATH_MAX(RenderInfo.mScale.x, MATH_MAX(RenderInfo.mScale.y, RenderInfo.mScale.z));

            if ((SelectedActor != nullptr && Entities[i].mMergedActorParent == GetSelectedActor()->mEntity) || mSelectedActorIndex == Entities[i].mEntityIndex || (!RenderInfo.mEntity->mBfresRenderer->mIsSystemModelTransparent && RenderInfo.mSphereScreenSize < 5.0f) || (!mRenderSettings.mRenderMergedActors && RenderInfo.mMergedActorParent != nullptr) || !mFrustum.SphereInFrustum(RenderInfo.mTranslate.x, RenderInfo.mTranslate.y, RenderInfo.mTranslate.z, RenderInfo.mEntity->mBfresRenderer->mBfresFile->Models.GetByIndex(0).mValue.BoundingBoxSphereRadius * std::fmax(std::fmax(RenderInfo.mScale.x, RenderInfo.mScale.y), RenderInfo.mScale.z)))
            {
                Shrinking++;
                continue;
            }

            InstanceMatrices[i - Shrinking] = glm::mat4(1.0f); // Identity matrix

            InstanceMatrices[i - Shrinking] = glm::translate(InstanceMatrices[i - Shrinking], RenderInfo.mTranslate);

            InstanceMatrices[i - Shrinking] = glm::rotate(InstanceMatrices[i - Shrinking], glm::radians(RenderInfo.mRotate.z), glm::vec3(0.0, 0.0f, 1.0));
            InstanceMatrices[i - Shrinking] = glm::rotate(InstanceMatrices[i - Shrinking], glm::radians(RenderInfo.mRotate.y), glm::vec3(0.0f, 1.0, 0.0));
            InstanceMatrices[i - Shrinking] = glm::rotate(InstanceMatrices[i - Shrinking], glm::radians(RenderInfo.mRotate.x), glm::vec3(1.0, 0.0f, 0.0));

            InstanceMatrices[i - Shrinking] = glm::scale(InstanceMatrices[i - Shrinking], RenderInfo.mScale);
        }

        if (Shrinking > 0)
            InstanceMatrices.resize(InstanceMatrices.size() - Shrinking);

        Entities[0].mEntity->mBfresRenderer->Draw(InstanceMatrices);
    }

    void UIMapEditor::DrawBancEntityRenderInfo(application::game::Scene::BancEntityRenderInfo& RenderInfo)
    {
        std::vector<glm::mat4> InstanceMatrix(1);

        glm::mat4& GLMModel = InstanceMatrix[0];
        GLMModel = glm::mat4(1.0f); // Identity matrix

        GLMModel = glm::translate(GLMModel, RenderInfo.mTranslate);

        GLMModel = glm::rotate(GLMModel, glm::radians(RenderInfo.mRotate.z), glm::vec3(0.0, 0.0f, 1.0));
        GLMModel = glm::rotate(GLMModel, glm::radians(RenderInfo.mRotate.y), glm::vec3(0.0f, 1.0, 0.0));
        GLMModel = glm::rotate(GLMModel, glm::radians(RenderInfo.mRotate.x), glm::vec3(1.0, 0.0f, 0.0));

        GLMModel = glm::scale(GLMModel, RenderInfo.mScale);

        RenderInfo.mEntity->mBfresRenderer->Draw(InstanceMatrix);

        //Render merged actors if any
        if (RenderInfo.mEntity->mMergedActorChildren != nullptr && !RenderInfo.mEntity->mMergedActorChildren->empty())
        {
            for (application::game::Scene::BancEntityRenderInfo* ChildRenderInfo : mScene.mDrawListRenderInfoIndices)
            {
				if (ChildRenderInfo->mMergedActorParent != RenderInfo.mEntity) continue;

				DrawBancEntityRenderInfo(*ChildRenderInfo);
            }
        }
    }

    application::game::Scene::BancEntityRenderInfo* UIMapEditor::GetSelectedActor()
    {
        if (mSelectedActorIndex < 0 || mSelectedActorIndex >= mScene.mDrawListRenderInfoIndices.size())
            return nullptr;

        return mScene.mDrawListRenderInfoIndices[mSelectedActorIndex];
    }

    bool UIMapEditor::IsAnyActorSelected()
    {
        return mSelectedActorIndex >= 0 && mScene.mDrawListRenderInfoIndices.size() > mSelectedActorIndex;
    }

    void UIMapEditor::DeselectActor()
    {
        mSelectedActorIndex = -1;
    }

    void UIMapEditor::SelectActor(int32_t NewIndex)
    {
        mSelectedActorIndex = NewIndex;

        mPropertiesWindowInfo.mSetDynamicColumnWidth = true;
        mPropertiesWindowInfo.mSetExternalParametersColumnWidth = true;
        mPropertiesWindowInfo.mSetPresenceColumnWidth = true;
        mPropertiesWindowInfo.mSetPhivePlacementColumnWidth = true;
    }

    void UIMapEditor::SelectNearestMergedBancEntityInstance(std::vector<application::game::BancEntity>* MergedActor, application::game::BancEntity& Entity, const glm::vec3& Pos)
    {
        std::vector<application::game::Scene::BancEntityRenderInfo*> RenderInfos;

        for (application::game::BancEntity& ParentEntity : mScene.mBancEntities)
        {
            if (ParentEntity.mMergedActorChildren != MergedActor)
                continue;

            for (application::game::Scene::BancEntityRenderInfo& RenderInfo : mScene.mDrawList[Entity.mBfresRenderer])
            {
                if (RenderInfo.mEntity->mHash == Entity.mHash && RenderInfo.mEntity->mSRTHash == Entity.mSRTHash)
                {
                    RenderInfos.push_back(&RenderInfo);
                }
            }
        }

        if (RenderInfos.empty())
            return;

        float MinDistance = FLT_MAX;
        application::game::Scene::BancEntityRenderInfo* TargetRenderInfo = RenderInfos[0];
        for (application::game::Scene::BancEntityRenderInfo* RenderInfo : RenderInfos)
        {
            float Distance = glm::distance2(Pos, RenderInfo->mTranslate);
            if (MinDistance > Distance)
            {
                MinDistance = Distance;
                TargetRenderInfo = RenderInfo;
            }
        }

        SelectActor(TargetRenderInfo->mEntityIndex);
    }

    void UIMapEditor::RenderPickingFramebuffer(const ImVec2& WindowSize, const ImVec2& MousePos)
    {
        if (!mRenderSettings.mAllowActorSelection || mActorPainter.mEnabled || mTerrainEditor.mEnabled) return;
        if (glfwGetMouseButton(application::manager::UIMgr::gWindow, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && glfwGetMouseButton(application::manager::UIMgr::gWindow, GLFW_MOUSE_BUTTON_RIGHT) != GLFW_PRESS && !ImGui::IsAnyItemHovered())
        {
            if (!(WindowSize.x > MousePos.x && WindowSize.y > MousePos.y && MousePos.x > 0 && MousePos.y > 0)) return;
            if (ImGuizmo::IsOver() && IsAnyActorSelected()) return;

            mSceneWindowFramebuffer->Bind();
            glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

            for (auto& [Renderer, Entities] : mScene.mDrawList)
            {
                if (Entities.empty() || IsBancEntityRenderInfoCulled(Entities[0])) continue;

                for (application::game::Scene::BancEntityRenderInfo& Info : Entities)
                {
                    if ((!Info.mEntity->mBfresRenderer->mIsSystemModelTransparent && Info.mSphereScreenSize < 5.0f) || (!mRenderSettings.mRenderMergedActors && Info.mMergedActorParent != nullptr) || !mFrustum.SphereInFrustum(Info.mTranslate.x, Info.mTranslate.y, Info.mTranslate.z, Info.mEntity->mBfresRenderer->mBfresFile->Models.GetByIndex(0).mValue.BoundingBoxSphereRadius * std::fmax(std::fmax(Info.mScale.x, Info.mScale.y), Info.mScale.z)))
                        continue;

                    int R = (Info.mEntityIndex & 0x000000FF) >> 0;
                    int G = (Info.mEntityIndex & 0x0000FF00) >> 8;
                    int B = (Info.mEntityIndex & 0x00FF0000) >> 16;

                    glUniform4f(glGetUniformLocation(gPickingShader->mID, "PickingColor"), R / 255.0f, G / 255.0f, B / 255.0f, 1.0f);


                    std::vector<glm::mat4> InstanceMatrix(1);

                    glm::mat4& GLMModel = InstanceMatrix[0];
                    GLMModel = glm::mat4(1.0f); // Identity matrix

                    GLMModel = glm::translate(GLMModel, Info.mTranslate);

                    GLMModel = glm::rotate(GLMModel, glm::radians(Info.mRotate.z), glm::vec3(0.0, 0.0f, 1.0));
                    GLMModel = glm::rotate(GLMModel, glm::radians(Info.mRotate.y), glm::vec3(0.0f, 1.0, 0.0));
                    GLMModel = glm::rotate(GLMModel, glm::radians(Info.mRotate.x), glm::vec3(1.0, 0.0f, 0.0));

                    GLMModel = glm::scale(GLMModel, Info.mScale);

                    Info.mEntity->mBfresRenderer->Draw(InstanceMatrix);
                }
            }

            glReadBuffer(GL_COLOR_ATTACHMENT0);
            unsigned char Data[3];

            glReadPixels((GLint)MousePos.x, (GLint)WindowSize.y - MousePos.y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, Data);

            unsigned int PickedActorId =
                Data[0] +
                Data[1] * 256 +
                Data[2] * 256 * 256;

            if (PickedActorId >= mScene.mDrawListIndices.size())
            {
                DeselectActor();
            }
            else
            {
                SelectActor(PickedActorId);
            }

            glReadBuffer(GL_NONE);
        }
    }

    void UIMapEditor::EnterPlayMode()
    {
        DeselectActor();

        mPlaySession = std::make_unique<application::play::PlaySession>(gInstancedShader);
        mPlaySession->InitializeFromScene(mScene, mCamera.mPosition);

        mPlaySession->mCamera.mWidth = mCamera.mWidth;
        mPlaySession->mCamera.mHeight = mCamera.mHeight;

        mEditorMode = EditorMode::PLAY;
    }

    void UIMapEditor::ExitPlayMode()
    {
		mCamera.mPosition = mPlaySession->mCamera.mPosition;
		mCamera.mOrientation = mPlaySession->mCamera.mOrientation;
		mCamera.mCameraMatrix = mPlaySession->mCamera.mCameraMatrix;
		mCamera.mView = mPlaySession->mCamera.mView;
		mCamera.mProjection = mPlaySession->mCamera.mProjection;


        mPlaySession->Shutdown();
        mPlaySession.reset();

        mEditorMode = EditorMode::EDIT;
	}

    void UIMapEditor::DrawOverlay()
    {
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        ImVec2 windowPos = ImGui::GetCursorScreenPos();
        ImVec2 windowSize = ImGui::GetWindowSize();
        ImVec2 padding = ImGui::GetStyle().WindowPadding;

        ImVec2 buttonSize(90.0f, 26.0f);

        ImVec2 buttonPos(
            windowPos.x + (windowSize.x - buttonSize.x) * 0.5f,
            windowPos.y + padding.y + 4.0f
        );

        // Use absolute positioning without affecting layout
        ImGui::SetCursorScreenPos(buttonPos);
        ImGui::SetNextItemAllowOverlap();

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.55f, 0.25f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.65f, 0.30f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.30f, 0.75f, 0.35f, 1.0f));

        if (ImGui::Button(ICON_FA_PLAY " Play", buttonSize))
        {
            EnterPlayMode();
        }

        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar();

        ImGui::SetCursorScreenPos(windowPos);

        ImGui::Text("FPS: %.1f (%.3f ms/frame)", ImGui::GetIO().Framerate, 1000.0 / ImGui::GetIO().Framerate);

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.5f));
        if (ImGui::Button(std::string("Mode: " + (mGuizmoMode == ImGuizmo::WORLD ? std::string("World") : std::string("Local"))).c_str()))
        {
            if (mGuizmoMode == ImGuizmo::WORLD) mGuizmoMode = ImGuizmo::LOCAL;
            else mGuizmoMode = ImGuizmo::WORLD;
        }
        ImGui::PopStyleColor();

        ImGui::SameLine();

        //ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_Button, mGuizmoOperation == ImGuizmo::TRANSLATE ? ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered] : ImVec4(0.0f, 0.0f, 0.0f, 0.5f));
        if (ImGui::ImageButton("TranslateGizmoButton", (ImTextureID)application::manager::TextureMgr::GetAssetTexture("TranslateGizmo")->mID, ImVec2(16.0f * ImGui::GetPlatformIO().Monitors[0].DpiScale, 16.0f * ImGui::GetPlatformIO().Monitors[0].DpiScale), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f)))
        {
            mGuizmoOperation = ImGuizmo::TRANSLATE;
        }
        ImGui::SetItemTooltip("Translate Gizmo (G)");
        ImGui::SameLine();
        ImGui::PopStyleColor();

        ImGui::PushStyleColor(ImGuiCol_Button, mGuizmoOperation == ImGuizmo::ROTATE ? ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered] : ImVec4(0.0f, 0.0f, 0.0f, 0.5f));
        if (ImGui::ImageButton("RotateGizmoButton", (ImTextureID)application::manager::TextureMgr::GetAssetTexture("RotateGizmo")->mID, ImVec2(16.0f * ImGui::GetPlatformIO().Monitors[0].DpiScale, 16.0f * ImGui::GetPlatformIO().Monitors[0].DpiScale), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f)))
        {
            mGuizmoOperation = ImGuizmo::ROTATE;
        }
        ImGui::SetItemTooltip("Rotate Gizmo (R)");
        ImGui::SameLine();
        ImGui::PopStyleColor();

        ImGui::PushStyleColor(ImGuiCol_Button, mGuizmoOperation == ImGuizmo::SCALE ? ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered] : ImVec4(0.0f, 0.0f, 0.0f, 0.5f));
        if (ImGui::ImageButton("ScaleGizmoButton", (ImTextureID)application::manager::TextureMgr::GetAssetTexture("ScaleGizmo")->mID, ImVec2(16.0f * ImGui::GetPlatformIO().Monitors[0].DpiScale, 16.0f * ImGui::GetPlatformIO().Monitors[0].DpiScale), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f)))
        {
            mGuizmoOperation = ImGuizmo::SCALE;
        }
        ImGui::SetItemTooltip("Scale Gizmo (S)");
        ImGui::PopStyleColor();

        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.5f));
        static const char* EditingModes[]{ "Entity", "Actor Painter", "Terrain" };
        ImGui::PushItemWidth(ImGui::CalcTextSize("Mode: Actor Painter").x + ImGui::GetFrameHeight() + ImGui::GetStyle().FramePadding.x + ImGui::GetStyle().ItemInnerSpacing.x);
        if (ImGui::BeginCombo("##EditingMode", ("Mode: " + std::string(EditingModes[*reinterpret_cast<int*>(&mEditingMode)])).c_str()))
        {
            bool Update = false;
            for (int n = 0; n < IM_ARRAYSIZE(EditingModes); n++)
            {
                bool IsSlected = (*reinterpret_cast<int*>(&mEditingMode) == n);
                if (ImGui::Selectable(EditingModes[n], IsSlected))
                {
                    *reinterpret_cast<int*>(&mEditingMode) = n;
                    Update = true;
                }

                if (IsSlected)
                    ImGui::SetItemDefaultFocus();
            }

            if (Update)
            {
                mActorPainter.SetEnabled(mEditingMode == EditingMode::ACTOR_PAINTER, this);
				mTerrainEditor.SetEnabled(mEditingMode == EditingMode::TERRAIN, this);
            }

            ImGui::EndCombo();
        }
        ImGui::PopItemWidth();

        float w = ImGui::GetCursorPosX();
        if (ImGui::Button("Actor Painter##ActorPainterButton"))
        {
            ImGui::OpenPopup("ActorPainter");
        }
        ImVec2 Pos = ImGui::GetCursorScreenPos();
        ImGui::PopStyleColor();
        mActorPainter.DrawMenu(w, Pos, this);
    }

    void UIMapEditor::DrawViewportWindow()
    {
        bool Focused = ImGui::Begin(CreateID("Scene").c_str());

        if (!Focused)
        {
            ImGui::End();
            return;
        }

        if (mEditorMode == EditorMode::PLAY)
        {
            mRenderSettings.mAllowActorSelection = false;
            glActiveTexture(GL_TEXTURE0);
            ImGuizmo::Enable(false);

            ImVec2 Pos = ImGui::GetCursorScreenPos();
            const ImVec2 WindowSize = ImGui::GetContentRegionAvail();

            if (mSceneWindowSize.x != WindowSize.x || mSceneWindowSize.y != WindowSize.y)
            {
                //Rescaling framebuffer
                mSceneWindowSize = WindowSize;
                mSceneWindowFramebuffer->RescaleFramebuffer(WindowSize.x, WindowSize.y);
                mPlaySession->mCamera.mWidth = WindowSize.x;
                mPlaySession->mCamera.mHeight = WindowSize.y;

                mCamera.mWidth = WindowSize.x;
                mCamera.mHeight = WindowSize.y;
            }
            else
            {
                //Bind
                mSceneWindowFramebuffer->Bind();
            }

            mPlaySession->mCamera.mWindowHovered = (ImGui::IsWindowHovered() && !ImGui::IsItemHovered());

            glViewport(0, 0, WindowSize.x, WindowSize.y);
            glClearColor(mClearColor.x * mClearColor.w, mClearColor.y * mClearColor.w, mClearColor.z * mClearColor.w, mClearColor.w);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

            //Render game simulation
            mPlaySession->Update(ImGui::GetIO().DeltaTime);
            mPlaySession->Render();

            ImGui::GetWindowDrawList()->AddImage(
                (ImTextureID)mSceneWindowFramebuffer->GetFramebufferTexture(),
                ImVec2(Pos.x, Pos.y),
                ImVec2(Pos.x + WindowSize.x, Pos.y + WindowSize.y),
                ImVec2(0, 1),
                ImVec2(1, 0)
            );

            ImGui::Text("FPS: %.1f (%.3f ms/frame)", ImGui::GetIO().Framerate, 1000.0 / ImGui::GetIO().Framerate);

            mSceneWindowFramebuffer->Unbind();

            if (ImGui::IsKeyDown(ImGuiKey_Escape))
            {
                ExitPlayMode();
            }

            ImGui::End();

            return;
        }

        mRenderSettings.mAllowActorSelection = !ImGui::GetIO().WantTextInput;
        if (mRenderSettings.mAllowActorSelection)
        {
            for (application::rendering::popup::PopUpBuilder* Builder : application::manager::PopUpMgr::gPopUps)
            {
                if (Builder->IsOpen())
                {
                    mRenderSettings.mAllowActorSelection = false;
                    break;
                }
            }
        }

        glActiveTexture(GL_TEXTURE0);

        ImGuizmo::Enable(mRenderSettings.mAllowActorSelection);
        ImGuizmo::SetDrawlist();

        ImVec2 Pos = ImGui::GetCursorScreenPos();

        const ImVec2 WindowSize = ImGui::GetContentRegionAvail();
        if (mSceneWindowSize.x != WindowSize.x || mSceneWindowSize.y != WindowSize.y)
        {
            //Rescaling framebuffer
            mSceneWindowSize = WindowSize;
            mSceneWindowFramebuffer->RescaleFramebuffer(WindowSize.x, WindowSize.y);
            mCamera.mWidth = WindowSize.x;
            mCamera.mHeight = WindowSize.y;
        }
        else
        {
            //Bind
            mSceneWindowFramebuffer->Bind();
        }

        mCamera.mWindowHovered = (ImGui::IsWindowHovered() && !ImGui::IsItemHovered());

        glViewport(0, 0, WindowSize.x, WindowSize.y);
        glClearColor(mClearColor.x * mClearColor.w, mClearColor.y * mClearColor.w, mClearColor.z * mClearColor.w, mClearColor.w);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        ImVec2 WindowPos = ImGui::GetWindowPos();
        mCamera.Inputs(ImGui::GetIO().Framerate, glm::vec2(WindowPos.x, WindowPos.y));
        mCamera.UpdateMatrix(45.0f, 0.1f, 30000.0f);

        gPickingShader->Bind();
        mCamera.Matrix(gPickingShader, "camMatrix");
        ImVec2 ScreenSize = ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y);
        ImVec2 MousePos = ImVec2(ImGui::GetMousePos().x - ImGui::GetWindowPos().x - ImGui::GetWindowContentRegionMin().x,
            ImGui::GetMousePos().y - ImGui::GetWindowPos().y - ImGui::GetWindowContentRegionMin().y);
        RenderPickingFramebuffer(ScreenSize, MousePos);

        glClearColor(mClearColor.x * mClearColor.w, mClearColor.y * mClearColor.w, mClearColor.z * mClearColor.w, mClearColor.w);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        gInstancedShader->Bind();
        mCamera.Matrix(gInstancedShader, "camMatrix");
        glUniform3fv(glGetUniformLocation(gInstancedShader->mID, "lightColor"), 1, &gLightColor[0]);
        glUniform3fv(glGetUniformLocation(gInstancedShader->mID, "lightPos"), 1, &mCamera.mPosition[0]);

        mFrustum.CalculateFrustum(mCamera);

        for (auto& [Model, Entities] : mScene.mDrawList)
        {
            DrawInstancedBancEntityRenderInfo(Entities);
        }

        if (application::game::Scene::BancEntityRenderInfo* Info = GetSelectedActor(); Info != nullptr)
        {
            gSelectedShader->Bind();
            mCamera.Matrix(gSelectedShader, "camMatrix");
            glUniform3fv(glGetUniformLocation(gSelectedShader->mID, "lightColor"), 1, &gLightColor[0]);
            glUniform3fv(glGetUniformLocation(gSelectedShader->mID, "lightPos"), 1, &mCamera.mPosition[0]);
            DrawBancEntityRenderInfo(*Info);
        }

        if (mRenderSettings.mRenderNavMesh)
        {
            if (mRenderSettings.mRenderNavMeshWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            mScene.mNavMeshImplementation->DrawNavMesh(mCamera);
            if (mRenderSettings.mRenderNavMeshWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        if(mScene.mTerrainRenderer != nullptr && mRenderSettings.mRenderTerrain)
        {
            if (mRenderSettings.mRenderTerrainWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            mScene.mTerrainRenderer->Draw(mCamera);
            if (mRenderSettings.mRenderTerrainWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}

        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        for (application::gl::SimpleMesh& Mesh : mScene.mDebugMeshes)
        {
            gNavMeshShader->Bind();
            mCamera.Matrix(gNavMeshShader, "camMatrix");
            glm::mat4 GLMModel = glm::mat4(1.0f);
            glUniformMatrix4fv(glGetUniformLocation(gNavMeshShader->mID, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(GLMModel));
            Mesh.Draw();
        }
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        if(mScene.mStaticCompoundImplementation != nullptr && mRenderSettings.mRenderTerrainCollision)
        {
            if (mRenderSettings.mRenderTerrainCollisionWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            mScene.mStaticCompoundImplementation->DrawStaticCompound(mCamera);
            if (mRenderSettings.mRenderTerrainCollisionWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        for (application::game::BancEntity& Entity : mScene.mBancEntities)
        {
            if (application::manager::CaveMgr::Cave* Cave = Entity.mActorPack->mCave; Cave != nullptr)
            {
                Cave->mRenderer.PrepareGlobal(mCamera);

                glm::mat4 GLMModel = glm::mat4(1.0f);

                GLMModel = glm::translate(GLMModel, Entity.mTranslate);

                GLMModel = glm::rotate(GLMModel, glm::radians(Entity.mRotate.z), glm::vec3(0.0, 0.0f, 1.0));
                GLMModel = glm::rotate(GLMModel, glm::radians(Entity.mRotate.y), glm::vec3(0.0f, 1.0, 0.0));
                GLMModel = glm::rotate(GLMModel, glm::radians(Entity.mRotate.x), glm::vec3(1.0, 0.0f, 0.0));

                GLMModel = glm::scale(GLMModel, Entity.mScale);

                Cave->mRenderer.Draw(GLMModel);
            }
        }
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        mActorPainter.RenderActorPainter(MousePos.x, MousePos.y, ScreenSize.x, ScreenSize.y, this);
        mTerrainEditor.RenderTerrainEditor(MousePos.x, MousePos.y, ScreenSize.x, ScreenSize.y, this);

        ImGui::GetWindowDrawList()->AddImage(
            (ImTextureID)mSceneWindowFramebuffer->GetFramebufferTexture(),
            ImVec2(Pos.x, Pos.y),
            ImVec2(Pos.x + WindowSize.x, Pos.y + WindowSize.y),
            ImVec2(0, 1),
            ImVec2(1, 0)
        );

        DrawOverlay();

        if (application::game::Scene::BancEntityRenderInfo* Info = GetSelectedActor(); Info != nullptr)
        {
            ImGuizmo::RecomposeMatrixFromComponents(&Info->mTranslate.x, &Info->mRotate.x, &Info->mScale.x, mGuizmoObjectMatrix);

            ImGuizmo::SetRect(ImGui::GetWindowPos().x + ImGui::GetStyle().WindowPadding.x, ImGui::GetWindowPos().y + ImGui::GetStyle().WindowPadding.y, WindowSize.x, WindowSize.y);

            ImGuizmo::Manipulate(glm::value_ptr(mCamera.GetViewMatrix()), glm::value_ptr(mCamera.GetProjectionMatrix()), mGuizmoOperation, mGuizmoMode, mGuizmoObjectMatrix);
        }

        if (ImGuizmo::IsUsingAny())
        {
            application::game::Scene::BancEntityRenderInfo* RenderInfo = GetSelectedActor();
            ImGuizmo::DecomposeMatrixToComponents(mGuizmoObjectMatrix, &RenderInfo->mTranslate.x, &RenderInfo->mRotate.x, &RenderInfo->mScale.x);
            mScene.SyncBancEntity(RenderInfo);
        }

        mSceneWindowFramebuffer->Unbind();

        ImGui::End();
    }

    void UIMapEditor::DrawImpl()
    {
        if (mFirstFrame)
            ImGui::SetNextWindowDockID(application::manager::UIMgr::gDockMain);

        bool Focused = ImGui::Begin(GetWindowTitle().c_str(), &mOpen, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar);

        if (!Focused)
        {
            ImGui::End();
            return;
        }

        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu(CreateID("Scene").c_str()))
            {
                if (ImGui::MenuItem(CreateID("New").c_str()))
                {
                    gNewScenePopUp.Open([this](application::rendering::popup::PopUpBuilder& Builder)
                        {
                            std::string Identifier = *reinterpret_cast<std::string*>(Builder.GetDataStorage(0).mPtr);
                            application::tool::SceneCreator::CreateScene(Identifier);

                            mScene.Load(application::game::Scene::SceneType::SMALL_DUNGEON, "Dungeon" + Identifier);

                            if (!mScene.mBancEntities.empty())
                            {
                                mCamera.mPosition = mScene.mBancEntities[0].mTranslate;
                            }
                        });
                }
                if (ImGui::MenuItem(CreateID("Load").c_str()))
                {
                    gLoadScenePopUp.Open([this](application::rendering::popup::PopUpBuilder& Builder)
                        {
                            std::string Identifier = *reinterpret_cast<std::string*>(Builder.GetDataStorage(1).mPtr);
                            application::game::Scene::SceneType Type = static_cast<application::game::Scene::SceneType>((uint8_t)*reinterpret_cast<uint32_t*>(Builder.GetDataStorage(0).mPtr) + 1);

                            switch (Type)
                            {
                            case application::game::Scene::SceneType::SMALL_DUNGEON:
                                Identifier = "Dungeon" + Identifier;
                                break;
                            default:
                                break;
                            }

                            mScene.Load(Type, Identifier);

                            if (!mScene.mBancEntities.empty())
                            {
                                mCamera.mPosition = mScene.mBancEntities[0].mTranslate;
                            }
                        });
                }
                if (ImGui::MenuItem(CreateID("Save").c_str()))
                {
                    mScene.Save(application::util::FileUtil::GetSaveFilePath());

                    ImGui::InsertNotification({ ImGuiToastType::Success, 3000, ("Scene " + mScene.mBcettName + " saved successfully.").c_str() });
                }
                ImGui::EndMenu();
            }

            if (ImGui::MenuItem("Ai Groups"))
            {
                mUIAiLinks.Open(&mScene);
            }

            if (application::manager::ProjectMgr::gIsTrialOfTheChosenHeroProject)
            {
                if (ImGui::BeginMenu(CreateID("Git").c_str()))
                {
                    if (ImGui::MenuItem("Update project"))
                    {
                        auto UpdateProject = []()
                            {
                                application::util::GitManager Git(application::util::FileUtil::GetSaveFilePath());

                                application::util::GitManager::OperationResult OpenRepoResult = Git.OpenRepository();

                                if (OpenRepoResult.mSuccess)
                                {
                                    auto UpdateResult = Git.UpdateProject();
                                    ImGui::InsertNotification({ static_cast<ImGuiToastType>(UpdateResult.mToastType), 3000, UpdateResult.mMessage.c_str() });
                                }
                                else
                                {
                                    ImGui::InsertNotification({ static_cast<ImGuiToastType>(OpenRepoResult.mToastType), 3000, OpenRepoResult.mMessage.c_str() });
                                }
                            };

                        if (application::util::GitManager::gUserIdentity.mAccessToken.empty())
                        {
                            gConfigureGitIdentityPopUp.Open([this, &UpdateProject](application::rendering::popup::PopUpBuilder& Builder)
                                {
                                    UpdateProject();
                                });
                        }
                        else
                        {
                            UpdateProject();
                        }
                    }

                    if (ImGui::MenuItem("Push project"))
                    {
                        auto PushProject = []()
                            {
                                application::util::GitManager Git(application::util::FileUtil::GetSaveFilePath());

                                application::util::GitManager::OperationResult OpenRepoResult = Git.OpenRepository();

                                if (OpenRepoResult.mSuccess)
                                {
                                    auto PushResult = Git.PushProject();
                                    ImGui::InsertNotification({ static_cast<ImGuiToastType>(PushResult.mToastType), 3000, PushResult.mMessage.c_str() });
                                }
                                else
                                {
                                    ImGui::InsertNotification({ static_cast<ImGuiToastType>(OpenRepoResult.mToastType), 3000, OpenRepoResult.mMessage.c_str() });
                                }
                            };

                        if (application::util::GitManager::gUserIdentity.mAccessToken.empty())
                        {
                            gConfigureGitIdentityPopUp.Open([this, &PushProject](application::rendering::popup::PopUpBuilder& Builder)
                                {
                                    PushProject();
                                });
                        }
                        else
                        {
                            PushProject();
                        }
                    }
                    ImGui::EndMenu();
                }
            }

            ImGui::EndMenuBar();
        }

        ImGuiID DockspaceId = ImGui::GetID(GetWindowTitle().c_str());
        ImGui::DockSpace(DockspaceId);

        if (mFirstFrame)
        {
            ImGui::DockBuilderRemoveNode(DockspaceId);
            ImGui::DockBuilderAddNode(DockspaceId, ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(DockspaceId, ImGui::GetWindowSize());

            ImGuiID DockLeft, DockMiddle, DockRight;
            ImGui::DockBuilderSplitNode(DockspaceId, ImGuiDir_Left, 0.15f, &DockLeft, &DockMiddle);
            ImGui::DockBuilderSplitNode(DockMiddle, ImGuiDir_Right, 0.25f, &DockRight, &DockMiddle);

            ImGuiID DockLeftTop, DockLeftBottom;
            ImGui::DockBuilderSplitNode(DockLeft, ImGuiDir_Up, 0.5f, &DockLeftTop, &DockLeftBottom);

            ImGui::DockBuilderDockWindow(CreateID("Tools").c_str(), DockLeftBottom);
            ImGui::DockBuilderDockWindow(CreateID("Outliner").c_str(), DockLeftTop);

            ImGui::DockBuilderDockWindow(CreateID("Scene").c_str(), DockMiddle);
            ImGui::DockBuilderDockWindow(CreateID("Properties").c_str(), DockRight);

            ImGui::DockBuilderFinish(DockspaceId);
        }

        if (!ImGui::GetIO().WantTextInput && !mCamera.IsInCameraMovement() && mHotkeyBlockFrameCount == 0)
        {
            if (ImGui::IsKeyPressed(ImGuiKey_G))
            {
                mGuizmoOperation = ImGuizmo::OPERATION::TRANSLATE;
            }
            else if (ImGui::IsKeyPressed(ImGuiKey_R))
            {
                mGuizmoOperation = ImGuizmo::OPERATION::ROTATE;
            }
            else if (ImGui::IsKeyPressed(ImGuiKey_S))
            {
                mGuizmoOperation = ImGuizmo::OPERATION::SCALE;
            }
        }

        ImGui::End();

        DrawOutlinerWindow();
        DrawToolsWindow();
        DrawDetailsWindow();
        DrawViewportWindow();

        if (mHotkeyBlockFrameCount > 0)
            mHotkeyBlockFrameCount--;

        if (mCamera.IsInCameraMovement()) mHotkeyBlockFrameCount = 100.0f / (1000.0 / ImGui::GetIO().Framerate); //500ms / ms per frame
    }

    void UIMapEditor::DeleteImpl()
    {
        if (!application::manager::FramebufferMgr::DeleteFramebuffer(mSceneWindowFramebuffer))
        {
            application::util::Logger::Error("MapEditor", "Error while deleting framebuffer");
        }
    }

    UIWindowBase::WindowType UIMapEditor::GetWindowType()
    {
        return UIWindowBase::WindowType::EDITOR_MAP;
    }

    std::string UIMapEditor::GetWindowTitle()
    {
        if (mSameWindowTypeCount > 0)
        {
            return "Map Editor (" + std::to_string(mSameWindowTypeCount) + ")###" + std::to_string(mWindowId);
        }

        return "Map Editor###" + std::to_string(mWindowId);
    }
}