#include "ActorInfoMgr.h"

#include <util/FileUtil.h>
#include <util/Logger.h>
#include <file/game/zstd/ZStdBackend.h>
#include <Editor.h>
#include <iostream>
#include <algorithm>
#include <unordered_set>
#include <filesystem>

namespace application::manager
{
    std::unordered_map<std::string, ActorInfoMgr::ActorInfoEntry> ActorInfoMgr::gActorInfoEntries;

    std::vector<std::string> ActorInfoMgr::ActorInfoEntry::gCategoryNames = { "Accessory", "MapActiveNearOnly", "MapStatic", "Animal", "MapMerged", "NonPlayerArmor", "Shootable", "ObjectPouchContentActive", "Shield", "NPC", "System", "AreaBgm", "ObjectPouchContent", "EventNPC", "ObjectGeneralActive", "EventObject", "Sheath", "EffectLocater", "SoundLocater", "Weapon", "CapturableActor", "MapActive", "Armor", "ObjectGeneral", "Bow", "Enemy", "Cave", "WeakPoint", "ObjectStaticActive", "ObjectStatic", "SpotBgm", "ObjectImmortal", "Tool", "Camera", "GameBalanceLocator", "Player" };
    std::vector<std::string> ActorInfoMgr::ActorInfoEntry::gClassNameNames = { "GameActor" };
    std::vector<std::string> ActorInfoMgr::ActorInfoEntry::gCalcPriorityNames = { "Before", "After" };
    std::vector<std::string> ActorInfoMgr::ActorInfoEntry::gLoadRadiusDistanceTypeNames = { "EuclideanXYZ", "ChebyshevXZ" };
    std::vector<std::string> ActorInfoMgr::ActorInfoEntry::gApplyScaleTypeNames = { "ScaleMax", "AddLength", "Ignore", "ScaleMaxLimit100", "ScaleMaxLimit2", "AddMax" };
    std::vector<std::string> ActorInfoMgr::ActorInfoEntry::gCalcRadiusDistanceTypeNames = { "EuclideanXYZ", "ChebyshevXZ" };
    std::vector<std::string> ActorInfoMgr::ActorInfoEntry::gDisplayRadiusDistanceTypeNames = { "EuclideanXYZ", "ChebyshevXZ" };

    void ActorInfoMgr::GenerateStringList()
    {
        const std::string RootName = "DisplayRadiusDistanceType";
        std::unordered_set<std::string> Names;

        for (auto& [Key, Entry] : gActorInfoEntries)
        {
            //if(Entry.mDisplayRadiusDistanceType.has_value()) Names.insert(Entry.mDisplayRadiusDistanceType.value());
        }

        int Count = 0;

        std::cout << "enum class " << RootName << " : int\n{\n";
        for (const std::string& Name : Names)
        {
            std::cout << "  " << Name << " = " << Count << ",\n";
            Count++;
        }
        std::cout << "}\n\n";

        for (const std::string& Name : Names)
        {
            std::cout << "\"" <<  Name << "\", ";
        }
        std::cout << std::endl;
    }

