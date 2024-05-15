#include "ActorMgr.h"

#include <unordered_map>
#include "Bfres.h"
#include "SARC.h"
#include "ZStdFile.h"
#include "Editor.h"
#include "Logger.h"
#include "Util.h"
#include "HashMgr.h"
#include "UMii.h"
#define _USE_MATH_DEFINES
#include <math.h>

#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>

std::vector<Actor> ActorMgr::Actors;
std::unordered_map<BfresFile*, std::vector<Actor*>> ActorMgr::OpaqueActors;
std::unordered_map<BfresFile*, std::vector<Actor*>> ActorMgr::TransparentActors;
std::unordered_map<std::string, ActorMgr::ActorInformation> ActorMgr::ActorInfo; //Gyml -> ActorInfo
std::vector<Actor*> ActorMgr::UMiiActors;

Actor* ActorMgr::AddActorFromByml(BymlFile::Node& Node, Actor* ModifyActor) //nullptr is data is corrupted
{
	if (!Node.HasChild("Gyaml")) return nullptr;

	Actor& BymlActor = ModifyActor == nullptr ? AddActor(Node.GetChild("Gyaml")->GetValue<std::string>(), false) : *ModifyActor;

	if (Node.HasChild("Hash")) BymlActor.Hash = Node.GetChild("Hash")->GetValue<uint64_t>();
	if (Node.HasChild("SRTHash")) BymlActor.SRTHash = Node.GetChild("SRTHash")->GetValue<uint32_t>();

	if (Node.HasChild("Translate")) BymlActor.Translate = Node.GetChild("Translate")->GetValue<Vector3F>();
	if (Node.HasChild("Rotate"))
	{
		BymlActor.Rotate = Node.GetChild("Rotate")->GetValue<Vector3F>();
		BymlActor.Rotate.SetX(Util::RadiansToDegrees(BymlActor.Rotate.GetX()));
		BymlActor.Rotate.SetY(Util::RadiansToDegrees(BymlActor.Rotate.GetY()));
		BymlActor.Rotate.SetZ(Util::RadiansToDegrees(BymlActor.Rotate.GetZ()));
	}
	if (Node.HasChild("Scale")) BymlActor.Scale = Node.GetChild("Scale")->GetValue<Vector3F>();

	if (Node.HasChild("Bakeable")) BymlActor.Bakeable = Node.GetChild("Bakeable")->GetValue<bool>();
	if (Node.HasChild("IsPhysicsStable")) BymlActor.PhysicsStable = Node.GetChild("IsPhysicsStable")->GetValue<bool>();
	if (Node.HasChild("MoveRadius")) BymlActor.MoveRadius = Node.GetChild("MoveRadius")->GetValue<float>();
	if (Node.HasChild("ExtraCreateRadius")) BymlActor.ExtraCreateRadius = Node.GetChild("ExtraCreateRadius")->GetValue<float>();
	if (Node.HasChild("IsForceActive")) BymlActor.ForceActive = Node.GetChild("IsForceActive")->GetValue<bool>();
	if (Node.HasChild("IsInWater")) BymlActor.InWater = Node.GetChild("IsInWater")->GetValue<bool>();
	if (Node.HasChild("TurnActorNearEnemy")) BymlActor.TurnActorNearEnemy = Node.GetChild("TurnActorNearEnemy")->GetValue<bool>();
	if (Node.HasChild("Name")) BymlActor.Name = Node.GetChild("Name")->GetValue<std::string>();

	if (Node.HasChild("Links"))
	{
		for (BymlFile::Node& LinksChild : Node.GetChild("Links")->GetChildren())
		{
			Actor::Link Link;
			for (BymlFile::Node& LinkNode : LinksChild.GetChildren())
			{
				if (LinkNode.GetKey() == "Dst") Link.Dest = LinkNode.GetValue<uint64_t>();
				if (LinkNode.GetKey() == "Gyaml") Link.Gyml = LinkNode.GetValue<std::string>();
				if (LinkNode.GetKey() == "Name") Link.Name = LinkNode.GetValue<std::string>();
				if (LinkNode.GetKey() == "Src") Link.Src = LinkNode.GetValue<uint64_t>();
			}
			BymlActor.Links.push_back(Link);
		}
	}

	if (Node.HasChild("Rails"))
	{
		for (BymlFile::Node& RailsChild : Node.GetChild("Rails")->GetChildren())
		{
			Actor::Rail Rail;
			for (BymlFile::Node& RailNode : RailsChild.GetChildren())
			{
				if (RailNode.GetKey() == "Dst") Rail.Dest = RailNode.GetValue<uint64_t>();
				if (RailNode.GetKey() == "Gyaml") Rail.Gyml = RailNode.GetValue<std::string>();
				if (RailNode.GetKey() == "Name") Rail.Name = RailNode.GetValue<std::string>();
			}
			BymlActor.Rails.push_back(Rail);
		}
	}

	if (Node.HasChild("Dynamic"))
	{
		for (BymlFile::Node& DynamicNode : Node.GetChild("Dynamic")->GetChildren())
		{
			if (DynamicNode.GetType() != BymlFile::Type::Array)
			{
				BymlActor.Dynamic.DynamicString.insert({ DynamicNode.GetKey(), DynamicNode.GetValue<std::string>() });
			}
			else
			{
				Vector3F VectorValue;
				VectorValue.SetX(DynamicNode.GetChild(0)->GetValue<float>());
				VectorValue.SetY(DynamicNode.GetChild(1)->GetValue<float>());
				VectorValue.SetZ(DynamicNode.GetChild(2)->GetValue<float>());
				BymlActor.Dynamic.DynamicVector.insert({ DynamicNode.GetKey(), VectorValue });
			}
		}
	}

	if (Node.HasChild("Presence"))
	{
		for (BymlFile::Node& PresenceNode : Node.GetChild("Presence")->GetChildren())
		{
			BymlActor.Presence.insert({ PresenceNode.GetKey(), PresenceNode.GetValue<std::string>() });
		}
	}

	if (Node.HasChild("Phive"))
	{
		BymlFile::Node* PhiveNode = Node.GetChild("Phive");
		if (PhiveNode->HasChild("Rails"))
		{
			for (BymlFile::Node& RailNode : PhiveNode->GetChild("Rails")->GetChildren())
			{
				Actor::PhiveData::RailData Rail;
				if (RailNode.HasChild("IsClosed"))
				{
					Rail.IsClosed = RailNode.GetChild("IsClosed")->GetValue<bool>();
				}
				if (RailNode.HasChild("Type"))
				{
					Rail.Type = RailNode.GetChild("Type")->GetValue<std::string>();
				}
				if (RailNode.HasChild("Nodes"))
				{
					for (BymlFile::Node& BymlNode : RailNode.GetChild("Nodes")->GetChildren())
					{
						for (BymlFile::Node& SubNode : BymlNode.GetChildren())
						{
							Actor::PhiveData::RailData::Node RailNode;
							RailNode.Key = SubNode.GetKey();

							RailNode.Value.SetX(SubNode.GetChild(0)->GetValue<float>());
							RailNode.Value.SetY(SubNode.GetChild(1)->GetValue<float>());
							RailNode.Value.SetZ(SubNode.GetChild(2)->GetValue<float>());

							Rail.Nodes.push_back(RailNode);
						}
					}
				}

				BymlActor.Phive.Rails.push_back(Rail);
			}
		}
		if (PhiveNode->HasChild("RopeHeadLink"))
		{
			if (PhiveNode->GetChild("RopeHeadLink")->HasChild("ID"))
			{
				BymlActor.Phive.RopeHeadLink.ID = PhiveNode->GetChild("RopeHeadLink")->GetChild("ID")->GetValue<uint64_t>();
			}
			if (PhiveNode->GetChild("RopeHeadLink")->HasChild("Owners"))
			{
				for (BymlFile::Node& OwnerNode : PhiveNode->GetChild("RopeHeadLink")->GetChild("Owners")->GetChildren())
				{
					BymlActor.Phive.RopeHeadLink.Owners.push_back(OwnerNode.GetChild("Refer")->GetValue<uint64_t>());
				}
			}
			if (PhiveNode->GetChild("RopeHeadLink")->HasChild("Refers"))
			{
				for (BymlFile::Node& ReferNode : PhiveNode->GetChild("RopeHeadLink")->GetChild("Refers")->GetChildren())
				{
					BymlActor.Phive.RopeHeadLink.Refers.push_back(ReferNode.GetChild("Owner")->GetValue<uint64_t>());
				}
			}
		}
		if (PhiveNode->HasChild("RopeTailLink"))
		{
			if (PhiveNode->GetChild("RopeTailLink")->HasChild("ID"))
			{
				BymlActor.Phive.RopeTailLink.ID = PhiveNode->GetChild("RopeTailLink")->GetChild("ID")->GetValue<uint64_t>();
			}
			if (PhiveNode->GetChild("RopeTailLink")->HasChild("Owners"))
			{
				for (BymlFile::Node& OwnerNode : PhiveNode->GetChild("RopeTailLink")->GetChild("Owners")->GetChildren())
				{
					BymlActor.Phive.RopeTailLink.Owners.push_back(OwnerNode.GetChild("Refer")->GetValue<uint64_t>());
				}
			}
			if (PhiveNode->GetChild("RopeTailLink")->HasChild("Refers"))
			{
				for (BymlFile::Node& ReferNode : PhiveNode->GetChild("RopeTailLink")->GetChild("Refers")->GetChildren())
				{
					BymlActor.Phive.RopeTailLink.Refers.push_back(ReferNode.GetChild("Owner")->GetValue<uint64_t>());
				}
			}
		}
		if (PhiveNode->HasChild("Placement"))
		{
			for (BymlFile::Node& PlacementNode : PhiveNode->GetChild("Placement")->GetChildren())
			{
				BymlActor.Phive.Placement.insert({ PlacementNode.GetKey(), PlacementNode.GetValue<std::string>() });
			}
		}
		if (PhiveNode->HasChild("ConstraintLink"))
		{
			BymlFile::Node* ConstraintLinkNode = PhiveNode->GetChild("ConstraintLink");
			if (ConstraintLinkNode->HasChild("ID"))
			{
				BymlActor.Phive.ConstraintLink.ID = ConstraintLinkNode->GetChild("ID")->GetValue<uint64_t>();
			}
			if (ConstraintLinkNode->HasChild("Refers"))
			{
				for (BymlFile::Node& ReferNode : ConstraintLinkNode->GetChild("Refers")->GetChildren())
				{
					Actor::PhiveData::ConstraintLinkData::ReferData Refer;
					if (ReferNode.HasChild("Owner"))
					{
						Refer.Owner = ReferNode.GetChild("Owner")->GetValue<uint64_t>();
					}
					if (ReferNode.HasChild("Type"))
					{
						Refer.Type = ReferNode.GetChild("Type")->GetValue<std::string>();
					}
					BymlActor.Phive.ConstraintLink.Refers.push_back(Refer);
				}
			}
			if (ConstraintLinkNode->HasChild("Owners"))
			{
				for (BymlFile::Node& OwnersNode : ConstraintLinkNode->GetChild("Owners")->GetChildren())
				{
					Actor::PhiveData::ConstraintLinkData::OwnerData Owner;
					if (OwnersNode.HasChild("BreakableData"))
					{
						for (BymlFile::Node& BreakableDataNode : OwnersNode.GetChild("BreakableData")->GetChildren())
						{
							Owner.BreakableData.insert({ BreakableDataNode.GetKey(), BreakableDataNode.GetValue<std::string>() });
						}
					}
					if (OwnersNode.HasChild("ClusterData"))
					{
						for (BymlFile::Node& ClusterDataNode : OwnersNode.GetChild("ClusterData")->GetChildren())
						{
							Owner.ClusterData.insert({ ClusterDataNode.GetKey(), ClusterDataNode.GetValue<std::string>() });
						}
					}
					if (OwnersNode.HasChild("UserData"))
					{
						for (BymlFile::Node& UserDataNode : OwnersNode.GetChild("UserData")->GetChildren())
						{
							Owner.UserData.insert({ UserDataNode.GetKey(), UserDataNode.GetValue<std::string>() });
						}
					}
					if (OwnersNode.HasChild("OwnerPose"))
					{
						if (OwnersNode.GetChild("OwnerPose")->HasChild("Rotate"))
						{
							Vector3F OwnerPoseRotate;
							OwnerPoseRotate.SetX(Util::RadiansToDegrees(OwnersNode.GetChild("OwnerPose")->GetChild("Rotate")->GetChild(0)->GetValue<float>()));
							OwnerPoseRotate.SetY(Util::RadiansToDegrees(OwnersNode.GetChild("OwnerPose")->GetChild("Rotate")->GetChild(1)->GetValue<float>()));
							OwnerPoseRotate.SetZ(Util::RadiansToDegrees(OwnersNode.GetChild("OwnerPose")->GetChild("Rotate")->GetChild(2)->GetValue<float>()));
							Owner.OwnerPose.Rotate = OwnerPoseRotate;
						}
						if (OwnersNode.GetChild("OwnerPose")->HasChild("Trans"))
						{
							Vector3F OwnerPoseTrans;
							OwnerPoseTrans.SetX(OwnersNode.GetChild("OwnerPose")->GetChild("Trans")->GetChild(0)->GetValue<float>());
							OwnerPoseTrans.SetY(OwnersNode.GetChild("OwnerPose")->GetChild("Trans")->GetChild(1)->GetValue<float>());
							OwnerPoseTrans.SetZ(OwnersNode.GetChild("OwnerPose")->GetChild("Trans")->GetChild(2)->GetValue<float>());
							Owner.OwnerPose.Translate = OwnerPoseTrans;
						}
					}
					if (OwnersNode.HasChild("ParamData"))
					{
						for (BymlFile::Node& ParamDataNode : OwnersNode.GetChild("ParamData")->GetChildren())
						{
							//				std::map<std::string, std::variant<uint32_t, int32_t, uint64_t, int64_t, float, bool, double, std::string, BymlFile::Node>> ParamData;

							switch (ParamDataNode.GetType())
							{
							case BymlFile::Type::Bool:
								Owner.ParamData.insert({ ParamDataNode.GetKey(), ParamDataNode.GetValue<bool>() });
								break;
							case BymlFile::Type::UInt32:
								Owner.ParamData.insert({ ParamDataNode.GetKey(), ParamDataNode.GetValue<uint32_t>() });
								break;
							case BymlFile::Type::Int32:
								Owner.ParamData.insert({ ParamDataNode.GetKey(), ParamDataNode.GetValue<int32_t>() });
								break;
							case BymlFile::Type::UInt64:
								Owner.ParamData.insert({ ParamDataNode.GetKey(), ParamDataNode.GetValue<uint64_t>() });
								break;
							case BymlFile::Type::Int64:
								Owner.ParamData.insert({ ParamDataNode.GetKey(), ParamDataNode.GetValue<int64_t>() });
								break;
							case BymlFile::Type::Float:
								Owner.ParamData.insert({ ParamDataNode.GetKey(), ParamDataNode.GetValue<float>() });
								break;
							case BymlFile::Type::Double:
								Owner.ParamData.insert({ ParamDataNode.GetKey(), ParamDataNode.GetValue<double>() });
								break;
							case BymlFile::Type::StringIndex:
								Owner.ParamData.insert({ ParamDataNode.GetKey(), ParamDataNode.GetValue<std::string>() });
								break;
							case BymlFile::Type::Array:
								Owner.ParamData.insert({ ParamDataNode.GetKey(), ParamDataNode });
								break;
							default:
								Logger::Error("ActorMgr", "Unknown physics ParamData option: " + ParamDataNode.GetKey());
								break;
							}
						}
					}
					if (OwnersNode.HasChild("PivotData"))
					{
						BymlFile::Node* PivotDataNode = OwnersNode.GetChild("PivotData");
						if (PivotDataNode->HasChild("Axis"))
						{
							Owner.PivotData.Axis = PivotDataNode->GetChild("Axis")->GetValue<int32_t>();
						}
						if (PivotDataNode->HasChild("AxisA"))
						{
							Owner.PivotData.AxisA = PivotDataNode->GetChild("AxisA")->GetValue<int32_t>();
						}
						if (PivotDataNode->HasChild("AxisB"))
						{
							Owner.PivotData.AxisB = PivotDataNode->GetChild("AxisB")->GetValue<int32_t>();
						}
						if (PivotDataNode->HasChild("Pivot"))
						{
							Vector3F Pivot;
							Pivot.SetX(PivotDataNode->GetChild("Pivot")->GetChild(0)->GetValue<float>());
							Pivot.SetY(PivotDataNode->GetChild("Pivot")->GetChild(1)->GetValue<float>());
							Pivot.SetZ(PivotDataNode->GetChild("Pivot")->GetChild(2)->GetValue<float>());
							Owner.PivotData.Pivot = Pivot;
						}
						if (PivotDataNode->HasChild("PivotA"))
						{
							Vector3F Pivot;
							Pivot.SetX(PivotDataNode->GetChild("PivotA")->GetChild(0)->GetValue<float>());
							Pivot.SetY(PivotDataNode->GetChild("PivotA")->GetChild(1)->GetValue<float>());
							Pivot.SetZ(PivotDataNode->GetChild("PivotA")->GetChild(2)->GetValue<float>());
							Owner.PivotData.PivotA = Pivot;
						}
						if (PivotDataNode->HasChild("PivotB"))
						{
							Vector3F Pivot;
							Pivot.SetX(PivotDataNode->GetChild("PivotB")->GetChild(0)->GetValue<float>());
							Pivot.SetY(PivotDataNode->GetChild("PivotB")->GetChild(1)->GetValue<float>());
							Pivot.SetZ(PivotDataNode->GetChild("PivotB")->GetChild(2)->GetValue<float>());
							Owner.PivotData.PivotB = Pivot;
						}
					}
					if (OwnersNode.HasChild("Refer"))
					{
						Owner.Refer = OwnersNode.GetChild("Refer")->GetValue<uint64_t>();
					}
					if (OwnersNode.HasChild("ReferPose"))
					{
						if (OwnersNode.GetChild("ReferPose")->HasChild("Rotate"))
						{
							Vector3F ReferPoseRotate;
							ReferPoseRotate.SetX(Util::RadiansToDegrees(OwnersNode.GetChild("ReferPose")->GetChild("Rotate")->GetChild(0)->GetValue<float>()));
							ReferPoseRotate.SetY(Util::RadiansToDegrees(OwnersNode.GetChild("ReferPose")->GetChild("Rotate")->GetChild(1)->GetValue<float>()));
							ReferPoseRotate.SetZ(Util::RadiansToDegrees(OwnersNode.GetChild("ReferPose")->GetChild("Rotate")->GetChild(2)->GetValue<float>()));
							Owner.ReferPose.Rotate = ReferPoseRotate;
						}
						if (OwnersNode.GetChild("ReferPose")->HasChild("Trans"))
						{
							Vector3F ReferPoseTrans;
							ReferPoseTrans.SetX(OwnersNode.GetChild("ReferPose")->GetChild("Trans")->GetChild(0)->GetValue<float>());
							ReferPoseTrans.SetY(OwnersNode.GetChild("ReferPose")->GetChild("Trans")->GetChild(1)->GetValue<float>());
							ReferPoseTrans.SetZ(OwnersNode.GetChild("ReferPose")->GetChild("Trans")->GetChild(2)->GetValue<float>());
							Owner.ReferPose.Translate = ReferPoseTrans;
						}
					}
					if (OwnersNode.HasChild("Type"))
					{
						Owner.Type = OwnersNode.GetChild("Type")->GetValue<std::string>();
					}
					BymlActor.Phive.ConstraintLink.Owners.push_back(Owner);
				}
			}
		}
	}

	//Merged actor stuff
	if (BymlActor.Dynamic.DynamicString.count("BancPath"))
	{
		if (BymlActor.Dynamic.DynamicString["BancPath"].length() == 0)
		{
			BymlActor.Dynamic.DynamicString.erase("BancPath");
			goto FinishedMergedActorLoading;
		}

		Logger::Info("ActorMgr", "Loading merged actor " + BymlActor.Dynamic.DynamicString["BancPath"]);

		if (!Util::FileExists(Editor::GetRomFSFile(BymlActor.Dynamic.DynamicString["BancPath"] + ".zs")))
		{
			Logger::Error("ActorMgr", "Could not load " + BymlActor.Dynamic.DynamicString["BancPath"] + ".zs: File not found");
			goto FinishedMergedActorLoading;
		}

		BymlFile File(ZStdFile::Decompress(Editor::GetRomFSFile(BymlActor.Dynamic.DynamicString["BancPath"] + ".zs"), ZStdFile::Dictionary::BcettByaml).Data);
		for (BymlFile::Node& ActorNode : File.GetNode("Actors")->GetChildren())
		{
			Actor MergedActor = ActorMgr::CreateBasicActor(ActorNode.GetChild("Gyaml")->GetValue<std::string>());
			ActorMgr::AddActorFromByml(ActorNode, &MergedActor);

			MergedActor.ActorType = Actor::Type::Merged;
			MergedActor.Translate.SetX(MergedActor.Translate.GetX() + BymlActor.Translate.GetX());
			MergedActor.Translate.SetY(MergedActor.Translate.GetY() + BymlActor.Translate.GetY());
			MergedActor.Translate.SetZ(MergedActor.Translate.GetZ() + BymlActor.Translate.GetZ());

			MergedActor.Scale.SetX(MergedActor.Scale.GetX() * BymlActor.Scale.GetX());
			MergedActor.Scale.SetY(MergedActor.Scale.GetY() * BymlActor.Scale.GetY());
			MergedActor.Scale.SetZ(MergedActor.Scale.GetZ() * BymlActor.Scale.GetZ());

			MergedActor.Rotate.SetX(MergedActor.Rotate.GetX() - BymlActor.Rotate.GetX());
			MergedActor.Rotate.SetY(MergedActor.Rotate.GetY() - BymlActor.Rotate.GetY());
			MergedActor.Rotate.SetZ(MergedActor.Rotate.GetZ() - BymlActor.Rotate.GetZ());

			Vector3F RadianRotate(Util::DegreesToRadians(BymlActor.Rotate.GetX()), Util::DegreesToRadians(BymlActor.Rotate.GetY()), Util::DegreesToRadians(BymlActor.Rotate.GetZ()));

			float NewX = BymlActor.Translate.GetX() + ((MergedActor.Translate.GetX() - BymlActor.Translate.GetX()) * (std::cosf(RadianRotate.GetZ()) * std::cosf(RadianRotate.GetY()))) +
				((MergedActor.Translate.GetY() - BymlActor.Translate.GetY()) * (std::cosf(RadianRotate.GetZ()) * std::sinf(RadianRotate.GetY()) * std::sinf(RadianRotate.GetX()) - std::sinf(RadianRotate.GetZ()) * std::cosf(RadianRotate.GetX()))) +
				((MergedActor.Translate.GetZ() - BymlActor.Translate.GetZ()) * (std::cosf(RadianRotate.GetZ()) * std::sinf(RadianRotate.GetY()) * std::cosf(RadianRotate.GetX()) + std::sinf(RadianRotate.GetZ()) * std::sinf(RadianRotate.GetX())));

			float NewY = BymlActor.Translate.GetY() + ((MergedActor.Translate.GetX() - BymlActor.Translate.GetX()) * (std::sinf(RadianRotate.GetZ()) * std::cosf(RadianRotate.GetY()))) +
				((MergedActor.Translate.GetY() - BymlActor.Translate.GetY()) * (std::sinf(RadianRotate.GetZ()) * std::sinf(RadianRotate.GetY()) * std::sinf(RadianRotate.GetX()) + std::cosf(RadianRotate.GetZ()) * std::cosf(RadianRotate.GetX()))) +
				((MergedActor.Translate.GetZ() - BymlActor.Translate.GetZ()) * (std::sinf(RadianRotate.GetZ()) * std::sinf(RadianRotate.GetY()) * std::cosf(RadianRotate.GetX()) - std::cosf(RadianRotate.GetZ()) * std::sinf(RadianRotate.GetX())));

			float NewZ = BymlActor.Translate.GetZ() + ((MergedActor.Translate.GetX() - BymlActor.Translate.GetX()) * (-std::sinf(RadianRotate.GetY()))) +
				((MergedActor.Translate.GetY() - BymlActor.Translate.GetY()) * (std::cosf(RadianRotate.GetY()) * std::sinf(RadianRotate.GetX()))) +
				((MergedActor.Translate.GetZ() - BymlActor.Translate.GetZ()) * (std::cosf(RadianRotate.GetY()) * std::cosf(RadianRotate.GetX())));

			MergedActor.Translate.SetX(NewX);
			MergedActor.Translate.SetY(NewY);
			MergedActor.Translate.SetZ(NewZ);

			BymlActor.MergedActorContent.push_back(MergedActor);
		}
		BymlActor.MergedActorByml = File;
	}
	FinishedMergedActorLoading:

	//Cave stuff
	//LocationArea - Location (Dynamic) - Cave_CentralHyrule_0009
	/*
	if (BymlActor.Dynamic.DynamicString.count("Location") && BymlActor.Gyml == "LocationArea")
	{
		if (BymlActor.Dynamic.DynamicString["Location"].length() == 0 || !BymlActor.Dynamic.DynamicString["Location"].starts_with("Cave_"))
		{
			BymlActor.Dynamic.DynamicString.erase("Location");
			goto FinishedCaveLoading;
		}
		Logger::Info("ActorMgr", "Loading cave " + BymlActor.Dynamic.DynamicString["Location"]);
		//BymlActor.Dynamic.DynamicString["Location"]
		BymlFile FileStatic(ZStdFile::Decompress(Editor::GetRomFSFile(Editor::BancDir + "/Cave/" + BymlActor.Dynamic.DynamicString["Location"] + "_GroupSet_000_Static.bcett.byml.zs"), ZStdFile::Dictionary::BcettByaml).Data);
		BymlFile FileDynamic(ZStdFile::Decompress(Editor::GetRomFSFile(Editor::BancDir + "/Cave/" + BymlActor.Dynamic.DynamicString["Location"] + "_GroupSet_000_Dynamic.bcett.byml.zs"), ZStdFile::Dictionary::BcettByaml).Data);
		
		auto DecodeByml = [](BymlFile& File, Actor::Type Type)
			{
				for (BymlFile::Node& ActorNode : File.GetNode("Actors")->GetChildren())
				{
					Actor* NewActor = ActorMgr::AddActorFromByml(ActorNode);
					if (NewActor == nullptr) return;

					NewActor->ActorType = Type;
				}
			};

		DecodeByml(FileStatic, Actor::Type::Static);
		DecodeByml(FileDynamic, Actor::Type::Dynamic);
	}

FinishedCaveLoading:
*/


	return &BymlActor;
}

