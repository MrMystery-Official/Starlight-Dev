#include "SceneMgr.h"

#include "Byml.h"
#include "ZStdFile.h"
#include "Editor.h"
#include "ActorMgr.h"
#include "HashMgr.h"
#include "Logger.h"
#include "UIOutliner.h"
#include "Util.h"
#include "UIMapView.h"
#include "GameDataListMgr.h"

SceneMgr::Type SceneMgr::SceneType = SceneMgr::Type::SmallDungeon;
Mesh SceneMgr::NavigationMeshModel;
std::vector<PhiveStaticCompound> SceneMgr::mStaticCompounds;

Mesh SceneMgr::LoadNavMeshBfres(PhiveNavMesh& NavMesh)
{
	std::pair<std::vector<float>, std::vector<uint32_t>> ModelData = NavMesh.ToVerticesIndices();
	return Mesh(ModelData.first, ModelData.second);
}

PhiveStaticCompound* SceneMgr::GetStaticCompoundForPos(glm::vec3 Pos)
{
	if (mStaticCompounds.empty())
		return nullptr;

	switch (SceneType)
	{
	case SceneMgr::Type::MainField:
	case SceneMgr::Type::MinusField:
	{
		int32_t MapUnitDistanceX = 0;
		int32_t MapUnitDistanceZ = 0;

		char MapUnitIdentifierX = Editor::Identifier[0];
		uint8_t MapUnitIdentifierZ = Editor::Identifier[2] - 48; //48 = '0'

		MapUnitDistanceZ = (5 - MapUnitIdentifierZ) * 1000;
		switch (MapUnitIdentifierX)
		{
		case 'A':
			MapUnitDistanceX = 5000;
			break;
		case 'B':
			MapUnitDistanceX = 4000;
			break;
		case 'C':
			MapUnitDistanceX = 3000;
			break;
		case 'D':
			MapUnitDistanceX = 2000;
			break;
		case 'E':
			MapUnitDistanceX = 1000;
			break;
		case 'F':
			MapUnitDistanceX = 0;
			break;
		case 'G':
			MapUnitDistanceX = -1000;
			break;
		case 'H':
			MapUnitDistanceX = -2000;
			break;
		case 'I':
			MapUnitDistanceX = -3000;
			break;
		case 'J':
			MapUnitDistanceX = -4000;
			break;
		default:
			break;
		}

		int8_t MapUnitDistanceZMultiplier = MapUnitIdentifierZ > 5 ? -1 : 1;

		Pos = Pos + glm::vec3(MapUnitDistanceX, 0, MapUnitDistanceZ * MapUnitDistanceZMultiplier);

		uint8_t StaticCompoundIndexX = 0;
		uint8_t StaticCompoundIndexZ = 0;

		if (Pos.x < 0)
		{
			StaticCompoundIndexX = 0;
		}
		else if (Pos.x > 250 && Pos.x <= 500)
		{
			StaticCompoundIndexX = 1;
		}
		else if (Pos.x > 500 && Pos.x <= 750)
		{
			StaticCompoundIndexX = 2;
		}
		else if(Pos.x > 750)
		{
			StaticCompoundIndexX = 3;
		}

		if (Pos.z < 0)
		{
			StaticCompoundIndexX = 0;
		}
		else if (Pos.z > 250 && Pos.z <= 500)
		{
			StaticCompoundIndexZ = 1;
		}
		else if (Pos.z > 500 && Pos.z <= 750)
		{
			StaticCompoundIndexZ = 2;
		}
		else if(Pos.z > 750)
		{
			StaticCompoundIndexZ = 3;
		}

		return &mStaticCompounds[StaticCompoundIndexX + 4 * StaticCompoundIndexZ];
	}
	case SceneMgr::Type::SkyIslands:
	case SceneMgr::Type::LargeDungeon:
	case SceneMgr::Type::SmallDungeon:
	case SceneMgr::Type::NormalStage:
		return &mStaticCompounds[0];
	default:
		return nullptr;
	}
}

