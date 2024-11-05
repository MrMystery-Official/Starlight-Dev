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
#include "PhiveSocketConnector.h"
#include "PhiveShape2.h"
#include "SceneCreator.h"
#include "PhiveNavMesh.h"
#include "PhiveClassGenerator.h"
#include "PhiveStaticCompound.h"
#include "UIEventEditor.h"
#include "HGHT.h"
#include "EventNodeDefMgr.h"
#include "DynamicPropertyMgr.h"

#include <fstream>


void SearchForBytes()
{
	unsigned char BytesNum[] = "DmF_SY_SmallDungeon3Arrive.engine__event__EventSettingParam";

	std::vector<std::string> SkipDirectories = {};

	std::vector<std::string> Directories = { "Event", "Scene", "Banc" };

	using recursive_directory_iterator = std::filesystem::recursive_directory_iterator;
	for (const std::string& Dir : Directories)
	{
		for (const auto& dirEntry : recursive_directory_iterator(Editor::GetRomFSFile(Dir, false)))
		{
			bool SkipFile = false;
			for (const std::string& Skip : SkipDirectories)
			{
				if (dirEntry.path().string().starts_with(Editor::GetRomFSFile(Skip, false)))
				{
					SkipFile = true;
					break;
				}
			}
			if (SkipFile)
				continue;

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
						std::cerr << "Failed to open file " << dirEntry.path().string() << std::endl;
					}
				}

				if (Bytes.size() < (sizeof(BytesNum) / sizeof(unsigned char))) continue;

				// Check if the byte sequence is contained in the vector
				for (size_t i = 0; i <= Bytes.size() - (sizeof(BytesNum) / sizeof(unsigned char)); ++i) {
					if (memcmp(&Bytes[i], BytesNum, (sizeof(BytesNum) / sizeof(unsigned char))) == 0) {
						std::cout << "Found in file " << dirEntry.path().string() << std::endl;
						break;
					}
				}
			}
		}
	}
}

