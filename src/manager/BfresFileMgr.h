#pragma once

#include <unordered_map>
#include <string>
#include <file/game/bfres/BfresFile.h>

namespace application::manager
{
	namespace BfresFileMgr
	{
		extern std::unordered_map<std::string, application::file::game::bfres::BfresFile> gBfresFiles;

		application::file::game::bfres::BfresFile* GetBfresFile(const std::string& AbsolutePath);
	}
}