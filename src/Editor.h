#pragma once

#include <string>

namespace application
{
	namespace Editor
	{
		extern bool gInitializedStage2;
		extern std::string gInternalGameVersion;
		extern std::string gInternalGameDataListVersion;

		bool Initialize();
		void InitializeDefaultModels();
		bool InitializeRomFSPathDependant();
		void Shutdown();
	}
}