void WriteBymlVector(BymlFile::Node* Root, std::string Key, Vector3F Vector, Vector3F DefaultValues, bool SkipDefaultVectorCheck)
{
	if (Vector.GetX() != DefaultValues.GetX() || Vector.GetY() != DefaultValues.GetY() || Vector.GetZ() != DefaultValues.GetZ() || SkipDefaultVectorCheck)
	{
		BymlFile::Node VectorRoot(BymlFile::Type::Array, Key);

		BymlFile::Node VectorX(BymlFile::Type::Float, "0");
		VectorX.SetValue<float>(Vector.GetX());
		VectorRoot.AddChild(VectorX);

		BymlFile::Node VectorY(BymlFile::Type::Float, "1");
		VectorY.SetValue<float>(Vector.GetY());
		VectorRoot.AddChild(VectorY);

		BymlFile::Node VectorZ(BymlFile::Type::Float, "2");
		VectorZ.SetValue<float>(Vector.GetZ());
		VectorRoot.AddChild(VectorZ);

		Root->AddChild(VectorRoot);
	}
}

BymlFile::Type GetDynamicValueDataType(std::string Value)
{
	if (Value.empty())
	{
		return BymlFile::Type::StringIndex;
	}

	if (Value == "true" || Value == "false")
	{
		return BymlFile::Type::Bool;
	}

	// Value for signed integers
	if (Value.find_first_not_of("0123456789") == Value.npos && Value.find("-") != std::string::npos)
	{
		if (Value.size() <= 11)
		{
			return BymlFile::Type::Int32;
		}
		else if (Value.size() <= 20)
		{
			return BymlFile::Type::Int64;
		}
		else
		{
			return BymlFile::Type::Null;
		}
	}

	// Check for unsigned integers
	if (Value.find_first_not_of("0123456789") == Value.npos)
	{
		if (Value.size() <= 10)
		{
			return BymlFile::Type::UInt32;
		}
		else if (Value.size() <= 20)
		{
			return BymlFile::Type::UInt64;
		}
		else
		{
			return BymlFile::Type::Null;
		}
	}

	// Check for floats
	if (Value.find(".") != std::string::npos && Value.find_first_not_of("0123456789.-") == Value.npos)
	{
		return BymlFile::Type::Float;
	}

	// Default to String
	return BymlFile::Type::StringIndex;
}

