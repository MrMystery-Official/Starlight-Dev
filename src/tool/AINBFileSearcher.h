#pragma once

#include <file/game/ainb/AINBFile.h>
#include <functional>

namespace application::tool
{
	struct AINBFileSearcher
	{
		void Start(const std::function<bool(application::file::game::ainb::AINBFile& File)>& Func);
	};
}