    void ActorInfoMgr::GenerateActorInfoStruct(application::file::game::byml::BymlFile& File)
    {
        //Key -> {DataType, Required}
        std::unordered_map<std::string, std::pair<application::file::game::byml::BymlFile::Type, bool>> Keys;
        for (auto& Node : File.GetNodes())
        {
            for (auto& [Key, Data] : Keys)
            {
                if (!Data.second)
                    continue;

                bool Found = false;
                for (auto& Child : Node.GetChildren())
                {
                    if (Child.GetKey() == Key)
                    {
                        Found = true;
                        break;
                    }
                }

                if (!Found)
                    Data.second = false;
            }

            for (auto& Child : Node.GetChildren())
            {
                if (!Keys.contains(Child.GetKey()))
                {
                    Keys[Child.GetKey()] = std::make_pair(Child.GetType(), true);
                }
            }
        }

        std::cout << "struct ActorInfoEntry\n{\n";
        for (auto& [Key, Data] : Keys)
        {
            std::string DataType = "";
            switch (Data.first)
            {
                case file::game::byml::BymlFile::Type::Bool:
                    DataType = "bool";
                    break;
                case file::game::byml::BymlFile::Type::Double:
                    DataType = "double";
                    break;
                case file::game::byml::BymlFile::Type::Float:
                    DataType = "float";
                    break;
                case file::game::byml::BymlFile::Type::Int32:
                    DataType = "int32_t";
                    break;
                case file::game::byml::BymlFile::Type::UInt32:
                    DataType = "uint32_t";
                    break;
                case file::game::byml::BymlFile::Type::Int64:
                    DataType = "int64_t";
                    break;
                case file::game::byml::BymlFile::Type::UInt64:
                    DataType = "uint64_t";
                    break;
                case file::game::byml::BymlFile::Type::StringIndex:
                    DataType = "std::string";
                    break;
                default:
                    application::util::Logger::Warning("ActorInfoMgr", "Could not generate data type string, key: %s, required: %s", Key.c_str(), Data.second ? "true" : "false");
            }

            if (DataType.empty())
                continue;

            if (!Data.second)
                DataType = "std::optional<" + DataType + ">";

            std::cout << "  " << DataType << " m" << Key << (!Data.second ? " = std::nullopt;" : ";") << "\n";
        }
        std::cout << "};";
    }

