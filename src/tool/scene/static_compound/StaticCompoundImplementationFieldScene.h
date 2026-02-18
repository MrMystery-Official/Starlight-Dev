#pragma once

#include <game/Scene.h>
#include <tool/scene/StaticCompoundImplementationBase.h>
#include <file/game/phive/PhiveStaticCompoundFile.h>
#include <string>
#include <unordered_set>
#include <gl/SimpleMesh.h>
#include <gl/Shader.h>

namespace application::tool::scene::static_compound
{
	class StaticCompoundImplementationFieldScene : public application::tool::scene::StaticCompoundImplementationBase
	{
	public:
		virtual bool Initialize() override;
		virtual bool Save() override;
		virtual void DrawStaticCompound(application::gl::Camera& Camera) override;
		virtual uint64_t RequestUniqueHash() override;
		virtual uint32_t RequestUniqueSRTHash() override;
		virtual void OnBancEntityDelete(void* RenderInfo) override;
		virtual void CleanupCollision(void* Scene) override;

		void LoadHashes();
		void SyncAiGroupHashes(const uint64_t& OldHash, const uint64_t& NewHash);
		application::file::game::phive::PhiveStaticCompoundFile* GetStaticCompoundForPos(glm::vec3 Pos);
		application::file::game::phive::PhiveStaticCompoundFile* GetStaticCompoundAtIndex(const uint8_t& X, const uint8_t& Z);
		glm::vec3 GetStaticCompoundMiddlePoint(uint8_t X, uint8_t Z);
		void ReloadMeshes();

		StaticCompoundImplementationFieldScene(application::game::Scene* Scene, const std::string& Path);

		std::vector<bool> mModified;
		std::vector<application::file::game::phive::PhiveStaticCompoundFile> mStaticCompoundFiles;

	private:
		application::game::Scene* mScene;
		std::string mPath = "";


		application::gl::Shader* mShader = nullptr;
		std::vector<application::gl::SimpleMesh> mMeshes;

		uint64_t mSmallestHash = 0;
		uint32_t mSmallestSRTHash = 0;
		//Those two are not kept in sync by the scene itself, only when the this implementation is called
		std::unordered_set<uint64_t> mSceneHashes;
		std::unordered_set<uint32_t> mSceneSRTHashes;
	};
}