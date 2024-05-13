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

	std::string ActorName = "Starlight_Collision_" + CollisionActorParent.Gyml;

	SarcFile CollisionActorPack(ZStdFile::Decompress(Editor::GetRomFSFile("Pack/Actor/AirWall.pack.zs"), ZStdFile::Dictionary::Pack).Data);
	for (SarcFile::Entry& Entry : CollisionActorPack.GetEntries())
	{
		if (((Entry.Bytes[0] == 'Y' && Entry.Bytes[1] == 'B') || (Entry.Bytes[0] == 'B' && Entry.Bytes[1] == 'Y')) && !Entry.Name.starts_with("Component/ModelInfo/"))
		{
			std::cout << "Renaming " << Entry.Name << std::endl;
			BymlFile Byml(Entry.Bytes);
			for (BymlFile::Node& Node : Byml.GetNodes())
			{
				ReplaceStringInBymlNodes(&Node, "AirWall", ActorName);
			}
			Entry.Bytes = Byml.ToBinary(BymlFile::TableGeneration::Auto);
		}

		Util::ReplaceString(Entry.Name, "AirWall", ActorName);
	}
	
	std::vector<SarcFile::Entry> CollisionActorPackEntries = CollisionActorPack.GetEntries();
	for (auto Iter = CollisionActorPackEntries.begin(); Iter != CollisionActorPackEntries.cend(); )
	{
		if (Iter->Name.starts_with("Phive/"))
		{
			Iter = CollisionActorPackEntries.erase(Iter);
			continue;
		}
		++Iter;
	}

	for (SarcFile::Entry& Entry : ActorPack.GetEntries())
	{
		if (Entry.Name.starts_with("Phive/"))
		{
			SarcFile::Entry NewEntry = Entry;
			Util::ReplaceString(NewEntry.Name, CollisionActorParent.Gyml, ActorName);

			if ((NewEntry.Bytes[0] == 'Y' && NewEntry.Bytes[1] == 'B') || (NewEntry.Bytes[0] == 'B' && NewEntry.Bytes[1] == 'Y'))
			{
				BymlFile Byml(NewEntry.Bytes);
				for (BymlFile::Node& Node : Byml.GetNodes())
				{
					ReplaceStringInBymlNodes(&Node, CollisionActorParent.Gyml, ActorName, ".phsh");
				}
				NewEntry.Bytes = Byml.ToBinary(BymlFile::TableGeneration::Auto);
			}

			CollisionActorPackEntries.push_back(NewEntry);
		}
	}

	CollisionActorPack.GetEntries() = CollisionActorPackEntries;

	if (!Util::FileExists(Editor::GetWorkingDirFile("Save/Pack/Actor")))
	{
		Util::CreateDir(Editor::GetWorkingDirFile("Save/Pack/Actor"));
	}
	ZStdFile::Compress(CollisionActorPack.ToBinary(), ZStdFile::Dictionary::Pack).WriteToFile(Editor::GetWorkingDirFile("Save/Pack/Actor/" + ActorName + ".pack.zs"));

	Actor& NewActor = ActorMgr::AddActor(ActorName);
	NewActor.ActorType = Actor::Type::Dynamic;
	NewActor.Translate = CollisionActorParent.Translate;
	NewActor.Rotate = CollisionActorParent.Rotate;
	NewActor.Scale = CollisionActorParent.Scale;
}