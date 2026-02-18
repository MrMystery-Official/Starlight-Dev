#include "ProjectMgr.h"

#include <util/FileUtil.h>
#include <iostream>
#include <filesystem>
#include <manager/ActorPackMgr.h>
#include <manager/BfresFileMgr.h>
#include <manager/BfresRendererMgr.h>
#include <manager/MergedActorMgr.h>
#include <manager/TexToGoFileMgr.h>
#include <manager/TextureMgr.h>
#include <manager/AINBNodeMgr.h>
#include <tool/ResourceSizeTableGenerator.h>
#include <Editor.h>

namespace application::manager
{
	std::vector<std::string> ProjectMgr::gProjects;
	std::string ProjectMgr::gProject = "";
	std::string ProjectMgr::gExportProjectPath = "";
	std::vector<std::function<void(const std::string&)>> ProjectMgr::gProjectChangeCallbacks;
	bool ProjectMgr::gIsTrialOfTheChosenHeroProject = false;

	void ProjectMgr::Initialize()
	{
		for (const auto& Entry : std::filesystem::directory_iterator(application::util::FileUtil::GetWorkingDirFilePath("Projects")))
		{
			if (Entry.is_directory())
			{
				gProjects.push_back(Entry.path().filename().string());
			}
		}
	}

	void ProjectMgr::AddProject(const std::string& Name)
	{
		if (std::find(gProjects.begin(), gProjects.end(), Name) != gProjects.end())
			return;

		std::filesystem::create_directory(application::util::FileUtil::GetWorkingDirFilePath("Projects/" + Name));

		gProjects.push_back(Name);
		SelectProject(Name);

		for (auto& Func : gProjectChangeCallbacks)
		{
			Func(Name);
		}
	}

	void ProjectMgr::SelectProject(const std::string& Name)
	{
		if (std::find(gProjects.begin(), gProjects.end(), Name) == gProjects.end())
			return;


		//Resetting caches of romfs data
		application::manager::ActorPackMgr::gActorPacks.clear();
		application::manager::BfresRendererMgr::Cleanup();
		application::manager::BfresFileMgr::gBfresFiles.clear();
		application::manager::MergedActorMgr::gMergedActors.clear();

		for (auto& [Key, Tex] : application::manager::TextureMgr::gTexToGoSurfaceTextures)
		{
			Tex.Delete();
		}
		application::manager::TextureMgr::gTexToGoSurfaceTextures.clear();
		application::manager::TexToGoFileMgr::gTexToGoFiles.clear();


		gProject = Name;
		gIsTrialOfTheChosenHeroProject = std::filesystem::exists(application::util::FileUtil::GetRomFSFilePath("Banc/GameBancParam/TrialField.game__banc__GameBancParam.bgyml"));

		application::manager::ActorPackMgr::LoadActorList();
		application::Editor::InitializeDefaultModels();
		application::manager::AINBNodeMgr::ReloadAdditionalProjectNodes();

		for (auto& Func : gProjectChangeCallbacks)
		{
			Func(Name);
		}
	}

	bool ProjectMgr::RemoveProject(const std::string& Name)
	{
		if (!application::util::FileUtil::FileExists(application::util::FileUtil::GetWorkingDirFilePath("Projects/" + Name)))
			return false;

		std::filesystem::remove_all(application::util::FileUtil::GetWorkingDirFilePath("Projects/" + Name));

		if (application::util::FileUtil::FileExists(application::util::FileUtil::GetWorkingDirFilePath("Projects/" + Name)))
			return false;

		gProjects.erase(std::remove_if(gProjects.begin(), gProjects.end(), [&Name](const std::string& A)
			{
				return A == Name;
			}), gProjects.end());

		if (gProject == Name)
			UnloadProject();

		for (auto& Func : gProjectChangeCallbacks)
		{
			Func("");
		}

		return true;
	}

	bool ProjectMgr::IsAnyProjectSelected()
	{
		return !gProject.empty();
	}

	void ProjectMgr::UnloadProject()
	{
		gProject = "";
		for (auto& Func : gProjectChangeCallbacks)
		{
			Func("");
		}
	}

	void ProjectMgr::ExportProject(bool GenerateRSTB)
	{
		if (!IsAnyProjectSelected())
			return;

		if(GenerateRSTB)
			application::tool::ResourceSizeTableGenerator::Generate();

		application::util::FileUtil::CopyDirectoryRecursively(application::util::FileUtil::GetSaveFilePath(), gExportProjectPath);
	}
}