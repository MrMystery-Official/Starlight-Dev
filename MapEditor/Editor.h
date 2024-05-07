#pragma once

#include <string>
#include "Byml.h"

namespace Editor
{
	extern std::string WorkingDir;
	extern std::string RomFSDir;
	extern std::string BfresDir;
	extern std::string ExportDir;
	extern BymlFile StaticActorsByml;
	extern BymlFile DynamicActorsByml;
	extern std::string BancDir;
	extern std::string BancPrefix;
	extern std::string Identifier;
	extern std::string InternalGameVersion;
	//extern std::map<uint32_t, BymlFile> MergedActorBymls;

	void DetectInternalGameVersion();

	std::string GetRomFSFile(std::string LocalPath, bool Replaceable = true);
	bool RomFSFileExists(std::string LocalPath);
	std::string GetWorkingDirFile(std::string File);
	std::string GetBfresFile(std::string Name);
	std::string GetInternalGameVersion();

	//If any paths are updated or the program starts, call this function to initialize ZStd, ActorTool, ...
	void InitializeWithEdtc();
}