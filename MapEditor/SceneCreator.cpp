#include "SceneCreator.h"

#include "Util.h"
#include "Editor.h"
#include "ZStdFile.h"
#include "AINB.h"
#include <iostream>

void SceneCreator::ReplaceStringInBymlNodes(BymlFile::Node* Node, std::string Search, std::string Replace, std::string DoesNotContain) {
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

void SceneCreator::CreateScene(std::string Identifier, std::string Template)
{
	auto CopyFileSimple = [&, Identifier, Template](std::string Directory, std::string Prefix, std::string Suffix)
		{
			if (!Util::FileExists(Editor::GetRomFSFile(Directory + "/" + Prefix + Template + Suffix)))
				return;

			if (Util::FileExists(Editor::GetWorkingDirFile("Save/" + Directory + "/" + Prefix + Identifier + Suffix)))
				Util::RemoveFile(Editor::GetWorkingDirFile("Save/" + Directory + "/" + Prefix + Identifier + Suffix));

			Util::CreateDir(Editor::GetWorkingDirFile("Save/" + Directory));
			Util::CopyFileToDest(Editor::GetRomFSFile(Directory + "/" + Prefix + Template + Suffix), Editor::GetWorkingDirFile("Save/" + Directory + "/" + Prefix + Identifier + Suffix));
		};

	auto ConvertByml = [&, Identifier, Template](std::string Path, bool Decompress = true, ZStdFile::Dictionary Dict = ZStdFile::Dictionary::Base, bool MapFile = false)
		{
			BymlFile File(Decompress ? ZStdFile::Decompress(Editor::GetRomFSFile(Path), Dict).Data : Util::ReadFile(Editor::GetRomFSFile(Path)));
			if (MapFile)
			{
				if (File.HasChild("AiGroups"))
					ReplaceStringInBymlNodes(File.GetNode("AiGroups"), "Dungeon" + Template, "Dungeon" + Identifier);
			}
			else
			{
				for (BymlFile::Node& Node : File.GetNodes())
				{
					ReplaceStringInBymlNodes(&Node, "Dungeon" + Template, "Dungeon" + Identifier);
				}
			}

			if (Decompress)
				ZStdFile::Compress(File.ToBinary(BymlFile::TableGeneration::Auto), Dict).WriteToFile(Editor::GetRomFSFile(Path));
			else
				File.WriteToFile(Editor::GetRomFSFile(Path), BymlFile::TableGeneration::Auto);
		};

	auto ProcessSceneAINBs = [&, Identifier, Template](std::string Path)
		{
			BymlFile File(ZStdFile::Decompress(Editor::GetRomFSFile(Path), ZStdFile::Dictionary::BcettByaml).Data);

			if (File.HasChild("AiGroups"))
			{
				for (BymlFile::Node& Child : File.GetNode("AiGroups")->GetChildren())
				{
					std::string LogicMetaKey = Child.HasChild("Logic") ? "Logic" : "Meta";
					if (LogicMetaKey == "Logic")
					{
						std::string LogicPrefix = "";
						std::string Path = Child.GetChild(LogicMetaKey)->GetValue<std::string>();
						if (Path.starts_with("Logic/"))
						{
							LogicPrefix = "Logic/";
						}
						else if (Path.starts_with("AI/"))
						{
							LogicPrefix = "AI/";
						}
						Path = Path.substr(Path.find_last_of('/') + 1);

						if (!Path.starts_with("Dungeon" + Template + "_"))
							continue;

						std::string NewName = Path + "b";

						Util::ReplaceString(NewName, "Dungeon" + Template, "Dungeon" + Identifier);

						if (Util::FileExists(Editor::GetWorkingDirFile("Save/" + LogicPrefix + NewName)))
							Util::RemoveFile(Editor::GetWorkingDirFile("Save/" + LogicPrefix + NewName));

						Util::CreateDir(Editor::GetWorkingDirFile("Save/" + LogicPrefix));
						Util::CopyFileToDest(Editor::GetRomFSFile(LogicPrefix + Path + "b"), Editor::GetWorkingDirFile("Save/" + LogicPrefix + NewName));

						AINBFile AINB(Editor::GetRomFSFile(LogicPrefix + NewName));
						NewName.erase(NewName.size() - 5, 5);
						AINB.Header.FileName = NewName;
						AINB.Write(Editor::GetRomFSFile(LogicPrefix + NewName));
					}
				}
			}
		};

	CopyFileSimple("Bake/Scene", "Dungeon", ".bkres.zs");

	CopyFileSimple("Banc/GameBancParam", "Dungeon", ".game__banc__GameBancParam.bgyml");
	ConvertByml("Banc/GameBancParam/Dungeon" + Identifier + ".game__banc__GameBancParam.bgyml", false);

	CopyFileSimple("Banc/SmallDungeon", "Dungeon", "_Dynamic.bcett.byml.zs");
	CopyFileSimple("Banc/SmallDungeon", "Dungeon", "_Static.bcett.byml.zs");
	ProcessSceneAINBs("Banc/SmallDungeon/Dungeon" + Identifier + "_Static.bcett.byml.zs");
	ConvertByml("Banc/SmallDungeon/Dungeon" + Identifier + "_Dynamic.bcett.byml.zs", true, ZStdFile::Dictionary::BcettByaml);
	ConvertByml("Banc/SmallDungeon/Dungeon" + Identifier + "_Static.bcett.byml.zs", true, ZStdFile::Dictionary::BcettByaml);
	
	CopyFileSimple("Phive/NavMesh", "Dungeon", ".Nin_NX_NVN.bphnm.zs");
	CopyFileSimple("Phive/StaticCompoundBody/SmallDungeon", "Dungeon", ".Nin_NX_NVN.bphsc.zs");

	CopyFileSimple("Scene/Component/SmallDungeonParam", "Dungeon", ".game__scene__SmallDungeonParam.bgyml");
	ConvertByml("Scene/Component/SmallDungeonParam/Dungeon" + Identifier + ".game__scene__SmallDungeonParam.bgyml", false);
	
	CopyFileSimple("TexToGo", "RBake_Scene_Dungeon", "_d0_t0.txtg");

	BymlFile StartPosByml(ZStdFile::Decompress(Editor::GetRomFSFile("Banc/SmallDungeon/StartPos/SmallDungeon.startpos.byml.zs"), ZStdFile::Dictionary::Base).Data);
	if (StartPosByml.GetNode("OnElevator")->HasChild("Dungeon" + Template))
	{
		BymlFile::Node StartPosNode = *StartPosByml.GetNode("OnElevator")->GetChild("Dungeon" + Template);
		StartPosNode.m_Key = "Dungeon" + Identifier;
		if (StartPosByml.GetNode("OnElevator")->HasChild("Dungeon" + Identifier))
		{
			*StartPosByml.GetNode("OnElevator")->GetChild("Dungeon" + Identifier) = StartPosNode;
		}
		else
		{
			StartPosByml.GetNode("OnElevator")->AddChild(StartPosNode);
		}
	}

	Util::CreateDir(Editor::GetWorkingDirFile("Save/Banc/SmallDungeon/StartPos"));
	ZStdFile::Compress(StartPosByml.ToBinary(BymlFile::TableGeneration::Auto), ZStdFile::Dictionary::Base).WriteToFile(Editor::GetWorkingDirFile("Save/Banc/SmallDungeon/StartPos/SmallDungeon.startpos.byml.zs"));
}