    void ActorInfoMgr::ConvertBymlNodeToActorInfoEntry(application::file::game::byml::BymlFile::Node &Node, ActorInfoMgr::ActorInfoEntry &Entry)
    {
        for (auto& Child : Node.GetChildren())
        {
            /* Required */
            if (Child.GetKey() == "Category") Entry.mCategory = static_cast<ActorInfoEntry::Category>(std::distance(ActorInfoEntry::gCategoryNames.begin(), std::find(ActorInfoEntry::gCategoryNames.begin(), ActorInfoEntry::gCategoryNames.end(), Child.GetValue<std::string>())));
            if (Child.GetKey() == "ClassName") Entry.mClassName = static_cast<ActorInfoEntry::ClassName>(std::distance(ActorInfoEntry::gClassNameNames.begin(), std::find(ActorInfoEntry::gClassNameNames.begin(), ActorInfoEntry::gClassNameNames.end(), Child.GetValue<std::string>())));
            if (Child.GetKey() == "__RowId") Entry.mRowId = Child.GetValue<std::string>();
            if (Child.GetKey() == "InstanceHeapSize") Entry.mInstanceHeapSize = Child.GetValue<int32_t>();
            if (Child.GetKey() == "PreActorRenderInfoHeapSize") Entry.mPreActorRenderInfoHeapSize = Child.GetValue<int32_t>();
            if (Child.GetKey() == "DeleteRadiusOffset") Entry.mDeleteRadiusOffset = Child.GetValue<float>();
            if (Child.GetKey() == "UnloadRadiusOffset") Entry.mUnloadRadiusOffset = Child.GetValue<float>();

            /* Optional */
            if (Child.GetKey() == "ActorName") Entry.mActorName = Child.GetValue<std::string>();
            if (Child.GetKey() == "CalcPriority") Entry.mCalcPriority = static_cast<ActorInfoEntry::CalcPriority>(std::distance(ActorInfoEntry::gCalcPriorityNames.begin(), std::find(ActorInfoEntry::gCalcPriorityNames.begin(), ActorInfoEntry::gCalcPriorityNames.end(), Child.GetValue<std::string>())));
            if (Child.GetKey() == "ELinkUserName") Entry.mELinkUserName = Child.GetValue<std::string>();
            if (Child.GetKey() == "FmdbName") Entry.mFmdbName = Child.GetValue<std::string>();
            if (Child.GetKey() == "ModelProjectName") Entry.mModelProjectName = Child.GetValue<std::string>();
            if (Child.GetKey() == "SLinkUserName") Entry.mSLinkUserName = Child.GetValue<std::string>();
            if (Child.GetKey() == "ModelVariationFmabName") Entry.mModelVariationFmabName = Child.GetValue<std::string>();
            if (Child.GetKey() == "ApplyScaleType") Entry.mApplyScaleType = static_cast<ActorInfoEntry::ApplyScaleType>(std::distance(ActorInfoEntry::gApplyScaleTypeNames.begin(), std::find(ActorInfoEntry::gApplyScaleTypeNames.begin(), ActorInfoEntry::gApplyScaleTypeNames.end(), Child.GetValue<std::string>())));
            if (Child.GetKey() == "CalcRadiusDistanceType") Entry.mCalcRadiusDistanceType = static_cast<ActorInfoEntry::CalcRadiusDistanceType>(std::distance(ActorInfoEntry::gCalcRadiusDistanceTypeNames.begin(), std::find(ActorInfoEntry::gCalcRadiusDistanceTypeNames.begin(), ActorInfoEntry::gCalcRadiusDistanceTypeNames.end(), Child.GetValue<std::string>())));
            if (Child.GetKey() == "DisplayRadiusDistanceType") Entry.mDisplayRadiusDistanceType = static_cast<ActorInfoEntry::DisplayRadiusDistanceType>(std::distance(ActorInfoEntry::gDisplayRadiusDistanceTypeNames.begin(), std::find(ActorInfoEntry::gDisplayRadiusDistanceTypeNames.begin(), ActorInfoEntry::gDisplayRadiusDistanceTypeNames.end(), Child.GetValue<std::string>())));
            if (Child.GetKey() == "LoadRadiusDistanceType") Entry.mLoadRadiusDistanceType = static_cast<ActorInfoEntry::LoadRadiusDistanceType>(std::distance(ActorInfoEntry::gLoadRadiusDistanceTypeNames.begin(), std::find(ActorInfoEntry::gLoadRadiusDistanceTypeNames.begin(), ActorInfoEntry::gLoadRadiusDistanceTypeNames.end(), Child.GetValue<std::string>())));
            if (Child.GetKey() == "ASInfoAutoPlaySkeletals")
            {
                Entry.mASInfoAutoPlaySkeletals.emplace();

                for (auto& Dict : Child.GetChildren())
                {
                    ActorInfoEntry::ASInfoAutoPlaySkeletal Resource = {};

                    for (auto& AnimationResourceValueNode : Dict.GetChildren())
                    {
                        if (AnimationResourceValueNode.GetKey() == "FileName") Resource.mFileName = AnimationResourceValueNode.GetValue<std::string>();
                    }

                    Entry.mASInfoAutoPlaySkeletals.value().push_back(Resource);
                }
            }
            if (Child.GetKey() == "ASInfoAutoPlayBoneVisibilities")
            {
                Entry.mASInfoAutoPlayBoneVisibilities.emplace();

                for (auto& Dict : Child.GetChildren())
                {
                    ActorInfoEntry::ASInfoAutoPlayBoneVisibility Resource = {};

                    for (auto& AnimationResourceValueNode : Dict.GetChildren())
                    {
                        if (AnimationResourceValueNode.GetKey() == "FileName") Resource.mFileName = AnimationResourceValueNode.GetValue<std::string>();
                    }

                    Entry.mASInfoAutoPlayBoneVisibilities.value().push_back(Resource);
                }
            }
            if (Child.GetKey() == "ASInfoAutoPlayMaterials")
            {
                Entry.mASInfoAutoPlayMaterials.emplace();

                for (auto& Dict : Child.GetChildren())
                {
                    ActorInfoEntry::ASInfoAutoPlayMaterial Resource = {};

                    for (auto& AnimationResourceValueNode : Dict.GetChildren())
                    {
                        if (AnimationResourceValueNode.GetKey() == "FileName") Resource.mFileName = AnimationResourceValueNode.GetValue<std::string>();
                        if (AnimationResourceValueNode.GetKey() == "IsRandom") Resource.mIsRandom = AnimationResourceValueNode.GetValue<bool>();
                    }

                    Entry.mASInfoAutoPlayMaterials.value().push_back(Resource);
                }
            }
            if (Child.GetKey() == "AnimationResources")
            {
                Entry.mAnimationResources.emplace();

                for (auto& Dict : Child.GetChildren())
                {
                    ActorInfoEntry::AnimationResource Resource = {};

                    for (auto& AnimationResourceValueNode : Dict.GetChildren())
                    {
                        if (AnimationResourceValueNode.GetKey() == "ModelProjectName") Resource.mModelProjectName = AnimationResourceValueNode.GetValue<std::string>();
                        if (AnimationResourceValueNode.GetKey() == "IsRetarget") Resource.mIsRetarget = AnimationResourceValueNode.GetValue<bool>();
                        if (AnimationResourceValueNode.GetKey() == "IgnoreRetargetRate") Resource.mIgnoreRetargetRate = AnimationResourceValueNode.GetValue<bool>();
                    }

                    Entry.mAnimationResources.value().push_back(Resource);
                }
            }
            
            if (Child.GetKey() == "CalcRadius") Entry.mCalcRadius = Child.GetValue<float>();
            if (Child.GetKey() == "CreateRadiusOffset") Entry.mCreateRadiusOffset = Child.GetValue<float>();
            if (Child.GetKey() == "DisplayRadius") Entry.mDisplayRadius = Child.GetValue<float>();
            if (Child.GetKey() == "LoadRadius") Entry.mLoadRadius = Child.GetValue<float>();
            if (Child.GetKey() == "ModelVariationFmabFrame") Entry.mModelVariationFmabFrame = Child.GetValue<float>();

            if (Child.GetKey() == "DebugModelScale") Entry.mDebugModelScale = Child.GetValue<glm::vec3>();
            if (Child.GetKey() == "ModelAabbMax") Entry.mModelAabbMax = Child.GetValue<glm::vec3>();
            if (Child.GetKey() == "ModelAabbMin") Entry.mModelAabbMin = Child.GetValue<glm::vec3>();

            if (Child.GetKey() == "UsePreActorRenderer") Entry.mUsePreActorRenderer = Child.GetValue<bool>();
            if (Child.GetKey() == "IsNotTurnToActor") Entry.mIsNotTurnToActor = Child.GetValue<bool>();
            if (Child.GetKey() == "IsCalcNodePushBack") Entry.mIsCalcNodePushBack = Child.GetValue<bool>();
            if (Child.GetKey() == "IsEventHideCharacter") Entry.mIsEventHideCharacter = Child.GetValue<bool>();
            if (Child.GetKey() == "IsSinglePreActorRender") Entry.mIsSinglePreActorRender = Child.GetValue<bool>();
            if (Child.GetKey() == "NoUseIActorPresenceJudgeIfBanc") Entry.mNoUseIActorPresenceJudgeIfBanc = Child.GetValue<bool>();
            if (Child.GetKey() == "IsFarActor") Entry.mIsFarActor = Child.GetValue<bool>();
        }
    }

