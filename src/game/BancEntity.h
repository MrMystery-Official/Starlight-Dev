#pragma once

#include <cstdint>
#include <string>
#include <optional>
#include <variant>
#include <glm/glm.hpp>
#include <file/game/byml/BymlFile.h>
#include <game/ActorPack.h>
#include <gl/BfresRenderer.h>

namespace application::game
{
	class BancEntity
	{
	public:
		enum class BancType : uint32_t
		{
			STATIC = 0,
			DYNAMIC = 1,
			MERGED = 2
		};

		struct Link
		{
			uint64_t mSrc = 0;
			uint64_t mDest = 0;
			std::string mGyml = "";
			std::string mName = "";
		};

		struct Rail
		{
			uint64_t mDest = 0;
			std::string mGyml = "";
			std::string mName = "";
		};

		struct Phive
		{
			struct Rail
			{
				struct Node
				{
					std::string mKey = "";
					glm::vec3 mValue = glm::vec3(0);
				};

				bool mIsClosed = false;
				std::vector<Node> mNodes;
				std::string mType = "";
			};

			struct RopeLink
			{
				uint64_t mID = 0;
				std::vector<uint64_t> mOwners;
				std::vector<uint64_t> mRefers;
			};

			struct ConstraintLink
			{
				struct Refer
				{
					uint64_t mOwner = 0;
					std::string mType = "";
				};

				struct Owner
				{
					struct Pose
					{
						glm::vec3 mRotate = glm::vec3(0);
						glm::vec3 mTranslate = glm::vec3(0);
					};

					struct Pivot
					{
						std::optional<int32_t> mAxis = std::nullopt;
						std::optional<int32_t> mAxisA = std::nullopt;
						std::optional<int32_t> mAxisB = std::nullopt;
						
						std::optional<glm::vec3> mPivot = std::nullopt;
						std::optional<glm::vec3> mPivotA = std::nullopt;
						std::optional<glm::vec3> mPivotB = std::nullopt;
					};

					std::map<std::string, std::variant<std::string, bool, int32_t, int64_t, uint32_t, uint64_t, float, glm::vec3>> mBreakableData;
					std::map<std::string, std::variant<std::string, bool, int32_t, int64_t, uint32_t, uint64_t, float, glm::vec3>> mAliasData;
					std::map<std::string, std::variant<std::string, bool, int32_t, int64_t, uint32_t, uint64_t, float, glm::vec3>> mClusterData;
					Pose mOwnerPose;
					std::map<std::string, std::variant<std::string, bool, int32_t, int64_t, uint32_t, uint64_t, float, glm::vec3>> mParamData;
					Pivot mPivotData;
					uint64_t mRefer = 0;
					Pose mReferPose;
					std::string mType = "";
					std::map<std::string, std::variant<std::string, bool, int32_t, int64_t, uint32_t, uint64_t, float, glm::vec3>> mUserData;
				};

				uint64_t mID = 0;
				std::vector<Owner> mOwners;
				std::vector<Refer> mRefers;
			};

			std::vector<Rail> mRails;
			std::optional<RopeLink> mRopeHeadLink = std::nullopt;
			std::optional<RopeLink> mRopeTailLink = std::nullopt;
			std::map<std::string, std::variant<std::string, bool, int32_t, int64_t, uint32_t, uint64_t, float, glm::vec3>> mPlacement;
			std::optional<ConstraintLink> mConstraintLink = std::nullopt;
		};

		struct GameData
		{
			uint8_t mExtraByte = 0;
			uint64_t mHash = 0;
			int32_t mSaveFileIndex = 0;

			bool mOnBancChange = false;
			bool mOnNewDay = false;
			bool mOnDefaultOptionRevert = false;
			bool mOnBloodMoon = false;
			bool mOnNewGame = false;
			bool mUnknown0 = false;
			bool mUnknown1 = false;
			bool mOnZonaiRespawnDay = false;
			bool mOnMaterialRespawn = false;
			bool mNoResetOnNewGame = false;
		};

		enum class FluidShapeType : uint32_t
		{
			BOX = 0,
			CYLINDER = 1
		};

		enum class FluidMaterialType : uint32_t
		{
			WATER = 0,
			LAVA = 1,
			DRIFT_SAND = 2,
			WARM_WATER = 3,
			ICE_WATER = 4,
			ABYSS_WATER = 5
		};

		struct BakedFluid
		{
			static std::vector<const char*> gFluidShapeTypes;
			static std::vector<const char*> gFluidMaterialTypes;
			static std::vector<std::pair<uint32_t, FluidMaterialType>> gFluidMaterialTypeMap;

			static FluidMaterialType ToMaterialType(uint32_t Type);
			static uint32_t ToInternalIdentifier(FluidMaterialType Type);

			bool mBakeFluid = false;
			FluidShapeType mFluidShapeType = FluidShapeType::BOX;
			FluidMaterialType mFluidMaterialType = FluidMaterialType::WATER;
			float mFluidFlowSpeed = 0.0f;
			float mFluidBoxHeight = 5.0f;
		};

		BancEntity(BancType BancType, const std::string& Gyml);
		BancEntity() = default;

		bool GenerateBancEntityInfo();
		bool FromByml(application::file::game::byml::BymlFile::Node& Node);
		bool FromGyml(const std::string& Gyml);
		application::file::game::byml::BymlFile::Node ToByml();

		void ParseDynamicTypeParameter(application::file::game::byml::BymlFile::Node& Node, const std::string& Key, std::map<std::string, std::variant<std::string, bool, int32_t, int64_t, uint32_t, uint64_t, float, glm::vec3>>& Map);
		void WriteDynamicTypeParameter(application::file::game::byml::BymlFile::Node& Node, const std::string& Key, std::map<std::string, std::variant<std::string, bool, int32_t, int64_t, uint32_t, uint64_t, float, glm::vec3>>& Map, bool SkipEmptyCheck = false);

		/* -- Game stuff -- */
		BancType mBancType;
		std::string mGyml;
		uint64_t mHash = 0;
		uint32_t mSRTHash = 0;

		glm::vec3 mTranslate = glm::vec3(0);
		glm::vec3 mRotate = glm::vec3(0); //Degrees
		glm::vec3 mScale = glm::vec3(1, 1, 1);

		bool mBakeable = false;
		bool mIsPhysicsStable = false;
		float mMoveRadius = 0.0f;
		float mExtraCreateRadius = 0.0f;
		bool mIsForceActive = false;
		bool mIsInWater = false;
		bool mTurnActorNearEnemy = false;
		std::string mName = "";
		int32_t mVersion = -1;

		std::vector<Link> mLinks;
		std::vector<Rail> mRails;
		std::map<std::string, std::variant<std::string, bool, int32_t, int64_t, uint32_t, uint64_t, float, glm::vec3>> mDynamic;
		std::map<std::string, std::variant<std::string, bool, int32_t, int64_t, uint32_t, uint64_t, float, glm::vec3>> mExternalParameter;
		std::map<std::string, std::variant<std::string, bool, int32_t, int64_t, uint32_t, uint64_t, float, glm::vec3>> mPresence;

		Phive mPhive;


		/* -- Editor Stuff --  */
		std::vector<application::game::BancEntity>* mMergedActorChildren = nullptr;
		bool mMergedActorChildrenModified = false;
		BakedFluid mBakedFluid;

		//This is set to true if the BancEntity was added using the map editor AND it has not been saved yet
		bool mCheckForHashGeneration = false;

		GameData mGameData;

		application::game::ActorPack* mActorPack = nullptr;
		application::gl::BfresRenderer* mBfresRenderer = nullptr;
	};
}