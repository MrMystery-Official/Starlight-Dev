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

#include <fstream>

/*
void SearchForBytes()
{
	unsigned char BytesNum[] = "Exterminator";

	using recursive_directory_iterator = std::filesystem::recursive_directory_iterator;
	for (const auto& dirEntry : recursive_directory_iterator("/romfs/upd_v1.2.1"))
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
		EditorConfig::Load();
		Editor::DetectInternalGameVersion();
		ZStdFile::Initialize(Editor::GetRomFSFile("Pack/ZsDic.pack.zs"));
	}
	UIAINBEditor::Initialize();
	AINBNodeDefMgr::Initialize();

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

	while (!UI::ShouldWindowClose())
	{
		UI::Render();
	}

	UI::Cleanup();
}