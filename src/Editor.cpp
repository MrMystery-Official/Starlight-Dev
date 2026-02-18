#include "Editor.h"

#include <manager/UIMgr.h>
#include <manager/BfresFileMgr.h>
#include <manager/BfresRendererMgr.h>
#include <manager/AINBNodeMgr.h>
#include <manager/ProjectMgr.h>
#include <manager/ActorInfoMgr.h>
#include <manager/ActorPackMgr.h>
#include <manager/GameDataListMgr.h>
#include <manager/PluginMgr.h>
#include <util/Logger.h>
#include <rendering/ainb/UIAINBEditor.h>
#include <rendering/ImGuiExt.h>
#include <rendering/popup/PopUpCommon.h>
#include <rendering/mapeditor/UIMapEditor.h>
#include <rendering/collision/UICollisionGenerator.h>
#include <filesystem>
#include <util/GitManager.h>
#include <util/FileUtil.h>
#include <file/game/zstd/ZStdBackend.h>
#include <file/tool/PathConfigFile.h>
#include <file/tool/ProjectConfigFile.h>
#include <file/tool/LicenseFile.h>
#include <file/tool/GitIdentityFile.h>
#include <file/tool/AdditionalAINBNodes.h>
#include <file/game/bfres/BfresFile.h>
#include <game/actor_component/ActorComponentFactory.h>

#include <forever_confidential/PhiveClassGenerator.h>
#include <file/game/phive/shape/PhiveShape.h>
#include <file/game/phive/navmesh/PhiveNavMesh.h>

#include <tool/AINBFileSearcher.h>

namespace application
{
    bool Editor::gInitializedStage2 = false;
    std::string Editor::gInternalGameVersion = "100";
    std::string Editor::gInternalGameDataListVersion = "100";

