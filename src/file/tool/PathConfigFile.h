#pragma once

#include <string>

namespace application::file::tool
{
	namespace PathConfigFile
	{
		void Load(const std::string& Path);
		void Save(const std::string& Path);
	}
}