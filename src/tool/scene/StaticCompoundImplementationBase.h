#pragma once

#include <game/BancEntity.h>
#include <gl/Camera.h>

namespace application::tool::scene
{
	class StaticCompoundImplementationBase
	{
	public:
		virtual bool Initialize();
		virtual bool Save();
		virtual void SyncBancEntityHashes(application::game::BancEntity& SceneEntity, uint64_t OldHash, uint64_t NewHash);
		
		virtual void DrawStaticCompound(application::gl::Camera& Camera);

		virtual void OnBancEntityDelete(void* RenderInfo);
		virtual void CleanupCollision(void* Scene);
		virtual void CleanupCollisionVolume(void* Scene);

		virtual uint64_t RequestUniqueHash();
		virtual uint32_t RequestUniqueSRTHash();

		virtual ~StaticCompoundImplementationBase() = default;

		bool mGeneratePhysicsHashes = true;
	};
}