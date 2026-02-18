#pragma once

#include <unordered_map>
#include <string>
#include <file/game/cave/CrbinFile.h>
#include <gl/CaveRenderer.h>

namespace application::manager
{
	namespace CaveMgr
	{
		struct Cave
		{
			application::file::game::cave::CrbinFile mFile;
			application::gl::CaveRenderer mRenderer;
		};
		extern std::unordered_map<std::string, Cave> gCaveFiles;

		Cave* GetCave(const std::string& Name);
	}
}