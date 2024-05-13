#include "ProjectRebuilder.h"

#include "Logger.h"
#include "Exporter.h"
#include "Editor.h"
#include "Byml.h"
#include "ZStdFile.h"
#include "Util.h"
#include <iostream>
#include <filesystem>

BymlFile::Node* GetPivotDataNode(BymlFile::Node& ActorNode)
{
	if (ActorNode.HasChild("Phive"))
	{
		BymlFile::Node* PhiveNode = ActorNode.GetChild("Phive");
		if (PhiveNode->HasChild("ConstraintLink"))
		{
			BymlFile::Node* ConstraintLinkNode = PhiveNode->GetChild("ConstraintLink");
			if (ConstraintLinkNode->HasChild("Owners"))
			{
				BymlFile::Node* OwnersNode = ConstraintLinkNode->GetChild("Owners");

				for (BymlFile::Node& OwnerChild : OwnersNode->GetChildren())
				{
					if (OwnerChild.HasChild("PivotData"))
					{
						return OwnerChild.GetChild("PivotData");
					}
				}
			}
		}
	}

	return nullptr;
}

void ProjectRebuilder::RebuildProject()
{
	Logger::Info("ProjectRebuilder", "Rebuilding project... (takes some time)");

	uint32_t FixedEntries = 0;
	uint32_t NotFixable = 0;
	Exporter::Export(Editor::GetWorkingDirFile("Save"), Exporter::Operation::Save);

	using recursive_directory_iterator = std::filesystem::recursive_directory_iterator;
	for (const auto& DirEntry : recursive_directory_iterator(Editor::GetWorkingDirFile("Save/Banc")))
	{
		if (DirEntry.path().string().ends_with(".bcett.byml.zs"))
		{
			std::string LocalRomFSPath = DirEntry.path().string();
			Util::ReplaceString(LocalRomFSPath, Editor::GetWorkingDirFile("Save/"), "");
			BymlFile MapFile(ZStdFile::Decompress(DirEntry.path().string(), ZStdFile::Dictionary::BcettByaml).Data);
			if (MapFile.HasChild("Actors"))
			{
				for (BymlFile::Node& ActorNode : MapFile.GetNode("Actors")->GetChildren())
				{
					BymlFile::Node* PivotDataNode = GetPivotDataNode(ActorNode);
					if (PivotDataNode != nullptr)
					{
						//NEEDS FIXING
						if (PivotDataNode->GetChildren().empty())
						{
							bool Fixed = false;
							BymlFile VanillaMapFile(ZStdFile::Decompress(Editor::GetRomFSFile(LocalRomFSPath, false), ZStdFile::Dictionary::BcettByaml).Data);
							for (BymlFile::Node& VanillaActorNode : VanillaMapFile.GetNode("Actors")->GetChildren())
							{
								if (VanillaActorNode.GetChild("Gyaml")->GetValue<std::string>() == ActorNode.GetChild("Gyaml")->GetValue<std::string>() &&
									VanillaActorNode.GetChild("Hash")->GetValue<uint64_t>() == ActorNode.GetChild("Hash")->GetValue<uint64_t>())
								{
									BymlFile::Node* VanillaPivotDataNode = GetPivotDataNode(VanillaActorNode);
									if (VanillaPivotDataNode != nullptr)
									{
										PivotDataNode->GetChildren() = VanillaPivotDataNode->GetChildren();
										FixedEntries++;
										Fixed = true;
									}
									break;
								}
							}

							if (!Fixed)
							{
								Logger::Error("ProjectRebuilder", "Could not fix PivotData in " + LocalRomFSPath);
								NotFixable++;
							}
						}
					}
				}
			}
			ZStdFile::Compress(MapFile.ToBinary(BymlFile::TableGeneration::Auto), ZStdFile::Dictionary::BcettByaml).WriteToFile(Editor::GetWorkingDirFile("Save/" + LocalRomFSPath));
			Logger::Info("ProjectRebuilder", "Processed " + LocalRomFSPath);
		}
	}		

	Logger::Info("ProjectRebuilder", "Project rebuilt, " + std::to_string(FixedEntries) + " entries have been fixed, " + std::to_string(NotFixable) + " entries could not be fixed");
}