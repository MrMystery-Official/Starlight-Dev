#pragma once

#include <game/actor_component/ActorComponentBase.h>
#include <game/ActorPack.h>
#include <optional>

namespace application::game::actor_component
{
	struct ActorComponentModelInfo : public ActorComponentBase
	{
		application::game::ActorPack* mActorPack = nullptr;

		std::optional<std::string> mModelProjectName;
		std::optional<std::string> mFmdbName;
		std::optional<bool> mEnableModelBake = false;

		ActorComponentModelInfo(application::game::ActorPack& ActorPack);

		static const std::string GetDisplayNameImpl();
		static const std::string GetInternalNameImpl();
		static bool ContainsComponent(application::game::ActorPack& Pack);

		static const ComponentType GetComponentTypeImpl()
		{
			return ActorComponentBase::ComponentType::MODEL_INFO;
		}

		virtual const ComponentType GetComponentType() const
		{
			return ActorComponentModelInfo::GetComponentTypeImpl();
		}

		virtual const std::string GetDisplayName() const
		{
			return ActorComponentModelInfo::GetDisplayNameImpl();
		}

		virtual const std::string GetInternalName() const
		{
			return ActorComponentModelInfo::GetInternalNameImpl();
		}

		virtual void DrawEditingMenu() override;
	};
}