BymlFile::Node ActorMgr::ActorToByml(Actor& ExportedActor)
{
	BymlFile::Node ActorNode(BymlFile::Type::Dictionary);

	/*
	if (ExportedActor.GetType() == Actor::Type::Merged)
	{
		auto Iter = Config::MergedActorBymls.begin();
		std::advance(Iter, ExportedActor.GetMergedActorIndex());

		ExportedActor.GetTranslate().SetX(ExportedActor.GetTranslate().GetX() - Actors->at(Iter->first).GetTranslate().GetX());
		ExportedActor.GetTranslate().SetY(ExportedActor.GetTranslate().GetY() - Actors->at(Iter->first).GetTranslate().GetY());
		ExportedActor.GetTranslate().SetZ(ExportedActor.GetTranslate().GetZ() - Actors->at(Iter->first).GetTranslate().GetZ());
	}
	*/

	if (ExportedActor.Bakeable)
	{
		BymlFile::Node Bakeable(BymlFile::Type::Bool, "Bakeable");
		Bakeable.SetValue<bool>(true);
		ActorNode.AddChild(Bakeable);
	}

	if (!ExportedActor.Dynamic.DynamicString.empty() || !ExportedActor.Dynamic.DynamicVector.empty())
	{
		BymlFile::Node Dynamic(BymlFile::Type::Dictionary, "Dynamic");
		for (auto const& [Key, Value] : ExportedActor.Dynamic.DynamicString)
		{
			BymlFile::Node DynamicEntry(GetDynamicValueDataType(Value), Key);
			if (DynamicEntry.GetType() == BymlFile::Type::UInt32) DynamicEntry.GetType() = BymlFile::Type::Int32;

			switch (DynamicEntry.GetType())
			{
			case BymlFile::Type::StringIndex:
				DynamicEntry.SetValue<std::string>(Value);
				break;
			case BymlFile::Type::Float:
				DynamicEntry.SetValue<float>(Util::StringToNumber<float>(Value));
				break;
			case BymlFile::Type::UInt32:
				DynamicEntry.SetValue<uint32_t>(Util::StringToNumber<uint32_t>(Value));
				break;
			case BymlFile::Type::UInt64:
				DynamicEntry.SetValue<uint64_t>(Util::StringToNumber<uint64_t>(Value));
				break;
			case BymlFile::Type::Int32:
				DynamicEntry.SetValue<int32_t>(Util::StringToNumber<int32_t>(Value));
				break;
			case BymlFile::Type::Int64:
				DynamicEntry.SetValue<int64_t>(Util::StringToNumber<int64_t>(Value));
				break;
			case BymlFile::Type::Bool:
				DynamicEntry.SetValue<bool>(Value == "true");
				break;
			}
			Dynamic.AddChild(DynamicEntry);
		}
		for (auto const& [Key, Value] : ExportedActor.Dynamic.DynamicVector)
		{
			WriteBymlVector(&Dynamic, Key, Value, Vector3F(0, 0, 0), true);
		}
		ActorNode.AddChild(Dynamic);
	}

	if (ExportedActor.ExtraCreateRadius != 0.0f)
	{
		BymlFile::Node ExtraCreateRadius(BymlFile::Type::Float, "ExtraCreateRadius");
		ExtraCreateRadius.SetValue<float>(ExportedActor.ExtraCreateRadius);
		ActorNode.AddChild(ExtraCreateRadius);
	}

	BymlFile::Node Gyml(BymlFile::Type::StringIndex, "Gyaml");
	Gyml.SetValue<std::string>(ExportedActor.Gyml);
	ActorNode.AddChild(Gyml);

	BymlFile::Node Hash(BymlFile::Type::UInt64, "Hash");
	Hash.SetValue<uint64_t>(ExportedActor.Hash);
	ActorNode.AddChild(Hash);

	if (ExportedActor.ForceActive)
	{
		BymlFile::Node ForceActive(BymlFile::Type::Bool, "IsForceActive");
		ForceActive.SetValue<bool>(ExportedActor.ForceActive);
		ActorNode.AddChild(ForceActive);
	}

	if (ExportedActor.InWater)
	{
		BymlFile::Node IsInWater(BymlFile::Type::Bool, "IsInWater");
		IsInWater.SetValue<bool>(ExportedActor.InWater);
		ActorNode.AddChild(IsInWater);
	}

	if (ExportedActor.PhysicsStable)
	{
		BymlFile::Node PhysicStable(BymlFile::Type::Bool, "IsPhysicsStable");
		PhysicStable.SetValue<bool>(ExportedActor.PhysicsStable);
		ActorNode.AddChild(PhysicStable);
	}

	if (!ExportedActor.Links.empty())
	{
		BymlFile::Node Links(BymlFile::Type::Array, "Links");
		for (Actor::Link& Link : ExportedActor.Links)
		{

			BymlFile::Node LinkDict(BymlFile::Type::Dictionary);

			BymlFile::Node LinkDst(BymlFile::Type::UInt64, "Dst");
			LinkDst.SetValue<uint64_t>(Link.Dest);
			LinkDict.AddChild(LinkDst);

			if (Link.Gyml != "")
			{
				BymlFile::Node LinkGyaml(BymlFile::Type::StringIndex, "Gyaml");
				LinkGyaml.SetValue<std::string>(Link.Gyml);
				LinkDict.AddChild(LinkGyaml);
			}

			BymlFile::Node LinkName(BymlFile::Type::StringIndex, "Name");
			LinkName.SetValue<std::string>(Link.Name);

			BymlFile::Node LinkSrc(BymlFile::Type::UInt64, "Src");
			LinkSrc.SetValue<uint64_t>(Link.Src);

			LinkDict.AddChild(LinkName);
			LinkDict.AddChild(LinkSrc);
			Links.AddChild(LinkDict);
		}
		ActorNode.AddChild(Links);
	}

	if (ExportedActor.MoveRadius != 0)
	{
		BymlFile::Node MoveRadius(BymlFile::Type::Float, "MoveRadius");
		MoveRadius.SetValue<float>(ExportedActor.MoveRadius);
		ActorNode.AddChild(MoveRadius);
	}

	if (ExportedActor.Name.length() > 0)
	{
		BymlFile::Node Name(BymlFile::Type::StringIndex, "Name");
		Name.SetValue<std::string>(ExportedActor.Name);
		ActorNode.AddChild(Name);
	}

	if (!ExportedActor.Phive.Placement.empty() || !ExportedActor.Phive.ConstraintLink.Owners.empty() || ExportedActor.Phive.ConstraintLink.ID != 0 || !ExportedActor.Phive.ConstraintLink.Refers.empty())
	{
		BymlFile::Node Phive(BymlFile::Type::Dictionary, "Phive");

		if (!ExportedActor.Phive.ConstraintLink.Owners.empty() || ExportedActor.Phive.ConstraintLink.ID != 0 || !ExportedActor.Phive.ConstraintLink.Refers.empty())
		{
			BymlFile::Node ConstraintLink(BymlFile::Type::Dictionary, "ConstraintLink");

			if (ExportedActor.Phive.ConstraintLink.ID != 0)
			{
				BymlFile::Node ID(BymlFile::Type::UInt64, "ID");
				ID.SetValue<uint64_t>(ExportedActor.Phive.ConstraintLink.ID);
				ConstraintLink.AddChild(ID);
			}

			if (!ExportedActor.Phive.ConstraintLink.Owners.empty())
			{
				BymlFile::Node Owners(BymlFile::Type::Array, "Owners");
				for (Actor::PhiveData::ConstraintLinkData::OwnerData& Owner : ExportedActor.Phive.ConstraintLink.Owners)
				{
					BymlFile::Node OwnerNode(BymlFile::Type::Dictionary);

					if (!Owner.BreakableData.empty())
					{
						BymlFile::Node BreakableDataNode(BymlFile::Type::Dictionary, "BreakableData");

						for (auto const& [Key, Value] : Owner.BreakableData)
						{
							BymlFile::Node BreakableDataEntry(GetDynamicValueDataType(Value), Key);
							switch (BreakableDataEntry.GetType())
							{
							case BymlFile::Type::StringIndex:
								BreakableDataEntry.SetValue<std::string>(Value);
								break;
							case BymlFile::Type::Float:
								BreakableDataEntry.SetValue<float>(Util::StringToNumber<float>(Value));
								break;
							case BymlFile::Type::UInt32:
								BreakableDataEntry.SetValue<uint32_t>(Util::StringToNumber<uint32_t>(Value));
								break;
							case BymlFile::Type::UInt64:
								BreakableDataEntry.SetValue<uint64_t>(Util::StringToNumber<uint64_t>(Value));
								break;
							case BymlFile::Type::Int32:
								BreakableDataEntry.SetValue<int32_t>(Util::StringToNumber<int32_t>(Value));
								break;
							case BymlFile::Type::Int64:
								BreakableDataEntry.SetValue<int64_t>(Util::StringToNumber<int64_t>(Value));
								break;
							case BymlFile::Type::Bool:
								BreakableDataEntry.SetValue<bool>(Value == "true");
								break;
							}
							BreakableDataNode.AddChild(BreakableDataEntry);
						}

						OwnerNode.AddChild(BreakableDataNode);
					}


					if (!Owner.ClusterData.empty())
					{
						BymlFile::Node ClusterDataNode(BymlFile::Type::Dictionary, "ClusterData");

						for (auto const& [Key, Value] : Owner.ClusterData)
						{
							BymlFile::Node ClusterDataEntry(GetDynamicValueDataType(Value), Key);
							switch (ClusterDataEntry.GetType())
							{
							case BymlFile::Type::StringIndex:
								ClusterDataEntry.SetValue<std::string>(Value);
								break;
							case BymlFile::Type::Float:
								ClusterDataEntry.SetValue<float>(Util::StringToNumber<float>(Value));
								break;
							case BymlFile::Type::UInt32:
								ClusterDataEntry.SetValue<uint32_t>(Util::StringToNumber<uint32_t>(Value));
								break;
							case BymlFile::Type::UInt64:
								ClusterDataEntry.SetValue<uint64_t>(Util::StringToNumber<uint64_t>(Value));
								break;
							case BymlFile::Type::Int32:
								ClusterDataEntry.SetValue<int32_t>(Util::StringToNumber<int32_t>(Value));
								break;
							case BymlFile::Type::Int64:
								ClusterDataEntry.SetValue<int64_t>(Util::StringToNumber<int64_t>(Value));
								break;
							case BymlFile::Type::Bool:
								ClusterDataEntry.SetValue<bool>(Value == "true");
								break;
							}
							ClusterDataNode.AddChild(ClusterDataEntry);
						}

						OwnerNode.AddChild(ClusterDataNode);
					}

					if (Owner.OwnerPose.Rotate.GetX() != 0 ||
						Owner.OwnerPose.Rotate.GetY() != 0 ||
						Owner.OwnerPose.Rotate.GetZ() != 0 ||
						Owner.OwnerPose.Translate.GetX() != 0 ||
						Owner.OwnerPose.Translate.GetY() != 0 ||
						Owner.OwnerPose.Translate.GetZ() != 0)
					{
						BymlFile::Node OwnerPoseNode(BymlFile::Type::Dictionary, "OwnerPose");

						Vector3F RotateVector(Util::DegreesToRadians(Owner.OwnerPose.Rotate.GetX()), Util::DegreesToRadians(Owner.OwnerPose.Rotate.GetY()), Util::DegreesToRadians(Owner.OwnerPose.Rotate.GetZ()));
						WriteBymlVector(&OwnerPoseNode, "Rotate", RotateVector, Vector3F(0, 0, 0), true);
						WriteBymlVector(&OwnerPoseNode, "Trans", Owner.OwnerPose.Translate, Vector3F(0, 0, 0), true);

						OwnerNode.AddChild(OwnerPoseNode);
					}

					if (!Owner.ParamData.empty())
					{
						BymlFile::Node ParamDataNode(BymlFile::Type::Dictionary, "ParamData");

						for (auto& [Key, Value] : Owner.ParamData)
						{
							BymlFile::Node ParamDataEntry(BymlFile::Type::Null, Key);

							if (std::holds_alternative<bool>(Value))
							{
								ParamDataEntry.m_Type = BymlFile::Type::Bool;
								ParamDataEntry.SetValue<bool>(*reinterpret_cast<bool*>(&Value));
							}
							else if (std::holds_alternative<uint32_t>(Value))
							{
								ParamDataEntry.m_Type = BymlFile::Type::UInt32;
								ParamDataEntry.SetValue<uint32_t>(*reinterpret_cast<uint32_t*>(&Value));
							}
							else if (std::holds_alternative<int32_t>(Value))
							{
								ParamDataEntry.m_Type = BymlFile::Type::Int32;
								ParamDataEntry.SetValue<int32_t>(*reinterpret_cast<int32_t*>(&Value));
							}
							else if (std::holds_alternative<uint64_t>(Value))
							{
								ParamDataEntry.m_Type = BymlFile::Type::UInt64;
								ParamDataEntry.SetValue<uint64_t>(*reinterpret_cast<uint64_t*>(&Value));
							}
							else if (std::holds_alternative<int64_t>(Value))
							{
								ParamDataEntry.m_Type = BymlFile::Type::Int64;
								ParamDataEntry.SetValue<int64_t>(*reinterpret_cast<int64_t*>(&Value));
							}
							else if (std::holds_alternative<float>(Value))
							{
								ParamDataEntry.m_Type = BymlFile::Type::Float;
								ParamDataEntry.SetValue<float>(*reinterpret_cast<float*>(&Value));
							}
							else if (std::holds_alternative<double>(Value))
							{
								ParamDataEntry.m_Type = BymlFile::Type::Double;
								ParamDataEntry.SetValue<double>(*reinterpret_cast<double*>(&Value));
							}
							else if (std::holds_alternative<std::string>(Value))
							{
								ParamDataEntry.m_Type = BymlFile::Type::StringIndex;
								ParamDataEntry.SetValue<std::string>(*reinterpret_cast<std::string*>(&Value));
							}
							else if (std::holds_alternative<BymlFile::Node>(Value))
							{
								ParamDataEntry = *reinterpret_cast<BymlFile::Node*>(&Value);
							}
							else
							{
								Logger::Error("ActorMgr", "Unknown data type while writing ParamData " + Key);
								goto AddParamDataNode;
							}

							ParamDataNode.AddChild(ParamDataEntry);
						}

					AddParamDataNode:
						OwnerNode.AddChild(ParamDataNode);
					}

					BymlFile::Node PivotDataNode(BymlFile::Type::Dictionary, "PivotData");
					//Axis
					if (Owner.PivotData.Axis != std::numeric_limits<int32_t>::max())
					{
						BymlFile::Node PivotDataAxisNode(BymlFile::Type::Int32, "Axis");
						PivotDataAxisNode.SetValue<int32_t>(Owner.PivotData.Axis);
						PivotDataNode.AddChild(PivotDataAxisNode);
					}
					if (Owner.PivotData.AxisA != std::numeric_limits<int32_t>::max())
					{
						BymlFile::Node PivotDataAxisNode(BymlFile::Type::Int32, "AxisA");
						PivotDataAxisNode.SetValue<int32_t>(Owner.PivotData.AxisA);
						PivotDataNode.AddChild(PivotDataAxisNode);
					}
					if (Owner.PivotData.AxisB != std::numeric_limits<int32_t>::max())
					{
						BymlFile::Node PivotDataAxisNode(BymlFile::Type::Int32, "AxisB");
						PivotDataAxisNode.SetValue<int32_t>(Owner.PivotData.AxisB);
						PivotDataNode.AddChild(PivotDataAxisNode);
					}
					//Vectors
					if (Owner.PivotData.Pivot.GetX() != std::numeric_limits<float>::max() || Owner.PivotData.Pivot.GetY() != std::numeric_limits<float>::max() || Owner.PivotData.Pivot.GetZ() != std::numeric_limits<float>::max())
					{
						WriteBymlVector(&PivotDataNode, "Pivot", Owner.PivotData.Pivot, Vector3F(0, 0, 0), true);
					}
					if (Owner.PivotData.PivotA.GetX() != std::numeric_limits<float>::max() || Owner.PivotData.PivotA.GetY() != std::numeric_limits<float>::max() || Owner.PivotData.PivotA.GetZ() != std::numeric_limits<float>::max())
					{
						WriteBymlVector(&PivotDataNode, "PivotA", Owner.PivotData.PivotA, Vector3F(0, 0, 0), true);
					}
					if (Owner.PivotData.PivotB.GetX() != std::numeric_limits<float>::max() || Owner.PivotData.PivotB.GetY() != std::numeric_limits<float>::max() || Owner.PivotData.PivotB.GetZ() != std::numeric_limits<float>::max())
					{
						WriteBymlVector(&PivotDataNode, "PivotB", Owner.PivotData.PivotB, Vector3F(0, 0, 0), true);
					}
					OwnerNode.AddChild(PivotDataNode);

					BymlFile::Node ReferNode(BymlFile::Type::UInt64, "Refer");
					ReferNode.SetValue<uint64_t>(Owner.Refer);
					OwnerNode.AddChild(ReferNode);

					if (Owner.ReferPose.Rotate.GetX() != 0 ||
						Owner.ReferPose.Rotate.GetY() != 0 ||
						Owner.ReferPose.Rotate.GetZ() != 0 ||
						Owner.ReferPose.Translate.GetX() != 0 ||
						Owner.ReferPose.Translate.GetY() != 0 ||
						Owner.ReferPose.Translate.GetZ() != 0)
					{
						BymlFile::Node ReferPoseNode(BymlFile::Type::Dictionary, "ReferPose");

						Vector3F RotateVector(Util::DegreesToRadians(Owner.ReferPose.Rotate.GetX()), Util::DegreesToRadians(Owner.ReferPose.Rotate.GetY()), Util::DegreesToRadians(Owner.ReferPose.Rotate.GetZ()));
						WriteBymlVector(&ReferPoseNode, "Rotate", RotateVector, Vector3F(0, 0, 0), true);
						WriteBymlVector(&ReferPoseNode, "Trans", Owner.ReferPose.Translate, Vector3F(0, 0, 0), true);

						OwnerNode.AddChild(ReferPoseNode);
					}

					if (Owner.Type.length() > 0)
					{
						BymlFile::Node TypeNode(BymlFile::Type::StringIndex, "Type");
						TypeNode.SetValue<std::string>(Owner.Type);
						OwnerNode.AddChild(TypeNode);
					}

					if (!Owner.UserData.empty())
					{
						BymlFile::Node UserDataNode(BymlFile::Type::Dictionary, "UserData");

						for (auto const& [Key, Value] : Owner.UserData)
						{
							BymlFile::Node UserDataEntry(GetDynamicValueDataType(Value), Key);
							switch (UserDataEntry.GetType())
							{
							case BymlFile::Type::StringIndex:
								UserDataEntry.SetValue<std::string>(Value);
								break;
							case BymlFile::Type::Float:
								UserDataEntry.SetValue<float>(Util::StringToNumber<float>(Value));
								break;
							case BymlFile::Type::UInt32:
								UserDataEntry.SetValue<uint32_t>(Util::StringToNumber<uint32_t>(Value));
								break;
							case BymlFile::Type::UInt64:
								UserDataEntry.SetValue<uint64_t>(Util::StringToNumber<uint64_t>(Value));
								break;
							case BymlFile::Type::Int32:
								UserDataEntry.SetValue<int32_t>(Util::StringToNumber<int32_t>(Value));
								break;
							case BymlFile::Type::Int64:
								UserDataEntry.SetValue<int64_t>(Util::StringToNumber<int64_t>(Value));
								break;
							case BymlFile::Type::Bool:
								UserDataEntry.SetValue<bool>(Value == "true");
								break;
							}
							UserDataNode.AddChild(UserDataEntry);
						}

						OwnerNode.AddChild(UserDataNode);
					}
					Owners.AddChild(OwnerNode);
				}
				ConstraintLink.AddChild(Owners);
			}

			if (!ExportedActor.Phive.ConstraintLink.Refers.empty())
			{
				BymlFile::Node Refers(BymlFile::Type::Array, "Refers");
				for (Actor::PhiveData::ConstraintLinkData::ReferData Refer : ExportedActor.Phive.ConstraintLink.Refers)
				{
					BymlFile::Node ReferNode(BymlFile::Type::Dictionary);

					BymlFile::Node Owner(BymlFile::Type::UInt64, "Owner");
					Owner.SetValue<uint64_t>(Refer.Owner);

					BymlFile::Node Type(BymlFile::Type::StringIndex, "Type");
					Type.SetValue<std::string>(Refer.Type);

					ReferNode.AddChild(Owner);
					ReferNode.AddChild(Type);

					Refers.AddChild(ReferNode);
				}
				ConstraintLink.AddChild(Refers);
			}

			Phive.AddChild(ConstraintLink);
		}

		if (!ExportedActor.Phive.Placement.empty())
		{
			BymlFile::Node Placement(BymlFile::Type::Dictionary, "Placement");

			for (auto const& [Key, Value] : ExportedActor.Phive.Placement)
			{
				BymlFile::Node PlacementEntry(GetDynamicValueDataType(Value), Key);
				if (Key == "GroupID")
				{
					PlacementEntry.GetType() = BymlFile::Type::Int32;
				}
				switch (PlacementEntry.GetType())
				{
				case BymlFile::Type::StringIndex:
					PlacementEntry.SetValue<std::string>(Value);
					break;
				case BymlFile::Type::Float:
					PlacementEntry.SetValue<float>(Util::StringToNumber<float>(Value));
					break;
				case BymlFile::Type::UInt32:
					PlacementEntry.SetValue<uint32_t>(Util::StringToNumber<uint32_t>(Value));
					break;
				case BymlFile::Type::UInt64:
					PlacementEntry.SetValue<uint64_t>(Util::StringToNumber<uint64_t>(Value));
					break;
				case BymlFile::Type::Int32:
					PlacementEntry.SetValue<int32_t>(Util::StringToNumber<int32_t>(Value));
					break;
				case BymlFile::Type::Int64:
					PlacementEntry.SetValue<int64_t>(Util::StringToNumber<int64_t>(Value));
					break;
				}
				Placement.AddChild(PlacementEntry);
			}
			Phive.AddChild(Placement);
		}

		if (!ExportedActor.Phive.Rails.empty())
		{
			BymlFile::Node Rails(BymlFile::Type::Array, "Rails");

			for (Actor::PhiveData::RailData& Rail : ExportedActor.Phive.Rails)
			{
				BymlFile::Node RailNode(BymlFile::Type::Dictionary);

				BymlFile::Node IsClosed(BymlFile::Type::Bool, "IsClosed");
				IsClosed.SetValue<bool>(Rail.IsClosed);
				RailNode.AddChild(IsClosed);

				if (!Rail.Nodes.empty())
				{
					BymlFile::Node Nodes(BymlFile::Type::Array, "Nodes");

					for (Actor::PhiveData::RailData::Node RailNode : Rail.Nodes)
					{
						BymlFile::Node RailDict(BymlFile::Type::Dictionary);
						WriteBymlVector(&RailDict, RailNode.Key, RailNode.Value, Vector3F(0, 0, 0), true);
						Nodes.AddChild(RailDict);
					}

					RailNode.AddChild(Nodes);
				}

				if (Rail.Type != "")
				{
					BymlFile::Node Type(BymlFile::Type::StringIndex, "Type");
					Type.SetValue<std::string>(Rail.Type);
					RailNode.AddChild(Type);
				}

				Rails.AddChild(RailNode);
			}

			Phive.AddChild(Rails);
		}

		if (ExportedActor.Phive.RopeHeadLink.ID != 0)
		{
			BymlFile::Node RopeHeadLinkNode(BymlFile::Type::Dictionary, "RopeHeadLink");

			BymlFile::Node ID(BymlFile::Type::UInt64, "ID");
			ID.SetValue<uint64_t>(ExportedActor.Phive.RopeHeadLink.ID);
			RopeHeadLinkNode.AddChild(ID);

			if (!ExportedActor.Phive.RopeHeadLink.Owners.empty())
			{
				BymlFile::Node Owners(BymlFile::Type::Array, "Owners");

				for (uint64_t Owner : ExportedActor.Phive.RopeHeadLink.Owners)
				{
					BymlFile::Node OwnerNode(BymlFile::Type::Dictionary);

					BymlFile::Node Refer(BymlFile::Type::UInt64, "Refer");
					Refer.SetValue<uint64_t>(Owner);
					OwnerNode.AddChild(Refer);

					Owners.AddChild(OwnerNode);
				}
				RopeHeadLinkNode.AddChild(Owners);
			}

			if (!ExportedActor.Phive.RopeHeadLink.Refers.empty())
			{
				BymlFile::Node Refers(BymlFile::Type::Array, "Refers");

				for (uint64_t Refer : ExportedActor.Phive.RopeHeadLink.Refers)
				{
					BymlFile::Node ReferNode(BymlFile::Type::Dictionary);

					BymlFile::Node Owner(BymlFile::Type::UInt64, "Owner");
					Owner.SetValue<uint64_t>(Refer);
					ReferNode.AddChild(Owner);

					Refers.AddChild(ReferNode);
				}
				RopeHeadLinkNode.AddChild(Refers);
			}

			Phive.AddChild(RopeHeadLinkNode);
		}

		if (ExportedActor.Phive.RopeTailLink.ID != 0)
		{
			BymlFile::Node RopeTailLinkNode(BymlFile::Type::Dictionary, "RopeTailLink");

			BymlFile::Node ID(BymlFile::Type::UInt64, "ID");
			ID.SetValue<uint64_t>(ExportedActor.Phive.RopeTailLink.ID);
			RopeTailLinkNode.AddChild(ID);

			if (!ExportedActor.Phive.RopeTailLink.Owners.empty())
			{
				BymlFile::Node Owners(BymlFile::Type::Array, "Owners");

				for (uint64_t Owner : ExportedActor.Phive.RopeTailLink.Owners)
				{
					BymlFile::Node OwnerNode(BymlFile::Type::Dictionary);

					BymlFile::Node Refer(BymlFile::Type::UInt64, "Refer");
					Refer.SetValue<uint64_t>(Owner);
					OwnerNode.AddChild(Refer);

					Owners.AddChild(OwnerNode);
				}
				RopeTailLinkNode.AddChild(Owners);
			}

			if (!ExportedActor.Phive.RopeTailLink.Refers.empty())
			{
				BymlFile::Node Refers(BymlFile::Type::Array, "Refers");

				for (uint64_t Refer : ExportedActor.Phive.RopeTailLink.Refers)
				{
					BymlFile::Node ReferNode(BymlFile::Type::Dictionary);

					BymlFile::Node Owner(BymlFile::Type::UInt64, "Owner");
					Owner.SetValue<uint64_t>(Refer);
					ReferNode.AddChild(Owner);

					Refers.AddChild(ReferNode);
				}
				RopeTailLinkNode.AddChild(Refers);
			}

			Phive.AddChild(RopeTailLinkNode);
		}

		ActorNode.AddChild(Phive);
	}

	if (!ExportedActor.Presence.empty())
	{
		BymlFile::Node Presence(BymlFile::Type::Dictionary, "Presence");
		for (auto const& [Key, Value] : ExportedActor.Presence)
		{
			BymlFile::Node PresenceEntry(GetDynamicValueDataType(Value), Key);
			switch (PresenceEntry.GetType())
			{
			case BymlFile::Type::StringIndex:
				PresenceEntry.SetValue<std::string>(Value);
				break;
			case BymlFile::Type::Float:
				PresenceEntry.SetValue<float>(Util::StringToNumber<float>(Value));
				break;
			case BymlFile::Type::UInt32:
				PresenceEntry.SetValue<uint32_t>(Util::StringToNumber<uint32_t>(Value));
				break;
			case BymlFile::Type::UInt64:
				PresenceEntry.SetValue<uint64_t>(Util::StringToNumber<uint64_t>(Value));
				break;
			case BymlFile::Type::Int32:
				PresenceEntry.SetValue<int32_t>(Util::StringToNumber<int32_t>(Value));
				break;
			case BymlFile::Type::Int64:
				PresenceEntry.SetValue<int64_t>(Util::StringToNumber<int64_t>(Value));
				break;
			case BymlFile::Type::Bool:
				PresenceEntry.SetValue<bool>(Value == "true");
				break;
			}
			Presence.AddChild(PresenceEntry);
		}
		ActorNode.AddChild(Presence);
	}

	if (!ExportedActor.Rails.empty())
	{
		BymlFile::Node Rails(BymlFile::Type::Array, "Rails");
		for (Actor::Rail Rail : ExportedActor.Rails)
		{
			BymlFile::Node RailDict(BymlFile::Type::Dictionary);

			BymlFile::Node RailDst(BymlFile::Type::UInt64, "Dst");
			RailDst.SetValue<uint64_t>(Rail.Dest);
			RailDict.AddChild(RailDst);

			if (Rail.Gyml != "")
			{
				BymlFile::Node RailGyaml(BymlFile::Type::StringIndex, "Gyaml");
				RailGyaml.SetValue<std::string>(Rail.Gyml);
				RailDict.AddChild(RailGyaml);
			}

			BymlFile::Node RailName(BymlFile::Type::StringIndex, "Name");
			RailName.SetValue<std::string>(Rail.Name);

			RailDict.AddChild(RailName);
			Rails.AddChild(RailDict);
		}
		ActorNode.AddChild(Rails);
	}

	Vector3F RotateVector(Util::DegreesToRadians(ExportedActor.Rotate.GetX()), Util::DegreesToRadians(ExportedActor.Rotate.GetY()), Util::DegreesToRadians(ExportedActor.Rotate.GetZ()));
	WriteBymlVector(&ActorNode, "Rotate", RotateVector, Vector3F(0, 0, 0), false);

	BymlFile::Node SRTHash(BymlFile::Type::UInt32, "SRTHash");
	SRTHash.SetValue<uint32_t>(ExportedActor.SRTHash);
	ActorNode.AddChild(SRTHash);

	WriteBymlVector(&ActorNode, "Scale", ExportedActor.Scale, Vector3F(1, 1, 1), false);
	WriteBymlVector(&ActorNode, "Translate", ExportedActor.Translate, Vector3F(0, 0, 0), false);

	if (ExportedActor.TurnActorNearEnemy)
	{
		BymlFile::Node TurnActorNearEnemy(BymlFile::Type::Bool, "TurnActorNearEnemy");
		TurnActorNearEnemy.SetValue<bool>(ExportedActor.TurnActorNearEnemy);
		ActorNode.AddChild(TurnActorNearEnemy);
	}

	return ActorNode;
}

