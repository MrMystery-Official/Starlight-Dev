#include "Exporter.h"

#include "Editor.h"
#include "Util.h"
#include "ZStdFile.h"
#include "RESTBL.h"
#include <filesystem>
#include "UIAINBEditor.h"
#include "BinaryVectorReader.h"
#include "UIActorTool.h"
#include "Logger.h"
#include "HashMgr.h"
#include "SceneMgr.h"
#include "RailMgr.h"

void Exporter::CreateExportOnlyFiles(std::string Path)
{
	//RSTB
	ResTableFile ResTable(ZStdFile::Decompress(Editor::GetRomFSFile("System/Resource/ResourceSizeTable.Product." + Editor::GetInternalGameVersion() + ".rsizetable.zs"), ZStdFile::Dictionary::None).Data);
	
	auto RSTBEstimateBymlSize = [](std::string Path)
		{
			uint32_t Size = ZStdFile::GetDecompressedFileSize(Path);
			Size = (floor((Size + 0x1F) / 0x20)) * 0x20 + 256;
			return Size;
		};

	auto RSTBEstimateBfevflSize = [](std::string Path)
		{
			uint32_t Size = ZStdFile::GetDecompressedFileSize(Path);
			Size = (floor((Size + 0x1F) / 0x20)) * 0x20 + 288;
			return Size;
		};

	auto RSTBEstimateBphnmSize = [](std::string Path)
		{
			uint32_t Size = ZStdFile::GetDecompressedFileSize(Path);
			Size = (floor((Size + 0x1F) / 0x20)) * 0x20 + 288;
			return Size;
		};

	auto RSTBEstimateBphscSize = [](std::string Path)
		{
			uint32_t Size = ZStdFile::GetDecompressedFileSize(Path);
			Size = (Size + 1500) * 4;
			return Size;
		};

	auto RSTBEstimateBgymlSizeWithoutZStd = [](std::string Path)
		{
			std::ifstream File(Path, std::ios::binary);

			if (File.eof() || File.fail()) return (uint32_t)0;

			File.seekg(0, std::ios_base::end);
			std::streampos FileSize = File.tellg();

			uint32_t Size = FileSize;

			File.close();

			Size = (floor((Size + 0x1F) / 0x20)) * 0x20 + 256;
			return Size * 10;
		};

	auto RSTBEstimateAinbSizeByte = [](std::vector<unsigned char> Bytes)
		{
			BinaryVectorReader Reader(Bytes);
			Reader.Seek(0x44, BinaryVectorReader::Position::Begin);
			uint32_t Offset = Reader.ReadUInt32();
			bool HasEXB = Offset != 0;
			uint32_t Size = (floor((Bytes.size() + 0x1F) / 0x20)) * 0x20 + 392;

			if (HasEXB)
			{
				Reader.Seek(Offset + 0x20, BinaryVectorReader::Position::Begin);
				uint32_t NewOffset = Reader.ReadUInt32();
				Reader.Seek(NewOffset + Offset, BinaryVectorReader::Position::Begin);
				uint32_t SignatureCount = Reader.ReadUInt32();
				Size += 16 + floor((SignatureCount + 1) / 2) * 8;
			}

			return Size;
		};

	auto RSTBEstimateAinbSize = [&RSTBEstimateAinbSizeByte](std::string Path)
		{
			std::ifstream File(Path, std::ios::binary);

			File.seekg(0, std::ios_base::end);
			std::streampos FileSize = File.tellg();

			std::vector<unsigned char> Bytes(FileSize);

			File.seekg(0, std::ios_base::beg);
			File.read(reinterpret_cast<char*>(Bytes.data()), FileSize);

			File.close();

			return RSTBEstimateAinbSizeByte(Bytes);
		};

	auto RSTBEstimateSarcSize = [&ResTable, &RSTBEstimateAinbSizeByte](std::string Path)
		{
			SarcFile Sarc(ZStdFile::Decompress(Path, ZStdFile::Dictionary::Pack).Data);
			for (SarcFile::Entry& Entry : Sarc.GetEntries())
			{
				if (Entry.Bytes[0] == 'A' && Entry.Bytes[1] == 'I' && Entry.Bytes[2] == 'B')
				{
					ResTable.SetFileSize(Entry.Name, RSTBEstimateAinbSizeByte(Entry.Bytes) * 5);
					continue;
				}
				if ((Entry.Bytes[0] == 'Y' && Entry.Bytes[1] == 'B') || (Entry.Bytes[0] == 'B' && Entry.Bytes[1] == 'Y'))
				{
					uint32_t Size = Entry.Bytes.size();
					Size = (floor((Size + 0x1F) / 0x20)) * 0x20 + 256;
					Size = (Size + 1000) * 8;
					ResTable.SetFileSize(Entry.Name, Size);
					continue;
				}
			}
		};

	using recursive_directory_iterator = std::filesystem::recursive_directory_iterator;
	for (const auto& dirEntry : recursive_directory_iterator(Editor::GetWorkingDirFile("Save")))
	{
		if (dirEntry.is_regular_file())
		{
			if (dirEntry.path().string().ends_with(".byml.zs"))
			{
				std::string LocalPath = dirEntry.path().string().substr(Editor::GetWorkingDirFile("Save").length() + 1, dirEntry.path().string().length() - 4 - Editor::GetWorkingDirFile("Save").length());
				std::replace(LocalPath.begin(), LocalPath.end(), '\\', '/');
				ResTable.SetFileSize(LocalPath, RSTBEstimateBymlSize(dirEntry.path().string()));
			}
			if (dirEntry.path().string().ends_with(".bgyml"))
			{
				std::string LocalPath = dirEntry.path().string().substr(Editor::GetWorkingDirFile("Save").length() + 1, dirEntry.path().string().length() - 1 - Editor::GetWorkingDirFile("Save").length());
				std::replace(LocalPath.begin(), LocalPath.end(), '\\', '/');
				ResTable.SetFileSize(LocalPath, RSTBEstimateBgymlSizeWithoutZStd(dirEntry.path().string()));
			}
			if (dirEntry.path().string().ends_with(".ainb"))
			{
				std::string LocalPath = dirEntry.path().string().substr(Editor::GetWorkingDirFile("Save").length() + 1, dirEntry.path().string().length() - 1 - Editor::GetWorkingDirFile("Save").length());
				std::replace(LocalPath.begin(), LocalPath.end(), '\\', '/');
				ResTable.SetFileSize(LocalPath, RSTBEstimateAinbSize(dirEntry.path().string()));
			}
			if (dirEntry.path().string().ends_with(".bfevfl.zs"))
			{
				std::string LocalPath = dirEntry.path().string().substr(Editor::GetWorkingDirFile("Save").length() + 1, dirEntry.path().string().length() - 4 - Editor::GetWorkingDirFile("Save").length());
				std::replace(LocalPath.begin(), LocalPath.end(), '\\', '/');
				ResTable.SetFileSize(LocalPath, RSTBEstimateBfevflSize(dirEntry.path().string()));
			}
			if (dirEntry.path().string().ends_with(".bphnm.zs"))
			{
				std::string LocalPath = dirEntry.path().string().substr(Editor::GetWorkingDirFile("Save").length() + 1, dirEntry.path().string().length() - 4 - Editor::GetWorkingDirFile("Save").length());
				std::replace(LocalPath.begin(), LocalPath.end(), '\\', '/');
				ResTable.SetFileSize(LocalPath, RSTBEstimateBphnmSize(dirEntry.path().string()));
			}
			if (dirEntry.path().string().ends_with(".bphsc.zs"))
			{
				std::string LocalPath = dirEntry.path().string().substr(Editor::GetWorkingDirFile("Save").length() + 1, dirEntry.path().string().length() - 4 - Editor::GetWorkingDirFile("Save").length());
				std::replace(LocalPath.begin(), LocalPath.end(), '\\', '/');
				ResTable.SetFileSize(LocalPath, RSTBEstimateBphscSize(dirEntry.path().string()));
			}
			if (dirEntry.path().string().ends_with(".pack.zs"))
			{
				RSTBEstimateSarcSize(dirEntry.path().string());
			}
		}
	}

	Util::CreateDir(Path + "/System/Resource");
	ZStdFile::Compress(ResTable.ToBinary(), ZStdFile::Dictionary::None).WriteToFile(Path + "/System/Resource/ResourceSizeTable.Product." + Editor::GetInternalGameVersion() + ".rsizetable.zs");
}

