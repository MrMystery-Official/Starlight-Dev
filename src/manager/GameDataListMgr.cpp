#include <manager/GameDataListMgr.h>

#include <util/FileUtil.h>
#include <util/Logger.h>
#include <file/game/zstd/ZStdBackend.h>
#include <Editor.h>

namespace application::manager
{
	application::file::game::byml::BymlFile GameDataListMgr::gGameDataList;
	std::unordered_map<uint64_t, application::file::game::byml::BymlFile::Node> GameDataListMgr::gBoolU64HashNodes;

	void GameDataListMgr::Initialize()
	{
		gGameDataList.Initialize(application::file::game::ZStdBackend::Decompress(application::util::FileUtil::ReadFile(application::util::FileUtil::GetRomFSFilePath("GameData/GameDataList.Product." + application::Editor::gInternalGameDataListVersion + ".byml.zs"))));
		gBoolU64HashNodes.clear();
		for (application::file::game::byml::BymlFile::Node& Node : gGameDataList.GetNode("Data")->GetChild("Bool64bitKey")->GetChildren())
		{
			gBoolU64HashNodes.insert({ Node.GetChild("Hash")->GetValue<uint64_t>(), Node });
		}

		application::util::Logger::Info("GameDataListMgr", "Loaded GameDataList with " + std::to_string(gBoolU64HashNodes.size()) + " entries");
	}

	void GameDataListMgr::RebuildAndSave(application::game::Scene& Scene)
	{
		bool NeedsRebuild = false;

		auto SetBit = [](uint32_t& Value, uint8_t Index, bool State)
			{
				Value = (Value & ~((uint32_t)1 << Index)) | ((uint32_t)State << Index);
			};

		int32_t NewAllDataSaveSize = gGameDataList.GetNode("MetaData")->GetChild("AllDataSaveSize")->GetValue<int32_t>();
		int32_t SaveDataSizeBool64Bit = gGameDataList.GetNode("MetaData")->GetChild("SaveDataSize")->GetChild(0)->GetValue<int32_t>();

		for (application::game::Scene::BancEntityRenderInfo* RenderInfo : Scene.mDrawListRenderInfoIndices)
		{
			if (!RenderInfo->mEntity->mActorPack->mNeedsPhysicsHash)
				continue;

			uint32_t ResetTypeValue = 0;

			SetBit(ResetTypeValue, 0, RenderInfo->mEntity->mGameData.mOnBancChange);
			SetBit(ResetTypeValue, 1, RenderInfo->mEntity->mGameData.mOnNewDay);
			SetBit(ResetTypeValue, 2, RenderInfo->mEntity->mGameData.mOnDefaultOptionRevert);
			SetBit(ResetTypeValue, 3, RenderInfo->mEntity->mGameData.mOnBloodMoon);
			SetBit(ResetTypeValue, 4, RenderInfo->mEntity->mGameData.mOnNewGame);
			SetBit(ResetTypeValue, 5, RenderInfo->mEntity->mGameData.mUnknown0);
			SetBit(ResetTypeValue, 6, RenderInfo->mEntity->mGameData.mUnknown1);
			SetBit(ResetTypeValue, 7, RenderInfo->mEntity->mGameData.mOnZonaiRespawnDay);
			SetBit(ResetTypeValue, 8, RenderInfo->mEntity->mGameData.mOnMaterialRespawn);
			SetBit(ResetTypeValue, 9, RenderInfo->mEntity->mGameData.mNoResetOnNewGame);

			if (auto Node = gBoolU64HashNodes.find(RenderInfo->mEntity->mHash); Node != gBoolU64HashNodes.end())
			{
				application::file::game::byml::BymlFile::Node* ResetTypeValueNode = Node->second.GetChild("ResetTypeValue");
				if (ResetTypeValueNode->GetValue<int32_t>() != ResetTypeValue)
				{
					ResetTypeValueNode->SetValue<int32_t>(ResetTypeValue);
					NeedsRebuild = true;
				}

				application::file::game::byml::BymlFile::Node* SaveFileIndexNode = Node->second.GetChild("SaveFileIndex");
				if (SaveFileIndexNode->GetValue<int32_t>() != RenderInfo->mEntity->mGameData.mSaveFileIndex)
				{
					SaveFileIndexNode->SetValue<int32_t>(RenderInfo->mEntity->mGameData.mSaveFileIndex);
					NeedsRebuild = true;
				}

				if (RenderInfo->mEntity->mGameData.mOnMaterialRespawn)
				{
					if (Node->second.HasChild("ExtraByte"))
					{
						application::file::game::byml::BymlFile::Node* ExtraByteNode = Node->second.GetChild("ExtraByte");
						if (ExtraByteNode->GetValue<int32_t>() != RenderInfo->mEntity->mGameData.mExtraByte)
						{
							ExtraByteNode->SetValue<int32_t>(RenderInfo->mEntity->mGameData.mExtraByte);
							NeedsRebuild = true;
						}
					}
					else
					{
						application::file::game::byml::BymlFile::Node ExtraByteNode(application::file::game::byml::BymlFile::Type::Int32, "ExtraByte");
						ExtraByteNode.SetValue<int32_t>(RenderInfo->mEntity->mGameData.mExtraByte);
						Node->second.mChildren.insert(Node->second.mChildren.begin(), ExtraByteNode);
						NeedsRebuild = true;
					}
				}
			}
			else if (ResetTypeValue > 0)
			{
				application::file::game::byml::BymlFile::Node NodeDict(application::file::game::byml::BymlFile::Type::Dictionary);

				if (RenderInfo->mEntity->mGameData.mOnMaterialRespawn)
				{
					application::file::game::byml::BymlFile::Node ExtraByteNode(application::file::game::byml::BymlFile::Type::Int32, "ExtraByte");
					ExtraByteNode.SetValue<int32_t>(RenderInfo->mEntity->mGameData.mExtraByte);
					NodeDict.AddChild(ExtraByteNode);
				}

				application::file::game::byml::BymlFile::Node HashNode(application::file::game::byml::BymlFile::Type::UInt64, "Hash");
				HashNode.SetValue<uint64_t>(RenderInfo->mEntity->mHash);
				NodeDict.AddChild(HashNode);

				application::file::game::byml::BymlFile::Node ResetTypeValueNode(application::file::game::byml::BymlFile::Type::Int32, "ResetTypeValue");
				ResetTypeValueNode.SetValue<int32_t>(ResetTypeValue);
				NodeDict.AddChild(ResetTypeValueNode);

				application::file::game::byml::BymlFile::Node SaveFileIndexNode(application::file::game::byml::BymlFile::Type::Int32, "SaveFileIndex");
				SaveFileIndexNode.SetValue<int32_t>(RenderInfo->mEntity->mGameData.mSaveFileIndex);
				NodeDict.AddChild(SaveFileIndexNode);

				gBoolU64HashNodes.insert({ RenderInfo->mEntity->mHash, NodeDict });
				NewAllDataSaveSize += 8;
				SaveDataSizeBool64Bit += 8;

				NeedsRebuild = true;


				application::util::Logger::Info("GameDataListMgr", "Added new actor to GameDataList: " + std::to_string(RenderInfo->mEntity->mHash));
			}
		}

		if (NeedsRebuild)
		{
			gGameDataList.GetNode("Data")->GetChild("Bool64bitKey")->mChildren.clear();
			std::map<uint64_t, application::file::game::byml::BymlFile::Node> Ordered(gBoolU64HashNodes.begin(), gBoolU64HashNodes.end());
			for (auto it = Ordered.begin(); it != Ordered.end(); ++it)
			{
				gGameDataList.GetNode("Data")->GetChild("Bool64bitKey")->AddChild(it->second);
			}

			gGameDataList.GetNode("MetaData")->GetChild("AllDataSaveSize")->SetValue<int32_t>(NewAllDataSaveSize);
			gGameDataList.GetNode("MetaData")->GetChild("SaveDataSize")->GetChild(0)->SetValue<int32_t>(SaveDataSizeBool64Bit);

			std::filesystem::create_directories(application::util::FileUtil::GetSaveFilePath("GameData"));

			application::util::FileUtil::WriteFile(application::util::FileUtil::GetSaveFilePath("GameData/GameDataList.Product." + application::Editor::gInternalGameDataListVersion + ".byml.zs"), application::file::game::ZStdBackend::Compress(gGameDataList.ToBinary(application::file::game::byml::BymlFile::TableGeneration::MANUAL), application::file::game::ZStdBackend::Dictionary::Base));

		}
	}

