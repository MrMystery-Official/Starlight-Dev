#include "UICollisionGenerator.h"

#include "imgui_internal.h"
#include "imgui_stdlib.h"
#include <manager/UIMgr.h>
#include <util/portable-file-dialogs.h>
#include <manager/BfresFileMgr.h>
#include <manager/BfresRendererMgr.h>
#include <manager/ShaderMgr.h>
#include <manager/FramebufferMgr.h>
#include <manager/ActorPackMgr.h>
#include <util/FileUtil.h>
#include <file/game/zstd/ZStdBackend.h>
#include <file/game/byml/BymlFile.h>
#include <rendering/mapeditor/UIMapEditor.h>
#include <rendering/popup/PopUpCommon.h>
#include <game/actor_component/ActorComponentModelInfo.h>
#include <util/ImGuiNotify.h>

namespace application::rendering::collision
{
    application::gl::Shader* UICollisionGenerator::gInstancedShader = nullptr;
    application::gl::Shader* UICollisionGenerator::gNavMeshShader = nullptr;
    application::gl::Shader* UICollisionGenerator::gSelectedShader = nullptr;

    void UICollisionGenerator::InitializeImpl()
    {
        if (gInstancedShader == nullptr)
            gInstancedShader = application::manager::ShaderMgr::GetShader("Instanced");
        if (gNavMeshShader == nullptr)
            gNavMeshShader = application::manager::ShaderMgr::GetShader("NavMesh");
        if (gSelectedShader == nullptr)
            gSelectedShader = application::manager::ShaderMgr::GetShader("Selected");

        mFramebuffer = application::manager::FramebufferMgr::CreateFramebuffer(1, 1, "CollisionView_" + std::to_string(mWindowId));
        mCamera.mWindow = application::manager::UIMgr::gWindow;
        mCamera.mSpeed = 0.1f;

        mPhiveShape = application::file::game::ZStdBackend::Decompress(application::util::FileUtil::ReadFile(application::util::FileUtil::GetRomFSFilePath("Phive/Shape/Dcc/DgnObj_Small_BoxFloor_4x4_A__Physical.Nin_NX_NVN.bphsh.zs", false)));
        
        //std::vector<float> VerticesRaw = mPhiveShape.ToVertices();
        //std::vector<unsigned int> IndicesRaw = mPhiveShape.ToIndices();
        //mCollisionMesh = application::gl::SimpleMesh(VerticesRaw, IndicesRaw);

        mPhiveShape.mTagFile.mRootClass.mGeometrySections.mElements.clear();

        //LoadActorPack("DgnObj_Small_BoxFloor_A_07");

        /*
        application::file::game::phive::PhiveShape DebugPhiveShape = application::util::FileUtil::ReadFile("C:/Users/X/Desktop/TotKBfresTool/BfresTool/rootDir/WorkingDir/WMapNakaBaseKaiganED.Nin_NX_NVN.bphsh");
        std::vector<float> VerticesRaw = DebugPhiveShape.ToVertices();
        std::vector<unsigned int> IndicesRaw = DebugPhiveShape.ToIndices();
        mCollisionMesh = application::gl::SimpleMesh(VerticesRaw, IndicesRaw);
        */
    }

    void UICollisionGenerator::DrawShapesWindow()
    {
        ImGui::Begin(CreateID("Shapes").c_str());

        for (ShapeEntry& Entry : mShapes)
        {
            if (!Entry.mEnabled)
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
            else if(Entry.mMaterialIndex == -1)
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.2f, 0.2f, 1.0f));

            if (ImGui::Selectable(Entry.mShape->Name.c_str(), mDetailsEditorContent.mType == DetailsEditorContentType::SHAPE && std::get<DetailsEditorContentShape>(mDetailsEditorContent.mContent).mShape == &Entry))
            {
                DetailsEditorContentShape Content;
                Content.mShape = &Entry;

                mDetailsEditorContent.mType = DetailsEditorContentType::SHAPE;
                mDetailsEditorContent.mContent = Content;
            }

