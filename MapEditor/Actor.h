#pragma once

#include "Vector3F.h"
#include "GLBfres.h"
#include "Byml.h"
#include "UMii.h"
#include "GameDataListMgr.h"
#include <limits.h>
#include <variant>

struct Actor
{
	enum class Type : int
	{
		Static = 0,
		Dynamic = 1,
		Merged = 2
	};

	struct Link
	{
		uint64_t Src;
		uint64_t Dest;
		std::string Gyml = "";
		std::string Name = "";
	};

	struct Rail
	{
		uint64_t Dest;
		std::string Gyml = "";
		std::string Name = "";
	};

	struct PhiveData
	{
		struct ConstraintLinkData
		{
			struct ReferData
			{
				uint64_t Owner;
				std::string Type;
			};

			struct OwnerData
			{
				struct OwnerPoseData
				{
					Vector3F Rotate;
					Vector3F Translate;
				};

				struct PivotDataStruct
				{
					int32_t Axis = INT_MAX;
					int32_t AxisA = INT_MAX;
					int32_t AxisB = INT_MAX;

					Vector3F Pivot = Vector3F(
						FLT_MAX,
						FLT_MAX,
						FLT_MAX
					);

					Vector3F PivotA = Vector3F(
						FLT_MAX,
						FLT_MAX,
						FLT_MAX
					);

					Vector3F PivotB = Vector3F(
						FLT_MAX,
						FLT_MAX,
						FLT_MAX
					);
				};

				struct ReferPoseData
				{
					Vector3F Rotate;
					Vector3F Translate;
				};

				std::map<std::string, std::string> BreakableData;
				std::map<std::string, std::string> AliasData;
				std::map<std::string, std::string> ClusterData;
				OwnerPoseData OwnerPose;
				std::map<std::string, std::variant<uint32_t, int32_t, uint64_t, int64_t, float, bool, double, std::string, BymlFile::Node>> ParamData;
				PivotDataStruct PivotData;
				uint64_t Refer;
				ReferPoseData ReferPose;
				std::string Type = "";
				std::map<std::string, std::string> UserData;
			};

			uint64_t ID = 0;
			std::vector<OwnerData> Owners;
			std::vector<ReferData> Refers;
		};

		struct RopeLinkData
		{
			uint64_t ID = 0;
			std::vector<uint64_t> Owners;
			std::vector<uint64_t> Refers;
		};

		struct RailData
		{
			struct Node
			{
				std::string Key;
				Vector3F Value;
			};

			bool IsClosed = false;
			std::vector<Node> Nodes;
			std::string Type = "";
		};

		std::map<std::string, std::string> Placement;
		ConstraintLinkData ConstraintLink;
		RopeLinkData RopeHeadLink;
		RopeLinkData RopeTailLink;
		std::vector<RailData> Rails;
	};

	struct DynamicData
	{
		std::variant<std::string, bool, int32_t, int64_t, uint32_t, uint64_t, float, Vector3F> Data;
		BymlFile::Type Type;
	};

	enum class WaterShapeType : uint32_t
	{
		Box = 0,
		Cylinder = 1
	};

	enum class WaterFluidType : uint32_t
	{
		Water = 0,
		Lava = 1,
		DriftSand = 2
	};

	Actor::Type ActorType = Actor::Type::Dynamic;

	std::string Gyml = "";
	uint64_t Hash = 0;
	uint32_t SRTHash = 0;

	Vector3F Translate = Vector3F(0, 0, 0);
	Vector3F Scale = Vector3F(1, 1, 1); //Default scale is 1, if it is zero, the model would be invisible, because all vertices would get multiplied by 0 (x * 0 = 0)
	Vector3F Rotate = Vector3F(0, 0, 0); //Converted from radians to degrees to make editing more comfortable

	std::string Name = "";

	/* Properties */
	bool Bakeable = false;
	bool PhysicsStable = false;
	bool ForceActive = false;
	float MoveRadius = 0.0f;
	float ExtraCreateRadius = 0.0f;
	bool TurnActorNearEnemy = false;
	bool InWater = false;
	int32_t Version = 0;

	std::vector<Actor::Link> Links;
	std::vector<Actor::Rail> Rails; //Not required
	std::map<std::string, std::string> Presence; //Not required

	Actor::PhiveData Phive;
	std::map<std::string, Actor::DynamicData> Dynamic; //Not required
	std::map<std::string, Actor::DynamicData> ExternalParameters; //Not required

	/* Required for the functionality of the editor */
	GLBfres* Model = nullptr;
	std::vector<Actor> MergedActorContent;
	BymlFile MergedActorByml;
	Actor* MergedActorParent = nullptr;

	/* UMii */
	bool IsUMii = false;
	UMii UMiiData;

	/* Water */
	bool mBakeFluid = false;
	WaterShapeType mWaterShapeType = WaterShapeType::Box;
	WaterFluidType mWaterFluidType = WaterFluidType::Water;
	float mWaterFlowSpeed = 0.0f;
	float mWaterBoxHeight = 5.0f;

	GameDataListMgr::GameData mGameData;

	bool operator==(Actor AnotherActor);
};