    //Stage 1 - no RomFS files needed
	bool Editor::Initialize()
	{
        application::util::Logger::Info("Editor", "Starting stage 1 initialization");

        /*
        char ExePath[MAX_PATH];
        GetModuleFileNameA(NULL, ExePath, MAX_PATH);
        std::string ExeDir(ExePath);
        ExeDir = ExeDir.substr(0, ExeDir.find_last_of("\\/"));

        std::string DllPath = ExeDir + "\\Assets\\Libs";
        BOOL DllDirResult = SetDllDirectoryA(DllPath.c_str());
        if (DllDirResult == 0)
        {
            application::util::Logger::Error("Editor", "Unable to initialize DLL runtime loader");
            return false;
        }
        SetEnvironmentVariableA("GVPLUGIN_PATH", DllPath.c_str());
        SetEnvironmentVariableA("GVBINDIR", DllPath.c_str());
        */

        std::filesystem::create_directories(application::util::FileUtil::GetWorkingDirFilePath("")); //Creating WorkingDir
        std::filesystem::create_directories(application::util::FileUtil::GetWorkingDirFilePath("Cache"));
        std::filesystem::create_directories(application::util::FileUtil::GetWorkingDirFilePath("Projects"));

        if (!application::manager::AINBNodeMgr::Initialize())
        {
            application::util::Logger::Error("Editor", "Aborting stage 1 initialization due to failed AINBNodeMgr initialization");
            return false;
        }

        if (!application::manager::UIMgr::Initialize())
        {
            application::util::Logger::Error("Editor", "Aborting stage 1 initialization due to failed UI initialization");
            return false;
        }

        if (!ImGuiExt::Initialize())
        {
            application::util::Logger::Error("Editor", "Aborting stage 1 initialization due to failed ImGuiExt initialization");
            return false;
        }

        application::rendering::ainb::UIAINBEditor::Initialize();
        application::rendering::map_editor::UIMapEditor::InitializeGlobal();

        //application::manager::UIMgr::OpenWindow(std::make_unique<application::rendering::ainb::UIAINBEditor>("H:/XX/switchemulator/Zelda TotK/Dumps/v1.2.1/romfs/Logic/Dungeon001_1800.logic.root.ainb"));
        //application::manager::UIMgr::OpenWindow(std::make_unique<application::rendering::ainb::UIAINBEditor>("/Users/XX/Downloads/romfs/AI/Assassin_Senior.action.interuptlargedamage.module.ainb"));
        //application::manager::UIMgr::OpenWindow(std::make_unique<application::rendering::ainb::UIAINBEditor>("/Users/XX/Downloads/romfs/AI/_LocalModule_ad9fcddc440a.module.ainb"));
        //application::manager::UIMgr::OpenWindow(std::make_unique<application::rendering::ainb::UIAINBEditor>("H:/XX/switchemulator/Zelda TotK/MapEditorV4/ainb-main-v2/ainb/_LocalModule_ad9fcddc440a.module.ainb"));
        //application::manager::UIMgr::OpenWindow(std::make_unique<application::rendering::ainb::UIAINBEditor>("/Users/XX/Downloads/romfs/AI/TurnEventNPC.event.root.ainb"));
        //application::manager::UIMgr::OpenWindow(std::make_unique<application::rendering::ainb::UIAINBEditor>("/Users/XX/Downloads/romfs/Logic/Dungeon001_1800.logic.root.ainb"));
        //application::manager::UIMgr::OpenWindow(std::make_unique<application::rendering::ainb::UIAINBEditor>("/Users/XX/Downloads/romfs/Sequence/_LocalModule_ffa16b344754.module.ainb"));

        //application::manager::UIMgr::OpenWindow(std::make_unique<application::rendering::ainb::UIAINBEditor>());

        /*
        for (const auto& DirEntry : std::filesystem::directory_iterator(application::util::FileUtil::GetRomFSFilePath("Sequence")))
        {
            if (DirEntry.is_regular_file())
            {
                std::ifstream File(DirEntry.path().string(), std::ios::binary);

                if (!File.eof() && !File.fail())
                {
                    File.seekg(0, std::ios_base::end);
                    std::streampos FileSize = File.tellg();

                    std::vector<unsigned char> Bytes(FileSize);

                    File.seekg(0, std::ios_base::beg);
                    File.read(reinterpret_cast<char*>(Bytes.data()), FileSize);

                    File.close();

                    //this->AINBFile::AINBFile(Bytes);
                    //DecodeAINB(Bytes, "AI/" + DirEntry.path().filename().string());
                    application::file::game::ainb::AINBFile File;
                    File.Initialize(Bytes, true);

                    std::unordered_map<std::string, uint32_t> NodeCounts;
                    for (application::file::game::ainb::AINBFile::Node& Node : File.Nodes)
                    {
                        if (Node.Name.ends_with(".module"))
                        {
                            NodeCounts[Node.Name]++;
                        }
                    }

                    std::vector<application::file::game::ainb::AINBFile::EmbeddedAinb> OldModules = File.EmbeddedAinbArray;

                    File.ToBinary();

                    if (OldModules.size() != File.EmbeddedAinbArray.size())
                    {
                        application::util::Logger::Warning("Debug", "Size mismatch, Old: %u, New: %u, Name: %s", OldModules.size(), File.EmbeddedAinbArray.size(), DirEntry.path().filename().string().c_str());
                    }
                    else
                    {
                        for (uint8_t i = 0; i < File.EmbeddedAinbArray.size(); i++)
                        {
                            if (OldModules[i].Count != File.EmbeddedAinbArray[i].Count)
                            {
                                application::util::Logger::Warning("Debug", "Wrong Count, Old: %u, New: %u, Name: %s, Module: %s", OldModules[i].Count, File.EmbeddedAinbArray[i].Count, DirEntry.path().filename().string().c_str(), File.EmbeddedAinbArray[i].FilePath.c_str());
                            }
                        }
                    }
                }
                else
                {
                    application::util::Logger::Error("AINBNodeMgr", "Could not open file \"%s\"", DirEntry.path().string().c_str());
                }
            }
        }
        */

        application::file::tool::AdditionalAINBNodes::Load(application::util::FileUtil::GetWorkingDirFilePath("Nodes.ainode.byml"));

        application::manager::ProjectMgr::Initialize();
        application::file::tool::ProjectConfigFile::Load(application::util::FileUtil::GetWorkingDirFilePath("Project.eprojcfg"));

        application::file::tool::PathConfigFile::Load(application::util::FileUtil::GetWorkingDirFilePath("Config.epathcfg"));
        application::file::tool::GitIdentityFile::Load(application::util::FileUtil::GetWorkingDirFilePath("Identity.egit"));
        application::file::tool::LicenseFile::gNoLicenseFileFound = !application::util::FileUtil::FileExists(application::util::FileUtil::GetWorkingDirFilePath("License.elicense"));


        application::util::Logger::Info("Editor", "All stage 1 systems initialized");
        return true;
	}

