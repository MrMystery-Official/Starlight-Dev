#include "ProjectConfigFile.h"

#include <util/FileUtil.h>
#include <util/BinaryVectorReader.h>
#include <util/BinaryVectorWriter.h>
#include <util/Logger.h>
#include <manager/ProjectMgr.h>
#include <manager/AINBNodeMgr.h>

namespace application::file::tool
{
	void ProjectConfigFile::Load(const std::string& Path)
	{
		if (!application::util::FileUtil::FileExists(Path))
			return;

		application::util::BinaryVectorReader Reader(application::util::FileUtil::ReadFile(Path));

		char Magic[9];
		Reader.ReadStruct(&Magic[0], 8); //EPATHCFG
		Magic[8] = '\0';
		if (strcmp(Magic, "EPROJCFG") != 0)
		{
			application::util::Logger::Error("ProjectConfig", "File magic invalid, expected EPROJCFG");
			return;
		}

		uint8_t Version = Reader.ReadUInt8();
		if (Version != 0x01)
		{
			application::util::Logger::Error("ProjectConfig", "Version invalid, expected 0x01");
			return;
		}

		uint16_t ProjectNameSize = Reader.ReadUInt16();
		uint16_t ExportPathSize = Reader.ReadUInt16();

		application::manager::ProjectMgr::gProject.resize(ProjectNameSize);
		application::manager::ProjectMgr::gExportProjectPath.resize(ExportPathSize);

		Reader.ReadStruct(application::manager::ProjectMgr::gProject.data(), ProjectNameSize);
		Reader.ReadStruct(application::manager::ProjectMgr::gExportProjectPath.data(), ExportPathSize);

		if (std::find(application::manager::ProjectMgr::gProjects.begin(), application::manager::ProjectMgr::gProjects.end(), application::manager::ProjectMgr::gProject) != application::manager::ProjectMgr::gProjects.end())
		{
			application::manager::ProjectMgr::gIsTrialOfTheChosenHeroProject = application::util::FileUtil::FileExists(application::util::FileUtil::GetRomFSFilePath("Banc/GameBancParam/TrialField.game__banc__GameBancParam.bgyml"));
			application::manager::AINBNodeMgr::ReloadAdditionalProjectNodes();

			application::util::Logger::Info("ProjectConfig", "Loaded selected project");
		}
		else
		{
			application::util::Logger::Warning("ProjectConfig", "Invalid project name");
			application::manager::ProjectMgr::gProject = "";
		}
	}

	void ProjectConfigFile::Save(const std::string& Path)
	{
		if (application::manager::ProjectMgr::gProject.empty())
			return;

		application::util::BinaryVectorWriter Writer;

		Writer.WriteBytes("EPROJCFG"); //Magic
		Writer.WriteInteger(0x01, sizeof(uint8_t)); //Version

		Writer.WriteInteger(application::manager::ProjectMgr::gProject.size(), sizeof(uint16_t));
		Writer.WriteInteger(application::manager::ProjectMgr::gExportProjectPath.size(), sizeof(uint16_t));

		Writer.WriteBytes(application::manager::ProjectMgr::gProject.c_str());
		Writer.WriteBytes(application::manager::ProjectMgr::gExportProjectPath.c_str());

		application::util::FileUtil::WriteFile(Path, Writer.GetData());

		application::util::Logger::Info("ProjectConfig", "Saved selected project");
	}
}