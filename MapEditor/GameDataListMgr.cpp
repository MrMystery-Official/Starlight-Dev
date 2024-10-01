#include "GameDataListMgr.h"

#include "ZStdFile.h"
#include "Editor.h"
#include "Util.h"
#include "ActorMgr.h"
#include "Logger.h"

#include <iostream>

BymlFile GameDataListMgr::mGameDataList;
std::unordered_map<uint64_t, BymlFile::Node> GameDataListMgr::mBoolU64HashNodes;

void GameDataListMgr::Initialize()
{
	mGameDataList = BymlFile(ZStdFile::Decompress(Editor::GetRomFSFile("GameData/GameDataList.Product." + Editor::GameDataListVersion + ".byml.zs"), ZStdFile::Dictionary::Base).Data);
	mBoolU64HashNodes.clear();
	for (BymlFile::Node& Node : mGameDataList.GetNode("Data")->GetChild("Bool64bitKey")->GetChildren())
	{
		mBoolU64HashNodes.insert({ Node.GetChild("Hash")->GetValue<uint64_t>(), Node });
	}
	//mGameDataList.WriteToFile(Editor::GetWorkingDirFile("GameDataList.byml"), BymlFile::TableGeneration::Manual);
}

void GameDataListMgr::RebuildAndSave()
{
	bool NeedsRebuild = false;

	auto SetBit = [](uint32_t& Value, uint8_t Index, bool State)
		{
			Value = (Value & ~((uint32_t)1 << Index)) | ((uint32_t)State << Index);
		};

	int32_t NewAllDataSaveSize = mGameDataList.GetNode("MetaData")->GetChild("AllDataSaveSize")->GetValue<int32_t>();
	int32_t SaveDataSizeBool64Bit = mGameDataList.GetNode("MetaData")->GetChild("SaveDataSize")->GetChild(0)->GetValue<int32_t>();

	for (Actor& SceneActor : ActorMgr::GetActors())
	{
		if (!ActorMgr::IsPhysicsActor(SceneActor.Gyml))
			continue;

		uint32_t ResetTypeValue = 0;

		SetBit(ResetTypeValue, 0, SceneActor.mGameData.mOnBancChange);
		SetBit(ResetTypeValue, 1, SceneActor.mGameData.mOnNewDay);
		SetBit(ResetTypeValue, 2, SceneActor.mGameData.mOnDefaultOptionRevert);
		SetBit(ResetTypeValue, 3, SceneActor.mGameData.mOnBloodMoon);
		SetBit(ResetTypeValue, 4, SceneActor.mGameData.mOnNewGame);
		SetBit(ResetTypeValue, 5, SceneActor.mGameData.mUnknown0);
		SetBit(ResetTypeValue, 6, SceneActor.mGameData.mUnknown1);
		SetBit(ResetTypeValue, 7, SceneActor.mGameData.mOnZonaiRespawnDay);
		SetBit(ResetTypeValue, 8, SceneActor.mGameData.mOnMaterialRespawn);
		SetBit(ResetTypeValue, 9, SceneActor.mGameData.mNoResetOnNewGame);

		if (auto Node = mBoolU64HashNodes.find(SceneActor.Hash); Node != mBoolU64HashNodes.end())
		{
			BymlFile::Node* ResetTypeValueNode = Node->second.GetChild("ResetTypeValue");
			if (ResetTypeValueNode->GetValue<int32_t>() != ResetTypeValue)
			{
				ResetTypeValueNode->SetValue<int32_t>(ResetTypeValue);
				NeedsRebuild = true;
			}

			BymlFile::Node* SaveFileIndexNode = Node->second.GetChild("SaveFileIndex");
			if (SaveFileIndexNode->GetValue<int32_t>() != SceneActor.mGameData.mSaveFileIndex)
			{
				SaveFileIndexNode->SetValue<int32_t>(SceneActor.mGameData.mSaveFileIndex);
				NeedsRebuild = true;
			}

			if (SceneActor.mGameData.mOnMaterialRespawn)
			{
				if (Node->second.HasChild("ExtraByte"))
				{
					BymlFile::Node* ExtraByteNode = Node->second.GetChild("ExtraByte");
					if (ExtraByteNode->GetValue<int32_t>() != SceneActor.mGameData.mExtraByte)
					{
						ExtraByteNode->SetValue<int32_t>(SceneActor.mGameData.mExtraByte);
						NeedsRebuild = true;
					}
				}
				else
				{
					BymlFile::Node ExtraByteNode(BymlFile::Type::Int32, "ExtraByte");
					ExtraByteNode.SetValue<int32_t>(SceneActor.mGameData.mExtraByte);
					Node->second.m_Children.insert(Node->second.m_Children.begin(), ExtraByteNode);
					NeedsRebuild = true;
				}
			}
		}
		else if (ResetTypeValue > 0)
		{
			BymlFile::Node NodeDict(BymlFile::Type::Dictionary);

			if (SceneActor.mGameData.mOnMaterialRespawn)
			{
				BymlFile::Node ExtraByteNode(BymlFile::Type::Int32, "ExtraByte");
				ExtraByteNode.SetValue<int32_t>(SceneActor.mGameData.mExtraByte);
				NodeDict.AddChild(ExtraByteNode);
			}

			BymlFile::Node HashNode(BymlFile::Type::UInt64, "Hash");
			HashNode.SetValue<uint64_t>(SceneActor.Hash);
			NodeDict.AddChild(HashNode);

			BymlFile::Node ResetTypeValueNode(BymlFile::Type::Int32, "ResetTypeValue");
			ResetTypeValueNode.SetValue<int32_t>(ResetTypeValue);
			NodeDict.AddChild(ResetTypeValueNode);

			BymlFile::Node SaveFileIndexNode(BymlFile::Type::Int32, "SaveFileIndex");
			SaveFileIndexNode.SetValue<int32_t>(SceneActor.mGameData.mSaveFileIndex);
			NodeDict.AddChild(SaveFileIndexNode);

			mBoolU64HashNodes.insert({ SceneActor.Hash, NodeDict });
			NewAllDataSaveSize += 8;
			SaveDataSizeBool64Bit += 8;

			NeedsRebuild = true;

			Logger::Info("GameDataListMgr", "Added new actor flag: " + std::to_string(SceneActor.Hash));
		}
	}

	if (NeedsRebuild)
	{
		mGameDataList.GetNode("Data")->GetChild("Bool64bitKey")->m_Children.clear();
		std::map<uint64_t, BymlFile::Node> Ordered(mBoolU64HashNodes.begin(), mBoolU64HashNodes.end());
		for (auto it = Ordered.begin(); it != Ordered.end(); ++it)
		{
			mGameDataList.GetNode("Data")->GetChild("Bool64bitKey")->AddChild(it->second);
		}

		mGameDataList.GetNode("MetaData")->GetChild("AllDataSaveSize")->SetValue<int32_t>(NewAllDataSaveSize);
		mGameDataList.GetNode("MetaData")->GetChild("SaveDataSize")->GetChild(0)->SetValue<int32_t>(SaveDataSizeBool64Bit);

		Util::CreateDir(Editor::GetWorkingDirFile("Save/GameData"));
		ZStdFile::Compress(mGameDataList.ToBinary(BymlFile::TableGeneration::Manual), ZStdFile::Dictionary::Base).WriteToFile(Editor::GetWorkingDirFile("Save/GameData/GameDataList.Product." + Editor::GameDataListVersion + ".byml.zs"));
	}
}

