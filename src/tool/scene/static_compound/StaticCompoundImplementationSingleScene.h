#pragma once

#include <game/Scene.h>
#include <tool/scene/StaticCompoundImplementationBase.h>
#include <file/game/phive/PhiveStaticCompoundFile.h>
#include <string>
#include <unordered_set>

namespace application::tool::scene::static_compound
{
	class StaticCompoundImplementationSingleScene : public application::tool::scene::StaticCompoundImplementationBase
	{
	public:
		virtual bool Initialize() override;
		virtual bool Save() override;
		virtual uint64_t RequestUniqueHash() override;
		virtual uint32_t RequestUniqueSRTHash() override;
		virtual void OnBancEntityDelete(void* RenderInfo) override;
		virtual void CleanupCollision(void* Scene) override;
		virtual void CleanupCollisionVolume(void* Scene) override;

		void LoadHashes();
		void SyncAiGroupHashes(const uint64_t& OldHash, const uint64_t& NewHash);
		void RemoveBakedCollision();

		StaticCompoundImplementationSingleScene(application::game::Scene* Scene, const std::string& Path);

	private:
		application::game::Scene* mScene;
		std::string mPath = "";
		application::file::game::phive::PhiveStaticCompoundFile mStaticCompoundFile;

		uint64_t mSmallestHash = 0;
		uint32_t mSmallestSRTHash = 0;
		//Those two are not kept in sync by the scene itself, only when the this implementation is called
		std::unordered_set<uint64_t> mSceneHashes;
		std::unordered_set<uint32_t> mSceneSRTHashes;
	};
}