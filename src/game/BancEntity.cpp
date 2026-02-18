#include "BancEntity.h"

#include <util/Math.h>
#include <util/FileUtil.h>
#include <manager/ActorPackMgr.h>
#include <manager/BfresFileMgr.h>
#include <manager/BfresRendererMgr.h>
#include <manager/MergedActorMgr.h>
#include <game/actor_component/ActorComponentModelInfo.h>

namespace application::game
{
	std::vector<const char*> BancEntity::BakedFluid::gFluidShapeTypes = 
	{
		"Box",
		"Cylinder"
	};
	std::vector<const char*> BancEntity::BakedFluid::gFluidMaterialTypes =
	{
		"Water",
		"Lava",
		"Drift Sand",
		"Warm Water (No Damage)",
		"Ice Water (Immediate Damage)",
		"Abyss Water (Immediately Drowns)"
	};
	std::vector<std::pair<uint32_t, BancEntity::FluidMaterialType>> BancEntity::BakedFluid::gFluidMaterialTypeMap =
	{
		{38, BancEntity::FluidMaterialType::WATER},
		{41, BancEntity::FluidMaterialType::LAVA},
		{3, BancEntity::FluidMaterialType::DRIFT_SAND},
		{39, BancEntity::FluidMaterialType::WARM_WATER},
		{40, BancEntity::FluidMaterialType::ICE_WATER},
		{42, BancEntity::FluidMaterialType::ABYSS_WATER}
	};

	BancEntity::FluidMaterialType BancEntity::BakedFluid::ToMaterialType(uint32_t Type)
	{
		for (auto& [Id, Material] : BancEntity::BakedFluid::gFluidMaterialTypeMap)
		{
			if (Id == Type)
				return Material;
		}

		application::util::Logger::Warning("BancEntity", "Could not match type %u with internal fluid type", Type);
		return BancEntity::BakedFluid::gFluidMaterialTypeMap[0].second;
	}

	uint32_t BancEntity::BakedFluid::ToInternalIdentifier(BancEntity::FluidMaterialType Type)
	{
		for (auto& [Id, Material] : BancEntity::BakedFluid::gFluidMaterialTypeMap)
		{
			if (Material == Type)
				return Id;
		}

		return BancEntity::BakedFluid::gFluidMaterialTypeMap[0].first;
	}

	void BancEntity::ParseDynamicTypeParameter(application::file::game::byml::BymlFile::Node& Node, const std::string& Key, std::map<std::string, std::variant<std::string, bool, int32_t, int64_t, uint32_t, uint64_t, float, glm::vec3>>& Map)
	{
		if (Node.HasChild(Key))
		{
			for (application::file::game::byml::BymlFile::Node& DynamicNode : Node.GetChild(Key)->GetChildren())
			{
				std::variant<std::string, bool, int32_t, int64_t, uint32_t, uint64_t, float, glm::vec3> Data;

				switch (DynamicNode.GetType())
				{
				case application::file::game::byml::BymlFile::Type::StringIndex:
				{
					Data = DynamicNode.GetValue<std::string>();
					break;
				}
				case application::file::game::byml::BymlFile::Type::Bool:
				{
					Data = DynamicNode.GetValue<bool>();
					break;
				}
				case application::file::game::byml::BymlFile::Type::Int32:
				{
					Data = DynamicNode.GetValue<int32_t>();
					break;
				}
				case application::file::game::byml::BymlFile::Type::Int64:
				{
					Data = DynamicNode.GetValue<int64_t>();
					break;
				}
				case application::file::game::byml::BymlFile::Type::UInt32:
				{
					Data = DynamicNode.GetValue<uint32_t>();
					break;
				}
				case application::file::game::byml::BymlFile::Type::UInt64:
				{
					Data = DynamicNode.GetValue<uint64_t>();
					break;
				}
				case application::file::game::byml::BymlFile::Type::Float:
				{
					Data = DynamicNode.GetValue<float>();
					break;
				}
				case application::file::game::byml::BymlFile::Type::Array:
				{
					glm::vec3 Vec;
					Vec.x = DynamicNode.GetChild(0)->GetValue<float>();
					Vec.y = DynamicNode.GetChild(1)->GetValue<float>();
					Vec.z = DynamicNode.GetChild(2)->GetValue<float>();
					Data = Vec;
					break;
				}
				default:
				{
					application::util::Logger::Error("BancEntity", "Invalid dynamic data type");
					break;
				}
				}
				Map.insert({ DynamicNode.GetKey(), Data });
			}
		}
	}

	template <typename T>
	application::file::game::byml::BymlFile::Node GenerateBymlNode(application::file::game::byml::BymlFile::Type Type, const std::string& Key, const T& Value)
	{
		application::file::game::byml::BymlFile::Node Node(Type, Key);
		Node.SetValue<T>(Value);
		return Node;
	}

	void GenerateBymlVectorNode(application::file::game::byml::BymlFile::Node& Root, const std::string& Key, const glm::vec3& Vector, const glm::vec3& DefaultValues, bool SkipDefaultVectorCheck)
	{
		if (Vector.x != DefaultValues.x || Vector.y != DefaultValues.y || Vector.z != DefaultValues.z || SkipDefaultVectorCheck)
		{
			application::file::game::byml::BymlFile::Node VectorRoot(application::file::game::byml::BymlFile::Type::Array, Key);

			VectorRoot.AddChild(GenerateBymlNode(application::file::game::byml::BymlFile::Type::Float, "0", Vector.x));
			VectorRoot.AddChild(GenerateBymlNode(application::file::game::byml::BymlFile::Type::Float, "1", Vector.y));
			VectorRoot.AddChild(GenerateBymlNode(application::file::game::byml::BymlFile::Type::Float, "2", Vector.z));

			Root.AddChild(VectorRoot);
		}
	}

