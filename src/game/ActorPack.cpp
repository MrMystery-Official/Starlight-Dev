#include "ActorPack.h"

#include <util/FileUtil.h>
#include <util/Math.h>
#include <util/Logger.h>
#include <util/General.h>
#include <file/game/zstd/ZStdBackend.h>
#include <file/game/byml/BymlFile.h>
#include <game/actor_component/ActorComponentFactory.h>

namespace application::game
{
	ActorPack::ActorPack(const std::string& Path)
	{
		Initialize(application::file::game::ZStdBackend::Decompress(Path));
	}

	ActorPack::ActorPack(std::vector<unsigned char> Bytes)
	{
		Initialize(Bytes);
	}

	void ActorPack::GetRigidBodyEntityParamBymlName(const std::string& ControllerPath)
	{
		if (mPack.HasEntry(ControllerPath))
		{
			application::file::game::byml::BymlFile PhiveControllerFile(mPack.GetEntry(ControllerPath).mBytes);

			if (PhiveControllerFile.HasChild("RigidBodyEntityNamePathAry"))
			{
				for (auto& Child : PhiveControllerFile.GetNode("RigidBodyEntityNamePathAry")->GetChildren())
				{
					std::string Name = Child.GetChild("Name")->GetValue<std::string>();
					if (Name.find("Physical") == std::string::npos && Name.find("Collision") == std::string::npos)
						continue;
					
					mRigidBodyEntityParamBymlName = Child.GetChild("FilePath")->GetValue<std::string>();
				}
			}

			if (PhiveControllerFile.HasChild("$parent") && mRigidBodyEntityParamBymlName.empty())
			{
				std::string ParentControllerPath = PhiveControllerFile.GetNode("$parent")->GetValue<std::string>();
				application::util::General::ReplaceString(ParentControllerPath, "Work/", "");
				application::util::General::ReplaceString(ParentControllerPath, ".gyml", ".bgyml");
				if (mPack.HasEntry(ParentControllerPath))
				{
					GetRigidBodyEntityParamBymlName(ParentControllerPath);
				}
			}
		}
	}

	void ActorPack::GetRigidBodyMotionType(const std::string& RootPath)
	{
		if (mPack.HasEntry(RootPath))
		{
			application::file::game::byml::BymlFile RigidBodyParamFile(mPack.GetEntry(RootPath).mBytes);

			if (RigidBodyParamFile.HasChild("MotionType") && !mCalculatedNeedsPhysicsHash)
			{
				std::string Val = RigidBodyParamFile.GetNode("MotionType")->GetValue<std::string>();
				mNeedsPhysicsHash = Val.find("Dynamic") != std::string::npos;
				mCalculatedNeedsPhysicsHash = true;
			}

			if (RigidBodyParamFile.HasChild("Mass") && mMass == 0.0f)
			{
				mMass = RigidBodyParamFile.GetNode("Mass")->GetValue<float>();
			}

			if (RigidBodyParamFile.HasChild("$parent"))
			{
				std::string ParentControllerPath = RigidBodyParamFile.GetNode("$parent")->GetValue<std::string>();
				application::util::General::ReplaceString(ParentControllerPath, "Work/", "");
				application::util::General::ReplaceString(ParentControllerPath, ".gyml", ".bgyml");
				if (mPack.HasEntry(ParentControllerPath))
				{
					GetRigidBodyMotionType(ParentControllerPath);
				}
			}
		}
	}

	application::game::actor_component::ActorComponentBase* ActorPack::GetComponent(application::game::actor_component::ActorComponentBase::ComponentType Type)
	{
		for (std::unique_ptr<application::game::actor_component::ActorComponentBase>& Component : mComponents)
		{
			if (Component->GetComponentType() == Type)
			{
				return Component.get();
			}
		}

		return nullptr;
	}

	void ActorPack::Initialize(std::vector<unsigned char> Bytes)
	{
		if (Bytes.empty())
		{
			application::util::Logger::Error("ActorPack", "Sarc compression invalid, data.size() == 0");
			return;
		}

		mPack.Initialize(Bytes);


		if (std::unique_ptr<application::game::actor_component::ActorComponentBase> ModelInfoComponent = std::move(application::game::actor_component::ActorComponentFactory::MakeActorComponent(application::game::actor_component::ActorComponentBase::ComponentType::MODEL_INFO, *this)); ModelInfoComponent != nullptr)
		{
			mComponents.push_back(std::move(ModelInfoComponent));
		}

		if (std::unique_ptr<application::game::actor_component::ActorComponentBase> ShapeParamComponent = std::move(application::game::actor_component::ActorComponentFactory::MakeActorComponent(application::game::actor_component::ActorComponentBase::ComponentType::SHAPE_PARAM, *this)); ShapeParamComponent != nullptr)
		{
			mComponents.push_back(std::move(ShapeParamComponent));
		}

		std::string PhysicsComponentPath = "";
		std::string CaveComponentPath = "";
		for (application::file::game::SarcFile::Entry& Entry : mPack.GetEntries())
		{
			if (Entry.mName.starts_with("Component/Physics/"))
			{
				PhysicsComponentPath = Entry.mName;
			}
			if (Entry.mName.starts_with("Component/CaveParam/"))
			{
				CaveComponentPath = Entry.mName;
			}
		}

		//Some fucking physics stuff
		{
			if (PhysicsComponentPath.empty())
			{
				//application::util::Logger::Warning("ActorPack", "Physics component missing");
				goto PhysicsComponentSkip;
			}

			application::file::game::byml::BymlFile PhysicsParamFile(mPack.GetEntry(PhysicsComponentPath).mBytes);
			if (PhysicsParamFile.HasChild("ControllerSetPath"))
			{
				std::string PhiveControllerPath = PhysicsParamFile.GetNode("ControllerSetPath")->GetValue<std::string>();
				application::util::General::ReplaceString(PhiveControllerPath, "Work/", "");
				application::util::General::ReplaceString(PhiveControllerPath, ".gyml", ".bgyml");

				GetRigidBodyEntityParamBymlName(PhiveControllerPath);
				if (!mRigidBodyEntityParamBymlName.empty())
				{
					std::filesystem::path p(mRigidBodyEntityParamBymlName);
					mRigidBodyEntityParamBymlName = p.stem().string();

					GetRigidBodyMotionType("Phive/RigidBodyEntityParam/" + mRigidBodyEntityParamBymlName + ".bgyml");
				}
			}
		}
	PhysicsComponentSkip:

		//The cave param shit
		{
			if (CaveComponentPath.empty())
			{
				goto CaveComponentSkip;
			}

			application::file::game::byml::BymlFile CaveParamFile(mPack.GetEntry(CaveComponentPath).mBytes);
			if (CaveParamFile.HasChild("CavePath"))
			{
				std::string PhiveControllerPath = CaveParamFile.GetNode("CavePath")->GetValue<std::string>();
				mCaveName = std::filesystem::path(PhiveControllerPath).stem().string();

				//mCave = application::manager::CaveMgr::GetCave(mCaveName);

				application::util::Logger::Info("ActorPack", "Found cave " + mCaveName);
			}
		}

	CaveComponentSkip:;
		/*
		{
			mName = ModelInfoPath;
			application::util::General::ReplaceString(mName, "Component/ModelInfo/", "");
			application::util::General::ReplaceString(mName, ".engine__component__ModelInfo.bgyml", "");
		}
		*/
	}
}