    void ActorInfoMgr::ConvertActorInfoEntryToBymlNode(application::file::game::byml::BymlFile::Node& Node, ActorInfoEntry& Entry)
    {
        if (Entry.mActorName.has_value()) Node.mChildren.emplace_back(application::file::game::byml::BymlFile::Type::StringIndex, "ActorName", Entry.mActorName.value());
        
        if (Entry.mAnimationResources.has_value())
        {
            application::file::game::byml::BymlFile::Node AnimationResourcesModelProjectNameNode(application::file::game::byml::BymlFile::Type::Array, "AnimationResources");

            for (ActorInfoEntry::AnimationResource& Resource : Entry.mAnimationResources.value())
            {
                application::file::game::byml::BymlFile::Node Root(application::file::game::byml::BymlFile::Type::Dictionary);

                if (Resource.mModelProjectName.has_value()) Root.mChildren.emplace_back(application::file::game::byml::BymlFile::Type::StringIndex, "ModelProjectName", Resource.mModelProjectName.value());
                if (Resource.mIsRetarget.has_value()) Root.mChildren.emplace_back(application::file::game::byml::BymlFile::Type::Bool, "IsRetarget", Resource.mIsRetarget.value());
                if (Resource.mIgnoreRetargetRate.has_value()) Root.mChildren.emplace_back(application::file::game::byml::BymlFile::Type::Bool, "IgnoreRetargetRate", Resource.mIgnoreRetargetRate.value());

                AnimationResourcesModelProjectNameNode.mChildren.push_back(Root);
            }

            Node.mChildren.push_back(AnimationResourcesModelProjectNameNode);
        }

        if (Entry.mApplyScaleType.has_value()) Node.mChildren.emplace_back(application::file::game::byml::BymlFile::Type::StringIndex, "ApplyScaleType", ActorInfoEntry::gApplyScaleTypeNames[static_cast<int>(Entry.mApplyScaleType.value())]);
        
        if (Entry.mASInfoAutoPlayMaterials.has_value())
        {
            application::file::game::byml::BymlFile::Node ASInfoAutoPlayMaterialsNode(application::file::game::byml::BymlFile::Type::Array, "ASInfoAutoPlayMaterials");

            for (ActorInfoEntry::ASInfoAutoPlayMaterial& Resource : Entry.mASInfoAutoPlayMaterials.value())
            {
                application::file::game::byml::BymlFile::Node Root(application::file::game::byml::BymlFile::Type::Dictionary);

                if (Resource.mFileName.has_value()) Root.mChildren.emplace_back(application::file::game::byml::BymlFile::Type::StringIndex, "FileName", Resource.mFileName.value());
                if (Resource.mIsRandom.has_value()) Root.mChildren.emplace_back(application::file::game::byml::BymlFile::Type::Bool, "IsRandom", Resource.mIsRandom.value());

                ASInfoAutoPlayMaterialsNode.mChildren.push_back(Root);
            }

            Node.mChildren.push_back(ASInfoAutoPlayMaterialsNode);
        }

        if (Entry.mASInfoAutoPlaySkeletals.has_value())
        {
            application::file::game::byml::BymlFile::Node ASInfoAutoPlaySkeletalsFileNameNode(application::file::game::byml::BymlFile::Type::Array, "ASInfoAutoPlaySkeletals");

            for (ActorInfoEntry::ASInfoAutoPlaySkeletal& Resource : Entry.mASInfoAutoPlaySkeletals.value())
            {
                application::file::game::byml::BymlFile::Node Root(application::file::game::byml::BymlFile::Type::Dictionary);

                if (Resource.mFileName.has_value()) Root.mChildren.emplace_back(application::file::game::byml::BymlFile::Type::StringIndex, "FileName", Resource.mFileName.value());

                ASInfoAutoPlaySkeletalsFileNameNode.mChildren.push_back(Root);
            }

            Node.mChildren.push_back(ASInfoAutoPlaySkeletalsFileNameNode);
        }

        if (Entry.mASInfoAutoPlayBoneVisibilities.has_value())
        {
            application::file::game::byml::BymlFile::Node ASInfoAutoPlayBoneVisibilitiesNode(application::file::game::byml::BymlFile::Type::Array, "ASInfoAutoPlayBoneVisibilities");

            for (ActorInfoEntry::ASInfoAutoPlayBoneVisibility& Resource : Entry.mASInfoAutoPlayBoneVisibilities.value())
            {
                application::file::game::byml::BymlFile::Node Root(application::file::game::byml::BymlFile::Type::Dictionary);

                if (Resource.mFileName.has_value()) Root.mChildren.emplace_back(application::file::game::byml::BymlFile::Type::StringIndex, "FileName", Resource.mFileName.value());

                ASInfoAutoPlayBoneVisibilitiesNode.mChildren.push_back(Root);
            }

            Node.mChildren.push_back(ASInfoAutoPlayBoneVisibilitiesNode);
        }

        if (Entry.mCalcRadius.has_value()) Node.mChildren.emplace_back(application::file::game::byml::BymlFile::Type::Float, "CalcRadius", Entry.mCalcRadius.value());
        if (Entry.mCalcRadiusDistanceType.has_value()) Node.mChildren.emplace_back(application::file::game::byml::BymlFile::Type::StringIndex, "CalcRadiusDistanceType", ActorInfoEntry::gCalcRadiusDistanceTypeNames[static_cast<int>(Entry.mCalcRadiusDistanceType.value())]);
        if (Entry.mCalcPriority.has_value()) Node.mChildren.emplace_back(application::file::game::byml::BymlFile::Type::StringIndex, "CalcPriority", ActorInfoEntry::gCalcPriorityNames[static_cast<int>(Entry.mCalcPriority.value())]);
        Node.mChildren.emplace_back(application::file::game::byml::BymlFile::Type::StringIndex, "Category", ActorInfoEntry::gCategoryNames[static_cast<int>(Entry.mCategory)]);
        Node.mChildren.emplace_back(application::file::game::byml::BymlFile::Type::StringIndex, "ClassName", ActorInfoEntry::gClassNameNames[static_cast<int>(Entry.mClassName)]);
        if (Entry.mCreateRadiusOffset.has_value()) Node.mChildren.emplace_back(application::file::game::byml::BymlFile::Type::Float, "CreateRadiusOffset", Entry.mCreateRadiusOffset.value());
        
        if (Entry.mDebugModelScale.has_value())
        {
            application::file::game::byml::BymlFile::Node VectorRoot(application::file::game::byml::BymlFile::Type::Dictionary, "DebugModelScale");

            VectorRoot.mChildren.emplace_back(application::file::game::byml::BymlFile::Type::Float, "X", Entry.mDebugModelScale.value().x);
            VectorRoot.mChildren.emplace_back(application::file::game::byml::BymlFile::Type::Float, "Y", Entry.mDebugModelScale.value().y);
            VectorRoot.mChildren.emplace_back(application::file::game::byml::BymlFile::Type::Float, "Z", Entry.mDebugModelScale.value().z);

            Node.mChildren.push_back(VectorRoot);
        }

        Node.mChildren.emplace_back(application::file::game::byml::BymlFile::Type::Float, "DeleteRadiusOffset", Entry.mDeleteRadiusOffset);
        if (Entry.mDisplayRadius.has_value()) Node.mChildren.emplace_back(application::file::game::byml::BymlFile::Type::Float, "DisplayRadius", Entry.mDisplayRadius.value());
        if (Entry.mDisplayRadiusDistanceType.has_value()) Node.mChildren.emplace_back(application::file::game::byml::BymlFile::Type::StringIndex, "DisplayRadiusDistanceType", ActorInfoEntry::gDisplayRadiusDistanceTypeNames[static_cast<int>(Entry.mDisplayRadiusDistanceType.value())]);
        if (Entry.mELinkUserName.has_value()) Node.mChildren.emplace_back(application::file::game::byml::BymlFile::Type::StringIndex, "ELinkUserName", Entry.mELinkUserName.value());
        if (Entry.mFmdbName.has_value()) Node.mChildren.emplace_back(application::file::game::byml::BymlFile::Type::StringIndex, "FmdbName", Entry.mFmdbName.value());
        Node.mChildren.emplace_back(application::file::game::byml::BymlFile::Type::Int32, "InstanceHeapSize", Entry.mInstanceHeapSize);
        if (Entry.mIsCalcNodePushBack.has_value()) Node.mChildren.emplace_back(application::file::game::byml::BymlFile::Type::Bool, "IsCalcNodePushBack", Entry.mIsCalcNodePushBack.value());
        if (Entry.mIsEventHideCharacter.has_value()) Node.mChildren.emplace_back(application::file::game::byml::BymlFile::Type::Bool, "IsEventHideCharacter", Entry.mIsEventHideCharacter.value());
        if (Entry.mIsFarActor.has_value()) Node.mChildren.emplace_back(application::file::game::byml::BymlFile::Type::Bool, "IsFarActor", Entry.mIsFarActor.value());
        if (Entry.mIsNotTurnToActor.has_value()) Node.mChildren.emplace_back(application::file::game::byml::BymlFile::Type::Bool, "IsNotTurnToActor", Entry.mIsNotTurnToActor.value());
        if (Entry.mIsSinglePreActorRender.has_value()) Node.mChildren.emplace_back(application::file::game::byml::BymlFile::Type::Bool, "IsSinglePreActorRender", Entry.mIsSinglePreActorRender.value());
        if (Entry.mLoadRadius.has_value()) Node.mChildren.emplace_back(application::file::game::byml::BymlFile::Type::Float, "LoadRadius", Entry.mLoadRadius.value());
        if (Entry.mLoadRadiusDistanceType.has_value()) Node.mChildren.emplace_back(application::file::game::byml::BymlFile::Type::StringIndex, "LoadRadiusDistanceType", ActorInfoEntry::gLoadRadiusDistanceTypeNames[static_cast<int>(Entry.mLoadRadiusDistanceType.value())]);
       
        if (Entry.mModelAabbMax.has_value())
        {
            application::file::game::byml::BymlFile::Node VectorRoot(application::file::game::byml::BymlFile::Type::Dictionary, "ModelAabbMax");

            VectorRoot.mChildren.emplace_back(application::file::game::byml::BymlFile::Type::Float, "X", Entry.mModelAabbMax.value().x);
            VectorRoot.mChildren.emplace_back(application::file::game::byml::BymlFile::Type::Float, "Y", Entry.mModelAabbMax.value().y);
            VectorRoot.mChildren.emplace_back(application::file::game::byml::BymlFile::Type::Float, "Z", Entry.mModelAabbMax.value().z);

            Node.mChildren.push_back(VectorRoot);
        }
        
        if (Entry.mModelAabbMin.has_value())
        {
            application::file::game::byml::BymlFile::Node VectorRoot(application::file::game::byml::BymlFile::Type::Dictionary, "ModelAabbMin");

            VectorRoot.mChildren.emplace_back(application::file::game::byml::BymlFile::Type::Float, "X", Entry.mModelAabbMin.value().x);
            VectorRoot.mChildren.emplace_back(application::file::game::byml::BymlFile::Type::Float, "Y", Entry.mModelAabbMin.value().y);
            VectorRoot.mChildren.emplace_back(application::file::game::byml::BymlFile::Type::Float, "Z", Entry.mModelAabbMin.value().z);

            Node.mChildren.push_back(VectorRoot);
        }
        
        if (Entry.mModelProjectName.has_value()) Node.mChildren.emplace_back(application::file::game::byml::BymlFile::Type::StringIndex, "ModelProjectName", Entry.mModelProjectName.value());
        if (Entry.mModelVariationFmabFrame.has_value()) Node.mChildren.emplace_back(application::file::game::byml::BymlFile::Type::Float, "ModelVariationFmabFrame", Entry.mModelVariationFmabFrame.value());
        if (Entry.mModelVariationFmabName.has_value()) Node.mChildren.emplace_back(application::file::game::byml::BymlFile::Type::StringIndex, "ModelVariationFmabName", Entry.mModelVariationFmabName.value());
        if (Entry.mNoUseIActorPresenceJudgeIfBanc.has_value()) Node.mChildren.emplace_back(application::file::game::byml::BymlFile::Type::Bool, "NoUseIActorPresenceJudgeIfBanc", Entry.mNoUseIActorPresenceJudgeIfBanc.value());
        Node.mChildren.emplace_back(application::file::game::byml::BymlFile::Type::Int32, "PreActorRenderInfoHeapSize", Entry.mPreActorRenderInfoHeapSize);
        if (Entry.mSLinkUserName.has_value()) Node.mChildren.emplace_back(application::file::game::byml::BymlFile::Type::StringIndex, "SLinkUserName", Entry.mSLinkUserName.value());
        Node.mChildren.emplace_back(application::file::game::byml::BymlFile::Type::Float, "UnloadRadiusOffset", Entry.mUnloadRadiusOffset);
        if (Entry.mUsePreActorRenderer.has_value()) Node.mChildren.emplace_back(application::file::game::byml::BymlFile::Type::Bool, "UsePreActorRenderer", Entry.mUsePreActorRenderer.value());
        Node.mChildren.emplace_back(application::file::game::byml::BymlFile::Type::StringIndex, "__RowId", Entry.mRowId);
    }