	void BancEntity::WriteDynamicTypeParameter(application::file::game::byml::BymlFile::Node& Node, const std::string& Key, std::map<std::string, std::variant<std::string, bool, int32_t, int64_t, uint32_t, uint64_t, float, glm::vec3>>& Map, bool SkipEmptyCheck)
	{
		if (Map.empty() && !SkipEmptyCheck)
			return;

		application::file::game::byml::BymlFile::Node Root(application::file::game::byml::BymlFile::Type::Dictionary, Key);

		for (auto const& [Key, Value] : Map)
		{
			application::file::game::byml::BymlFile::Type EntryType = application::file::game::byml::BymlFile::Type::Null;

			if(std::holds_alternative<std::string>(Value)) EntryType = application::file::game::byml::BymlFile::Type::StringIndex;
			if(std::holds_alternative<float>(Value)) EntryType = application::file::game::byml::BymlFile::Type::Float;
			if(std::holds_alternative<uint32_t>(Value)) EntryType = application::file::game::byml::BymlFile::Type::UInt32;
			if(std::holds_alternative<uint64_t>(Value)) EntryType = application::file::game::byml::BymlFile::Type::UInt64;
			if(std::holds_alternative<int32_t>(Value)) EntryType = application::file::game::byml::BymlFile::Type::Int32;
			if(std::holds_alternative<int64_t>(Value)) EntryType = application::file::game::byml::BymlFile::Type::Int64;
			if(std::holds_alternative<bool>(Value)) EntryType = application::file::game::byml::BymlFile::Type::Bool;
			if(std::holds_alternative<glm::vec3>(Value)) EntryType = application::file::game::byml::BymlFile::Type::Array;

			if (EntryType == application::file::game::byml::BymlFile::Type::Null)
			{
				application::util::Logger::Warning("BancEntity", "Invalid dynamic parameter type");
				continue;
			}

			application::file::game::byml::BymlFile::Node Entry(EntryType, Key);

			if (EntryType != application::file::game::byml::BymlFile::Type::Array)
			{
				switch (EntryType)
				{
				case application::file::game::byml::BymlFile::Type::StringIndex:
					Entry.SetValue<std::string>(std::get<std::string>(Value));
					break;
				case application::file::game::byml::BymlFile::Type::Float:
					Entry.SetValue<float>(std::get<float>(Value));
					break;
				case application::file::game::byml::BymlFile::Type::UInt32:
					Entry.SetValue<uint32_t>(std::get<uint32_t>(Value));
					break;
				case application::file::game::byml::BymlFile::Type::UInt64:
					Entry.SetValue<uint64_t>(std::get<uint64_t>(Value));
					break;
				case application::file::game::byml::BymlFile::Type::Int32:
					Entry.SetValue<int32_t>(std::get<int32_t>(Value));
					break;
				case application::file::game::byml::BymlFile::Type::Int64:
					Entry.SetValue<int64_t>(std::get<int64_t>(Value));
					break;
				case application::file::game::byml::BymlFile::Type::Bool:
					Entry.SetValue<bool>(std::get<bool>(Value));
					break;
				}
				Root.AddChild(Entry);
			}
			else
			{
				GenerateBymlVectorNode(Root, Key, std::get<glm::vec3>(Value), glm::vec3(0, 0, 0), true);
			}
		}

		Node.AddChild(Root);
	}

