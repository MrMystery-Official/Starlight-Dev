#pragma once

#include "Vector3F.h"
#include "Bfres.h"
#include "Byml.h"
#include "UMii.h"

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
					int32_t Axis = 0;
					Vector3F Pivot;
				};

				struct ReferPoseData
				{
					Vector3F Rotate;
					Vector3F Translate;
				};

				std::map<std::string, std::string> BreakableData;
				std::map<std::string, std::string> ClusterData;
				OwnerPoseData OwnerPose;
				std::map<std::string, std::string> ParamData;
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
		std::map<std::string, std::string> DynamicString;
		std::map<std::string, Vector3F> DynamicVector;
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

	std::vector<Actor::Link> Links;
	std::vector<Actor::Rail> Rails; //Not required
	std::map<std::string, std::string> Presence; //Not required

	Actor::PhiveData Phive;
	Actor::DynamicData Dynamic; //Not required

	/* Required for the functionality of the editor */
	BfresFile* Model = nullptr;
	std::vector<Actor> MergedActorContent;
	BymlFile MergedActorByml;
	Actor* MergedActorParent = nullptr;

	/* UMii */
	bool IsUMii = false;
	UMii UMiiData;

	bool operator==(Actor AnotherActor);
};