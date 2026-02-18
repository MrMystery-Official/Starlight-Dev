#include "BfresFileMgr.h"

namespace application::manager
{
	std::unordered_map<std::string, application::file::game::bfres::BfresFile> BfresFileMgr::gBfresFiles;

	application::file::game::bfres::BfresFile* BfresFileMgr::GetBfresFile(const std::string& AbsolutePath)
	{
		if (!gBfresFiles.contains(AbsolutePath))
			gBfresFiles.emplace(AbsolutePath, AbsolutePath);

		return &gBfresFiles[AbsolutePath];
	}
}