#pragma once

#include <string>

namespace application::file::tool
{
	namespace LicenseFile
	{
		extern bool gNoLicenseFileFound;
		extern std::string gLicenseSeed;

		bool Load(const std::string& Path);
		void Save(const std::string& Path);
	}
}