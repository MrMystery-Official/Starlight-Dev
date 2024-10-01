#include "ActorCollisionCreator.h"

#include "Editor.h"
#include "SARC.h"
#include "ZStdFile.h"
#include "Logger.h"
#include "Util.h"
#include "ActorMgr.h"
#include <iostream>

void ActorCollisionCreator::ReplaceStringInBymlNodes(BymlFile::Node* Node, std::string Search, std::string Replace, std::string DoesNotContain) {
	Util::ReplaceString(Node->GetKey(), Search, Replace);
	if (Node->GetType() == BymlFile::Type::StringIndex)
	{
		std::string Value = Node->GetValue<std::string>();
		if (!DoesNotContain.empty() && Value.find(DoesNotContain) != std::string::npos)
		{
			goto ProcessChilds;
		}
		Util::ReplaceString(Value, Search, Replace);
		Node->SetValue<std::string>(Value);
	}

	ProcessChilds:
	for (BymlFile::Node& Child : Node->GetChildren()) {
		ReplaceStringInBymlNodes(&Child, Search, Replace, DoesNotContain);
	}
}

void ReplaceStringInBymlNodesActorInfo(BymlFile::Node* Node, std::string Search, std::string Replace) {
	
	if (Node->GetKey() == "FmdbName" || Node->GetKey() == "ModelProjectName")
		goto ProcessChilds;

	Util::ReplaceString(Node->GetKey(), Search, Replace);
	if (Node->GetType() == BymlFile::Type::StringIndex)
	{
		std::string Value = Node->GetValue<std::string>();
		Util::ReplaceString(Value, Search, Replace);
		Node->SetValue<std::string>(Value);
	}

ProcessChilds:
	for (BymlFile::Node& Child : Node->GetChildren()) {
		ReplaceStringInBymlNodesActorInfo(&Child, Search, Replace);
	}
}

void ReplaceLocalPathsInBymlNodes(BymlFile::Node* Node, std::string Search, std::string Replace, SarcFile& Sarc) {
	Util::ReplaceString(Node->GetKey(), Search, Replace);
	if (Node->GetType() == BymlFile::Type::StringIndex)
	{
		std::string Value = Node->GetValue<std::string>();

		if (Value.starts_with("?"))
		{
			std::string Path = Value.substr(1, Value.size() - 1);
			bool Found = false;
			for (SarcFile::Entry& Entry : Sarc.GetEntries())
			{
				if (Entry.Name.starts_with(Path))
				{
					Found = true;
					break;
				}
			}
			if (!Found)
				goto ProcessChilds;
		}

		Util::ReplaceString(Value, Search, Replace);
		Node->SetValue<std::string>(Value);
	}

ProcessChilds:
	for (BymlFile::Node& Child : Node->GetChildren()) {
		ReplaceLocalPathsInBymlNodes(&Child, Search, Replace, Sarc);
	}
}