	bool BancEntity::FromByml(application::file::game::byml::BymlFile::Node& Node)
	{
		if (!Node.HasChild("Gyaml")) return false;

		mGyml = Node.GetChild("Gyaml")->GetValue<std::string>();

		if (Node.HasChild("Hash")) mHash = Node.GetChild("Hash")->GetValue<uint64_t>();
		if (Node.HasChild("SRTHash")) mSRTHash = Node.GetChild("SRTHash")->GetValue<uint32_t>();

		if (Node.HasChild("Translate")) mTranslate = Node.GetChild("Translate")->GetValue<glm::vec3>();
		if (Node.HasChild("Rotate")) mRotate = application::util::Math::RadiansToDegrees(Node.GetChild("Rotate")->GetValue<glm::vec3>());
		if (Node.HasChild("Scale")) mScale = Node.GetChild("Scale")->GetValue<glm::vec3>();

		if (Node.HasChild("Bakeable")) mBakeable = Node.GetChild("Bakeable")->GetValue<bool>();
		if (Node.HasChild("IsPhysicsStable")) mIsPhysicsStable = Node.GetChild("IsPhysicsStable")->GetValue<bool>();
		if (Node.HasChild("MoveRadius")) mMoveRadius = Node.GetChild("MoveRadius")->GetValue<float>();
		if (Node.HasChild("ExtraCreateRadius")) mExtraCreateRadius = Node.GetChild("ExtraCreateRadius")->GetValue<float>();
		if (Node.HasChild("IsForceActive")) mIsForceActive = Node.GetChild("IsForceActive")->GetValue<bool>();
		if (Node.HasChild("IsInWater")) mIsInWater = Node.GetChild("IsInWater")->GetValue<bool>();
		if (Node.HasChild("TurnActorNearEnemy")) mTurnActorNearEnemy = Node.GetChild("TurnActorNearEnemy")->GetValue<bool>();
		if (Node.HasChild("Name")) mName = Node.GetChild("Name")->GetValue<std::string>();
		if (Node.HasChild("Version")) mVersion = Node.GetChild("Version")->GetValue<int32_t>();

		if (Node.HasChild("Links"))
		{
			mLinks.reserve(Node.GetChild("Links")->GetChildren().size());

			for (application::file::game::byml::BymlFile::Node& LinksChild : Node.GetChild("Links")->GetChildren())
			{
				BancEntity::Link EntityLink;
				if (LinksChild.HasChild("Dst")) EntityLink.mDest = LinksChild.GetChild("Dst")->GetValue<uint64_t>();
				if (LinksChild.HasChild("Gyaml")) EntityLink.mGyml = LinksChild.GetChild("Gyaml")->GetValue<std::string>();
				if (LinksChild.HasChild("Name")) EntityLink.mName = LinksChild.GetChild("Name")->GetValue<std::string>();
				if (LinksChild.HasChild("Src")) EntityLink.mSrc = LinksChild.GetChild("Src")->GetValue<uint64_t>();

				mLinks.push_back(EntityLink);
			}
		}

		if (Node.HasChild("Rails"))
		{
			mRails.reserve(Node.GetChild("Rails")->GetChildren().size());

			for (application::file::game::byml::BymlFile::Node& RailsChild : Node.GetChild("Rails")->GetChildren())
			{
				BancEntity::Rail EntityRail;
				if (RailsChild.HasChild("Dst")) EntityRail.mDest = RailsChild.GetChild("Dst")->GetValue<uint64_t>();
				if (RailsChild.HasChild("Gyaml")) EntityRail.mGyml = RailsChild.GetChild("Gyaml")->GetValue<std::string>();
				if (RailsChild.HasChild("Name")) EntityRail.mName = RailsChild.GetChild("Name")->GetValue<std::string>();

				mRails.push_back(EntityRail);
			}
		}
		
		ParseDynamicTypeParameter(Node, "Dynamic", mDynamic);
		ParseDynamicTypeParameter(Node, "ExternalParameter", mExternalParameter);
		ParseDynamicTypeParameter(Node, "Presence", mPresence);
		
		if (Node.HasChild("Phive"))
		{
			application::file::game::byml::BymlFile::Node* PhiveNode = Node.GetChild("Phive");
			if (PhiveNode->HasChild("Rails"))
			{
				mPhive.mRails.reserve(PhiveNode->GetChild("Rails")->GetChildren().size());

				for (application::file::game::byml::BymlFile::Node& RailNode : PhiveNode->GetChild("Rails")->GetChildren())
				{
					Phive::Rail Rail;
					if (RailNode.HasChild("IsClosed")) Rail.mIsClosed = RailNode.GetChild("IsClosed")->GetValue<bool>();
					if (RailNode.HasChild("Type")) Rail.mType = RailNode.GetChild("Type")->GetValue<std::string>();

					if (RailNode.HasChild("Nodes"))
					{
						for (application::file::game::byml::BymlFile::Node& BymlNode : RailNode.GetChild("Nodes")->GetChildren())
						{
							for (application::file::game::byml::BymlFile::Node& SubNode : BymlNode.GetChildren())
							{
								Phive::Rail::Node PhiveRailNode;
								PhiveRailNode.mKey = SubNode.GetKey();

								PhiveRailNode.mValue.x = SubNode.GetChild(0)->GetValue<float>();
								PhiveRailNode.mValue.y = SubNode.GetChild(1)->GetValue<float>();
								PhiveRailNode.mValue.z = SubNode.GetChild(2)->GetValue<float>();

								Rail.mNodes.push_back(PhiveRailNode);
							}
						}
					}

					mPhive.mRails.push_back(Rail);
				}
			}

			if (PhiveNode->HasChild("RopeHeadLink"))
			{
				Phive::RopeLink RopeLink;
				if (PhiveNode->GetChild("RopeHeadLink")->HasChild("ID")) RopeLink.mID = PhiveNode->GetChild("RopeHeadLink")->GetChild("ID")->GetValue<uint64_t>();
				if (PhiveNode->GetChild("RopeHeadLink")->HasChild("Owners"))
				{
					for (application::file::game::byml::BymlFile::Node& OwnerNode : PhiveNode->GetChild("RopeHeadLink")->GetChild("Owners")->GetChildren())
					{
						RopeLink.mOwners.push_back(OwnerNode.GetChild("Refer")->GetValue<uint64_t>());
					}
				}
				if (PhiveNode->GetChild("RopeHeadLink")->HasChild("Refers"))
				{
					for (application::file::game::byml::BymlFile::Node& ReferNode : PhiveNode->GetChild("RopeHeadLink")->GetChild("Refers")->GetChildren())
					{
						RopeLink.mRefers.push_back(ReferNode.GetChild("Owner")->GetValue<uint64_t>());
					}
				}
				mPhive.mRopeHeadLink = RopeLink;
			}

			if (PhiveNode->HasChild("RopeTailLink"))
			{
				Phive::RopeLink RopeLink;
				if (PhiveNode->GetChild("RopeTailLink")->HasChild("ID")) RopeLink.mID = PhiveNode->GetChild("RopeTailLink")->GetChild("ID")->GetValue<uint64_t>();
				if (PhiveNode->GetChild("RopeTailLink")->HasChild("Owners"))
				{
					for (application::file::game::byml::BymlFile::Node& OwnerNode : PhiveNode->GetChild("RopeTailLink")->GetChild("Owners")->GetChildren())
					{
						RopeLink.mOwners.push_back(OwnerNode.GetChild("Refer")->GetValue<uint64_t>());
					}
				}
				if (PhiveNode->GetChild("RopeTailLink")->HasChild("Refers"))
				{
					for (application::file::game::byml::BymlFile::Node& ReferNode : PhiveNode->GetChild("RopeTailLink")->GetChild("Refers")->GetChildren())
					{
						RopeLink.mRefers.push_back(ReferNode.GetChild("Owner")->GetValue<uint64_t>());
					}
				}
				mPhive.mRopeTailLink = RopeLink;
			}

			ParseDynamicTypeParameter(*PhiveNode, "Placement", mPhive.mPlacement);
			
			if (PhiveNode->HasChild("ConstraintLink"))
			{
				application::file::game::byml::BymlFile::Node* ConstraintLinkNode = PhiveNode->GetChild("ConstraintLink");
				Phive::ConstraintLink ConstraintLink;

				if (ConstraintLinkNode->HasChild("ID")) ConstraintLink.mID = ConstraintLinkNode->GetChild("ID")->GetValue<uint64_t>();

				if (ConstraintLinkNode->HasChild("Refers"))
				{
					for (application::file::game::byml::BymlFile::Node& ReferNode : ConstraintLinkNode->GetChild("Refers")->GetChildren())
					{
						Phive::ConstraintLink::Refer Refer;
						if (ReferNode.HasChild("Owner")) Refer.mOwner = ReferNode.GetChild("Owner")->GetValue<uint64_t>();
						if (ReferNode.HasChild("Type")) Refer.mType = ReferNode.GetChild("Type")->GetValue<std::string>();
						ConstraintLink.mRefers.push_back(Refer);
					}
				}
				if (ConstraintLinkNode->HasChild("Owners"))
				{
					for (application::file::game::byml::BymlFile::Node& OwnersNode : ConstraintLinkNode->GetChild("Owners")->GetChildren())
					{
						Phive::ConstraintLink::Owner Owner;
						ParseDynamicTypeParameter(OwnersNode, "AliasData", Owner.mAliasData);
						ParseDynamicTypeParameter(OwnersNode, "BreakableData", Owner.mBreakableData);
						ParseDynamicTypeParameter(OwnersNode, "ClusterData", Owner.mClusterData);
						ParseDynamicTypeParameter(OwnersNode, "UserData", Owner.mUserData);

						if (OwnersNode.HasChild("OwnerPose"))
						{
							if (OwnersNode.GetChild("OwnerPose")->HasChild("Rotate"))
							{
								Owner.mOwnerPose.mRotate.x = OwnersNode.GetChild("OwnerPose")->GetChild("Rotate")->GetChild(0)->GetValue<float>();
								Owner.mOwnerPose.mRotate.y = OwnersNode.GetChild("OwnerPose")->GetChild("Rotate")->GetChild(1)->GetValue<float>();
								Owner.mOwnerPose.mRotate.z = OwnersNode.GetChild("OwnerPose")->GetChild("Rotate")->GetChild(2)->GetValue<float>();
								Owner.mOwnerPose.mRotate = application::util::Math::RadiansToDegrees(Owner.mOwnerPose.mRotate);
							}
							if (OwnersNode.GetChild("OwnerPose")->HasChild("Trans"))
							{
								Owner.mOwnerPose.mTranslate.x = OwnersNode.GetChild("OwnerPose")->GetChild("Trans")->GetChild(0)->GetValue<float>();
								Owner.mOwnerPose.mTranslate.y = OwnersNode.GetChild("OwnerPose")->GetChild("Trans")->GetChild(1)->GetValue<float>();
								Owner.mOwnerPose.mTranslate.z = OwnersNode.GetChild("OwnerPose")->GetChild("Trans")->GetChild(2)->GetValue<float>();
							}
						}
						ParseDynamicTypeParameter(OwnersNode, "ParamData", Owner.mParamData);

						if (OwnersNode.HasChild("PivotData"))
						{
							application::file::game::byml::BymlFile::Node* PivotDataNode = OwnersNode.GetChild("PivotData");
							if (PivotDataNode->HasChild("Axis")) Owner.mPivotData.mAxis = PivotDataNode->GetChild("Axis")->GetValue<int32_t>();
							if (PivotDataNode->HasChild("AxisA")) Owner.mPivotData.mAxisA = PivotDataNode->GetChild("AxisA")->GetValue<int32_t>();
							if (PivotDataNode->HasChild("AxisB")) Owner.mPivotData.mAxisB = PivotDataNode->GetChild("AxisB")->GetValue<int32_t>();

							if (PivotDataNode->HasChild("Pivot"))
							{
								glm::vec3 Vec;
								Vec.x = PivotDataNode->GetChild("Pivot")->GetChild(0)->GetValue<float>();
								Vec.y = PivotDataNode->GetChild("Pivot")->GetChild(1)->GetValue<float>();
								Vec.z = PivotDataNode->GetChild("Pivot")->GetChild(2)->GetValue<float>();
								Owner.mPivotData.mPivot = Vec;
							}
							if (PivotDataNode->HasChild("PivotA"))
							{
								glm::vec3 Vec;
								Vec.x = PivotDataNode->GetChild("PivotA")->GetChild(0)->GetValue<float>();
								Vec.y = PivotDataNode->GetChild("PivotA")->GetChild(1)->GetValue<float>();
								Vec.z = PivotDataNode->GetChild("PivotA")->GetChild(2)->GetValue<float>();
								Owner.mPivotData.mPivotA = Vec;
							}
							if (PivotDataNode->HasChild("PivotB"))
							{
								glm::vec3 Vec;
								Vec.x = PivotDataNode->GetChild("PivotB")->GetChild(0)->GetValue<float>();
								Vec.y = PivotDataNode->GetChild("PivotB")->GetChild(1)->GetValue<float>();
								Vec.z = PivotDataNode->GetChild("PivotB")->GetChild(2)->GetValue<float>();
								Owner.mPivotData.mPivotB = Vec;
							}
						}
						if (OwnersNode.HasChild("Refer")) Owner.mRefer = OwnersNode.GetChild("Refer")->GetValue<uint64_t>();
						if (OwnersNode.HasChild("ReferPose"))
						{
							if (OwnersNode.GetChild("ReferPose")->HasChild("Rotate"))
							{
								Owner.mReferPose.mRotate.x = OwnersNode.GetChild("ReferPose")->GetChild("Rotate")->GetChild(0)->GetValue<float>();
								Owner.mReferPose.mRotate.y = OwnersNode.GetChild("ReferPose")->GetChild("Rotate")->GetChild(1)->GetValue<float>();
								Owner.mReferPose.mRotate.z = OwnersNode.GetChild("ReferPose")->GetChild("Rotate")->GetChild(2)->GetValue<float>();
								Owner.mReferPose.mRotate = application::util::Math::RadiansToDegrees(Owner.mReferPose.mRotate);
							}
							if (OwnersNode.GetChild("ReferPose")->HasChild("Trans"))
							{
								Owner.mReferPose.mTranslate.x = OwnersNode.GetChild("ReferPose")->GetChild("Trans")->GetChild(0)->GetValue<float>();
								Owner.mReferPose.mTranslate.y = OwnersNode.GetChild("ReferPose")->GetChild("Trans")->GetChild(1)->GetValue<float>();
								Owner.mReferPose.mTranslate.z = OwnersNode.GetChild("ReferPose")->GetChild("Trans")->GetChild(2)->GetValue<float>();
							}
						}
						if (OwnersNode.HasChild("Type")) Owner.mType = OwnersNode.GetChild("Type")->GetValue<std::string>();
						ConstraintLink.mOwners.push_back(Owner);
					}
				}

				mPhive.mConstraintLink = ConstraintLink;
			}
		}

		if (mDynamic.contains("BancPath"))
		{
			mMergedActorChildren = application::manager::MergedActorMgr::GetMergedActor(std::get<std::string>(mDynamic["BancPath"]));
		}

		return GenerateBancEntityInfo();
	}