    ActorInfoMgr::ActorInfoEntry* ActorInfoMgr::GetActorInfo(const std::string& Gyml)
    {
        return &gActorInfoEntries[Gyml];
    }

    void ActorInfoMgr::Initialize()
    {
        application::file::game::byml::BymlFile ActorInfoByml(application::file::game::ZStdBackend::Decompress(application::util::FileUtil::GetRomFSFilePath("RSDB/ActorInfo.Product." + application::Editor::gInternalGameVersion + ".rstbl.byml.zs")));

        gActorInfoEntries.reserve(ActorInfoByml.GetNodes().size());

        for (auto& Node : ActorInfoByml.GetNodes())
        {
            ActorInfoMgr::ActorInfoEntry Entry = {};
            ConvertBymlNodeToActorInfoEntry(Node, Entry);

            gActorInfoEntries.insert(std::make_pair(Entry.mRowId, Entry));
        }

        //GenerateStringList();

        application::util::Logger::Info("ActorInfoMgr", "Loaded %u ActorInfo entries", static_cast<int>(gActorInfoEntries.size()));
    }

    void ActorInfoMgr::Save()
    {
        application::file::game::byml::BymlFile ActorInfoByml;
        ActorInfoByml.GetType() = application::file::game::byml::BymlFile::Type::Array;

        std::vector<std::pair<std::string, ActorInfoMgr::ActorInfoEntry>> SortedEntries(
            ActorInfoMgr::gActorInfoEntries.begin(),
            ActorInfoMgr::gActorInfoEntries.end()
        );

        std::sort(SortedEntries.begin(), SortedEntries.end(),
            [](const auto& a, const auto& b) 
            {
                const std::string& keyA = a.first;
                const std::string& keyB = b.first;

                if (keyA.empty() || keyB.empty()) 
                {
                    return keyA < keyB;
                }

                bool aIsUpper = std::isupper(keyA[0]);
                bool bIsUpper = std::isupper(keyB[0]);

                if (aIsUpper != bIsUpper) 
                {
                    return aIsUpper;
                }

                return keyA < keyB;
            });


        for (auto& [Key, ActorInfo] : SortedEntries)
        {
            application::file::game::byml::BymlFile::Node Node(application::file::game::byml::BymlFile::Type::Dictionary);

            ConvertActorInfoEntryToBymlNode(Node, ActorInfo);

            ActorInfoByml.GetNodes().push_back(Node);
        }

        std::filesystem::create_directories(application::util::FileUtil::GetSaveFilePath("RSDB"));
        application::util::FileUtil::WriteFile(application::util::FileUtil::GetSaveFilePath("RSDB/ActorInfo.Product." + application::Editor::gInternalGameVersion + ".rstbl.byml.zs"), application::file::game::ZStdBackend::Compress(ActorInfoByml.ToBinary(application::file::game::byml::BymlFile::TableGeneration::AUTO), application::file::game::ZStdBackend::Dictionary::Base));
        
        application::util::Logger::Info("ActorInfoMgr", "Serialized and saved %u ActorInfo entries", static_cast<int>(gActorInfoEntries.size()));
    }
}