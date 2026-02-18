#include "CaveMgr.h"

#include <util/FileUtil.h>
#include <filesystem>

namespace application::manager
{
	std::unordered_map<std::string, CaveMgr::Cave> CaveMgr::gCaveFiles;

	CaveMgr::Cave* CaveMgr::GetCave(const std::string& Name)
	{
		if (!gCaveFiles.contains(Name))
		{
			gCaveFiles[Name].mFile.Initialize(application::util::FileUtil::ReadFile(application::util::FileUtil::GetRomFSFilePath("Cave/cave017/" + Name + "/C.crbin")));
			
			for (const auto& Entry : std::filesystem::directory_iterator(application::util::FileUtil::GetRomFSFilePath("Cave/cave017/" + Name)))
			{
				if (Entry.is_directory())
				{
					gCaveFiles[Name].mFile.LoadChunkFiles(Entry.path().string());
					break;
				}
			}

			gCaveFiles[Name].mRenderer.Initialize(gCaveFiles[Name].mFile);
		}

		return &gCaveFiles[Name];
	}
}