void SceneMgr::LoadScene(SceneMgr::Type Type, std::string Identifier)
{
	SceneType = Type;

	std::string BancPrefix = "Banc/";
	switch (Type)
	{
	case SceneMgr::Type::SmallDungeon:
		BancPrefix += "SmallDungeon/Dungeon";
		Editor::BancDir = "Banc/SmallDungeon";
		break;
	case SceneMgr::Type::MainField:
		BancPrefix += "MainField/";
		Editor::BancDir = "Banc/MainField";
		break;
	case SceneMgr::Type::MinusField:
		BancPrefix += "MinusField/";
		Editor::BancDir = "Banc/MinusField";
		break;
	case SceneMgr::Type::LargeDungeon:
	{
		if (Identifier != "LargeDungeonFire")
		{
			BancPrefix += "MainField/LargeDungeon/";
			Editor::BancDir = "Banc/MainField/LargeDungeon";
		}
		else
		{
			BancPrefix += "MinusField/LargeDungeon/";
			Editor::BancDir = "Banc/MinusField/LargeDungeon";
		}
		break;
	}
	case SceneMgr::Type::NormalStage:
		BancPrefix += "NormalStage/";
		Editor::BancDir = "Banc/NormalStage";
		break;
	case SceneMgr::Type::SkyIslands:
		BancPrefix += "MainField/Sky/";
		Editor::BancDir = "Banc/MainField/Sky";
		break;
	case SceneMgr::Type::Custom:
		size_t Found = Identifier.find_last_of("/\\");
		Editor::BancDir = "Banc/" + Identifier.substr(0, Found);
		break;
	}

	Logger::Info("SceneMgr", "Loading map " + Identifier + " of type " + std::to_string((int)Type));

	BymlFile DynamicActorsByml(ZStdFile::Decompress(Editor::GetRomFSFile(BancPrefix + Identifier + "_Dynamic.bcett.byml.zs"), ZStdFile::Dictionary::BcettByaml).Data);
	BymlFile StaticActorsByml(ZStdFile::Decompress(Editor::GetRomFSFile(BancPrefix + Identifier + "_Static.bcett.byml.zs"), ZStdFile::Dictionary::BcettByaml).Data);

	ActorMgr::GetActors().clear();
	ActorMgr::OpaqueActors.clear();
	ActorMgr::TransparentActorsDiscard.clear();
	mStaticCompounds.clear();
	UIOutliner::SelectedActor = nullptr;

	Editor::DynamicActorsByml = DynamicActorsByml;
	Editor::StaticActorsByml = StaticActorsByml;
	Editor::BancPrefix = BancPrefix;
	Editor::Identifier = Identifier;

	std::string StaticCompoundDirectory = "";
	std::string StaticCompoundPrefix = "";
	switch (SceneType)
	{
	case SceneMgr::Type::SkyIslands:
		StaticCompoundDirectory = "Phive/StaticCompoundBody/MainField/Sky";
		StaticCompoundPrefix = "Phive/StaticCompoundBody/MainField/Sky/";
		break;
	case SceneMgr::Type::MainField:
		StaticCompoundDirectory = "Phive/StaticCompoundBody/MainField";
		StaticCompoundPrefix = "Phive/StaticCompoundBody/MainField/MainField_";
		break;
	case SceneMgr::Type::MinusField:
		StaticCompoundDirectory = "Phive/StaticCompoundBody/MinusField";
		StaticCompoundPrefix = "Phive/StaticCompoundBody/MinusField/MinusField_";
		break;
	case SceneMgr::Type::LargeDungeon:
		StaticCompoundDirectory = "Phive/StaticCompoundBody/" + (std::string)(Identifier == "LargeDungeonFire" ? "MinusField" : "MainField") + "/LargeDungeon";
		StaticCompoundPrefix = "Phive/StaticCompoundBody/" + (std::string)(Identifier == "LargeDungeonFire" ? "MinusField" : "MainField") + "/LargeDungeon/";
		break;
	case SceneMgr::Type::SmallDungeon:
		StaticCompoundDirectory = "Phive/StaticCompoundBody/SmallDungeon";
		StaticCompoundPrefix = "Phive/StaticCompoundBody/SmallDungeon/Dungeon";
		break;
	case SceneMgr::Type::NormalStage:
		StaticCompoundDirectory = "Phive/StaticCompoundBody/NormalStage";
		StaticCompoundPrefix = "Phive/StaticCompoundBody/NormalStage/";
		break;
	case SceneMgr::Type::Custom:
		Logger::Warning("SceneMgr", "Custom scene types do not support static compounds");
		break;
	default:
		break;
	}
	
	Editor::StaticCompoundDirectory = StaticCompoundDirectory;
	Editor::StaticCompoundPrefix = StaticCompoundPrefix;
	if (!StaticCompoundDirectory.empty())
	{
		switch (SceneType)
		{
		case SceneMgr::Type::MainField:
		case SceneMgr::Type::MinusField:
			mStaticCompounds.emplace_back(ZStdFile::Decompress(Editor::GetRomFSFile(StaticCompoundPrefix + Editor::Identifier + "_X0_Z0.Nin_NX_NVN.bphsc.zs"), ZStdFile::Dictionary::Base).Data);
			mStaticCompounds.emplace_back(ZStdFile::Decompress(Editor::GetRomFSFile(StaticCompoundPrefix + Editor::Identifier + "_X1_Z0.Nin_NX_NVN.bphsc.zs"), ZStdFile::Dictionary::Base).Data);
			mStaticCompounds.emplace_back(ZStdFile::Decompress(Editor::GetRomFSFile(StaticCompoundPrefix + Editor::Identifier + "_X2_Z0.Nin_NX_NVN.bphsc.zs"), ZStdFile::Dictionary::Base).Data);
			mStaticCompounds.emplace_back(ZStdFile::Decompress(Editor::GetRomFSFile(StaticCompoundPrefix + Editor::Identifier + "_X3_Z0.Nin_NX_NVN.bphsc.zs"), ZStdFile::Dictionary::Base).Data);
			mStaticCompounds.emplace_back(ZStdFile::Decompress(Editor::GetRomFSFile(StaticCompoundPrefix + Editor::Identifier + "_X0_Z1.Nin_NX_NVN.bphsc.zs"), ZStdFile::Dictionary::Base).Data);
			mStaticCompounds.emplace_back(ZStdFile::Decompress(Editor::GetRomFSFile(StaticCompoundPrefix + Editor::Identifier + "_X1_Z1.Nin_NX_NVN.bphsc.zs"), ZStdFile::Dictionary::Base).Data);
			mStaticCompounds.emplace_back(ZStdFile::Decompress(Editor::GetRomFSFile(StaticCompoundPrefix + Editor::Identifier + "_X2_Z1.Nin_NX_NVN.bphsc.zs"), ZStdFile::Dictionary::Base).Data);
			mStaticCompounds.emplace_back(ZStdFile::Decompress(Editor::GetRomFSFile(StaticCompoundPrefix + Editor::Identifier + "_X3_Z1.Nin_NX_NVN.bphsc.zs"), ZStdFile::Dictionary::Base).Data);
			mStaticCompounds.emplace_back(ZStdFile::Decompress(Editor::GetRomFSFile(StaticCompoundPrefix + Editor::Identifier + "_X0_Z2.Nin_NX_NVN.bphsc.zs"), ZStdFile::Dictionary::Base).Data);
			mStaticCompounds.emplace_back(ZStdFile::Decompress(Editor::GetRomFSFile(StaticCompoundPrefix + Editor::Identifier + "_X1_Z2.Nin_NX_NVN.bphsc.zs"), ZStdFile::Dictionary::Base).Data);
			mStaticCompounds.emplace_back(ZStdFile::Decompress(Editor::GetRomFSFile(StaticCompoundPrefix + Editor::Identifier + "_X2_Z2.Nin_NX_NVN.bphsc.zs"), ZStdFile::Dictionary::Base).Data);
			mStaticCompounds.emplace_back(ZStdFile::Decompress(Editor::GetRomFSFile(StaticCompoundPrefix + Editor::Identifier + "_X3_Z2.Nin_NX_NVN.bphsc.zs"), ZStdFile::Dictionary::Base).Data);
			mStaticCompounds.emplace_back(ZStdFile::Decompress(Editor::GetRomFSFile(StaticCompoundPrefix + Editor::Identifier + "_X0_Z3.Nin_NX_NVN.bphsc.zs"), ZStdFile::Dictionary::Base).Data);
			mStaticCompounds.emplace_back(ZStdFile::Decompress(Editor::GetRomFSFile(StaticCompoundPrefix + Editor::Identifier + "_X1_Z3.Nin_NX_NVN.bphsc.zs"), ZStdFile::Dictionary::Base).Data);
			mStaticCompounds.emplace_back(ZStdFile::Decompress(Editor::GetRomFSFile(StaticCompoundPrefix + Editor::Identifier + "_X2_Z3.Nin_NX_NVN.bphsc.zs"), ZStdFile::Dictionary::Base).Data);
			mStaticCompounds.emplace_back(ZStdFile::Decompress(Editor::GetRomFSFile(StaticCompoundPrefix + Editor::Identifier + "_X3_Z3.Nin_NX_NVN.bphsc.zs"), ZStdFile::Dictionary::Base).Data);
			break;
		case SceneMgr::Type::SkyIslands:
		case SceneMgr::Type::LargeDungeon:
		case SceneMgr::Type::SmallDungeon:
		case SceneMgr::Type::NormalStage:
			mStaticCompounds.emplace_back(ZStdFile::Decompress(Editor::GetRomFSFile(StaticCompoundPrefix + Editor::Identifier + ".Nin_NX_NVN.bphsc.zs"), ZStdFile::Dictionary::Base).Data);
			break;
		default:
			break;
		}
	}

	auto DecodeByml = [](BymlFile& File, Actor::Type Type)
		{
			if (!File.HasChild("Actors")) return;
			for (BymlFile::Node& ActorNode : File.GetNode("Actors")->GetChildren())
			{
				Actor* NewActor = ActorMgr::AddActorFromByml(ActorNode);
				if (NewActor == nullptr) return;

				NewActor->ActorType = Type;
			}
		};

	DecodeByml(DynamicActorsByml, Actor::Type::Dynamic);
	DecodeByml(StaticActorsByml, Actor::Type::Static);

	switch (SceneType)
	{
	case SceneMgr::Type::LargeDungeon:
	{
		std::string NavMeshIdentifier = Editor::Identifier == "LargeDungeonWater_AllinArea" ? "LargeDungeonWater" : Editor::Identifier;
		if (Util::FileExists(Editor::GetRomFSFile("Phive/NavMesh/" + NavMeshIdentifier + ".Nin_NX_NVN.bphnm.zs")))
		{
			PhiveNavMesh NavMesh(ZStdFile::Decompress(Editor::GetRomFSFile("Phive/NavMesh/" + NavMeshIdentifier + ".Nin_NX_NVN.bphnm.zs"), ZStdFile::Dictionary::Base).Data);
			NavigationMeshModel = LoadNavMeshBfres(NavMesh);
		}
		break;
	}
	case SceneMgr::Type::SmallDungeon:
	{
		if (Util::FileExists(Editor::GetRomFSFile("Phive/NavMesh/Dungeon" + Editor::Identifier + ".Nin_NX_NVN.bphnm.zs")))
		{
			PhiveNavMesh NavMesh(ZStdFile::Decompress(Editor::GetRomFSFile("Phive/NavMesh/Dungeon" + Editor::Identifier + ".Nin_NX_NVN.bphnm.zs"), ZStdFile::Dictionary::Base).Data);
			NavigationMeshModel = LoadNavMeshBfres(NavMesh);
		}
		break;
	}
	case SceneMgr::Type::NormalStage:
	{
		if (Util::FileExists(Editor::GetRomFSFile("Phive/NavMesh/" + Editor::Identifier + ".Nin_NX_NVN.bphnm.zs")))
		{
			PhiveNavMesh NavMesh(ZStdFile::Decompress(Editor::GetRomFSFile("Phive/NavMesh/" + Editor::Identifier + ".Nin_NX_NVN.bphnm.zs"), ZStdFile::Dictionary::Base).Data);
			NavigationMeshModel = LoadNavMeshBfres(NavMesh);
		}
		break;
	}
	default:
	{
		NavigationMeshModel.Delete();
		UIMapView::RenderSettings.RenderNavMesh = false;
		Logger::Warning("SceneMgr", "This scene does not support NavMesh");
	}
	}

	uint32_t WaterBoxCount = 0;
	uint32_t WaterCylinderCount = 0;
	for (PhiveStaticCompound& Compound : mStaticCompounds)
	{
		WaterBoxCount += Compound.mWaterBoxArray.size();
		WaterCylinderCount += Compound.mWaterCylinderArray.size();
	}

	if (WaterBoxCount > 0 || WaterCylinderCount > 0)
	{
		Logger::Warning("SceneMgr", std::to_string(WaterBoxCount) + " water boxes and " + std::to_string(WaterCylinderCount) + " water cylinders could not be linked to an actor. Be careful when removing a fluid type, some of them are not linked to the static compound");
	}
	
	GameDataListMgr::LoadActorGameData();
	ActorMgr::UpdateModelOrder();
	HashMgr::Initialize();

	if (!ActorMgr::GetActors().empty())
	{
		UIMapView::CameraView.Position.x = ActorMgr::GetActors()[0].Translate.GetX();
		UIMapView::CameraView.Position.y = ActorMgr::GetActors()[0].Translate.GetY();
		UIMapView::CameraView.Position.z = ActorMgr::GetActors()[0].Translate.GetZ();
	}
}

