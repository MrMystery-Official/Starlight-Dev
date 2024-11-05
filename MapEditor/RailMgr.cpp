#include "RailMgr.h"

#include "Editor.h"
#include "Logger.h"

std::vector<RailMgr::Rail> RailMgr::mRails;

void RailMgr::LoadRailsForCurrentScene()
{
	mRails.clear();

	if (!Editor::StaticActorsByml.HasChild("Rails"))
		return;

	BymlFile::Node* RailsArray = Editor::StaticActorsByml.GetNode("Rails");
	
	for (BymlFile::Node& RailNode : RailsArray->GetChildren())
	{
		RailMgr::Rail NewRail;

		BymlFile::Node* GymlNode = RailNode.GetChild("Gyaml");
		BymlFile::Node* HashNode = RailNode.GetChild("Hash");
		BymlFile::Node* IsClosedNode = RailNode.GetChild("IsClosed");
		BymlFile::Node* NameNode = RailNode.GetChild("Name");

		BymlFile::Node* PointsNode = RailNode.GetChild("Points");

		if (GymlNode)
			NewRail.mGyml = GymlNode->GetValue<std::string>();

		if (HashNode)
			NewRail.mHash = HashNode->GetValue<uint64_t>();

		if (IsClosedNode)
			NewRail.mIsClosed = IsClosedNode->GetValue<bool>();

		if (NameNode)
			NewRail.mName = NameNode->GetValue<std::string>();

		if (PointsNode)
		{
			for (BymlFile::Node& PointNode : PointsNode->GetChildren())
			{
				RailMgr::Rail::Point NewPoint;

				BymlFile::Node* PointGymlNode = PointNode.GetChild("Gyaml");
				BymlFile::Node* PointHashNode = PointNode.GetChild("Hash");
				BymlFile::Node* PointNextDistanceNode = PointNode.GetChild("NextDistance");
				BymlFile::Node* PointPrevDistanceNode = PointNode.GetChild("PrevDistance");
				BymlFile::Node* PointTranslateNode = PointNode.GetChild("Translate");
				BymlFile::Node* PointDynamicNode = PointNode.GetChild("Dynamic");

				if (PointGymlNode)
					NewPoint.mGyml = PointGymlNode->GetValue<std::string>();

				if (PointHashNode)
					NewPoint.mHash = PointHashNode->GetValue<uint64_t>();

				if (PointNextDistanceNode)
					NewPoint.mNextDistance = PointNextDistanceNode->GetValue<float>();

				if (PointPrevDistanceNode)
					NewPoint.mPrevDistance = PointPrevDistanceNode->GetValue<float>();

				if (PointTranslateNode)
					NewPoint.mTranslate = glm::vec3(PointTranslateNode->GetChild(0)->GetValue<float>(), PointTranslateNode->GetChild(1)->GetValue<float>(), PointTranslateNode->GetChild(2)->GetValue<float>());

				if (PointDynamicNode)
				{
					for (BymlFile::Node& DynamicNode : PointDynamicNode->GetChildren())
					{
						RailMgr::Rail::Point::Dynamic Dynamic;
						Dynamic.mType = DynamicNode.GetType();

						glm::vec3 Vec;

						switch (DynamicNode.GetType())
						{
						case BymlFile::Type::StringIndex:
							Dynamic.mData = DynamicNode.GetValue<std::string>();
							break;
						case BymlFile::Type::Bool:
							Dynamic.mData = DynamicNode.GetValue<bool>();
							break;
						case BymlFile::Type::Int32:
							Dynamic.mData = DynamicNode.GetValue<int32_t>();
							break;
						case BymlFile::Type::Int64:
							Dynamic.mData = DynamicNode.GetValue<int64_t>();
							break;
						case BymlFile::Type::UInt32:
							Dynamic.mData = DynamicNode.GetValue<uint32_t>();
							break;
						case BymlFile::Type::UInt64:
							Dynamic.mData = DynamicNode.GetValue<uint64_t>();
							break;
						case BymlFile::Type::Float:
							Dynamic.mData = DynamicNode.GetValue<float>();
							break;
						case BymlFile::Type::Array:
							Vec.x = DynamicNode.GetChild(0)->GetValue<float>();
							Vec.y = DynamicNode.GetChild(1)->GetValue<float>();
							Vec.z = DynamicNode.GetChild(2)->GetValue<float>();
							Dynamic.mData = Vec;
							break;
						default:
							Logger::Error("RailMgr", "Invalid dynamic data type");
							break;
						}
						NewPoint.mDynamic.insert({ DynamicNode.GetKey(), Dynamic });
					}
				}

				NewRail.mPoints.push_back(NewPoint);
			}
		}

		RailMgr::mRails.push_back(NewRail);
	}

	Logger::Info("RailMgr", "Loaded " + std::to_string(mRails.size()) + " rails");
}

