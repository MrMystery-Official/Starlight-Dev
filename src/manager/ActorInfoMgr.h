#pragma once

#include <file/game/byml/BymlFile.h>
#include <optional>
#include <unordered_map>
#include <glm/vec3.hpp>
#include <vector>

namespace application::manager
{
    namespace ActorInfoMgr
    {
        struct ActorInfoEntry
        {
            static std::vector<std::string> gCategoryNames;
            static std::vector<std::string> gClassNameNames;
            static std::vector<std::string> gCalcPriorityNames;
            static std::vector<std::string> gLoadRadiusDistanceTypeNames;
            static std::vector<std::string> gApplyScaleTypeNames;
            static std::vector<std::string> gCalcRadiusDistanceTypeNames;
            static std::vector<std::string> gDisplayRadiusDistanceTypeNames;

            struct AnimationResource
            {
                std::optional<std::string> mModelProjectName = std::nullopt;
                std::optional<bool> mIsRetarget = std::nullopt;
                std::optional<bool> mIgnoreRetargetRate = std::nullopt;
            };

            struct ASInfoAutoPlaySkeletal
            {
                std::optional<std::string> mFileName = std::nullopt;
            };

            struct ASInfoAutoPlayBoneVisibility
            {
                std::optional<std::string> mFileName = std::nullopt;
            };

            struct ASInfoAutoPlayMaterial
            {
                std::optional<std::string> mFileName = std::nullopt;
                std::optional<bool> mIsRandom = std::nullopt;
            };

            enum class Category : int
            {
                Accessory = 0,
                MapActiveNearOnly = 1,
                MapStatic = 2,
                Animal = 3,
                MapMerged = 4,
                NonPlayerArmor = 5,
                Shootable = 6,
                ObjectPouchContentActive = 7,
                Shield = 8,
                NPC = 9,
                System = 10,
                AreaBgm = 11,
                ObjectPouchContent = 12,
                EventNPC = 13,
                ObjectGeneralActive = 14,
                EventObject = 15,
                Sheath = 16,
                EffectLocater = 17,
                SoundLocater = 18,
                Weapon = 19,
                CapturableActor = 20,
                MapActive = 21,
                Armor = 22,
                ObjectGeneral = 23,
                Bow = 24,
                Enemy = 25,
                Cave = 26,
                WeakPoint = 27,
                ObjectStaticActive = 28,
                ObjectStatic = 29,
                SpotBgm = 30,
                ObjectImmortal = 31,
                Tool = 32,
                Camera = 33,
                GameBalanceLocator = 34,
                Player = 35
            };

            enum class ClassName : int
            {
                GameActor = 0
            };

            enum class CalcPriority : int
            {
                Before = 0,
                After = 1
            };

            enum class LoadRadiusDistanceType : int
            {
                EuclideanXYZ = 0,
                ChebyshevXZ = 1
            };            

            enum class ApplyScaleType : int
            {
                ScaleMax = 0,
                AddLength = 1,
                Ignore = 2,
                ScaleMaxLimit100 = 3,
                ScaleMaxLimit2 = 4,
                AddMax = 5,
            };

            enum class CalcRadiusDistanceType : int
            {
                EuclideanXYZ = 0,
                ChebyshevXZ = 1
            };            
            
            enum class DisplayRadiusDistanceType : int
            {
                EuclideanXYZ = 0,
                ChebyshevXZ = 1
            };

            Category mCategory = Category::System;
            ClassName mClassName = ClassName::GameActor;
            std::string mRowId = ""; // should me m__RowId
            int32_t mInstanceHeapSize = 0;
            int32_t mPreActorRenderInfoHeapSize = 0;
            float mDeleteRadiusOffset = 0.0f;
            float mUnloadRadiusOffset = 0.0f;

            std::optional<std::string> mActorName = std::nullopt;
            std::optional<CalcPriority> mCalcPriority = std::nullopt;
            std::optional<std::vector<AnimationResource>> mAnimationResources = std::nullopt;
            std::optional<glm::vec3> mModelAabbMax = std::nullopt;
            std::optional<float> mCalcRadius = std::nullopt;
            std::optional<LoadRadiusDistanceType> mLoadRadiusDistanceType = std::nullopt;
            std::optional<float> mCreateRadiusOffset = std::nullopt;
            std::optional<float> mDisplayRadius = std::nullopt;
            std::optional<std::string> mELinkUserName = std::nullopt;
            std::optional<std::string> mFmdbName = std::nullopt;
            std::optional<bool> mUsePreActorRenderer = std::nullopt;
            std::optional<glm::vec3> mDebugModelScale = std::nullopt;
            std::optional<float> mLoadRadius = std::nullopt;
            std::optional<glm::vec3> mModelAabbMin = std::nullopt;
            std::optional<std::string> mModelProjectName = std::nullopt;
            std::optional<float> mModelVariationFmabFrame = std::nullopt;
            std::optional<std::string> mModelVariationFmabName = std::nullopt;
            std::optional<std::string> mSLinkUserName = std::nullopt;
            std::optional<std::vector<ASInfoAutoPlaySkeletal>> mASInfoAutoPlaySkeletals = std::nullopt;
            std::optional<ApplyScaleType> mApplyScaleType = std::nullopt;
            std::optional<bool> mIsNotTurnToActor = std::nullopt;
            std::optional<bool> mIsCalcNodePushBack = std::nullopt;
            std::optional<bool> mIsEventHideCharacter = std::nullopt;
            std::optional<bool> mIsSinglePreActorRender = std::nullopt;
            std::optional<bool> mNoUseIActorPresenceJudgeIfBanc = std::nullopt;
            std::optional<std::vector<ASInfoAutoPlayMaterial>> mASInfoAutoPlayMaterials = std::nullopt;
            std::optional<std::vector<ASInfoAutoPlayBoneVisibility>> mASInfoAutoPlayBoneVisibilities = std::nullopt;
            std::optional<CalcRadiusDistanceType> mCalcRadiusDistanceType = std::nullopt;
            std::optional<DisplayRadiusDistanceType> mDisplayRadiusDistanceType = std::nullopt;
            std::optional<bool> mIsFarActor = std::nullopt;
        };

        extern std::unordered_map<std::string, ActorInfoEntry> gActorInfoEntries;

        void GenerateActorInfoStruct(application::file::game::byml::BymlFile& File); //This is a development function not meant to be used anymore
        void GenerateStringList(); //This is a development function not meant to be used anymore

        void ConvertBymlNodeToActorInfoEntry(application::file::game::byml::BymlFile::Node& Node, ActorInfoEntry& Entry);
        void ConvertActorInfoEntryToBymlNode(application::file::game::byml::BymlFile::Node& Node, ActorInfoEntry& Entry);

        ActorInfoEntry* GetActorInfo(const std::string& Gyml);

        void Initialize();
        void Save();
    }
}