void ActorCollisionCreator::AddCollisionActor(Actor& CollisionActorParent)
{
	ZStdFile::Result Result = ZStdFile::Decompress(Editor::GetRomFSFile("Pack/Actor/" + CollisionActorParent.Gyml + ".pack.zs"), ZStdFile::Dictionary::Pack);
	if (Result.Data.size() == 0)
	{
		Logger::Error("ActorCollisionCreator", "Could not open CollisionActorParent pack, error while decompressing");
		return;
	}

	SarcFile ActorPack(Result.Data);
	if (ActorPack.GetEntries().empty())
	{
		Logger::Error("ActorCollisionCreator", "Could not decode actor pack");
		return;
	}

	SarcFile OriginalActorPack = ActorPack;

	std::string ActorName = "C" + CollisionActorParent.Gyml;

	for (SarcFile::Entry& Entry : ActorPack.GetEntries())
	{
		if (((Entry.Bytes[0] == 'Y' && Entry.Bytes[1] == 'B') || (Entry.Bytes[0] == 'B' && Entry.Bytes[1] == 'Y')) && 
			!Entry.Name.starts_with("Component/ModelInfo/") &&
			!Entry.Name.starts_with("Phive/ShapeParam/"))
		{
			std::cout << "Renaming " << Entry.Name << std::endl;
			BymlFile Byml(Entry.Bytes);
			for (BymlFile::Node& Node : Byml.GetNodes())
			{
				if (Entry.Name == "Actor/" + CollisionActorParent.Gyml + ".engine__actor__ActorParam.bgyml")
				{
					ReplaceLocalPathsInBymlNodes(&Node, CollisionActorParent.Gyml, ActorName, OriginalActorPack);
				}
				else
				{
					ReplaceStringInBymlNodes(&Node, CollisionActorParent.Gyml, ActorName);
				}
			}
			Entry.Bytes = Byml.ToBinary(BymlFile::TableGeneration::Auto);
		}

		Util::ReplaceString(Entry.Name, CollisionActorParent.Gyml, ActorName);
	}

	//Now that we have a working clone of the actor, lets make a non-static collision
	for (SarcFile::Entry& Entry : ActorPack.GetEntries())
	{
		if (Entry.Name.starts_with("Phive/ControllerSetParam/"))
		{
			bool Changed = false;
			BymlFile Byml(Entry.Bytes);
			for (BymlFile::Node& CtrNode : Byml.GetNodes())
			{
				if (CtrNode.GetKey() == "ControllerEntityNamePathAry")
				{
					for (BymlFile::Node& CtrChildNode : CtrNode.GetChildren())
					{
						if (CtrChildNode.HasChild("Name"))
						{
							CtrChildNode.GetChild("Name")->SetValue<std::string>("Body");
						}
					}

					Changed = true;
				}
			}
			if (Changed)
			{
				Entry.Bytes = Byml.ToBinary(BymlFile::TableGeneration::Auto);
				break;
			}
		}
	}

	if (!Util::FileExists(Editor::GetWorkingDirFile("Save/Pack/Actor")))
	{
		Util::CreateDir(Editor::GetWorkingDirFile("Save/Pack/Actor"));
	}
	ZStdFile::Compress(ActorPack.ToBinary(), ZStdFile::Dictionary::Pack).WriteToFile(Editor::GetWorkingDirFile("Save/Pack/Actor/" + ActorName + ".pack.zs"));

	//Now that the collision actor is written, we need to regenerate the ActorInfo RSDB. This process takes the most amount of time, ~4 seconds
	BymlFile ActorInfo(ZStdFile::Decompress(Editor::GetRomFSFile("RSDB/ActorInfo.Product." + Editor::GetInternalGameVersion() + ".rstbl.byml.zs"), ZStdFile::Dictionary::Base).Data);

	//If the same actor has already been created, skip the registering proccess
	bool FoundSameActorName = false;
	BymlFile::Node OriginalActorEntry(BymlFile::Type::Null, "NULL");
	for (BymlFile::Node& Node : ActorInfo.GetNodes())
	{
		if (Node.HasChild("__RowId"))
		{
			if (Node.GetChild("__RowId")->GetValue<std::string>() == ActorName)
			{
				FoundSameActorName = true;
				break;
			}
			if (Node.GetChild("__RowId")->GetValue<std::string>() == CollisionActorParent.Gyml)
			{
				OriginalActorEntry = Node;
			}
		}
	}
	if (OriginalActorEntry.GetType() != BymlFile::Type::Null)
	{
		if (!FoundSameActorName)
		{
			ReplaceStringInBymlNodesActorInfo(&OriginalActorEntry, CollisionActorParent.Gyml, ActorName);
			ActorInfo.GetNodes().push_back(OriginalActorEntry);
			if (!Util::FileExists(Editor::GetWorkingDirFile("Save/RSDB")))
			{
				Util::CreateDir(Editor::GetWorkingDirFile("Save/RSDB"));
			}
			//To make writing faster, we manage the StringTable manually
			ActorInfo.AddStringTableEntry(ActorName);
			ZStdFile::Compress(ActorInfo.ToBinary(BymlFile::TableGeneration::Manual), ZStdFile::Dictionary::Base).WriteToFile(Editor::GetWorkingDirFile("Save/RSDB/ActorInfo.Product." + Editor::GetInternalGameVersion() + ".rstbl.byml.zs"));
		}
		else
		{
			Logger::Info("ActorCollisionCreator", "No need to regenerate ActorInfo RSDB");
		}
	}
	else
	{
		Logger::Error("ActorCollisionCreator", "Could not regenerate ActorInfo RSDB");
	}

	CollisionActorParent.Gyml = ActorName;
}