	void GameDataListMgr::LoadActorGameData(application::game::Scene& Scene)
	{
		for (application::game::Scene::BancEntityRenderInfo* RenderInfo : Scene.mDrawListRenderInfoIndices)
		{
			if (!RenderInfo->mEntity->mActorPack->mNeedsPhysicsHash)
				continue;

			if (auto Node = gBoolU64HashNodes.find(RenderInfo->mEntity->mHash); Node != gBoolU64HashNodes.end())
			{
				RenderInfo->mEntity->mGameData.mHash = RenderInfo->mEntity->mHash;
				RenderInfo->mEntity->mGameData.mSaveFileIndex = Node->second.GetChild("SaveFileIndex")->GetValue<int32_t>();

				int32_t ResetTypeValue = Node->second.GetChild("ResetTypeValue")->GetValue<int32_t>();
				RenderInfo->mEntity->mGameData.mOnBancChange = (ResetTypeValue & 1) > 0;
				RenderInfo->mEntity->mGameData.mOnNewDay = (ResetTypeValue & 2) > 0;
				RenderInfo->mEntity->mGameData.mOnDefaultOptionRevert = (ResetTypeValue & 4) > 0;
				RenderInfo->mEntity->mGameData.mOnBloodMoon = (ResetTypeValue & 8) > 0;
				RenderInfo->mEntity->mGameData.mOnNewGame = (ResetTypeValue & 16) > 0;
				RenderInfo->mEntity->mGameData.mUnknown0 = (ResetTypeValue & 32) > 0;
				RenderInfo->mEntity->mGameData.mUnknown1 = (ResetTypeValue & 64) > 0;
				RenderInfo->mEntity->mGameData.mOnZonaiRespawnDay = (ResetTypeValue & 128) > 0;
				RenderInfo->mEntity->mGameData.mOnMaterialRespawn = (ResetTypeValue & 256) > 0;
				RenderInfo->mEntity->mGameData.mNoResetOnNewGame = (ResetTypeValue & 512) > 0;

				if (RenderInfo->mEntity->mGameData.mOnMaterialRespawn)
					RenderInfo->mEntity->mGameData.mExtraByte = Node->second.GetChild("ExtraByte")->GetValue<uint8_t>();

				application::util::Logger::Info("GameDataListMgr", "Loaded actor from GameDataList: " + std::to_string(RenderInfo->mEntity->mHash));
			}
		}

		application::util::Logger::Info("GameDataListMgr", "Loaded actor flags");
	}
}