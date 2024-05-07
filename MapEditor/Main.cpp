#include "UI.h"
#include "ZStdFile.h"
#include "Bfres.h"
#include "Editor.h"
#include "SceneMgr.h"
#include "Logger.h"
#include "Util.h"
#include <filesystem>
#include "ActorMgr.h"
#include "EditorConfig.h"
#include "UIAINBEditor.h"
#include "AINBNodeDefMgr.h"
#include "MSBT.h"
#include "PhiveShape.h"
#include <iostream>
#include "BHTMP.h"
#include "UIActorTool.h"
#include "PreferencesConfig.h"

#include <fstream>

/*
void SearchForBytes()
{
	unsigned char BytesNum[] = "TmbMesh";

	using recursive_directory_iterator = std::filesystem::recursive_directory_iterator;
	for (const auto& dirEntry : recursive_directory_iterator(Editor::GetRomFSFile("", false)))
	{
		if (dirEntry.is_regular_file())
		{
			std::vector<unsigned char> Bytes;
			if (dirEntry.path().string().ends_with(".zs"))
			{
				Bytes = ZStdFile::Decompress(dirEntry.path().string(), ZStdFile::Dictionary::Base).Data;
			}
			else
			{
				std::ifstream File(dirEntry.path().string(), std::ios::binary);

				if (!File.eof() && !File.fail())
				{
					File.seekg(0, std::ios_base::end);
					std::streampos FileSize = File.tellg();

					Bytes.resize(FileSize);

					File.seekg(0, std::ios_base::beg);
					File.read(reinterpret_cast<char*>(Bytes.data()), FileSize);

					File.close();
				}
				else
				{
					std::cerr << "Faild to open file " << dirEntry.path().string() << std::endl;
				}
			}

			if (Bytes.size() < sizeof(uint64_t)) continue;

			// Check if the byte sequence is contained in the vector
			for (size_t i = 0; i <= Bytes.size() - sizeof(uint64_t); ++i) {
				if (memcmp(&Bytes[i], BytesNum, sizeof(uint64_t)) == 0) {
					std::cout << "Found in file " << dirEntry.path().string() << std::endl;
					break;
				}
			}
		}
	}
}
*/