    void Editor::InitializeDefaultModels()
    {
        application::file::game::bfres::BfresFile DefaultModel("Default", application::util::FileUtil::ReadFile("Assets/Models/TexturedCube.bfres"), "Assets/Models");
        application::file::game::bfres::BfresFile AreaModel("Area", application::util::FileUtil::ReadFile("Assets/Models/TexturedArea.bfres"), "Assets/Models");
        application::file::game::bfres::BfresFile SphereModel("Sphere", application::util::FileUtil::ReadFile("Assets/Models/TexturedSphere.bfres"), "Assets/Models");
        DefaultModel.mDefaultModel = true;
        AreaModel.mDefaultModel = true;
        SphereModel.mDefaultModel = true;

        application::manager::BfresFileMgr::gBfresFiles.insert({ "Default", DefaultModel });
        application::manager::BfresFileMgr::gBfresFiles.insert({ "Area", AreaModel });
        application::manager::BfresFileMgr::gBfresFiles.insert({ "Sphere", SphereModel });

        application::file::game::bfres::BfresFile* DefaultModelPtr = &application::manager::BfresFileMgr::gBfresFiles["Default"];
        application::file::game::bfres::BfresFile* AreaModelPtr = &application::manager::BfresFileMgr::gBfresFiles["Area"];
        application::file::game::bfres::BfresFile* SphereModelPtr = &application::manager::BfresFileMgr::gBfresFiles["Sphere"];

        application::manager::BfresRendererMgr::gModels.insert({ DefaultModelPtr, application::gl::BfresRenderer(DefaultModelPtr) });
        application::manager::BfresRendererMgr::gModels.insert({ AreaModelPtr, application::gl::BfresRenderer(AreaModelPtr) });
        application::manager::BfresRendererMgr::gModels.insert({ SphereModelPtr, application::gl::BfresRenderer(SphereModelPtr) });

        application::manager::BfresRendererMgr::gModels[AreaModelPtr].mIsSystemModelTransparent = true; //Makes that Areas are rendered last
    }