std::string ActorMgr::GetDefaultModelKey(Actor& Actor)
{
	for (auto& [Key, Val] : BfresLibrary::Models)
	{
		if (Actor.Gyml.find(Key) != std::string::npos)
		{
			return Key;
		}
	}
	return "Default";
}

Actor& ActorMgr::AddActor(Actor Template, bool UpdateOrder)
{
	HashMgr::Hash Hash = HashMgr::GetHash(!Template.Phive.Placement.empty());
	Template.Hash = Hash.Hash;
	Template.SRTHash = Hash.SRTHash;

	Actors.push_back(Template);

	if (UpdateOrder)
		UpdateModelOrder();

	return Actors[Actors.size() - 1];
}

Actor& ActorMgr::AddActor(std::string Gyml, bool UpdateOrder, bool UseCachedData)
{
	Actor NewActor;
	NewActor.Gyml = Gyml;

	HashMgr::Hash Hash = HashMgr::GetHash(false); //TODO: Check if actor needs physics, search in actor pack
	NewActor.Hash = Hash.Hash;
	NewActor.SRTHash = Hash.SRTHash;

	//Looking for a model
	if (!ActorInfo.count(Gyml) || !UseCachedData)
	{
		if (ActorInfo.count(Gyml))
		{
			ActorInfo.erase(Gyml);
		}

		ZStdFile::Result Result = ZStdFile::Decompress(Editor::GetRomFSFile("Pack/Actor/" + Gyml + ".pack.zs"), ZStdFile::Dictionary::Pack);
		if (Result.Data.size() == 0)
		{
			ActorInfo.insert({ Gyml, { BfresLibrary::GetModel(GetDefaultModelKey(NewActor)) }});
			Logger::Warning("ActorMgr", "Could not find the model for actor " + Gyml);
			goto ActorInfoInterpretation;
		}

		SarcFile ActorPack(Result.Data);

		std::string UMiiParamPath = "";
		for (SarcFile::Entry& Entry : ActorPack.GetEntries())
		{
			if (Entry.Name.starts_with("Component/UMiiParam/"))
			{
				UMiiParamPath = Entry.Name;
				break;
			}
		}

		bool IsNpc = UMiiParamPath.length() > 0;

		if (IsNpc)
		{
			Logger::Info("ActorMgr", "Decoding UMii " + Gyml);
			ActorInfo.insert({ Gyml, { BfresLibrary::GetModel(GetDefaultModelKey(NewActor)), true, UMii(ActorPack.GetEntry(UMiiParamPath).Bytes) } });
			//BfresFile* Model = BfresLibrary::GetModel("UMii_" + NPC.Race + "_Body" + NPC.Type + "_" + NPC.SexAge + "_" + NPC.Number + "." + "UMii_" + NPC.Race + "_Body" + NPC.Type + "_" + NPC.SexAge + "_" + NPC.Number);

			goto ActorInfoInterpretation;
		}

		std::string ModelInfoPath = "";
		for (SarcFile::Entry& Entry : ActorPack.GetEntries())
		{
			if (Entry.Name.rfind("Component/ModelInfo/", 0) == 0)
			{
				ModelInfoPath = Entry.Name;
				break;
			}
		}
		if (ModelInfoPath.length() == 0)
		{
			ActorInfo.insert({ Gyml, { BfresLibrary::GetModel(GetDefaultModelKey(NewActor)) } });
			Logger::Warning("ActorMgr", "Could not find the model for actor " + Gyml);
			goto ActorInfoInterpretation;
		}

		BymlFile ModelInfo(ActorPack.GetEntry(ModelInfoPath).Bytes);
		if (ModelInfo.GetNodes().size() == 0)
		{
			ActorInfo.insert({ Gyml, { BfresLibrary::GetModel(GetDefaultModelKey(NewActor)) } });
			Logger::Warning("ActorMgr", "Could not find the model for actor " + Gyml + ". (No ModelInfo found)");
			goto ActorInfoInterpretation;
		}

		if (!ModelInfo.HasChild("ModelProjectName") || !ModelInfo.HasChild("FmdbName"))
		{
			ActorInfo.insert({ Gyml, { BfresLibrary::GetModel(GetDefaultModelKey(NewActor)) } });
			Logger::Warning("ActorMgr", "Could not find the model for actor " + Gyml + ". (No ModelProjectName or FmdbName found)");
			goto ActorInfoInterpretation;
		}

		if (ModelInfo.GetNode("ModelProjectName")->GetValue<std::string>().empty() || ModelInfo.GetNode("FmdbName")->GetValue<std::string>().empty())
		{
			ActorInfo.insert({ Gyml, { BfresLibrary::GetModel(GetDefaultModelKey(NewActor)) } });
			Logger::Warning("ActorMgr", "Could not find the model for actor " + Gyml + ". (ModelProjectName or FmdbName were empty)");
			goto ActorInfoInterpretation;
		}

		BfresFile* Model = BfresLibrary::GetModel(ModelInfo.GetNode("ModelProjectName")->GetValue<std::string>() + "." + ModelInfo.GetNode("FmdbName")->GetValue<std::string>());

		if (Model->GetModels()[0].LODs.empty())
		{
			Model = BfresLibrary::GetModel(GetDefaultModelKey(NewActor));
			BfresLibrary::Models[ModelInfo.GetNode("ModelProjectName")->GetValue<std::string>() + "." + ModelInfo.GetNode("FmdbName")->GetValue<std::string>()] = *Model;
		}

		ActorInfo.insert({ Gyml, { Model } });
	}

	ActorInfoInterpretation:
	NewActor.Model = ActorInfo[Gyml].Model;
	if (ActorInfo[NewActor.Gyml].IsUMii)
	{
		NewActor.IsUMii = true;
		NewActor.UMiiData = ActorInfo[NewActor.Gyml].UMiiData;
	}

	Actors.push_back(NewActor);

	if (UpdateOrder)
		UpdateModelOrder();

	return Actors[Actors.size() - 1];
}

