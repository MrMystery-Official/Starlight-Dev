#include "ActorPackMgr.h"

#include <util/FileUtil.h>
#include <util/General.h>
#include <manager/ProjectMgr.h>
#include <manager/ActorInfoMgr.h>

namespace application::manager
{
	std::unordered_map<std::string, application::game::ActorPack> ActorPackMgr::gActorPacks;
	std::vector<std::string> ActorPackMgr::gActorList;
	std::vector<std::string> ActorPackMgr::gActorListLowerCase;

	application::game::ActorPack* ActorPackMgr::GetActorPack(const std::string& Gyml)
	{
		if (!gActorPacks.contains(Gyml))
		{
			if (!application::util::FileUtil::FileExists(application::util::FileUtil::GetRomFSFilePath("Pack/Actor/" + Gyml + ".pack.zs")))
			{
				//application::util::Logger::Warning("ActorPackMgr", "Got invalid actor pack name \"%s\", returning nullptr", Gyml.c_str());
				return nullptr;
			}

			gActorPacks.emplace(Gyml, application::util::FileUtil::GetRomFSFilePath("Pack/Actor/" + Gyml + ".pack.zs"));

			if (application::manager::ActorInfoMgr::ActorInfoEntry* Entry = application::manager::ActorInfoMgr::GetActorInfo(Gyml); Entry != nullptr)
			{
				gActorPacks[Gyml].mNeedsPhysicsHash |= Entry->mCategory == application::manager::ActorInfoMgr::ActorInfoEntry::Category::Animal ||
					Entry->mCategory == application::manager::ActorInfoMgr::ActorInfoEntry::Category::NPC ||
					Entry->mCategory == application::manager::ActorInfoMgr::ActorInfoEntry::Category::EventNPC ||
					Entry->mCategory == application::manager::ActorInfoMgr::ActorInfoEntry::Category::Weapon ||
					Entry->mCategory == application::manager::ActorInfoMgr::ActorInfoEntry::Category::Armor ||
					Entry->mCategory == application::manager::ActorInfoMgr::ActorInfoEntry::Category::Bow ||
					Entry->mCategory == application::manager::ActorInfoMgr::ActorInfoEntry::Category::Enemy ||
					Entry->mCategory == application::manager::ActorInfoMgr::ActorInfoEntry::Category::Player;
			}

			gActorPacks[Gyml].mName = Gyml;
		}
		

		return &gActorPacks[Gyml];
	}

	void ActorPackMgr::LoadActorList()
	{
		gActorList.clear();
		gActorListLowerCase.clear();

		for (const auto& DirEntry : std::filesystem::directory_iterator(application::util::FileUtil::GetRomFSFilePath("Pack/Actor", false)))
		{
			std::string Name = DirEntry.path().stem().string();
			application::util::General::ReplaceString(Name, ".pack", "");
			gActorList.push_back(Name);

			std::transform(Name.begin(), Name.end(), Name.begin(),
				[](unsigned char c) { return std::tolower(c); });
			gActorListLowerCase.push_back(Name);
		}

		if (application::manager::ProjectMgr::IsAnyProjectSelected() && application::util::FileUtil::FileExists(application::util::FileUtil::GetSaveFilePath("Pack/Actor")))
		{
			for (const auto& DirEntry : std::filesystem::directory_iterator(application::util::FileUtil::GetSaveFilePath("Pack/Actor")))
			{
				std::string Name = DirEntry.path().stem().string();
				application::util::General::ReplaceString(Name, ".pack", "");

				if (std::find(gActorList.begin(), gActorList.end(), Name) != gActorList.end())
					continue;

				gActorList.push_back(Name);

				std::transform(Name.begin(), Name.end(), Name.begin(),
					[](unsigned char c) { return std::tolower(c); });
				gActorListLowerCase.push_back(Name);
			}
		}
	}
}