int main()
{
	Util::CreateDir(Editor::GetWorkingDirFile("Save"));
	Util::CreateDir(Editor::GetWorkingDirFile("Cache"));
	Util::CreateDir(Editor::GetWorkingDirFile("EditorModels"));

	//PhiveClassGenerator::Generate("H:/Paul/switchemulator/Zelda TotK/MapEditorV4/NavMeshWorkspace/havok_type_info.json", { "71041c2d00" });
	//return 0;

	//HGHTFile HeightMap("/switchemulator/Zelda TotK/MapEditorV4/TerrainWorkspace/5700001C8C.hght");
	//return 0;

	UI::Initialize();
	//BfresLibrary::Initialize();
	
	if (Util::FileExists(Editor::GetWorkingDirFile("Editor.edtc")))
	{
		Editor::InitializeWithEdtc();
	}
	UIAINBEditor::Initialize();
	UIEventEditor::Initialize();
	AINBNodeDefMgr::Initialize();
	DynamicPropertyMgr::Initialize();
	PhiveSocketConnector::Initialize();
	//DynamicPropertyMgr::Generate();
	//AINBNodeDefMgr::Generate();
	//EventNodeDefMgr::Initialize();
	//EventNodeDefMgr::Generate();

	if (Util::FileExists(Editor::GetWorkingDirFile("Preferences.eprf")))
	{
		PreferencesConfig::Load();
	}

	//SearchForBytes();

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

	//SceneMgr::LoadScene(SceneMgr::Type::SmallDungeon, "001");

	//ActorMgr::AddActor("DgnObj_Small_BoxPillarStart_A_01");

	/*
	ActorMgr::AddActor("Obj_TreeApple_A_L_01");
	ActorMgr::AddActor("Obj_TreeApple_A_L_01");
	ActorMgr::AddActor("Enemy_Drake");
	ActorMgr::AddActor("Enemy_Drake");
	*/

	//PhiveShape Shape(Util::ReadFile(Editor::GetWorkingDirFile("Moon5.bphsh")));
	//PhiveShape Shape(ZStdFile::Decompress(Editor::GetRomFSFile("Phive/Shape/Dcc/DgnObj_Small_BoxWall_A_21__Physical.Nin_NX_NVN.bphsh.zs"), ZStdFile::Dictionary::Base).Data);
	//PhiveShape Shape(ZStdFile::Decompress(Editor::GetRomFSFile("Phive/Shape/Dcc/DgnObj_Small_BoxFloor_2x2_A__Physical.Nin_NX_NVN.bphsh.zs"), ZStdFile::Dictionary::Base).Data);
	//PhiveShape Shape(ZStdFile::Decompress(Editor::GetRomFSFile("Phive/Shape/Dcc/FldObj_AncientPillar_A_01__Physical.Nin_NX_NVN.bphsh.zs"), ZStdFile::Dictionary::Base).Data);
	//PhiveShape Shape(ZStdFile::Decompress(Editor::GetWorkingDirFile("Save/Phive/Shape/Dcc/Test.bphsh.zs"), ZStdFile::Dictionary::Base).Data);
	//PhiveShape Shape(ZStdFile::Decompress(Editor::GetRomFSFile("Phive/Shape/Dcc/DgnObj_Water_Bucket_A_01__Physical.Nin_NX_NVN.bphsh.zs"), ZStdFile::Dictionary::Base).Data);

	/*
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
	*/

	//Shape.GetMeshShape().PrimitiveMapping = Shape2.GetMeshShape().PrimitiveMapping;

	//Shape.WriteToFile("MapEditorV4/PhiveWorkspace/Test.bphsh", Vertices, Indices);
	//Shape.WriteToFile("MapEditorV4/PhiveWorkspace/Test.bphsh", Shape.ToVertices(), Shape.ToIndices());

	/*
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

	//PhiveShape Shape = SplatoonShapeToTotK::Convert("switchemulator/Zelda TotK/MapEditorV4/OldVersion/MapEditor/WorkingDir/Moon5.bphsh.splat");
	//Shape.WriteToFile(Editor::GetWorkingDirFile("Moon5New.bphsh"));

/*
	PhiveShape2 Shape(Util::ReadFile(Editor::GetWorkingDirFile("Write.bphsh")));
	for (PhiveShape2::hknpMeshShapeGeometrySection& GeometrySection : Shape.GetMeshShape().m_GeometrySections.m_Elements)
	{
		//Updating data
		float DecodedX = GeometrySection.m_VertexBuffer.m_Elements[0].m_X * Shape.GetMeshShape().m_VertexConversionUtil.m_BitScale16Inv[0] + GeometrySection.m_BitOffset[0] * GeometrySection.m_BitScale8Inv[0];
		float DecodedY = GeometrySection.m_VertexBuffer.m_Elements[0].m_Y * Shape.GetMeshShape().m_VertexConversionUtil.m_BitScale16Inv[1] + GeometrySection.m_BitOffset[1] * GeometrySection.m_BitScale8Inv[1];
		float DecodedZ = GeometrySection.m_VertexBuffer.m_Elements[0].m_Z * Shape.GetMeshShape().m_VertexConversionUtil.m_BitScale16Inv[2] + GeometrySection.m_BitOffset[2] * GeometrySection.m_BitScale8Inv[2];

		GeometrySection.m_BitScale8Inv[0] = 0.169457f;
		GeometrySection.m_BitScale8Inv[1] = 0.169457f;
		GeometrySection.m_BitScale8Inv[2] = 0.169457f;

		GeometrySection.m_BitOffset[0] = (DecodedX - GeometrySection.m_VertexBuffer.m_Elements[0].m_X * Shape.GetMeshShape().m_VertexConversionUtil.m_BitScale16Inv[0]) / GeometrySection.m_BitScale8Inv[0];
		GeometrySection.m_BitOffset[1] = (DecodedY - GeometrySection.m_VertexBuffer.m_Elements[0].m_Y * Shape.GetMeshShape().m_VertexConversionUtil.m_BitScale16Inv[1]) / GeometrySection.m_BitScale8Inv[1];
		GeometrySection.m_BitOffset[2] = (DecodedZ - GeometrySection.m_VertexBuffer.m_Elements[0].m_Z * Shape.GetMeshShape().m_VertexConversionUtil.m_BitScale16Inv[2]) / GeometrySection.m_BitScale8Inv[2];

		GeometrySection.m_SectionOffset[0] = 0xFFFFD520;
		GeometrySection.m_SectionOffset[1] = 0xFFFFD520;
		GeometrySection.m_SectionOffset[2] = 0xFFFFD520;

		//Updating section bvh

		uint32_t SectionBvhCount = std::ceil(GeometrySection.m_Primitives.m_Elements.size() / 4.0f);
		GeometrySection.m_SectionBvh.m_Elements.resize(SectionBvhCount);
		for (uint32_t i = 0; i < SectionBvhCount; i++)
		{
			GeometrySection.m_SectionBvh.m_Elements[i].m_TransposedFourAabbs8.m_MinX = 0x01010101;
			GeometrySection.m_SectionBvh.m_Elements[i].m_TransposedFourAabbs8.m_MaxX = 0xFFFFFF02;
			GeometrySection.m_SectionBvh.m_Elements[i].m_TransposedFourAabbs8.m_MinY = 0xFE010101;
			GeometrySection.m_SectionBvh.m_Elements[i].m_TransposedFourAabbs8.m_MaxY = 0xFF02FFFF;
			GeometrySection.m_SectionBvh.m_Elements[i].m_TransposedFourAabbs8.m_MinZ = 0x0101FE01;
			GeometrySection.m_SectionBvh.m_Elements[i].m_TransposedFourAabbs8.m_MaxZ = 0xFFFFFFFF;

			GeometrySection.m_SectionBvh.m_Elements[i].m_Data[0] = i * 4;
			GeometrySection.m_SectionBvh.m_Elements[i].m_Data[1] = GeometrySection.m_Primitives.m_Elements.size() <= i * 4 + 1 ? i * 4 : i * 4 + 1;
			GeometrySection.m_SectionBvh.m_Elements[i].m_Data[2] = GeometrySection.m_Primitives.m_Elements.size() <= i * 4 + 2 ? i * 4 : i * 4 + 2;
			GeometrySection.m_SectionBvh.m_Elements[i].m_Data[3] = GeometrySection.m_Primitives.m_Elements.size() <= i * 4 + 3 ? i * 4 : i * 4 + 3;
		}
	}
	Shape.WriteToFile(Editor::GetWorkingDirFile("Write.bphsh"));
	*/

	/*
	PhiveNavMesh NavMesh(ZStdFile::Decompress(Editor::GetRomFSFile("Phive/StitchedNavMesh/MainField/X8_Z14.Nin_NX_NVN.bphnm.zs", false), ZStdFile::Dictionary::Base).Data);
	std::pair<std::vector<float>, std::vector<uint32_t>> ModelData = NavMesh.ToVerticesIndices();

	{
		std::vector<float> Vertices = ModelData.first;
		std::vector<unsigned int> Indices = ModelData.second;

		BfresFile DefaultModel;
		DefaultModel.IsDefaultModel() = false;

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
			TextureToGoLibrary::Textures.insert({ "CollisionActorTest", TexToGo });

		Texture.Texture = TextureToGoLibrary::GetTexture("CollisionActorTest");

		Material.Textures.push_back(Texture);
		Model.Materials.push_back(Material);
		Model.LODs.push_back(LODModel);
		DefaultModel.GetModels().push_back(Model);

		DefaultModel.CreateOpenGLObjects();

		BfresLibrary::Models.insert({ "CollisionActorTest", DefaultModel });
	}

	*/

	/*

	Actor NewActor;
	NewActor.Gyml = "CollisionActorTest";
	NewActor.Model = &BfresLibrary::Models["CollisionActorTest"];

	ActorMgr::AddActor(NewActor);
	*/

	//std::vector<unsigned char> NavMeshData = PhiveNavMesh::ProcessNavMesh(ZStdFile::Decompress(Editor::GetRomFSFile("Phive/NavMesh/Dungeon136.Nin_NX_NVN.bphnm.zs", false), ZStdFile::Dictionary::Base).Data, 10000, Vector3F(0, 0, 0));
	//Util::WriteFile(Editor::GetWorkingDirFile("Test.bphnm"), NavMeshData);
	//Util::CreateDir(Editor::GetWorkingDirFile("Save/Phive/NavMesh"));
	//Util::WriteFile(Editor::GetWorkingDirFile("Save/Phive/NavMesh/Dungeon152.Nin_NX_NVN.bphnm.zs"), ZStdFile::Compress(NavMeshData, ZStdFile::Dictionary::Base).Data);

	/*
	std::vector<uint32_t> ActorCount;

	for (int i = 0; i < 151; i++)
	{
		std::string Identifier = std::to_string(i);
		if (Identifier.length() == 1) Identifier = "00" + Identifier;
		if (Identifier.length() == 2) Identifier = "0" + Identifier;
		SceneMgr::LoadScene(SceneMgr::Type::SmallDungeon, Identifier);
		std::vector<unsigned char> StaticCompound = ZStdFile::Decompress(Editor::GetRomFSFile("Phive/StaticCompoundBody/SmallDungeon/Dungeon" + Identifier + ".Nin_NX_NVN.bphsc.zs"), ZStdFile::Dictionary::Base).Data;
		auto ContainsHash = [&StaticCompound](uint64_t Hash)
			{
				for (size_t i = 0; i <= StaticCompound.size() - sizeof(uint64_t); ++i) {
					if (memcmp(&StaticCompound[i], reinterpret_cast<const char*>(&Hash), sizeof(uint64_t)) == 0) {
						return true;
					}
				}
				return false;
			};
		uint32_t Count = 0;
		for (Actor& Child : ActorMgr::GetActors())
		{
			if(ContainsHash(Child.Hash)) Count++;
			for (Actor& Merged : Child.MergedActorContent)
			{
				if (ContainsHash(Merged.Hash)) Count++;
			}
		}
		ActorCount.push_back(Count);
	}
	for (int i = 0; i < ActorCount.size(); i++)
	{
		std::string Identifier = std::to_string(i);
		if (Identifier.length() == 1) Identifier = "00" + Identifier;
		if (Identifier.length() == 2) Identifier = "0" + Identifier;
		std::cout << "Dungeon" << Identifier << ": " << ActorCount[i] << std::endl;
	}
	*/

	/*
	std::vector<uint64_t> Hashes;

	std::vector<unsigned char> StaticCompound = ZStdFile::Decompress(Editor::GetRomFSFile("Phive/StaticCompoundBody/SmallDungeon/Dungeon045.Nin_NX_NVN.bphsc.zs"), ZStdFile::Dictionary::Base).Data;
	auto ContainsHash = [&StaticCompound](uint64_t Hash)
		{
			for (size_t i = 0; i <= StaticCompound.size() - sizeof(uint64_t); ++i) {
				if (memcmp(&StaticCompound[i], reinterpret_cast<const char*>(&Hash), sizeof(uint64_t)) == 0) {
					return true;
				}
			}
			return false;
		};

	SceneMgr::LoadScene(SceneMgr::Type::SmallDungeon, "045");
	for (Actor& Child : ActorMgr::GetActors())
	{
		if(ContainsHash(Child.Hash)) Hashes.push_back(Child.Hash);
		for (Actor& Merged : Child.MergedActorContent)
		{
			if(ContainsHash(Merged.Hash)) Hashes.push_back(Merged.Hash);
		}
	}
	for (uint64_t& Hash : Hashes)
	{
		std::cout << Hash << std::endl;
	}
	*/

	/*
	PhiveNavMesh NavMesh(ZStdFile::Decompress(Editor::GetRomFSFile("Phive/NavMesh/Dungeon136.Nin_NX_NVN.bphnm.zs", false), ZStdFile::Dictionary::Base).Data);
	std::pair<std::vector<float>, std::vector<uint32_t>> NavMeshModel = NavMesh.ToVerticesIndices();
	//Write model to .obj
	//WriteToObj(NavMeshModel, Editor::GetWorkingDirFile("NavMeshModel.obj"));
	//NavMesh.WriteToFile(Editor::GetWorkingDirFile("Save/Phive/NavMesh/Dungeon136_OriginalModel.Nin_NX_NVN.bphnm"));

	NavMesh.SetNavMeshModel(NavMeshModel.first, NavMeshModel.second);
	//NavMesh.SetNavMeshModel(Vertices, Indices);
	ZStdFile::Compress(NavMesh.ToBinary(), ZStdFile::Dictionary::Base).WriteToFile(Editor::GetWorkingDirFile("Save/Phive/NavMesh/Dungeon136.Nin_NX_NVN.bphnm.zs"));
	*/

	/*
	PhiveStaticCompound StaticCompound(ZStdFile::Decompress(Editor::GetRomFSFile("Phive/StaticCompoundBody/SmallDungeon/Dungeon152.Nin_NX_NVN.bphsc.zs", true), ZStdFile::Dictionary::Base).Data);
	StaticCompound.mActorHashMap.clear();
	StaticCompound.mActorLinks.clear();
	ZStdFile::Compress(StaticCompound.ToBinary(), ZStdFile::Dictionary::Base).WriteToFile(Editor::GetWorkingDirFile("Save/Phive/StaticCompoundBody/SmallDungeon/Dungeon152.Nin_NX_NVN.bphsc.zs"));
	*/

	//BFEVFLFile Event(ZStdFile::Decompress("/switchemulator/Zelda TotK/TrialsOfTheChosenHero/Events/SwordTrial_CameraFocus.bfevfl.zs", ZStdFile::Dictionary::Base).Data);

/*
	ActorMgr::AddActor("CaveEntrance_0f2134c2");

	PhiveShape2 Shape(ZStdFile::Decompress(Editor::GetRomFSFile("Phive/Shape/Dcc/CaveEntrance_0f2134c2.Nin_NX_NVN.bphsh.zs"), ZStdFile::Dictionary::Base).Data);
	//PhiveShape2 Shape(ZStdFile::Decompress(Editor::GetRomFSFile("Phive/Shape/Dcc/DgnObj_Small_BoxFloor_4x4_A__Physical.Nin_NX_NVN.bphsh.zs"), ZStdFile::Dictionary::Base).Data);
	{

		for (PhiveShape2::hknpMeshShapeGeometrySection& GeometrySection : Shape.GetMeshShape().m_GeometrySections.m_Elements)
		{
			Shape.DecodeTreeNode(GeometrySection, GeometrySection.m_SectionBvh.m_Elements[0], glm::vec3(1.0f / GeometrySection.m_BitScale8Inv[0],
				1.0f / GeometrySection.m_BitScale8Inv[1],
				1.0f / GeometrySection.m_BitScale8Inv[2]));
		}
	}
	*/

	/*
	PhiveShape2 Shape(Util::ReadFile(Editor::GetWorkingDirFile("PotCollisionGeneratedByTotK_Materials.bphsh")));
	{
		for (PhiveShape2::hknpMeshShapeGeometrySection& GeometrySection : Shape.GetMeshShape().m_GeometrySections.m_Elements)
		{
			glm::vec3 BitScale = glm::vec3(1.0f / GeometrySection.m_BitScale8Inv[0],
				1.0f / GeometrySection.m_BitScale8Inv[1],
				1.0f / GeometrySection.m_BitScale8Inv[2]);

			std::vector<PhiveShape2::QuadBVHNode> Nodes;
			for (PhiveShape2::hknpAabb8TreeNode& TreeNode : GeometrySection.m_SectionBvh.m_Elements)
			{
				Nodes.resize(Nodes.size() + 1);
				for (int i = 0; i < 4; i++)
				{
					//std::cout << "Reverse MinX: " << ((BVHTreeNodeMinX + (float)GeometrySection.m_BitOffset[0]) / BitScale.x) << ", MaxX:" << ((BVHTreeNodeMaxX + (float)GeometrySection.m_BitOffset[0]) / BitScale.x) << std::endl;;

					Nodes.back().mMin[i] = glm::vec3(((TreeNode.m_TransposedFourAabbs8.m_MinX[i] + (float)GeometrySection.m_BitOffset[0]) / BitScale.x),
						((TreeNode.m_TransposedFourAabbs8.m_MinY[i] + (float)GeometrySection.m_BitOffset[1]) / BitScale.y),
						((TreeNode.m_TransposedFourAabbs8.m_MinZ[i] + (float)GeometrySection.m_BitOffset[2]) / BitScale.z));

					Nodes.back().mMax[i] = glm::vec3(((TreeNode.m_TransposedFourAabbs8.m_MaxX[i] + (float)GeometrySection.m_BitOffset[0]) / BitScale.x),
						((TreeNode.m_TransposedFourAabbs8.m_MaxY[i] + (float)GeometrySection.m_BitOffset[1]) / BitScale.y),
						((TreeNode.m_TransposedFourAabbs8.m_MaxZ[i] + (float)GeometrySection.m_BitOffset[2]) / BitScale.z));
				}
				Nodes.back().mIsLeaf = TreeNode.m_Data[2] > TreeNode.m_Data[3];
			}

			for (size_t i = 0; i < GeometrySection.m_SectionBvh.m_Elements.size(); i++)
			{
				if (!Nodes[i].mIsLeaf)
				{
					for (int j = 0; j < 4; j++)
					{
						if (Nodes[i].mMax[j].x > Nodes[i].mMin[j].x && Nodes[i].mMax[j].y > Nodes[i].mMin[j].y && Nodes[i].mMax[j].z > Nodes[i].mMin[j].z)
							Nodes[i].mChildren[j] = &Nodes[GeometrySection.m_SectionBvh.m_Elements[i].m_Data[j]];
					}
				}
				else
				{
					for (int j = 0; j < 4; j++)
					{
						Nodes[i].mPrimitiveIndices[j] = GeometrySection.m_SectionBvh.m_Elements[i].m_Data[j];
					}
				}
			}
			std::cout << "BUILT BVH, INJECTING...\n";
			GeometrySection.InjectBVH(Nodes);
		}
		Util::WriteFile(Editor::GetWorkingDirFile("BVHInjection.bphsh"), Shape.ToBinary());
	}*/
	
	//Shape.GetMeshShape().m_GeometrySections.m_Elements[0].m_InteriorPrimitiveBitField.m_Elements.clear();
	//Util::WriteFile(Editor::GetWorkingDirFile("PhiveShapeRegen.bphsh"), Shape.ToBinary());

	{
		GLBfres* Model = GLBfresLibrary::GetModel(BfresLibrary::GetModel("DgnObj_Small_Warpin_B.DgnObj_Small_Warpin_B_02"));
		std::vector<glm::vec3> Vertices;
		std::vector<std::pair<std::tuple<uint32_t, uint32_t, uint32_t>, uint32_t>> Indices;
		uint32_t IndexOffset = 0;

		for (size_t SubModelIndex = 0; SubModelIndex < Model->mBfres->Models.GetByIndex(0).Value.Shapes.Nodes.size(); SubModelIndex++)
		{
			std::vector<glm::vec4> Positions = Model->mBfres->Models.GetByIndex(0).Value.Shapes.GetByIndex(SubModelIndex).Value.Vertices;
			uint32_t Offset = Vertices.size();
			Vertices.resize(Vertices.size() + Positions.size());

			for (size_t i = 0; i < Positions.size(); i++)
			{
				Vertices[Offset + i] = glm::vec3(Positions[i].x, Positions[i].y, Positions[i].z);
			}

			std::vector<uint32_t> Triangles = Model->mBfres->Models.GetByIndex(0).Value.Shapes.GetByIndex(SubModelIndex).Value.Meshes[0].GetIndices();
			Offset = Indices.size();
			Indices.resize(Indices.size() + (Triangles.size() / 3));
			for (size_t i = 0; i < Triangles.size() / 3; i++)
			{
				Indices[Offset + i] = std::make_pair(std::make_tuple(Triangles[i * 3] + IndexOffset, Triangles[i * 3 + 1] + IndexOffset, Triangles[i * 3 + 2] + IndexOffset), 0);
			}

			IndexOffset += Positions.size();
		}

		PhiveShape2 Shape(Util::ReadFile(Editor::GetWorkingDirFile("PotCollisionGeneratedByTotK_Materials.bphsh")));
		Shape.InjectModel(Vertices, Indices);
		std::vector<unsigned char> Bytes = Shape.ToBinary();
		Util::WriteFile(Editor::GetWorkingDirFile("LocalGenerator.bphsh"), Bytes);
		ZStdFile::Compress(Bytes, ZStdFile::Dictionary::Base).WriteToFile(Editor::GetWorkingDirFile("Save/Phive/Shape/Dcc/CollisionTest_DgnObj_Small_BoxFloor_A_07__Physical_Col.Nin_NX_NVN.bphsh.zs"));
	}

	while (!UI::ShouldWindowClose())
	{
		UI::Render();
	}

	UI::Cleanup();
}