Actor ActorMgr::CreateBasicActor(std::string Gyml)
{
	Actor NewActor;
	NewActor.Gyml = Gyml;

	HashMgr::Hash Hash = HashMgr::GetHash(false); //TODO: Check if actor needs physics, research in actor pack
	NewActor.Hash = Hash.Hash;
	NewActor.SRTHash = Hash.SRTHash;

	//Looking for a model
	if (!ActorInfo.count(Gyml))
	{
		ZStdFile::Result Result = ZStdFile::Decompress(Editor::GetRomFSFile("Pack/Actor/" + Gyml + ".pack.zs"), ZStdFile::Dictionary::Pack);
		if (Result.Data.size() == 0)
		{
			ActorInfo.insert({ Gyml, { BfresLibrary::GetModel(GetDefaultModelKey(NewActor)) } });
			Logger::Warning("ActorMgr", "Could not find the model for actor " + Gyml);
			goto ActorInfoInterpretation;
		}

		SarcFile ActorPack(Result.Data);
		std::string ModelInfoPath = "";
		for (SarcFile::Entry& Entry : ActorPack.GetEntries())
		{
			if (Entry.Name.rfind("Component/ModelInfo/", 0) == 0)
			{
				ModelInfoPath = Entry.Name;
				break;
			}
		}
		if (ModelInfoPath.length() == 0)
		{
			ActorInfo.insert({ Gyml, { BfresLibrary::GetModel(GetDefaultModelKey(NewActor)) } });
			Logger::Warning("ActorMgr", "Could not find the model for actor " + Gyml);
			goto ActorInfoInterpretation;
		}

		BymlFile ModelInfo(ActorPack.GetEntry(ModelInfoPath).Bytes);
		if (ModelInfo.GetNodes().size() == 0)
		{
			ActorInfo.insert({ Gyml, { BfresLibrary::GetModel(GetDefaultModelKey(NewActor)) } });
			Logger::Warning("ActorMgr", "Could not find the model for actor " + Gyml);
			goto ActorInfoInterpretation;
		}

		if (!ModelInfo.HasChild("ModelProjectName") || !ModelInfo.HasChild("FmdbName"))
		{
			ActorInfo.insert({ Gyml, { BfresLibrary::GetModel(GetDefaultModelKey(NewActor)) } });
			Logger::Warning("ActorMgr", "Could not find the model for actor " + Gyml);
			goto ActorInfoInterpretation;
		}

		BfresFile* Model = BfresLibrary::GetModel(ModelInfo.GetNode("ModelProjectName")->GetValue<std::string>() + "." + ModelInfo.GetNode("FmdbName")->GetValue<std::string>());

		if (Model->GetModels()[0].LODs.empty())
		{
			Model = BfresLibrary::GetModel(GetDefaultModelKey(NewActor));
			BfresLibrary::Models[ModelInfo.GetNode("ModelProjectName")->GetValue<std::string>() + "." + ModelInfo.GetNode("FmdbName")->GetValue<std::string>()] = *Model;
		}

		ActorInfo.insert({ Gyml, { Model } });
	}

ActorInfoInterpretation:
	NewActor.Model = ActorInfo[Gyml].Model;

	return NewActor;
}