void GameDataListMgr::LoadActorGameData()
{
	for (Actor& SceneActor : ActorMgr::GetActors())
	{
		if (!ActorMgr::IsPhysicsActor(SceneActor.Gyml))
			continue;

		if (auto Node = mBoolU64HashNodes.find(SceneActor.Hash); Node != mBoolU64HashNodes.end())
		{
			SceneActor.mGameData.mHash = SceneActor.Hash;
			SceneActor.mGameData.mSaveFileIndex = Node->second.GetChild("SaveFileIndex")->GetValue<int32_t>();

			int32_t ResetTypeValue = Node->second.GetChild("ResetTypeValue")->GetValue<int32_t>();
			SceneActor.mGameData.mOnBancChange = (ResetTypeValue & 1) > 0;
			SceneActor.mGameData.mOnNewDay = (ResetTypeValue & 2) > 0;
			SceneActor.mGameData.mOnDefaultOptionRevert = (ResetTypeValue & 4) > 0;
			SceneActor.mGameData.mOnBloodMoon = (ResetTypeValue & 8) > 0;
			SceneActor.mGameData.mOnNewGame = (ResetTypeValue & 16) > 0;
			SceneActor.mGameData.mUnknown0 = (ResetTypeValue & 32) > 0;
			SceneActor.mGameData.mUnknown1 = (ResetTypeValue & 64) > 0;
			SceneActor.mGameData.mOnZonaiRespawnDay = (ResetTypeValue & 128) > 0;
			SceneActor.mGameData.mOnMaterialRespawn = (ResetTypeValue & 256) > 0;
			SceneActor.mGameData.mNoResetOnNewGame = (ResetTypeValue & 512) > 0;

			if (SceneActor.mGameData.mOnMaterialRespawn)
				SceneActor.mGameData.mExtraByte = Node->second.GetChild("ExtraByte")->GetValue<uint8_t>();
		}
	}

	Logger::Info("GameDataListMgr", "Loaded actor flags");
}