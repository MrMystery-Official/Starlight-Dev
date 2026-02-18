#include "MateArchive.h"

#include <file/game/sarc/SarcFile.h>
#include <file/game/zstd/ZStdBackend.h>
#include <util/FileUtil.h>
#include <util/General.h>

namespace application::file::game::terrain
{
	void MateArchive::Initialize(const std::string& Path, application::file::game::terrain::TerrainSceneFile* SceneFile)
	{
		application::file::game::SarcFile Archive(application::file::game::ZStdBackend::Decompress(application::util::FileUtil::ReadFile(Path)));

		for (application::file::game::SarcFile::Entry& File : Archive.GetEntries())
		{
			std::string Key = File.mName;
			application::util::General::ReplaceString(Key, ".mate", "");

			mMateFiles.insert({ Key, application::file::game::terrain::MateFile(File.mBytes, SceneFile) });
		}
	}

	std::vector<unsigned char> MateArchive::ToBinary()
	{
		application::file::game::SarcFile Pack;

		for (auto& [Name, File] : mMateFiles)
		{
			application::file::game::SarcFile::Entry Entry;
			Entry.mName = Name + ".mate";
			Entry.mBytes = File.ToBinary();
			File.mModified = false;
			Pack.GetEntries().push_back(Entry);
		}

		return Pack.ToBinary();
	}
}