void ActorMgr::UpdateModel(Actor* ModelActor)
{
	//Looking for a model
	if (!ActorInfo.count(ModelActor->Gyml))
	{
		ZStdFile::Result Result = ZStdFile::Decompress(Editor::GetRomFSFile("Pack/Actor/" + ModelActor->Gyml + ".pack.zs"), ZStdFile::Dictionary::Pack);
		if (Result.Data.size() == 0)
		{
			ActorInfo.insert({ ModelActor->Gyml, { BfresLibrary::GetModel(GetDefaultModelKey(*ModelActor)) } });
			Logger::Warning("ActorMgr", "Could not find the model for actor " + ModelActor->Gyml);
			goto ActorInfoInterpretation;
		}

		SarcFile ActorPack(Result.Data);

		std::string UMiiParamPath = "";
		for (SarcFile::Entry& Entry : ActorPack.GetEntries())
		{
			if (Entry.Name.starts_with("Component/UMiiParam/"))
			{
				UMiiParamPath = Entry.Name;
				break;
			}
		}

		bool IsNpc = UMiiParamPath.length() > 0;

		if (IsNpc)
		{
			Logger::Info("ActorMgr", "Decoding UMii " + ModelActor->Gyml);
			ActorInfo.insert({ ModelActor->Gyml, { BfresLibrary::GetModel(GetDefaultModelKey(*ModelActor)), true, UMii(ActorPack.GetEntry(UMiiParamPath).Bytes) } });
			//BfresFile* Model = BfresLibrary::GetModel("UMii_" + NPC.Race + "_Body" + NPC.Type + "_" + NPC.SexAge + "_" + NPC.Number + "." + "UMii_" + NPC.Race + "_Body" + NPC.Type + "_" + NPC.SexAge + "_" + NPC.Number);

			goto ActorInfoInterpretation;
		}

		std::string ModelInfoPath = "";
		for (SarcFile::Entry& Entry : ActorPack.GetEntries())
		{
			if (Entry.Name.rfind("Component/ModelInfo/", 0) == 0)
			{
				ModelInfoPath = Entry.Name;
				break;
			}
		}
		if (ModelInfoPath.length() == 0)
		{
			ActorInfo.insert({ ModelActor->Gyml, { BfresLibrary::GetModel(GetDefaultModelKey(*ModelActor)) } });
			Logger::Warning("ActorMgr", "Could not find the model for actor " + ModelActor->Gyml);
			goto ActorInfoInterpretation;
		}

		BymlFile ModelInfo(ActorPack.GetEntry(ModelInfoPath).Bytes);
		if (ModelInfo.GetNodes().size() == 0)
		{
			ActorInfo.insert({ ModelActor->Gyml, { BfresLibrary::GetModel(GetDefaultModelKey(*ModelActor)) } });
			Logger::Warning("ActorMgr", "Could not find the model for actor " + ModelActor->Gyml);
			goto ActorInfoInterpretation;
		}

		if (!ModelInfo.HasChild("ModelProjectName") || !ModelInfo.HasChild("FmdbName"))
		{
			ActorInfo.insert({ ModelActor->Gyml, { BfresLibrary::GetModel(GetDefaultModelKey(*ModelActor)) } });
			Logger::Warning("ActorMgr", "Could not find the model for actor " + ModelActor->Gyml);
			goto ActorInfoInterpretation;
		}

		BfresFile* Model = BfresLibrary::GetModel(ModelInfo.GetNode("ModelProjectName")->GetValue<std::string>() + "." + ModelInfo.GetNode("FmdbName")->GetValue<std::string>());

		if (Model->GetModels()[0].LODs.empty())
		{
			Model = BfresLibrary::GetModel(GetDefaultModelKey(*ModelActor));
			BfresLibrary::Models[ModelInfo.GetNode("ModelProjectName")->GetValue<std::string>() + "." + ModelInfo.GetNode("FmdbName")->GetValue<std::string>()] = *Model;
		}

		ActorInfo.insert({ ModelActor->Gyml, { Model } });
	}

ActorInfoInterpretation:
	ModelActor->Model = ActorInfo[ModelActor->Gyml].Model;
	if (ActorInfo[ModelActor->Gyml].IsUMii)
	{
		ModelActor->IsUMii = true;
		ModelActor->UMiiData = ActorInfo[ModelActor->Gyml].UMiiData;
	}
}

