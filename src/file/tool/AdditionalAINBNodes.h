#pragma once

#include <manager/AINBNodeMgr.h>
#include <string>

namespace application::file::tool
{
	namespace AdditionalAINBNodes
	{
		extern std::vector<application::manager::AINBNodeMgr::NodeDef> gAdditionalNodeDefs;

		void Load(const std::string& Path);
		void Save(const std::string& Path);
	}
}