    bool Editor::InitializeRomFSPathDependant()
    {
        if (gInitializedStage2)
            return true;

        application::util::Logger::Info("Editor", "Starting stage 2 initialization");
        if (application::util::FileUtil::gPathsValid)
        {
            application::file::game::ZStdBackend::InitializeDicts(application::util::FileUtil::GetRomFSFilePath("Pack/ZsDic.pack.zs"));

            //Detecting internal game version
            for (const auto& DirEntry : std::filesystem::recursive_directory_iterator(application::util::FileUtil::GetRomFSFilePath("System/Resource", false))) 
            {
                std::string FileName = DirEntry.path().string().substr(DirEntry.path().string().find_last_of("/\\") + 1);
                if (FileName.find("ResourceSizeTable.Product.") != 0 ||
                    FileName.find(".rsizetable.zs") == std::string::npos) 
                {
                    continue;
                }

                size_t Start = FileName.find("ResourceSizeTable.Product.") + strlen("ResourceSizeTable.Product.");
                size_t End = FileName.find(".", Start);

                std::string Version = FileName.substr(Start, End - Start);
                if (stoi(gInternalGameVersion) <= stoi(Version)) 
                {
                    Editor::gInternalGameVersion = Version;
                }
            }

            //Detecting internal game data list version
            for (const auto& DirEntry : std::filesystem::recursive_directory_iterator(application::util::FileUtil::GetRomFSFilePath("GameData", false)))
            {
                std::string FileName = DirEntry.path().string().substr(DirEntry.path().string().find_last_of("/\\") + 1);
                if (FileName.find("GameDataList.Product.") != 0 ||
                    FileName.find(".byml.zs") == std::string::npos)
                {
                    continue;
                }

                size_t Start = FileName.find("GameDataList.Product.") + strlen("GameDataList.Product.");
                size_t End = FileName.find(".", Start);

                std::string Version = FileName.substr(Start, End - Start);
                if (stoi(gInternalGameDataListVersion) <= stoi(Version))
                {
                    Editor::gInternalGameDataListVersion = Version;
                }
            }

            application::game::actor_component::ActorComponentFactory::Initialize();
            application::manager::ActorPackMgr::LoadActorList();
            application::manager::ActorInfoMgr::Initialize();
            application::manager::PluginMgr::Initialize();
            application::rendering::popup::PopUpCommon::Initialize();

            application::file::game::bfres::BfresFile::Initialize();
            InitializeDefaultModels();

            application::manager::GameDataListMgr::Initialize();
            
            application::util::GitManager::Initialize();

            /*
            application::tool::AINBFileSearcher Searcher;
            Searcher.Start([](application::file::game::ainb::AINBFile& File)
                {
                    return File.Commands.size() > 4 && File.Nodes.size() > 30;
                });
                */

            /*
            application::file::game::phive::PhiveStaticCompoundFile BrokenStaticCompound(application::file::game::ZStdBackend::Decompress(application::util::FileUtil::GetSaveFilePath("Phive/StaticCompoundBody/MinusField/MinusField_H-3_X3_Z3.Nin_NX_NVN.bphsc.zs")));
            application::file::game::phive::PhiveStaticCompoundFile VanillaStaticCompound(application::file::game::ZStdBackend::Decompress(application::util::FileUtil::GetRomFSFilePath("Phive/StaticCompoundBody/MinusField/MinusField_H-3_X3_Z3.Nin_NX_NVN.bphsc.zs", false)));

            BrokenStaticCompound.mExternalBphshMeshes[0] = VanillaStaticCompound.mExternalBphshMeshes[0];
            application::util::FileUtil::WriteFile(application::util::FileUtil::GetSaveFilePath("Phive/StaticCompoundBody/MinusField/MinusField_H-3_X3_Z3.Nin_NX_NVN.bphsc.zs"), application::file::game::ZStdBackend::Compress(BrokenStaticCompound.ToBinary(), application::file::game::ZStdBackend::Dictionary::Base));
            */

            /*
            application::file::game::phive::PhiveStaticCompoundFile StaticCompound(application::file::game::ZStdBackend::Decompress(application::util::FileUtil::GetSaveFilePath("Phive/StaticCompoundBody/SmallDungeon/Dungeon325.Nin_NX_NVN.bphsc.zs")));
            application::file::game::phive::PhiveStaticCompoundFile::PhiveStaticCompoundExternalMesh ExternalMesh;
            ExternalMesh.mPositionX = 0.0f;
            ExternalMesh.mPositionY = 0.0f;
            ExternalMesh.mPositionZ = 0.0f;
            ExternalMesh.mRotationX = 0.0f;
            ExternalMesh.mRotationY = 0.0f;
            ExternalMesh.mRotationZ = 0.0f;
            ExternalMesh.mScaleX = 1.0f;
            ExternalMesh.mScaleY = 1.0f;
            ExternalMesh.mScaleZ = 1.0f;
            ExternalMesh.mReserve3 = 0;
            ExternalMesh.mReserve4 = 0;
            StaticCompound.mExternalMeshArray.push_back(ExternalMesh);

            application::file::game::phive::PhiveStaticCompoundFile::PhiveStaticCompoundBphsh ExternalBphsh;
            ExternalBphsh.mReserializeCollision = true;
			ExternalBphsh.mPhiveShape = application::file::game::phive::shape::PhiveShape(application::file::game::ZStdBackend::Decompress(application::util::FileUtil::GetSaveFilePath("Phive/Shape/Dcc/DgnObj_ChristmasCelebration_DungeonPart_Road__Physical.Nin_NX_NVN.bphsh.zs")));
			StaticCompound.mExternalBphshMeshes.push_back(ExternalBphsh);

            application::util::FileUtil::WriteFile(application::util::FileUtil::GetSaveFilePath("Phive/StaticCompoundBody/SmallDungeon/Dungeon325.Nin_NX_NVN.bphsc.zs"), application::file::game::ZStdBackend::Compress(StaticCompound.ToBinary(), application::file::game::ZStdBackend::Dictionary::Base));
            */


            //application::file::game::byml::BymlFile ActorInfoByml(application::file::game::ZStdBackend::Decompress(application::util::FileUtil::GetRomFSFilePath("RSDB/ActorInfo.Product." + application::Editor::gInternalGameVersion + ".rstbl.byml.zs")));
            //application::util::FileUtil::WriteFile(application::util::FileUtil::GetWorkingDirFilePath("ActorInfo.Product." + gInternalGameVersion + ".rstbl.byml"), ActorInfoByml.ToBinary(application::file::game::byml::BymlFile::TableGeneration::AUTO));

            //application::forever_confidential::PhiveClassGenerator::Initialize("H:/XX/switchemulator/Zelda TotK/MapEditorV6/rootDir/WorkingDir/havok_type_info.json");
            //application::forever_confidential::PhiveClassGenerator::GeneratePhiveClasses("710417c840");
            //application::file::game::phive::PhiveShape2 NewShape(application::file::game::ZStdBackend::Decompress(application::util::FileUtil::GetRomFSFilePath("Phive/Shape/Dcc/DgnObj_Small_BoxFloor_2x2_A__Physical.Nin_NX_NVN.bphsh.zs")));
            //application::util::FileUtil::WriteFile(application::util::FileUtil::GetWorkingDirFilePath("PhiveShape2.bphsh"), NewShape.ToBinary());

            //application::manager::UIMgr::OpenWindow(std::make_unique<application::rendering::general::UIContentBrowser>());

 

            application::util::Logger::Info("Editor", "All stage 2 systems initialized, application state: READY");

            gInitializedStage2 = true;
            

            /*
            application::file::game::terrain::TerrainSceneFile MainFieldTscb;
            MainFieldTscb.Initialize(application::util::FileUtil::ReadFile(application::util::FileUtil::GetRomFSFilePath("TerrainArc/MainField.tscb")));

            application::file::game::terrain::TerrainSceneFile::ResArea& Area = MainFieldTscb.mTerrainScene.mAreas.back().mObj;

            application::util::FileUtil::WriteFile(application::util::FileUtil::GetWorkingDirFilePath("MinusFieldReserialized.tscb"), MainFieldTscb.ToBinary());
            */

            return true;
        }
        else
        {
#if !defined(TOOL_FORCE_PATHS)
            gInitializedStage2 = true;
#endif
            application::util::Logger::Info("Editor", "Stage 2 initialization failed due to invalid external paths. Please set the correct RomFS and Model Dump path");
            return false;
        }
    }

    void Editor::Shutdown()
    {
        application::util::GitManager::Shutdown();
        application::manager::UIMgr::Cleanup();
        application::file::tool::AdditionalAINBNodes::Save(application::util::FileUtil::GetWorkingDirFilePath("Nodes.ainode.byml"));
        application::file::tool::ProjectConfigFile::Save(application::util::FileUtil::GetWorkingDirFilePath("Project.eprojcfg"));
        if(!application::util::GitManager::gUserIdentity.mAccessToken.empty()) application::file::tool::GitIdentityFile::Save(application::util::FileUtil::GetWorkingDirFilePath("Identity.egit"));
    }
}