void SceneMgr::Reload()
{
	Logger::Info("SceneMgr", "Reloading map " + Editor::Identifier);

	BymlFile DynamicActorsByml(ZStdFile::Decompress(Editor::GetRomFSFile(Editor::BancPrefix + Editor::Identifier + "_Dynamic.bcett.byml.zs"), ZStdFile::Dictionary::BcettByaml).Data);
	BymlFile StaticActorsByml(ZStdFile::Decompress(Editor::GetRomFSFile(Editor::BancPrefix + Editor::Identifier + "_Static.bcett.byml.zs"), ZStdFile::Dictionary::BcettByaml).Data);

	ActorMgr::GetActors().clear();
	ActorMgr::OpaqueActors.clear();
	ActorMgr::TransparentActorsDiscard.clear();
	mStaticCompounds.clear();
	UIOutliner::SelectedActor = nullptr;

	Editor::DynamicActorsByml = DynamicActorsByml;
	Editor::StaticActorsByml = StaticActorsByml;

	std::string StaticCompoundDirectory = "";
	std::string StaticCompoundPrefix = "";
	switch (SceneType)
	{
	case SceneMgr::Type::SkyIslands:
		StaticCompoundDirectory = "Phive/StaticCompoundBody/MainField/Sky";
		StaticCompoundPrefix = "Phive/StaticCompoundBody/MainField/Sky/";
		break;
	case SceneMgr::Type::MainField:
		StaticCompoundDirectory = "Phive/StaticCompoundBody/MainField";
		StaticCompoundPrefix = "Phive/StaticCompoundBody/MainField/MainField_";
		break;
	case SceneMgr::Type::MinusField:
		StaticCompoundDirectory = "Phive/StaticCompoundBody/MinusField";
		StaticCompoundPrefix = "Phive/StaticCompoundBody/MinusField/MinusField_";
		break;
	case SceneMgr::Type::LargeDungeon:
		StaticCompoundDirectory = "Phive/StaticCompoundBody/" + (std::string)(Editor::Identifier == "LargeDungeonFire" ? "MinusField" : "MainField") + "/LargeDungeon";
		StaticCompoundPrefix = "Phive/StaticCompoundBody/" + (std::string)(Editor::Identifier == "LargeDungeonFire" ? "MinusField" : "MainField") + "/LargeDungeon/";
		break;
	case SceneMgr::Type::SmallDungeon:
		StaticCompoundDirectory = "Phive/StaticCompoundBody/SmallDungeon";
		StaticCompoundPrefix = "Phive/StaticCompoundBody/SmallDungeon/Dungeon";
		break;
	case SceneMgr::Type::NormalStage:
		StaticCompoundDirectory = "Phive/StaticCompoundBody/NormalStage";
		StaticCompoundPrefix = "Phive/StaticCompoundBody/NormalStage/";
		break;
	case SceneMgr::Type::Custom:
		Logger::Warning("SceneMgr", "Custom scene types do not support static compounds");
		break;
	default:
		break;
	}

	Editor::StaticCompoundDirectory = StaticCompoundDirectory;
	Editor::StaticCompoundPrefix = StaticCompoundPrefix;
	if (!StaticCompoundDirectory.empty())
	{
		switch (SceneType)
		{
		case SceneMgr::Type::MainField:
		case SceneMgr::Type::MinusField:
			mStaticCompounds.emplace_back(ZStdFile::Decompress(Editor::GetRomFSFile(StaticCompoundPrefix + Editor::Identifier + "_X0_Z0.Nin_NX_NVN.bphsc.zs"), ZStdFile::Dictionary::Base).Data);
			mStaticCompounds.emplace_back(ZStdFile::Decompress(Editor::GetRomFSFile(StaticCompoundPrefix + Editor::Identifier + "_X1_Z0.Nin_NX_NVN.bphsc.zs"), ZStdFile::Dictionary::Base).Data);
			mStaticCompounds.emplace_back(ZStdFile::Decompress(Editor::GetRomFSFile(StaticCompoundPrefix + Editor::Identifier + "_X2_Z0.Nin_NX_NVN.bphsc.zs"), ZStdFile::Dictionary::Base).Data);
			mStaticCompounds.emplace_back(ZStdFile::Decompress(Editor::GetRomFSFile(StaticCompoundPrefix + Editor::Identifier + "_X3_Z0.Nin_NX_NVN.bphsc.zs"), ZStdFile::Dictionary::Base).Data);
			mStaticCompounds.emplace_back(ZStdFile::Decompress(Editor::GetRomFSFile(StaticCompoundPrefix + Editor::Identifier + "_X0_Z1.Nin_NX_NVN.bphsc.zs"), ZStdFile::Dictionary::Base).Data);
			mStaticCompounds.emplace_back(ZStdFile::Decompress(Editor::GetRomFSFile(StaticCompoundPrefix + Editor::Identifier + "_X1_Z1.Nin_NX_NVN.bphsc.zs"), ZStdFile::Dictionary::Base).Data);
			mStaticCompounds.emplace_back(ZStdFile::Decompress(Editor::GetRomFSFile(StaticCompoundPrefix + Editor::Identifier + "_X2_Z1.Nin_NX_NVN.bphsc.zs"), ZStdFile::Dictionary::Base).Data);
			mStaticCompounds.emplace_back(ZStdFile::Decompress(Editor::GetRomFSFile(StaticCompoundPrefix + Editor::Identifier + "_X3_Z1.Nin_NX_NVN.bphsc.zs"), ZStdFile::Dictionary::Base).Data);
			mStaticCompounds.emplace_back(ZStdFile::Decompress(Editor::GetRomFSFile(StaticCompoundPrefix + Editor::Identifier + "_X0_Z2.Nin_NX_NVN.bphsc.zs"), ZStdFile::Dictionary::Base).Data);
			mStaticCompounds.emplace_back(ZStdFile::Decompress(Editor::GetRomFSFile(StaticCompoundPrefix + Editor::Identifier + "_X1_Z2.Nin_NX_NVN.bphsc.zs"), ZStdFile::Dictionary::Base).Data);
			mStaticCompounds.emplace_back(ZStdFile::Decompress(Editor::GetRomFSFile(StaticCompoundPrefix + Editor::Identifier + "_X2_Z2.Nin_NX_NVN.bphsc.zs"), ZStdFile::Dictionary::Base).Data);
			mStaticCompounds.emplace_back(ZStdFile::Decompress(Editor::GetRomFSFile(StaticCompoundPrefix + Editor::Identifier + "_X3_Z2.Nin_NX_NVN.bphsc.zs"), ZStdFile::Dictionary::Base).Data);
			mStaticCompounds.emplace_back(ZStdFile::Decompress(Editor::GetRomFSFile(StaticCompoundPrefix + Editor::Identifier + "_X0_Z3.Nin_NX_NVN.bphsc.zs"), ZStdFile::Dictionary::Base).Data);
			mStaticCompounds.emplace_back(ZStdFile::Decompress(Editor::GetRomFSFile(StaticCompoundPrefix + Editor::Identifier + "_X1_Z3.Nin_NX_NVN.bphsc.zs"), ZStdFile::Dictionary::Base).Data);
			mStaticCompounds.emplace_back(ZStdFile::Decompress(Editor::GetRomFSFile(StaticCompoundPrefix + Editor::Identifier + "_X2_Z3.Nin_NX_NVN.bphsc.zs"), ZStdFile::Dictionary::Base).Data);
			mStaticCompounds.emplace_back(ZStdFile::Decompress(Editor::GetRomFSFile(StaticCompoundPrefix + Editor::Identifier + "_X3_Z3.Nin_NX_NVN.bphsc.zs"), ZStdFile::Dictionary::Base).Data);
			break;
		case SceneMgr::Type::SkyIslands:
		case SceneMgr::Type::LargeDungeon:
		case SceneMgr::Type::SmallDungeon:
		case SceneMgr::Type::NormalStage:
			mStaticCompounds.emplace_back(ZStdFile::Decompress(Editor::GetRomFSFile(StaticCompoundPrefix + Editor::Identifier + ".Nin_NX_NVN.bphsc.zs"), ZStdFile::Dictionary::Base).Data);
			break;
		default:
			break;
		}
	}

	auto DecodeByml = [](BymlFile& File, Actor::Type Type)
		{
			if (!File.HasChild("Actors")) return;
			for (BymlFile::Node& ActorNode : File.GetNode("Actors")->GetChildren())
			{
				Actor* NewActor = ActorMgr::AddActorFromByml(ActorNode);
				if (NewActor == nullptr) return;

				NewActor->ActorType = Type;
			}
		};

	DecodeByml(DynamicActorsByml, Actor::Type::Dynamic);
	DecodeByml(StaticActorsByml, Actor::Type::Static);

	uint32_t WaterBoxCount = 0;
	uint32_t WaterCylinderCount = 0;
	for (PhiveStaticCompound& Compound : mStaticCompounds)
	{
		WaterBoxCount += Compound.mWaterBoxArray.size();
		WaterCylinderCount += Compound.mWaterCylinderArray.size();
	}

	if (WaterBoxCount > 0 || WaterCylinderCount > 0)
	{
		Logger::Warning("SceneMgr", std::to_string(WaterBoxCount) + " water boxes and " + std::to_string(WaterCylinderCount) + " water cylinders could not be linked to an actor. Be careful when removing a fluid type, some of them are not linked to the static compound");
	}

	GameDataListMgr::LoadActorGameData();
	ActorMgr::UpdateModelOrder();
	HashMgr::Initialize();
	Logger::Info("SceneMgr", "Scene reload completed");
}