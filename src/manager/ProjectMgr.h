#pragma once

#include <string>
#include <vector>
#include <functional>

namespace application::manager
{
	namespace ProjectMgr
	{
		extern std::vector<std::string> gProjects;
		extern std::string gProject;
		extern std::string gExportProjectPath;
		extern bool gIsTrialOfTheChosenHeroProject;
		extern std::vector<std::function<void(const std::string&)>> gProjectChangeCallbacks;

		void Initialize();
		void AddProject(const std::string& Name);
		bool RemoveProject(const std::string& Name);

		void UnloadProject();
		void SelectProject(const std::string& Name);
		bool IsAnyProjectSelected();

		void ExportProject(bool GenerateRSTB);
	}
}