            if (!Entry.mEnabled)
                ImGui::PopStyleColor();
            else if (Entry.mMaterialIndex == -1)
                ImGui::PopStyleColor();
        }

        ImGui::End();
    }

    void UICollisionGenerator::DrawMaterialsWindow()
    {
        ImGui::Begin(CreateID("Materials").c_str());

        if (ImGui::Button("Add", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
        {
            MaterialEntry Entry;
            Entry.mName = "Material" + std::to_string(mMaterials.size());
            Entry.mPhiveMaterial.mCollisionFlags[0] = true; //HitAll

            mMaterials.push_back(Entry);

            DetailsEditorContentMaterial Content;
            Content.mMaterial = &mMaterials.back();

            mDetailsEditorContent.mType = DetailsEditorContentType::MATERIAL;
            mDetailsEditorContent.mContent = Content;
        }
        ImGui::Separator();

        for (MaterialEntry& Entry : mMaterials)
        {
            if (ImGui::Selectable((Entry.mName + "##Selectable").c_str(), mDetailsEditorContent.mType == DetailsEditorContentType::MATERIAL && std::get<DetailsEditorContentMaterial>(mDetailsEditorContent.mContent).mMaterial == &Entry))
            {
                DetailsEditorContentMaterial Content;
                Content.mMaterial = &Entry;

                mDetailsEditorContent.mType = DetailsEditorContentType::MATERIAL;
                mDetailsEditorContent.mContent = Content;
            }
        }

        ImGui::End();
    }

    application::file::game::byml::BymlFile GenerateShapeParamFile(const std::string& PhshName, const glm::vec3& Min, const glm::vec3& Max)
    {
        application::file::game::byml::BymlFile Byml;
        Byml.GetType() = application::file::game::byml::BymlFile::Type::Dictionary;

        application::file::game::byml::BymlFile::Node PhshMeshNode(application::file::game::byml::BymlFile::Type::Array, "PhshMesh");

        application::file::game::byml::BymlFile::Node PhshMeshNodeDict(application::file::game::byml::BymlFile::Type::Dictionary);

        application::file::game::byml::BymlFile::Node PhshMeshNodeString(application::file::game::byml::BymlFile::Type::StringIndex, "PhshMeshPath");
        PhshMeshNodeString.SetValue<std::string>("Work/Phive/Shape/Dcc/" + PhshName + ".phsh");

        PhshMeshNodeDict.AddChild(PhshMeshNodeString);

        PhshMeshNode.AddChild(PhshMeshNodeDict);


        application::file::game::byml::BymlFile::Node AutoCalcNode(application::file::game::byml::BymlFile::Type::Dictionary, "AutoCalc");

        application::file::game::byml::BymlFile::Node AxisNode(application::file::game::byml::BymlFile::Type::Dictionary, "Axis");
        application::file::game::byml::BymlFile::Node AxisXNode(application::file::game::byml::BymlFile::Type::Float, "X");
        AxisXNode.SetValue<float>(0.0f);
        application::file::game::byml::BymlFile::Node AxisYNode(application::file::game::byml::BymlFile::Type::Float, "Y");
        AxisYNode.SetValue<float>(0.0f);
        application::file::game::byml::BymlFile::Node AxisZNode(application::file::game::byml::BymlFile::Type::Float, "Z");
        AxisZNode.SetValue<float>(0.0f);
        AxisNode.AddChild(AxisXNode);
        AxisNode.AddChild(AxisYNode);
        AxisNode.AddChild(AxisZNode);

        application::file::game::byml::BymlFile::Node CenterNode(application::file::game::byml::BymlFile::Type::Dictionary, "Center");
        application::file::game::byml::BymlFile::Node CenterXNode(application::file::game::byml::BymlFile::Type::Float, "X");
        CenterXNode.SetValue<float>(0.0f);
        application::file::game::byml::BymlFile::Node CenterYNode(application::file::game::byml::BymlFile::Type::Float, "Y");
        CenterYNode.SetValue<float>(0.0f);
        application::file::game::byml::BymlFile::Node CenterZNode(application::file::game::byml::BymlFile::Type::Float, "Z");
        CenterZNode.SetValue<float>(0.0f);
        CenterNode.AddChild(CenterXNode);
        CenterNode.AddChild(CenterYNode);
        CenterNode.AddChild(CenterZNode);

        application::file::game::byml::BymlFile::Node MaxNode(application::file::game::byml::BymlFile::Type::Dictionary, "Max");
        application::file::game::byml::BymlFile::Node MaxXNode(application::file::game::byml::BymlFile::Type::Float, "X");
        MaxXNode.SetValue<float>(Max.x);
        application::file::game::byml::BymlFile::Node MaxYNode(application::file::game::byml::BymlFile::Type::Float, "Y");
        MaxYNode.SetValue<float>(Max.y);
        application::file::game::byml::BymlFile::Node MaxZNode(application::file::game::byml::BymlFile::Type::Float, "Z");
        MaxZNode.SetValue<float>(Max.z);
        MaxNode.AddChild(MaxXNode);
        MaxNode.AddChild(MaxYNode);
        MaxNode.AddChild(MaxZNode);

        application::file::game::byml::BymlFile::Node MinNode(application::file::game::byml::BymlFile::Type::Dictionary, "Min");
        application::file::game::byml::BymlFile::Node MinXNode(application::file::game::byml::BymlFile::Type::Float, "X");
        MinXNode.SetValue<float>(Min.x);
        application::file::game::byml::BymlFile::Node MinYNode(application::file::game::byml::BymlFile::Type::Float, "Y");
        MinYNode.SetValue<float>(Min.y);
        application::file::game::byml::BymlFile::Node MinZNode(application::file::game::byml::BymlFile::Type::Float, "Z");
        MinZNode.SetValue<float>(Min.z);
        MinNode.AddChild(MinXNode);
        MinNode.AddChild(MinYNode);
        MinNode.AddChild(MinZNode);

        application::file::game::byml::BymlFile::Node TensorNode(application::file::game::byml::BymlFile::Type::Dictionary, "Tensor");
        application::file::game::byml::BymlFile::Node TensorXNode(application::file::game::byml::BymlFile::Type::Float, "X");
        TensorXNode.SetValue<float>(0.0f);
        application::file::game::byml::BymlFile::Node TensorYNode(application::file::game::byml::BymlFile::Type::Float, "Y");
        TensorYNode.SetValue<float>(0.0f);
        application::file::game::byml::BymlFile::Node TensorZNode(application::file::game::byml::BymlFile::Type::Float, "Z");
        TensorZNode.SetValue<float>(0.0f);
        TensorNode.AddChild(TensorXNode);
        TensorNode.AddChild(TensorYNode);
        TensorNode.AddChild(TensorZNode);

        AutoCalcNode.AddChild(AxisNode);
        AutoCalcNode.AddChild(CenterNode);
        AutoCalcNode.AddChild(MaxNode);
        AutoCalcNode.AddChild(MinNode);
        AutoCalcNode.AddChild(TensorNode);

        Byml.GetNodes().push_back(AutoCalcNode);
        Byml.GetNodes().push_back(PhshMeshNode);

        return Byml;
    }

    application::file::game::byml::BymlFile GenerateRigidBodyEntityParamByml(const std::string& ShapeName)
    {
        application::file::game::byml::BymlFile Byml;
        Byml.GetType() = application::file::game::byml::BymlFile::Type::Dictionary;

        application::file::game::byml::BymlFile::Node BuoyancyScaleNode(application::file::game::byml::BymlFile::Type::Float, "BuoyancyScale");
        BuoyancyScaleNode.SetValue<float>(1.0f);

        application::file::game::byml::BymlFile::Node MassNode(application::file::game::byml::BymlFile::Type::Float, "Mass");
        MassNode.SetValue<float>(1.0f);

        application::file::game::byml::BymlFile::Node WaterFlowScaleNode(application::file::game::byml::BymlFile::Type::Float, "WaterFlowScale");
        WaterFlowScaleNode.SetValue<float>(1.0f);

        application::file::game::byml::BymlFile::Node MotionPropertyNode(application::file::game::byml::BymlFile::Type::StringIndex, "MotionProperty");
        MotionPropertyNode.SetValue<std::string>("Default");

        application::file::game::byml::BymlFile::Node LayerEntityNode(application::file::game::byml::BymlFile::Type::StringIndex, "LayerEntity");
        LayerEntityNode.SetValue<std::string>("GroundHighRes");

        application::file::game::byml::BymlFile::Node MotionTypeNode(application::file::game::byml::BymlFile::Type::StringIndex, "MotionType");
        MotionTypeNode.SetValue<std::string>("Static");

        application::file::game::byml::BymlFile::Node SubLayerEntityNode(application::file::game::byml::BymlFile::Type::StringIndex, "SubLayerEntity");
        SubLayerEntityNode.SetValue<std::string>("Unspecified");

        application::file::game::byml::BymlFile::Node ShapeNameNode(application::file::game::byml::BymlFile::Type::StringIndex, "ShapeName");
        ShapeNameNode.SetValue<std::string>(ShapeName);

        Byml.GetNodes().push_back(BuoyancyScaleNode);
        Byml.GetNodes().push_back(LayerEntityNode);
        Byml.GetNodes().push_back(MassNode);
        Byml.GetNodes().push_back(MotionPropertyNode);
        Byml.GetNodes().push_back(MotionTypeNode);
        Byml.GetNodes().push_back(ShapeNameNode);
        Byml.GetNodes().push_back(SubLayerEntityNode);
        Byml.GetNodes().push_back(WaterFlowScaleNode);

        return Byml;
    }

    application::file::game::byml::BymlFile GenerateRigidBodyControllerEntityParamByml()
    {
        application::file::game::byml::BymlFile Byml;
        Byml.GetType() = application::file::game::byml::BymlFile::Type::Dictionary;

        application::file::game::byml::BymlFile::Node RigidBodyControllerUnitAryNode(application::file::game::byml::BymlFile::Type::Array, "RigidBodyControllerUnitAry");

        auto CreateBoolNode = [](const std::string& Key, bool Value)
            {
                application::file::game::byml::BymlFile::Node Node(application::file::game::byml::BymlFile::Type::Bool, Key);
                Node.SetValue<bool>(Value);
                return Node;
            };

        auto CreateStringNode = [](const std::string& Key, const std::string &Value)
            {
                application::file::game::byml::BymlFile::Node Node(application::file::game::byml::BymlFile::Type::StringIndex, Key);
                Node.SetValue<std::string>(Value);
                return Node;
            };

        application::file::game::byml::BymlFile::Node ChildDict(application::file::game::byml::BymlFile::Type::Dictionary);

        ChildDict.AddChild(CreateStringNode("BoneBindModePosition", "All"));
        ChildDict.AddChild(CreateStringNode("BoneBindModeRotation", "All"));
        ChildDict.AddChild(CreateStringNode("ContactCollectionName", ""));
        ChildDict.AddChild(CreateBoolNode("IgnoreChangeMassOnScaling", false));
        ChildDict.AddChild(CreateBoolNode("IsAddToWorldOnReset", true));
        ChildDict.AddChild(CreateBoolNode("IsSkipTrackingWhenNoHit", true));
        ChildDict.AddChild(CreateBoolNode("IsTrackingActor", false));
        ChildDict.AddChild(CreateBoolNode("IsTrackingEntity", false));
        ChildDict.AddChild(CreateStringNode("Name", "Physical"));
        ChildDict.AddChild(CreateStringNode("TrackingBoneName", ""));
        ChildDict.AddChild(CreateStringNode("TrackingEntityAlias", ""));
        ChildDict.AddChild(CreateBoolNode("UseNextMainRigidBodyMatrix", false));
        ChildDict.AddChild(CreateStringNode("WarpMode", "AfterUpdateWorldMtx"));

        RigidBodyControllerUnitAryNode.AddChild(ChildDict);

        Byml.GetNodes().push_back(RigidBodyControllerUnitAryNode);

        return Byml;
    }

    application::file::game::byml::BymlFile GenerateControllerSetParamByml(const std::string& ActorName)
    {
        application::file::game::byml::BymlFile Byml;
        Byml.GetType() = application::file::game::byml::BymlFile::Type::Dictionary;

        auto CreateStringNode = [](const std::string& Key, const std::string &Value)
            {
                application::file::game::byml::BymlFile::Node Node(application::file::game::byml::BymlFile::Type::StringIndex, Key);
                Node.SetValue<std::string>(Value);
                return Node;
            };

        Byml.GetNodes().push_back(CreateStringNode("ConstraintControllerPath", ""));

        application::file::game::byml::BymlFile::Node ControllerEntityNamePathAryNode(application::file::game::byml::BymlFile::Type::Array, "ControllerEntityNamePathAry");

        application::file::game::byml::BymlFile::Node ControllerEntityNamePathAryDictNode(application::file::game::byml::BymlFile::Type::Dictionary);
        ControllerEntityNamePathAryDictNode.AddChild(CreateStringNode("FilePath", "Work/Phive/RigidBodyControllerEntityParam/" + ActorName + "_Body.phive__RigidBodyControllerEntityParam.gyml"));
        ControllerEntityNamePathAryDictNode.AddChild(CreateStringNode("Name", "Body"));
        ControllerEntityNamePathAryNode.AddChild(ControllerEntityNamePathAryDictNode);

        application::file::game::byml::BymlFile::Node RigidBodyEntityNamePathAryNode(application::file::game::byml::BymlFile::Type::Array, "RigidBodyEntityNamePathAry");

        application::file::game::byml::BymlFile::Node RigidBodyEntityNamePathAryDictNode(application::file::game::byml::BymlFile::Type::Dictionary);
        RigidBodyEntityNamePathAryDictNode.AddChild(CreateStringNode("FilePath", "Work/Phive/RigidBodyEntityParam/" + ActorName + "_Body.phive__RigidBodyEntityParam.gyml"));
        RigidBodyEntityNamePathAryDictNode.AddChild(CreateStringNode("Name", "Physical"));
        RigidBodyEntityNamePathAryNode.AddChild(RigidBodyEntityNamePathAryDictNode);

        application::file::game::byml::BymlFile::Node ShapeNamePathAryNode(application::file::game::byml::BymlFile::Type::Array, "ShapeNamePathAry");
        application::file::game::byml::BymlFile::Node ShapeNamePathAryDictNode(application::file::game::byml::BymlFile::Type::Dictionary);
        ShapeNamePathAryDictNode.AddChild(CreateStringNode("FilePath", "Work/Phive/ShapeParam/" + ActorName + "__Physical.phive__ShapeParam.gyml"));
        ShapeNamePathAryDictNode.AddChild(CreateStringNode("Name", "Physical"));
        ShapeNamePathAryNode.AddChild(ShapeNamePathAryDictNode);

        Byml.GetNodes().push_back(ControllerEntityNamePathAryNode);
        Byml.GetNodes().push_back(RigidBodyEntityNamePathAryNode);
        Byml.GetNodes().push_back(ShapeNamePathAryNode);

        return Byml;
    }

    void UICollisionGenerator::GenerateActorPack()
    {
        //Clearing actor pack
        for (auto Iter = mActorPack->mPack.GetEntries().begin(); Iter != mActorPack->mPack.GetEntries().end(); ) {
            if (Iter->mName.starts_with("Phive/"))
                Iter = mActorPack->mPack.GetEntries().erase(Iter);
            else
                Iter++;
        }

        {
            application::file::game::SarcFile::Entry Entry;
            Entry.mBytes = GenerateShapeParamFile(mPhiveShapeName, glm::vec3(-10, -10, -10), glm::vec3(10, 10, 10)).ToBinary(application::file::game::byml::BymlFile::TableGeneration::AUTO);
            Entry.mName = "Phive/ShapeParam/" + mPhiveShapeName + ".phive__ShapeParam.bgyml";
            mActorPack->mPack.GetEntries().push_back(Entry);
        }

        {
            application::file::game::SarcFile::Entry Entry;
            Entry.mBytes = GenerateRigidBodyEntityParamByml("Physical").ToBinary(application::file::game::byml::BymlFile::TableGeneration::AUTO);
            Entry.mName = "Phive/RigidBodyEntityParam/" + mActorPack->mName + "_Body.phive__RigidBodyEntityParam.bgyml";
            mActorPack->mPack.GetEntries().push_back(Entry);
        }

        //RigidBodyControllerEntityParam
        application::file::game::SarcFile::Entry RigidBodyControllerEntityParamEntry;
        RigidBodyControllerEntityParamEntry.mBytes = GenerateRigidBodyControllerEntityParamByml().ToBinary(application::file::game::byml::BymlFile::TableGeneration::AUTO);
        RigidBodyControllerEntityParamEntry.mName = "Phive/RigidBodyControllerEntityParam/" + mActorPack->mName + "_Body.phive__RigidBodyControllerEntityParam.bgyml";
        mActorPack->mPack.GetEntries().push_back(RigidBodyControllerEntityParamEntry);

        //ControllerSetParam
        application::file::game::SarcFile::Entry ControllerSetParamEntry;
        ControllerSetParamEntry.mBytes = GenerateControllerSetParamByml(mActorPack->mName).ToBinary(application::file::game::byml::BymlFile::TableGeneration::AUTO);
        ControllerSetParamEntry.mName = "Phive/ControllerSetParam/" + mActorPack->mName + ".phive__ControllerSetParam.bgyml";
        mActorPack->mPack.GetEntries().push_back(ControllerSetParamEntry);

        //PhysicsRef
        application::file::game::SarcFile::Entry* PhysicsEntry = nullptr;
        for (application::file::game::SarcFile::Entry& Entry : mActorPack->mPack.GetEntries())
        {
            if (Entry.mName.starts_with("Component/Physics/"))
            {
                PhysicsEntry = &Entry;
                break;
            }
        }
        if (PhysicsEntry != nullptr)
        {
            application::file::game::byml::BymlFile PhysicsRefByml(PhysicsEntry->mBytes);
            if (PhysicsRefByml.HasChild("ControllerSetPath"))
            {
                application::file::game::byml::BymlFile::Node* ControllerRefNode = PhysicsRefByml.GetNode("ControllerSetPath");
                ControllerRefNode->SetValue<std::string>("Work/Phive/ControllerSetParam/" + mActorPack->mName + ".phive__ControllerSetParam.gyml");
                PhysicsEntry->mBytes = PhysicsRefByml.ToBinary(application::file::game::byml::BymlFile::TableGeneration::AUTO);
            }
        }

        std::filesystem::create_directories(application::util::FileUtil::GetSaveFilePath("Pack/Actor"));

        application::util::FileUtil::WriteFile(application::util::FileUtil::GetSaveFilePath("Pack/Actor/" + mActorPack->mName + ".pack.zs"), application::file::game::ZStdBackend::Compress(mActorPack->mPack.ToBinary(), application::file::game::ZStdBackend::Dictionary::Pack));
        
    }

    void UICollisionGenerator::GenerateBphsh()
    {
        mGenerator.mMaterials.clear();
        for (MaterialEntry& Entry : mMaterials)
        {
            mGenerator.mMaterials.push_back(Entry.mPhiveMaterial);
        }

        uint32_t MaterialOffset = 0;
        uint32_t IndexOffset = 0;
        bool HasGeometry = false;

        mGenerator.mVertices.clear();
        mGenerator.mIndices.clear();

        for (ShapeEntry& Entry : mShapes)
        {
            if (!Entry.mEnabled)
            {
                MaterialOffset++;
                continue;
            }

            std::vector<glm::vec3> Positions;
            for (const glm::vec4& Vec : Entry.mShape->Vertices)
            {
                Positions.emplace_back(Vec.x, Vec.y, Vec.z);
            }

            std::vector<uint32_t> Indices = Entry.mShape->Meshes[0].GetIndices();

            uint32_t Offset = mGenerator.mVertices.size();
            mGenerator.mVertices.resize(mGenerator.mVertices.size() + Positions.size());

            for (size_t i = 0; i < Positions.size(); i++)
            {
                mGenerator.mVertices[Offset + i] = Positions[i];
            }

            Offset = mGenerator.mIndices.size();
            mGenerator.mIndices.resize(mGenerator.mIndices.size() + (Indices.size() / 3));
            for (size_t i = 0; i < Indices.size() / 3; i++)
            {
                mGenerator.mIndices[Offset + i] = std::make_pair(std::make_tuple(Indices[i * 3] + IndexOffset, Indices[i * 3 + 1] + IndexOffset, Indices[i * 3 + 2] + IndexOffset), Entry.mMaterialIndex);
            }

            IndexOffset += Positions.size();
            HasGeometry = true;
        }

        if (!HasGeometry)
        {
            ImGui::InsertNotification({ ImGuiToastType::Error, 3000, "Collision generation failed due to empty geometry." });
            return;
        }

        mPhiveShape.InjectModel(mGenerator);
        std::vector<unsigned char> Bytes = mPhiveShape.ToBinary();
        application::util::FileUtil::WriteFile(application::util::FileUtil::GetWorkingDirFilePath("LocalGenerator.bphsh"), Bytes);
        std::filesystem::create_directories(application::util::FileUtil::GetSaveFilePath("Phive/Shape/Dcc"));
        application::util::FileUtil::WriteFile(application::util::FileUtil::GetSaveFilePath("Phive/Shape/Dcc/" + mPhiveShapeName + ".Nin_NX_NVN.bphsh.zs"), application::file::game::ZStdBackend::Compress(Bytes, application::file::game::ZStdBackend::Dictionary::Base));

        std::vector<float> VerticesRaw = mPhiveShape.ToVertices();
        std::vector<unsigned int> IndicesRaw = mPhiveShape.ToIndices();
        mCollisionMesh = application::gl::SimpleMesh(VerticesRaw, IndicesRaw);

        if (mActorPack != nullptr)
            GenerateActorPack();

        application::util::Logger::Info("UICollisionGenerator", "Success");

        ImGui::InsertNotification({ ImGuiToastType::Success, 3000, "Collision generation successful." });
    }

    void UICollisionGenerator::DrawModelViewWindow()
    {
        ImGui::Begin(CreateID("Model View").c_str());

        ImVec2 Pos = ImGui::GetCursorScreenPos();

        const ImVec2 WindowSize = ImGui::GetContentRegionAvail();
        if (mSceneWindowSize.x != WindowSize.x || mSceneWindowSize.y != WindowSize.y)
        {
            //Rescaling framebuffer
            mSceneWindowSize = WindowSize;
            mFramebuffer->RescaleFramebuffer(WindowSize.x, WindowSize.y);
            mCamera.mWidth = WindowSize.x;
            mCamera.mHeight = WindowSize.y;
        }
        else
        {
            //Bind
            mFramebuffer->Bind();
        }

        mCamera.mWindowHovered = (ImGui::IsWindowHovered() && !ImGui::IsItemHovered());

        glViewport(0, 0, WindowSize.x, WindowSize.y);
        glClearColor(mClearColor.x * mClearColor.w, mClearColor.y * mClearColor.w, mClearColor.z * mClearColor.w, mClearColor.w);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        ImVec2 WindowPos = ImGui::GetWindowPos();
        mCamera.Inputs(ImGui::GetIO().Framerate, glm::vec2(WindowPos.x, WindowPos.y));
        mCamera.UpdateMatrix(45.0f, 0.1f, 30000.0f);

        gInstancedShader->Bind();
        mCamera.Matrix(gInstancedShader, "camMatrix");
        glUniform3fv(glGetUniformLocation(gInstancedShader->mID, "lightColor"), 1, &application::rendering::map_editor::UIMapEditor::gLightColor[0]);
        glUniform3fv(glGetUniformLocation(gInstancedShader->mID, "lightPos"), 1, &mCamera.mPosition[0]);

        gSelectedShader->Bind();
        mCamera.Matrix(gSelectedShader, "camMatrix");
        glUniform3fv(glGetUniformLocation(gSelectedShader->mID, "lightColor"), 1, &application::rendering::map_editor::UIMapEditor::gLightColor[0]);
        glUniform3fv(glGetUniformLocation(gSelectedShader->mID, "lightPos"), 1, &mCamera.mPosition[0]);

        if (mRenderModel && mBfresFile != nullptr && mBfresRenderer != nullptr)
        {
            std::vector<glm::mat4> InstanceMatrix(1);

            glm::mat4& GLMModel = InstanceMatrix[0];
            GLMModel = glm::mat4(1.0f); // Identity matrix

            if (mRenderModelWireframe)
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            

            mBfresRenderer->mInstanceMatrix.SetData<glm::mat4>(InstanceMatrix);

            glDisable(GL_CULL_FACE);


            for (uint16_t i : mBfresRenderer->mOpaqueObjects)
            {
                if (mDetailsEditorContent.mType == DetailsEditorContentType::SHAPE && std::get<DetailsEditorContentShape>(mDetailsEditorContent.mContent).mShape == &mShapes[i])
                    gSelectedShader->Bind();
                else
                    gInstancedShader->Bind();

                mBfresRenderer->mShapeVAOs[i].Enable();
                mBfresRenderer->mShapeVAOs[i].Use();

                mBfresRenderer->mMaterials[i].mAlbedoTexture->Bind();

                glDrawElementsInstanced(GL_TRIANGLES, mBfresRenderer->mIndexBuffers[i].second, mBfresRenderer->mMaterials[i].mIndexFormat, 0, InstanceMatrix.size());
            }

            for (uint16_t i : mBfresRenderer->mTransparentObjects)
            {
                if (mDetailsEditorContent.mType == DetailsEditorContentType::SHAPE && std::get<DetailsEditorContentShape>(mDetailsEditorContent.mContent).mShape == &mShapes[i])
                    gSelectedShader->Bind();
                else
                    gInstancedShader->Bind();

                mBfresRenderer->mShapeVAOs[i].Enable();
                mBfresRenderer->mShapeVAOs[i].Use();

                mBfresRenderer->mMaterials[i].mAlbedoTexture->Bind();

                glDrawElementsInstanced(GL_TRIANGLES, mBfresRenderer->mIndexBuffers[i].second, mBfresRenderer->mMaterials[i].mIndexFormat, 0, InstanceMatrix.size());
            }


            if (mRenderModelWireframe)
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        gNavMeshShader->Bind();
        mCamera.Matrix(gNavMeshShader, "camMatrix");
        glm::mat4 GLMModel = glm::mat4(1.0f);
        glUniformMatrix4fv(glGetUniformLocation(gNavMeshShader->mID, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(GLMModel));

        if (mRenderCollisionPreview && mCollisionMesh.has_value())
        {
            if(mRenderCollisionPreviewWireframe) 
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

            mCollisionMesh.value().Draw();

            if (mRenderCollisionPreviewWireframe) 
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        for (application::gl::SimpleMesh& Mesh : mDebugShapes)
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            Mesh.Draw();
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        mFramebuffer->Unbind();

        ImGui::GetWindowDrawList()->AddImage(
            (ImTextureID)mFramebuffer->GetFramebufferTexture(),
            ImVec2(Pos.x, Pos.y),
            ImVec2(Pos.x + WindowSize.x, Pos.y + WindowSize.y),
            ImVec2(0, 1),
            ImVec2(1, 0)
        );

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.5f));
        float w = ImGui::GetCursorPosX();
        if (ImGui::Button("Model/Collision Stats"))
        {
            GenerateCollisionStats();
            ImGui::OpenPopup("CollisionStats");
        }
        Pos = ImGui::GetCursorScreenPos();
        ImGui::PopStyleColor();

        ImGui::SetNextWindowPos(ImVec2(Pos.x + w, Pos.y));
        if (ImGui::BeginPopup("CollisionStats"))
        {
            ImGui::Text("Model vertex count: %i", mCollisionStats.mModelVertexCount);
            ImGui::Text("Model face count: %i", mCollisionStats.mModelFaceCount);
            ImGui::NewLine();
            ImGui::Text("Collision geometry section count: %i", mCollisionStats.mCollisionGeometrySectionCount);
            ImGui::Text("Collision vertex count: %i", mCollisionStats.mCollisionVertexCount);
            ImGui::Text("Collision primitive count: %i", mCollisionStats.mCollisionPrimitiveCount);
            ImGui::Text("Collision BVH count: %i", mCollisionStats.mCollisionBVHCount);
            ImGui::EndPopup();
        }

        ImGui::End();
    }

    void UICollisionGenerator::DrawDetailsWindow()
    {
        ImGui::Begin(CreateID("Details").c_str());

        ImGui::SeparatorText("Details");
        ImGui::Indent();
        ImGui::Text("Actor Name: %s", mActorPack == nullptr ? "None" : mActorPack->mName.c_str());
        ImGui::Unindent();

        ImGui::NewLine();

        if (mDetailsEditorContent.mType == DetailsEditorContentType::SHAPE)
        {
            DetailsEditorContentShape& ShapeContent = std::get<DetailsEditorContentShape>(mDetailsEditorContent.mContent);

            ImGui::SeparatorText(ShapeContent.mShape->mShape->Name.c_str());

            ImGui::Indent();

            if (ImGui::BeginTable("ShapeContentTable", 2, ImGuiTableFlags_BordersInnerV))
            {
                ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("Material").x);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Enabled");
                ImGui::TableNextColumn();
                ImGui::Checkbox("##Enabled", &ShapeContent.mShape->mEnabled);
                ImGui::TableNextRow();
                
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Material");
                ImGui::TableNextColumn();
                if (ImGui::BeginCombo("##Material", ShapeContent.mShape->mMaterialIndex == -1 || ShapeContent.mShape->mMaterialIndex >= mMaterials.size() ? "None" : mMaterials[ShapeContent.mShape->mMaterialIndex].mName.c_str()))
                {
                    for (size_t i = 0; i < mMaterials.size(); i++)
                    {
                        bool Selected = ShapeContent.mShape->mMaterialIndex == i;
                        if (ImGui::Selectable(mMaterials[i].mName.c_str(), Selected))
                        {
                            ShapeContent.mShape->mMaterialIndex = i;
                        }
                        if (Selected)
                        {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }

                ImGui::EndTable();
            }

            ImGui::Unindent();
        }
        else if (mDetailsEditorContent.mType == DetailsEditorContentType::MATERIAL)
        {
            bool OpenPopUp = false;
            ImVec2 PopUpPos;
            DetailsEditorContentMaterial& MaterialContent = std::get<DetailsEditorContentMaterial>(mDetailsEditorContent.mContent);

            ImGui::SeparatorText(MaterialContent.mMaterial->mName.c_str());

            ImGui::Indent();

            if (ImGui::BeginTable("MaterialContentTable", 2, ImGuiTableFlags_BordersInnerV))
            {
                ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("Material Type").x);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Name");
                ImGui::TableNextColumn();
                ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
                ImGui::InputText("##Name", &MaterialContent.mMaterial->mName);
                ImGui::PopItemWidth();
                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Material Type");
                ImGui::TableNextColumn();
                ImGui::PushItemWidth(ImGui::GetCurrentTable()->Columns[1].WidthMax);
                ImGui::Combo("##MaterialType", reinterpret_cast<int*>(&MaterialContent.mMaterial->mPhiveMaterial.mMaterialId), application::file::game::phive::util::PhiveMaterialData::gMaterialNames.data(), application::file::game::phive::util::PhiveMaterialData::gMaterialNames.size());
                ImGui::PopItemWidth();
                ImGui::TableNextRow();

                ImGui::EndTable();

                ImGui::Columns(2);
                ImGui::AlignTextToFramePadding();

                if (ImGui::Button("Flags", ImVec2(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize, 0)))
                {
                    ImGui::OpenPopup("FlagsPopUp");
                    OpenPopUp = true;
                    PopUpPos = ImGui::GetCursorScreenPos();
                }
                ImGui::NextColumn();
                if (ImGui::Button("Filters", ImVec2(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize, 0)))
                {
                    ImGui::OpenPopup("FiltersPopUp");
                    OpenPopUp = true;
                    PopUpPos = ImGui::GetCursorScreenPos();
                }

                ImGui::Columns();
            }

            if (ImGui::Button("Apply to all shapes", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
            {
                uint32_t MaterialIndex = std::distance(mMaterials.begin(), std::find_if(mMaterials.begin(), mMaterials.end(),
                    [&MaterialContent](const MaterialEntry& Entry) 
                    {
                        return &Entry == MaterialContent.mMaterial;
                    }
                ));
                for (ShapeEntry& Entry : mShapes)
                {
                    Entry.mMaterialIndex = MaterialIndex;
                }
            }

            if (OpenPopUp)
            {
                ImGui::SetNextWindowPos(PopUpPos);
            }
            if (ImGui::BeginPopup("FlagsPopUp"))
            {
                for (size_t i = 0; i < application::file::game::phive::util::PhiveMaterialData::gMaterialFlagNames.size(); i++)
                {
                    ImGui::Checkbox(application::file::game::phive::util::PhiveMaterialData::gMaterialFlagNames[i], &MaterialContent.mMaterial->mPhiveMaterial.mFlags[i]);
                }
                ImGui::EndPopup();
            }
            if (ImGui::BeginPopup("FiltersPopUp"))
            {
                for (size_t i = 0; i < application::file::game::phive::util::PhiveMaterialData::gMaterialFilterNames.size(); i++)
                {
                    ImGui::Checkbox(application::file::game::phive::util::PhiveMaterialData::gMaterialFilterNames[i], &MaterialContent.mMaterial->mPhiveMaterial.mCollisionFlags[i]);
                }
                ImGui::EndPopup();
            }

            ImGui::Unindent();
        }

        ImGui::End();
    }

    void UICollisionGenerator::GenerateCollisionStats()
    {
        if (mBfresFile != nullptr)
        {
            mCollisionStats.mModelVertexCount = 0;
            mCollisionStats.mModelFaceCount = 0;
            for (auto& [Key, Val] : mBfresFile->Models.GetByIndex(0).mValue.Shapes.mNodes)
            {
                mCollisionStats.mModelVertexCount += Val.mValue.Vertices.size();
                mCollisionStats.mModelFaceCount += Val.mValue.Meshes[0].GetIndices().size();
            }
        }

        if (!mPhiveShape.mTagFile.mRootClass.mGeometrySections.mElements.empty())
        {
            mCollisionStats.mCollisionVertexCount = 0;
            mCollisionStats.mCollisionPrimitiveCount = 0;
            mCollisionStats.mCollisionBVHCount = 0;
            mCollisionStats.mCollisionGeometrySectionCount = mPhiveShape.mTagFile.mRootClass.mGeometrySections.mElements.size();
            for (const application::file::game::phive::classes::HavokClasses::hknpMeshShape__GeometrySection& GeometrySection : mPhiveShape.mTagFile.mRootClass.mGeometrySections.mElements)
            {
                mCollisionStats.mCollisionVertexCount += (GeometrySection.mVertexBuffer.mElements.size() - 2) / 6;
                mCollisionStats.mCollisionPrimitiveCount += GeometrySection.mPrimitives.mElements.size();
                mCollisionStats.mCollisionBVHCount += GeometrySection.mSectionBvh.mElements.size();
            }
        }
    }

    void UICollisionGenerator::LoadBfres(const std::string& Path)
    {
        mBfresFile = application::manager::BfresFileMgr::GetBfresFile(Path);

        if (mBfresFile != nullptr) //Should never be the case
        {
            mGenerationTarget = GenerationTarget::BFRES_FILE;
            mBfresRenderer = application::manager::BfresRendererMgr::GetRenderer(mBfresFile);

            mShapes.clear();
            for (auto& [Key, Val] : mBfresFile->Models.GetByIndex(0).mValue.Shapes.mNodes)
            {
                ShapeEntry Entry;
                Entry.mShape = &Val.mValue;
                mShapes.push_back(Entry);
            }

            mPhiveShapeName = mBfresFile->Models.GetByIndex(0).mValue.Name + "__Physical";
            mCollisionMesh = std::nullopt;
        }
    }

    void UICollisionGenerator::LoadActorPack(const std::string& Gyml)
    {
        mActorPack = application::manager::ActorPackMgr::GetActorPack(Gyml);

        if (mActorPack == nullptr)
            return;

        if (application::game::actor_component::ActorComponentBase* BaseComponent = mActorPack->GetComponent(application::game::actor_component::ActorComponentBase::ComponentType::MODEL_INFO); BaseComponent != nullptr)
        {
            application::game::actor_component::ActorComponentModelInfo* ModelInfoComponent = static_cast<application::game::actor_component::ActorComponentModelInfo*>(BaseComponent);
            if (ModelInfoComponent)
            {
                if (ModelInfoComponent->mModelProjectName.has_value() && ModelInfoComponent->mFmdbName.has_value() && !ModelInfoComponent->mModelProjectName.value().empty() && !ModelInfoComponent->mFmdbName.value().empty())
                {
                    LoadBfres(application::util::FileUtil::GetBfresFilePath(ModelInfoComponent->mModelProjectName.value() + "." + ModelInfoComponent->mFmdbName.value() + ".bfres"));
                }
            }
        }

        mPhiveShapeName = mActorPack->mName + "__Physical";
        mGenerationTarget = GenerationTarget::ACTOR_PACK;
    }

    void UICollisionGenerator::DrawOperationsWindow()
    {
        ImGui::Begin(CreateID("Operations").c_str());

        ImGui::SeparatorText("Render Settings");
        ImGui::Indent();

        ImGui::Checkbox("Render model", &mRenderModel);
        ImGui::Indent();
        if (!mRenderModel)
            ImGui::BeginDisabled();
        ImGui::Checkbox("Wireframe mode##RenderModel", &mRenderModelWireframe);
        if (!mRenderModel)
            ImGui::EndDisabled();
        ImGui::Unindent();

        ImGui::Checkbox("Render collision preview", &mRenderCollisionPreview);
        ImGui::Indent();
        if (!mRenderCollisionPreview)
            ImGui::BeginDisabled();
        ImGui::Checkbox("Wireframe mode##RenderCollisionPreview", &mRenderCollisionPreviewWireframe);
        if (!mRenderCollisionPreview)
            ImGui::EndDisabled();
        ImGui::Unindent();

        ImGui::Unindent();

        ImGui::NewLine();
        ImGui::SeparatorText("Generator settings");
        ImGui::Indent();

        ImGui::Checkbox("Run mesh optimizer", &mGenerator.mRunMeshOptimizer);

        ImGui::NewLine();
        /*
        if (!mGenerator.mConvertToQuads)
            ImGui::BeginDisabled();
        ImGui::InputScalar("Quad Normal Vec Angle", ImGuiDataType_Float, &mGenerator.mQuadNormalVecEpsilon);
        if (!mGenerator.mConvertToQuads)
            ImGui::EndDisabled();
            */

        if (!mGenerator.mRunMeshOptimizer)
            ImGui::BeginDisabled();
        ImGui::InputFloat("Optimizer max error (0-1)", &mGenerator.mMeshOptimizerTriangleReductionMaxError, 0.0f, 0.0f, "%.6f");
        if (!mGenerator.mRunMeshOptimizer)
            ImGui::EndDisabled();

        ImGui::Unindent();

        ImGui::InputText("Phive Shape Name", &mPhiveShapeName);

        if (mPhiveShapeName.empty())
            ImGui::BeginDisabled();

        if (ImGui::Button("Generate collision", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
        {
            GenerateBphsh();
        }

        if (mPhiveShapeName.empty())
            ImGui::EndDisabled();

        ImGui::End();
    }

    void UICollisionGenerator::DrawImpl()
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
            if (ImGui::BeginMenu(CreateID("File").c_str()))
            {
                if (ImGui::MenuItem("Load Actor"))
                {
                    application::rendering::popup::PopUpCommon::gLoadActorPopUp.Open([this](application::rendering::popup::PopUpBuilder& Builder)
                        {
                            const std::string& Gyml = *static_cast<std::string*>(Builder.GetDataStorage(0).mPtr);
                            LoadActorPack(Gyml);
                        });
                   //LoadActorPack("SwordTrial_WarpPointDeco_A_01");
                }
                if (ImGui::MenuItem(CreateID("Open BFRES").c_str()))
                {
                    auto Dialog = pfd::open_file("Choose BFRES file to open", application::util::FileUtil::gBfresPath,
                        { "Binary Cafe Resource (.bfres, .bfres.mc)", "*.bfres *.bfres.mc" },
                        pfd::opt::none);

                    if (!Dialog.result().empty())
                    {
                        LoadBfres(Dialog.result()[0]);
                    }

                    //LoadBfres("C:/Users/X/AppData/Roaming/Ryujinx/sdcard/mc_output0/DgnObj_Small_BoxParts_A.DgnObj_Small_BoxFloor_A_07.bfres");
                    //LoadBfres("C:/Users/X/AppData/Roaming/Ryujinx/sdcard/mc_output0/DgnObj_Small_BoxParts_A.DgnObj_Small_BoxFloor_A_07.bfres");
                    //LoadBfres("C:/Users/X/Desktop/TotKBfresTool/BfresTool/rootDir/Assets/Models/TexturedCube.bfres");
                    //LoadBfres("H:/X/switchemulator/Zelda TotK/TrialsOfTheChosenHero/Rooms/Beginning/01/SwordTrial_Terrain.SwordTrial_TerrainCollision_B_01.bfres.mc");
                    //LoadBfres("C:/Users/X/Desktop/TotKBfresTool/BfresTool/rootDir/WorkingDir/Projects/TrialOfTheChosenHero/Model/SwordTrial_Terrain.SwordTrial_Terrain_B_01.bfres.mc");
                    //LoadBfres("C:/Users/X/Desktop/TotKBfresTool/BfresTool/rootDir/WorkingDir/Projects/TrialOfTheChosenHero/Model/SwordTrial_Common.SwordTrial_WarpPointDeco_A_01.bfres.mc");
                }
                ImGui::EndMenu();
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

            ImGuiID DockRightTop, DockRightBottom;
            ImGui::DockBuilderSplitNode(DockRight, ImGuiDir_Up, 0.5f, &DockRightTop, &DockRightBottom);

            ImGui::DockBuilderDockWindow(CreateID("Materials").c_str(), DockLeftBottom);
            ImGui::DockBuilderDockWindow(CreateID("Shapes").c_str(), DockLeftTop);

            ImGui::DockBuilderDockWindow(CreateID("Model View").c_str(), DockMiddle);

            ImGui::DockBuilderDockWindow(CreateID("Details").c_str(), DockRightTop);
            ImGui::DockBuilderDockWindow(CreateID("Operations").c_str(), DockRightBottom);

            ImGui::DockBuilderFinish(DockspaceId);
        }

        ImGui::End();

        DrawShapesWindow();
        DrawMaterialsWindow();
        DrawDetailsWindow();
        DrawOperationsWindow();
        DrawModelViewWindow();
    }

    void UICollisionGenerator::DeleteImpl()
    {

    }

    UIWindowBase::WindowType UICollisionGenerator::GetWindowType()
    {
        return UIWindowBase::WindowType::EDITOR_COLLISION;
    }

    std::string UICollisionGenerator::GetWindowTitle()
    {
        if (mSameWindowTypeCount > 0)
        {
            return "Collision Generator (" + std::to_string(mSameWindowTypeCount) + ")###" + std::to_string(mWindowId);
        }

        return "Collision Generator###" + std::to_string(mWindowId);
    }
}