	bool BancEntity::FromGyml(const std::string& Gyml)
	{
		if (!application::util::FileUtil::FileExists(application::util::FileUtil::GetRomFSFilePath("Pack/Actor/" + Gyml + ".pack.zs")))
			return false;

		mGyml = Gyml;

		return GenerateBancEntityInfo();
	}

	bool BancEntity::GenerateBancEntityInfo()
	{
		mActorPack = application::manager::ActorPackMgr::GetActorPack(mGyml);

		if (mActorPack == nullptr)
			return false;


		if (application::game::actor_component::ActorComponentBase* BaseComponent = mActorPack->GetComponent(application::game::actor_component::ActorComponentBase::ComponentType::MODEL_INFO); BaseComponent != nullptr)
		{
			application::game::actor_component::ActorComponentModelInfo* ModelInfoComponent = static_cast<application::game::actor_component::ActorComponentModelInfo*>(BaseComponent);
			if (ModelInfoComponent)
			{
				if (ModelInfoComponent->mModelProjectName.has_value() && ModelInfoComponent->mFmdbName.has_value() && !ModelInfoComponent->mModelProjectName.value().empty() && !ModelInfoComponent->mFmdbName.value().empty())
				{
					application::file::game::bfres::BfresFile* File = application::manager::BfresFileMgr::GetBfresFile(application::util::FileUtil::GetBfresFilePath(ModelInfoComponent->mModelProjectName.value() + "." + ModelInfoComponent->mFmdbName.value() + ".bfres"));
					mBfresRenderer = application::manager::BfresRendererMgr::GetRenderer(File);
					return mBfresRenderer != nullptr;
				}
			}
		}

		application::file::game::bfres::BfresFile* File = &application::manager::BfresFileMgr::gBfresFiles["Default"];
		for (auto& [Key, Val] : application::manager::BfresFileMgr::gBfresFiles)
		{
			if (mGyml.find(Key) != std::string::npos)
			{
				File = &Val;
				break;
			}
		}

		mBfresRenderer = application::manager::BfresRendererMgr::GetRenderer(File);
		return mBfresRenderer != nullptr;
	}