void ActorMgr::UpdateMergedActorContent(Actor* Parent, Actor OldActor)
{
	float TranslateXDifference = Parent->Translate.GetX() - OldActor.Translate.GetX();
	float TranslateYDifference = Parent->Translate.GetY() - OldActor.Translate.GetY();
	float TranslateZDifference = Parent->Translate.GetZ() - OldActor.Translate.GetZ();

	float RotateXDifference = Parent->Rotate.GetX() - OldActor.Rotate.GetX();
	float RotateYDifference = Parent->Rotate.GetY() - OldActor.Rotate.GetY();
	float RotateZDifference = Parent->Rotate.GetZ() - OldActor.Rotate.GetZ();

	for (Actor& Child : Parent->MergedActorContent)
	{
		float NewTranslateX = Parent->Translate.GetX() + (Child.Translate.GetX() - Parent->Translate.GetX()) * (Parent->Scale.GetX() / OldActor.Scale.GetX());
		float NewTranslateY = Parent->Translate.GetY() + (Child.Translate.GetY() - Parent->Translate.GetY()) * (Parent->Scale.GetY() / OldActor.Scale.GetY());
		float NewTranslateZ = Parent->Translate.GetZ() + (Child.Translate.GetZ() - Parent->Translate.GetZ()) * (Parent->Scale.GetZ() / OldActor.Scale.GetZ());

		Child.Translate.SetX(NewTranslateX + TranslateXDifference);
		Child.Translate.SetY(NewTranslateY + TranslateYDifference);
		Child.Translate.SetZ(NewTranslateZ + TranslateZDifference);

		Child.Scale.SetX(Child.Scale.GetX() + (Parent->Scale.GetX() - OldActor.Scale.GetX()));
		Child.Scale.SetY(Child.Scale.GetY() + (Parent->Scale.GetY() - OldActor.Scale.GetY()));
		Child.Scale.SetZ(Child.Scale.GetZ() + (Parent->Scale.GetZ() - OldActor.Scale.GetZ()));

		/*
		//Rotate
		Child.Rotate.SetX(Child.Rotate.GetX() + RotateXDifference);
		Child.Rotate.SetY(Child.Rotate.GetY() + RotateYDifference);
		Child.Rotate.SetZ(Child.Rotate.GetZ() + RotateZDifference);

		
		Vector3F RadianRotate(Util::DegreesToRadians(RotateXDifference), Util::DegreesToRadians(RotateYDifference), Util::DegreesToRadians(RotateZDifference));
	

		float NewX = Parent->Translate.GetX() + ((Child.Translate.GetX() - Parent->Translate.GetX()) * (std::cosf(RadianRotate.GetZ()) * std::cosf(RadianRotate.GetY()))) +
			((Child.Translate.GetY() - Parent->Translate.GetY()) * (std::cosf(RadianRotate.GetZ()) * std::sinf(RadianRotate.GetY()) * std::sinf(RadianRotate.GetX()) - std::sinf(RadianRotate.GetZ()) * std::cosf(RadianRotate.GetX()))) +
			((Child.Translate.GetZ() - Parent->Translate.GetZ()) * (std::cosf(RadianRotate.GetZ()) * std::sinf(RadianRotate.GetY()) * std::cosf(RadianRotate.GetX()) + std::sinf(RadianRotate.GetZ()) * std::sinf(RadianRotate.GetX())));

		float NewY = Parent->Translate.GetY() + ((Child.Translate.GetX() - Parent->Translate.GetX()) * (std::sinf(RadianRotate.GetZ()) * std::cosf(RadianRotate.GetY()))) +
			((Child.Translate.GetY() - Parent->Translate.GetY()) * (std::sinf(RadianRotate.GetZ()) * std::sinf(RadianRotate.GetY()) * std::sinf(RadianRotate.GetX()) + std::cosf(RadianRotate.GetZ()) * std::cosf(RadianRotate.GetX()))) +
			((Child.Translate.GetZ() - Parent->Translate.GetZ()) * (std::sinf(RadianRotate.GetZ()) * std::sinf(RadianRotate.GetY()) * std::cosf(RadianRotate.GetX()) - std::cosf(RadianRotate.GetZ()) * std::sinf(RadianRotate.GetX())));

		float NewZ = Parent->Translate.GetZ() + ((Child.Translate.GetX() - Parent->Translate.GetX()) * (-std::sinf(RadianRotate.GetY()))) +
			((Child.Translate.GetY() - Parent->Translate.GetY()) * (std::cosf(RadianRotate.GetY()) * std::sinf(RadianRotate.GetX()))) +
			((Child.Translate.GetZ() - Parent->Translate.GetZ()) * (std::cosf(RadianRotate.GetY()) * std::cosf(RadianRotate.GetX())));

		Child.Translate.SetX(NewX);
		Child.Translate.SetY(NewY);
		Child.Translate.SetZ(NewZ);
		*/

		/*
		float NewX = std::cosf(RotateZDifference) * Child.Translate.GetX() - std::sinf(RotateZDifference) * Child.Translate.GetY();
		float NewY = std::sinf(RotateZDifference) * Child.Translate.GetX() + std::cosf(RotateZDifference) * Child.Translate.GetY();

		NewX = std::cosf(RotateYDifference) * NewX + std::sinf(RotateYDifference) * Child.Translate.GetZ();
		float NewZ = (-std::sinf(RotateYDifference)) * NewX + std::cosf(RotateYDifference) * Child.Translate.GetZ();

		NewY = std::cosf(RotateXDifference) * NewY - std::sinf(RotateXDifference) * NewZ;
		NewZ = std::sinf(RotateXDifference) * NewY + std::cosf(RotateXDifference) * NewZ;

		Child.Translate.SetX(NewX);
		Child.Translate.SetY(NewY);
		Child.Translate.SetZ(NewZ);
		*/

		//Hell
		//https://en.wikipedia.org/wiki/Rotation_matrix
	}
}

