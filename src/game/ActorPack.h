#pragma once

#include <string>
#include <vector>
#include <file/game/sarc/SarcFile.h>
#include <game/actor_component/ActorComponentBase.h>
#include <manager/CaveMgr.h>
#include <memory>
#include <optional>
#include <glm/vec3.hpp>

namespace application::game
{
	class ActorPack
	{
	public:
		ActorPack() = default;
		ActorPack(const std::string& Path);
		ActorPack(std::vector<unsigned char> Bytes);

		void Initialize(std::vector<unsigned char> Bytes);
		
		application::file::game::SarcFile mPack;

		std::vector<std::unique_ptr<application::game::actor_component::ActorComponentBase>> mComponents;
		
		application::game::actor_component::ActorComponentBase* GetComponent(application::game::actor_component::ActorComponentBase::ComponentType Type);

		bool mNeedsPhysicsHash = false;
		bool mCalculatedNeedsPhysicsHash = false;
		std::string mName = "";
		std::string mRigidBodyEntityParamBymlName = "";
		std::string mCaveName = "";

		float mMass = 0.0f;

		application::manager::CaveMgr::Cave* mCave = nullptr;
	private:
		void GetRigidBodyEntityParamBymlName(const std::string& ControllerPath);
		void GetRigidBodyMotionType(const std::string& RootPath);
	};
}