	application::file::game::byml::BymlFile::Node BancEntity::ToByml()
	{
		application::file::game::byml::BymlFile::Node Node(application::file::game::byml::BymlFile::Type::Dictionary);

		if (mBakeable) Node.AddChild(GenerateBymlNode(application::file::game::byml::BymlFile::Type::Bool, "Bakeable", mBakeable));
		WriteDynamicTypeParameter(Node, "Dynamic", mDynamic);
		WriteDynamicTypeParameter(Node, "ExternalParameter", mExternalParameter);
		if (mExtraCreateRadius != 0.0f) Node.AddChild(GenerateBymlNode(application::file::game::byml::BymlFile::Type::Float, "ExtraCreateRadius", mExtraCreateRadius));
		Node.AddChild(GenerateBymlNode(application::file::game::byml::BymlFile::Type::StringIndex, "Gyaml", mGyml));
		Node.AddChild(GenerateBymlNode(application::file::game::byml::BymlFile::Type::UInt64, "Hash", mHash));
		if (mIsForceActive) Node.AddChild(GenerateBymlNode(application::file::game::byml::BymlFile::Type::Bool, "IsForceActive", mIsForceActive));
		if (mIsInWater) Node.AddChild(GenerateBymlNode(application::file::game::byml::BymlFile::Type::Bool, "IsInWater", mIsInWater));
		if (mIsPhysicsStable) Node.AddChild(GenerateBymlNode(application::file::game::byml::BymlFile::Type::Bool, "IsPhysicsStable", mIsPhysicsStable));

		if (!mLinks.empty())
		{
			application::file::game::byml::BymlFile::Node LinksNode(application::file::game::byml::BymlFile::Type::Array, "Links");

			for (Link& Link : mLinks)
			{
				application::file::game::byml::BymlFile::Node LinkNodeRoot(application::file::game::byml::BymlFile::Type::Dictionary);

				LinkNodeRoot.AddChild(GenerateBymlNode(application::file::game::byml::BymlFile::Type::UInt64, "Dst", Link.mDest));
				if (!Link.mGyml.empty()) LinkNodeRoot.AddChild(GenerateBymlNode(application::file::game::byml::BymlFile::Type::StringIndex, "Gyaml", Link.mGyml));
				LinkNodeRoot.AddChild(GenerateBymlNode(application::file::game::byml::BymlFile::Type::StringIndex, "Name", Link.mName));
				LinkNodeRoot.AddChild(GenerateBymlNode(application::file::game::byml::BymlFile::Type::UInt64, "Src", Link.mSrc));

				LinksNode.AddChild(LinkNodeRoot);
			}

			Node.AddChild(LinksNode);
		}

		if (mMoveRadius != 0.0f) Node.AddChild(GenerateBymlNode(application::file::game::byml::BymlFile::Type::Float, "MoveRadius", mMoveRadius));
		if (!mName.empty()) Node.AddChild(GenerateBymlNode(application::file::game::byml::BymlFile::Type::StringIndex, "Name", mName));
		
		//Phive - Begin

		application::file::game::byml::BymlFile::Node PhiveRoot(application::file::game::byml::BymlFile::Type::Dictionary, "Phive");

		if (mPhive.mConstraintLink.has_value())
		{
			Phive::ConstraintLink& ConstraintLink = mPhive.mConstraintLink.value();
			application::file::game::byml::BymlFile::Node ConstraintLinkNode(application::file::game::byml::BymlFile::Type::Dictionary, "ConstraintLink");
			
			ConstraintLinkNode.AddChild(GenerateBymlNode(application::file::game::byml::BymlFile::Type::UInt64, "ID", ConstraintLink.mID));
			
			if (!ConstraintLink.mOwners.empty())
			{
				application::file::game::byml::BymlFile::Node ConstraintLinkOwnersNode(application::file::game::byml::BymlFile::Type::Array, "Owners");
				
				for (Phive::ConstraintLink::Owner& Owner : ConstraintLink.mOwners)
				{
					application::file::game::byml::BymlFile::Node OwnerNode(application::file::game::byml::BymlFile::Type::Dictionary);

					WriteDynamicTypeParameter(OwnerNode, "AliasData", Owner.mAliasData);
					WriteDynamicTypeParameter(OwnerNode, "BreakableData", Owner.mBreakableData);
					WriteDynamicTypeParameter(OwnerNode, "ClusterData", Owner.mClusterData);
					
					if (Owner.mOwnerPose.mRotate != glm::vec3(0.0f, 0.0f, 0.0f) ||
						Owner.mOwnerPose.mTranslate != glm::vec3(0.0f, 0.0f, 0.0f))
					{
						application::file::game::byml::BymlFile::Node OwnerPoseNode(application::file::game::byml::BymlFile::Type::Dictionary, "OwnerPose");

						GenerateBymlVectorNode(OwnerPoseNode, "Rotate", glm::radians(Owner.mOwnerPose.mRotate), glm::vec3(0.0f, 0.0f, 0.0f), true);
						GenerateBymlVectorNode(OwnerPoseNode, "Trans", Owner.mOwnerPose.mTranslate, glm::vec3(0.0f, 0.0f, 0.0f), true);

						OwnerNode.AddChild(OwnerPoseNode);
					}

					WriteDynamicTypeParameter(OwnerNode, "ParamData", Owner.mParamData);

					application::file::game::byml::BymlFile::Node PivotDataNode(application::file::game::byml::BymlFile::Type::Dictionary, "PivotData");
					//Axis
					if (Owner.mPivotData.mAxis.has_value())
					{
						PivotDataNode.AddChild(GenerateBymlNode(application::file::game::byml::BymlFile::Type::Int32, "Axis", Owner.mPivotData.mAxis.value()));
					}
					if (Owner.mPivotData.mAxisA.has_value())
					{
						PivotDataNode.AddChild(GenerateBymlNode(application::file::game::byml::BymlFile::Type::Int32, "AxisA", Owner.mPivotData.mAxisA.value()));
					}
					if (Owner.mPivotData.mAxisB.has_value())
					{
						PivotDataNode.AddChild(GenerateBymlNode(application::file::game::byml::BymlFile::Type::Int32, "AxisB", Owner.mPivotData.mAxisB.value()));
					}
					//Vectors
					if (Owner.mPivotData.mPivot.has_value())
					{
						GenerateBymlVectorNode(PivotDataNode, "Pivot", Owner.mPivotData.mPivot.value(), glm::vec3(0.0f, 0.0f, 0.0f), true);
					}
					if (Owner.mPivotData.mPivotA.has_value())
					{
						GenerateBymlVectorNode(PivotDataNode, "PivotA", Owner.mPivotData.mPivotA.value(), glm::vec3(0.0f, 0.0f, 0.0f), true);
					}
					if (Owner.mPivotData.mPivotB.has_value())
					{
						GenerateBymlVectorNode(PivotDataNode, "PivotB", Owner.mPivotData.mPivotB.value(), glm::vec3(0.0f, 0.0f, 0.0f), true);
					}
					OwnerNode.AddChild(PivotDataNode);

					OwnerNode.AddChild(GenerateBymlNode(application::file::game::byml::BymlFile::Type::UInt64, "Refer", Owner.mRefer));

					if (Owner.mReferPose.mRotate != glm::vec3(0.0f, 0.0f, 0.0f) ||
						Owner.mReferPose.mTranslate != glm::vec3(0.0f, 0.0f, 0.0f))
					{
						application::file::game::byml::BymlFile::Node ReferPoseNode(application::file::game::byml::BymlFile::Type::Dictionary, "ReferPose");

						GenerateBymlVectorNode(ReferPoseNode, "Rotate", glm::radians(Owner.mReferPose.mRotate), glm::vec3(0.0f, 0.0f, 0.0f), true);
						GenerateBymlVectorNode(ReferPoseNode, "Trans", Owner.mReferPose.mTranslate, glm::vec3(0.0f, 0.0f, 0.0f), true);

						OwnerNode.AddChild(ReferPoseNode);
					}

					if(!Owner.mType.empty()) OwnerNode.AddChild(GenerateBymlNode(application::file::game::byml::BymlFile::Type::StringIndex, "Type", Owner.mType));

					WriteDynamicTypeParameter(OwnerNode, "UserData", Owner.mUserData);

					ConstraintLinkOwnersNode.AddChild(OwnerNode);
				}

				ConstraintLinkNode.AddChild(ConstraintLinkOwnersNode);
			}

			if (!ConstraintLink.mRefers.empty())
			{
				application::file::game::byml::BymlFile::Node ConstraintLinkRefersNode(application::file::game::byml::BymlFile::Type::Array, "Refers");

				for (Phive::ConstraintLink::Refer& Refer : ConstraintLink.mRefers)
				{
					application::file::game::byml::BymlFile::Node ReferNode(application::file::game::byml::BymlFile::Type::Dictionary);

					ReferNode.AddChild(GenerateBymlNode(application::file::game::byml::BymlFile::Type::UInt64, "Owner", Refer.mOwner));
					ReferNode.AddChild(GenerateBymlNode(application::file::game::byml::BymlFile::Type::StringIndex, "Type", Refer.mType));

					ConstraintLinkRefersNode.AddChild(ReferNode);
				}
				ConstraintLinkNode.AddChild(ConstraintLinkRefersNode);
			}

			PhiveRoot.AddChild(ConstraintLinkNode);
		}

		WriteDynamicTypeParameter(PhiveRoot, "Placement", mPhive.mPlacement);

		if (!mPhive.mRails.empty())
		{
			application::file::game::byml::BymlFile::Node Rails(application::file::game::byml::BymlFile::Type::Array, "Rails");

			for (Phive::Rail& Rail : mPhive.mRails)
			{
				application::file::game::byml::BymlFile::Node RailNode(application::file::game::byml::BymlFile::Type::Dictionary);

				RailNode.AddChild(GenerateBymlNode(application::file::game::byml::BymlFile::Type::Bool, "IsClosed", Rail.mIsClosed));

				if (!Rail.mNodes.empty())
				{
					application::file::game::byml::BymlFile::Node Nodes(application::file::game::byml::BymlFile::Type::Array, "Nodes");

					for (Phive::Rail::Node& RailNode : Rail.mNodes)
					{
						application::file::game::byml::BymlFile::Node RailDict(application::file::game::byml::BymlFile::Type::Dictionary);
						GenerateBymlVectorNode(RailDict, RailNode.mKey, RailNode.mValue, glm::vec3(0.0f, 0.0f, 0.0f), true);
						Nodes.AddChild(RailDict);
					}

					RailNode.AddChild(Nodes);
				}

				if (!Rail.mType.empty())
				{
					RailNode.AddChild(GenerateBymlNode(application::file::game::byml::BymlFile::Type::StringIndex, "Type", Rail.mType));
				}

				Rails.AddChild(RailNode);
			}

			PhiveRoot.AddChild(Rails);
		}

		if (mPhive.mRopeHeadLink.has_value())
		{
			Phive::RopeLink& RopeHeadLink = mPhive.mRopeHeadLink.value();

			application::file::game::byml::BymlFile::Node RopeHeadLinkNode(application::file::game::byml::BymlFile::Type::Dictionary, "RopeHeadLink");

			RopeHeadLinkNode.AddChild(GenerateBymlNode(application::file::game::byml::BymlFile::Type::UInt64, "ID", RopeHeadLink.mID));

			if (!RopeHeadLink.mOwners.empty())
			{
				application::file::game::byml::BymlFile::Node Owners(application::file::game::byml::BymlFile::Type::Array, "Owners");

				for (uint64_t Owner : RopeHeadLink.mOwners)
				{
					application::file::game::byml::BymlFile::Node OwnerNode(application::file::game::byml::BymlFile::Type::Dictionary);

					OwnerNode.AddChild(GenerateBymlNode(application::file::game::byml::BymlFile::Type::UInt64, "Refer", Owner));

					Owners.AddChild(OwnerNode);
				}
				RopeHeadLinkNode.AddChild(Owners);
			}

			if (!RopeHeadLink.mRefers.empty())
			{
				application::file::game::byml::BymlFile::Node Refers(application::file::game::byml::BymlFile::Type::Array, "Refers");

				for (uint64_t Refer : RopeHeadLink.mRefers)
				{
					application::file::game::byml::BymlFile::Node ReferNode(application::file::game::byml::BymlFile::Type::Dictionary);

					ReferNode.AddChild(GenerateBymlNode(application::file::game::byml::BymlFile::Type::UInt64, "Owner", Refer));

					Refers.AddChild(ReferNode);
				}
				RopeHeadLinkNode.AddChild(Refers);
			}

			PhiveRoot.AddChild(RopeHeadLinkNode);
		}

		if (mPhive.mRopeTailLink.has_value())
		{
			Phive::RopeLink& RopeHeadLink = mPhive.mRopeTailLink.value();

			application::file::game::byml::BymlFile::Node RopeHeadLinkNode(application::file::game::byml::BymlFile::Type::Dictionary, "RopeTailLink");

			RopeHeadLinkNode.AddChild(GenerateBymlNode(application::file::game::byml::BymlFile::Type::UInt64, "ID", RopeHeadLink.mID));

			if (!RopeHeadLink.mOwners.empty())
			{
				application::file::game::byml::BymlFile::Node Owners(application::file::game::byml::BymlFile::Type::Array, "Owners");

				for (uint64_t Owner : RopeHeadLink.mOwners)
				{
					application::file::game::byml::BymlFile::Node OwnerNode(application::file::game::byml::BymlFile::Type::Dictionary);

					OwnerNode.AddChild(GenerateBymlNode(application::file::game::byml::BymlFile::Type::UInt64, "Refer", Owner));

					Owners.AddChild(OwnerNode);
				}
				RopeHeadLinkNode.AddChild(Owners);
			}

			if (!RopeHeadLink.mRefers.empty())
			{
				application::file::game::byml::BymlFile::Node Refers(application::file::game::byml::BymlFile::Type::Array, "Refers");

				for (uint64_t Refer : RopeHeadLink.mRefers)
				{
					application::file::game::byml::BymlFile::Node ReferNode(application::file::game::byml::BymlFile::Type::Dictionary);

					ReferNode.AddChild(GenerateBymlNode(application::file::game::byml::BymlFile::Type::UInt64, "Owner", Refer));

					Refers.AddChild(ReferNode);
				}
				RopeHeadLinkNode.AddChild(Refers);
			}

			PhiveRoot.AddChild(RopeHeadLinkNode);
		}

		if (!PhiveRoot.GetChildren().empty())
		{
			Node.AddChild(PhiveRoot);
		}
		//Phive - End
		
		WriteDynamicTypeParameter(Node, "Presence", mPresence);
		
		if (!mRails.empty())
		{
			application::file::game::byml::BymlFile::Node RailsNode(application::file::game::byml::BymlFile::Type::Array, "Rails");

			for (Rail& Rail : mRails)
			{
				application::file::game::byml::BymlFile::Node RailNodeRoot(application::file::game::byml::BymlFile::Type::Dictionary);

				RailNodeRoot.AddChild(GenerateBymlNode(application::file::game::byml::BymlFile::Type::UInt64, "Dst", Rail.mDest));
				if (!Rail.mGyml.empty()) RailNodeRoot.AddChild(GenerateBymlNode(application::file::game::byml::BymlFile::Type::StringIndex, "Gyaml", Rail.mGyml));
				RailNodeRoot.AddChild(GenerateBymlNode(application::file::game::byml::BymlFile::Type::StringIndex, "Name", Rail.mName));

				RailsNode.AddChild(RailNodeRoot);
			}

			Node.AddChild(RailsNode);
		}

		GenerateBymlVectorNode(Node, "Rotate", glm::radians(mRotate), glm::vec3(0.0f, 0.0f, 0.0f), false);
		Node.AddChild(GenerateBymlNode(application::file::game::byml::BymlFile::Type::UInt32, "SRTHash", mSRTHash));
		GenerateBymlVectorNode(Node, "Scale", mScale, glm::vec3(1.0f, 1.0f, 1.0f), false);
		GenerateBymlVectorNode(Node, "Translate", mTranslate, glm::vec3(0.0f, 0.0f, 0.0f), false);

		if(mTurnActorNearEnemy) Node.AddChild(GenerateBymlNode(application::file::game::byml::BymlFile::Type::Bool, "TurnActorNearEnemy", mTurnActorNearEnemy));
		if(mVersion != -1) Node.AddChild(GenerateBymlNode(application::file::game::byml::BymlFile::Type::Int32, "Version", mVersion));

		return Node;
	}
}