BymlFile::Node RailMgr::RailToNode(RailMgr::Rail& Rail)
{
	//Recalculating distances
	if (Rail.mPoints.size() > 1)
	{
		for (size_t i = 0; i < Rail.mPoints.size(); i++)
		{
			if (i != (Rail.mPoints.size() - 1))
			{
				Rail.mPoints[i].mNextDistance = glm::distance(Rail.mPoints[i].mTranslate, Rail.mPoints[i + 1].mTranslate);
			}
			if (i > 0)
			{
				Rail.mPoints[i].mPrevDistance = glm::distance(Rail.mPoints[i - 1].mTranslate, Rail.mPoints[i].mTranslate);
			}
		}
	}
	else
	{
		Rail.mPoints[0].mNextDistance = FLT_MAX;
		Rail.mPoints[0].mPrevDistance = FLT_MAX;
	}

	//Writing byml data
	BymlFile::Node Root(BymlFile::Type::Dictionary);

	{
		BymlFile::Node GymlNode(BymlFile::Type::StringIndex, "Gyaml");
		GymlNode.SetValue<std::string>(Rail.mGyml);
		Root.AddChild(GymlNode);
	}


	if (!Rail.mName.empty())
	{
		BymlFile::Node NameNode(BymlFile::Type::StringIndex, "Name");
		NameNode.SetValue<std::string>(Rail.mName);
		Root.AddChild(NameNode);
	}

	{
		BymlFile::Node HashNode(BymlFile::Type::UInt64, "Hash");
		HashNode.SetValue<uint64_t>(Rail.mHash);
		Root.AddChild(HashNode);
	}

	{
		BymlFile::Node IsClosedNode(BymlFile::Type::Bool, "IsClosed");
		IsClosedNode.SetValue<bool>(Rail.mIsClosed);
		Root.AddChild(IsClosedNode);
	}

	if (!Rail.mPoints.empty())
	{
		BymlFile::Node PointsNode(BymlFile::Type::Array, "Points");

		for (const RailMgr::Rail::Point& Point : Rail.mPoints)
		{
			BymlFile::Node PointNode(BymlFile::Type::Dictionary);

			{
				BymlFile::Node GymlNode(BymlFile::Type::StringIndex, "Gyaml");
				GymlNode.SetValue<std::string>(Point.mGyml);
				PointNode.AddChild(GymlNode);
			}

			{
				BymlFile::Node HashNode(BymlFile::Type::UInt64, "Gyaml");
				HashNode.SetValue<uint64_t>(Point.mHash);
				PointNode.AddChild(HashNode);
			}

			if(Point.mNextDistance != FLT_MAX)
			{
				BymlFile::Node DistanceNode(BymlFile::Type::Float, "NextDistance");
				DistanceNode.SetValue<float>(Point.mNextDistance);
				PointNode.AddChild(DistanceNode);
			}

			if (Point.mPrevDistance != FLT_MAX)
			{
				BymlFile::Node DistanceNode(BymlFile::Type::Float, "PrevDistance");
				DistanceNode.SetValue<float>(Point.mPrevDistance);
				PointNode.AddChild(DistanceNode);
			}

			{
				BymlFile::Node VectorRoot(BymlFile::Type::Array, "Translate");

				BymlFile::Node VectorX(BymlFile::Type::Float, "0");
				VectorX.SetValue<float>(Point.mTranslate.x);
				VectorRoot.AddChild(VectorX);

				BymlFile::Node VectorY(BymlFile::Type::Float, "1");
				VectorY.SetValue<float>(Point.mTranslate.y);
				VectorRoot.AddChild(VectorY);

				BymlFile::Node VectorZ(BymlFile::Type::Float, "2");
				VectorZ.SetValue<float>(Point.mTranslate.z);
				VectorRoot.AddChild(VectorZ);

				PointNode.AddChild(VectorRoot);
			}

			if (!Point.mDynamic.empty())
			{
				BymlFile::Node Dynamic(BymlFile::Type::Dictionary, "Dynamic");
				for (auto const& [Key, Value] : Point.mDynamic)
				{
					if (Value.mType != BymlFile::Type::Array)
					{
						BymlFile::Node DynamicEntry(Value.mType, Key);

						switch (DynamicEntry.GetType())
						{
						case BymlFile::Type::StringIndex:
							DynamicEntry.SetValue<std::string>(std::get<std::string>(Value.mData));
							break;
						case BymlFile::Type::Float:
							DynamicEntry.SetValue<float>(std::get<float>(Value.mData));
							break;
						case BymlFile::Type::UInt32:
							DynamicEntry.SetValue<uint32_t>(std::get<uint32_t>(Value.mData));
							break;
						case BymlFile::Type::UInt64:
							DynamicEntry.SetValue<uint64_t>(std::get<uint64_t>(Value.mData));
							break;
						case BymlFile::Type::Int32:
							DynamicEntry.SetValue<int32_t>(std::get<int32_t>(Value.mData));
							break;
						case BymlFile::Type::Int64:
							DynamicEntry.SetValue<int64_t>(std::get<int64_t>(Value.mData));
							break;
						case BymlFile::Type::Bool:
							DynamicEntry.SetValue<bool>(std::get<bool>(Value.mData));
							break;
						}
						Dynamic.AddChild(DynamicEntry);
					}
					else
					{
						glm::vec3 Vec = std::get<glm::vec3>(Value.mData);
						BymlFile::Node VectorRoot(BymlFile::Type::Array, Key);

						BymlFile::Node VectorX(BymlFile::Type::Float, "0");
						VectorX.SetValue<float>(Vec.x);
						VectorRoot.AddChild(VectorX);

						BymlFile::Node VectorY(BymlFile::Type::Float, "1");
						VectorY.SetValue<float>(Vec.y);
						VectorRoot.AddChild(VectorY);

						BymlFile::Node VectorZ(BymlFile::Type::Float, "2");
						VectorZ.SetValue<float>(Vec.z);
						VectorRoot.AddChild(VectorZ);

						Dynamic.AddChild(VectorRoot);
					}
				}
				PointNode.AddChild(Dynamic);
			}

			PointsNode.AddChild(PointNode);
		}

		Root.AddChild(PointsNode);
	}

	return Root;
}