#include "TexToGoFileMgr.h"

#include <util/FileUtil.h>
#include <util/Logger.h>
#include <filesystem>

namespace application::manager
{
	std::unordered_map<std::string, application::file::game::texture::TexToGoFile> TexToGoFileMgr::gTexToGoFiles;

	bool TexToGoFileMgr::IsTextureLoaded(const std::string& Name, AccesMode Mode)
	{
		return gTexToGoFiles.contains(Mode == AccesMode::NAME ? Name : std::filesystem::path(Name).filename().string());
	}

	application::file::game::texture::TexToGoFile* TexToGoFileMgr::GetTexture(const std::string& Name, AccesMode Mode)
	{
		std::string AccessKey = Mode == AccesMode::NAME ? Name : std::filesystem::path(Name).filename().string();
		if (!gTexToGoFiles.contains(AccessKey))
		{
			application::file::game::texture::TexToGoFile TexToGo;
			TexToGo.GetFileName() = AccessKey;
			if (!TexToGo.IsCached())
			{
				TexToGo.SetData(application::util::FileUtil::ReadFile(Mode == AccesMode::NAME ? application::util::FileUtil::GetRomFSFilePath("TexToGo/" + Name) : Name));
				TexToGo.Parse();
			}
			gTexToGoFiles.insert({ AccessKey, TexToGo });
		}
		return &gTexToGoFiles[AccessKey];
	}
}