int main()
{
	Util::CreateDir(Editor::GetWorkingDirFile("Save"));
	Util::CreateDir(Editor::GetWorkingDirFile("Cache"));
	Util::CreateDir(Editor::GetWorkingDirFile("EditorModels"));

	UI::Initialize();
	BfresLibrary::Initialize();
	
	if (Util::FileExists(Editor::GetWorkingDirFile("Editor.edtc")))
	{
		Editor::InitializeWithEdtc();
	}
	UIAINBEditor::Initialize();
	AINBNodeDefMgr::Initialize();

	if (Util::FileExists(Editor::GetWorkingDirFile("Preferences.eprf")))
	{
		PreferencesConfig::Load();
	}

	/*
	for (const auto& Entry : std::filesystem::directory_iterator("/romfs/upd_v1.2.1/UI/Map/HeightMap"))
	{
		if (Entry.path().filename().string().starts_with("G_"))
		{
			BHTMPFile(ZStdFile::Decompress(Entry.path().string(), ZStdFile::Dictionary::Base).Data, Entry.path().filename().string());
		}
	}
	*/

	//MSBTFile MSBT("MapEditorV4/Workspace/Npc_kokiri006.msbt");

	//EffectFile EFile(ZStdFile::Decompress(Editor::GetRomFSFile("Effect/BeamosBeam.Nin_NX_NVN.esetb.byml.zs"), ZStdFile::Dictionary::Base).Data);

	//SearchForBytes();

	//AINBNodeDefMgr::Generate();

	//SceneMgr::LoadScene(SceneMgr::Type::SmallDungeon, "001");

	//ActorMgr::AddActor("DgnObj_Small_BoxPillarStart_A_01");

	/*
	ActorMgr::AddActor("Obj_TreeApple_A_L_01");
	ActorMgr::AddActor("Obj_TreeApple_A_L_01");
	ActorMgr::AddActor("Enemy_Drake");
	ActorMgr::AddActor("Enemy_Drake");
	*/

	/*

	PhiveShape Shape(ZStdFile::Decompress(Editor::GetRomFSFile("Phive/Shape/Dcc/DgnObj_Small_BoxFloor_4x4_A__Physical.Nin_NX_NVN.bphsh.zs"), ZStdFile::Dictionary::Base).Data);
	//PhiveShape Shape(ZStdFile::Decompress(Editor::GetRomFSFile("Phive/Shape/Dcc/DgnObj_Small_BoxWall_A_21__Physical.Nin_NX_NVN.bphsh.zs"), ZStdFile::Dictionary::Base).Data);
	//PhiveShape Shape(ZStdFile::Decompress(Editor::GetRomFSFile("Phive/Shape/Dcc/DgnObj_Small_BoxFloor_2x2_A__Physical.Nin_NX_NVN.bphsh.zs"), ZStdFile::Dictionary::Base).Data);
	//PhiveShape Shape(ZStdFile::Decompress(Editor::GetRomFSFile("Phive/Shape/Dcc/FldObj_AncientPillar_A_01__Physical.Nin_NX_NVN.bphsh.zs"), ZStdFile::Dictionary::Base).Data);
	//PhiveShape Shape(ZStdFile::Decompress(Editor::GetWorkingDirFile("Save/Phive/Shape/Dcc/Test.bphsh.zs"), ZStdFile::Dictionary::Base).Data);
	//PhiveShape Shape(ZStdFile::Decompress(Editor::GetRomFSFile("Phive/Shape/Dcc/Dm_SpiritualWorld_A_01__Physical.Nin_NX_NVN.bphsh.zs"), ZStdFile::Dictionary::Base).Data);

	Actor FloorActor = ActorMgr::AddActor("DgnObj_Small_BoxFloor_A_05");

	std::vector<float> Vertices;
	for (std::vector<float> Vertex : FloorActor.Model->GetModels()[0].Vertices)
	{
		for (float f : Vertex)
			Vertices.push_back(f);
	}

	std::vector<unsigned int> Indices;
	uint32_t FaceIndex = 0;
	for (std::vector<unsigned int> Index : FloorActor.Model->GetModels()[0].LODs[0].Faces)
	{
		for (unsigned int f : Index)
		{
			if (FaceIndex > 0)
			{
				f += FloorActor.Model->GetModels()[0].Vertices[FaceIndex - 1].size() / 3;
			}
			Indices.push_back(f);
		}

		FaceIndex++;
	}

	//Shape.GetMeshShape().PrimitiveMapping = Shape2.GetMeshShape().PrimitiveMapping;

	//Shape.WriteToFile("MapEditorV4/PhiveWorkspace/Test.bphsh", Vertices, Indices);
	Shape.WriteToFile("MapEditorV4/PhiveWorkspace/Test.bphsh", Shape.ToVertices(), Shape.ToIndices());

	{
		std::vector<float> Vertices = Shape.ToVertices();
		std::vector<unsigned int> Indices = Shape.ToIndices();

		BfresFile DefaultModel;
		DefaultModel.IsDefaultModel() = true;

		std::vector<float> TexCoords(Vertices.size() / 3 * 2);

		BfresFile::Model Model;
		BfresFile::LOD LODModel;
		LODModel.Faces.push_back(Indices);
		Model.Vertices.push_back(Vertices);

		BfresFile::Material Material;
		Material.Name = "Mt_DefaultModel";

		BfresFile::BfresTexture Texture;
		Texture.TexCoordinates = TexCoords;

		TextureToGo TexToGo;
		TexToGo.IsFail() = false;
		TexToGo.GetPixels() = { 0, 255, 0, 255 };
		TexToGo.GetWidth() = 1;
		TexToGo.GetHeight() = 1;
		if (!TextureToGoLibrary::IsTextureLoaded("CollisionActorTest"))
			TextureToGoLibrary::Textures.insert({ "CollisionActorTest", TexToGo});

		Texture.Texture = TextureToGoLibrary::GetTexture("CollisionActorTest");

		Material.Textures.push_back(Texture);
		Model.Materials.push_back(Material);
		Model.LODs.push_back(LODModel);
		DefaultModel.GetModels().push_back(Model);

		DefaultModel.CreateOpenGLObjects();

		BfresLibrary::Models.insert({ "CollisionActorTest", DefaultModel });
	}

	Actor NewActor;
	NewActor.Gyml = "CollisionActorTest";
	NewActor.Model = BfresLibrary::GetModel("CollisionActorTest");

	ActorMgr::AddActor(NewActor);
	*/

	//ActorMgr::AddActor("Obj_ShieldFenceWood_A_M_03");
	//ActorMgr::AddActor("DgnObj_Hrl_GoalSystem_B_01");
	//ActorMgr::AddActor("TBox_Dungeon_Stone");
	//ActorMgr::AddActor("DgnObj_ZonauShrineEntrance_A_01");
	//ActorMgr::AddActor("adasd"); //Nice name, right?

/*
	for (AINBNodeDefMgr::NodeDef& Def : AINBNodeDefMgr::NodeDefinitions)
	{
		for (int Type = 0; Type < AINBFile::ValueTypeCount; Type++)
		{
			for (AINBNodeDefMgr::NodeDef::InputParam Param : Def.InputParameters[Type])
			{
				if (Param.Class == "game::actor::ActorLinkBase")
				{
					std::cout << "Node with game::actor::ActorLinkBase as input: " << Def.Name << std::endl;
					goto NextNode;
				}
			}

			for (AINBFile::OutputEntry Param : Def.OutputParameters[Type])
			{
				if (Param.Class == "game::actor::ActorLinkBase")
				{
					std::cout << "Node with game::actor::ActorLinkBase as output: " << Def.Name << std::endl;
					goto NextNode;
				}
			}
		}
	NextNode:;
	}
	*/
	

	while (!UI::ShouldWindowClose())
	{
		UI::Render();
	}

	UI::Cleanup();
}