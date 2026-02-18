#include "MergedActorMgr.h"

#include <file/game/byml/BymlFile.h>
#include <file/game/zstd/ZStdBackend.h>
#include <util/FileUtil.h>
#include <game/BancEntity.h>
#include <util/Logger.h>

namespace application::manager
{
	std::unordered_map<std::string, std::vector<application::game::BancEntity>> MergedActorMgr::gMergedActors;

	std::vector<application::game::BancEntity>* MergedActorMgr::GetMergedActor(const std::string& RomFSPath, bool AllowNew)
	{
		if (gMergedActors.contains(RomFSPath))
			return &gMergedActors[RomFSPath];

		const std::string AbsolutePath = application::util::FileUtil::GetRomFSFilePath(RomFSPath + ".zs");

		if (!application::util::FileUtil::FileExists(AbsolutePath))
		{
			if (AllowNew)
			{
				gMergedActors.insert({ RomFSPath, std::vector<application::game::BancEntity>() });
				return &gMergedActors[RomFSPath];
			}
			return nullptr;
		}

		application::file::game::byml::BymlFile File(application::file::game::ZStdBackend::Decompress(AbsolutePath));
		for (application::file::game::byml::BymlFile::Node& ActorNode : File.GetNode("Actors")->GetChildren())
		{
			application::game::BancEntity Entity;
			if (!Entity.FromByml(ActorNode))
			{
				application::util::Logger::Error("MergedActorMgr", "Error while parsing child BancEntity");
				continue;
			}
			Entity.mBancType = application::game::BancEntity::BancType::MERGED;
			gMergedActors[RomFSPath].push_back(Entity);
		}

		application::util::Logger::Info("MergedActorMgr", "Loaded merged actor: %s", RomFSPath.c_str());
		return &gMergedActors[RomFSPath];
	}
	
	void MergedActorMgr::WriteMergedActor(const std::string& BancPath)
	{
		if (!gMergedActors.contains(BancPath))
		{
			application::util::Logger::Warning("MergedActorMgr", "No BancEntity ptr array found for %s", BancPath.c_str());
			return;
		}

		application::file::game::byml::BymlFile File;
		File.GetType() = application::file::game::byml::BymlFile::Type::Dictionary;

		application::file::game::byml::BymlFile::Node ActorsNode(application::file::game::byml::BymlFile::Type::Array, "Actors");

		for (application::game::BancEntity& Entity : gMergedActors[BancPath])
		{
			if (Entity.mBancType != application::game::BancEntity::BancType::MERGED)
			{
				application::util::Logger::Error("MergedActorMgr", "Merged BancEntity with hash %lu at %.3f %.3f %.3f has an invalid banc type, expected MERGED", Entity.mHash, Entity.mTranslate.x, Entity.mTranslate.y, Entity.mTranslate.z);
				continue;
			}

			ActorsNode.AddChild(Entity.ToByml());
		}

		File.GetNodes().push_back(ActorsNode);

		std::filesystem::create_directories(std::filesystem::path(application::util::FileUtil::GetSaveFilePath(BancPath + ".zs")).parent_path().string());
		application::util::FileUtil::WriteFile(application::util::FileUtil::GetSaveFilePath(BancPath + ".zs"), application::file::game::ZStdBackend::Compress(File.ToBinary(application::file::game::byml::BymlFile::TableGeneration::AUTO), application::file::game::ZStdBackend::Dictionary::BcettByaml));
	}

	bool MergedActorMgr::DoesMergedActorExist(const std::string& BancPath)
	{
		const std::string AbsolutePath = application::util::FileUtil::GetRomFSFilePath(BancPath + ".zs");

		return application::util::FileUtil::FileExists(AbsolutePath);
	}
}