void ActorMgr::UpdateModelOrder()
{
	OpaqueActors.clear();
	TransparentActors.clear();
	UMiiActors.clear();

	std::vector<Actor*> MergedActorContents;

	for (Actor& CurrentActor : Actors)
	{
		if (CurrentActor.IsUMii)
		{
			UMiiActors.push_back(&CurrentActor);
			continue;
		}

		for (Actor& Child : CurrentActor.MergedActorContent)
		{
			Child.MergedActorParent = &CurrentActor;
			MergedActorContents.push_back(&Child);
		}

		if (CurrentActor.Model->GetModels()[0].LODs[0].TransparentObjects.empty())
		{
			if (OpaqueActors.count(CurrentActor.Model))
			{
				OpaqueActors[CurrentActor.Model].push_back(&CurrentActor);
				continue;
			}
			std::vector<Actor*> ActorVec;
			ActorVec.push_back(&CurrentActor);
			OpaqueActors.insert({ CurrentActor.Model, ActorVec });
		}
		else
		{
			if (TransparentActors.count(CurrentActor.Model))
			{
				TransparentActors[CurrentActor.Model].push_back(&CurrentActor);
				continue;
			}
			std::vector<Actor*> ActorVec;
			ActorVec.push_back(&CurrentActor);
			TransparentActors.insert({ CurrentActor.Model, ActorVec });
		}
	}

	for (Actor* CurrentActor : MergedActorContents)
	{
		if (CurrentActor->IsUMii)
		{
			UMiiActors.push_back(CurrentActor);
			continue;
		}

		if (CurrentActor->Model->GetModels()[0].LODs[0].TransparentObjects.empty())
		{
			if (OpaqueActors.count(CurrentActor->Model))
			{
				OpaqueActors[CurrentActor->Model].push_back(CurrentActor);
				continue;
			}
			std::vector<Actor*> ActorVec;
			ActorVec.push_back(CurrentActor);
			OpaqueActors.insert({ CurrentActor->Model, ActorVec });
		}
		else
		{
			if (TransparentActors.count(CurrentActor->Model))
			{
				TransparentActors[CurrentActor->Model].push_back(CurrentActor);
				continue;
			}
			std::vector<Actor*> ActorVec;
			ActorVec.push_back(CurrentActor);
			TransparentActors.insert({ CurrentActor->Model, ActorVec });
		}
	}

	Logger::Info("ActorMgr", "Updated model order");
}

Actor* ActorMgr::GetActor(uint64_t Hash, uint32_t SRTHash)
{
	for (Actor& Actor : Actors)
	{
		if (Actor.Hash == Hash && Actor.SRTHash == SRTHash)
			return &Actor;
	}
	return nullptr;
}

Actor* ActorMgr::GetActor(std::string Gyml)
{
	for (Actor& Actor : Actors)
	{
		if (Actor.Gyml == Gyml)
			return &Actor;
	}
	return nullptr;
}

void ActorMgr::DeleteActor(uint64_t Hash, uint32_t SRTHash)
{
	Actor* DelActor = nullptr;

	for (Actor& Actor : Actors)
	{
		if (Actor.Hash == Hash && Actor.SRTHash == SRTHash)
		{
			DelActor = &Actor;
			break;
		}
	}

	if (DelActor == nullptr)
	{
		for (Actor& ActorMerged : Actors)
		{
			for (Actor& Child : ActorMerged.MergedActorContent)
			{
				if (Child.Hash == Hash && Child.SRTHash == SRTHash)
				{
					ActorMerged.MergedActorContent.erase(
						std::remove_if(ActorMerged.MergedActorContent.begin(), ActorMerged.MergedActorContent.end(), [&](Actor const& Actor) {
							return Actor.Gyml == Child.Gyml && Actor.Hash == Child.Hash && Actor.SRTHash == Child.SRTHash;
							}),
						ActorMerged.MergedActorContent.end());

					UpdateModelOrder();

					return;
				}
			}
		}

		Logger::Error("ActorMgr", "Actor to delete was nullptr, probably wrong Hash/SRTHash");
		return;
	}

	Actors.erase(
		std::remove_if(Actors.begin(), Actors.end(), [&](Actor const& Actor) {
			return Actor.Gyml == DelActor->Gyml && Actor.Hash == DelActor->Hash && Actor.SRTHash == DelActor->SRTHash;
			}),
		Actors.end());
	
	UpdateModelOrder();
}

std::vector<Actor>& ActorMgr::GetActors()
{
	return Actors;
}