void Exporter::CreateRSTBL(std::string Path)
{
	//ActorInfo
	if (Util::FileExists(Editor::GetWorkingDirFile("Save/Pack/Actor")))
	{
		std::vector<std::string> ModActors;
		for (const auto& Entry : std::filesystem::directory_iterator(Editor::GetWorkingDirFile("Save/Pack/Actor")))
		{
			if (Entry.is_regular_file() && Entry.path().filename().string().ends_with(".pack.zs"))
			{
				ModActors.push_back(Entry.path().filename().string().substr(0, Entry.path().filename().string().size() - 8)); // 8 = .pack.zs
			}
		}

		if (!ModActors.empty())
		{
			BymlFile ActorInfo(ZStdFile::Decompress(Editor::GetRomFSFile("RSDB/ActorInfo.Product." + Editor::GetInternalGameVersion() + ".rstbl.byml.zs"), ZStdFile::Dictionary::Base).Data);
			std::vector<std::string> UnknownActors;

			for (std::string Name : ModActors)
			{
				bool Found = false;

				for (BymlFile::Node& Node : ActorInfo.GetNodes())
				{
					if (Node.HasChild("__RowId"))
					{
						if (Node.GetChild("__RowId")->GetValue<std::string>() == Name)
						{
							Found = true;
							break;
						}
					}
				}

				if (!Found)
					UnknownActors.push_back(Name);
			}

			if(!UnknownActors.empty()) Logger::Info("Exporter", "Recreating ActorInfo, takes ~20 seconds");

			for (std::string Name : UnknownActors)
			{
				Logger::Info("Exporter", "Adding actor " + Name);

				BymlFile::Node NewActorDict(BymlFile::Type::Dictionary, std::to_string(ActorInfo.GetNodes().size()));

				BymlFile::Node NewActorCalcRadius(BymlFile::Type::Float, "CalcRadius");
				NewActorCalcRadius.SetValue<float>(110.0f);

				BymlFile::Node NewActorCategory(BymlFile::Type::StringIndex, "Category");
				NewActorCategory.SetValue<std::string>("System");

				BymlFile::Node NewActorClassName(BymlFile::Type::StringIndex, "ClassName");
				NewActorClassName.SetValue<std::string>("GameActor");

				BymlFile::Node NewActorCreateRadiusOffset(BymlFile::Type::Float, "CreateRadiusOffset");
				NewActorCreateRadiusOffset.SetValue<float>(5.0f);

				BymlFile::Node NewActorDeleteRadiusOffset(BymlFile::Type::Float, "DeleteRadiusOffset");
				NewActorDeleteRadiusOffset.SetValue<float>(10.0f);

				BymlFile::Node NewActorDisplayRadius(BymlFile::Type::Float, "DisplayRadius");
				NewActorDisplayRadius.SetValue<float>(110.0f);

				BymlFile::Node NewActorELinkUserName(BymlFile::Type::StringIndex, "ELinkUserName");
				NewActorELinkUserName.SetValue<std::string>(Name);

				BymlFile::Node NewActorInstanceHeapSize(BymlFile::Type::Int32, "InstanceHeapSize");
				NewActorInstanceHeapSize.SetValue<int32_t>(77776);

				BymlFile::Node NewActorLoadRadius(BymlFile::Type::Float, "LoadRadius");
				NewActorLoadRadius.SetValue<float>(210.0f);

				BymlFile::Node NewActorModelAabbMax(BymlFile::Type::Dictionary, "ModelAabbMax");
				BymlFile::Node NewActorModelAabbMaxX(BymlFile::Type::Float, "X");
				NewActorModelAabbMaxX.SetValue<float>(1.0f);
				BymlFile::Node NewActorModelAabbMaxY(BymlFile::Type::Float, "Y");
				NewActorModelAabbMaxY.SetValue<float>(2.0f);
				BymlFile::Node NewActorModelAabbMaxZ(BymlFile::Type::Float, "Z");
				NewActorModelAabbMaxZ.SetValue<float>(4.0f);
				NewActorModelAabbMax.AddChild(NewActorModelAabbMaxX);
				NewActorModelAabbMax.AddChild(NewActorModelAabbMaxY);
				NewActorModelAabbMax.AddChild(NewActorModelAabbMaxZ);

				BymlFile::Node NewActorModelAabbMin(BymlFile::Type::Dictionary, "ModelAabbMin");
				BymlFile::Node NewActorModelAabbMinX(BymlFile::Type::Float, "X");
				NewActorModelAabbMinX.SetValue<float>(-1.0f);
				BymlFile::Node NewActorModelAabbMinY(BymlFile::Type::Float, "Y");
				NewActorModelAabbMinY.SetValue<float>(0.0f);
				BymlFile::Node NewActorModelAabbMinZ(BymlFile::Type::Float, "Z");
				NewActorModelAabbMinZ.SetValue<float>(-4.0f);
				NewActorModelAabbMin.AddChild(NewActorModelAabbMinX);
				NewActorModelAabbMin.AddChild(NewActorModelAabbMinY);
				NewActorModelAabbMin.AddChild(NewActorModelAabbMinZ);

				BymlFile::Node NewActorPreActorRenderInfoHeapSize(BymlFile::Type::Int32, "PreActorRenderInfoHeapSize");
				NewActorPreActorRenderInfoHeapSize.SetValue<int32_t>(13200);

				BymlFile::Node NewActorUnloadRadiusOffset(BymlFile::Type::Float, "UnloadRadiusOffset");
				NewActorUnloadRadiusOffset.SetValue<float>(10.0f);

				BymlFile::Node NewActorRowId(BymlFile::Type::StringIndex, "__RowId");
				NewActorRowId.SetValue<std::string>(Name);

				NewActorDict.AddChild(NewActorCalcRadius);
				NewActorDict.AddChild(NewActorCategory);
				NewActorDict.AddChild(NewActorClassName);
				NewActorDict.AddChild(NewActorCreateRadiusOffset);
				NewActorDict.AddChild(NewActorDeleteRadiusOffset);
				NewActorDict.AddChild(NewActorDisplayRadius);
				NewActorDict.AddChild(NewActorELinkUserName);
				NewActorDict.AddChild(NewActorInstanceHeapSize);
				NewActorDict.AddChild(NewActorLoadRadius);
				NewActorDict.AddChild(NewActorModelAabbMax);
				NewActorDict.AddChild(NewActorModelAabbMin);
				NewActorDict.AddChild(NewActorPreActorRenderInfoHeapSize);
				NewActorDict.AddChild(NewActorUnloadRadiusOffset);
				NewActorDict.AddChild(NewActorRowId);

				ActorInfo.GetNodes().push_back(NewActorDict);

				ActorInfo.AddStringTableEntry(Name);
			}

			if (!UnknownActors.empty())
			{
				Util::CreateDir(Path + "/RSDB");
				ZStdFile::Compress(ActorInfo.ToBinary(BymlFile::TableGeneration::Manual), ZStdFile::Dictionary::Base).WriteToFile(Editor::GetWorkingDirFile("Save/RSDB/ActorInfo.Product." + Editor::GetInternalGameVersion() + ".rstbl.byml.zs"));
			}
		}
	}
}

