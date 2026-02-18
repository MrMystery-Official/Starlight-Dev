#include "SceneCreator.h"

#include <file/game/byml/BymlFile.h>
#include <file/game/zstd/ZStdBackend.h> 
#include <file/game/ainb/AINBFile.h>
#include <file/game/phive/PhiveStaticCompoundFile.h>
#include <util/General.h>
#include <util/FileUtil.h>
#include <filesystem>

namespace application::tool
{
	void SceneCreator::CreateScene(const std::string& Identifier)
	{
		application::file::game::byml::BymlFile GameBancParamByml;
		GameBancParamByml.GetType() = application::file::game::byml::BymlFile::Type::Dictionary;
		GameBancParamByml.GetNodes().push_back(application::file::game::byml::BymlFile::Node(application::file::game::byml::BymlFile::Type::StringIndex, "$parent", "Work/Banc/GameBancParam/SmallDungeonBase.game__banc__GameBancParam.gyml"));

		application::file::game::byml::BymlFile::Node GameBancParamBancNode(application::file::game::byml::BymlFile::Type::Dictionary, "Banc");
		GameBancParamBancNode.AddChild(application::file::game::byml::BymlFile::Node(application::file::game::byml::BymlFile::Type::StringIndex, "Main", "Work/Banc/Scene/SmallDungeon/Dungeon" + Identifier + ".bcett.json"));
		GameBancParamByml.GetNodes().push_back(GameBancParamBancNode);

		GameBancParamByml.GetNodes().push_back(application::file::game::byml::BymlFile::Node(application::file::game::byml::BymlFile::Type::StringIndex, "BaseLocation", "Work/Location/Dungeon" + Identifier + ".game__location__Location.gyml"));
		GameBancParamByml.GetNodes().push_back(application::file::game::byml::BymlFile::Node(application::file::game::byml::BymlFile::Type::Bool, "IsDungeonMergeEnabled", true));
		GameBancParamByml.GetNodes().push_back(application::file::game::byml::BymlFile::Node(application::file::game::byml::BymlFile::Type::StringIndex, "NavMeshMgrMode", "None"));
		GameBancParamByml.GetNodes().push_back(application::file::game::byml::BymlFile::Node(application::file::game::byml::BymlFile::Type::StringIndex, "SceneBakeType", "None"));
		GameBancParamByml.GetNodes().push_back(application::file::game::byml::BymlFile::Node(application::file::game::byml::BymlFile::Type::StringIndex, "SmallDungeonParamRef", "Work/Scene/Component/SmallDungeonParam/Dungeon" + Identifier + ".game__scene__SmallDungeonParam.gyml"));

		application::file::game::byml::BymlFile::Node TraverseBasePosNode(application::file::game::byml::BymlFile::Type::Dictionary, "TraverseBasePos");
		TraverseBasePosNode.AddChild(application::file::game::byml::BymlFile::Node(application::file::game::byml::BymlFile::Type::Float, "X", 0.0f));
		TraverseBasePosNode.AddChild(application::file::game::byml::BymlFile::Node(application::file::game::byml::BymlFile::Type::Float, "Y", 0.0f));
		TraverseBasePosNode.AddChild(application::file::game::byml::BymlFile::Node(application::file::game::byml::BymlFile::Type::Float, "Z", 0.0f));
		GameBancParamByml.GetNodes().push_back(TraverseBasePosNode);

		GameBancParamByml.GetNodes().push_back(application::file::game::byml::BymlFile::Node(application::file::game::byml::BymlFile::Type::StringIndex, "VolumeStatsWorldName", "Work/VolumeStats/WorldParam/CDungeon.game__vstats__WorldParam.gyml"));
		std::filesystem::create_directories(application::util::FileUtil::GetSaveFilePath("Banc/GameBancParam"));
		application::util::FileUtil::WriteFile(application::util::FileUtil::GetSaveFilePath("Banc/GameBancParam/Dungeon" + Identifier + ".game__banc__GameBancParam.bgyml"), GameBancParamByml.ToBinary(application::file::game::byml::BymlFile::TableGeneration::AUTO));


		application::file::game::byml::BymlFile StaticBcettByml;
		StaticBcettByml.GetType() = application::file::game::byml::BymlFile::Type::Dictionary;
		StaticBcettByml.GetNodes().push_back(application::file::game::byml::BymlFile::Node(application::file::game::byml::BymlFile::Type::Dictionary, "Actors"));
		StaticBcettByml.GetNodes().push_back(application::file::game::byml::BymlFile::Node(application::file::game::byml::BymlFile::Type::Dictionary, "AiGroups"));
		StaticBcettByml.GetNodes().push_back(application::file::game::byml::BymlFile::Node(application::file::game::byml::BymlFile::Type::StringIndex, "FilePath", "Work/Banc/Scene/SmallDungeon/Dungeon" + Identifier + ".bcett.json"));
		std::filesystem::create_directories(application::util::FileUtil::GetSaveFilePath("Banc/SmallDungeon"));
		application::util::FileUtil::WriteFile(application::util::FileUtil::GetSaveFilePath("Banc/SmallDungeon/Dungeon" + Identifier + "_Static.bcett.byml.zs"), application::file::game::ZStdBackend::Compress(StaticBcettByml.ToBinary(application::file::game::byml::BymlFile::TableGeneration::AUTO), application::file::game::ZStdBackend::Dictionary::BcettByaml));


		application::file::game::byml::BymlFile DynamicBcettByml;
		DynamicBcettByml.GetType() = application::file::game::byml::BymlFile::Type::Dictionary;
		DynamicBcettByml.GetNodes().push_back(application::file::game::byml::BymlFile::Node(application::file::game::byml::BymlFile::Type::Dictionary, "Actors"));
		DynamicBcettByml.GetNodes().push_back(application::file::game::byml::BymlFile::Node(application::file::game::byml::BymlFile::Type::StringIndex, "FilePath", "Work/Banc/Scene/SmallDungeon/Dungeon" + Identifier + ".bcett.json"));
		std::filesystem::create_directories(application::util::FileUtil::GetSaveFilePath("Banc/SmallDungeon"));
		application::util::FileUtil::WriteFile(application::util::FileUtil::GetSaveFilePath("Banc/SmallDungeon/Dungeon" + Identifier + "_Dynamic.bcett.byml.zs"), application::file::game::ZStdBackend::Compress(DynamicBcettByml.ToBinary(application::file::game::byml::BymlFile::TableGeneration::AUTO), application::file::game::ZStdBackend::Dictionary::BcettByaml));


		application::file::game::byml::BymlFile StartPosByml(application::file::game::ZStdBackend::Decompress(application::util::FileUtil::GetRomFSFilePath("Banc/SmallDungeon/StartPos/SmallDungeon.startpos.byml.zs")));
		application::file::game::byml::BymlFile::Node StartPosNode(application::file::game::byml::BymlFile::Type::Dictionary, "Dungeon" + Identifier);

		application::file::game::byml::BymlFile::Node RotNode(application::file::game::byml::BymlFile::Type::Array, "Rot");
		RotNode.AddChild(application::file::game::byml::BymlFile::Node(application::file::game::byml::BymlFile::Type::Float, "X", 0.0f));
		RotNode.AddChild(application::file::game::byml::BymlFile::Node(application::file::game::byml::BymlFile::Type::Float, "Y", 0.0f));
		RotNode.AddChild(application::file::game::byml::BymlFile::Node(application::file::game::byml::BymlFile::Type::Float, "Z", 0.0f));

		application::file::game::byml::BymlFile::Node TransNode(application::file::game::byml::BymlFile::Type::Array, "Rot");
		TransNode.AddChild(application::file::game::byml::BymlFile::Node(application::file::game::byml::BymlFile::Type::Float, "X", 0.0f));
		TransNode.AddChild(application::file::game::byml::BymlFile::Node(application::file::game::byml::BymlFile::Type::Float, "Y", 0.0f));
		TransNode.AddChild(application::file::game::byml::BymlFile::Node(application::file::game::byml::BymlFile::Type::Float, "Z", 0.0f));

		StartPosNode.AddChild(RotNode);
		StartPosNode.AddChild(TransNode);
		StartPosByml.GetNode("OnElevator")->AddChild(StartPosNode);

		std::filesystem::create_directories(application::util::FileUtil::GetSaveFilePath("Banc/SmallDungeon/StartPos"));
		application::util::FileUtil::WriteFile(application::util::FileUtil::GetSaveFilePath("Banc/SmallDungeon/StartPos/SmallDungeon.startpos.byml.zs"), application::file::game::ZStdBackend::Compress(StartPosByml.ToBinary(application::file::game::byml::BymlFile::TableGeneration::AUTO), application::file::game::ZStdBackend::Dictionary::Base));
		

		application::file::game::phive::PhiveStaticCompoundFile StaticCompoundFile(application::file::game::ZStdBackend::Decompress(application::util::FileUtil::GetRomFSFilePath("Phive/StaticCompoundBody/SmallDungeon/Dungeon001.Nin_NX_NVN.bphsc.zs")));
		std::memset(StaticCompoundFile.mImage.mCompoundName, 0, sizeof(StaticCompoundFile.mImage.mCompoundName));
		std::strcpy(StaticCompoundFile.mImage.mCompoundName, ("Dungeon" + Identifier).c_str());
		StaticCompoundFile.mExternalBphshMeshes.clear();
		StaticCompoundFile.mCompoundTagFile.clear();
		StaticCompoundFile.mActorHashMap.clear();
		StaticCompoundFile.mActorLinks.clear();
		StaticCompoundFile.mWaterBoxArray.clear();
		StaticCompoundFile.mWaterCylinderArray.clear();
		StaticCompoundFile.mWaterFallArray.clear();
		StaticCompoundFile.mImage.mRigidBodyEntryEntityCount = 0;
		StaticCompoundFile.mImage.mRigidBodyEntrySensorCount = 0;
		std::filesystem::create_directories(application::util::FileUtil::GetSaveFilePath("Phive/StaticCompoundBody/SmallDungeon"));
		application::util::FileUtil::WriteFile(application::util::FileUtil::GetSaveFilePath("Phive/StaticCompoundBody/SmallDungeon/Dungeon" + Identifier + ".Nin_NX_NVN.bphsc.zs"), application::file::game::ZStdBackend::Compress(StaticCompoundFile.ToBinary(), application::file::game::ZStdBackend::Dictionary::Base));
		
		application::file::game::byml::BymlFile SmallDungeonParamByml;
		SmallDungeonParamByml.GetType() = application::file::game::byml::BymlFile::Type::Dictionary;
		SmallDungeonParamByml.GetNodes().push_back(application::file::game::byml::BymlFile::Node(application::file::game::byml::BymlFile::Type::StringIndex, "Type", "Gimmick"));
		std::filesystem::create_directories(application::util::FileUtil::GetSaveFilePath("Scene/Component/SmallDungeonParam"));
		application::util::FileUtil::WriteFile(application::util::FileUtil::GetSaveFilePath("Scene/Component/SmallDungeonParam/Dungeon" + Identifier + ".game__scene__SmallDungeonParam.bgyml"), SmallDungeonParamByml.ToBinary(application::file::game::byml::BymlFile::TableGeneration::AUTO));
	}
}