void Exporter::Export(std::string Path, Exporter::Operation Op)
{
	std::string OldPath;

	if (Op == Exporter::Operation::Export)
	{
		OldPath = Path;
		Path = Editor::GetWorkingDirFile("Save");
	}

	Util::CreateDir(Path + "/" + Editor::BancDir);
	UIAINBEditor::Save();
	UIActorTool::Save(Path);

	//Rail stuff
	{
		BymlFile& StaticByml = Editor::StaticActorsByml;

		BymlFile::Node* RailsNode = StaticByml.GetNode("Rails");

		if (RailsNode)
		{
			RailsNode->GetChildren().clear();
		}
		else
		{
			BymlFile::Node Root(BymlFile::Type::Array, "Rails");
			StaticByml.GetNodes().push_back(Root);
			RailsNode = StaticByml.GetNode("Rails");
		}

		for (RailMgr::Rail& Rail : RailMgr::mRails)
		{
			RailsNode->AddChild(RailMgr::RailToNode(Rail));
		}
	}

	//Static compound stuff
#ifdef STARLIGHT_SHARED
	if(false)
	{
#else
	if (!SceneMgr::mStaticCompounds.empty())
	{
#endif
		std::unordered_map<PhiveStaticCompound*, bool> StaticCompoundToRebuild;
		std::unordered_map<PhiveStaticCompound*, uint32_t> ActorLinkSize;

		for (PhiveStaticCompound& StaticCompound : SceneMgr::mStaticCompounds)
		{
			//StaticCompound.mWaterBoxArray.clear();

			ActorLinkSize[&StaticCompound] = StaticCompound.mActorLinks.size();
			StaticCompoundToRebuild[&StaticCompound] = false;
		}

		//Unused hash removal
		/*
		for (auto& [Key, Val] : StaticCompoundToRebuild)
		{
			for (auto Iter = Key->mActorLinks.begin(); Iter != Key->mActorLinks.end(); )
			{
				bool Found = false;
				for (Actor& SceneActor : ActorMgr::GetActors())
				{
					if (SceneActor.Hash == Iter->mActorHash)
					{
						Found = true;
						break;
					}

					for (Actor& Child : SceneActor.MergedActorContent)
					{
						if (Child.Hash == Iter->mActorHash)
						{
							Found = true;
							break;
						}
					}

					if (Found)
						break;
				}

				if (!Found)
				{
					Val = true;
					Iter = Key->mActorLinks.erase(Iter);
				}
				else
				{
					Iter++;
				}
			}
		}
		*/

		for (Actor& SceneActor : ActorMgr::GetActors())
		{
			if (std::find(HashMgr::mNewHashes.begin(), HashMgr::mNewHashes.end(), SceneActor.Hash) == HashMgr::mNewHashes.end())
				continue;

			if (ActorMgr::IsPhysicsActor(SceneActor.Gyml))
			{
				PhiveStaticCompound* StaticCompound = SceneMgr::GetStaticCompoundForPos(glm::vec3(SceneActor.Translate.GetX(), SceneActor.Translate.GetY(), SceneActor.Translate.GetZ()));
				if (StaticCompound == nullptr)
				{
					Logger::Error("Exporter", "Could not find position specific static compound for actor " + SceneActor.Gyml);
					continue;
				}

				bool Found = false;
				for (PhiveStaticCompound::PhiveStaticCompoundActorLink& Link : StaticCompound->mActorLinks)
				{
					if (Link.mActorHash == SceneActor.Hash)
					{
						Found = true;
						break;
					}
				}
				if (Found)
					continue;

				ActorLinkSize[StaticCompound] = ActorLinkSize[StaticCompound] + 1;
				StaticCompoundToRebuild[StaticCompound] = true;
			}
		}

		for (Actor& SceneActor : ActorMgr::GetActors())
		{
			if (std::find(HashMgr::mNewHashes.begin(), HashMgr::mNewHashes.end(), SceneActor.Hash) == HashMgr::mNewHashes.end())
				continue;

			if (ActorMgr::IsPhysicsActor(SceneActor.Gyml))
			{
				PhiveStaticCompound* StaticCompound = SceneMgr::GetStaticCompoundForPos(glm::vec3(SceneActor.Translate.GetX(), SceneActor.Translate.GetY(), SceneActor.Translate.GetZ()));
				if (StaticCompound == nullptr)
				{
					Logger::Error("Exporter", "Could not find position specific static compound for actor " + SceneActor.Gyml);
					continue;
				}

				bool Found = false;
				for (PhiveStaticCompound::PhiveStaticCompoundActorLink& Link : StaticCompound->mActorLinks)
				{
					if (Link.mActorHash == SceneActor.Hash)
					{
						Found = true;
						break;
					}
				}
				if (Found)
					continue;

				uint32_t OptimalIndex = 0xFFFFFFFF;
				for (uint64_t Index : StaticCompound->mActorHashMap)
				{
					if (Index == 0)
					{
						OptimalIndex = Index;
						break;
					}
				}
				if (OptimalIndex == 0xFFFFFFFF)
				{
					OptimalIndex = StaticCompound->mActorLinks.size() * 2;
				}

				uint64_t NewHash = 0;
				uint64_t OldHash = SceneActor.Hash;

				while (true)
				{
					NewHash = HashMgr::GenerateStaticCompoundHash(SceneActor.Hash,
						(StaticCompound->mActorLinks.size() + 1) * 2,
						OptimalIndex);

					bool Duplicate = false;
					for (Actor& ChildActor : ActorMgr::GetActors())
					{
						if (ChildActor.Hash == NewHash)
						{
							Duplicate = true;
							break;
						}

						for (Actor& MergedActorChild : ChildActor.MergedActorContent)
						{
							if (MergedActorChild.Hash == NewHash)
							{
								Duplicate = true;
								break;
							}
						}
						if (Duplicate)
							break;
					}

					SceneActor.Hash = NewHash;

					if (!Duplicate)
						break;

					SceneActor.Hash++;
				}

				Logger::Info("Exporter", "Hash transformer called, old hash: " + std::to_string(OldHash) + ", new hash: " + std::to_string(NewHash));

				if (StaticCompound->mActorHashMap.size() > OptimalIndex)
				{
					StaticCompound->mActorHashMap[OptimalIndex] = NewHash;
				}
				else
				{
					StaticCompound->mActorHashMap.resize(OptimalIndex + 2);
					StaticCompound->mActorHashMap[OptimalIndex] = NewHash;
				}

				if (NewHash != SceneActor.Hash)
				{
					for (Actor& ChildActor : ActorMgr::GetActors())
					{
						if (ChildActor.Hash == OldHash) ChildActor.Hash = NewHash;
						
						for (Actor::Link& Link : ChildActor.Links)
						{
							if (Link.Src == OldHash) Link.Src = NewHash;
							if (Link.Dest == OldHash) Link.Dest = NewHash;
						}

						for (Actor::Rail& Rail : ChildActor.Rails)
						{
							if (Rail.Dest == OldHash) Rail.Dest = NewHash;
						}

						for (auto& [Key, Val] : ChildActor.Phive.Placement)
						{
							if (!Util::IsNumber(Val))
								continue;

							if (std::stoull(Val) == OldHash)
								Val = std::to_string(NewHash);
						}

						if (ChildActor.Phive.ConstraintLink.ID == OldHash) ChildActor.Phive.ConstraintLink.ID = NewHash;
						for (Actor::PhiveData::ConstraintLinkData::OwnerData& Owner : ChildActor.Phive.ConstraintLink.Owners)
						{
							if (Owner.Refer == OldHash)
								Owner.Refer = NewHash;
						}

						if (ChildActor.Phive.RopeHeadLink.ID == OldHash) ChildActor.Phive.RopeHeadLink.ID = NewHash;
						for (uint64_t& HeadLink : ChildActor.Phive.RopeHeadLink.Owners)
						{
							if (HeadLink == OldHash)
								HeadLink = NewHash;
						}

						if (ChildActor.Phive.RopeTailLink.ID == OldHash) ChildActor.Phive.RopeTailLink.ID = NewHash;
						for (uint64_t& TailLink : ChildActor.Phive.RopeTailLink.Owners)
						{
							if (TailLink == OldHash)
								TailLink = NewHash;
						}
					}

					if (Editor::StaticActorsByml.HasChild("AiGroups"))
					{
						for (BymlFile::Node& AiGroup : Editor::StaticActorsByml.GetNode("AiGroups")->GetChildren())
						{
							if (!AiGroup.HasChild("References"))
								continue;

							for (BymlFile::Node& Ref : AiGroup.GetChild("References")->GetChildren())
							{
								if (!Ref.HasChild("Reference"))
									continue;

								if (Ref.GetChild("Reference")->GetValue<uint64_t>() == OldHash)
								{
									Ref.GetChild("Reference")->SetValue<uint64_t>(NewHash);
								}
							}
						}
					}
				}

				SceneActor.Phive.Placement["ID"] = std::to_string(SceneActor.Hash);

				PhiveStaticCompound::PhiveStaticCompoundActorLink Link;
				Link.mActorHash = NewHash;
				Link.mBaseIndex = 0xFFFFFFFF;
				Link.mEndIndex = 0xFFFFFFFF;
				Link.mCompoundRigidBodyIndex = 0;
				Link.mIsValid = 0;
				Link.mReserve0 = SceneActor.SRTHash;
				Link.mReserve1 = 0;
				Link.mReserve2 = 0;
				Link.mReserve3 = 0;

				StaticCompound->mActorLinks.push_back(Link);

				HashMgr::BiggestHash = std::max(HashMgr::BiggestHash, Link.mActorHash + 1);
			}
		}

		HashMgr::mNewHashes.clear();

		if (Editor::Identifier.length() != 0)
		{
			for (Actor& SceneActor : ActorMgr::GetActors())
			{
				if (!SceneActor.mBakeFluid)
					continue;

				switch (SceneActor.mWaterShapeType)
				{
				case Actor::WaterShapeType::Box:
				{
					std::optional<glm::vec2> Scale = PhiveStaticCompound::GetActorModelScale(SceneActor);
					if (!Scale.has_value())
					{
						Logger::Error("Exporter", "Could not bake water box, scale was empty");
						break;
					}

					PhiveStaticCompound* StaticCompound = SceneMgr::GetStaticCompoundForPos(glm::vec3(SceneActor.Translate.GetX(), SceneActor.Translate.GetY(), SceneActor.Translate.GetZ()));

					if (StaticCompound == nullptr)
					{
						Logger::Error("Exporter", "Could not bake water box, position specific static compound was a nullptr");
						continue;
					}

					PhiveStaticCompound::PhiveStaticCompoundWaterBox WaterBox;
					WaterBox.mCompoundId = 0;
					WaterBox.mReserve0 = 0;
					WaterBox.mFlowSpeed = SceneActor.mWaterFlowSpeed;
					WaterBox.mReserve2 = 1.0f;
					WaterBox.mScaleTimesTenX = Scale.value().x;
					WaterBox.mScaleTimesTenY = SceneActor.mWaterBoxHeight;
					WaterBox.mScaleTimesTenZ = Scale.value().y;
					WaterBox.mRotationX = Util::DegreesToRadians(SceneActor.Rotate.GetX());
					WaterBox.mRotationY = Util::DegreesToRadians(SceneActor.Rotate.GetY());
					WaterBox.mRotationZ = Util::DegreesToRadians(SceneActor.Rotate.GetZ());
					WaterBox.mPositionX = SceneActor.Translate.GetX();
					WaterBox.mPositionY = SceneActor.Translate.GetY();
					WaterBox.mPositionZ = SceneActor.Translate.GetZ();
					WaterBox.mFluidType = PhiveStaticCompound::ActorFluidTypeToInternalFluidType(SceneActor.mWaterFluidType);
					WaterBox.mReserve7 = 0;
					WaterBox.mReserve8 = 0;
					WaterBox.mReserve9 = 0;
					StaticCompound->mWaterBoxArray.push_back(WaterBox);
					break;
				}
				case Actor::WaterShapeType::Cylinder:
				{
					std::optional<glm::vec2> Scale = PhiveStaticCompound::GetActorModelScale(SceneActor);
					if (!Scale.has_value())
					{
						Logger::Error("Exporter", "Could not bake water cylinder, scale was empty");
						break;
					}

					PhiveStaticCompound* StaticCompound = SceneMgr::GetStaticCompoundForPos(glm::vec3(SceneActor.Translate.GetX(), SceneActor.Translate.GetY(), SceneActor.Translate.GetZ()));

					if (StaticCompound == nullptr)
					{
						Logger::Error("Exporter", "Could not bake water cylinder, position specific static compound was a nullptr");
						continue;
					}

					PhiveStaticCompound::PhiveStaticCompoundWaterCylinder WaterCylinder;
					WaterCylinder.mCompoundId = 0;
					WaterCylinder.mReserve0 = 0;
					WaterCylinder.mFlowSpeed = SceneActor.mWaterFlowSpeed;
					WaterCylinder.mReserve2 = 1.0f;
					WaterCylinder.mScaleTimesTenX = Scale.value().x;
					WaterCylinder.mScaleTimesTenY = SceneActor.mWaterBoxHeight;
					WaterCylinder.mScaleTimesTenZ = Scale.value().y;
					WaterCylinder.mRotationX = Util::DegreesToRadians(SceneActor.Rotate.GetX());
					WaterCylinder.mRotationY = Util::DegreesToRadians(SceneActor.Rotate.GetY());
					WaterCylinder.mRotationZ = Util::DegreesToRadians(SceneActor.Rotate.GetZ());
					WaterCylinder.mPositionX = SceneActor.Translate.GetX();
					WaterCylinder.mPositionY = SceneActor.Translate.GetY();
					WaterCylinder.mPositionZ = SceneActor.Translate.GetZ();
					WaterCylinder.mFluidType = PhiveStaticCompound::ActorFluidTypeToInternalFluidType(SceneActor.mWaterFluidType);
					WaterCylinder.mReserve7 = 0;
					WaterCylinder.mReserve8 = 0;
					WaterCylinder.mReserve9 = 0;
					StaticCompound->mWaterCylinderArray.push_back(WaterCylinder);
					break;
				}
				default:
					Logger::Error("Exporter", "Invalid fluid shape type: " + std::to_string((uint32_t)SceneActor.mWaterShapeType));
					break;
				}
			}
		}
		Util::CreateDir(Path + "/" + Editor::StaticCompoundDirectory);

		auto WriteBphsc = [&Path, &StaticCompoundToRebuild](PhiveStaticCompound& File, std::string FileNameSuffix = "")
			{
				if (!StaticCompoundToRebuild[&File])
					return;

				//std::vector<unsigned char> Data = File.ToBinary();
				//Util::WriteFile(Path + "/" + Editor::StaticCompoundPrefix + Editor::Identifier + FileNameSuffix + ".DEBUG.bphsc", Data);
				ZStdFile::Compress(File.ToBinary(), ZStdFile::Dictionary::Base).WriteToFile(Path + "/" + Editor::StaticCompoundPrefix + Editor::Identifier + FileNameSuffix + ".Nin_NX_NVN.bphsc.zs");
			};

		switch (SceneMgr::SceneType)
		{
		case SceneMgr::Type::MainField:
		case SceneMgr::Type::MinusField:
			WriteBphsc(SceneMgr::mStaticCompounds[0], "_X0_Z0");
			WriteBphsc(SceneMgr::mStaticCompounds[1], "_X1_Z0");
			WriteBphsc(SceneMgr::mStaticCompounds[2], "_X2_Z0");
			WriteBphsc(SceneMgr::mStaticCompounds[3], "_X3_Z0");
			WriteBphsc(SceneMgr::mStaticCompounds[4], "_X0_Z1");
			WriteBphsc(SceneMgr::mStaticCompounds[5], "_X1_Z1");
			WriteBphsc(SceneMgr::mStaticCompounds[6], "_X2_Z1");
			WriteBphsc(SceneMgr::mStaticCompounds[7], "_X3_Z1");
			WriteBphsc(SceneMgr::mStaticCompounds[8], "_X0_Z2");
			WriteBphsc(SceneMgr::mStaticCompounds[9], "_X1_Z2");
			WriteBphsc(SceneMgr::mStaticCompounds[10], "_X2_Z2");
			WriteBphsc(SceneMgr::mStaticCompounds[11], "_X3_Z2");
			WriteBphsc(SceneMgr::mStaticCompounds[12], "_X0_Z3");
			WriteBphsc(SceneMgr::mStaticCompounds[13], "_X1_Z3");
			WriteBphsc(SceneMgr::mStaticCompounds[14], "_X2_Z3");
			WriteBphsc(SceneMgr::mStaticCompounds[15], "_X3_Z3");
			break;
		case SceneMgr::Type::SkyIslands:
		case SceneMgr::Type::LargeDungeon:
		case SceneMgr::Type::SmallDungeon:
		case SceneMgr::Type::NormalStage:
		{
			WriteBphsc(SceneMgr::mStaticCompounds[0]);
		}
		default:
			break;
		}
	}

#ifndef STARLIGHT_SHARED
	GameDataListMgr::RebuildAndSave();
#endif // !STARLIGHT_SHARED

	if (Editor::Identifier.length() != 0)
	{
		BymlFile& StaticByml = Editor::StaticActorsByml;
		BymlFile& DynamicByml = Editor::DynamicActorsByml;

		StaticByml.GetNode("Actors")->GetChildren().clear();
		DynamicByml.GetNode("Actors")->GetChildren().clear();

		for (Actor& ExportedActor : ActorMgr::GetActors())
		{
			if (ExportedActor.Dynamic.count("BancPath"))
			{
				if (ExportedActor.Dynamic["BancPath"].Type != BymlFile::Type::StringIndex)
					goto SkipMergedActorProcessing;

				std::string BancPath = std::get<std::string>(ExportedActor.Dynamic["BancPath"].Data);

				if(ExportedActor.MergedActorByml.HasChild("Actors")) ExportedActor.MergedActorByml.GetNode("Actors")->GetChildren().clear();
				else
				{
					BymlFile::Node ActorsNode(BymlFile::Type::Array, "Actors");
					ExportedActor.MergedActorByml.GetNodes().push_back(ActorsNode);
					ExportedActor.MergedActorByml.GetType() = BymlFile::Type::Dictionary;
				}
				for (Actor MergedActor : ExportedActor.MergedActorContent)
				{
					Vector3F RadianRotate(Util::DegreesToRadians(-ExportedActor.Rotate.GetX()), Util::DegreesToRadians(-ExportedActor.Rotate.GetY()), Util::DegreesToRadians(-ExportedActor.Rotate.GetZ()));

					float NewX = ExportedActor.Translate.GetX() + ((MergedActor.Translate.GetX() - ExportedActor.Translate.GetX()) * (std::cosf(RadianRotate.GetZ()) * std::cosf(RadianRotate.GetY()))) +
						((MergedActor.Translate.GetY() - ExportedActor.Translate.GetY()) * (std::cosf(RadianRotate.GetZ()) * std::sinf(RadianRotate.GetY()) * std::sinf(RadianRotate.GetX()) - std::sinf(RadianRotate.GetZ()) * std::cosf(RadianRotate.GetX()))) +
						((MergedActor.Translate.GetZ() - ExportedActor.Translate.GetZ()) * (std::cosf(RadianRotate.GetZ()) * std::sinf(RadianRotate.GetY()) * std::cosf(RadianRotate.GetX()) + std::sinf(RadianRotate.GetZ()) * std::sinf(RadianRotate.GetX())));

					float NewY = ExportedActor.Translate.GetY() + ((MergedActor.Translate.GetX() - ExportedActor.Translate.GetX()) * (std::sinf(RadianRotate.GetZ()) * std::cosf(RadianRotate.GetY()))) +
						((MergedActor.Translate.GetY() - ExportedActor.Translate.GetY()) * (std::sinf(RadianRotate.GetZ()) * std::sinf(RadianRotate.GetY()) * std::sinf(RadianRotate.GetX()) + std::cosf(RadianRotate.GetZ()) * std::cosf(RadianRotate.GetX()))) +
						((MergedActor.Translate.GetZ() - ExportedActor.Translate.GetZ()) * (std::sinf(RadianRotate.GetZ()) * std::sinf(RadianRotate.GetY()) * std::cosf(RadianRotate.GetX()) - std::cosf(RadianRotate.GetZ()) * std::sinf(RadianRotate.GetX())));

					float NewZ = ExportedActor.Translate.GetZ() + ((MergedActor.Translate.GetX() - ExportedActor.Translate.GetX()) * (-std::sinf(RadianRotate.GetY()))) +
						((MergedActor.Translate.GetY() - ExportedActor.Translate.GetY()) * (std::cosf(RadianRotate.GetY()) * std::sinf(RadianRotate.GetX()))) +
						((MergedActor.Translate.GetZ() - ExportedActor.Translate.GetZ()) * (std::cosf(RadianRotate.GetY()) * std::cosf(RadianRotate.GetX())));

					MergedActor.Translate.SetX(NewX);
					MergedActor.Translate.SetY(NewY);
					MergedActor.Translate.SetZ(NewZ);

					MergedActor.Translate.SetX(MergedActor.Translate.GetX() - ExportedActor.Translate.GetX());
					MergedActor.Translate.SetY(MergedActor.Translate.GetY() - ExportedActor.Translate.GetY());
					MergedActor.Translate.SetZ(MergedActor.Translate.GetZ() - ExportedActor.Translate.GetZ());

					MergedActor.Scale.SetX(MergedActor.Scale.GetX() / ExportedActor.Scale.GetX());
					MergedActor.Scale.SetY(MergedActor.Scale.GetY() / ExportedActor.Scale.GetY());
					MergedActor.Scale.SetZ(MergedActor.Scale.GetZ() / ExportedActor.Scale.GetZ());

					MergedActor.Rotate.SetX(MergedActor.Rotate.GetX() + ExportedActor.Rotate.GetX());
					MergedActor.Rotate.SetY(MergedActor.Rotate.GetY() + ExportedActor.Rotate.GetY());
					MergedActor.Rotate.SetZ(MergedActor.Rotate.GetZ() + ExportedActor.Rotate.GetZ());

					/*
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
					*/

					ExportedActor.MergedActorByml.GetNode("Actors")->AddChild(ActorMgr::ActorToByml(MergedActor));
				}
				size_t Found = BancPath.find_last_of("/\\");
				Util::CreateDir(Path + "/" + BancPath.substr(0, Found));
				ZStdFile::Compress(ExportedActor.MergedActorByml.ToBinary(BymlFile::TableGeneration::Auto), ZStdFile::Dictionary::BcettByaml).WriteToFile(Path + "/" + BancPath + ".zs");
				//ExportedActor.Scale = Vector3F(1, 1, 1);
			}
		SkipMergedActorProcessing:

			if (ExportedActor.ActorType == Actor::Type::Static)
			{
				StaticByml.GetNode("Actors")->AddChild(ActorMgr::ActorToByml(ExportedActor));
			}
			else if (ExportedActor.ActorType == Actor::Type::Dynamic)
			{
				DynamicByml.GetNode("Actors")->AddChild(ActorMgr::ActorToByml(ExportedActor));
			}
		}

		ZStdFile::Compress(StaticByml.ToBinary(BymlFile::TableGeneration::Auto), ZStdFile::Dictionary::BcettByaml).WriteToFile(Path + "/" + Editor::BancPrefix + Editor::Identifier + "_Static.bcett.byml.zs");
		ZStdFile::Compress(DynamicByml.ToBinary(BymlFile::TableGeneration::Auto), ZStdFile::Dictionary::BcettByaml).WriteToFile(Path + "/" + Editor::BancPrefix + Editor::Identifier + "_Dynamic.bcett.byml.zs");
	}

Copy:

	if (Op == Exporter::Operation::Export)
	{
		CreateRSTBL(Path);
		CreateExportOnlyFiles(Path);

		Util::CopyDirectoryRecursively(Editor::GetWorkingDirFile("Save"), OldPath);
	}

	UIActorTool::UpdateActorList();
}